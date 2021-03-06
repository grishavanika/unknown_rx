#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_wrappers.h"
#include "concepts_observer.h"
#include "subject.h"
#include "observable_interface.h"
#include "utils_containers.h"
#include "xrx_prologue.h"
#include <type_traits>
#include <utility>
#include <memory>

namespace xrx::detail
{
    template<typename SourceDetach
        , typename OpeningsDetach
        , typename ClosingsDetach
        , typename WindowHandle>
    struct Unsubscribers
    {
        struct CloseState
        {
            WindowHandle _window;
            ClosingsDetach _detach;
            bool close()
            {
                return std::exchange(_detach, {})();
            }
        };

        SourceDetach _source;
        OpeningsDetach _openings;
        std::vector<CloseState> _closings;
        debug::AssertMutex<> _mutex;
    };

    template<typename Unsubscribers_
        , typename Observer
        , typename SourceValue
        , typename ErrorValue>
    struct WindowToggleShared_
    {
        using Subject = Subject_<SourceValue, ErrorValue>;
        using Windows = HandleVector<Subject>;
        using WindowHandle = typename Windows::Handle;
        Observer _observer;
        Windows _windows;
        Unsubscribers_ _unsubscribers;

        auto& mutex() { return _unsubscribers._mutex; }

        explicit WindowToggleShared_(XRX_RVALUE(Observer&&) observer)
            : _observer(XRX_MOV(observer))
            , _windows()
            , _unsubscribers()
        {
        }

        // From Openings stream.
        WindowHandle on_new_window()
        {
            // 1. We own the mutex.
            WindowHandle handle = _windows.push_back(Subject());
            Subject* subject = _windows.get(handle);
            assert(subject);
            const auto action = on_next_with_action(_observer, XRX_MOV(subject->as_observable()));
            return (action._stop ? WindowHandle() : handle);
        }
        // From Closings stream.
        bool close_window(WindowHandle handle)
        {
            // 1. We own the mutex.
            Subject* subject = _windows.get(handle);
            assert(subject);
            subject->on_completed();
            return _windows.erase(handle);
        }
        // From Source stream.
        void on_next_source(auto source_value)
        {
            // 1. We own the mutex.
            for (Subject& window : _windows)
            {
                auto copy = source_value;
                // Unsubscriptions are handled by the Subject_<> itself.
                window.on_next(XRX_MOV(copy));
            }
        }
        // From Source stream.
        void on_completed_source()
        {
            // 1. We own the mutex.
            for (Subject& window : _windows)
            {
                window.on_completed();
            }
        }
        // From any of (Openings, Closings, Source) stream.
        void on_any_error_unsafe(XRX_RVALUE(ErrorValue&&) e)
        {
            // 1. We own the mutex.
            for (Subject& window : _windows)
            {
                window.on_error(XRX_MOV(e));
            }
        }
    };

    template<typename Shared_>
    struct ClosingsObserver_
    {
        using WindowHandle = typename Shared_::WindowHandle;
        WindowHandle _window;
        std::shared_ptr<Shared_> _shared;

        void unsubscribe_rest_unsafe()
        {
            _shared->_unsubscribers._source();
            _shared->_unsubscribers._openings();
            auto& closings = _shared->_unsubscribers._closings;
            for (auto& closer : closings)
            {
                if (closer._window == _window)
                {
                    // Invalidate self-reference.
                    closer._detach = {};
                }
                else
                {
                    closer.close();
                }
            }
            closings.clear();
        }
        auto on_next(auto)
        {
            auto guard = std::lock_guard(_shared->mutex());
            _shared->close_window(_window);
            return unsubscribe(true);
        }
        void on_completed()
        {
            // Opened window remains opened if no
            // on_next() close event triggered.
        }
        void on_error(XRX_RVALUE(auto&&) e)
        {
            // Do not ignore errors. Propagate them from anywhere (closings stream).
            auto guard = std::lock_guard(_shared->mutex());
            unsubscribe_rest_unsafe();
            _shared->on_any_error_unsafe(XRX_MOV(e));
        }
    };

