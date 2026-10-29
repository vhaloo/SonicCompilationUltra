#pragma once
#include <cstddef>
namespace scu {
enum class HelpPicture {
    View,
    Zoom,
    Camera,
    Hud,
    Reminder,
    Quest,
    Background,
    Performance,
    Audio,
    Controller,
    Rewind,
    Save,
    Assist,
    Library
};
struct SettingHelp {
    const char *key;
    HelpPicture picture;
    const char *text[4];
};
const SettingHelp *settingHelp(const char *key);
const SettingHelp *allSettingHelp(size_t &count);
} // namespace scu
