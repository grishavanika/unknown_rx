#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_wrappers.h"
#include "utils_defines.h"
#include "concepts_observer.h"
#include "subject.h"
#include "observable_interface.h"
#include "utils_containers.h"
#include <type_traits>
#include <utility>
#include <memory>

namespace xrx::detail
{
    template<typename Observer, typename SourceValue>
    struct WindowToggleShared_
    {
        using Windows = HandleVector<Subject_<SourceValue>>;
        using WindowHandle = typename Windows::Handle;
        Observer _observer;
        Windows _windows;

        WindowHandle on_new_window()
        {
            auto handle = _windows.push_back(Subject_<SourceValue>());
            auto* subject = _windows.get(handle);
            assert(subject);
            auto action = on_next_with_action(_observer, XRX_MOV(subject->as_observable()));
            (void)action;
            return handle;
        }
        bool close_window(WindowHandle handle)
        {
            auto* subject = _windows.get(handle);
            assert(subject);
            subject->on_completed();
            return _windows.erase(handle);
        }
        void notify(auto source_value)
        {
            _windows.for_each([&](auto& window)
            {
                auto copy = source_value;
                (void)window.on_next(XRX_MOV(copy));
            });
        }
        void notify_completed()
        {
            // close all closing observables.
            _windows.for_each([&](auto& window)
            {
                (void)window.on_completed();
            });
        }
    };

    template<typename Shared_>
    struct ClosingsObserver_
    {
        using WindowHandle = typename Shared_::WindowHandle;
        WindowHandle _window;
        std::shared_ptr<Shared_> _shared;

        auto on_next(auto)
        {
            _shared->close_window(_window);
            return unsubscribe(true);
        }
        auto on_completed()
        {
        }
        auto on_error()
        {
        }
    };

    template<typename CloseObservableProducer, typename CloseObservable, typename Shared_>
    struct OpeningsObserver_
    {
        CloseObservableProducer _close_producer;
        std::shared_ptr<Shared_> _shared;

        auto on_next(XRX_RVALUE(auto) open_value)
        {
            CloseObservable closer = _close_producer(XRX_MOV(open_value));
            auto handle = _shared->on_new_window();
            auto unsusbscriber1 = XRX_MOV(closer).subscribe(ClosingsObserver_<Shared_>(XRX_MOV(handle), _shared));
            (void)unsusbscriber1;
        }
        auto on_completed()
        {
            // _shared->notify_completed();
        }
        auto on_error()
        {
        }
    };

    template<typename Shared_>
    struct WindowSourceObserver_
    {
        std::shared_ptr<Shared_> _shared;

        auto on_next(auto source_value)
        {
            _shared->notify(XRX_MOV(source_value));
        }
        auto on_completed()
        {
            _shared->notify_completed();
        }
        auto on_error()
        {
            // _shared->notify_error();
        }
    };

    template<typename SourceObservable, typename OpeningsObservable, typename CloseObservableProducer, typename CloseObservable>
    struct WindowToggleObservableImpl_
    {
        SourceObservable _source;
        OpeningsObservable _openings;
        CloseObservableProducer _close_producer;

        using source_type = typename SourceObservable::value_type;
        using Window = decltype(Subject_<source_type>().as_observable());
        using value_type = Window;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = std::true_type;
        using Unsubscriber = NoopUnsubscriber;

        WindowToggleObservableImpl_ fork() && { return WindowToggleObservableImpl_(XRX_MOV(_source), XRX_MOV(_openings), XRX_MOV(_close_producer)); }
        WindowToggleObservableImpl_ fork()  & { return WindowToggleObservableImpl_(_source.fork(), _openings, _close_producer); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            using Shared_ = WindowToggleShared_<Observer, source_type>;
            using OpeningsObserver = OpeningsObserver_<CloseObservableProducer, CloseObservable, Shared_>;
            using WindowSourceObserver = WindowSourceObserver_<Shared_>;

            std::shared_ptr<Shared_> shared = std::make_shared<Shared_>(Shared_(XRX_MOV(observer)));
            auto unsubscribe1 = XRX_MOV(_openings).subscribe(OpeningsObserver(XRX_MOV(_close_producer), shared));
            (void)unsubscribe1;
            auto unsubscribe2 = XRX_MOV(_source).subscribe(WindowSourceObserver(shared));
            (void)unsubscribe2;
            (void)observer;
            return Unsubscriber();
        }
    };

    template<typename SourceObservable, typename OpeningsObservable, typename CloseObservableProducer>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::WindowToggle
        , XRX_RVALUE(SourceObservable&&) source
        , XRX_RVALUE(OpeningsObservable&&) openings
        , XRX_RVALUE(CloseObservableProducer&&) close_producer)
    {
        static_assert(not std::is_lvalue_reference_v<SourceObservable>);
        static_assert(not std::is_lvalue_reference_v<OpeningsObservable>);
        static_assert(not std::is_lvalue_reference_v<CloseObservableProducer>);

        using opening_value = typename OpeningsObservable::value_type;
        using CloseObservable = decltype(close_producer(std::declval<opening_value>()));
        static_assert(not std::is_reference_v<CloseObservable>);
        static_assert(ConceptObservable<CloseObservable>);

        static_assert(IsAsyncObservable<SourceObservable>()
            , ".window_toggle() does not make sense for Sync Source Observable.");
        static_assert(IsAsyncObservable<OpeningsObservable>()
            , ".window_toggle() does not make sense for Sync Openings Observable.");
        static_assert(IsAsyncObservable<OpeningsObservable>()
            , ".window_toggle() does not make sense for Sync Closing Observable.");

        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using OpeningsObservable_ = std::remove_reference_t<OpeningsObservable>;
        using CloseObservableProducer_ = std::remove_reference_t<CloseObservableProducer>;
        using Impl = WindowToggleObservableImpl_<SourceObservable_, OpeningsObservable_, CloseObservableProducer_, CloseObservable>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(openings), XRX_MOV(close_producer)));
    }
} // namespace xrx::detail
