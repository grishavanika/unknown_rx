
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
 - 
