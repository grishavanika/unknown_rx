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

        template<typename E>
        struct ErrorInterface
        {
            virtual void on_error(XRX_RVALUE(E&&) e) = 0;

        protected:
            ~ErrorInterface() = default;
        };

        template<>
        struct ErrorInterface<void>
        {
            virtual void on_error() = 0;

        protected:
            ~ErrorInterface() = default;
        };

        template<>
        struct ErrorInterface<none_tag> : ErrorInterface<void> {};

        template<typename Value, typename Error>
        struct ObserverConcept : ErrorInterface<Error>
        {
            virtual ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v) = 0;
            virtual void on_completed() = 0;
            virtual void destroy_() = 0;

        protected:
            ~ObserverConcept() = default;
        };

        template<typename Alloc
            , typename Value
            , typename Error
            , typename ConcreateObserver
            , typename SelfCRTP>
        struct ObserverBase : ObserverConcept<Value, Error>
        {
            using SelfAlloc = std::allocator_traits<Alloc>::template rebind_alloc<SelfCRTP>;
            using AllocTraits = std::allocator_traits<SelfAlloc>;

            ObserverBase(const ObserverBase&) = delete;
            ObserverBase(ObserverBase&&) = delete;
            ObserverBase& operator=(const ObserverBase&) = delete;
            ObserverBase& operator=(ObserverBase&&) = delete;

            virtual ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v) override
            {
                return ::xrx::detail::on_next_with_action(_observer, XRX_MOV(v));
            }

            virtual void on_completed()
            {
                return ::xrx::detail::on_completed_optional(XRX_MOV(_observer));
            }

            explicit ObserverBase(XRX_RVALUE(ConcreateObserver&&) o, const Alloc& alloc)
                : _alloc(alloc)
                , _observer(XRX_MOV(o))
            {
                XRX_CHECK_RVALUE(o);
            }

            static ObserverConcept<Value, Error>* make_(XRX_RVALUE(ConcreateObserver&&) o, const Alloc& alloc_)
            {
                SelfAlloc allocator(alloc_);
                SelfCRTP* mem = AllocTraits::allocate(allocator, 1);
                AllocTraits::construct(allocator, mem, XRX_MOV(o), alloc_);
                return mem;
            }

            virtual void destroy_() override
            {
                SelfAlloc allocator(_alloc);
                SelfCRTP* self = static_cast<SelfCRTP*>(this);
                AllocTraits::destroy(allocator, self);
                AllocTraits::deallocate(allocator, self, 1);
            }

            [[no_unique_address]] Alloc _alloc;
            ConcreateObserver _observer;
        };

        template<typename Alloc, typename Value, typename Error, typename ConcreateObserver>
        struct ErasedObserver
            : ObserverBase<Alloc, Value, Error, ConcreateObserver
                , ErasedObserver<Alloc, Value, Error, ConcreateObserver>>
        {
            using Base = ObserverBase<Alloc, Value, Error, ConcreateObserver
                , ErasedObserver<Alloc, Value, Error, ConcreateObserver>>;
            using Base::Base;
            virtual void on_error(XRX_RVALUE(Error&&) e) override
            {
                on_error_optional(XRX_MOV(this->_observer), XRX_MOV(e));
            }
        };
        template<typename Alloc, typename Value, typename ConcreateObserver>
        struct ErasedObserver<Alloc, Value, void, ConcreateObserver>
            : ObserverBase<Alloc, Value, void, ConcreateObserver
                , ErasedObserver<Alloc, Value, void, ConcreateObserver>>
        {
            using Base = ObserverBase<Alloc, Value, void, ConcreateObserver
                , ErasedObserver<Alloc, Value, void, ConcreateObserver>>;
            using Base::Base;
            virtual void on_error() override
            {
                on_error_optional(XRX_MOV(this->_observer));
            }
        };
        template<typename Alloc, typename Value, typename ConcreateObserver>
        struct ErasedObserver<Alloc, Value, none_tag, ConcreateObserver>
            : ObserverBase<Alloc, Value, none_tag, ConcreateObserver
                , ErasedObserver<Alloc, Value, none_tag, ConcreateObserver>>
        {
            using Base = ObserverBase<Alloc, Value, none_tag, ConcreateObserver
                , ErasedObserver<Alloc, Value, none_tag, ConcreateObserver>>;
            using Base::Base;
            virtual void on_error() override
            {
                on_error_optional(XRX_MOV(this->_observer));
            }
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

        template<typename... VoidOrError>
        void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(_observer);
            if constexpr ((sizeof...(e)) == 0)
            {
                return _observer->on_error();
            }
            else
            {
                return _observer->on_error(XRX_MOV(e)...);
            }
        }

        void on_completed()
        {
            assert(_observer);
            return _observer->on_completed();
        }
    };
} // namespace xrx

#include "xrx_epilogue.h"
