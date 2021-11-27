#include "util.h"

#include <codecvt>
#include <locale>

// I am not confident in the correctness of this implementation.
std::string wstring_to_string(not_null<cwzstring> wstring) {
  std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>, wchar_t>
      convert;
  return convert.to_bytes(wstring);
}
