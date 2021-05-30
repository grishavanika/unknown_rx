#pragma once
#include "concepts_observer.h"
#include "utils_observers.h"
#include "xrx_prologue.h"
#include <type_traits>
#include <utility>
#include <memory>
#include <cassert>

// Type-erased version of any Observer.
namespace xrx
{
    template<typename Value, typename Error = void, typename Alloc = std::allocator<void>>
    struct AnyObserver;

    namespace detail
    {
        template<typename>
        struct IsAnyObserver : std::false_type {};
        template<typename Value, typename Error, typename Alloc>
        struct IsAnyObserver<AnyObserver<Value, Error, Alloc>> : std::true_type {};

        template<typename Value, typename Error>
        struct ObserverConcept
        {
            virtual ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v) = 0;
            virtual void on_completed() = 0;
            virtual void destroy_() = 0;
            virtual void on_error(XRX_RVALUE(Error&&) e) = 0;

        protected:
            ~ObserverConcept() = default;
        };

        template<typename Alloc
            , typename Value
            , typename Error
            , typename ConcreateObserver>
        struct ErasedObserver : ObserverConcept<Value, Error>
        {
            using SelfAlloc = std::allocator_traits<Alloc>::template rebind_alloc<ErasedObserver>;
            using AllocTraits = std::allocator_traits<SelfAlloc>;

            ErasedObserver(const ErasedObserver&) = delete;
            ErasedObserver(ErasedObserver&&) = delete;
            ErasedObserver& operator=(const ErasedObserver&) = delete;
            ErasedObserver& operator=(ErasedObserver&&) = delete;

            virtual ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v) override
            {
                return ::xrx::detail::on_next_with_action(_observer, XRX_MOV(v));
            }

            virtual void on_completed()
            {
                return ::xrx::detail::on_completed_optional(XRX_MOV(_observer));
            }

            virtual void on_error(XRX_RVALUE(Error&&) e) override
            {
                return on_error_optional(XRX_MOV(this->_observer), XRX_MOV(e));
            }

            explicit ErasedObserver(XRX_RVALUE(ConcreateObserver&&) o, const Alloc& alloc)
                : _alloc(alloc)
                , _observer(XRX_MOV(o))
            {
                XRX_CHECK_RVALUE(o);
            }

            static ObserverConcept<Value, Error>* make_(XRX_RVALUE(ConcreateObserver&&) o, const Alloc& alloc_)
            {
                SelfAlloc allocator(alloc_);
                ErasedObserver* mem = AllocTraits::allocate(allocator, 1);
                AllocTraits::construct(allocator, mem, XRX_MOV(o), alloc_);
                return mem;
            }

            virtual void destroy_() override
            {
                SelfAlloc allocator(_alloc);
                ErasedObserver* self = this;
                AllocTraits::destroy(allocator, self);
                AllocTraits::deallocate(allocator, self, 1);
            }

            [[no_unique_address]] Alloc _alloc;
            ConcreateObserver _observer;
        };
    } // namespace detail

    template<typename Value, typename Error, typename Alloc>
    struct AnyObserver
    {
        static_assert(not std::is_reference_v<Value>);
        static_assert(not std::is_reference_v<Error>);

        using allocator_type = Alloc;

        using ObserverConcept_ = detail::ObserverConcept<Value, Error>;

        template<typename ConcreateObserver>
        using Observer_ = detail::ErasedObserver<Alloc, Value, Error, ConcreateObserver>;

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
        /*explicit*/ AnyObserver(XRX_RVALUE(ConcreateObserver&&) o)
            : AnyObserver(XRX_MOV(o), Alloc())
        {
            XRX_CHECK_RVALUE(o);
        }

        template<typename ConcreateObserver>
            requires (!detail::IsAnyObserver<ConcreateObserver>::value)
        /*explicit*/ AnyObserver(XRX_RVALUE(ConcreateObserver&&) o, const Alloc& alloc)
            : _observer(Observer_<ConcreateObserver>::make_(XRX_MOV(o), alloc))
        {
            XRX_CHECK_RVALUE(o);
        }

        ~AnyObserver()
        {
            if (_observer)
            {
                _observer->destroy_();
                _observer = nullptr;
            }
        }

        AnyObserver(const AnyObserver& rhs) = delete;
        AnyObserver& operator=(const AnyObserver& rhs) = delete;

        AnyObserver(AnyObserver&& rhs) noexcept
            : _observer(std::exchange(rhs._observer, nullptr))
        {
        }
        AnyObserver& operator=(AnyObserver&& rhs) noexcept
        {
            if (this != &rhs)
            {
                this->~AnyObserver();
                _observer = std::exchange(rhs._observer, nullptr);
            }
            return *this;
        }

        explicit operator bool() const
        {
            return !!_observer;
        }

        ObserverConcept_* _observer = nullptr;

        ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v)
        {
            assert(_observer);
            return _observer->on_next(XRX_MOV(v));
        }

        void on_error(XRX_RVALUE(Error&&) e)
        {
            assert(_observer);
            return _observer->on_error(XRX_MOV(e));
        }

        void on_completed()
        {
            assert(_observer);
            return _observer->on_completed();
        }
    };
} // namespace xrx

#include "xrx_epilogue.h"
