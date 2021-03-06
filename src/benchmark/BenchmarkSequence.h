
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "BenchmarkBase.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Benchmark_Sequence
///////////////////////////////////////////////////////////////////////////////////////////////////
struct BenchmarkParams_Sequence
{
    BenchmarkParams_Sequence(int n, int k, int delay) :
        N(n),
        K(k),
        Delay(delay)
    {
    }

    const int N;
    const int K;
    const int Delay;

    void Print(std::ostream& out) const
    {
        out << "N = " << N
            << ", K = " << K
            << ", Delay = " << Delay;
    }
};

template <typename D>
struct Benchmark_Sequence : public BenchmarkBase<D>
{
    double Run(const BenchmarkParams_Sequence& params)
    {
        using MyDomain = D;
        using MyHandle = MyDomain::SignalT<int>;

        bool initializing = true;

        auto in = MyDomain::MakeVar(1);

        std::vector<MyHandle> nodes;
        auto f = [&initializing,&params] (int a)
        {
            if (params.Delay > 0 && !initializing)
            {
                auto t0 = std::chrono::high_resolution_clock::now();
                while (std::chrono::high_resolution_clock::now() - t0 < std::chrono::milliseconds(params.Delay));
            }
            return a + 1;
        };

        MyHandle cur = in;
        for (int i=0; i<params.N; i++)
            cur = cur ->* f;

        initializing = false;

        auto t0 = tbb::tick_count::now();
        for (int i=0; i<params.K; i++)
            in <<= 10+i;
        auto t1 = tbb::tick_count::now();

        double d = (t1 - t0).seconds();

        return d;
    }
};