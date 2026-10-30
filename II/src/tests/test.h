// Shared helpers for tests, standard library only.
#ifndef TEST_H
#define TEST_H

#include <cmath>
#include <cstring>
#include <iostream>

inline bool passed;

inline void expect(float got, float want) {
  if (got != want) {
    std::cout << "  expected " << want << ", got " << got << "\n";
    passed = false;
  }
}

inline void expect_near(float got, float want, float eps = 1e-4f) {
  if (std::fabs(got - want) > eps) {
    std::cout << "  expected " << want << " (+/- " << eps << "), got " << got << "\n";
    passed = false;
  }
}

inline void expect_true(bool cond, const char* msg = "") {
  if (!cond) {
    std::cout << "  condition failed: " << msg << "\n";
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
