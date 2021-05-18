#pragma once

#if defined(_MSC_VER)
  #define XRX_FORCEINLINE() __forceinline
  #define XRX_FORCEINLINE_LAMBDA()
#else
  // #XXX: temporary disabled, need to see why GCC says:
  // warning: 'always_inline' function might not be inlinable [-Wattributes]
  // on simple functions. Something done in a wrong way.
  // 
  // #define XRX_FORCEINLINE() __attribute__((always_inline))
  // #define XRX_FORCEINLINE_LAMBDA() __attribute__((always_inline))
  #define XRX_FORCEINLINE() 
  #define XRX_FORCEINLINE_LAMBDA() 
#endif
