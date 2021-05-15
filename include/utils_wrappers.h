#pragma once
#include "utils_defines.h"
#include <type_traits>

namespace xrx::detail
{
    template<typename T, bool MustBeValue>
    struct MaybeRef;

    template<typename T>
    struct MaybeRef<T, true/*value*/>
    {
        static_assert(not std::is_reference_v<T>);
        static_assert(not std::is_const_v<T>);

        [[no_unique_address]] T _value;
        XRX_FORCEINLINE() T& get() { return _value; }
    };

    template<typename T>
    struct MaybeRef<T, false/*reference*/>
    {
        static_assert(not std::is_reference_v<T>);
        static_assert(not std::is_const_v<T>);

        T* _value = nullptr;
        XRX_FORCEINLINE() T& get() { return *_value; }
    };

    template<typename T, bool MustBeValue>
    XRX_FORCEINLINE() auto maybe_ref(T& v, std::bool_constant<MustBeValue>)
    {
        if constexpr (MustBeValue)
        {
            return MaybeRef<T, true>(XRX_MOV(v));
        }
        else
        {
            return MaybeRef<T, false>(&v);
        }
    }
} // namespace xrx::detail


