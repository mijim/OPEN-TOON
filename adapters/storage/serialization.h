#pragma once
#include "opentoon/document.h"
#include <string>
namespace opentoon {
[[nodiscard]] std::string serializeDocument(const Document&);
[[nodiscard]] Document deserializeDocument(const std::string&);
} // namespace opentoon
