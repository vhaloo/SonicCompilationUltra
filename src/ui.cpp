#include "ui.h"
#include "quest.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <chrono>
#include <ctime>
#include <cfloat>
namespace scu {
void App::settingGuide(bool compact) {
    const auto *help = settingHelp(guideKey.c_str());
    if (!help)
        help = settingHelp("view");
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.9f, 0.94f, 1));
    ImGui::TextWrapped("%s", tr(help->key, language));
    ImGui::PopStyleColor();
    if (compact) {
        ImGui::TextWrapped("%s", help->text[std::clamp(language, 0, 3)]);
        ImGui::Spacing();
    }
    float width = std::max(40.0f, ImGui::GetContentRegionAvail().x);
    float height = std::min(compact ? 100.0f : 180.0f * uiFactor, width * 0.60f);
    auto start = ImGui::GetCursorScreenPos();
    auto draw = ImGui::GetWindowDrawList();
    auto point = [&](float x, float y) {
        return ImVec2(start.x + x * width, start.y + y * height);
    };
    const ImU32 cyan = IM_COL32(85, 219, 228, 255), gold = IM_COL32(255, 213, 104, 255),
                muted = IM_COL32(74, 103, 128, 255), ink = IM_COL32(12, 28, 47, 255);
    auto rect = [&](float x, float y, float w, float h, ImU32 color, bool filled = true) {
        if (filled)
            draw->AddRectFilled(point(x, y), point(x + w, y + h), color, 3);
        else
            draw->AddRect(point(x, y), point(x + w, y + h), color, 3, 0, 2);
    };
    auto line = [&](float x, float y, float xx, float yy, ImU32 color, float thickness = 2) {
        draw->AddLine(point(x, y), point(xx, yy), color, thickness);
    };
    rect(0, 0, 1, 1, ink);
    switch (help->picture) {
    case HelpPicture::View:
    case HelpPicture::Camera:
    case HelpPicture::Zoom:
    case HelpPicture::Hud: {
        for (int i = 0; i < 12; ++i) {
            float y = 0.56f + float((i * 7) % 5) * 0.045f;
            rect(0.035f + i * 0.078f, y, 0.074f, 0.90f - y, IM_COL32(31, 106, 121, 255));
            line(0.035f + i * 0.078f, y, 0.109f + i * 0.078f, y, cyan);
        }
        for (int i = 0; i < 3; ++i)
            rect(0.16f + i * 0.28f, 0.23f + i * 0.06f, 0.15f, 0.06f, muted);
        rect(0.06f, 0.10f, 0.88f, 0.81f, muted, false);
        float focusWidth = help->picture == HelpPicture::Zoom
                               ? float(std::clamp(0.48 / settings.pixelScale(), 0.16, 0.86))
                               : 0.40f;
        rect(0.5f - focusWidth / 2, 0.16f, focusWidth, 0.65f, gold, false);
        draw->AddCircleFilled(point(0.50f, 0.50f), std::max(3.0f, height * 0.045f), gold);
        if (help->picture == HelpPicture::Camera) {
            line(0.54f, 0.50f, 0.77f, 0.50f, cyan);
            draw->AddTriangleFilled(point(0.80f, 0.50f), point(0.75f, 0.45f), point(0.75f, 0.55f),
                                    cyan);
        }
        if (help->picture == HelpPicture::Hud) {
            rect(settings.hud == 2   ? 0.07f
                 : settings.hud == 1 ? 0.54f
                                     : 0.35f,
                 0.19f, 0.24f, 0.15f, cyan);
            rect(0.73f, 0.65f, 0.18f, 0.19f, cyan, false);
        }
        break;
    }
    case HelpPicture::Pinball: {
        ImVec2 a = point(.20f, .70f), b = point(.62f, .79f), c = point(.80f, .25f),
               d = point(.43f, .18f);
        draw->AddQuadFilled(a, b, c, d, muted);
        draw->AddQuad(a, b, c, d, cyan, 3);
        rect(.44f, .06f, .37f, .15f, cyan, false);
        for (int i = 0; i < 4; ++i) {
            float x = .43f + (i % 2) * .16f, y = .32f + (i / 2) * .17f;
            draw->AddCircle(point(x, y), height * .05f, gold, 18, 2);
        }
        line(.25f, .71f, .25f, .94f, muted, 5);
        line(.62f, .80f, .62f, .97f, muted, 5);
        line(.77f, .31f, .77f, .71f, muted, 4);
        line(.35f, .67f, .43f, .63f, gold, 4);
        line(.46f, .65f, .52f, .71f, gold, 4);
        draw->AddCircleFilled(point(.49f, .48f), height * .022f, gold);
        break;
    }
    case HelpPicture::Crt: {
        for (int i = 0; i < 12; ++i)
            rect(.02f + i * .082f, .42f + (i % 3) * .06f, .075f, .38f, muted);
        rect(.20f, .15f, .60f, .62f, ink);
        rect(.20f, .15f, .60f, .62f, cyan, false);
        rect(.25f, .21f, .50f, .44f, muted);
        for (int i = 0; i < 12; ++i)
            line(.26f, .23f + i * .033f, .74f, .23f + i * .033f, ink, 1);
        draw->AddCircleFilled(point(.50f, .43f), height * .045f, gold);
        rect(.30f, .70f, .31f, .025f, muted);
        draw->AddCircle(point(.71f, .71f), height * .025f, gold, 16, 2);
        rect(.28f, .79f, .44f, .16f, muted, false);
        break;
    }
    case HelpPicture::Resize: {
        rect(.10f, .12f, .80f, .65f, cyan, false);
        rect(.31f, .30f, .38f, .30f, gold, false);
        line(.32f, .84f, .16f, .84f, cyan, 3);
        line(.68f, .84f, .84f, .84f, cyan, 3);
        draw->AddCircleFilled(point(.32f, .84f), height * .05f, gold);
        draw->AddCircleFilled(point(.68f, .84f), height * .05f, gold);
        draw->AddTriangleFilled(point(.10f, .84f), point(.18f, .78f), point(.18f, .90f), cyan);
        draw->AddTriangleFilled(point(.90f, .84f), point(.82f, .78f), point(.82f, .90f), cyan);
        break;
    }
    case HelpPicture::Wall: {
        rect(.07f, .09f, .86f, .70f, muted, false);
        line(.03f, .80f, .97f, .80f, cyan, 3);
        rect(.12f, .24f, .70f, .49f, ink);
        rect(.12f, .24f, .70f, .49f, cyan, false);
        for (int i = 0; i < 12; ++i) {
            float x = .14f + i * .055f, y = .58f - (i % 3) * .055f;
            rect(x, y, .047f, .71f - y, muted);
            line(x, y, x + .047f, y, cyan);
        }
        draw->AddCircleFilled(point(.30f, .45f), height * .025f, gold);
        line(.87f, .24f, .87f, .73f, gold, 2);
        line(.85f, .24f, .89f, .24f, gold, 2);
        line(.85f, .73f, .89f, .73f, gold, 2);
        draw->AddText(point(.79f, .12f), gold, "2 m");
        rect(.36f, .87f, .18f, .08f, cyan, false);
        line(.37f, .86f, .29f, .75f, muted);
        line(.53f, .86f, .66f, .75f, muted);
        rect(.68f, .11f, .07f, .06f, gold, false);
        draw->AddCircle(point(.715f, .10f), height * .022f, gold, 12, 2);
        break;
    }
    case HelpPicture::Quest: {
        rect(0.05f, 0.12f, 0.90f, 0.78f, muted, false);
        line(0.05f, 0.90f, 0.32f, 0.61f, muted);
        line(0.95f, 0.90f, 0.68f, 0.61f, muted);
        rect(0.26f, 0.20f, 0.48f, 0.42f, ink);
        rect(0.26f, 0.20f, 0.48f, 0.42f, cyan, false);
        for (int i = 0; i < 5; ++i)
            rect(0.30f + i * 0.08f, 0.45f - (i % 2) * 0.08f, 0.07f, 0.12f, muted);
        draw->AddCircleFilled(point(0.50f, 0.40f), height * 0.035f, gold);
        rect(0.38f, 0.76f, 0.24f, 0.12f, cyan, false);
        line(0.18f, 0.74f, 0.50f, 0.40f, gold);
        draw->AddCircleFilled(point(0.18f, 0.74f), height * 0.04f, gold);
        break;
    }
    case HelpPicture::Reminder: {
        rect(0.05f, 0.08f, 0.90f, 0.84f, muted, false);
        for (int i = 0; i < 9; ++i)
            rect(0.08f + i * 0.10f, 0.65f + (i % 3) * 0.04f, 0.08f, 0.18f, muted);
        bool right = settings.hintCorner % 2 == 0, bottom = settings.hintCorner < 2;
        float x = right ? 0.63f : 0.08f, y = bottom ? 0.78f : 0.12f;
        rect(x, y, 0.29f, 0.10f, ink);
        rect(x, y, 0.29f, 0.10f, cyan, false);
        line(x + 0.025f, y + 0.05f, x + 0.26f, y + 0.05f, gold);
        draw->AddCircleFilled(point(0.5f, 0.52f), height * 0.04f, gold);
        break;
    }
    case HelpPicture::Background:
        for (int i = 0; i < 7; ++i) {
            float x = (i + 0.5f) / 7;
            draw->AddCircleFilled(point(x, 0.5f), height * (0.30f + (i % 3) * 0.035f),
                                  IM_COL32(22, 56 + i * 7, 92 + i * 6, 255));
        }
        rect(0.30f, 0.19f, 0.40f, 0.62f, ink);
        line(0.31f, 0.68f, 0.69f, 0.68f, cyan, 5);
        draw->AddCircleFilled(point(0.5f, 0.57f), height * 0.055f, gold);
        break;
    case HelpPicture::Controller:
        rect(0.19f, 0.27f, 0.62f, 0.47f, muted);
        rect(0.21f, 0.19f, 0.18f, 0.06f, cyan);
        rect(0.61f, 0.19f, 0.18f, 0.06f, cyan);
        line(0.26f, 0.50f, 0.41f, 0.50f, gold, 4);
        line(0.335f, 0.37f, 0.335f, 0.63f, gold, 4);
        for (auto p :
             {ImVec2(0.68f, 0.38f), ImVec2(0.75f, 0.5f), ImVec2(0.68f, 0.62f), ImVec2(0.61f, 0.5f)})
            draw->AddCircleFilled(point(p.x, p.y), height * 0.035f, cyan);
        break;
    case HelpPicture::Audio:
    case HelpPicture::Rewind:
        for (int i = 0; i < 40; ++i) {
            float v = float(0.06 + std::abs(std::sin(i * 0.67)) * 0.15);
            line(0.10f + i * 0.02f, 0.54f - v, 0.10f + i * 0.02f, 0.54f + v, cyan);
        }
        if (help->picture == HelpPicture::Rewind) {
            for (float x : {0.42f, 0.58f})
                draw->AddTriangleFilled(point(x - 0.09f, 0.20f), point(x, 0.11f), point(x, 0.29f),
                                        gold);
            line(0.12f, 0.84f, 0.88f, 0.84f, muted);
            draw->AddCircleFilled(point(0.39f, 0.84f), height * 0.035f, gold);
        }
        break;
    case HelpPicture::Performance:
        for (int i = 0; i < 3; ++i) {
            rect(0.16f + i * 0.25f, 0.72f - i * 0.19f, 0.16f, 0.15f + i * 0.19f,
                 i == 1 ? gold : cyan);
            line(0.18f + i * 0.25f, 0.92f, 0.30f + i * 0.25f, 0.92f, muted);
        }
        break;
    case HelpPicture::Assist:
        draw->AddCircle(point(0.50f, 0.48f), height * 0.29f, cyan, 40, 3);
        draw->AddCircleFilled(point(0.50f, 0.48f), height * 0.10f, gold);
        for (int i = 0; i < 3; ++i)
            line(0.18f, 0.36f + i * 0.12f, 0.32f, 0.36f + i * 0.12f, gold);
        break;
    case HelpPicture::Save:
    case HelpPicture::Library:
        for (int i = 0; i < 9; ++i) {
            float x = 0.19f + (i % 3) * 0.22f, y = 0.14f + (i / 3) * 0.25f;
            rect(x, y, 0.18f, 0.20f, i == settings.slot - 1 ? gold : muted, false);
            if (i % 3 != 2)
                rect(x + 0.04f, y + 0.05f, 0.10f, 0.04f, cyan);
        }
        break;
    }
    ImGui::Dummy({width, height});
    ImGui::TextDisabled("%s", tr("guidedrawing", language));
    ImGui::Spacing();
    if (!compact)
        ImGui::TextWrapped("%s", help->text[std::clamp(language, 0, 3)]);
    if (!compact) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextWrapped("%s", tr(
#ifdef SCU_QUEST
                                     "questguidehint",
#else
                                     "guidehint",
#endif
                                     language));
    }
}
void App::control(const char *key) {
    if (settingHelp(key) && !guideOpen && !guideRequested &&
        (ImGui::IsItemFocused() || ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)))
        guideKey = key;
#ifdef SCU_UI_TESTS
    controls.emplace(key, ImGui::GetItemID());
    auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
    controlRects.emplace(key, ImVec4(min.x, min.y, max.x, max.y));
