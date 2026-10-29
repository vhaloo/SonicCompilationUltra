#pragma once
#include <string>
namespace scu {
std::string translationGlyphs();
int detectLanguage(int requested);
const char *tr(const char *key, int language);
} // namespace scu
