// Runs all tests. Pass part of a test name as first argument (e.g. "inner"
// or "boundary") to run only the matching ones.

#include <iostream>

int run_spotlight_tests(const char* filter);

int main(int argc, char** argv) {
  const char* filter = argc > 1 ? argv[1] : nullptr;
  int failed = run_spotlight_tests(filter);

  std::cout << failed << " failed\n";
  return failed;
}