    template<typename CloseObservableProducer, typename CloseObservable, typename Shared_>
    struct OpeningsObserver_
    {
        using WindowHandle = typename Shared_::WindowHandle;
        CloseObservableProducer _close_producer;
        std::shared_ptr<Shared_> _shared;

        void unsubscribe_rest_unsafe()
        {
            // 1. We own mutex.
            // 2. We cant unsubscribe ourself (self-destroy), just invalidate the reference.
            _shared->_unsubscribers._openings = {};
            _shared->_unsubscribers._source();
            auto& closings = _shared->_unsubscribers._closings;
            for (auto& closer : closings)
            {
                closer.close();
            }
            closings.clear();
        }
        unsubscribe on_next(XRX_RVALUE(auto&&) open_value)
        {
            CloseObservable closer = _close_producer(XRX_MOV(open_value));

            auto guard = std::lock_guard(_shared->mutex());
            WindowHandle handle = _shared->on_new_window();
            if (not handle)
            {
                unsubscribe_rest_unsafe();
                return unsubscribe(true);
            }
            using ClosingsObserver = ClosingsObserver_<Shared_>;
            auto unsubscriber = XRX_MOV(closer).subscribe(ClosingsObserver(handle, _shared));
            _shared->_unsubscribers._closings.push_back({XRX_MOV(handle), XRX_MOV(unsubscriber)});
            return unsubscribe(false);
        }
        auto on_completed()
        {
            // Done. No more windows possible.
        }
        void on_error(XRX_RVALUE(auto&&) e)
        {
            // Do not ignore errors. Propagate them from anywhere (openings stream).
            auto guard = std::lock_guard(_shared->mutex());
            unsubscribe_rest_unsafe();
            _shared->on_any_error_unsafe(XRX_MOV(e));
        }
    };

    template<typename Shared_>
    struct WindowSourceObserver_
    {
        std::shared_ptr<Shared_> _shared;

        void unsubscribe_rest_unsafe()
        {
            // 1. We own mutex.
            // 2. We cant unsubscribe ourself (self-destroy), just invalidate the reference.
            _shared->_unsubscribers._source = {};
            _shared->_unsubscribers._openings();
            auto& closings = _shared->_unsubscribers._closings;
            for (auto& closer : closings)
            {
                closer.close();
            }
            closings.clear();
        }

        auto on_next(XRX_RVALUE(auto&&) source_value)
        {
            auto guard = std::lock_guard(_shared->mutex());
            _shared->on_next_source(XRX_MOV(source_value));
        }
        auto on_completed()
        {
            auto guard = std::lock_guard(_shared->mutex());
            unsubscribe_rest_unsafe();
            _shared->on_completed_source();
        }
        void on_error(XRX_RVALUE(auto&&) e)
        {
            auto guard = std::lock_guard(_shared->mutex());
            unsubscribe_rest_unsafe();
            _shared->on_any_error_unsafe(XRX_MOV(e));
        }
    };

    template<typename Unsubscribers_>
    struct WindowToggleDetach
    {
        using has_effect = std::true_type;

        std::shared_ptr<Unsubscribers_> _shared;

        bool operator()()
        {
            auto shared = std::exchange(_shared, {});
            if (not shared)
            {
                return false;
            }
            auto guard = std::lock_guard(shared->_mutex);
            const bool source_detached = std::exchange(shared->_source, {})();
            const bool openings_detached = std::exchange(shared->_openings, {})();
            bool at_least_one_closer = false;
            for (auto& closer : shared->_closings)
            {
                const bool detached = closer();
                at_least_one_closer |= detached;
            }
            shared->closings.clear();
            return (source_detached
                or openings_detached
                or at_least_one_closer);
        }
    };

