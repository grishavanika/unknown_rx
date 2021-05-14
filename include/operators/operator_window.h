#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "concepts_observable.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include <type_traits>
#include <vector>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable, bool IsAsync>
    struct WindowProducerObservable; // Not yet implemented.

    template<typename Value>
    struct WindowSyncObservable
    {
        using value_type = Value;
        using error_type = none_tag;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        std::vector<value_type> _values;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        NoopUnsubscriber subscribe(Observer observer) &&
        {
            for (value_type v : _values)
            {
                const auto action = on_next_with_action(observer, v);
                if (action._unsubscribe)
                {
                    return NoopUnsubscriber();
                }
            }
            (void)on_completed_optional(XRX_MOV(observer));
            return NoopUnsubscriber();
        }

        WindowSyncObservable fork() && { return WindowSyncObservable(XRX_MOV(_values)); }
        WindowSyncObservable fork() &  { return WindowSyncObservable(_values); }
    };

    template<typename SourceObservable>
    struct WindowProducerObservable<SourceObservable, false/*synchronous*/>
    {
        SourceObservable _source;
        std::size_t _count = 0;

        using source_value = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using ObservableValue_  = WindowSyncObservable<source_value>;
        using value_type   = Observable_<ObservableValue_>;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(Observer observer) &&
        {
            if (_count == 0)
            {
                // It's unknown how many observables we should emit, just end the sequence.
                (void)on_completed_optional(XRX_MOV(observer));
                return Unsubscriber();
            }
            bool unsubscribed = false;
            bool end_with_error = false;
            bool completed = false;
            std::vector<source_value> _values;
            _values.reserve(_count);
            auto unsubscribe = XRX_FWD(_source).subscribe(::xrx::observer::make(
                    [&](source_value value)
            {
                assert(not unsubscribed);
                if (_values.size() < _count)
                {
                    _values.push_back(XRX_MOV(value));
                }
                assert(_values.size() <= _count);
                if (_values.size() == _count)
                {
                    const auto action = on_next_with_action(observer
                        , value_type(ObservableValue_(XRX_MOV(_values))));
                    if (action._unsubscribe)
                    {
                        unsubscribed = true;
                        return ::xrx::unsubscribe(true);
                    }
                    _values.reserve(_count);
                }
                return ::xrx::unsubscribe(false);
            }
                , [&]()
            {
                assert(not unsubscribed);
                completed = true;

                bool finalize_ = true;
                if (_values.size() > 0)
                {
                    const auto action = on_next_with_action(observer
                        , value_type(ObservableValue_(XRX_MOV(_values))));
                    finalize_ = (not action._unsubscribe);
                }
                if (finalize_)
                {
                    (void)on_completed_optional(XRX_MOV(observer));
                }
            }
                , [&](source_error error)
            {
                assert(not unsubscribed);
                end_with_error = true;
                return on_error_optional(observer, XRX_FWD(error));
            }));
            static_assert(not decltype(unsubscribe)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (unsubscribed || end_with_error);
            (void)stop;
            assert((completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return Unsubscriber();
        }

        WindowProducerObservable fork() && { return WindowProducerObservable(XRX_MOV(_source), _count); };
        WindowProducerObservable fork() &  { return WindowProducerObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Window
        , SourceObservable source, std::size_t count)
            requires ConceptObservable<SourceObservable>
    {
        using Impl = WindowProducerObservable<SourceObservable
            , IsAsyncObservable<SourceObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), count));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        struct RememberWindow
        {
            std::size_t _count = 0;

            template<typename SourceObservable>
            auto pipe_(SourceObservable source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Window
                    , SourceObservable, std::size_t>
            {
                return make_operator(operator_tag::Window(), XRX_MOV(source), std::size_t(_count));
            }
        };
    } // namespace detail

    inline auto window(std::size_t count)
    {
        return detail::RememberWindow(count);
    }
} // namespace xrx
