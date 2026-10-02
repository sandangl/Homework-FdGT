// Runs all tests. Pass part of a test name as first argument (e.g. "broadcast"
// or "matrix") to run only the matching ones.

#include <iostream>

int run_vec_tests(const char* filter);
int run_matrix_tests(const char* filter);
int run_spline_tests(const char* filter);

int main(int argc, char** argv) {
  const char* filter = argc > 1 ? argv[1] : nullptr;
  int failed = run_vec_tests(filter) + run_matrix_tests(filter) + run_spline_tests(filter);

  std::cout << failed << " failed\n";
  return failed;
}
