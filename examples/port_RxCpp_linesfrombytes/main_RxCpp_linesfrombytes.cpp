// Port of linesfrombytes example from RxCpp.
// https://github.com/ReactiveX/RxCpp/blob/9002d9bea0e6b90624672e90a409b56de5286fc6/Rx/v2/examples/linesfrombytes/main.cpp
// 
#include "xrx/xrx_all.h"

#include <regex>
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
            return concat(std::move(body), std::move(delim));
            })
        | window(17)
        | flat_map([](auto&& observable)
        {
            return observable.fork_move()
                | reduce(std::vector<uint8_t>(),
                    [](std::vector<uint8_t>&& v, uint8_t b)
                    {
                        v.push_back(b);
                        return std::move(v);
                    });
        });

    // create strings split on \r
    auto strings = bytes.fork()
        .flat_map([](std::vector<uint8_t> v)
        {
            std::string s(v.begin(), v.end());
            std::regex delim(R"/(\r)/");
            std::cregex_token_iterator cursor(&s[0], &s[0] + s.size(), delim, {-1, 0});
            std::cregex_token_iterator end;
            std::vector<std::string> splits(cursor, end);
            // return iterate(move(splits));
            assert(splits.size() > 0);
            return observable::from(std::move(splits[0]));
        })
        .filter([](const std::string& s)
        {
            return !s.empty();
        })
        .publish()
        .ref_count();

    // WIP. To be continued.
    std::cout << "<bytes>" << "\n";
    bytes.fork()
        .subscribe([](auto v)
    {
        std::copy(v.begin(), v.end(), std::ostream_iterator<long>(std::cout, " "));
        std::cout << std::endl;
    });
    std::cout << "<strings>" << "\n";
    strings.fork()
        .subscribe([](auto v)
    {
        std::cout << v << std::endl;
    });

    XRX_PRINT_ALLOCS_COUNT("bytes");
}
