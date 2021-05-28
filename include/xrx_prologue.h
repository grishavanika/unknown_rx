#if defined(XRX_PROLOGUE_INCLUDED)
#  error Prologue is already included.
#endif
#define XRX_PROLOGUE_INCLUDED

#include <utility>
#include <type_traits>

#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wattributes"
#endif

// Hint that V is reference and may be moved from
// to MaybeRef<>.
#define XRX_MOV_IF_ASYNC(V) V

#if defined(_MSC_VER)
  #define XRX_FORCEINLINE() __forceinline
  #define XRX_FORCEINLINE_LAMBDA()
#else
  // #XXX: check why GCC says:
  // warning: 'always_inline' function might not be inlinable [-Wattributes]
  // on simple functions. Something done in a wrong way.
#  if (1)
#    define XRX_FORCEINLINE() __attribute__((always_inline))
#    define XRX_FORCEINLINE_LAMBDA() __attribute__((always_inline))
#  else
#    define XRX_FORCEINLINE() 
#    define XRX_FORCEINLINE_LAMBDA() 
#  endif
#endif

// #XXX: this needs to be without include-guard
// and in something like XRX_prelude.h header
// so it can be undefined in XRX_conclusion.h.
// #XXX: do with static_cast<>. Get read of <utility> include.
// https://vittorioromeo.info/index/blog/capturing_perfectly_forwarded_objects_in_lambdas.html
// https://github.com/SuperV1234/vrm_core/issues/1
#define XRX_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define XRX_MOV(...) ::std::move(__VA_ARGS__)

// Just an annotation that T&& should be rvalue reference
// (because of constraints from other places).
// XRX_MOV() can be used.
#define XRX_RVALUE(...) __VA_ARGS__
#define XRX_CHECK_RVALUE(...) \
    static_assert(not std::is_lvalue_reference_v<decltype(__VA_ARGS__)> \
        , "Expected to have rvalue reference. " \
        # __VA_ARGS__)

#define XRX_CHECK_TYPE_NOT_REF(...) \
    static_assert(not std::is_reference_v<__VA_ARGS__> \
        , "Expected to have non-reference type (not T& ot T&&; to be moved from). " \
        # __VA_ARGS__)

#define X_ANY_OBSERVER_SUPPORTS_COPY() 0