#endif
}
bool App::button(const char *key, const ImVec2 &size) {
    std::string label = std::string(tr(key, language)) + "###" + key;
    bool result = ImGui::Button(label.c_str(), size);
    control(key);
    return result;
}
void App::nextButton(const char *key) {
    float width = ImGui::CalcTextSize(tr(key, language)).x + ImGui::GetStyle().FramePadding.x * 2;
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width <= right)
        ImGui::SameLine();
}
bool App::tick(const char *key, bool &value) {
    ImGui::BeginGroup();
    bool changed = ImGui::Checkbox((std::string("##") + key).c_str(), &value);
    control(key);
    ImGui::SameLine();
    ImGui::TextWrapped("%s", tr(key, language));
    ImGui::EndGroup();
    return changed;
}
bool App::choice(const char *key, int &value, std::initializer_list<const char *> keys) {
    ImGui::TextWrapped("%s", tr(key, language));
    ImGui::SetNextItemWidth(-1);
    std::vector<const char *> labels;
    for (auto k : keys)
        labels.push_back(tr(k, language));
    bool changed =
        ImGui::Combo((std::string("##") + key).c_str(), &value, labels.data(), int(labels.size()));
    control(key);
    return changed;
}
bool App::slider(const char *key, int &value, int lo, int hi, const char *format) {
    ImGui::TextWrapped("%s", tr(key, language));
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::SliderInt((std::string("##") + key).c_str(), &value, lo, hi, format,
                                    ImGuiSliderFlags_NoInput);
    control(key);
    return changed;
}
void App::initialise(const fs::path &base, bool hidden) {
    SDL_Log("SCU startup: local settings and library");
    basePath = base;
    fs::create_directories(library.dataDir);
    settings.load(library.dataDir);
#ifdef SCU_QUEST
    settings.renderer = 0;
    settings.vsync = false;
    settings.fullscreen = false;
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
    library.load();
    if (menuVerificationOnly) {
        size_t readable = 0;
        for (const auto &entry : library.entries) {
            try {
                // File checks only: Core::load remains locked throughout.
                auto bytes = library.bytes(entry);
                if (!bytes.empty())
                    ++readable;
            } catch (const std::exception &error) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SCU library file unavailable: %s (%s)",
                            entry.id.c_str(), error.what());
            }
        }
        SDL_Log("SCU library file audit: entries=%zu readable=%zu cartridgeExecution=0",
                library.entries.size(), readable);
    }
    language = detectLanguage(settings.language);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    window = SDL_CreateWindow("Sonic Compilation Ultra", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 1280, 720,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
#ifdef SCU_QUEST
                                  SDL_WINDOW_OPENGL |
#endif
                                  (hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN));
    if (!window)
        throw std::runtime_error(SDL_GetError());
    SDL_SetWindowMinimumSize(window, 320, 240);
    renderer = SDL_CreateRenderer(window, -1,
                                  settings.renderer
                                      ? SDL_RENDERER_SOFTWARE
                                      : SDL_RENDERER_ACCELERATED |
                                            (settings.vsync ? SDL_RENDERER_PRESENTVSYNC : 0));
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
        throw std::runtime_error(SDL_GetError());
    SDL_RendererInfo rendererInfo{};
    SDL_GetRendererInfo(renderer, &rendererInfo);
    SDL_Log("SCU renderer: %s; max texture %d x %d", rendererInfo.name,
            rendererInfo.max_texture_width, rendererInfo.max_texture_height);
    IMGUI_CHECKVERSION();
    uiContext = ImGui::CreateContext();
    SDL_Log("SCU startup: interface font at %s", base.u8string().c_str());
    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigNavEscapeClearFocusItem = false;
    io.ConfigNavEscapeClearFocusWindow = false;
    io.IniFilename = nullptr;
    static ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    auto translated = translationGlyphs();
    builder.AddText(translated.c_str());
    size_t helpCount = 0;
    const auto *helpEntries = allSettingHelp(helpCount);
    for (size_t i = 0; i < helpCount; ++i)
        for (auto text : helpEntries[i].text)
            builder.AddText(text);
    static const ImWchar punctuation[] = {0x2000, 0x206F, 0};
    builder.AddRanges(punctuation);
    builder.AddChar(0x2212);
    builder.AddChar(0x00D7);
    builder.BuildRanges(&ranges);
    ImFontConfig fontCfg;
    auto font = (basePath / "assets/SCUInterface.ttf").u8string();
    if (fs::exists(fs::u8path(font)))
        io.Fonts->AddFontFromFileTTF(font.c_str(), 24, &fontCfg, ranges.Data);
    else
        io.Fonts->AddFontDefault();
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 16;
    style.FrameRounding = 8;
    style.ChildRounding = 10;
    style.PopupRounding = 8;
    style.WindowPadding = {22, 18};
    style.FramePadding = {10, 8};
    style.ItemSpacing = {12, 10};
    style.ScrollbarSize = 13;
    style.Colors[ImGuiCol_WindowBg] = {0.025f, 0.047f, 0.08f, 0.98f};
    style.Colors[ImGuiCol_ChildBg] = {0.04f, 0.075f, 0.12f, 0.96f};
    style.Colors[ImGuiCol_FrameBg] = {0.075f, 0.12f, 0.18f, 1};
    style.Colors[ImGuiCol_Button] = {0.04f, 0.29f, 0.43f, 1};
    style.Colors[ImGuiCol_ButtonHovered] = {0.07f, 0.49f, 0.65f, 1};
    style.Colors[ImGuiCol_ButtonActive] = {0.05f, 0.56f, 0.65f, 1};
    style.Colors[ImGuiCol_CheckMark] = {0.28f, 0.89f, 0.85f, 1};
    style.Colors[ImGuiCol_SliderGrab] = {0.2f, 0.78f, 0.87f, 1};
    style.Colors[ImGuiCol_Header] = {0.05f, 0.28f, 0.4f, 1};
    style.Colors[ImGuiCol_TabSelected] = {0.06f, 0.36f, 0.49f, 1};
    style.Colors[ImGuiCol_Text] = {0.87f, 0.94f, 0.98f, 1};
    style.Colors[ImGuiCol_NavCursor] = {1, 0.85f, 0.25f, 1};
    baseStyle = style;
    uiPlatformReady = ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    if (!uiPlatformReady)
        throw std::runtime_error("Cannot initialize the SDL interface backend");
    uiRendererReady = ImGui_ImplSDLRenderer2_Init(renderer);
    if (!uiRendererReady)
        throw std::runtime_error("Cannot initialize the interface renderer");
    SDL_Log("SCU startup: interface ready");
    refreshControllers();
    if (!hidden)
        applyDisplay();
}
App::~App() {
    try {
        flushSettings();
    } catch (...) {
    }
    core.unload();
    for (auto p : pads)
        if (p)
            SDL_GameControllerClose(p);
    if (uiContext) {
        ImGui::SetCurrentContext(uiContext);
        if (uiRendererReady)
            ImGui_ImplSDLRenderer2_Shutdown();
        if (uiPlatformReady)
            ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(uiContext);
    }
    if (worldTexture)
        SDL_DestroyTexture(worldTexture);
    if (nativeTexture)
        SDL_DestroyTexture(nativeTexture);
    if (bgTexture)
        SDL_DestroyTexture(bgTexture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
}
void App::refreshControllers() {
    // The UI and cartridge input share the same owned controller handles.
    ImGui_ImplSDL2_SetGamepadMode(ImGui_ImplSDL2_GamepadMode_Manual, nullptr, 0);
    for (auto &p : pads)
        if (p && !SDL_GameControllerGetAttached(p)) {
            SDL_GameControllerClose(p);
            p = nullptr;
        }
#ifdef SCU_QUEST
    if (!pads[0] && pads[1]) {
        std::swap(pads[0], pads[1]);
        activePad = 0;
    }
    // Touch is player one even if a Bluetooth pad was connected before startup.
    if (settings.questTouch) {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            const char *name = SDL_JoystickNameForIndex(i);
            if (!name || std::strcmp(name, "SCU Quest Touch") || !SDL_IsGameController(i))
                continue;
            auto instance = SDL_JoystickGetDeviceInstanceID(i);
            auto same = [&](SDL_GameController *pad) {
                return pad &&
                       SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad)) == instance;
            };
            if (same(pads[1])) {
                std::swap(pads[0], pads[1]);
                activePad = 1 - activePad;
            } else if (!same(pads[0])) {
                auto preferred = SDL_GameControllerOpen(i);
                if (preferred) {
                    if (pads[1])
                        SDL_GameControllerClose(pads[1]);
                    pads[1] = pads[0];
                    pads[0] = preferred;
                    activePad = 0;
                }
            }
            break;
        }
    }
