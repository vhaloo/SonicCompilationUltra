#include "ui.h"
#include "quest.h"
#include "mobile.h"
#include "voxel_export.h"
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
static constexpr int voxelFollowHeight = 288;
static constexpr int voxelStickDeadzone = 5200;
void App::renderImGui(ImDrawData *data) {
    const auto scale = data->FramebufferScale;
    if (scale.x == 1.f && scale.y == 1.f) {
        ImGui_ImplSDLRenderer2_RenderDrawData(data, renderer);
        return;
    }
    // The SDL2 ImGui backend scales scissors only. SDL must scale vertices and
    // scissors together so touch positions and rendered controls agree.
    float previousX, previousY;
    SDL_RenderGetScale(renderer, &previousX, &previousY);
    SDL_Rect viewport{}, clip{};
    SDL_RenderGetViewport(renderer, &viewport);
    const bool clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_RenderGetClipRect(renderer, &clip);
    SDL_RenderSetScale(renderer, scale.x, scale.y);
    ImGui_ImplSDLRenderer2_RenderDrawData(data, renderer);
    SDL_RenderSetScale(renderer, previousX, previousY);
    SDL_RenderSetViewport(renderer, &viewport);
    SDL_RenderSetClipRect(renderer, clipped ? &clip : nullptr);
}
bool App::drawGameExample(HelpPicture picture) {
    struct Comparison {
        const char *key;
        const char *file;
        const char *first;
        const char *second;
        bool wide = false;
        bool closeup = false;
    };
    static const Comparison comparisons[] = {
        {"overviewstyle", "terrain.bmp", "nativeart", "terrainmap", true},
        {"voxelframing", "voxel-framing.bmp", "voxelfollow", "voxelselectedview", true},
        {"voxeldetail", "voxel-detail.bmp", "voxelcompact", "voxelveryfine", false, true},
        {"voxelpreset", "voxel-style.bmp", "voxelsubtle", "voxelbold", false, true},
        {"voxeladjustments", "voxel-style.bmp", "voxelsubtle", "voxelbold", false, true},
        {"voxelthickness", "voxel-thickness.bmp", "guideminimum", "guidemaximum", false, true},
        {"voxelspacing", "voxel-spacing.bmp", "guideminimum", "guidemaximum", false, true},
        {"voxelrelief", "voxel-relief.bmp", "guideminimum", "guidemaximum", false, true},
        {"voxellight", "voxel-light.bmp", "guideminimum", "guidemaximum", false, true},
        {"voxelbackground", "voxel-background.bmp", "off", "guideenabled", false, true},
        {"voxelyaw", "voxel-angle.bmp", "voxelfront", "voxelangle", false, true},
        {"voxelpitch", "voxel-angle.bmp", "voxelfront", "voxelangle", false, true},
        {"voxelfront", "voxel-angle.bmp", "voxelfront", "voxelangle", false, true},
        {"voxelangle", "voxel-angle.bmp", "voxelfront", "voxelangle", false, true},
        {"backgroundstyle", "background-wide.bmp", "blurred", "gradient", true},
        {"blur", "background-blur.bmp", "guideminimum", "guidemaximum", true},
        {"brightness", "background-brightness.bmp", "guideminimum", "guidemaximum", true},
    };
    const auto comparison = std::find_if(std::begin(comparisons), std::end(comparisons),
                                         [&](const auto &item) { return item.key == guideKey; });
    const char *file = nullptr;
    int variant = 0;
    if (comparison != std::end(comparisons)) {
        file = comparison->file;
    } else if (picture == HelpPicture::View || picture == HelpPicture::Camera ||
               picture == HelpPicture::Zoom) {
        file = "views.bmp";
        if (guideKey == "lookpanorama" || guideKey == "panoramaheight")
            variant = 2;
    } else if (picture == HelpPicture::Crt) {
        file = "views.bmp";
        variant = 3;
    } else if (picture == HelpPicture::Depth) {
        file = "depth.bmp";
    } else if (picture == HelpPicture::Background) {
        file = "background.bmp";
    }
    if (!file)
        return false;
    const Game *game = gameById(core.loaded ? core.currentId : settings.lastGame);
    fs::path path;
    if (game)
        path = library.dataDir / "guide-captures" / game->id / file;
    if (path != guideCapturePath) {
        guideCapturePath = path;
        if (guideCapture)
            retiredGuideTextures.push_back(guideCapture);
        guideCapture = nullptr;
        std::error_code ec;
        if (!path.empty() && fs::is_regular_file(path, ec) && fs::file_size(path, ec) < 4 * 1024 * 1024) {
            if (auto surface = SDL_LoadBMP(path.u8string().c_str())) {
                if (surface->w > 0 && surface->h > 0 && surface->w <= 1024 && surface->h <= 1024)
                    guideCapture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                if (guideCapture)
                    SDL_SetTextureScaleMode(guideCapture, SDL_ScaleModeLinear);
            }
        }
    }
    if (guideCapture) {
        const char *views[] = {"guideoriginal", "guideoverview", "lookpanorama", "guideexamplecrt"};
        const char *depth[] = {"voxelflat", "voxelthick", "voxellayers", "voxelsculpt"};
        const char *background[] = {"guideoriginal", "guidebackgroundoff"};
        const bool custom = comparison != std::end(comparisons);
        const bool wide = custom && comparison->wide;
        const bool relief = !custom && picture == HelpPicture::Depth;
        const bool backdrop = !custom && picture == HelpPicture::Background;
        const char *customNames[] = {custom ? comparison->first : "", custom ? comparison->second : ""};
        const char *const *names = custom ? customNames : relief ? depth : backdrop ? background : views;
        const int count = relief ? 4 : 2;
        const int columns = wide ? 1 : 2;
        const int rows = wide || !(custom || backdrop) ? 2 : 1;
        ImGui::TextWrapped("%s · %s", tr("gamecaptureexample", language), game->name);
        if (wide || (custom && comparison->closeup))
            ImGui::TextWrapped("%s", tr(wide ? "guidefullwidth" : "guidecloseup", language));
        const bool pair = !wide && ImGui::GetContentRegionAvail().x >= 270;
        if (ImGui::BeginTable("##capture-pairs", pair ? 2 : 1, ImGuiTableFlags_SizingStretchSame)) {
            for (int i = 0; i < count; ++i) {
                ImGui::TableNextColumn();
                int cell = i == 1 && variant ? variant : i;
                ImGui::TextWrapped("%s", tr(names[cell], language));
                const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
                const float u = float(cell % columns) / columns, v = float(cell / columns) / rows;
                ImGui::Image(reinterpret_cast<ImTextureID>(guideCapture),
                             {width, width * 9 / (wide ? 48 : 16)}, {u, v},
                             {u + 1.f / columns, v + 1.f / rows});
            }
            ImGui::EndTable();
        }
        return true;
    }
    if (!core.loaded || core.native.empty())
        return false;
    if (!guideNative || guideNativeW != core.nativeW || guideNativeH != core.nativeH) {
        if (guideNative)
            retiredGuideTextures.push_back(guideNative);
        guideNativeW = core.nativeW;
        guideNativeH = core.nativeH;
        guideNative = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                        guideNativeW, guideNativeH);
        guideNativeVersion = ~uint64_t(0);
    }
    if (!guideNative)
        return false;
    if (guideNativeVersion != core.imageVersion) {
        if (SDL_UpdateTexture(guideNative, nullptr, core.native.data(), core.nativeW * 4))
            return false;
        guideNativeVersion = core.imageVersion;
    }
    ImGui::TextWrapped("%s", tr("currentgamecapture", language));
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    ImGui::Image(reinterpret_cast<ImTextureID>(guideNative), {width, width * guideNativeH / guideNativeW});
    return true;
}
void App::settingGuide(bool compact) {
    const auto *help = settingHelp(guideKey.c_str());
    if (!help)
        help = settingHelp("view");
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.9f, 0.94f, 1));
    ImGui::TextWrapped("%s", tr(help->key, language));
    ImGui::PopStyleColor();
    if (drawGameExample(help->picture)) {
        ImGui::TextWrapped("%s", help->text[std::clamp(language, 0, 3)]);
        return;
    }
    if (settingsPreview) {
        if (compact)
            ImGui::TextWrapped("%s", help->text[std::clamp(language, 0, 3)]);
        int index = std::clamp(int(help->picture), 0, 19);
        float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
        ImVec2 a{(index % 4) / 4.f, (index / 4) / 5.f},
            b{(index % 4 + 1) / 4.f, (index / 4 + 1) / 5.f};
        ImGui::Image(reinterpret_cast<ImTextureID>(settingsPreview), {width, width * 180 / 320}, a,
                     b);
        ImGui::TextDisabled("%s", tr("previewexample", language));
        if (!compact)
            ImGui::TextWrapped("%s", help->text[std::clamp(language, 0, 3)]);
        return;
    }
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
    case HelpPicture::Depth: {
        for (int i = 0; i < 3; ++i) {
            float x = .16f + i * .08f, y = .66f - i * .17f;
            ImVec2 a = point(x, y), b = point(x + .47f, y + .08f), c = point(x + .61f, y - .10f),
                   d = point(x + .15f, y - .18f);
            draw->AddQuadFilled(a, b, c, d, i == 1 ? muted : cyan);
            for (auto p : {a, b, c, d})
                draw->AddLine(p, {p.x, p.y + height * .08f}, gold, 2);
            draw->AddQuad(a, b, c, d, gold, 2);
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
    case HelpPicture::Wear:
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
void App::settingBadge(const char *key) {
    const auto *help = settingHelp(key);
    if (!help)
        return;
    ImGui::PushID(key);
    float side = ImGui::GetTextLineHeight() + 6;
    if (ImGui::Button("##explain", {side, side})) {
        guideKey = key;
        guideRequested = true;
    }
    auto draw = ImGui::GetWindowDrawList();
    auto p = ImGui::GetItemRectMin();
    ImU32 color = IM_COL32(102, 220, 231, 255);
    float q = side * .20f;
    auto line = [&](float x, float y, float xx, float yy) {
        draw->AddLine({p.x + x * side, p.y + y * side}, {p.x + xx * side, p.y + yy * side}, color,
                      1.5f);
    };
    int icon = int(help->picture);
    if (icon == 19) {
        line(.5f, .13f, .84f, .32f);
        line(.84f, .32f, .5f, .52f);
        line(.5f, .52f, .16f, .32f);
        line(.16f, .32f, .5f, .13f);
        line(.16f, .32f, .16f, .70f);
        line(.5f, .52f, .5f, .90f);
        line(.84f, .32f, .84f, .70f);
        line(.16f, .70f, .5f, .90f);
        line(.5f, .90f, .84f, .70f);
    } else if (icon == 11) {
        for (int i = 0; i < 3; ++i)
            draw->AddRectFilled({p.x + q + i * side * .22f, p.y + side * (.65f - i * .15f)},
                                {p.x + q + i * side * .22f + side * .13f, p.y + side * .81f}, color,
                                1);
    } else if (icon == 18 || icon == 6 || icon == 14) {
        draw->AddCircle({p.x + side * .5f, p.y + side * .5f}, side * .29f, color, 16, 1.5f);
        line(.5f, .3f, .5f, .55f);
        line(.5f, .55f, .67f, .55f);
    } else if (icon == 1 || icon == 9) {
        draw->AddCircle({p.x + side * .43f, p.y + side * .43f}, side * .2f, color, 16, 1.5f);
        line(.6f, .6f, .82f, .82f);
    } else if (icon == 12 || icon == 14) {
        line(.25f, .35f, .25f, .65f);
        line(.5f, .2f, .5f, .8f);
        line(.75f, .35f, .75f, .65f);
    } else {
        draw->AddRect({p.x + q, p.y + q}, {p.x + side - q, p.y + side - q}, color, 2, 0, 1.5f);
        line(.3f, .68f, .7f, .68f);
    }
    if (ImGui::IsItemHovered()) {
        hoveredHeading = key;
        headingReady = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal);
    }
    ImGui::PopID();
    ImGui::SameLine();
}
void App::settingTitle(const char *key) {
    hoveredHeading.clear();
    headingReady = false;
    settingBadge(key);
    ImGui::TextWrapped("%s", tr(key, language));
    if (ImGui::IsItemHovered()) {
        hoveredHeading = key;
        headingReady = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal);
    }
}
void App::control(const char *key) {
    if (settingHelp(key) && !guideOpen && !guideRequested &&
        (ImGui::IsItemFocused() || ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) ||
         hoveredHeading == key))
        guideKey = key;
