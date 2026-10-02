// Shared helpers for the tests, written with the standard library only.

#ifndef TEST_H
#define TEST_H

#include <cstring>
#include <iostream>

#include "../vec.h"

inline bool passed;

template <std::size_t N>
inline void expect(const vec<N>& got, const vec<N>& want) {
  for (std::size_t i = 0; i < N; i++) {
    if (got.e[i] != want.e[i]) {
      std::cout << "  expected " << want << ", got " << got << "\n";
      passed = false;
      return;
    }
  }
}

inline void expect(float got, float want) {
  if (got != want) {
    std::cout << "  expected " << want << ", got " << got << "\n";
    passed = false;
  }
}

inline void expect(std::size_t got, std::size_t want) {
  if (got != want) {
    std::cout << "  expected " << want << ", got " << got << "\n";
    passed = false;
  }
}

inline int run(const char* name, void (*test)(), const char* filter) {
  if (filter && !std::strstr(name, filter))
    return 0;

  passed = true;
  test();
  std::cout << (passed ? "PASS " : "FAIL ") << name << "\n";
  return passed ? 0 : 1;
}

#endif
