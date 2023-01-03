#pragma once

/* C++ Support Library */

// static_cast to rvalue reference
#define MOV(...) \
  static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)

// static_cast to identity
// The extra && aren't necessary as discussed above, but make it more robust in case it's used with a non-reference.
#define FWD(...) \
  static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)