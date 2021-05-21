// Port of linesfrombytes example from RxCpp.
// https://github.com/ReactiveX/RxCpp/blob/9002d9bea0e6b90624672e90a409b56de5286fc6/Rx/v2/examples/linesfrombytes/main.cpp
// 
#include "xrx/xrx_all.h"

#include <regex>
#include <random>
#include <iostream>
#include <iterator>
#include <algorithm>
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
    debug::EventLoop event_loop;

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
                | reduce(std::vector<std::uint8_t>(),
                    [](std::vector<std::uint8_t>&& v, std::uint8_t b)
                    {
                        v.push_back(b);
                        return std::move(v);
                    });
        })
        | tap([](std::vector<std::uint8_t>&& v)
        {
            // print input packet of bytes
            std::copy(v.begin(), v.end(), std::ostream_iterator<long>(std::cout, " "));
            std::cout << std::endl;
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
            return iterate(std::move(splits));
        })
        .filter([](const std::string& s)
        {
            return !s.empty();
        })
        // NOTE: this is only to make .window_toggle() work.
        // It implicitly "works" for RxCpp, because of implicit scheduler
        // passed to concat_map().
        .observe_on(event_loop.scheduler())
        .publish()
        .ref_count();

    // filter to last string in each line
    auto closes = strings.fork()
        .filter([](const std::string& s)
        {
            return s.back() == '\r';
        })
        .transform([](const std::string&)
        {
            return 0;
        });

    // group strings by line
    auto linewindows = strings.fork()
        .window_toggle(closes.fork().starts_with(0)
            , [&](int)
        {
            return closes.fork();
        });

    auto removespaces = [](std::string s)
    {
        auto is_space = [](char ch) { return isspace(ch); };
        s.erase(std::remove_if(s.begin(), s.end(), is_space), s.end());
        return s;
    };

    // reduce the strings for a line into one string
    auto lines = linewindows.fork_move()
        .flat_map([&](auto w)
        {
            auto no_spaces = removespaces;
            return w.fork_move()
                .starts_with(std::string(""))
                .reduce(std::string(""), std::plus<>())
                .transform(std::move(no_spaces));
        });

    lines.fork_move()
        .subscribe([](std::string line)
    {
        std::cout << line << std::endl;
    });

    while (event_loop.work_count() > 0)
    {
        event_loop.poll_one();
    }

    XRX_PRINT_ALLOCS_COUNT("bytes");
}
