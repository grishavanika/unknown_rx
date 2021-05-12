#pragma once
#include "concepts_observer.h"
#include "utils_observers.h"
#include <type_traits>
#include <utility>
#include <memory>
#include <cassert>

// Type-erased version of any Observer.
namespace xrx
{
    template<typename Value, typename Error>
    struct AnyObserver;

    namespace detail
    {
        template<typename>
        struct IsAnyObserver : std::false_type {};
        template<typename Value, typename Error>
        struct IsAnyObserver<AnyObserver<Value, Error>> : std::true_type {};
    } // namespace detail

    template<typename Value, typename Error = void>
    struct AnyObserver
    {
#define X_ANY_OBSERVER_SUPPORTS_COPY() 0

        struct ObserverConcept
        {
            virtual ~ObserverConcept() = default;
#if (X_ANY_OBSERVER_SUPPORTS_COPY())
            virtual std::unique_ptr<ObserverConcept> copy_() const = 0;
#endif
            virtual ::xrx::detail::OnNextAction on_next(Value v) = 0;
            virtual void on_error(Error e) = 0;
            virtual void on_completed() = 0;
        };

        template<typename ConcreateObserver>
        struct Observer : ObserverConcept
        {
            Observer(const Observer&) = delete;
            Observer(Observer&&) = delete;
            Observer& operator=(const Observer&) = delete;
            Observer& operator=(Observer&&) = delete;

#if (X_ANY_OBSERVER_SUPPORTS_COPY())
            virtual std::unique_ptr<ObserverConcept> copy_() const override
            {
                return std::make_unique<Observer>(_observer);
            }
#endif

            virtual ::xrx::detail::OnNextAction on_next(Value v) override
            {
                return ::xrx::detail::on_next_with_action(_observer, std::forward<Value>(v));
            }

            virtual void on_error(Error e) override
            {
                return ::xrx::detail::on_error(std::move(_observer), std::forward<Error>(e));
            }

            virtual void on_completed()
            {
                return ::xrx::detail::on_completed(std::move(_observer));
            }

            explicit Observer(ConcreateObserver o)
                : _observer(std::move(o))
            {
            }

            ConcreateObserver _observer;
        };

        // Should not be explicit to support implicit conversions
        // on the user side from any observable where AnyObserver can't be
        // directly constructed. Example:
        // observable::create([](AnyObserver<>) {})
        //     .subscribe(CustomObserver()).
        // (Here - conversions from CustomObserver to AnyObserver<> happens
        // when passing arguments to create() callback).
        /*explicit*/ AnyObserver() = default;

        template<typename ConcreateObserver>
            requires (!detail::IsAnyObserver<ConcreateObserver>::value)
        /*explicit*/ AnyObserver(ConcreateObserver o)
            : _observer(std::make_unique<Observer<ConcreateObserver>>(std::move(o)))
        {
        }

#if (X_ANY_OBSERVER_SUPPORTS_COPY())
        AnyObserver(const AnyObserver& rhs)
            : _observer((assert(rhs._observer), rhs._observer->copy_()))
        {
        }
        AnyObserver& operator=(const AnyObserver& rhs)
        {
            if (this != &rhs)
            {
                assert(rhs._observer);
                _observer = rhs._observer->copy_();
            }
            return *this;
        }
#endif
        AnyObserver(AnyObserver&& rhs) noexcept
            : _observer(std::move(rhs._observer))
        {
        }
        AnyObserver& operator=(AnyObserver&& rhs) noexcept
        {
            if (this != &rhs)
            {
                _observer = std::move(rhs._observer);
            }
            return *this;
        }

        explicit operator bool() const
        {
            return !!_observer;
        }

        std::unique_ptr<ObserverConcept> _observer;

        ::xrx::detail::OnNextAction on_next(Value v)
        {
            assert(_observer);
            return _observer->on_next(std::forward<Value>(v));
        }

        void on_error(Error e)
        {
            assert(_observer);
            return _observer->on_error(std::forward<Error>(e));
        }

        void on_completed()
        {
            assert(_observer);
            return _observer->on_completed();
        }
    };
} // namespace xrx

#undef X_ANY_OBSERVER_SUPPORTS_COPY
