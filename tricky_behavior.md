
## Observer's suvscription.

Observer (target) passed to .subscribe() may be moved multiple time
(implementation defined for a specific Observable) before (any) fist event.
Once first on_next() is triggered, target Observer can't be moved.  
This means that you can rely that all source sequence of events will
be invoked on one/same instance of Observer.





