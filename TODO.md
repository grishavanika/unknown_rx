
 - Do not use ::xrx::observer::make() in Observables implementation. Leads to bigger observer's size.
 - Finaly decide if reference types as value_type for Observables are possible/supportable.
   Currently there is a mess with XRX_MOV, XRX_FWD in non-template context.
 - Make all Observable copy c-tor/copy assignment as private. Only fork() can be used to copy.
 - Allocator-aware ? See if every allocation operator/action can take an allocator as optional parameter.
 - Observable<void, void> (i.e, Value = void -> on_next()) should be supported.
 - Make sure library supports move-only Observers.
 - For each observable & operator there should be a compilation test for move-only/copy-only Observers.
 - Debug/Asser Mutex should be locked on as big scope as possible as opposed to real mutex.
 - Check usage of std::forward<> on Value/Error for a templated classes.
 - Check usage of shared_ptr/weak_ptr *with* callbacks. Can cycle references be created.
 - Check usage of shared_ptr/weak_ptr in Unsibscribers.
 - Debug code/trackers that count live objects and report leaks.
   To be sure no cyclic references created in observers/callbacks/observables/unsubscribers.
 - fork() && and fork() & overloads.
 - Add allocators support to all interfaces.
 - Add Small Buffer/Object Optimization to type-erased interfaces.
 - Detaching Observer after on_completed()/on_error().
 - Safe guards/asserts for calls to on_next() after stream's end ?
 - Do subscription unsubscribe for Observables that support dynamic subscription when Observer returns ::xrx::unsubscribe(true).
 - Maybe Scheduler interface should support State passing & remembering.
   This should help to reduce usage of std::shared_ptr in few places.
 - subscribe_on(): when invoked for Observable that produces few on_next() calls
   and iff Scheduler is single-thread (?) only (other options are possible) - move whole Observable
   to that thread/task to optimize amoun of scheduling. Example: observable::range(0).subscribe_on(main_loop).
 - [*] Support Observables with only move-constructor. For them, no copy & fork should be available.
 - 


