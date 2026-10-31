#pragma once

// A minimal test harness: TEST(name) { CHECK(...); CHECK_NEAR(a, b, tolerance); }. Each failed check prints its
// file, line and expression; the exe's exit code is the number of failed tests.

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

namespace Check {

struct Test {
  const char *name;
  std::function<void()> body;
};

inline std::vector<Test> &Tests() {
  static std::vector<Test> tests;
  return tests;
}

inline int &Failures() {
  static int failures = 0;
  return failures;
}

struct Register {
  Register(const char *aName, std::function<void()> aBody) { Tests().push_back({aName, std::move(aBody)}); }
};

inline void Fail(const char *aFile, int aLine, const char *aWhat) {
  std::printf("  %s:%d: %s\n", aFile, aLine, aWhat);
  ++Failures();
}

} // namespace Check

#define CHECK_CONCAT_(a, b) a##b
#define CHECK_CONCAT(a, b) CHECK_CONCAT_(a, b)
#define TEST(name)                                                                                                     \
  static void name();                                                                                                  \
  static Check::Register CHECK_CONCAT(register_, name)(#name, name);                                                   \
  static void name()

#define CHECK(expr)                                                                                                    \
  do {                                                                                                                 \
    if (!(expr))                                                                                                       \
      Check::Fail(__FILE__, __LINE__, #expr);                                                                          \
  } while (0)

#define CHECK_NEAR(a, b, tolerance)                                                                                    \
  do {                                                                                                                 \
    double check_a = (a), check_b = (b);                                                                               \
    if (!(std::fabs(check_a - check_b) <= (tolerance))) {                                                              \
      char check_text[256];                                                                                            \
      std::snprintf(check_text, sizeof(check_text), "%s == %s (%g vs %g)", #a, #b, check_a, check_b);                  \
      Check::Fail(__FILE__, __LINE__, check_text);                                                                     \
    }                                                                                                                  \
  } while (0)
