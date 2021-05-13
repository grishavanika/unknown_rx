// Port of linesfrombytes example from RxCpp.
// https://github.com/ReactiveX/RxCpp/blob/9002d9bea0e6b90624672e90a409b56de5286fc6/Rx/v2/examples/linesfrombytes/main.cpp
// 
#include "xrx/xrx_all.h"

// #include <regex>
#include <random>
#include <iostream>
#include <cstdint>

using namespace xrx;
using namespace xrx::observable;

#define XRX_SHOW_ALLOCS_COUNT() 0

#if (XRX_SHOW_ALLOCS_COUNT())
#include <iostream>
struct Allocs
{
    std::size_t _count = 0;
    std::size_t _size = 0;

    static Allocs& get()
    {
        static Allocs o;
        return o;
    }

    void dump(const char* title)
    {
        std::cout << "[" << title << "] "
            << "Allocations count             : " << _count << "\n";
        std::cout << "[" << title << "] "
            << "Allocations total size (bytes): " << _size << "\n";
        std::cout.flush();
    }
};

void* operator new(std::size_t count)
{
    auto& x = Allocs::get();
    ++x._count;
    ++x._size += count;
    return malloc(count);
}

void* operator new[](std::size_t count)
{
    auto& x = Allocs::get();
    ++x._count;
    ++x._size += count;
    return malloc(count);
}

void operator delete(void* ptr) noexcept
{
    free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    free(ptr);
}

#define XRX_RESET_ALLOCS_COUNT() Allocs::get() = {}
#define XRX_PRINT_ALLOCS_COUNT(TITLE) Allocs::get().dump(TITLE)
#else
#define XRX_RESET_ALLOCS_COUNT() (void)0
#define XRX_PRINT_ALLOCS_COUNT(TITLE) (void)0
#endif

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(4, 18);

    XRX_RESET_ALLOCS_COUNT();
    // for testing purposes, produce byte stream that from lines of text
    auto bytes = range(0, 10)
        | flat_map([&](int i)
        {
            auto body = from(std::uint8_t('A' + i))
                | repeat(dist(gen));
            auto delim = from(std::uint8_t('\r'));
            return concat(body, delim);
        });

    // WIP. To be continued.
    bytes.fork()
        | transform([](std::uint8_t v) { return char(v); })
        | subscribe([](char ch)
    {
        if (ch == '\r') ch = '\n';
        std::cout.put(ch);
    });
    XRX_PRINT_ALLOCS_COUNT("bytes");
}