#endif
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i))
            continue;
        auto instance = SDL_JoystickGetDeviceInstanceID(i);
        bool already = false;
        for (auto p : pads)
            if (p && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(p)) == instance)
                already = true;
        if (already)
            continue;
        for (auto &p : pads)
            if (!p) {
                p = SDL_GameControllerOpen(i);
                break;
            }
    }
    syncNavigationController();
}
void App::syncNavigationController() {
    if (!pads[activePad])
        activePad = pads[0] ? 0 : 1;
}
void App::updateNavigationController() {
    auto &io = ImGui::GetIO();
    auto pad = pads[activePad];
    static const ImGuiKey keys[] = {ImGuiKey_GamepadFaceDown,
                                    ImGuiKey_GamepadFaceRight,
                                    ImGuiKey_GamepadFaceLeft,
                                    ImGuiKey_None, // Y/Triangle belongs to the illustrated guide.
                                    ImGuiKey_None,
                                    ImGuiKey_None,
                                    ImGuiKey_GamepadStart,
                                    ImGuiKey_GamepadL3,
                                    ImGuiKey_GamepadR3,
                                    ImGuiKey_GamepadL1,
                                    ImGuiKey_GamepadR1,
                                    ImGuiKey_GamepadDpadUp,
                                    ImGuiKey_GamepadDpadDown,
                                    ImGuiKey_GamepadDpadLeft,
                                    ImGuiKey_GamepadDpadRight};
    bool held = false;
    for (int n = 0; n < int(sizeof keys / sizeof keys[0]); ++n)
        if (keys[n] != ImGuiKey_None && pad &&
            SDL_GameControllerGetButton(pad, SDL_GameControllerButton(n)))
            held = true;
    if (!held)
        releaseNavigation = false;
    bool enabled = menu && pad && !releaseNavigation;
    if (pad)
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    for (int n = 0; n < int(sizeof keys / sizeof keys[0]); ++n)
        if (keys[n] != ImGuiKey_None)
            io.AddKeyEvent(
                keys[n], enabled && SDL_GameControllerGetButton(pad, SDL_GameControllerButton(n)));
    auto analog = [&](ImGuiKey key, int value, int dead) {
        float amount = enabled ? std::clamp(float(value - dead) / (32767 - dead), 0.0f, 1.0f) : 0;
        io.AddKeyAnalogEvent(key, amount > 0, amount);
    };
    int x = pad ? SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) : 0,
        y = pad ? SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) : 0;
    analog(ImGuiKey_GamepadLStickLeft, -x, settings.deadzone);
    analog(ImGuiKey_GamepadLStickRight, x, settings.deadzone);
    analog(ImGuiKey_GamepadLStickUp, -y, settings.deadzone);
    analog(ImGuiKey_GamepadLStickDown, y, settings.deadzone);
    analog(ImGuiKey_GamepadL2,
           pad ? SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) : 0, 3000);
    analog(ImGuiKey_GamepadR2,
           pad ? SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) : 0, 3000);
}
void App::toggleMenu() {
    if (!core.loaded) {
        menu = true;
        focusMenu = true;
        return;
    }
    menu = !menu;
    if (!menu) {
        hintShownAt = noticeShownAt = overlayNow();
    }
    focusMenu = menu;
    releaseNavigation = menu;
    releaseInput = !menu;
    pendingButtons[0] = pendingButtons[1] = 0;
    core.clearAudio();
    dirty = true;
}
void App::applyDisplay() {
#ifdef SCU_QUEST
    dirty = true;
    return;
#endif
    if (!previousFullscreen && settings.fullscreen) {
        SDL_GetWindowPosition(window, &windowedBounds.x, &windowedBounds.y);
        SDL_GetWindowSize(window, &windowedBounds.w, &windowedBounds.h);
    }
    SDL_SetWindowFullscreen(window, 0);
    SDL_SetWindowBordered(window, settings.fullscreen ? SDL_FALSE : SDL_TRUE);
    if (settings.fullscreen) {
        SDL_Rect r{};
        int display = settings.display < 0 ? SDL_GetWindowDisplayIndex(window) : settings.display;
        display = std::clamp(display, 0, std::max(0, SDL_GetNumVideoDisplays() - 1));
        SDL_GetDisplayBounds(std::max(0, display), &r);
        if (settings.span) {
            for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
                SDL_Rect b{}, u;
                SDL_GetDisplayBounds(i, &b);
                SDL_UnionRect(&r, &b, &u);
                r = u;
            }
        }
        SDL_SetWindowPosition(window, r.x, r.y);
        SDL_SetWindowSize(window, r.w, r.h);
    } else if (previousFullscreen) {
        SDL_Rect bounds{};
        SDL_GetDisplayUsableBounds(std::max(0, SDL_GetWindowDisplayIndex(window)), &bounds);
        SDL_SetWindowSize(window, std::max(320, std::min(windowedBounds.w, bounds.w)),
                          std::max(240, std::min(windowedBounds.h, bounds.h)));
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    previousFullscreen = settings.fullscreen;
    SDL_RenderSetVSync(renderer, settings.vsync ? 1 : 0);
    dirty = true;
}
void App::error(const std::exception &e) {
    notify(std::string(tr("error", language)) + ": " + e.what());
}
void App::notify(std::string text) {
    notice = std::move(text);
    lastOverlayNotice = notice;
    noticeShownAt = overlayNow();
}
void App::import(const fs::path &path, bool folder) {
    try {
        int n = folder ? library.importFolder(path) : library.importFile(path);
        notify(std::string(tr(n ? "imported" : "noimport", language)) +
               (n ? " (" + std::to_string(n) + ")" : ""));
        settings.lastDirectory = (folder ? path : path.parent_path()).u8string();
        dirty = true;
    } catch (const std::exception &e) {
        error(e);
    }
}
bool App::play(const std::string &id) {
    if (menuVerificationOnly) {
        notify("Menu verification: cartridge execution is disabled.");
        SDL_Log("SCU menu verification: cartridge launch blocked");
        return false;
    }
    if (!core.load(id)) {
        notify(std::string(tr("error", language)) + ": " + core.status);
        return false;
    }
    menu = false;
    releaseInput = true;
    dirty = true;
    notice.clear();
    hintShownAt = overlayNow();
    backgroundStage = -1;
    backgroundFrozen = false;
    return true;
}
void App::beginBrowser(bool folder) {
    folderMode = folder;
    browserOpen = true;
    focusBrowser = true;
    browsePath =
        settings.lastDirectory.empty() ? fs::current_path().u8string() : settings.lastDirectory;
    std::error_code ec;
    if (!fs::is_directory(fs::u8path(browsePath), ec))
        browsePath = fs::current_path().u8string();
    snprintf(pathBuffer, sizeof pathBuffer, "%s", browsePath.c_str());
}
void App::flushSettings() {
    if (dirty && !menuVerificationOnly) {
        settings.save(library.dataDir);
        dirty = false;
    }
}
void App::event(const SDL_Event &e) {
    ImGui_ImplSDL2_ProcessEvent(&e);
    if (e.type == SDL_QUIT) {
        if (settings.confirmQuit && core.loaded) {
            requestAction(1);
        } else
            running = false;
    }
    if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_CONTROLLERDEVICEREMOVED)
        refreshControllers();
    if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
            focused = true;
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            focused = false;
            pendingButtons[0] = pendingButtons[1] = 0;
            core.clearAudio();
        }
        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            backgroundStage = -1;
        }
    }
    if (e.type == SDL_DROPFILE) {
        if (e.drop.file) {
            import(fs::u8path(e.drop.file));
            SDL_free(e.drop.file);
        }
        menu = true;
        tab = 0;
    }
    if (e.type == SDL_CONTROLLERBUTTONDOWN ||
        (e.type == SDL_CONTROLLERAXISMOTION && std::abs(int(e.caxis.value)) > settings.deadzone)) {
        padUsed = true;
        for (int p = 0; p < 2; ++p)
            if (pads[p] &&
                SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pads[p])) == e.cdevice.which)
                activePad = p;
        syncNavigationController();
    }
    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_KEYDOWN)
        padUsed = false;
    if (menu && !releaseNavigation &&
        (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP)) {
        // Preserve taps that begin and end between display frames. The platform
        // backend also polls held buttons; ImGui deduplicates those state events.
        static const ImGuiKey keys[] = {ImGuiKey_GamepadFaceDown,
                                        ImGuiKey_GamepadFaceRight,
                                        ImGuiKey_GamepadFaceLeft,
                                        ImGuiKey_None,
                                        ImGuiKey_None,
                                        ImGuiKey_None,
                                        ImGuiKey_GamepadStart,
                                        ImGuiKey_GamepadL3,
                                        ImGuiKey_GamepadR3,
                                        ImGuiKey_GamepadL1,
                                        ImGuiKey_GamepadR1,
                                        ImGuiKey_GamepadDpadUp,
                                        ImGuiKey_GamepadDpadDown,
                                        ImGuiKey_GamepadDpadLeft,
                                        ImGuiKey_GamepadDpadRight};
        auto button = e.cbutton.button;
        if (button < sizeof keys / sizeof keys[0] && keys[button] != ImGuiKey_None)
            ImGui::GetIO().AddKeyEvent(keys[button], e.type == SDL_CONTROLLERBUTTONDOWN);
    }
    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        auto b = e.cbutton.button;
        if (b == SDL_CONTROLLER_BUTTON_BACK) {
            if (menu)
                selectRequested = true;
            else
                toggleMenu();
            return;
        }
        if (menu && b == SDL_CONTROLLER_BUTTON_B)
            backRequested = true;
        if (menu && b == SDL_CONTROLLER_BUTTON_Y && !browserOpen && !pathEditor && !actionPopup)
            guideRequested = true;
        if (!menu && core.loaded && settings.quickControls && pads[activePad] &&
            SDL_GameControllerGetButton(pads[activePad], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
            auto scene = core.world.inspect(core.snapshot, core.game);
            if (b == SDL_CONTROLLER_BUTTON_Y)
                settings.view = settings.view == 2 ? 0 : 2;
            else if (b == SDL_CONTROLLER_BUTTON_DPAD_UP || b == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                settings.view = 0;
                settings.stepZoom(b == SDL_CONTROLLER_BUTTON_DPAD_UP ? 1 : -1);
            } else if (b == SDL_CONTROLLER_BUTTON_X) {
                if (settings.camera == 2)
                    settings.camera = 0;
                else {
                    if (settings.view == 2) {
                        settings.view = 0;
                        settings.left = int(scene.playerX - drawableW / settings.pixelScale() / 2);
                        settings.top = int(scene.playerY - drawableH / settings.pixelScale() / 2);
                        settings.camera = 2;
                    } else
                        core.world.freeze(scene, settings);
                }
            } else if (b == SDL_CONTROLLER_BUTTON_B) {
                settings.view = 0;
                settings.camera = 1;
            } else if (b == SDL_CONTROLLER_BUTTON_START)
                screenshotRequested = true;
            dirty = true;
            pendingButtons[0] = pendingButtons[1] = 0;
            return;
        }
        if (!menu && core.loaded && !releaseInput) {
            int bits[] = {RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_B,
                          RETRO_DEVICE_ID_JOYPAD_A};
            if (settings.jumpLayout == 1) {
                bits[0] = RETRO_DEVICE_ID_JOYPAD_A;
                bits[1] = RETRO_DEVICE_ID_JOYPAD_Y;
                bits[2] = RETRO_DEVICE_ID_JOYPAD_B;
            } else if (settings.jumpLayout == 2) {
                bits[0] = RETRO_DEVICE_ID_JOYPAD_A;
                bits[1] = RETRO_DEVICE_ID_JOYPAD_B;
                bits[2] = RETRO_DEVICE_ID_JOYPAD_Y;
            }
            int bit = b == SDL_CONTROLLER_BUTTON_X       ? bits[0]
                      : b == SDL_CONTROLLER_BUTTON_A     ? bits[1]
                      : b == SDL_CONTROLLER_BUTTON_B     ? bits[2]
                      : b == SDL_CONTROLLER_BUTTON_START ? RETRO_DEVICE_ID_JOYPAD_START
                                                         : -1;
            if (bit >= 0)
                pendingButtons[activePad] |= uint16_t(1u << bit);
        }
    }
    if (e.type == SDL_KEYDOWN && !e.key.repeat) {
        auto k = e.key.keysym.sym;
        auto mod = e.key.keysym.mod;
        if (menu && k == SDLK_F1 && !browserOpen && !pathEditor && !actionPopup) {
            guideRequested = true;
            return;
        }
        if (k == SDLK_ESCAPE || k == SDLK_F10) {
            if (menu) {
                if (k == SDLK_F10)
                    selectRequested = true;
                else
                    backRequested = true;
            } else if (core.loaded)
                toggleMenu();
            else
                menu = true;
            return;
        }
        if (k == SDLK_RETURN && (mod & KMOD_ALT)) {
            settings.fullscreen = !settings.fullscreen;
            applyDisplay();
            return;
        }
        if (!menu && core.loaded) {
            // Keep a short press until the next native tick, even if the display
            // loop observes its down and up events within one refresh.
            auto scan = e.key.keysym.scancode;
            auto tap = [&](int player, SDL_Scancode expected, unsigned bit) {
                if (scan == expected)
                    pendingButtons[player] |= uint16_t(1u << bit);
            };
            if (!(mod & (KMOD_CTRL | KMOD_ALT))) {
                tap(0, SDL_SCANCODE_UP, RETRO_DEVICE_ID_JOYPAD_UP);
                tap(0, SDL_SCANCODE_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN);
                tap(0, SDL_SCANCODE_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT);
                tap(0, SDL_SCANCODE_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT);
                tap(0, SDL_SCANCODE_Z, RETRO_DEVICE_ID_JOYPAD_Y);
                tap(0, SDL_SCANCODE_X, RETRO_DEVICE_ID_JOYPAD_B);
                tap(0, SDL_SCANCODE_SPACE, RETRO_DEVICE_ID_JOYPAD_B);
                tap(0, SDL_SCANCODE_C, RETRO_DEVICE_ID_JOYPAD_A);
                tap(0, SDL_SCANCODE_RETURN, RETRO_DEVICE_ID_JOYPAD_START);
                tap(1, SDL_SCANCODE_I, RETRO_DEVICE_ID_JOYPAD_UP);
                tap(1, SDL_SCANCODE_K, RETRO_DEVICE_ID_JOYPAD_DOWN);
                tap(1, SDL_SCANCODE_J, RETRO_DEVICE_ID_JOYPAD_LEFT);
                tap(1, SDL_SCANCODE_L, RETRO_DEVICE_ID_JOYPAD_RIGHT);
                tap(1, SDL_SCANCODE_U, RETRO_DEVICE_ID_JOYPAD_Y);
                tap(1, SDL_SCANCODE_O, RETRO_DEVICE_ID_JOYPAD_B);
                tap(1, SDL_SCANCODE_P, RETRO_DEVICE_ID_JOYPAD_A);
                tap(1, SDL_SCANCODE_RSHIFT, RETRO_DEVICE_ID_JOYPAD_START);
            }
            if (k >= SDLK_F1 && k <= SDLK_F9) {
                try {
                    if (mod & KMOD_SHIFT)
                        core.save((int)(k - SDLK_F1 + 1));
                    else
                        core.loadState((int)(k - SDLK_F1 + 1));
                } catch (const std::exception &e) {
                    error(e);
                }
                return;
            }
            if (k == SDLK_F12) {
                screenshotRequested = true;
                return;
            }
            if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS) {
                settings.view = 0;
                settings.stepZoom(1);
                dirty = true;
            }
            if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
                settings.view = 0;
                settings.stepZoom(-1);
                dirty = true;
            }
            if (k == SDLK_f) {
                core.world.freeze(core.world.inspect(core.snapshot, core.game), settings);
                dirty = true;
            }
            if (k == SDLK_r) {
                settings.camera = 0;
                dirty = true;
            }
            if (k == SDLK_m) {
                settings.minimap = !settings.minimap;
                dirty = true;
            }
            if (k == SDLK_h) {
                settings.hud = (settings.hud + 1) % 4;
                dirty = true;
            }
            if ((mod & KMOD_CTRL) && k >= SDLK_1 && k <= SDLK_5) {
                int n = k - SDLK_1;
                settings.view = n == 3 ? 1 : n == 4 ? 2 : 0;
                settings.zoom = n < 3 ? n + 1 : settings.zoom;
                if (n < 3)
                    settings.zoomOut = 0;
                dirty = true;
            }
        }
    }
    if (e.type == SDL_MOUSEWHEEL && !menu && (SDL_GetModState() & KMOD_CTRL)) {
        settings.view = 0;
        settings.stepZoom(e.wheel.y > 0 ? 1 : -1);
        dirty = true;
    }
}
void App::pollInput() {
    const Uint8 *k = SDL_GetKeyboardState(nullptr);
    core.buttons[0] = core.buttons[1] = 0;
    rewindHeld = false;
    if (menu || (!focused && !headless)) {
        rewindTrigger = false;
        return;
    }
    if (settings.rewind) {
        bool padRewind = false;
        int rewindBinding = settings.rewindButton;
#ifdef SCU_QUEST
        if (settings.questTouch)
            rewindBinding = 0;
#endif
        if (pads[0]) {
            if (rewindBinding == 1)
                padRewind =
                    SDL_GameControllerGetButton(pads[0], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            else {
                const auto axis = rewindBinding == 2 ? SDL_CONTROLLER_AXIS_TRIGGERRIGHT
                                                     : SDL_CONTROLLER_AXIS_TRIGGERLEFT;
                const int value = SDL_GameControllerGetAxis(pads[0], axis);
                rewindTrigger = value > (rewindTrigger ? 4096 : 8192);
                padRewind = rewindTrigger;
            }
        } else
            rewindTrigger = false;
        rewindHeld = k[SDL_SCANCODE_BACKSPACE] || padRewind;
    } else
        rewindTrigger = false;
    core.buttons[0] = pendingButtons[0];
    core.buttons[1] = pendingButtons[1];
    auto set = [&](int p, bool on, int bit) {
        if (on)
            core.buttons[p] |= 1 << bit;
    };
    set(0, k[SDL_SCANCODE_UP], RETRO_DEVICE_ID_JOYPAD_UP);
    set(0, k[SDL_SCANCODE_DOWN], RETRO_DEVICE_ID_JOYPAD_DOWN);
    set(0, k[SDL_SCANCODE_LEFT], RETRO_DEVICE_ID_JOYPAD_LEFT);
    set(0, k[SDL_SCANCODE_RIGHT], RETRO_DEVICE_ID_JOYPAD_RIGHT);
    set(0, k[SDL_SCANCODE_Z], RETRO_DEVICE_ID_JOYPAD_Y);
    set(0, k[SDL_SCANCODE_X] || k[SDL_SCANCODE_SPACE], RETRO_DEVICE_ID_JOYPAD_B);
    set(0, k[SDL_SCANCODE_C], RETRO_DEVICE_ID_JOYPAD_A);
    set(0, k[SDL_SCANCODE_RETURN], RETRO_DEVICE_ID_JOYPAD_START);
    set(1, k[SDL_SCANCODE_I], RETRO_DEVICE_ID_JOYPAD_UP);
    set(1, k[SDL_SCANCODE_K], RETRO_DEVICE_ID_JOYPAD_DOWN);
    set(1, k[SDL_SCANCODE_J], RETRO_DEVICE_ID_JOYPAD_LEFT);
    set(1, k[SDL_SCANCODE_L], RETRO_DEVICE_ID_JOYPAD_RIGHT);
    set(1, k[SDL_SCANCODE_U], RETRO_DEVICE_ID_JOYPAD_Y);
    set(1, k[SDL_SCANCODE_O], RETRO_DEVICE_ID_JOYPAD_B);
    set(1, k[SDL_SCANCODE_P], RETRO_DEVICE_ID_JOYPAD_A);
    set(1, k[SDL_SCANCODE_RSHIFT], RETRO_DEVICE_ID_JOYPAD_START);
    for (int p = 0; p < 2; p++)
        if (pads[p]) {
            auto pad = pads[p];
            int dx = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX),
                dy = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY),
                dead = settings.deadzone;
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP) || dy < -dead,
                RETRO_DEVICE_ID_JOYPAD_UP);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || dy > dead,
                RETRO_DEVICE_ID_JOYPAD_DOWN);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || dx < -dead,
                RETRO_DEVICE_ID_JOYPAD_LEFT);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || dx > dead,
                RETRO_DEVICE_ID_JOYPAD_RIGHT);
            int bits[] = {RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_B,
                          RETRO_DEVICE_ID_JOYPAD_A};
            if (settings.jumpLayout == 1) {
                bits[0] = RETRO_DEVICE_ID_JOYPAD_A;
                bits[1] = RETRO_DEVICE_ID_JOYPAD_Y;
                bits[2] = RETRO_DEVICE_ID_JOYPAD_B;
            } else if (settings.jumpLayout == 2) {
                bits[0] = RETRO_DEVICE_ID_JOYPAD_A;
                bits[1] = RETRO_DEVICE_ID_JOYPAD_B;
                bits[2] = RETRO_DEVICE_ID_JOYPAD_Y;
            }
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X), bits[0]);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A), bits[1]);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B), bits[2]);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START),
                RETRO_DEVICE_ID_JOYPAD_START);
            if (settings.quickControls &&
                SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
                core.buttons[p] = 0;
        }
    if (releaseInput) {
        if (!(core.buttons[0] | core.buttons[1]) && !rewindHeld)
            releaseInput = false;
        core.buttons[0] = core.buttons[1] = 0;
        pendingButtons[0] = pendingButtons[1] = 0;
        rewindHeld = false;
    }
}
unsigned App::advanceGame(double elapsed, bool unpaced) {
    const bool paused = menu || (!focused && settings.pauseUnfocused && !headless)
#ifdef SCU_QUEST
                        || (quest && quest->simulationPaused())
#endif
        ;
    if (paused || !core.loaded) {
        gameAccumulator = 0;
        core.setRewinding(false);
        core.clearAudio();
        return 0;
    }
    const bool backward = settings.rewind && rewindHeld;
    core.setRewinding(backward);
    const double period = 1 / core.fps;
    gameAccumulator += std::clamp(elapsed, 0.0, 0.25);
    unsigned ticks = 0, executed = 0;
    constexpr unsigned maxCatchup = 16;
    while ((unpaced ? ticks == 0 : gameAccumulator >= period) && ticks < maxCatchup) {
        if (backward)
            executed += core.rewind() ? 1 : 0;
        else {
            core.tick();
            ++executed;
        }
        pendingButtons[0] = pendingButtons[1] = 0;
        ++ticks;
        if (!unpaced)
            gameAccumulator -= period;
    }
    if (unpaced || ticks == maxCatchup)
        gameAccumulator = 0;
    if (!backward)
        pan();
    return executed;
}
void App::pan() {
    if (menu || settings.camera != 3)
        return;
    auto k = SDL_GetKeyboardState(nullptr);
    int speed = settings.panSpeed * ((k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) ? 3 : 1);
    settings.left += (k[SDL_SCANCODE_D] - k[SDL_SCANCODE_A]) * speed;
    settings.top += (k[SDL_SCANCODE_S] - k[SDL_SCANCODE_W]) * speed;
    if (pads[0]) {
        int dx = SDL_GameControllerGetAxis(pads[0], SDL_CONTROLLER_AXIS_RIGHTX),
            dy = SDL_GameControllerGetAxis(pads[0], SDL_CONTROLLER_AXIS_RIGHTY);
        if (std::abs(dx) > settings.deadzone)
            settings.left += dx * speed / 32767;
        if (std::abs(dy) > settings.deadzone)
            settings.top += dy * speed / 32767;
    }
    settings.left = std::clamp(settings.left, -32768, 32768);
    settings.top = std::clamp(settings.top, -16384, 16384);
}
uint64_t App::overlayNow() const {
#ifdef SCU_UI_TESTS
    if (overlayTestClock)
        return overlayTestClock;
#endif
    return SDL_GetTicks64();
}
void App::logicalWindowSize(int &width, int &height) const {
#ifdef SCU_QUEST
    if (quest) {
        quest->logicalSize(width, height);
        return;
    }
#endif
    SDL_GetWindowSize(window, &width, &height);
}
void App::prepareOverlays() {
    overlays.clear();
    const auto now = overlayNow();
    if (lastOverlayNotice != notice) {
        lastOverlayNotice = notice;
        noticeShownAt = now;
    }
    if (menu) {
        hintShownAt = noticeShownAt = now;
        return;
    }
    int width = 0, height = 0;
    logicalWindowSize(width, height);
    if (width <= 0 || height <= 0)
        return;
    const float margin =
        std::min(float(std::clamp(settings.hintMargin, 0, 64)), std::min(width, height) / 8.0f);
    const float fontSize = 14.0f * std::clamp(settings.hintScale, 60, 180) / 100;
    const float maxWidth = std::max(1.0f, std::min(420.0f, width - margin * 2));
    auto font = ImGui::GetIO().Fonts->Fonts[0];
    auto add = [&](const char *id, std::string text, bool clickable, bool accent, float maxHeight) {
        const float padding = 6;
        auto size = font->CalcTextSizeA(fontSize, FLT_MAX, std::max(1.0f, maxWidth - padding * 2),
                                        text.c_str());
        overlays.push_back({id,
                            std::move(text),
                            {0, 0, std::min(maxWidth, std::ceil(size.x) + padding * 2),
                             std::min(maxHeight, std::ceil(size.y) + padding * 2)},
                            fontSize,
                            clickable,
                            accent});
    };
    const bool hintVisible =
        settings.help &&
        (!settings.hintSeconds ||
         now - hintShownAt < uint64_t(std::clamp(settings.hintSeconds, 0, 30)) * 1000);
    const bool detailed = std::clamp(settings.hintDetail, 0, 2) == 2;
    const char *binding = settings.rewindButton == 1   ? "L1 / LB"
                          : settings.rewindButton == 2 ? "R2 / RT"
                                                       : "L2 / LT";
#ifdef SCU_QUEST
    if (settings.questTouch)
        binding = tr("questtrigger", language);
#endif
    auto rewindText = [&](bool active) {
        char text[256];
        std::snprintf(text, sizeof text, tr(active ? "rewinding" : "rewindhint", language), binding,
                      core.rewindSeconds());
        return std::string(text);
    };
    if (hintVisible) {
        const char *key = "hintshortcuts";
#ifdef SCU_QUEST
        key = settings.questMode ? "questwallhint" : "questhint";
#else
        if (settings.hintDetail == 1) {
            int labels = controllerLabels();
            key = !padUsed || !pads[activePad] ? "hintkeyboard"
                  : labels == 2                ? "hintps"
                  : labels == 3                ? "hintnintendo"
                                               : "hintxbox";
        }
#endif
        std::string text = tr(key, language);
        if (detailed && settings.rewind)
            text += "\n" + rewindText(core.isRewinding());
        add("hint", std::move(text), true, false, height * 0.30f);
    }
    if (settings.rewindIndicator && core.isRewinding() && !(hintVisible && detailed))
        add("rewind", rewindText(true), false, true, height * 0.20f);
    if (settings.performance) {
        char text[160];
        std::snprintf(text, sizeof text, "%.1f ticks/s · %.1f fps · %d × %d", measuredFps,
                      presentationFps, drawableW, drawableH);
        add("performance", text, false, false, height * 0.20f);
    }
    if (!notice.empty() &&
        (!settings.noticeSeconds ||
         now - noticeShownAt < uint64_t(std::clamp(settings.noticeSeconds, 0, 30)) * 1000))
        add("notice", notice, true, true, height * 0.25f);
    float totalHeight = 0;
    for (const auto &card : overlays)
        totalHeight += card.bounds.w;
    const float available =
        std::max(1.0f, height - margin * 2 - std::max(0, int(overlays.size()) - 1) * 4);
    if (totalHeight > available)
        for (auto &card : overlays)
            card.bounds.w *= available / totalHeight;
    const int corner = std::clamp(settings.hintCorner, 0, 3);
    const bool right = corner % 2 == 0, bottom = corner < 2;
    float cursor = bottom ? height - margin : margin;
    for (auto &card : overlays) {
        const float w = card.bounds.z, h = card.bounds.w;
        card.bounds.x = right ? width - margin - w : margin;
        card.bounds.y = bottom ? cursor - h : cursor;
        card.bounds.z += card.bounds.x;
        card.bounds.w += card.bounds.y;
        cursor += (bottom ? -1 : 1) * (h + 4);
    }
}
SDL_Rect App::avoidOverlays(SDL_Rect rect) const {
    int width = 1, height = 1;
    logicalWindowSize(width, height);
    const float sx = float(drawableW) / std::max(1, width);
    const float sy = float(drawableH) / std::max(1, height);
    for (size_t pass = 0; pass < overlays.size(); ++pass)
        for (const auto &card : overlays) {
            SDL_Rect obstacle{int(card.bounds.x * sx) - 4, int(card.bounds.y * sy) - 4,
                              int((card.bounds.z - card.bounds.x) * sx) + 8,
                              int((card.bounds.w - card.bounds.y) * sy) + 8};
            if (SDL_HasIntersection(&rect, &obstacle)) {
                int up = obstacle.y - rect.h - 4, down = obstacle.y + obstacle.h + 4;
                if (up >= 4)
                    rect.y = up;
                else if (down + rect.h <= drawableH - 4)
                    rect.y = down;
            }
        }
    return rect;
}
void App::gameOverlays() {
    const float opacity = std::clamp(settings.hintOpacity, 20, 100) / 100.0f;
    auto font = ImGui::GetIO().Fonts->Fonts[0];
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
    for (const auto &card : overlays) {
        ImVec2 pos{card.bounds.x, card.bounds.y};
        ImVec2 size{card.bounds.z - card.bounds.x, card.bounds.w - card.bounds.y};
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::SetNextWindowBgAlpha(settings.highContrast ? 0.96f : opacity);
        ImGui::PushStyleColor(ImGuiCol_Border, card.accent ? ImVec4{0.9f, 0.75f, 0.3f, 0.8f}
                                                           : ImVec4{0.2f, 0.6f, 0.7f, 0.55f});
        ImGui::Begin(("##overlay-" + card.id).c_str(), nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar |
                         (card.clickable ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoInputs));
        if (card.clickable) {
            ImGui::SetCursorScreenPos(pos);
            if (ImGui::InvisibleButton("open-options", size))
                toggleMenu();
            if (ImGui::IsItemHovered()) {
                ImGui::GetWindowDrawList()->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y},
                                                          IM_COL32(40, 126, 147, 80), 6);
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(360);
                ImGui::TextUnformatted(card.id == "notice" ? card.text.c_str()
                                                           : tr("hinttooltip", language));
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
        auto draw = ImGui::GetWindowDrawList();
        draw->PushClipRect({pos.x + 5, pos.y + 3}, {pos.x + size.x - 5, pos.y + size.y - 3}, true);
        draw->AddText(font, card.fontSize, {pos.x + 6, pos.y + 6},
                      settings.highContrast ? IM_COL32_WHITE : IM_COL32(224, 242, 249, 255),
                      card.text.c_str(), nullptr, std::max(1.0f, size.x - 12));
        draw->PopClipRect();
        ImGui::End();
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar(4);
}
void App::present(bool capture) {
    Settings transientSettings;
    auto *projectionSettings = &settings;
#ifdef SCU_QUEST
    if (quest) {
        transientSettings = settings;
        quest->configureRender(transientSettings);
        projectionSettings = &transientSettings;
    }
#else
    if (settings.desktopPresentation) {
        transientSettings = settings;
        transientSettings.projectionCleanScene = true;
        transientSettings.view = 2;
        transientSettings.camera = 4;
        transientSettings.mapExtension = true;
        transientSettings.fitSpawns = false;
        projectionSettings = &transientSettings;
    }
#endif
    auto &renderSettings = *projectionSettings;
    SDL_GetRendererOutputSize(renderer, &drawableW, &drawableH);
    if (drawableW <= 0 || drawableH <= 0)
        return;
    prepareOverlays();
    nativeBounds = insetBounds = {};
    SDL_SetRenderDrawColor(renderer, 6, 12, 24, 255);
    SDL_RenderClear(renderer);
    Scene scene = core.world.inspect(core.snapshot, core.game);
    uint64_t hash = 1469598103934665603ull;
    for (int v : {drawableW,
                  drawableH,
                  renderSettings.view,
                  renderSettings.zoom,
                  renderSettings.zoomOut,
                  renderSettings.panoramaHeight,
                  int(renderSettings.mapExtension),
                  int(renderSettings.smoothMap),
                  renderSettings.camera,
                  renderSettings.left,
                  renderSettings.top,
                  renderSettings.background,
                  renderSettings.blur,
                  renderSettings.brightness,
                  renderSettings.overviewStyle,
                  renderSettings.quality,
                  int(renderSettings.projectionUiOnly),
                  int(renderSettings.projectionCleanScene),
                  renderSettings.fixedMapWidth,
                  renderSettings.fixedMapHeight,
                  renderSettings.hud,
                  int(renderSettings.playerInset),
                  (int)renderSettings.parallax,
                  (int)renderSettings.reduceMotion,
                  (int)renderSettings.enemyPreview,
                  renderSettings.enemyOpacity,
                  renderSettings.enemyStyle,
                  (int)renderSettings.bossPreview,
                  (int)renderSettings.scriptPreview,
                  (int)renderSettings.spawnTint,
                  (int)renderSettings.fitSpawns}) {
        hash ^= (uint32_t)v;
        hash *= 1099511628211ull;
    }
    bool presentationChanged = hash != lastPresentationHash;
    bool refresh = core.imageVersion != lastImageVersion || presentationChanged;
    if (refresh) {
        cachedExpanded = !renderSettings.projectionUiOnly && core.loaded &&
                         core.world.compose(core.snapshot, core.game, renderSettings, drawableW,
                                            drawableH, &core.native, core.nativeW, core.nativeH);
        lastImageVersion = core.imageVersion;
        lastPresentationHash = hash;
    }
    bool expanded = core.loaded && cachedExpanded;
    if (core.loaded && !renderSettings.projectionUiOnly) {
        bool fixed = !renderSettings.parallax || renderSettings.reduceMotion ||
                     (scene.ready && (renderSettings.camera == 2 || renderSettings.camera == 4 ||
                                      renderSettings.view == 2));
        uint64_t backdropHash = 1469598103934665603ull;
        for (int value :
             {renderSettings.background, renderSettings.blur, renderSettings.brightness}) {
            backdropHash ^= uint32_t(value);
            backdropHash *= 1099511628211ull;
        }
        bool retainBackground = fixed && backgroundFrozen && backgroundStage == scene.stage &&
                                backdropSettingsHash == backdropHash;
        if (refresh && !retainBackground)
            core.world.makeBackdrop(core.snapshot, scene, renderSettings, core.native, core.nativeW,
                                    core.nativeH);
        if (retainBackground)
            core.world.backdrop = frozenBackground;
        else {
            frozenBackground = core.world.backdrop;
            backgroundFrozen = fixed;
            backgroundStage = scene.stage;
            backdropSettingsHash = backdropHash;
        }
        if (!bgTexture) {
            bgTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, 320, 224);
            SDL_SetTextureScaleMode(bgTexture, SDL_ScaleModeLinear);
        }
        if (refresh && !retainBackground)
            SDL_UpdateTexture(bgTexture, nullptr, core.world.backdrop.data(), 320 * 4);
        if (renderSettings.background == 2 && expanded) {
            int scale = std::max(1, (int)core.world.canvas.scale), tw = 320 * scale,
                th = 224 * scale;
            for (int y = 0; y < drawableH; y += th)
                for (int x = 0; x < drawableW; x += tw) {
                    SDL_Rect d{x, y, tw, th};
                    SDL_RenderCopy(renderer, bgTexture, nullptr, &d);
                }
        } else
            SDL_RenderCopy(renderer, bgTexture, nullptr, nullptr);
        if (expanded) {
            auto &c = core.world.canvas;
            if (!worldTexture || textureW != c.width || textureH != c.height) {
                if (worldTexture)
                    SDL_DestroyTexture(worldTexture);
                worldTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_STREAMING, c.width, c.height);
                textureW = c.width;
                textureH = c.height;
                SDL_SetTextureBlendMode(worldTexture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(worldTexture, SDL_ScaleModeNearest);
            }
            if (!worldTexture)
                throw std::runtime_error(SDL_GetError());
            SDL_SetTextureScaleMode(worldTexture,
                                    renderSettings.smoothMap && c.coverageW > c.width * 1.01
                                        ? SDL_ScaleModeLinear
                                        : SDL_ScaleModeNearest);
            if (refresh)
                SDL_UpdateTexture(worldTexture, nullptr, c.pixels.data(), c.width * 4);
            SDL_RenderCopy(renderer, worldTexture, nullptr, nullptr);
            if (!renderSettings.projectionCleanScene &&
                ((renderSettings.view == 2 && renderSettings.playerInset) ||
                 (scene.family == 4 && renderSettings.hud != 3)) &&
                !core.native.empty()) {
                if (!nativeTexture || nativeTW != core.nativeW || nativeTH != core.nativeH) {
                    if (nativeTexture)
                        SDL_DestroyTexture(nativeTexture);
                    nativeTexture =
                        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, core.nativeW, core.nativeH);
                    nativeTW = core.nativeW;
                    nativeTH = core.nativeH;
                }
                SDL_SetTextureScaleMode(nativeTexture, SDL_ScaleModeNearest);
                if (refresh)
                    SDL_UpdateTexture(nativeTexture, nullptr, core.native.data(), core.nativeW * 4);
                int safeW = std::min(drawableW, int(drawableH * 16.0 / 9)),
                    w = int(std::min(safeW * 0.28, drawableH * 0.26 * core.nativeW / core.nativeH)),
                    h = w * core.nativeH / core.nativeW, x = (drawableW + safeW) / 2 - w - 16,
                    y = drawableH - h - 16;
                SDL_Rect d = avoidOverlays({x, y, w, h});
                SDL_Rect border{d.x - 3, d.y - 3, w + 6, h + 6};
                if (renderSettings.view == 2 && renderSettings.playerInset) {
                    insetBounds = {float(d.x), float(d.y), float(d.x + d.w), float(d.y + d.h)};
                    SDL_SetRenderDrawColor(renderer, 85, 206, 217, 255);
                    SDL_RenderFillRect(renderer, &border);
                    SDL_RenderCopy(renderer, nativeTexture, nullptr, &d);
                }
            }
        } else if (!core.native.empty()) {
            if (!nativeTexture || nativeTW != core.nativeW || nativeTH != core.nativeH) {
                if (nativeTexture)
                    SDL_DestroyTexture(nativeTexture);
                nativeTexture =
                    SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING, core.nativeW, core.nativeH);
                nativeTW = core.nativeW;
                nativeTH = core.nativeH;
            }
            SDL_SetTextureScaleMode(nativeTexture, renderSettings.integerScale
                                                       ? SDL_ScaleModeNearest
                                                       : SDL_ScaleModeLinear);
            if (refresh)
                SDL_UpdateTexture(nativeTexture, nullptr, core.native.data(), core.nativeW * 4);
            int availableH = drawableH, safeW = std::min(drawableW, (int)(drawableH * 16.0 / 9));
            float scale = renderSettings.view == 0     ? float(renderSettings.pixelScale())
                          : renderSettings.nativeScale ? renderSettings.nativeScale
                                                       : std::min(float(safeW) / core.nativeW,
                                                                  float(availableH) / core.nativeH);
            if (renderSettings.integerScale && scale >= 1)
                scale = std::floor(scale);
            scale = std::min(
                scale, std::min(float(drawableW) / core.nativeW, float(availableH) / core.nativeH));
            SDL_Rect d{(drawableW - (int)(core.nativeW * scale)) / 2,
                       (availableH - (int)(core.nativeH * scale)) / 2, (int)(core.nativeW * scale),
                       (int)(core.nativeH * scale)};
            nativeBounds = {float(d.x), float(d.y), float(d.x + d.w), float(d.y + d.h)};
            SDL_RenderCopy(renderer, nativeTexture, nullptr, &d);
        }
        if (renderSettings.scanlines) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, renderSettings.scanlines * 255 / 100);
            for (int y = 1; y < drawableH; y += 3)
                SDL_RenderDrawLine(renderer, 0, y, drawableW, y);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