#ifdef SCU_UI_TESTS
    controls.emplace(key, ImGui::GetItemID());
    auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
    controlRects.emplace(key, ImVec4(min.x, min.y, max.x, max.y));
#endif
    if (settingHelp(key) && guideTooltipFrame != ImGui::GetFrameCount() && !menuVerificationOnly &&
        !guideOpen && !guideRequested && !settings.wearActive &&
        ((hoveredHeading == key && headingReady) ||
         ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal |
                              ImGuiHoveredFlags_AllowWhenDisabled))) {
        guideTooltipFrame = ImGui::GetFrameCount();
        ImGui::SetNextWindowSize(
            {std::min(310.f * uiFactor, ImGui::GetIO().DisplaySize.x * .70f), 0});
        ImGui::BeginTooltip();
        settingGuide(true);
        ImGui::EndTooltip();
    }
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
    hoveredHeading.clear();
    headingReady = false;
    settingBadge(key);
    bool changed = ImGui::Checkbox((std::string("##") + key).c_str(), &value);
    control(key);
    ImGui::SameLine();
    ImGui::TextWrapped("%s", tr(key, language));
    if (ImGui::IsItemClicked() && !guideRequested) {
        value = !value;
        changed = true;
    }
    ImGui::EndGroup();
    control(key);
    return changed;
}
bool App::choice(const char *key, int &value, std::initializer_list<const char *> keys) {
    settingTitle(key);
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
    settingTitle(key);
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
#if !defined(SCU_MOBILE) && !defined(SCU_QUEST)
    if (!settings.renderer)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif
#ifdef SCU_MOBILE
    settings.renderer = 0;
    settings.vsync = false;
    settings.fullscreen = false;
    settings.span = false;
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
#endif
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
#if !defined(SCU_MOBILE) && !defined(SCU_QUEST)
    if (!renderer && !settings.renderer) {
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        renderer = SDL_CreateRenderer(window, -1,
                                      SDL_RENDERER_ACCELERATED |
                                          (settings.vsync ? SDL_RENDERER_PRESENTVSYNC : 0));
    }
#endif
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
        throw std::runtime_error(SDL_GetError());
    loadSettingsPreview();
    SDL_RendererInfo rendererInfo{};
    SDL_GetRendererInfo(renderer, &rendererInfo);
    voxelRenderer = rendererInfo.name &&
                    (!std::strcmp(rendererInfo.name, "opengl") ||
                     !std::strcmp(rendererInfo.name, "opengles2"));
    if (rendererInfo.max_texture_width && rendererInfo.max_texture_height)
        voxelTextureLimit = std::min(rendererInfo.max_texture_width, rendererInfo.max_texture_height);
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
    volumeGpu.release();
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
    if (settingsPreview)
        SDL_DestroyTexture(settingsPreview);
    if (guideCapture)
        SDL_DestroyTexture(guideCapture);
    if (guideNative)
        SDL_DestroyTexture(guideNative);
    for (auto *texture : retiredGuideTextures)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
}
void App::loadSettingsPreview() {
    if (settingsPreview)
        SDL_DestroyTexture(settingsPreview);
    settingsPreview = nullptr;
    if (guideCapture)
        SDL_DestroyTexture(guideCapture);
    if (guideNative)
        SDL_DestroyTexture(guideNative);
    guideCapture = guideNative = nullptr;
    guideCapturePath.clear();
    if (auto surface = SDL_LoadBMP((basePath / "assets/settings-preview.bmp").u8string().c_str())) {
        settingsPreview = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        if (settingsPreview)
            SDL_SetTextureScaleMode(settingsPreview, SDL_ScaleModeLinear);
    }
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
#if defined(SCU_QUEST) || defined(SCU_MOBILE)
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
    voxelPanX = voxelPanY = 0;
    return true;
}
void App::beginBrowser(bool folder) {
#ifdef SCU_MOBILE
    if (mobile && mobile->chooseRom()) {
        return;
    }
#endif
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
    if (e.type == SDL_RENDER_TARGETS_RESET || e.type == SDL_RENDER_DEVICE_RESET) {
        if (e.type == SDL_RENDER_TARGETS_RESET)
            volumeGpu.release();
        else
            volumeGpu.invalidate();
        for (auto **texture : {&worldTexture, &nativeTexture, &bgTexture}) {
            if (*texture)
                SDL_DestroyTexture(*texture);
            *texture = nullptr;
        }
        loadSettingsPreview();
        ImGui_ImplSDLRenderer2_DestroyDeviceObjects();
        lastImageVersion = uploadedNative = uploadedBackdrop = ~uint64_t(0);
        worldTextureStale = true;
    }
#ifdef SCU_MOBILE
    if (mobile && mobile->event(e))
        return;
#endif
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
    }
    if (e.type == SDL_DROPFILE) {
        if (e.drop.file) {
            import(fs::u8path(e.drop.file));
            SDL_free(e.drop.file);
        }
        menu = true;
        tab = 0;
    }
    const bool orbitAxis = e.type == SDL_CONTROLLERAXISMOTION && settings.voxelMode && !menu &&
                           (e.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTX ||
                            e.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY);
    if (e.type == SDL_CONTROLLERBUTTONDOWN ||
        (e.type == SDL_CONTROLLERAXISMOTION &&
         std::abs(int(e.caxis.value)) > (orbitAxis ? voxelStickDeadzone : settings.deadzone))) {
        padUsed = true;
        const auto instance = e.type == SDL_CONTROLLERAXISMOTION ? e.caxis.which : e.cbutton.which;
        for (int p = 0; p < 2; ++p)
            if (pads[p] &&
                SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pads[p])) == instance)
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
        if (!menu && core.loaded && settings.voxelMode && b == SDL_CONTROLLER_BUTTON_RIGHTSTICK) {
            resetVoxelOrbit();
            return;
        }
        if (!menu && core.loaded && settings.quickControls && pads[activePad] &&
            SDL_GameControllerGetButton(pads[activePad], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
            if (b == SDL_CONTROLLER_BUTTON_Y) {
                if (settings.wearActive)
                    settings.wearZoom = settings.wearZoom ? 0 : 1000;
                else {
                    const bool overview = settings.view == 2 &&
                                          (!settings.voxelMode || settings.voxelFraming == 1);
                    selectView(overview ? 0 : 2);
                }
            } else if (b == SDL_CONTROLLER_BUTTON_DPAD_UP || b == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                selectView(0);
                settings.stepZoom(b == SDL_CONTROLLER_BUTTON_DPAD_UP ? 1 : -1);
            } else if (b == SDL_CONTROLLER_BUTTON_X) {
                if (settings.camera == 2)
                    settings.camera = 0;
                else
                    freezeView();
            } else if (b == SDL_CONTROLLER_BUTTON_B) {
                selectView(0);
                settings.voxelFraming = 0;
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
                selectView(0);
                settings.stepZoom(1);
                dirty = true;
            }
            if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
                selectView(0);
                settings.stepZoom(-1);
                dirty = true;
            }
            if (k == SDLK_f) {
                freezeView();
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
        bool pinballTriggers = core.currentId == "spinball" && settings.spinballTriggers;
        if (pinballTriggers)
            rewindBinding = 1;
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
    if (core.currentId == "spinball" && settings.spinballTriggers)
        for (int p = 0; p < 2; ++p)
            if (pads[p]) {
                set(p, SDL_GameControllerGetAxis(pads[p], SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 8192,
                    RETRO_DEVICE_ID_JOYPAD_Y);
                set(p, SDL_GameControllerGetAxis(pads[p], SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 8192,
                    RETRO_DEVICE_ID_JOYPAD_B);
            }
#ifdef SCU_QUEST
    core.buttons[0] |= spatialButtons;
    rewindHeld |= settings.rewind && spatialRewind;
#endif
#ifdef SCU_MOBILE
    core.buttons[0] |= mobileButtons;
    rewindHeld |= settings.rewind && mobileRewind;
#endif
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
        pan(unpaced ? period : elapsed);
    return executed;
}
void App::resetVoxelOrbit() {
    const Settings defaults;
    settings.voxelYaw = defaults.voxelYaw;
    settings.voxelPitch = defaults.voxelPitch;
    orbitYawRemainder = orbitPitchRemainder = 0;
    voxelPanX = voxelPanY = 0;
    dirty = true;
}
bool App::voxelFollows(const Scene &scene) const {
#ifdef SCU_QUEST
    (void)scene;
    return false;
#else
    return voxelRenderer && std::strcmp(volumeGpu.status(), "voxelunsupported") &&
           settings.voxelMode && settings.voxelFraming == 0 && !settings.wearActive &&
           !settings.desktopPresentation && settings.view != 4 && scene.ready && scene.family != 4;
#endif
}
void App::selectView(int view) {
    settings.view = view;
    if (settings.voxelMode)
        settings.voxelFraming = 1;
}
void App::freezeView() {
    const auto scene = core.world.inspect(core.snapshot, core.game);
    if (voxelFollows(scene)) {
        settings.panoramaHeight = voxelFollowHeight;
        selectView(1);
    } else if (settings.view == 2) {
        selectView(0);
        settings.left = int(scene.playerX - drawableW / settings.pixelScale() / 2);
        settings.top = int(scene.playerY - drawableH / settings.pixelScale() / 2);
        settings.camera = 2;
        return;
    }
    if (cachedExpanded)
        core.world.freeze(scene, settings);
    else {
        settings.left = scene.cameraX;
        settings.top = scene.cameraY;
        settings.camera = 2;
    }
}
void App::pan(double elapsed) {
    if (menu)
        return;
    float dx = 0, dy = 0;
    if (settings.voxelMode) {
        auto *pad = pads[activePad];
        constexpr float deadzone = voxelStickDeadzone / 32767.f;
        float best = deadzone;
        // A companion or an idle player-one device cannot mask the stick on
        // the controller the player is actually using.
        for (auto *candidate : {pads[activePad], pads[1 - activePad]}) {
            if (!candidate || !SDL_GameControllerGetAttached(candidate))
                continue;
            const float x = SDL_GameControllerGetAxis(candidate, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.f;
            const float y = SDL_GameControllerGetAxis(candidate, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.f;
            const float magnitude = std::hypot(x, y);
            if (magnitude > best) {
                const float scale = std::min(1.f, (magnitude - deadzone) / (1 - deadzone)) / magnitude;
                dx = x * scale;
                dy = y * scale;
                best = magnitude;
                pad = candidate;
            }
        }
        if (dx || dy) {
            const float dt = float(std::clamp(elapsed, 0., .05));
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
                const auto scene = core.world.inspect(core.snapshot, core.game);
                if (voxelFollows(scene)) {
                    auto move = [](float offset, float delta, int player, int level, double span,
                                   float limit) {
                        const int edge = std::max(0, int(level - span));
                        const int base = std::clamp(int(player - span / 2), 0, edge);
                        return std::clamp(offset + delta, std::max(-limit, float(-base)),
                                          std::min(limit, float(edge - base)));
                    };
                    const double width = double(voxelFollowHeight) * drawableW / std::max(1, drawableH);
                    voxelPanX = move(voxelPanX, dx * 520.f * dt, scene.playerX, scene.width, width, 960);
                    voxelPanY = move(voxelPanY, dy * 360.f * dt, scene.playerY, scene.height,
                                     voxelFollowHeight, 480);
                    return;
                }
            }
            auto turn = [&](int &angle, float &remainder, float movement, int low, int high) {
                remainder += movement;
                const int step = int(remainder);
                remainder -= step;
                const int next = std::clamp(angle + step, low, high);
                if (next != angle) {
                    angle = next;
                    dirty = true;
                }
            };
#ifdef SCU_QUEST
            turn(settings.voxelYaw, orbitYawRemainder, dx * 65.f * dt, -57, 33);
            turn(settings.voxelPitch, orbitPitchRemainder, dy * 48.f * dt, -13, 37);
#else
            turn(settings.voxelYaw, orbitYawRemainder, dx * 115.f * dt, -60, 60);
            turn(settings.voxelPitch, orbitPitchRemainder, dy * 90.f * dt, -60, 60);
#endif
            dx = dy = 0;
        }
    } else if (pads[0]) {
        auto axis = [&](SDL_GameControllerAxis which) {
            int value = SDL_GameControllerGetAxis(pads[0], which);
            float intensity = float(std::max(0, std::abs(value) - settings.deadzone)) /
                              std::max(1, 32767 - settings.deadzone);
            return std::copysign(std::min(1.f, intensity), float(value));
        };
        dx = axis(SDL_CONTROLLER_AXIS_RIGHTX);
        dy = axis(SDL_CONTROLLER_AXIS_RIGHTY);
    }
    if (settings.camera != 3 ||
        (settings.voxelMode && voxelFollows(core.world.inspect(core.snapshot, core.game))))
        return;
    auto k = SDL_GetKeyboardState(nullptr);
    int speed = settings.panSpeed * ((k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) ? 3 : 1);
    settings.left += (k[SDL_SCANCODE_D] - k[SDL_SCANCODE_A]) * speed;
    settings.top += (k[SDL_SCANCODE_S] - k[SDL_SCANCODE_W]) * speed;
    settings.left += int(std::lround(dx * speed));
    settings.top += int(std::lround(dy * speed));
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
#ifdef SCU_MOBILE
    if (mobile) {
        mobile->logicalSize(width, height);
        return;
    }
#endif
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
    if (core.currentId == "spinball" && settings.spinballTriggers)
        binding = "L1 / LB";
#ifdef SCU_QUEST
    if (core.currentId == "spinball" && settings.spinballTriggers)
        binding = "X";
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
        if (core.currentId == "spinball" && settings.spinballTriggers)
            key = pads[activePad] ? "spinballpadhint" : "spinballkeyhint";
#ifdef SCU_QUEST
        if (core.currentId == "spinball" && settings.spinballTriggers && settings.questTouch)
            key = "remote_spinball_hint";
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
        std::snprintf(text, sizeof text, "%.1f ticks/s · %.1f fps · %d%% · CPU %.1f ms · %d × %d",
                      measuredFps, presentationFps, performanceGovernor.scale(),
                      performanceGovernor.cpuMs(), core.world.canvas.width,
                      core.world.canvas.height);
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
bool App::updateNativeTexture() {
    if (core.native.empty())
        return false;
    if (!nativeTexture || nativeTW != core.nativeW || nativeTH != core.nativeH) {
        if (nativeTexture)
            SDL_DestroyTexture(nativeTexture);
        nativeTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, core.nativeW, core.nativeH);
        nativeTW = core.nativeW;
        nativeTH = core.nativeH;
        uploadedNative = ~uint64_t(0);
    }
    if (!nativeTexture)
        return false;
    if (uploadedNative != core.imageVersion) {
        if (SDL_UpdateTexture(nativeTexture, nullptr, core.native.data(), core.nativeW * 4) != 0)
            return false;
        uploadedNative = core.imageVersion;
    }
    return true;
}
void App::present(bool capture) {
    const auto workStarted = SDL_GetPerformanceCounter();
    settings.compositionScale = settings.autoPerformance ? performanceGovernor.scale() : 100;
    settings.compositionPixels = settings.autoPerformance ? (settings.quality == 0   ? 1000000
                                                             : settings.quality == 2 ? 8000000
                                                                                     : 2500000)
                                                          : 0;
#ifdef SCU_MOBILE
    settings.compositionPixels = settings.mobileRenderQuality == 0   ? 1000000
                                 : settings.mobileRenderQuality == 2 ? 8000000
                                                                     : 2500000;
#endif
    Settings transientSettings = settings;
    auto *projectionSettings = &transientSettings;
#ifdef SCU_QUEST
    if (quest) {
        quest->configureRender(transientSettings);
        projectionSettings = &transientSettings;
    }
#else
    if (settings.desktopPresentation && !settings.wearActive) {
        transientSettings.projectionCleanScene = true;
        transientSettings.view = 2;
        transientSettings.camera = 4;
        transientSettings.mapExtension = true;
        transientSettings.fitSpawns = false;
        projectionSettings = &transientSettings;
    }
#endif
    auto &renderSettings = *projectionSettings;
    const Scene scene = core.world.inspect(core.snapshot, core.game);
    const bool followVolume = voxelFollows(scene);
    std::array<int, 2> viewOffset{};
    if (followVolume) {
        renderSettings.view = 1;
        renderSettings.camera = 0;
        renderSettings.panoramaHeight = voxelFollowHeight;
        viewOffset = {int(std::round(voxelPanX / 4)) * 4, int(std::round(voxelPanY / 4)) * 4};
    }
#ifndef SCU_QUEST
    if (scene.ready && scene.family == 4 && renderSettings.voxelMode &&
        renderSettings.voxelFraming == 0 && renderSettings.view != 4 &&
        !settings.wearActive && !settings.desktopPresentation) {
        renderSettings.view = 2;
        renderSettings.camera = 4;
    }
    if (!voxelRenderer)
        renderSettings.voxelMode = 0;
    renderSettings.voxelTextureLimit = voxelTextureLimit;
#endif
    if (renderSettings.voxelMode) {
        renderSettings.smoothMap = false;
        renderSettings.performancePixelLock = false;
    }
    bool useVolume = renderSettings.voxelMode && core.loaded;
    if (!useVolume && voxelWasEnabled)
        volumeGpu.release();
    voxelWasEnabled = useVolume;
#ifndef SCU_MOBILE
    renderSettings.compositionPixels = renderSettings.autoPerformance
                                           ? (renderSettings.quality == 0   ? 1000000
                                              : renderSettings.quality == 2 ? 8000000
                                                                            : 2500000)
                                           : 0;
#endif
    if (renderSettings.autoPerformance && renderSettings.adaptiveEffects) {
        renderSettings.blur =
            std::min(renderSettings.blur, renderSettings.effectsLevel >= 2 ? 16 : 3);
        if (renderSettings.effectsLevel == 0)
            renderSettings.smoothMap = false;
    }
#ifdef SCU_MOBILE
    if (settings.wearActive) {
        renderSettings.view = settings.wearZoomValue >= 999.5 ? 4 : 0;
        renderSettings.desktopPresentation = 0;
        renderSettings.compositionPixels = std::min(renderSettings.compositionPixels, 300000);
    }
#endif
    SDL_GetRendererOutputSize(renderer, &drawableW, &drawableH);
    if (drawableW <= 0 || drawableH <= 0)
        return;
    prepareOverlays();
    nativeBounds = insetBounds = {};
    bool transparentCanvas = renderSettings.background == 6 || renderSettings.projectionTransparent;
    SDL_SetRenderDrawColor(renderer, transparentCanvas ? 0 : 6, transparentCanvas ? 0 : 12,
                           transparentCanvas ? 0 : 24, transparentCanvas ? 0 : 255);
    SDL_RenderClear(renderer);
    uint64_t hash = 1469598103934665603ull;
    for (int v : {renderSettings.voxelMode,
                  renderSettings.voxelDetail,
                  renderSettings.voxelThickness,
                  renderSettings.voxelSpacing,
                  renderSettings.voxelRelief,
                  int(renderSettings.voxelBackground),
                  renderSettings.effectsLevel,
                  drawableW,
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
                  viewOffset[0],
                  viewOffset[1],
                  renderSettings.background,
                  renderSettings.blur,
                  renderSettings.brightness,
                  renderSettings.overviewStyle,
                  renderSettings.quality,
                  renderSettings.compositionScale,
                  renderSettings.compositionPixels,
                  int(renderSettings.wearActive),
                  int(std::round(renderSettings.wearZoomValue)),
                  int(renderSettings.performancePixelLock),
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
                  int(renderSettings.objectPreview),
                  int(renderSettings.frozenPreviews),
                  int(renderSettings.objectOrigins),
                  int(renderSettings.projectionTransparent),
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
                                            drawableH, &core.native, core.nativeW, core.nativeH,
                                            viewOffset);
        cachedVolume =
            !renderSettings.projectionUiOnly && core.loaded &&
            core.world.prepareVolume(core.snapshot, core.game, renderSettings, core.native,
                                     core.nativeW, core.nativeH, cachedExpanded);
        lastImageVersion = core.imageVersion;
        lastPresentationHash = hash;
    }
    bool expanded = core.loaded && cachedExpanded;
    SDL_Texture *volumeTexture = nullptr;
    if (core.loaded && !renderSettings.projectionUiOnly) {
        if (!transparentCanvas) {
            if (refresh || !bgTexture)
                core.world.makeBackdrop(core.snapshot, scene, renderSettings);
            if (!bgTexture) {
                bgTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                              SDL_TEXTUREACCESS_STREAMING, 320, 224);
                uploadedBackdrop = ~uint64_t(0);
            }
            if (bgTexture && uploadedBackdrop != core.world.backdropVersion &&
                SDL_UpdateTexture(bgTexture, nullptr, core.world.backdrop.data(), 320 * 4) == 0)
                uploadedBackdrop = core.world.backdropVersion;
            if (!bgTexture || uploadedBackdrop == ~uint64_t(0)) {
                // The clear colour is also the texture-allocation fallback.
            } else if (renderSettings.background == 0) {
                SDL_SetTextureScaleMode(bgTexture, SDL_ScaleModeLinear);
                const int width = std::max(1, (drawableH * 320 + 112) / 224);
                const int center = (drawableW - width) / 2;
                int first = center, tile = 0;
                while (first > 0) {
                    first -= width;
                    --tile;
                }
                for (int x = first; x < drawableW; x += width, ++tile) {
                    SDL_Rect destination{x, 0, width, drawableH};
                    SDL_RenderCopyEx(renderer, bgTexture, nullptr, &destination, 0, nullptr,
                                     tile % 2 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
                }
            } else if (renderSettings.background == 2 && expanded) {
                SDL_SetTextureScaleMode(bgTexture, SDL_ScaleModeNearest);
                int scale = std::max(1, (int)core.world.canvas.scale), tw = 320 * scale,
                    th = 224 * scale;
                for (int y = 0; y < drawableH; y += th)
                    for (int x = 0; x < drawableW; x += tw) {
                        SDL_Rect d{x, y, tw, th};
                        SDL_RenderCopy(renderer, bgTexture, nullptr, &d);
                    }
            } else {
                SDL_SetTextureScaleMode(bgTexture, renderSettings.background == 1
                                                      ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
                SDL_RenderCopy(renderer, bgTexture, nullptr, nullptr);
            }
        }
        if (expanded) {
            auto &c = core.world.canvas;
#ifndef SCU_QUEST
            if (cachedVolume)
                volumeTexture = volumeGpu.render(renderer, core.world.volume, core.world.volumeMesh,
                                                 renderSettings, drawableW, drawableH);
#endif
            if (volumeTexture) {
                // A voxel picture never samples the flattened scene texture.
                // Restore it completely if the flat fallback is needed later.
                worldTextureStale |= refresh;
            } else {
                if (!worldTexture || textureW != c.width || textureH != c.height) {
                    if (worldTexture)
                        SDL_DestroyTexture(worldTexture);
                    worldTexture =
                        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, c.width, c.height);
                    if (!worldTexture)
                        throw std::runtime_error(SDL_GetError());
                    textureW = c.width;
                    textureH = c.height;
                    worldTextureStale = true;
                    SDL_SetTextureBlendMode(worldTexture, SDL_BLENDMODE_BLEND);
                }
                SDL_SetTextureScaleMode(worldTexture,
                                        renderSettings.smoothMap && c.coverageW > c.width * 1.01
                                            ? SDL_ScaleModeLinear
                                            : SDL_ScaleModeNearest);
                if (worldTextureStale) {
                    SDL_UpdateTexture(worldTexture, nullptr, c.pixels.data(), c.width * 4);
                    uploadedBytes += size_t(c.width) * c.height * 4;
                    worldTextureStale = false;
                } else if (refresh) {
                    for (const auto &r : core.world.uploadRegions) {
                        SDL_Rect rect{r.x, r.y, r.w, r.h};
                        SDL_UpdateTexture(worldTexture, &rect,
                                          c.pixels.data() + size_t(r.y) * c.width + r.x,
                                          c.width * 4);
                        uploadedBytes += size_t(r.w) * r.h * 4;
                    }
                }
            }
            SDL_RenderCopy(renderer, volumeTexture ? volumeTexture : worldTexture, nullptr,
                           nullptr);
            if (!renderSettings.projectionCleanScene &&
                ((renderSettings.view == 2 && renderSettings.playerInset) ||
                 (scene.family == 4 && renderSettings.hud != 3)) &&
                updateNativeTexture()) {
                SDL_SetTextureScaleMode(nativeTexture, SDL_ScaleModeNearest);
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
        } else if (updateNativeTexture()) {
            SDL_SetTextureScaleMode(nativeTexture, renderSettings.integerScale
                                                       ? SDL_ScaleModeNearest
                                                       : SDL_ScaleModeLinear);
            int availableH = drawableH, safeW = std::min(drawableW, (int)(drawableH * 16.0 / 9));
            float scale =
                renderSettings.wearActive
                    ? std::min(float(drawableW) / core.nativeW, float(availableH) / core.nativeH)
                : renderSettings.view == 0 ? float(renderSettings.pixelScale())
                : renderSettings.nativeScale
                    ? renderSettings.nativeScale
                    : std::min(float(safeW) / core.nativeW, float(availableH) / core.nativeH);
            if (renderSettings.integerScale && scale >= 1)
                scale = std::floor(scale);
            scale = std::min(
                scale, std::min(float(drawableW) / core.nativeW, float(availableH) / core.nativeH));
            SDL_Rect d{(drawableW - (int)(core.nativeW * scale)) / 2,
                       (availableH - (int)(core.nativeH * scale)) / 2, (int)(core.nativeW * scale),
                       (int)(core.nativeH * scale)};
            nativeBounds = {float(d.x), float(d.y), float(d.x + d.w), float(d.y + d.h)};
#ifndef SCU_QUEST
            if (cachedVolume)
                volumeTexture = volumeGpu.render(renderer, core.world.volume, core.world.volumeMesh,
                                                 renderSettings, d.w, d.h);
#endif
            SDL_RenderCopy(renderer, volumeTexture ? volumeTexture : nativeTexture, nullptr, &d);
        }
        if (renderSettings.scanlines) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, renderSettings.scanlines * 255 / 100);
            if (scanlineWidth != drawableW || scanlineHeight != drawableH) {
                scanlineWidth = drawableW;
                scanlineHeight = drawableH;
                scanlineRects.clear();
                for (int y = 1; y < drawableH; y += 3)
                    scanlineRects.push_back({0, y, drawableW, 1});
            }
            SDL_RenderFillRects(renderer, scanlineRects.data(), int(scanlineRects.size()));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
#ifndef SCU_QUEST
    if (settings.desktopPresentation && !settings.wearActive && core.loaded &&
        updateNativeTexture()) {
        SDL_SetTextureScaleMode(nativeTexture,
                                settings.integerScale ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
        desktopFrame.draw(renderer,
                          volumeTexture ? volumeTexture
                          : expanded    ? worldTexture
                                        : nativeTexture,
                          nativeTexture, core.nativeW, core.nativeH, scene, core.world.canvas,
                          renderSettings, drawableW, drawableH);
    }
#endif
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
#ifdef SCU_QUEST
    if (quest)
        quest->prepareUi();
#endif
#ifdef SCU_MOBILE
    if (mobile)
        mobile->prepareUi();
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
#ifdef SCU_MOBILE
    // Density is handled by the presenter. Size controls in logical dp.
    uiFactor = settings.uiScale == 1   ? .9f
               : settings.uiScale == 2 ? 1.2f
               : settings.uiScale == 3 ? 1.4f
                                       : 1.f;
    if (settings.wearActive)
        uiFactor = settings.uiScale == 2 ? 1.f : settings.uiScale == 3 ? 1.2f : .75f;
#endif
    ImGui::GetStyle() = baseStyle;
    ImGui::GetStyle().ScaleAllSizes(uiFactor);
#ifdef SCU_MOBILE
    ImGui::GetStyle().FramePadding.y = (settings.wearActive ? 4 : 13) * uiFactor;
    ImGui::GetStyle().TouchExtraPadding = {3, 3};
    ImGui::GetStyle().ScrollbarSize = (settings.wearActive ? 12 : 24) * uiFactor;
#endif
    if (settings.highContrast) {
        ImGui::GetStyle().Colors[ImGuiCol_Text] = {1, 1, 1, 1};
        ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = {0.02f, 0.04f, 0.08f, 1};
        ImGui::GetStyle().Colors[ImGuiCol_NavCursor] = {1, 1, 0, 1};
    }
    io.FontGlobalScale = (settings.wearActive ? .65f : .75f) * uiFactor;
    ImGui::NewFrame();
    menuInput(editing);
#ifdef SCU_UI_TESTS
    controls.clear();
    controlRects.clear();
#endif
    volumeOnScreen = volumeTexture != nullptr;
    if (expanded && !renderSettings.projectionCleanScene)
        hud(volumeOnScreen);
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
    renderImGui(ImGui::GetDrawData());
#ifdef SCU_PRIVATE_QA
    if (checksumRequested) {
        checksumRequested = false;
        checksumPixels.resize(size_t(drawableW) * drawableH * 4);
        if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888,
                                 checksumPixels.data(), drawableW * 4) != 0)
            throw std::runtime_error(SDL_GetError());
        sampledChecksum = checksum(checksumPixels);
    }
#endif
    if (capture || screenshotRequested) {
        fs::path p = capturePath.empty()
                         ? library.dataDir / "screenshots" /
                               (core.currentId + "-" + std::to_string(SDL_GetTicks64()) + ".bmp")
                         : fs::u8path(capturePath);
        screenshot(p);
        screenshotRequested = false;
    }
    // Flush drawing separately from display waiting: vsync wait is not GPU/CPU work.
    SDL_RenderFlush(renderer);
    for (auto *texture : retiredGuideTextures)
        SDL_DestroyTexture(texture);
    retiredGuideTextures.clear();
    lastRenderMs =
        double(SDL_GetPerformanceCounter() - workStarted) * 1000 / SDL_GetPerformanceFrequency();
    lastPresentMs = 0;
#if !defined(SCU_QUEST) && !defined(SCU_MOBILE)
    const auto swapStarted = SDL_GetPerformanceCounter();
    SDL_RenderPresent(renderer);
    lastPresentMs =
        double(SDL_GetPerformanceCounter() - swapStarted) * 1000 / SDL_GetPerformanceFrequency();
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
#ifdef SCU_PRIVATE_QA
uint32_t App::presentationChecksum() {
    checksumRequested = true;
    present();
    return sampledChecksum;
}
#endif
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
void App::hud(bool volume) {
    auto &io = ImGui::GetIO();
    float topControls = 0.f;
#ifdef SCU_MOBILE
    if (mobile)
        topControls = mobile->topControlsBottom();
#endif
    auto v = core.world.inspect(core.snapshot, core.game);
    auto &c = core.world.canvas;
    const float sx = float(io.DisplaySize.x / c.coverageW), sy = float(io.DisplaySize.y / c.coverageH);
    voxel::Options depth{settings.voxelMode, float(settings.voxelThickness), float(settings.voxelSpacing),
                         float(settings.voxelRelief)};
    const auto &frame = core.world.volume;
    auto worldPoint = [&](int x, int y, voxel::Plane plane) {
        if (!volume)
            return ImVec2{(x - c.left) * sx, (y - c.top) * sy};
        const int tx = int(std::floor((x - frame.left) * frame.width / frame.coverageWidth));
        const int ty = int(std::floor((y - frame.top) * frame.height / frame.coverageHeight));
        auto color = frame.at(plane, tx, ty);
        if (plane == voxel::Plane::ActorsHigh && !color) {
            plane = voxel::Plane::ActorsLow;
            color = frame.at(plane, tx, ty);
        }
        if (settings.voxelMode == 1 && plane != voxel::Plane::Preview)
            plane = voxel::Plane::TerrainLow;
        const float z = voxel::column(plane, color, depth)[1];
        const auto clip = volumeGpu.project({float(x), float(y), z});
        return ImVec2{(clip.x + 1.f) * .5f * io.DisplaySize.x, (1.f - clip.y) * .5f * io.DisplaySize.y};
    };
    const auto player = worldPoint(v.playerX, v.playerY, voxel::Plane::ActorsHigh);
    const float px = player.x, py = player.y;
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
                auto marker = worldPoint(p.x, p.y, voxel::Plane::Preview);
                float x = marker.x, y = marker.y;
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
    if (settings.ghost) {
        const auto plane = voxel::Plane::TerrainLow;
        const ImVec2 corners[] = {
            worldPoint(v.cameraX, v.cameraY, plane),
            worldPoint(v.cameraX + core.nativeW, v.cameraY, plane),
            worldPoint(v.cameraX + core.nativeW, v.cameraY + core.nativeH, plane),
            worldPoint(v.cameraX, v.cameraY + core.nativeH, plane)};
        draw->AddPolyline(corners, 4, IM_COL32(255, 255, 255, 110), ImDrawFlags_Closed, 1);
    }
    if (settings.hud != 3) {
        float safe = std::min(io.DisplaySize.x, io.DisplaySize.y * 16 / 9),
              x = settings.hud == 2 ? 18 : (io.DisplaySize.x - safe) / 2 + 18, y = 18;
        if (settings.hud == 1) {
            x = std::clamp(px + 30.0f, 0.0f, std::max(0.0f, io.DisplaySize.x - 320));
            y = std::clamp(py - 85.0f, 0.0f, std::max(0.0f, io.DisplaySize.y - 150));
        }
        const auto &style = ImGui::GetStyle();
        const auto padding = style.WindowPadding;
        const float size = settings.hudScale ? float(settings.hudScale)
                                             : std::clamp(io.DisplaySize.y / 720.0f, 0.85f, 3.0f);
        auto panel = [&](float width, float height) {
            const float fullWidth = width + padding.x * 2;
            const ImVec2 first{std::clamp(x, 0.f, std::max(0.f, io.DisplaySize.x - fullWidth)),
                               std::max(y, topControls)};
            const ImVec2 last{first.x + fullWidth, first.y + height + padding.y * 2};
            auto background = style.Colors[ImGuiCol_WindowBg];
            background.w = .76f;
            draw->AddRectFilled(first, last, ImGui::GetColorU32(background), style.WindowRounding);
            if (style.WindowBorderSize > 0)
                draw->AddRect(first, last, ImGui::GetColorU32(ImGuiCol_Border), style.WindowRounding,
                              0, style.WindowBorderSize);
            return ImVec2{first.x + padding.x, first.y + padding.y};
        };
        // The panel dimensions are known before drawing the first frame of a
        // look. An auto-sized ImGui window can hide its initial sizing frame.
        if (v.family == 4 && nativeTexture && uploadedNative == core.imageVersion) {
            const float w = std::min(320.f * size, io.DisplaySize.x - 60);
            const float h = w * 16 / core.nativeW;
            const auto pos = panel(w, h);
            draw->AddImage((ImTextureID)(intptr_t)nativeTexture, pos, {pos.x + w, pos.y + h},
                           {0, 0}, {1, 16.f / core.nativeH});
        } else {
            const auto &r = core.snapshot.ram;
            auto *font = ImGui::GetFont();
            const float fontSize = ImGui::GetFontSize() * std::min(3.f, size) / uiFactor;
            const float advance = fontSize + style.ItemSpacing.y;
            std::array<std::array<char, 96>, 4> rows{};
            int count = 0;
            float width = 0;
            auto line = [&](const char *format, auto... args) {
                auto &text = rows[count++];
                std::snprintf(text.data(), text.size(), format, args...);
                width = std::max(width, font->CalcTextSizeA(fontSize, FLT_MAX, 0, text.data()).x);
            };
            if (v.family == 5) {
                line("%s  %u", tr("rings", language), unsigned(be16(r, 0x0A5A)));
                line("%s  %u", tr("lives", language), unsigned(be16(r, 0x0680)));
            } else {
                const unsigned score = (uint32_t(be16(r, 0xFE26)) << 16) | be16(r, 0xFE28);
                line("%s  %06u", tr("score", language), score * 10);
                line("%s  %02u:%02u", tr("time", language), unsigned(r[0xFE23]), unsigned(r[0xFE24]));
                line("%s  %u", tr("rings", language), unsigned(be16(r, 0xFE20)));
                line("%s  %u", tr("lives", language), unsigned(r[0xFE12]));
            }
            const auto pos = panel(width, fontSize + (count - 1) * advance);
            for (int i = 0; i < count; ++i) {
                const auto color = i == 0 && v.family != 5 ? IM_COL32(242, 217, 89, 255)
                                                          : ImGui::GetColorU32(ImGuiCol_Text);
                draw->AddText(font, fontSize, {pos.x, pos.y + i * advance}, color, rows[i].data());
            }
        }
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
#ifdef SCU_MOBILE
    return tr("mobilenav", language);
#endif
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
        changeSection(direction);
        ImGui::SetNavCursorVisible(true);
    }
    if (core.loaded && ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false))
        toggleMenu();
}
bool App::visibleSection(int n) const {
    return settings.advancedOptions || n == 0 || n == 1 || n == 5 || n == 7 || n == 8;
}
void App::changeSection(int direction) {
    do {
        tab = (tab + direction + sectionCount()) % sectionCount();
    } while (!visibleSection(tab));
    focusPage = true;
}
void App::scrollPage() {
#ifdef SCU_MOBILE
    if (mobile && mobile->scrollDirection() != 0)
        ImGui::SetScrollY(ImGui::GetScrollY() +
                          mobile->scrollDirection() * ImGui::GetIO().DeltaTime * 650);
#endif
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
        (action != 1 && action != 8 && action != 9 && !settings.confirmActions))
        commitAction();
}
Settings App::defaultSettings() const {
    Settings defaults;
#if defined(SCU_MOBILE) || defined(SCU_QUEST)
    defaults.fullscreen = false;
    defaults.vsync = false;
    defaults.quality = 0;
    defaults.voxelDetail = 1;
    defaults.blur = 3;
    defaults.rewindMemory = 0;
#endif
#ifdef SCU_QUEST
    defaults.questMode = 2;
    defaults.questPassthrough = true;
#endif
    defaults.wearActive = settings.wearActive;
    defaults.wearRound = settings.wearRound;
    if (defaults.wearActive) {
        defaults.performanceHeadroom = 30;
        defaults.help = false;
        defaults.mobileHints = false;
        defaults.playerInset = false;
    }
    return defaults;
}
void App::invalidatePresentation() {
    lastImageVersion = ~uint64_t(0);
    worldTextureStale = true;
    core.world.invalidateBackdrop();
    voxelPanX = voxelPanY = 0;
    orbitYawRemainder = orbitPitchRemainder = 0;
    performanceGovernor.reset();
}
void App::settingsApplied(const Settings &previous, bool recenter) {
    language = detectLanguage(settings.language);
    core.updateOptions();
    core.clearAudio();
    invalidatePresentation();
    releaseInput = true;
    if (settings.fullscreen != previous.fullscreen || settings.span != previous.span ||
        settings.display != previous.display)
        applyDisplay();
    if (settings.vsync != previous.vsync)
        SDL_RenderSetVSync(renderer, settings.vsync ? 1 : 0);
#ifdef SCU_QUEST
    if (quest && recenter)
        quest->recenter();
#else
    (void)recenter;
#endif
    dirty = true;
    flushSettings();
}
void App::restoreSettings(int section) {
#ifdef SCU_QUEST
    if (section == 9)
        section = 10;
#endif
    const Settings previous = settings;
    settings.restoreDefaults(defaultSettings(), section);
    settingsApplied(previous, section == 0 || section == 10);
    notify(tr(section ? "sectiondefaultsdone" : "defaultsdone", language));
}
fs::path App::optionsProfile(int slot) const {
    return library.dataDir / "option-profiles" / std::to_string(slot);
}
void App::saveOptionsProfile(int slot) {
    Settings profile = settings;
    profile.lastGame.clear();
    profile.lastDirectory.clear();
    profile.save(optionsProfile(slot));
    notify(tr("optionprofilesaved", language));
}
void App::loadOptionsProfile(int slot) {
    const auto directory = optionsProfile(slot);
    if (!fs::is_regular_file(directory / "settings.ini"))
        throw std::runtime_error(tr("optionprofileempty", language));
    const Settings previous = settings;
    Settings profile = defaultSettings();
    profile.load(directory);
    profile.advancedOptions = settings.advancedOptions;
    settings.restoreDefaults(profile, 0);
    settingsApplied(previous, true);
    notify(tr("optionprofileloaded", language));
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
        } else if (action == 3 || action == 7) {
            restoreSettings(action == 7 ? actionSlot : 0);
        } else if (action == 4) {
            core.loadState(actionSlot);
            notify(tr("loaded", language));
        } else if (action == 5) {
            core.save(actionSlot);
            notify(tr("saved", language));
        } else if (action == 6) {
            play(actionGame);
        } else if (action == 8) {
            core.unload(false);
            const auto count = library.archiveSaves();
            showSection(0);
            notify(std::string(tr("progressresetdone", language)) + " " + std::to_string(count));
        } else if (action == 9)
            saveOptionsProfile(actionSlot);
        else if (action == 10)
            loadOptionsProfile(actionSlot);
    } catch (const std::exception &e) {
        error(e);
    }
    focusPage = true;
}
void App::ui() {
    auto &io = ImGui::GetIO();
    const bool watch = settings.wearActive;
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
    if (!watch) {
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
    }
    if (!watch)
        nextButton(settings.advancedOptions ? "simplemode" : "advancedmode");
    if (button(settings.advancedOptions ? "simplemode" : "advancedmode")) {
        settings.advancedOptions = !settings.advancedOptions;
        if (tab != 0)
            tab = 1;
        focusPage = true;
        dirty = true;
        flushSettings();
    }
    focusMenu = false;
    ImGui::Separator();
    const char *tabs[] = {"library",
                          settings.advancedOptions ? "video" : "lookpresets",
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
#elif defined(SCU_MOBILE)
                          ,
                          "mobiletab"
#endif
                          , "optionprofiles"
    };
#ifdef SCU_MOBILE
    if (watch && sectionCount() > 9)
        tabs[9] = "weartab";
#endif
    if (!visibleSection(tab))
        tab = 1;
    if (small) {
        if (!watch) {
            if (button("prevsection"))
                changeSection(-1);
            ImGui::SameLine();
        }
        ImGui::SetNextItemWidth(watch ? -1 : std::max(80.0f, safe - 100 * uiFactor));
        if (ImGui::BeginCombo("##section", tr(tabs[tab], language))) {
            for (int n = 0; n < sectionCount(); ++n) {
                if (!visibleSection(n))
                    continue;
                if (ImGui::Selectable(tr(tabs[n], language), tab == n)) {
                    tab = n;
                    focusPage = true;
                }
                if (tab == n)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        control("section");
        if (!watch) {
            ImGui::SameLine();
            if (button("nextsection"))
                changeSection(1);
        }
    }
    float footer =
        ImGui::GetFrameHeightWithSpacing() +
        ImGui::CalcTextSize(navigationHint().c_str(), nullptr, false, safe).y +
        ImGui::GetStyle().ItemSpacing.y * 3 +
        (!tiny ? ImGui::CalcTextSize(tr("autosettings", language), nullptr, false, safe).y : 0);
#ifdef SCU_MOBILE
    footer = ImGui::GetFrameHeightWithSpacing() + 12;
    if (watch)
        footer = ImGui::GetFrameHeightWithSpacing() + 3;
#endif
    float contentH = std::max(50.0f, ImGui::GetContentRegionAvail().y - footer);
    if (!small) {
        ImGui::BeginChild("##sections", {230 * uiFactor, contentH}, ImGuiChildFlags_NavFlattened);
        for (int n = 0; n < sectionCount(); ++n) {
            if (!visibleSection(n))
                continue;
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
    bool sideGuide = tab > 0 && tab != 8 && tab != profilesSection() &&
                     safe >= 1000 * uiFactor && !tiny;
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
    if ((tab == 0 && !watch) || tab == 1) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.09f, .34f, .42f, 1));
        if (button("defaults", {-1, ImGui::GetFrameHeight() * 1.18f}))
            requestAction(3);
        ImGui::PopStyleColor();
    }
    if (tab > 0 && tab != 8 && tab != profilesSection() &&
        (tab != 1 || settings.advancedOptions) && button("sectiondefaults", {-1, 0}))
        requestAction(7, tab);
    if (tab == 0)
        libraryUi();
    else if (tab == 1 && !settings.advancedOptions)
        presetUi();
    else if (tab == profilesSection())
        profilesUi();
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
    if (button(watch ? "guidebadge" :
#ifdef SCU_QUEST
                     "questguidebutton"
#else
                     "settingguide"
#endif
               ))
        guideRequested = true;
#ifndef SCU_MOBILE
    ImGui::TextWrapped("%s", navigationHint().c_str());
    if (!tiny)
        ImGui::TextDisabled("%s", tr("autosettings", language));
#endif
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
        if (!actionPopup) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        const char *messages[] = {"",
                                  "quitconfirm",
                                  "resetconfirm",
                                  "defaultsconfirm",
                                  "loadconfirm",
                                  "overwriteconfirm",
                                  "switchconfirm",
                                  "sectiondefaultsconfirm",
                                  "progressresetconfirm",
                                  "optionprofileoverwrite",
                                  "optionprofileapply"};
        ImGui::TextWrapped("%s", tr(messages[std::clamp(actionPopup, 0, 10)], language));
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
    const bool canContinue = !core.loaded && library.find(settings.lastGame);
    if (settings.wearActive) {
        if (canContinue && button("continue", {-1, 0}))
            play(settings.lastGame);
        if (library.entries.empty() && button("import", {-1, 0}))
            beginBrowser(false);
    }
    if (!settings.advancedOptions && button("lookpresets", {-1, 0}))
        showSection(1);
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
    if (!settings.wearActive && canContinue) {
        if (button("continue"))
            play(settings.lastGame);
    }
    if (library.entries.empty()) {
        ImGui::TextWrapped("%s", tr("empty", language));
        ImGui::TextWrapped("%s", tr("legalshort", language));
    }
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
            ImGui::BeginChild("##card", {0, 0},
                              ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened |
                                  ImGuiChildFlags_AutoResizeY,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::TextColored(r ? ImVec4{0.35f, 0.89f, 0.91f, 1} : ImVec4{0.55f, 0.62f, 0.69f, 1},
                               "%02d", ++index);
            ImGui::TextWrapped("%s", g.name);
            ImGui::TextWrapped("%s",
                               r ? (r->derived ? tr("derived", language) : tr("ready", language))
                                 : tr("missing", language));
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
static constexpr const char *looks[] = {"lookstage", "lookoriginal", "lookpanorama",
                                        "lookvoxel", "lookcrt", "lookarwall"};
void App::applyLook(int look) {
    if (look < 0 || look >= int(std::size(looks)))
        return;
    settings.applyLook(look, defaultSettings());
    invalidatePresentation();
#ifdef SCU_QUEST
    if (quest)
        quest->recenter();
#endif
    releaseInput = true;
    dirty = true;
    flushSettings();
    notify(tr(looks[look], language));
}
void App::presetUi() {
    ImGui::TextWrapped("%s", tr("lookpresetintro", language));
#ifndef SCU_QUEST
    const int active = settings.matchingLook(defaultSettings(), false);
#else
    const int active = settings.matchingLook(defaultSettings(), true);
#endif
    ImGui::TextWrapped("%s · %s", tr("activelook", language),
                       tr(active < 0 ? "customprofile" : looks[active], language));
    for (int n = 0; n < int(std::size(looks)); ++n) {
#ifndef SCU_QUEST
        if (n == 5)
            continue;
#endif
        if (settings.wearActive && n == 4)
            continue;
        settingBadge(looks[n]);
        ImGui::SameLine();
        if (button(looks[n], {-1, ImGui::GetFrameHeight() * 1.15f}))
            applyLook(n);
    }
}
void App::profilesUi() {
    ImGui::TextWrapped("%s", tr("optionprofileintro", language));
    constexpr const char *load[] = {"optionload1", "optionload2", "optionload3"};
    constexpr const char *save[] = {"optionsave1", "optionsave2", "optionsave3"};
    for (int n = 0; n < int(std::size(load)); ++n) {
        const int slot = n + 1;
        std::error_code ec;
        bool occupied = fs::is_regular_file(optionsProfile(slot) / "settings.ini", ec);
        ImGui::Text("%s %d  ·  %s", tr("optionprofileslot", language), slot,
                    tr(occupied ? "optionprofileoccupied" : "optionprofileempty", language));
        ImGui::BeginDisabled(!occupied);
        if (button(load[n]))
            requestAction(10, slot);
        ImGui::EndDisabled();
        nextButton(save[n]);
        if (button(save[n])) {
            if (occupied)
                requestAction(9, slot);
            else {
                actionSlot = slot;
                actionPopup = 9;
                commitAction();
            }
        }
        ImGui::Separator();
    }
}
void App::performanceOptions() {
    int profile = settings.performanceHeadroom == 35   ? 0
                  : settings.performanceHeadroom == 25 ? 1
                  : settings.performanceHeadroom == 15 ? 2
                                                       : 3;
    if (choice("qualityprofile", profile,
               {"smoothpriority", "balanced", "detailpriority", "customprofile"}) &&
        profile < 3) {
        settings.performanceHeadroom = std::array<int, 3>{35, 25, 15}[profile];
        settings.autoPerformance = settings.adaptiveEffects = true;
        dirty = true;
    }
    bool changed = tick("autoperformance", settings.autoPerformance);
#ifndef SCU_QUEST
#ifdef SCU_MOBILE
    ImGui::BeginDisabled(settings.mobileRefresh != 0);
#endif
    changed |= slider("performancetarget", settings.performanceTarget, 30, 120, "%d fps");
#ifdef SCU_MOBILE
    ImGui::EndDisabled();
#endif
#else
    ImGui::Text("%s: %.0f Hz", tr("questrefresh", language), quest ? quest->refreshLimit() : 90.);
#endif
    changed |= tick("adaptiveeffects", settings.adaptiveEffects);
    changed |= slider("performanceheadroom", settings.performanceHeadroom, 10, 45, "%d%%");
    if (ImGui::CollapsingHeader(tr("advancedperformance", language))) {
        if (slider("minimumrenderscale", settings.minimumRenderScale, 25, 100, "%d%%")) {
            settings.maximumRenderScale =
                std::max(settings.maximumRenderScale, settings.minimumRenderScale);
            changed = true;
        }
        if (slider("maximumrenderscale", settings.maximumRenderScale, 25, 100, "%d%%")) {
            settings.minimumRenderScale =
                std::min(settings.minimumRenderScale, settings.maximumRenderScale);
            changed = true;
        }
        ImGui::BeginDisabled(settings.voxelMode != 0);
        bool pixelLock = settings.voxelMode ? false : settings.performancePixelLock;
        if (tick("performancepixellock", pixelLock)) {
            settings.performancePixelLock = pixelLock;
            changed = true;
        }
        ImGui::EndDisabled();
        if (settings.voxelMode)
            ImGui::TextWrapped("%s", tr("voxelsamplinghint", language));
        changed |= tick("thermalaware", settings.thermalAware);
    }
    ImGui::TextWrapped("%s", tr("performancehint", language));
    char gpu[48];
    if (performanceGovernor.gpuMs() >= 0)
        std::snprintf(gpu, sizeof gpu, "%.2f ms", performanceGovernor.gpuMs());
    else
        std::snprintf(gpu, sizeof gpu, "%s", tr("unmeasuredgpu", language));
    ImGui::TextWrapped("%s: %d%% / CPU %.2f ms / GPU %s / %d x %d",
                       tr("performancereadout", language), performanceGovernor.scale(),
                       performanceGovernor.cpuMs(), gpu, core.world.canvas.width,
                       core.world.canvas.height);
    ImGui::TextWrapped("%s: %.1f%% / %s: %d%% / %s: %d / %.2f ms",
                       tr("availableheadroom", language), performanceGovernor.availableHeadroom(),
                       tr("stereodetail", language), performanceGovernor.stereoScale(),
                       tr("effectdetail", language), performanceGovernor.effectLevel(),
                       performanceGovernor.usableBudgetMs());
    if (performanceGovernor.coreLimited())
        ImGui::TextWrapped("%s", tr("performancesimbound", language));
    dirty |= changed;
}
void App::voxelOptions() {
    bool changed = choice("voxelmode", settings.voxelMode,
                          {"voxelflat", "voxelthick", "voxellayers", "voxelsculpt"});
    ImGui::TextWrapped("%s", tr("voxelintro", language));
#ifdef SCU_QUEST
    ImGui::TextWrapped("%s", tr("voxelstickxr", language));
#else
    ImGui::TextWrapped("%s", tr("voxelstickhint", language));
#endif
    if (settings.voxelMode) {
        int preset = settings.voxelThickness == 2 && settings.voxelSpacing == 2 &&
                             settings.voxelRelief == 1 && settings.voxelLight == 45
                         ? 0
                     : settings.voxelThickness == 6 && settings.voxelSpacing == 8 &&
                             settings.voxelRelief == 3 && settings.voxelLight == 70
                         ? 1
                     : settings.voxelThickness == 12 && settings.voxelSpacing == 16 &&
                             settings.voxelRelief == 6 && settings.voxelLight == 85
                         ? 2
                         : 3;
        if (choice("voxelpreset", preset,
                   {"voxelsubtle", "voxeldiorama", "voxelbold", "customprofile"}) &&
            preset < 3) {
            settings.voxelThickness = std::array<int, 3>{2, 6, 12}[preset];
            settings.voxelSpacing = std::array<int, 3>{2, 8, 16}[preset];
            settings.voxelRelief = std::array<int, 3>{1, 3, 6}[preset];
            settings.voxelLight = std::array<int, 3>{45, 70, 85}[preset];
            changed = true;
        }
        int detail = std::min(settings.voxelDetail, 2);
        if (choice("voxeldetail", detail, {"voxelcompact", "balanced", "voxelveryfine"})) {
            settings.voxelDetail = detail == 2 ? 3 : detail;
            changed = true;
        }
#ifndef SCU_QUEST
        if (!settings.wearActive && !settings.desktopPresentation && settings.view != 4 &&
            core.currentId != "spinball")
            changed |= choice("voxelframing", settings.voxelFraming,
                              {"voxelfollow", "voxelselectedview"});
#endif
        tick("voxeladjustments", voxelFineTuning);
        if (voxelFineTuning) {
            changed |= slider("voxelthickness", settings.voxelThickness, 1, 32, "%d px");
            if (settings.voxelMode > 1)
                changed |= slider("voxelspacing", settings.voxelSpacing, 0, 64, "%d px");
            if (settings.voxelMode == 3)
                changed |= slider("voxelrelief", settings.voxelRelief, 0, 12, "%d px");
            changed |= slider("voxellight", settings.voxelLight, 0, 100, "%d%%");
            changed |= tick("voxelbackground", settings.voxelBackground);
#ifdef SCU_QUEST
            changed |= tick("voxelcrt", settings.voxelCrt);
#else
            changed |= slider("voxelyaw", settings.voxelYaw, -60, 60, "%d°");
            changed |= slider("voxelpitch", settings.voxelPitch, -60, 60, "%d°");
            if (button("voxelfront")) {
                resetVoxelOrbit();
                settings.voxelYaw = settings.voxelPitch = 0;
                changed = true;
            }
            if (button("voxelangle")) {
                resetVoxelOrbit();
                changed = true;
            }
#endif
        }
#ifdef SCU_QUEST
        ImGui::TextWrapped("%s", tr("voxelvrhint", language));
#else
        if (!voxelRenderer || !std::strcmp(volumeGpu.status(), "voxelunsupported"))
            ImGui::TextWrapped("%s", tr("voxelunsupported", language));
#endif
        if (core.loaded && core.world.volume.valid()) {
            ImGui::TextWrapped(
                "%s",
                tr(core.world.volume.hardwareLayers ? "voxelknown" : "voxelnative", language));
            const auto output = voxelPictureSize();
            if (output[0])
                ImGui::TextWrapped("%s %d × %d · %s %d × %d", tr("voxeltexturesize", language),
                                   core.world.volume.width, core.world.volume.height,
                                   tr("voxeloutputsizing", language), output[0], output[1]);
        }
#if !defined(SCU_MOBILE) && !defined(SCU_QUEST)
        ImGui::BeginDisabled(!core.loaded || !core.world.volume.valid() ||
                             !core.world.volumeMesh.valid());
        if (button("voxelexport")) {
            try {
                auto folder =
                    library.dataDir / "exports" / ("voxels-" + std::to_string(SDL_GetTicks64()));
                unsigned suffix = 0;
                while (fs::exists(folder))
                    folder = library.dataDir / "exports" /
                             ("voxels-" + std::to_string(SDL_GetTicks64()) + "-" +
                              std::to_string(++suffix));
                voxel::exportScene(core.world.volume, core.world.volumeMesh, folder);
                notify(std::string(tr("voxelexported", language)) + ": " + folder.u8string());
            } catch (const std::exception &error) {
                notify(std::string(tr("voxelexportfailed", language)) + ": " + error.what());
            }
        }
        ImGui::EndDisabled();
#endif
    }
    dirty |= changed;
}
void App::optionsUi(int section) {
    bool changed = false;
    auto check = [&](const char *key, bool &value) { changed |= tick(key, value); };
    auto slide = [&](const char *key, int &v, int a, int b, const char *fmt = "%d") {
        changed |= slider(key, v, a, b, fmt);
    };
    switch (section) {
    case 1: {
        if (ImGui::CollapsingHeader(tr("voxelgroup", language),
                                    settings.voxelMode ? ImGuiTreeNodeFlags_DefaultOpen : 0))
            voxelOptions();
        int lang = settings.language + 1;
        if (choice("language", lang, {"auto", "English", "Français", "Español", "日本語"})) {
            settings.language = lang - 1;
            language = detectLanguage(settings.language);
            changed = true;
        }
        if (ImGui::CollapsingHeader(tr("quickviewgroup", language),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            int preset = settings.quality;
            if (choice("preset", preset, {"low", "balanced", "ultra"})) {
                settings.preset(preset);
                performanceGovernor.reset();
                changed = true;
            }
            if (settings.wearActive) {
                slide("wearzoom", settings.wearZoom, 0, 1000, "%d / 1000");
                check("wearfisheye", settings.wearFisheye);
            } else {
                int view = settings.view == 4 ? 3 : settings.view;
                if (choice("view", view, {"pixel", "panorama", "overview", "classic"})) {
                    selectView(view == 3 ? 4 : view);
                    changed = true;
                }
                ImGui::TextWrapped("%s", tr("overviewhelp", language));
                if (slider("zoom", settings.zoom, 1, 8, "%d×")) {
                    selectView(0);
                    changed = true;
                }
                if (choice("zoomout", settings.zoomOut, {"1×", "2×", "4×", "8×"})) {
                    selectView(0);
                    changed = true;
                }
                ImGui::Text("%s: %.3g×", tr("effectivescale", language), settings.pixelScale());
                if (button("pixelreset")) {
                    selectView(0);
                    settings.zoom = 1;
                    settings.zoomOut = 0;
                    changed = true;
                }
                if (slider("panoramaheight", settings.panoramaHeight, 224, 4096, "%d px")) {
                    selectView(1);
                    changed = true;
                }
            }
            if (settings.wearActive || settings.view == 2) {
                bool silhouette = settings.overviewStyle == 2;
                if (tick("overviewstyle", silhouette)) {
                    settings.overviewStyle = silhouette ? 2 : 0;
                    changed = true;
                }
            }
            check("mapextension", settings.mapExtension);
            ImGui::BeginDisabled(settings.voxelMode != 0);
            bool smooth = settings.voxelMode ? false : settings.smoothMap;
            if (tick("smoothmap", smooth)) {
                settings.smoothMap = smooth;
                changed = true;
            }
            ImGui::EndDisabled();
            if (settings.voxelMode)
                ImGui::TextWrapped("%s", tr("voxelsamplinghint", language));
            if (core.loaded) {
                auto scene = core.world.inspect(core.snapshot, core.game);
                ImGui::TextWrapped("%s", tr(scene.ready ? (scene.family >= 4 ? "atlascapability"
                                                                             : "platformcapability")
                                                        : "nativecapability",
                                            language));
            }
            if (!settings.wearActive)
                ImGui::TextWrapped("%s", tr("pixelhelp", language));
        }
        if (ImGui::CollapsingHeader(tr("quickdisplaygroup", language),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
#if !defined(SCU_QUEST) && !defined(SCU_MOBILE)
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
#if !defined(SCU_QUEST) && !defined(SCU_MOBILE)
            check("vsync", settings.vsync);
#endif
            check("nearest", settings.integerScale);
#ifndef SCU_QUEST
            if (!settings.wearActive) {
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
                        slide("questsurroundbrightness", settings.questSurroundBrightness, 0, 100,
                              "%d%%");
                    }
                }
            }
#endif
            if (!settings.wearActive)
                slide("nativescale", settings.nativeScale, 0, 8);
            slide("workers", settings.workers, 0, 16);
#if !defined(SCU_QUEST) && !defined(SCU_MOBILE)
            bool soft = settings.renderer != 0;
            if (tick("soft", soft)) {
                settings.renderer = soft ? 1 : 0;
                changed = true;
            }
#endif
            slide("scanlines", settings.scanlines, 0, 60);
        }
        if (ImGui::CollapsingHeader(tr("quickqualitygroup", language),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            performanceOptions();
            check("performance", settings.performance);
        }
        break;
    }
    case 2:
        if (button("showstage")) {
            if (settings.wearActive)
                settings.wearZoom = 0;
            selectView(2);
            settings.locator = true;
            changed = true;
        }
        if (button("focusplayer")) {
            if (settings.wearActive)
                settings.wearZoom = 1000;
            selectView(0);
            settings.voxelFraming = 0;
            settings.camera = 1;
            changed = true;
        }
        check("playerinset", settings.playerInset);
        check("enemypreview", settings.enemyPreview);
        ImGui::BeginDisabled(!settings.enemyPreview);
        check("bosspreview", settings.bossPreview);
        check("scriptpreview", settings.scriptPreview);
        check("objectpreview", settings.objectPreview);
        check("objectorigins", settings.objectOrigins);
        check("frozenpreviews", settings.frozenPreviews);
        changed |= choice("enemystyle", settings.enemyStyle,
                          {"enemyspritesmarkers", "enemysprites", "enemymarkers"});
        slide("enemyopacity", settings.enemyOpacity, 10, 80, "%d%%");
        check("spawntint", settings.spawnTint);
        check("spawnlabels", settings.spawnLabels);
        check("fitspawns", settings.fitSpawns);
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", tr("enemyhelp", language));
        if (!settings.wearActive) {
            if (choice("cameramode", settings.camera,
                       {"bounded", "centred", "fixed", "free", "level"})) {
                if (settings.camera == 2 || settings.camera == 3) {
                    const auto scene = core.world.inspect(core.snapshot, core.game);
                    settings.left = cachedExpanded ? core.world.canvas.left : scene.cameraX;
                    settings.top = cachedExpanded ? core.world.canvas.top : scene.cameraY;
                }
                selectView(settings.camera == 4 ? 2 : 0);
                changed = true;
            }
            ImGui::TextWrapped("%s", tr("overviewcamera", language));
            ImGui::BeginDisabled(settings.view == 2);
            if (button("freeze")) {
                freezeView();
                changed = true;
            }
            ImGui::EndDisabled();
            if (button("follow")) {
                settings.camera = 0;
                selectView(0);
                changed = true;
            }
            slide("panspeed", settings.panSpeed, 1, 32);
        }
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
        changed |= choice(
            "backgroundstyle", settings.background,
            {"blurred", "sharpbg", "parallaxbg", "gradient", "dark", "black", "transparentbg"});
        if (settings.background == 0)
            slide("blur", settings.blur, 0, 16);
        if (settings.background < 5)
            slide("brightness", settings.brightness, 10, 100, "%d%%");
        if (settings.background < 4)
            check("parallax", settings.parallax);
        check("motion", settings.reduceMotion);
        ImGui::TextWrapped("%s", tr("bghelp", language));
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
        changed |= tick("spinballtriggers", settings.spinballTriggers);
        changed |=
            choice("jumpbuttons", settings.jumpLayout, {"jumpclassic", "jumpabc", "jumpbac"});
        if (core.currentId == "spinball" && settings.spinballTriggers) {
            const char *binding = "L1 / LB";
#ifdef SCU_QUEST
            if (settings.questTouch)
                binding = "X";
#endif
            ImGui::TextWrapped("%s: %s", tr("rewindbutton", language), binding);
        } else {
#ifdef SCU_QUEST
            if (settings.questTouch)
                ImGui::TextWrapped("%s: %s", tr("rewindbutton", language),
                                   tr("questtrigger", language));
            else
#endif
                changed |= choice("rewindbutton", settings.rewindButton,
                                  {"L2 / LT", "L1 / LB", "R2 / RT"});
        }
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
        ImGui::Text("%s %d", tr("slotnumber", language), settings.slot);
        ImGui::BeginDisabled(!core.loaded);
        auto path = library.dataDir / "saves" /
                    (core.saveKey + "-" + std::to_string(settings.slot) + ".state");
        std::error_code ec;
        if (button("save")) {
            if (fs::exists(path, ec))
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
        nextButton("load");
        ImGui::BeginDisabled(!fs::exists(path, ec));
        if (button("load"))
            requestAction(4, settings.slot);
        ImGui::EndDisabled();
        nextButton("reset");
        if (button("reset"))
            requestAction(2);
        nextButton("capture");
        if (button("capture"))
            screenshotRequested = true;
        ImGui::EndDisabled();
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
        ImGui::TextWrapped("%s", tr("savehelp", language));
        if (button("progressreset", {-1, 0}))
            requestAction(8);
        break;
    }
#ifdef SCU_QUEST
    case 9:
        quest->options();
        break;
#elif defined(SCU_MOBILE)
    case 9:
        if (mobile)
            mobile->options();
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
