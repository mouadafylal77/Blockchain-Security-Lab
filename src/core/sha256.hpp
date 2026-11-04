#pragma once

#include <string>
#include <string_view>

namespace bsl {
std::string sha256(std::string_view text);
}
