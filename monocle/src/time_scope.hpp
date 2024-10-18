#pragma once

#include <iostream>
#include <chrono>

#define _CONCAT1(a, b) a##b
#define _CONCAT2(a, b) _CONCAT1(a, b)

#define TIME_SCOPE(name) TimeScope _CONCAT2(_ts, __COUNTER__)(name)
#define TIME_FUNC() TIME_SCOPE(__FUNCTION__)

struct TimeScope {

    using _clock = std::chrono::high_resolution_clock;

    const char* name;
    _clock::time_point start;

    TimeScope(const char* name) : name{name}, start{_clock::now()} {}

    ~TimeScope()
    {
        auto diff = std::chrono::duration<double>(_clock::now() - start);
        std::cout << "TIMER: " << name << " took " << diff.count() << "s\n";
    }
};
