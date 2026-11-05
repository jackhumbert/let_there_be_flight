// Unit tests for the parts of the plugin that need no game: ctest --test-dir build, or run
// ltbf_tests.exe [substring] to run only the tests whose name contains it.
#include "check.hpp"

#include <cstring>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  // the shared spdlog build has no default logger (the plugin installs its own): code under test logs to nothing
  spdlog::set_default_logger(spdlog::null_logger_mt("tests"));
  const char *filter = argc > 1 ? argv[1] : nullptr;
  int failed = 0, ran = 0;
  for (const auto &test : Check::Tests()) {
    if (filter && !std::strstr(test.name, filter)) {
      continue;
    }
    const int before = Check::Failures();
    test.body();
    const bool ok = Check::Failures() == before;
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", test.name);
    failed += ok ? 0 : 1;
    ++ran;
  }
  std::printf("%d tests, %d failed\n", ran, failed);
  return failed;
}