#ifndef SCU_QUEST
    if (settings.desktopPresentation && core.loaded && !core.native.empty()) {
        bool upload = refresh;
        if (!nativeTexture || nativeTW != core.nativeW || nativeTH != core.nativeH) {
            if (nativeTexture)
                SDL_DestroyTexture(nativeTexture);
            nativeTexture =
                SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                  core.nativeW, core.nativeH);
            nativeTW = core.nativeW;
            nativeTH = core.nativeH;
            upload = true;
        }
        if (!nativeTexture)
            throw std::runtime_error(SDL_GetError());
        SDL_SetTextureScaleMode(nativeTexture,
                                settings.integerScale ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
        if (upload)
            SDL_UpdateTexture(nativeTexture, nullptr, core.native.data(), core.nativeW * 4);
        desktopFrame.draw(renderer, expanded ? worldTexture : nativeTexture, nativeTexture,
                          core.nativeW, core.nativeH, scene, core.world.canvas, settings, drawableW,
                          drawableH);
    }
#endif
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
#ifdef SCU_QUEST
    if (quest)
        quest->prepareUi();
#endif
    updateNavigationController();
    bool editing = GImGui->ActiveId != 0 ||
                   (!GImGui->OpenPopupStack.empty() && GImGui->OpenPopupStack.back().Window &&
                    !(GImGui->OpenPopupStack.back().Window->Flags & ImGuiWindowFlags_Modal));
    auto &io = ImGui::GetIO();
    uiFactor = settings.uiScale == 1   ? 0.85f
               : settings.uiScale == 2 ? 1.3f
               : settings.uiScale == 3 ? 1.65f
                                       : std::clamp(io.DisplaySize.y / 720.0f, 0.85f, 2.0f);
    ImGui::GetStyle() = baseStyle;
    ImGui::GetStyle().ScaleAllSizes(uiFactor);
    if (settings.highContrast) {
        ImGui::GetStyle().Colors[ImGuiCol_Text] = {1, 1, 1, 1};
        ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = {0.02f, 0.04f, 0.08f, 1};
        ImGui::GetStyle().Colors[ImGuiCol_NavCursor] = {1, 1, 0, 1};
    }
    io.FontGlobalScale = 0.75f * uiFactor;
    ImGui::NewFrame();
    menuInput(editing);