    template<typename E1, typename E2, typename E3>
    struct MergedErrors3
    {
        static constexpr bool are_all_same = (std::is_same_v<E1, E2> and std::is_same_v<E1, E3>);
        using are_compatible = std::bool_constant<are_all_same>;
        using E = E1;
    };

    template<typename SourceObservable, typename OpeningsObservable, typename CloseObservableProducer, typename CloseObservable>
    struct WindowToggleObservableImpl_
    {
        SourceObservable _source;
        OpeningsObservable _openings;
        CloseObservableProducer _close_producer;

        using MergedErrors = MergedErrors3<
              typename SourceObservable::error_type
            , typename OpeningsObservable::error_type
            , typename CloseObservable::error_type>;

        static_assert(MergedErrors::are_compatible::value
            , "All steams (Opening, Closings, Source) should have same errors.");

        using source_type = typename SourceObservable::value_type;
        using error_type = typename MergedErrors::E;
        using Subject = Subject_<source_type, error_type>;
        using Windows = HandleVector<Subject>;
        using WindowHandle = typename Windows::Handle;
        using UnsubscribersState = Unsubscribers<
              typename SourceObservable::DetachHandle
            , typename OpeningsObservable::DetachHandle
            , typename CloseObservable::DetachHandle
            , WindowHandle>;
        using Window = typename Subject::Observable;
        using value_type = Window;
        using is_async = std::true_type;
        using DetachHandle = WindowToggleDetach<UnsubscribersState>;

        WindowToggleObservableImpl_ fork() && { return WindowToggleObservableImpl_(XRX_MOV(_source), XRX_MOV(_openings), XRX_MOV(_close_producer)); }
        WindowToggleObservableImpl_ fork() &  { return WindowToggleObservableImpl_(_source.fork(), _openings, _close_producer); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            using Shared_ = WindowToggleShared_<UnsubscribersState, Observer, source_type, error_type>;
            using OpeningsObserver = OpeningsObserver_<CloseObservableProducer, CloseObservable, Shared_>;
            using WindowSourceObserver = WindowSourceObserver_<Shared_>;

            std::shared_ptr<Shared_> shared = std::make_shared<Shared_>(XRX_MOV(observer));
            std::shared_ptr<UnsubscribersState> unsubscribers(shared, &shared->_unsubscribers);
            unsubscribers->_openings = XRX_MOV(_openings).subscribe(OpeningsObserver(XRX_MOV(_close_producer), shared));
            unsubscribers->_source = XRX_MOV(_source).subscribe(WindowSourceObserver(shared));
            return DetachHandle(XRX_MOV(unsubscribers));
        }
    };

    template<typename SourceObservable, typename OpeningsObservable, typename CloseObservableProducer>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::WindowToggle
        , XRX_RVALUE(SourceObservable&&) source
        , XRX_RVALUE(OpeningsObservable&&) openings
        , XRX_RVALUE(CloseObservableProducer&&) close_producer)
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(openings);
        XRX_CHECK_RVALUE(close_producer);
        using opening_value = typename OpeningsObservable::value_type;
        using CloseObservable = decltype(close_producer(std::declval<opening_value>()));
        XRX_CHECK_TYPE_NOT_REF(CloseObservable);
        static_assert(ConceptObservable<CloseObservable>);

        static_assert(IsAsyncObservable<SourceObservable>()
            , ".window_toggle() does not make sense for Sync Source Observable.");
        static_assert(IsAsyncObservable<OpeningsObservable>()
            , ".window_toggle() does not make sense for Sync Openings Observable.");
        static_assert(IsAsyncObservable<OpeningsObservable>()
            , ".window_toggle() does not make sense for Sync Closing Observable.");

        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(OpeningsObservable);
        XRX_CHECK_TYPE_NOT_REF(CloseObservableProducer);
        using Impl = WindowToggleObservableImpl_<SourceObservable, OpeningsObservable, CloseObservableProducer, CloseObservable>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(openings), XRX_MOV(close_producer)));
    }
} // namespace xrx::detail

#include "xrx_epilogue.h"
