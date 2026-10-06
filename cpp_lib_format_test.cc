#include <format>
#include <string>
#include <iostream>

#if !defined(__cpp_lib_format) || __cpp_lib_format < 202207L
#warning "Insufficient std::format support"
#endif

template <typename... Args>
void test(std::format_string<Args...> fmt, Args&&... args) {}

int main() {
#ifdef __cpp_lib_format
  std::cout << "_cpp_lib_format " << __cpp_lib_format << std::endl;
#endif
  std::string s = std::format("Hello {}", 42);
  test("Hello {}", 42);
  return 0;
}

