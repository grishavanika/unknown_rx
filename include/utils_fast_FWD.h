#pragma once
#include <utility>

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
