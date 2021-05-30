#if !defined(XRX_PROLOGUE_INCLUDED)
#  error Epilogue is included, but Prologue is missing.
#endif
#undef XRX_PROLOGUE_INCLUDED

#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#undef XRX_MOV_IF_ASYNC
#undef XRX_FORCEINLINE
#undef XRX_FORCEINLINE_LAMBDA
#undef XRX_FWD
#undef XRX_MOV
#undef XRX_RVALUE
#undef XRX_CHECK_RVALUE
#undef XRX_CHECK_TYPE_NOT_REF
