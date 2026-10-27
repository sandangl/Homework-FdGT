// ppl.h shim — provides concurrency::parallel_for via OpenMP for MinGW/GCC.
#pragma once
#ifndef PPL_SHIM_H
#define PPL_SHIM_H

#ifndef _MSC_VER

#include <functional>

namespace concurrency {

template <typename Index, typename Function>
inline void parallel_for(Index first, Index last, const Function& f) {
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic)
    for (Index i = first; i < last; ++i)
        f(i);
#else
    for (Index i = first; i < last; ++i)
        f(i);
#endif
}

} // namespace concurrency

using concurrency::parallel_for;

#endif // !_MSC_VER
#endif // PPL_SHIM_H
