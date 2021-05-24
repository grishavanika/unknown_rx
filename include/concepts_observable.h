#pragma once
#include "meta_utils.h"
#include <type_traits>
#include <concepts>

namespace xrx::detail
{
    template<typename Observable_>
        requires requires { typename Observable_::is_async; }
    constexpr auto detect_async() { return typename Observable_::is_async(); }
    template<typename Observable_>
        requires (not requires { typename Observable_::is_async; })
    constexpr auto detect_async() { return std::true_type(); }

    template<typename Observable_>
    using IsAsyncObservable = decltype(detect_async<Observable_>());

    template<typename Handle_>
    concept ConceptHandle =
           std::is_copy_constructible_v<Handle_>
        && std::is_move_constructible_v<Handle_>;
    // #TODO: revisit is_copy/move_assignable.

    template<typename Detach_>
    concept ConceptDetachHandle =
           ConceptHandle<Detach_>
        && requires(Detach_ detach)
    {
        // MSVC fails to compile ConceptObservable<> if this line
        // is enabled (because of subscribe() -> ConceptObservable<> check.
        // typename Detach_::has_effect;

        { detach() } -> std::convertible_to<bool>;
    };

    template<typename Value, typename Error>
    struct ObserverArchetype
    {
        void on_next(Value);
        void on_error(Error);
        void on_completed();
    };

    template<typename Value>
    struct ObserverArchetype<Value, void>
    {
        void on_next(Value);
        void on_error();
        void on_completed();
    };

    template<typename Value>
    struct ObserverArchetype<Value, none_tag>
    {
        void on_next(Value);
        void on_error();
        void on_completed();
    };

    template<typename ObservableLike_>
    concept ConceptObservable =
           (not std::is_reference_v<ObservableLike_>)
        && ConceptHandle<ObservableLike_>
        && requires(ObservableLike_ observable)
    {
        // #TODO: value_type and error_type should not be references.
        typename ObservableLike_::value_type;
        typename ObservableLike_::error_type;
        typename ObservableLike_::DetachHandle;

        { std::declval<ObservableLike_>().subscribe(ObserverArchetype<
              typename ObservableLike_::value_type
            , typename ObservableLike_::error_type>()) }
                -> ConceptDetachHandle<>;
        // #TODO: should return Self - ConceptObservable<> ?
        { std::declval<ObservableLike_&>().fork() }
            -> ConceptHandle<>;
    };
} // namespace xrx::detail
