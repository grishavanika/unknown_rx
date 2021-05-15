#pragma once
#include "utils_defines.h"
#include <type_traits>

namespace xrx::detail
{
    // Hint that V is reference and may be moved from
    // to MaybeRef<>.
#define XRX_MOV_IF_ASYNC(V) V

    template<typename T, bool MustBeValue>
    struct MaybeRef;

    template<typename T>
    struct MaybeRef<T, true/*value*/>
    {
        static_assert(not std::is_reference_v<T>);
        static_assert(not std::is_const_v<T>);

        [[no_unique_address]] T _value;
        XRX_FORCEINLINE() T& get() { return _value; }

        XRX_FORCEINLINE() MaybeRef(T& v)
            : _value(XRX_MOV(v))
        {
        }
    };

    template<typename T>
    struct MaybeRef<T, false/*reference*/>
    {
        static_assert(not std::is_reference_v<T>);
        static_assert(not std::is_const_v<T>);

        T* _value = nullptr;
        XRX_FORCEINLINE() T& get() { return *_value; }

        XRX_FORCEINLINE() MaybeRef(T& v)
            : _value(&v)
        {
        }
    };
} // namespace xrx::detail