#ifdef SCU_UI_TESTS
    controls.clear();
    controlRects.clear();
#endif
    if (expanded && !renderSettings.projectionCleanScene)
        hud();
    if (!menu && (!renderSettings.projectionCleanScene
#ifndef SCU_QUEST
                  || settings.desktopPresentation
#endif
                  ))
        gameOverlays();
    if (menu)
        ui();
#ifdef SCU_QUEST
    if (quest)
        quest->drawReticle();
#endif
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    if (capture || screenshotRequested) {
        fs::path p = capturePath.empty()
                         ? library.dataDir / "screenshots" /
                               (core.currentId + "-" + std::to_string(SDL_GetTicks64()) + ".bmp")
                         : fs::u8path(capturePath);
        screenshot(p);
        screenshotRequested = false;
    }
#ifdef SCU_QUEST
    SDL_RenderFlush(renderer);
#else
    SDL_RenderPresent(renderer);
#endif
    ++presentationFrames;
    uint64_t now = SDL_GetTicks64();
    if (!presentationClock)
        presentationClock = now;
    if (now - presentationClock >= 1000) {
        presentationFps = presentationFrames * 1000.0 / (now - presentationClock);
        presentationFrames = 0;
        presentationClock = now;
    }
}
void App::screenshot(const fs::path &path) {
    fs::create_directories(path.parent_path());
    auto surf =
        SDL_CreateRGBSurfaceWithFormat(0, drawableW, drawableH, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surf)
        throw std::runtime_error(SDL_GetError());
    if (SDL_RenderReadPixels(renderer, nullptr, surf->format->format, surf->pixels, surf->pitch) !=
        0) {
        SDL_FreeSurface(surf);
        throw std::runtime_error(SDL_GetError());
    }
    std::string p = path.u8string();
    auto rw = SDL_RWFromFile(p.c_str(), "wb");
    int result = rw ? SDL_SaveBMP_RW(surf, rw, 1) : -1;
    SDL_FreeSurface(surf);
    if (result)
        throw std::runtime_error(SDL_GetError());
    fprintf(stderr, "SCU capture: %d x %d\n", drawableW, drawableH);
}
void App::hud() {
    auto &io = ImGui::GetIO();
    auto v = core.world.inspect(core.snapshot, core.game);
    auto &c = core.world.canvas;
    float sx = (float)(io.DisplaySize.x / c.coverageW),
          sy = (float)(io.DisplaySize.y / c.coverageH), px = (v.playerX - c.left) * sx,
          py = (v.playerY - c.top) * sy;
    auto draw = ImGui::GetBackgroundDrawList();
    if (settings.enemyPreview && settings.spawnLabels) {
        std::vector<ImVec4> labels;
        // Boss labels have priority. Avoid overlapping labels on tiny maps.
        for (auto kind : {SpawnKind::Boss, SpawnKind::Scripted})
            for (const auto &p : core.world.enemies.visible) {
                size_t maxLabels = size_t(std::clamp(int(io.DisplaySize.x / 350), 3, 12));
                if (p.kind != kind || labels.size() >= maxLabels ||
                    (kind == SpawnKind::Scripted && !p.variable))
                    continue;
                float x = (p.x - c.left) * sx, y = (p.y - c.top) * sy;
                if (x < 0 || y < 0 || x >= io.DisplaySize.x || y >= io.DisplaySize.y)
                    continue;
                const char *text = tr(p.variable                ? "spawnorigin"
                                      : kind == SpawnKind::Boss ? "bosslabel"
                                                                : "scriptlabel",
                                      language);
                auto size = ImGui::CalcTextSize(text);
                x = std::clamp(x - size.x / 2, 4.0f, std::max(4.0f, io.DisplaySize.x - size.x - 4));
                y = std::clamp(y - size.y - 12, 4.0f,
                               std::max(4.0f, io.DisplaySize.y - size.y - 4));
                ImVec4 box{x - 3, y - 2, x + size.x + 3, y + size.y + 2};
                bool overlap = false;
                for (auto b : labels)
                    overlap |=
                        box.x < b.z + 8 && box.z + 8 > b.x && box.y < b.w + 4 && box.w + 4 > b.y;
                if (overlap)
                    continue;
                labels.push_back(box);
                draw->AddRectFilled({box.x, box.y}, {box.z, box.w}, IM_COL32(6, 12, 24, 185), 3);
                draw->AddText({x, y},
                              kind == SpawnKind::Boss ? IM_COL32(255, 129, 148, 230)
                                                      : IM_COL32(193, 140, 255, 230),
                              text);
            }
    }
    if (settings.locator) {
        float r = 18, mx = std::clamp(px, 24.0f, io.DisplaySize.x - 24),
              my = std::clamp(py, 32.0f, io.DisplaySize.y - 60);
        draw->AddCircle({mx, my}, r, IM_COL32(110, 242, 245, 210), 32, 2);
        draw->AddLine({mx, my - r - 5}, {mx, my - r - 16}, IM_COL32(240, 240, 240, 220), 2);
        if (mx != px || my != py) {
            float dx = px - mx, dy = py - my, len = std::sqrt(dx * dx + dy * dy);
            dx /= len;
            dy /= len;
            draw->AddTriangleFilled(
                {mx + dx * 24, my + dy * 24}, {mx + dx * 12 - dy * 6, my + dy * 12 + dx * 6},
                {mx + dx * 12 + dy * 6, my + dy * 12 - dx * 6}, IM_COL32(255, 230, 103, 255));
        }
        if (core.spinCharge) {
            draw->AddText({mx + 22, my - 10}, IM_COL32(255, 226, 103, 255), tr("charge", language));
            draw->AddRectFilled({mx + 22, my + 16}, {mx + 22 + core.spinCharge * 12.0f, my + 21},
                                IM_COL32(255, 222, 76, 255));
        }
    }
    if (settings.view == 2 &&
        (settings.overviewStyle == 2 || (settings.overviewStyle == 0 && v.family == 3)))
        draw->AddText(
            {io.DisplaySize.x / 2 - ImGui::CalcTextSize(tr("terrainmap", language)).x / 2, 12},
            IM_COL32(220, 248, 250, 255), tr("terrainmap", language));
    if (settings.ghost) {
        float x = (v.cameraX - c.left) * sx, y = (v.cameraY - c.top) * sy;
        draw->AddRect({x, y}, {x + 320 * sx, y + 224 * sy}, IM_COL32(255, 255, 255, 110), 0, 0, 1);
    }
    if (settings.hud != 3) {
        float safe = std::min(io.DisplaySize.x, io.DisplaySize.y * 16 / 9),
              x = settings.hud == 2 ? 18 : (io.DisplaySize.x - safe) / 2 + 18, y = 18;
        if (settings.hud == 1) {
            x = std::clamp(px + 30.0f, 0.0f, std::max(0.0f, io.DisplaySize.x - 320));
            y = std::clamp(py - 85.0f, 0.0f, std::max(0.0f, io.DisplaySize.y - 150));
        }
        ImGui::SetNextWindowPos({x, y});
        ImGui::SetNextWindowBgAlpha(0.76f);
        ImGui::Begin("##score", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);
        float size = settings.hudScale ? float(settings.hudScale)
                                       : std::clamp(io.DisplaySize.y / 720.0f, 0.85f, 3.0f);
        ImGui::SetWindowFontScale(std::min(3.0f, size) / uiFactor);
        auto &r = core.snapshot.ram;
        if (v.family == 4 && nativeTexture) {
            float w = std::min(320.0f * size, io.DisplaySize.x - 60);
            ImGui::Image((ImTextureID)(intptr_t)nativeTexture, {w, w * 16 / 320}, {0, 0},
                         {1, 16.0f / 224});
        } else if (v.family == 5) {
            ImGui::Text("%s  %u", tr("rings", language), be16(r, 0x0A5A));
            ImGui::Text("%s  %u", tr("lives", language), be16(r, 0x0680));
        } else {
            unsigned score = (uint32_t(be16(r, 0xFE26)) << 16) | be16(r, 0xFE28);
            ImGui::TextColored({0.95f, 0.85f, 0.35f, 1}, "%s  %06u", tr("score", language),
                               score * 10);
            ImGui::Text("%s  %02u:%02u", tr("time", language), r[0xFE23], r[0xFE24]);
            ImGui::Text("%s  %u", tr("rings", language), be16(r, 0xFE20));
            ImGui::Text("%s  %u", tr("lives", language), r[0xFE12]);
        }
        ImGui::End();
    }
    if (settings.minimap) {
        float mapW = std::min(420.0f, io.DisplaySize.x * 0.34f),
              mapH = std::clamp(mapW * v.height / v.width, 35.0f, 100.0f),
              left = io.DisplaySize.x / 2 - mapW / 2, top = io.DisplaySize.y - mapH - 65;
        draw->AddRectFilled({left - 8, top - 8}, {left + mapW + 8, top + mapH + 8},
                            IM_COL32(6, 18, 32, 200), 8);
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 128; x++) {
                int wx = x * v.width / 128, wy = y * v.height / 16;
                if (core.world.attribute(core.snapshot, v.family, wx, wy))
                    draw->AddRectFilled({left + x * mapW / 128, top + y * mapH / 16},
                                        {left + (x + 1) * mapW / 128, top + (y + 1) * mapH / 16},
                                        IM_COL32(48, 134, 159, 190));
            }
        float mx = mapW / v.width, my = mapH / v.height;
        draw->AddRect(
            {left + c.left * mx, top + c.top * my},
            {left + float(c.left + c.coverageW) * mx, top + float(c.top + c.coverageH) * my},
            IM_COL32(230, 244, 255, 180));
        draw->AddCircleFilled({left + v.playerX * mx, top + v.playerY * my}, 4,
                              IM_COL32(255, 224, 88, 255));
    }
}
int App::controllerLabels() const {
    int labels = settings.padLabels;
    if (!labels && pads[activePad]) {
        auto type = SDL_GameControllerGetType(pads[activePad]);
        labels = type == SDL_CONTROLLER_TYPE_PS3 || type == SDL_CONTROLLER_TYPE_PS4 ||
                         type == SDL_CONTROLLER_TYPE_PS5
                     ? 2
                 : type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO ||
                         type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR
                     ? 3
                     : 1;
    }
    return labels;
}
std::string App::navigationHint() const {
#ifdef SCU_QUEST
    if (settings.questTouch)
        return tr("questnav", language);
#endif
    int labels = controllerLabels();
    return tr(labels == 2 ? "navps" : labels == 3 ? "navnintendo" : "navxbox", language);
}
void App::menuInput(bool editing) {
    if (guideOpen && (backRequested || selectRequested)) {
        guideOpen = false;
        backRequested = selectRequested = false;
        return;
    }
    if (!menu) {
        backRequested = selectRequested = false;
        return;
    }
    if (backRequested || selectRequested) {
        if (!editing) {
            if (pathEditor)
                pathEditor = false;
            else if (actionPopup)
                actionPopup = 0;
            else if (browserOpen) {
                browserOpen = false;
                focusMenu = true;
            } else if (core.loaded)
                toggleMenu();
            else if (tab != 0) {
                tab = 0;
                focusPage = true;
            }
        }
        backRequested = selectRequested = false;
    }
    if (!menu || editing || browserOpen || pathEditor || actionPopup || guideOpen)
        return;
    int direction = 0;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false))
        direction = -1;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false))
        direction = 1;
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Tab, false))
        direction = ImGui::GetIO().KeyShift ? -1 : 1;
    if (direction) {
        tab = (tab + direction + sectionCount()) % sectionCount();
        focusPage = true;
        ImGui::SetNavCursorVisible(true);
    }
    if (core.loaded && ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false))
        toggleMenu();
}
void App::scrollPage() {
    float direction = ImGui::GetKeyData(ImGuiKey_GamepadR2)->AnalogValue -
                      ImGui::GetKeyData(ImGuiKey_GamepadL2)->AnalogValue;
    if (std::abs(direction) > 0.1f)
        ImGui::SetScrollY(ImGui::GetScrollY() +
                          direction * ImGui::GetIO().DeltaTime * 750 * uiFactor);
}
void App::requestAction(int action, int slot, const std::string &game) {
    actionSlot = slot;
    actionGame = game;
    actionPopup = action;
    menu = true;
    core.clearAudio();
    pendingButtons[0] = pendingButtons[1] = 0;
    if ((action == 1 && (!settings.confirmQuit || !core.loaded)) ||
        (action != 1 && !settings.confirmActions))
        commitAction();
}
void App::commitAction() {
    int action = actionPopup;
    actionPopup = 0;
    try {
        if (action == 1)
            running = false;
        else if (action == 2) {
            if (settings.lockon != core.locked && core.currentId != "sonic2k" &&
                core.currentId != "sonic3k" && core.currentId != "sphere") {
                bool automatic = settings.autosave;
                settings.autosave = false;
                std::string id = core.currentId;
                play(id);
                settings.autosave = automatic;
                menu = true;
            } else
                core.reset();
            notify(tr("resetdone", language));
        } else if (action == 3) {
            auto previous = settings;
            settings = Settings{};
            settings.language = previous.language;
            settings.lastDirectory = previous.lastDirectory;
            settings.lastGame = previous.lastGame;
            settings.mute = previous.mute;
            language = detectLanguage(settings.language);
            core.updateOptions();
            backgroundStage = -1;
            applyDisplay();
            dirty = true;
            notify(tr("defaultsdone", language));
        } else if (action == 4) {
            core.loadState(actionSlot);
            notify(tr("loaded", language));
        } else if (action == 5) {
            core.save(actionSlot);
            notify(tr("saved", language));
        } else if (action == 6)
            play(actionGame);
    } catch (const std::exception &e) {
        error(e);
    }
    focusPage = true;
}
void App::ui() {
    auto &io = ImGui::GetIO();
    bool compact = settings.menuLayout == 1;
    float width = compact ? std::min(1120 * uiFactor, io.DisplaySize.x - 12) : io.DisplaySize.x;
    float height = compact ? std::min(820 * uiFactor, io.DisplaySize.y - 12) : io.DisplaySize.y;
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({width, height});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,
                        compact ? baseStyle.WindowRounding * uiFactor : 0.0f);
    if (focusMenu)
        ImGui::SetNextWindowFocus();
    ImGui::Begin("###scu-menu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
    float safe = std::min(ImGui::GetContentRegionAvail().x, 1220 * uiFactor);
    ImGui::SetCursorPosX((width - safe) / 2);
    ImGui::BeginChild("##safe", {safe, 0}, ImGuiChildFlags_NavFlattened);
    bool small = safe < 650 * uiFactor, tiny = height < 450 * uiFactor;
    if (!tiny) {
        ImGui::TextColored({0.25f, 0.84f, 0.91f, 1}, "SONIC COMPILATION ULTRA");
        if (!small)
            ImGui::TextWrapped("%s", tr("tagline", language));
    }
    if (focusMenu) {
        ImGui::SetKeyboardFocusHere();
        ImGui::SetNavCursorVisible(true);
    }
    const char *importKey = safe < 400 * uiFactor ? "importshort" : "import";
    if (core.loaded) {
        if (button("resume"))
            toggleMenu();
        nextButton(importKey);
    }
    if (button(importKey))
#ifdef SCU_QUEST
        quest->chooseRom();
#else
        beginBrowser(false);
#endif
    control("import");
    nextButton("quit");
    if (button("quit"))
        requestAction(1);
    focusMenu = false;
    ImGui::Separator();
    const char *tabs[] = {"library",
                          "video",
                          "camera",
                          "atmosphere",
                          "sound",
                          "controls",
                          "gameplay",
                          "saves",
                          "about"
#ifdef SCU_QUEST
                          ,
                          "questtab"
#endif
    };
    if (small) {
        if (button("prevsection")) {
            tab = (tab + sectionCount() - 1) % sectionCount();
            focusPage = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(std::max(80.0f, safe - 100 * uiFactor));
        std::vector<const char *> labels;
        for (auto key : tabs)
            labels.push_back(tr(key, language));
        if (ImGui::Combo("##section", &tab, labels.data(), sectionCount()))
            focusPage = true;
        control("section");
        ImGui::SameLine();
        if (button("nextsection")) {
            tab = (tab + 1) % sectionCount();
            focusPage = true;
        }
    }
    float footer =
        ImGui::GetFrameHeightWithSpacing() +
        ImGui::CalcTextSize(navigationHint().c_str(), nullptr, false, safe).y +
        ImGui::GetStyle().ItemSpacing.y * 3 +
        (!tiny ? ImGui::CalcTextSize(tr("autosettings", language), nullptr, false, safe).y : 0);
    float contentH = std::max(50.0f, ImGui::GetContentRegionAvail().y - footer);
    if (!small) {
        ImGui::BeginChild("##sections", {230 * uiFactor, contentH}, ImGuiChildFlags_NavFlattened);
        for (int n = 0; n < sectionCount(); ++n) {
            if (ImGui::Selectable((std::string(tr(tabs[n], language)) + "###" + tabs[n]).c_str(),
                                  tab == n)) {
                tab = n;
                focusPage = true;
            }
            control(tabs[n]);
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }
    bool sideGuide = tab > 0 && (tab < 8 || tab == 9) && safe >= 1000 * uiFactor && !tiny;
    float guideW = sideGuide ? 310 * uiFactor : 0;
    float pageW =
        sideGuide ? ImGui::GetContentRegionAvail().x - guideW - ImGui::GetStyle().ItemSpacing.x : 0;
    ImGui::BeginChild("##page", {pageW, contentH}, ImGuiChildFlags_NavFlattened);
    scrollPage();
    if (focusPage) {
        ImGui::SetScrollY(0);
        ImGui::SetKeyboardFocusHere();
    }
    if (!notice.empty()) {
        ImGui::TextWrapped("%s", notice.c_str());
        if (button("dismiss"))
            notice.clear();
        ImGui::Separator();
    }
    if (tab == 0)
        libraryUi();
    else
        optionsUi(tab);
    focusPage = false;
    ImGui::EndChild();
    if (sideGuide) {
        ImGui::SameLine();
        ImGui::BeginChild("##setting-guide", {guideW, contentH});
        settingGuide();
        ImGui::EndChild();
    }
    ImGui::Separator();
    if (button(
#ifdef SCU_QUEST
            "questguidebutton"
#else
            "settingguide"
#endif
            ))
        guideRequested = true;
    ImGui::TextWrapped("%s", navigationHint().c_str());
    if (!tiny)
        ImGui::TextDisabled("%s", tr("autosettings", language));
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
    if (guideRequested) {
        guideRequested = false;
        guideOpen = true;
        ImGui::OpenPopup("##setting-detail");
    }
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({std::min(620 * uiFactor, io.DisplaySize.x - 12),
                              std::max(100.0f, io.DisplaySize.y * 0.88f)});
    if (ImGui::BeginPopupModal("##setting-detail", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {
        if (!guideOpen)
            ImGui::CloseCurrentPopup();
        if (button("guideclose")) {
            guideOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::BeginChild("##guide-scroll", {0, 0});
        scrollPage();
        settingGuide(io.DisplaySize.y < 450);
        ImGui::EndChild();
        ImGui::EndPopup();
    }
    if (browserOpen || ImGui::IsPopupOpen("##browse"))
        browserUi();
    if (actionPopup)
        ImGui::OpenPopup("##confirm");
    ImGui::SetNextWindowSize({std::min(650 * uiFactor, io.DisplaySize.x - 16), 0});
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    if (ImGui::BeginPopupModal("##confirm", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        if (!actionPopup)
            ImGui::CloseCurrentPopup();
        const char *messages[] = {"",
                                  "quitconfirm",
                                  "resetconfirm",
                                  "defaultsconfirm",
                                  "loadconfirm",
                                  "overwriteconfirm",
                                  "switchconfirm"};
        ImGui::TextWrapped("%s", tr(messages[std::clamp(actionPopup, 0, 6)], language));
        if (button("cancel")) {
            actionPopup = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (button("yes")) {
            commitAction();
            ImGui::CloseCurrentPopup();
        }
        ImGui::TextWrapped("%s", navigationHint().c_str());
        ImGui::EndPopup();
    }
}
void App::libraryUi() {
    if (button("folder"))
        beginBrowser(true);
    if (ImGui::GetContentRegionAvail().x > 580 * uiFactor)
        ImGui::SameLine();
    if (button("sourcehelp")) {
        tab = 8;
        focusPage = true;
    }
    if (tick("readyonly", settings.readyOnly))
        dirty = true;
    if (!core.loaded && library.find(settings.lastGame)) {
        if (button("continue"))
            play(settings.lastGame);
    }
    if (library.entries.empty())
        ImGui::TextWrapped("%s", tr("empty", language));
    ImGui::TextWrapped("%s", tr("legalshort", language));
    float available = ImGui::GetContentRegionAvail().x;
    int cols = available > 750 * uiFactor ? 3 : available > 470 * uiFactor ? 2 : 1;
    if (ImGui::BeginTable("##games", cols, ImGuiTableFlags_SizingStretchSame)) {
        int index = 0;
        for (auto &g : games()) {
            if (std::string(g.id) == "skchip")
                continue;
            auto r = library.find(g.id);
            if (settings.readyOnly && !r)
                continue;
            ImGui::TableNextColumn();
            ImGui::PushID(g.id);
            ImGui::BeginChild("##card", {0, 186 * uiFactor},
                              ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened);
            ImGui::TextColored(r ? ImVec4{0.35f, 0.89f, 0.91f, 1} : ImVec4{0.55f, 0.62f, 0.69f, 1},
                               "%02d", ++index);
            ImGui::TextWrapped("%s", g.name);
            ImGui::TextWrapped("%s",
                               r ? (r->derived ? tr("derived", language) : tr("ready", language))
                                 : tr("missing", language));
            ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), 134 * uiFactor));
            if (r) {
                if (button("play", {-1, 0})) {
                    if (core.loaded && core.currentId != g.id)
                        requestAction(6, 1, g.id);
                    else
                        play(g.id);
                }
                control((std::string("play-") + g.id).c_str());
            } else if (button("import", {-1, 0}))
                beginBrowser(false);
            ImGui::EndChild();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    for (auto &r : library.entries)
        if (r.id.rfind("custom-", 0) == 0) {
            ImGui::PushID(r.id.c_str());
            ImGui::TextWrapped("%s", fs::u8path(r.path).stem().u8string().c_str());
            if (button("play")) {
                if (core.loaded && core.currentId != r.id)
                    requestAction(6, 1, r.id);
                else
                    play(r.id);
            }
            control(("play-" + r.id).c_str());
            ImGui::PopID();
        }
}
void App::optionsUi(int section) {
    bool changed = false;
    auto check = [&](const char *key, bool &value) { changed |= tick(key, value); };
    auto slide = [&](const char *key, int &v, int a, int b, const char *fmt = "%d") {
        changed |= slider(key, v, a, b, fmt);
    };
    switch (section) {
    case 1: {
        int lang = settings.language + 1;
        if (choice("language", lang, {"auto", "English", "Français", "Español", "日本語"})) {
            settings.language = lang - 1;
            language = detectLanguage(settings.language);
            changed = true;
        }
        int preset = settings.quality;
        if (choice("preset", preset, {"low", "balanced", "ultra"})) {
            settings.preset(preset);
            backgroundStage = -1;
            changed = true;
        }
        int view = settings.view == 4 ? 3 : settings.view;
        if (choice("view", view, {"pixel", "panorama", "overview", "classic"})) {
            settings.view = view == 3 ? 4 : view;
            changed = true;
        }
        changed |=
            choice("overviewstyle", settings.overviewStyle, {"auto", "nativeart", "terrainmap"});
        ImGui::TextWrapped("%s", tr("maphelp", language));
        ImGui::TextWrapped("%s", tr("overviewhelp", language));
        if (slider("zoom", settings.zoom, 1, 8, "%d×")) {
            settings.view = 0;
            changed = true;
        }
        if (choice("zoomout", settings.zoomOut, {"1×", "2×", "4×", "8×"})) {
            settings.view = 0;
            changed = true;
        }
        ImGui::Text("%s: %.3g×", tr("effectivescale", language), settings.pixelScale());
        if (button("pixelreset")) {
            settings.view = 0;
            settings.zoom = 1;
            settings.zoomOut = 0;
            changed = true;
        }
        if (slider("panoramaheight", settings.panoramaHeight, 224, 4096, "%d px")) {
            settings.view = 1;
            changed = true;
        }
        check("mapextension", settings.mapExtension);
        check("smoothmap", settings.smoothMap);
        if (core.loaded) {
            auto scene = core.world.inspect(core.snapshot, core.game);
            ImGui::TextWrapped("%s", tr(scene.ready ? (scene.family >= 4 ? "atlascapability"
                                                                         : "platformcapability")
                                                    : "nativecapability",
                                        language));
        }
        ImGui::TextWrapped("%s", tr("pixelhelp", language));
#ifndef SCU_QUEST
        bool full = settings.fullscreen, span = settings.span;
        int display = settings.display;
        check("fullscreen", settings.fullscreen);
        check("span", settings.span);
        ImGui::TextWrapped("%s", tr("display", language));
        std::vector<std::string> names{tr("currentdisplay", language)};
        for (int n = 0; n < SDL_GetNumVideoDisplays(); ++n) {
            SDL_Rect bounds{};
            SDL_GetDisplayBounds(n, &bounds);
            names.push_back(std::to_string(n + 1) + " · " + SDL_GetDisplayName(n) + " · " +
                            std::to_string(bounds.w) + " × " + std::to_string(bounds.h));
        }
        int chosen = std::clamp(settings.display + 1, 0, int(names.size()) - 1);
        std::vector<const char *> list;
        for (auto &name : names)
            list.push_back(name.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##display", &chosen, list.data(), int(list.size()))) {
            settings.display = chosen - 1;
            changed = true;
        }
        control("display");
        if (full != settings.fullscreen || span != settings.span || display != settings.display)
            applyDisplay();
#endif
        changed |= choice("menulayout", settings.menuLayout, {"fullmenu", "compactmenu"});
        changed |=
            choice("uiscale", settings.uiScale, {"auto", "smalltext", "largetext", "hugetext"});
        check("contrast", settings.highContrast);
#ifndef SCU_QUEST
        check("vsync", settings.vsync);
#endif
        check("nearest", settings.integerScale);
#ifndef SCU_QUEST
        changed |= choice("desktoppresentation", settings.desktopPresentation,
                          {"desktopplain", "desktopcabinet", "desktopcrt"});
        if (settings.desktopPresentation) {
            slide("desktopframescale", settings.desktopFrameScale, 50, 130, "%d%%");
            if (settings.desktopPresentation == 1)
                slide("desktopcabinetangle", settings.desktopCabinetAngle, 10, 60, "%d°");
            else {
                slide("desktopcrtstrength", settings.desktopCrtStrength, 0, 100, "%d%%");
                check("desktopcrtmask", settings.desktopCrtMask);
                check("desktopsurround", settings.desktopSurround);
                slide("questsurroundbrightness", settings.questSurroundBrightness, 0, 100, "%d%%");
            }
        }
#endif
        slide("nativescale", settings.nativeScale, 0, 8);
        slide("workers", settings.workers, 0, 16);
#ifndef SCU_QUEST
        bool soft = settings.renderer != 0;
        if (tick("soft", soft)) {
            settings.renderer = soft ? 1 : 0;
            changed = true;
        }
#endif
        slide("scanlines", settings.scanlines, 0, 60);
        check("performance", settings.performance);
        if (button("defaults"))
            requestAction(3);
        break;
    }
    case 2:
        if (button("showstage")) {
            settings.view = 2;
            settings.locator = true;
            changed = true;
        }
        if (button("focusplayer")) {
            settings.view = 0;
            settings.camera = 1;
            changed = true;
        }
        check("playerinset", settings.playerInset);
        check("enemypreview", settings.enemyPreview);
        ImGui::BeginDisabled(!settings.enemyPreview);
        check("bosspreview", settings.bossPreview);
        check("scriptpreview", settings.scriptPreview);
        changed |= choice("enemystyle", settings.enemyStyle,
                          {"enemyspritesmarkers", "enemysprites", "enemymarkers"});
        slide("enemyopacity", settings.enemyOpacity, 10, 80, "%d%%");
        check("spawntint", settings.spawnTint);
        check("spawnlabels", settings.spawnLabels);
        check("fitspawns", settings.fitSpawns);
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", tr("enemyhelp", language));
        if (choice("cameramode", settings.camera,
                   {"bounded", "centred", "fixed", "free", "level"})) {
            settings.view = settings.camera == 4 ? 2 : 0;
            changed = true;
        }
        ImGui::TextWrapped("%s", tr("overviewcamera", language));
        ImGui::BeginDisabled(settings.view == 2);
        if (button("freeze")) {
            core.world.freeze(core.world.inspect(core.snapshot, core.game), settings);
            changed = true;
        }
        ImGui::EndDisabled();
        if (button("follow")) {
            settings.camera = 0;
            settings.view = 0;
            changed = true;
        }
        slide("panspeed", settings.panSpeed, 1, 32);
        changed |= choice("hud", settings.hud, {"hudcentre", "hudplayer", "hudedge", "hidden"});
        slide("hudsize", settings.hudScale, 0, 6);
        check("locator", settings.locator);
        check("minimap", settings.minimap);
        check("ghost", settings.ghost);
        ImGui::Separator();
        check("help", settings.help);
        changed |= choice("hintcorner", settings.hintCorner,
                          {"bottomright", "bottomleft", "topright", "topleft"});
        changed |= choice("hintdetail", settings.hintDetail,
                          {"hintcompact", "hintdevice", "hintdetailed"});
        slide("hintscale", settings.hintScale, 60, 180, "%d%%");
        slide("hintopacity", settings.hintOpacity, 20, 100, "%d%%");
        slide("hintmargin", settings.hintMargin, 0, 64, "%d px");
        slide("hintseconds", settings.hintSeconds, 0, 30, "%d s");
        slide("noticeseconds", settings.noticeSeconds, 0, 30, "%d s");
        check("rewindindicator", settings.rewindIndicator);
        break;
    case 3:
        changed |= choice("backgroundstyle", settings.background,
                          {"blurred", "sharpbg", "parallaxbg", "gradient", "dark", "black"});
        slide("blur", settings.blur, 0, 16);
        slide("brightness", settings.brightness, 10, 100, "%d%%");
        check("parallax", settings.parallax);
        check("motion", settings.reduceMotion);
        ImGui::TextWrapped("%s", tr("bghelp", language));
        if (changed)
            backgroundStage = -1;
        break;
    case 4:
        slide("volume", settings.volume, 0, 100, "%d%%");
        check("mute", settings.mute);
        check("pausefocus", settings.pauseUnfocused);
        ImGui::TextWrapped("%s", tr("audiohelp", language));
        break;
    case 5: {
        changed |=
            choice("padlabels", settings.padLabels, {"auto", "Xbox", "PlayStation", "Nintendo"});
        changed |=
            choice("jumpbuttons", settings.jumpLayout, {"jumpclassic", "jumpabc", "jumpbac"});
#ifdef SCU_QUEST
        if (settings.questTouch)
            ImGui::TextWrapped("%s: %s", tr("rewindbutton", language),
                               tr("questtrigger", language));
        else
#endif
            changed |=
                choice("rewindbutton", settings.rewindButton, {"L2 / LT", "L1 / LB", "R2 / RT"});
        check("quickcontrols", settings.quickControls);
        slide("deadzone", settings.deadzone, 3000, 28000);
        ImGui::TextWrapped("%s", tr("controllers", language));
        bool any = false;
        for (int p = 0; p < 2; ++p)
            if (pads[p]) {
                any = true;
                ImGui::TextWrapped("%d · %s", p + 1, SDL_GameControllerName(pads[p]));
                auto dx = SDL_GameControllerGetAxis(pads[p], SDL_CONTROLLER_AXIS_LEFTX);
                auto dy = SDL_GameControllerGetAxis(pads[p], SDL_CONTROLLER_AXIS_LEFTY);
                ImGui::Text("X: %d  Y: %d", dx, dy);
            }
        if (!any)
            ImGui::TextWrapped("%s", tr("nopad", language));
        ImGui::Separator();
        ImGui::TextWrapped("%s", navigationHint().c_str());
        ImGui::TextWrapped("%s", tr("quickhelp", language));
        ImGui::TextWrapped("%s", tr("keyhelp", language));
        ImGui::TextWrapped("%s", tr("inputhelp", language));
        break;
    }
    case 6:
        check("lockon", settings.lockon);
        ImGui::TextWrapped("%s", tr("lockonhelp", language));
        check("spindash", settings.spindash);
        ImGui::TextWrapped("%s", tr("dashhelp", language));
        check("fastboot", settings.fastBoot);
        check("spritelimit", settings.noSpriteLimit);
        check("rewind", settings.rewind);
        check("rewindaudio", settings.rewindAudio);
        changed |= choice("rewindmemory", settings.rewindMemory, {"24 MB", "96 MB", "192 MB"});
        ImGui::TextWrapped("%s", tr("rewindhelp", language));
        check("assist", settings.invincible);
        check("air", settings.infiniteAir);
        changed |= choice("region", settings.region,
                          {"auto", "USA / NTSC", "Europe / PAL", "Japan / NTSC"});
        check("confirmactions", settings.confirmActions);
        check("confirmquit", settings.confirmQuit);
        ImGui::TextWrapped("%s", tr("accuracy", language));
        break;
    case 7: {
        check("autosave", settings.autosave);
        ImGui::TextWrapped("%s", tr("slothelp", language));
        for (int n = 1; n <= 9; ++n) {
            auto path =
                library.dataDir / "saves" / (core.saveKey + "-" + std::to_string(n) + ".state");
            std::error_code ec;
            bool exists = core.loaded && fs::exists(path, ec);
            std::string status = tr(exists ? "occupied" : "emptyslot", language);
            if (exists) {
                auto stamp = fs::last_write_time(path, ec);
                if (!ec) {
                    auto clock = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        stamp - fs::file_time_type::clock::now() +
                        std::chrono::system_clock::now());
                    std::time_t time = std::chrono::system_clock::to_time_t(clock);
                    char buf[32]{};
                    if (auto tm = std::localtime(&time))
                        std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", tm);
                    status += " · ";
                    status += buf;
                }
            }
            std::string label = std::string(tr("slotnumber", language)) + " " + std::to_string(n) +
                                " · " + status + "###slot" + std::to_string(n);
            if (ImGui::Selectable(label.c_str(), settings.slot == n)) {
                settings.slot = n;
                changed = true;
            }
            control(("slot" + std::to_string(n)).c_str());
        }
        ImGui::BeginDisabled(!core.loaded);
        if (button("save")) {
            auto path = library.dataDir / "saves" /
                        (core.saveKey + "-" + std::to_string(settings.slot) + ".state");
            if (fs::exists(path))
                requestAction(5, settings.slot);
            else {
                try {
                    core.save(settings.slot);
                    notify(tr("saved", language));
                } catch (const std::exception &e) {
                    error(e);
                }
            }
        }
        std::error_code ec;
        auto path = library.dataDir / "saves" /
                    (core.saveKey + "-" + std::to_string(settings.slot) + ".state");
        ImGui::BeginDisabled(!fs::exists(path, ec));
        if (button("load"))
            requestAction(4, settings.slot);
        ImGui::EndDisabled();
        if (button("reset"))
            requestAction(2);
        if (button("capture"))
            screenshotRequested = true;
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", tr("savehelp", language));
        break;
    }
#ifdef SCU_QUEST
    case 9:
        quest->options();
        break;
#endif
    case 8:
        ImGui::Text("Sonic Compilation Ultra %s", SCU_VERSION);
        if (button("sourcehelp"))
            notify(tr("romguide", language));
        ImGui::TextWrapped("%s", tr("romguide", language));
        ImGui::Separator();
        ImGui::TextWrapped("%s", tr("romformats", language));
        ImGui::TextWrapped("%s", tr("romrevision", language));
        if (ImGui::BeginTable("##identity", 3,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
            ImGui::TableSetupColumn(tr("gamename", language));
            ImGui::TableSetupColumn(tr("bytes", language));
            ImGui::TableSetupColumn("CRC32");
            ImGui::TableHeadersRow();
            for (auto &g : games()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", g.name);
                ImGui::TableNextColumn();
                ImGui::Text("%zu", g.size);
                ImGui::TableNextColumn();
                ImGui::Text("%08X", g.crc);
            }
            ImGui::EndTable();
        }
        ImGui::Separator();
        ImGui::TextWrapped("%s", tr("credits", language));
        ImGui::Separator();
        ImGui::TextWrapped("%s", tr("limitations", language));
        break;
    }
    if (changed) {
        dirty = true;
        core.updateOptions();
        SDL_RenderSetVSync(renderer, settings.vsync ? 1 : 0);
        core.clearAudio();
    }
}
void App::changeDirectory(const fs::path &path) {
    std::error_code ec;
    if (!fs::is_directory(path, ec)) {
        notify(tr("pathmissing", language));
        return;
    }
    browsePath = path.u8string();
    snprintf(pathBuffer, sizeof pathBuffer, "%s", browsePath.c_str());
    focusBrowser = true;
}
void App::browserUi() {
    auto &io = ImGui::GetIO();
    if (browserOpen)
        ImGui::OpenPopup("##browse");
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({std::min(1000 * uiFactor, io.DisplaySize.x - 12),
                              std::min(750 * uiFactor, io.DisplaySize.y - 12)});
    if (ImGui::BeginPopupModal("##browse", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        if (!browserOpen)
            ImGui::CloseCurrentPopup();
        ImGui::TextWrapped("%s", tr(folderMode ? "folder" : "import", language));
        ImGui::TextWrapped("%s", browsePath.c_str());
        if (focusBrowser) {
            ImGui::SetKeyboardFocusHere();
            ImGui::SetNavCursorVisible(true);
        }
        if (button(io.DisplaySize.x < 600 ? "upshort" : "up"))
            changeDirectory(fs::u8path(browsePath).parent_path());
        control("up");
        nextButton(io.DisplaySize.x < 600 ? "pathshort" : "editpath");
        if (button(io.DisplaySize.x < 600 ? "pathshort" : "editpath")) {
            pathEditor = true;
            snprintf(pathBuffer, sizeof pathBuffer, "%s", browsePath.c_str());
        }
        control("editpath");
        nextButton("cancel");
        if (button("cancel")) {
            browserOpen = false;
            ImGui::CloseCurrentPopup();
            focusMenu = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) && !pathEditor)
            changeDirectory(fs::u8path(browsePath).parent_path());
        ImGui::SetNextItemWidth(-1);
        std::vector<std::string> roots;
#ifdef _WIN32
        for (char d = 'A'; d <= 'Z'; ++d) {
            std::string root = std::string(1, d) + ":/";
            std::error_code ec;
            if (fs::is_directory(fs::u8path(root), ec))
                roots.push_back(root);
        }
#else
        roots = {"/", fs::current_path().u8string(), "/Volumes", "/media", "/mnt"};
        if (const char *home = SDL_getenv("HOME"))
            roots.insert(roots.begin(), home);
#endif
        if (ImGui::BeginCombo("##locations", tr("locations", language))) {
            for (auto &root : roots)
                if (ImGui::Selectable(root.c_str()))
                    changeDirectory(fs::u8path(root));
            ImGui::EndCombo();
        }
        control("locations");
        if (folderMode && button("thisfolder")) {
            import(fs::u8path(browsePath), true);
            browserOpen = false;
            ImGui::CloseCurrentPopup();
        }
        if (!notice.empty())
            ImGui::TextWrapped("%s", notice.c_str());
        ImGui::BeginChild("##files", {0, 0},
                          ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened);
        scrollPage();
        if (focusBrowser)
            ImGui::SetScrollY(0);
        std::error_code ec;
        std::vector<fs::directory_entry> files;
        fs::directory_iterator it(fs::u8path(browsePath),
                                  fs::directory_options::skip_permission_denied, ec);
        for (fs::directory_iterator end; !ec && it != end; it.increment(ec)) {
            std::error_code fileError;
            bool dir = it->is_directory(fileError);
            auto ext = it->path().extension().u8string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return char(std::tolower(c)); });
            if (dir || (!folderMode && (ext == ".zip" || ext == ".bin" || ext == ".md" ||
                                        ext == ".gen" || ext == ".smd")))
                files.push_back(*it);
        }
        std::sort(files.begin(), files.end(), [](auto &a, auto &b) {
            std::error_code ea, eb;
            bool da = a.is_directory(ea), db = b.is_directory(eb);
            return da != db ? da > db : a.path().filename() < b.path().filename();
        });
        if (files.empty())
            ImGui::TextWrapped("%s", tr(ec ? "pathmissing" : "nofiles", language));
        std::string next;
        for (auto &entry : files) {
            std::error_code error;
            bool dir = entry.is_directory(error);
            auto name = entry.path().filename().u8string();
            std::string label = (dir ? "[+] " : "    ") + name;
            if (ImGui::Selectable(label.c_str())) {
                if (dir)
                    next = entry.path().u8string();
                else {
                    import(entry.path());
                    browserOpen = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            control(("file:" + name).c_str());
        }
        ImGui::EndChild();
        focusBrowser = false;
        if (!next.empty())
            changeDirectory(fs::u8path(next));
        if (pathEditor || ImGui::IsPopupOpen("##path"))
            pathEditorUi();
        ImGui::EndPopup();
    }
}
void App::pathEditorUi() {
    auto &io = ImGui::GetIO();
    if (pathEditor)
        ImGui::OpenPopup("##path");
    ImGui::SetNextWindowSize({std::min(850 * uiFactor, io.DisplaySize.x - 12),
                              std::min(660 * uiFactor, io.DisplaySize.y - 12)});
    if (ImGui::BeginPopupModal("##path", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        if (!pathEditor)
            ImGui::CloseCurrentPopup();
        ImGui::TextWrapped("%s", tr("editpath", language));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##pathtext", pathBuffer, sizeof pathBuffer);
        control("pathtext");
        if (button("applypath") || ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false)) {
            std::error_code ec;
            auto path = fs::u8path(pathBuffer);
            if (fs::is_directory(path, ec)) {
                changeDirectory(path);
                pathEditor = false;
                ImGui::CloseCurrentPopup();
            } else
                notify(tr("pathmissing", language));
        }
        nextButton("backspace");
        if (button("backspace")) {
            size_t n = std::strlen(pathBuffer);
            if (n) {
                --n;
                while (n && (uint8_t(pathBuffer[n]) & 0xC0) == 0x80)
                    --n;
                pathBuffer[n] = 0;
            }
        }
        nextButton("cancel");
        if (button("cancel")) {
            pathEditor = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::BeginChild("##keys", {0, 0}, ImGuiChildFlags_NavFlattened);
        scrollPage();
        static const char keys[] =
            "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:/\\._- ()[]";
        if (ImGui::BeginTable("##keyboard", 10, ImGuiTableFlags_SizingStretchSame)) {
            for (int n = 0; keys[n]; ++n) {
                ImGui::TableNextColumn();
                ImGui::PushID(n);
                char label[2] = {keys[n], 0};
                if (ImGui::Button(keys[n] == ' ' ? "_" : label, {-1, 0})) {
                    size_t len = std::strlen(pathBuffer);
                    if (len + 1 < sizeof pathBuffer) {
                        pathBuffer[len] = keys[n];
                        pathBuffer[len + 1] = 0;
                    }
                }
                control((std::string("key:") + keys[n]).c_str());
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
}
} // namespace scu
