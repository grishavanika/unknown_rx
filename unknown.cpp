#include "concepts_observer.h"
#include "utils_observers.h"
#include "subject.h"
#include "observable_interface.h"
#include "operators/operator_filter.h"
#include "operators/operator_transform.h"

#include <string>
#include <cstdio>

using namespace xrx;
using namespace detail;

namespace detail2
{
    struct none_tag {};
} // namespace detail2

struct InitialSourceObservable_
{
    using value_type = int;
    using error_type = ::detail2::none_tag;

    struct Unsubscriber
    {
        using has_effect = std::false_type;
        bool detach() { return false; }
    };

    template<typename Observer>
    Unsubscriber subscribe(Observer&& observer) &&
    {
        auto strict = observer::make_complete(std::forward<Observer>(observer));
        strict.on_next(1);
        strict.on_next(2);
        strict.on_next(3);
        strict.on_next(4);
        strict.on_completed();
        return Unsubscriber();
    }
};

int main()
{
    InitialSourceObservable_ initial;
    using O = Observable_<InitialSourceObservable_>;
    O observable(std::move(initial));

    {
        auto unsubscriber = O(observable).subscribe([](int value)
        {
            std::printf("Observer: %i.\n", value);
        });
        using Unsubscriber = typename decltype(unsubscriber)::Unsubscriber;
        static_assert(std::is_same_v<Unsubscriber, InitialSourceObservable_::Unsubscriber>);
        static_assert(not Unsubscriber::has_effect());
    }

    {
        auto unsubscriber = O(observable)
            .filter([](int)   { return true; })
            .filter([](int v) { return ((v % 2) == 0); })
            .filter([](int)   { return true; })
            .subscribe([](int value)
        {
            std::printf("Filtered even numbers: %i.\n", value);
        });
        using Unsubscriber = typename decltype(unsubscriber)::Unsubscriber;
        static_assert(std::is_same_v<Unsubscriber, InitialSourceObservable_::Unsubscriber>);
        static_assert(not Unsubscriber::has_effect());
    }

    Subject_<int, int> subject;
    using ExpectedUnsubscriber = typename decltype(subject)::Unsubsriber;

    subject.subscribe([](int) {});

    auto unsubscriber = subject.as_observable()
        .filter([](int)   { return true; })
        .filter([](int v) { return ((v % 2) == 0); })
        .filter([](int)   { return true; })
        .subscribe(observer::make([](int value)
    {
        std::printf("[Subject1] Filtered even numbers: %i.\n", value);
    }
        , []()
    {
        std::printf("[Subject1] Completed.\n");
    }));
    static_assert(std::is_same_v<decltype(unsubscriber), ExpectedUnsubscriber>);
    static_assert(ExpectedUnsubscriber::has_effect());

    subject.as_observable()
        .filter([](int v) { return (v == 3); })
        .transform([](int v) { return std::to_string(v); })
        .subscribe([](std::string v)
    {
        std::printf("[Subject2] Exactly 3: %s.\n", v.c_str());
    });

    subject.on_next(1);
    subject.on_next(2);

    unsubscriber.detach();

    subject.on_next(3);
    subject.on_next(4);
    subject.on_completed();
}
