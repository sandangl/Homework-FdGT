// Small tests for vec.h, written with the standard library only.
// Run all of them, or pass part of a test name as first argument (e.g. "broadcast")
// to run only the matching ones.
//
// Note: mixing sizes in a free operator (vec<3> + vec<4>) must not compile,
// so it cannot be checked here at runtime.

#include <cstring>
#include <iostream>
#include <sstream>

#include "../vec.h"

static bool passed;

template <std::size_t N>
static void expect(const vec<N>& got, const vec<N>& want) {
  for (std::size_t i = 0; i < N; i++) {
    if (got.e[i] != want.e[i]) {
      std::cout << "  expected " << want << ", got " << got << "\n";
      passed = false;
      return;
    }
  }
}

static void expect(float got, float want) {
  if (got != want) {
    std::cout << "  expected " << want << ", got " << got << "\n";
    passed = false;
  }
}

static int run(const char* name, void (*test)(), const char* filter) {
  if (filter && !std::strstr(name, filter))
    return 0;

  passed = true;
  test();
  std::cout << (passed ? "PASS " : "FAIL ") << name << "\n";
  return passed ? 0 : 1;
}

//
// Basic operations
//

static void zero_init() {
  vec<4> v{};
  expect(v, vec<4>{0, 0, 0, 0});
}

static void brace_init() {
  vec<4> v{1, 2, 3, 4};
  expect(v[0], 1);
  expect(v[3], 4);
}

static void add() {
  vec<3> a{1, 2, 3};
  vec<3> b{4, 5, 6};
  expect(a + b, vec<3>{5, 7, 9});
}

static void subtract() {
  vec<3> a{1, 2, 3};
  vec<3> b{4, 5, 6};
  expect(a - b, vec<3>{-3, -3, -3});
}

static void negate() {
  vec<3> a{1, -2, 3};
  expect(-a, vec<3>{-1, 2, -3});
  expect(a, vec<3>{1, -2, 3});
}

static void multiply_elementwise() {
  vec<3> a{1, 2, 3};
  vec<3> b{4, 5, 6};
  expect(a * b, vec<3>{4, 10, 18});
}

static void multiply_scalar() {
  vec<3> a{1, 2, 3};
  expect(2.0f * a, vec<3>{2, 4, 6});
  expect(a * 2.0f, vec<3>{2, 4, 6});
}

static void divide() {
  vec<3> a{1, 2, 3};
  expect(a / 2.0f, vec<3>{0.5f, 1, 1.5f});
}

static void dot_product() {
  vec<3> a{1, 2, 3};
  vec<3> b{4, 5, 6};
  expect(dot(a, b), 32);
}

static void print() {
  std::ostringstream out;
  out << vec<4>{1, 2, 3, 4};
  if (out.str() != "1 2 3 4 ") {
    std::cout << "  expected \"1 2 3 4 \", got \"" << out.str() << "\"\n";
    passed = false;
  }
}

//
// Broadcasting: missing components count as 0 (1 for *=), extra ones are ignored.
//

static void broadcast_add_smaller() {
  vec<4> a{1, 1, 1, 1};
  a += vec<3>{1, 2, 3};
  expect(a, vec<4>{2, 3, 4, 1});
}

static void broadcast_add_bigger() {
  vec<3> a{1, 2, 3};
  a += vec<4>{1, 1, 1, 99};
  expect(a, vec<3>{2, 3, 4});
}

static void broadcast_multiply_smaller() {
  vec<4> a{1, 2, 3, 4};
  a *= vec<2>{2, 3};
  expect(a, vec<4>{2, 6, 3, 4});
}

static void broadcast_multiply_bigger() {
  vec<3> a{1, 2, 3};
  a *= vec<4>{2, 2, 2, 99};
  expect(a, vec<3>{2, 4, 6});
}

static void broadcast_dot_smaller() {
  vec<3> a{1, 2, 3};
  expect(a.dot(vec<2>{1, 1}), 3);
}

static void broadcast_dot_bigger() {
  vec<3> a{1, 2, 3};
  expect(a.dot(vec<4>{1, 1, 1, 99}), 6);
}

int main(int argc, char** argv) {
  const char* filter = argc > 1 ? argv[1] : nullptr;
  int failed = 0;

  failed += run("zero_init", zero_init, filter);
  failed += run("brace_init", brace_init, filter);
  failed += run("add", add, filter);
  failed += run("subtract", subtract, filter);
  failed += run("negate", negate, filter);
  failed += run("multiply_elementwise", multiply_elementwise, filter);
  failed += run("multiply_scalar", multiply_scalar, filter);
  failed += run("divide", divide, filter);
  failed += run("dot_product", dot_product, filter);
  failed += run("print", print, filter);

  failed += run("broadcast_add_smaller", broadcast_add_smaller, filter);
  failed += run("broadcast_add_bigger", broadcast_add_bigger, filter);
  failed += run("broadcast_multiply_smaller", broadcast_multiply_smaller, filter);
  failed += run("broadcast_multiply_bigger", broadcast_multiply_bigger, filter);
  failed += run("broadcast_dot_smaller", broadcast_dot_smaller, filter);
  failed += run("broadcast_dot_bigger", broadcast_dot_bigger, filter);

  std::cout << failed << " failed\n";
  return failed;
}
