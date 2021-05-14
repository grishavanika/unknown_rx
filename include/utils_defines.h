#pragma once

#if defined(_MSC_VER)
  #define XRX_FORCEINLINE() __forceinline
  #define XRX_FORCEINLINE_LAMBDA()
#else
  #define XRX_FORCEINLINE() __attribute__((always_inline))
  #define XRX_FORCEINLINE_LAMBDA() __attribute__((always_inline))
#endif
