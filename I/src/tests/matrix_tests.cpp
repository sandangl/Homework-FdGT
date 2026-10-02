// Small tests for matrix.h, written with the standard library only.
//
// Note: a product with mismatched inner sizes (matrix<2, 3> * matrix<2, 3>)
// must not compile, so it cannot be checked here at runtime.

#include "test.h"
#include "../matrix.h"

template <std::size_t R, std::size_t C>
static void expect(const matrix<R, C>& got, const matrix<R, C>& want) {
  for (std::size_t i = 0; i < R; i++)
    expect(got.rows[i], want.rows[i]);
}

static matrix<2, 3> sample() {
  return {{{1, 2, 3}, {4, 5, 6}}};
}

//
// Basic operations
//

static void matrix_zero_init() {
  matrix<2, 3> m;
  expect(m, matrix<2, 3>{{{0, 0, 0}, {0, 0, 0}}});
}

static void matrix_brace_init() {
  matrix<2, 3> m = sample();
  expect(m[0][0], 1);
  expect(m[1][2], 6);
}

static void matrix_row_access() {
  matrix<2, 3> m = sample();
  m[0][1] = 7;
  expect(m[0], vec<3>{1, 7, 3});
  expect(m[1], vec<3>{4, 5, 6});
}

static void matrix_add() {
  expect(sample() + sample(), matrix<2, 3>{{{2, 4, 6}, {8, 10, 12}}});
}

static void matrix_subtract() {
  expect(sample() - sample(), matrix<2, 3>{});
}

static void matrix_negate() {
  matrix<2, 3> m = sample();
  expect(-m, matrix<2, 3>{{{-1, -2, -3}, {-4, -5, -6}}});
  expect(m, sample());
}

static void matrix_scalar_in_place() {
  matrix<4, 3> m{{{1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}}};
  m *= 2.0f;
  expect(m, matrix<4, 3>{{{2, 2, 2}, {2, 2, 2}, {2, 2, 2}, {2, 2, 2}}});
}

static void matrix_scalar_both_sides() {
  matrix<2, 3> want{{{2, 4, 6}, {8, 10, 12}}};
  expect(2.0f * sample(), want);
  expect(sample() * 2.0f, want);
}

static void matrix_divide() {
  expect(sample() / 2.0f, matrix<2, 3>{{{0.5f, 1, 1.5f}, {2, 2.5f, 3}}});
}

//
// Products
//

static void matrix_vector_times_matrix() {
  expect(vec<2>{1, 2} * sample(), vec<3>{9, 12, 15});
}

static void matrix_times_vector() {
  expect(sample() * vec<3>{1, 1, 1}, vec<2>{6, 15});
}

static void matrix_times_matrix_square() {
  matrix<2, 2> a{{{1, 2}, {3, 4}}};
  matrix<2, 2> b{{{5, 6}, {7, 8}}};
  expect(a * b, matrix<2, 2>{{{19, 22}, {43, 50}}});
}

static void matrix_times_matrix_rectangular() {
  matrix<3, 2> transposed{{{1, 4}, {2, 5}, {3, 6}}};
  expect(sample() * transposed, matrix<2, 2>{{{14, 32}, {32, 77}}});
}

static void matrix_hadamard() {
  matrix<2, 3> want{{{1, 4, 9}, {16, 25, 36}}};
  expect(sample().hadamard(sample()), want);
  expect(hadamard(sample(), sample()), want);
}

//
// Catmull-Rom: p(t) = (T * M) * G, with T = [t^3 t^2 t 1] and G = control points.
//

static matrix<4, 4> catmull_rom_basis() {
  return 0.5f * matrix<4, 4>{{{-1, 3, -3, 1}, {2, -5, 4, -1}, {-1, 0, 1, 0}, {0, 2, 0, 0}}};
}

static vec<3> catmull_rom(float t, const matrix<4, 3>& points) {
  vec<4> T{t * t * t, t * t, t, 1};
  return (T * catmull_rom_basis()) * points;
}

static void matrix_catmull_rom_endpoints() {
  matrix<4, 3> points{{{0, 0, 0}, {1, 1, 0}, {2, 0, 0}, {3, 1, 0}}};
  expect(catmull_rom(0, points), points[1]);
  expect(catmull_rom(1, points), points[2]);
}

static void matrix_catmull_rom_midpoint() {
  // Evenly spaced points on a line: the curve is the line itself.
  matrix<4, 3> points{{{0, 0, 1}, {1, 2, 1}, {2, 4, 1}, {3, 6, 1}}};
  expect(catmull_rom(0.5f, points), vec<3>{1.5f, 3, 1});
}

int run_matrix_tests(const char* filter) {
  int failed = 0;

  failed += run("matrix_zero_init", matrix_zero_init, filter);
  failed += run("matrix_brace_init", matrix_brace_init, filter);
  failed += run("matrix_row_access", matrix_row_access, filter);
  failed += run("matrix_add", matrix_add, filter);
  failed += run("matrix_subtract", matrix_subtract, filter);
  failed += run("matrix_negate", matrix_negate, filter);
  failed += run("matrix_scalar_in_place", matrix_scalar_in_place, filter);
  failed += run("matrix_scalar_both_sides", matrix_scalar_both_sides, filter);
  failed += run("matrix_divide", matrix_divide, filter);

  failed += run("matrix_vector_times_matrix", matrix_vector_times_matrix, filter);
  failed += run("matrix_times_vector", matrix_times_vector, filter);
  failed += run("matrix_times_matrix_square", matrix_times_matrix_square, filter);
  failed += run("matrix_times_matrix_rectangular", matrix_times_matrix_rectangular, filter);
  failed += run("matrix_hadamard", matrix_hadamard, filter);

  failed += run("matrix_catmull_rom_endpoints", matrix_catmull_rom_endpoints, filter);
  failed += run("matrix_catmull_rom_midpoint", matrix_catmull_rom_midpoint, filter);

  return failed;
}
