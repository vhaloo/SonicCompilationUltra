#include "desktop_frame.h"
#include <algorithm>
#include <cmath>
namespace scu {
namespace {
constexpr SDL_Color ink{8, 15, 27, 255}, body{12, 34, 69, 255}, metal{83, 105, 131, 255},
    cyan{60, 208, 224, 255};
void quad(SDL_Renderer *r, SDL_Texture *t, SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, SDL_FPoint d,
          SDL_Color color = {255, 255, 255, 255}) {
    SDL_Vertex v[4]{{a, color, {0, 0}}, {b, color, {1, 0}}, {c, color, {1, 1}}, {d, color, {0, 1}}};
    constexpr int indices[]{0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(r, t, v, 4, indices, 6);
}
void rect(SDL_Renderer *r, float x, float y, float w, float h, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect dst{x, y, w, h};
    SDL_RenderFillRectF(r, &dst);
}
void image(SDL_Renderer *r, SDL_Texture *t, float x, float y, float w, float h) {
    SDL_FRect dst{x, y, w, h};
    if (t)
        SDL_RenderCopyF(r, t, nullptr, &dst);
}
void effectRect(std::vector<SDL_Vertex> &v, float x, float y, float w, float h, SDL_Color color) {
    SDL_Vertex q[4]{{{x, y}, color, {}},
                    {{x + w, y}, color, {}},
                    {{x + w, y + h}, color, {}},
                    {{x, y + h}, color, {}}};
    for (int i : {0, 1, 2, 0, 2, 3})
        v.push_back(q[i]);
}
} // namespace
void DesktopFrame::draw(SDL_Renderer *r, SDL_Texture *level, SDL_Texture *native, int nw, int nh,
                        const Scene &scene, const Canvas &canvas, const Settings &s, int width,
                        int height) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    float W = float(width), H = float(height), scale = s.desktopFrameScale * .01f;
    rect(r, 0, 0, W, H, {5, 11, 21, 255});
    for (int i = 0; i < 32; ++i)
        rect(r, 0, H * i / 32, W, H / 32 + 1,
             {uint8_t(7 + i / 5), uint8_t(15 + i / 4), uint8_t(29 + i / 3), 255});
    if (s.desktopPresentation == 1) {
        float ratio =
            scene.ready ? float(canvas.coverageW / canvas.coverageH) : float(nw) / std::max(1, nh);
        float h = H * .64f * scale, w = std::min(W * .76f, h * ratio * 1.25f), cx = W * .5f,
              top = H * .22f, bottom = top + h;
        float taper = std::clamp(1.f - s.desktopCabinetAngle * .005f, .65f, .95f), front = w / 2,
              back = front * taper;
        float trim = std::max(5.f, H * .012f), depth = H * .07f;
        SDL_FPoint a{cx - back, top}, b{cx + back, top}, c{cx + front, bottom},
            d{cx - front, bottom};
        rect(r, cx - front * .84f, bottom + depth, trim * 1.2f, std::max(0.f, H - bottom - depth),
             metal);
        rect(r, cx + front * .78f, bottom + depth, trim * 1.2f, std::max(0.f, H - bottom - depth),
             metal);
        quad(r, nullptr, {a.x - trim, a.y}, {b.x + trim, b.y}, {c.x + trim, c.y + depth},
             {d.x - trim, d.y + depth}, body);
        quad(r, level, a, b, c, d);
        quad(r, nullptr, {a.x - trim, a.y}, {a.x, a.y}, {d.x, d.y}, {d.x - trim, d.y}, metal);
        quad(r, nullptr, {b.x, b.y}, {b.x + trim, b.y}, {c.x + trim, c.y}, {c.x, c.y}, metal);
        quad(r, nullptr, {a.x, a.y}, {a.x + trim * .25f, a.y}, {d.x + trim * .25f, d.y}, {d.x, d.y},
             cyan);
        quad(r, nullptr, {b.x - trim * .25f, b.y}, {b.x, b.y}, {c.x, c.y}, {c.x - trim * .25f, c.y},
             cyan);
        rect(r, cx - front - trim, bottom, w + trim * 2, depth, body);
        rect(r, cx - front - trim, bottom + depth * .8f, w + trim * 2, trim * .6f, metal);
        rect(r, cx - front - trim * 1.6f, bottom - H * .08f, trim * .8f, H * .04f,
             {246, 172, 49, 255});
        rect(r, cx + front + trim * .8f, bottom - H * .08f, trim * .8f, H * .04f,
             {246, 172, 49, 255});
        float bh = std::min(H * .13f, w * .72f * nh / std::max(1, nw)),
              bw = bh * nw / std::max(1, nh);
        rect(r, cx - back - trim, top - H * .17f, back * 2 + trim * 2, H * .17f, body);
        rect(r, cx - back, top - trim, back * 2, trim * .3f, cyan);
        image(r, native, cx - bw / 2, top - H * .16f, bw, bh);
    } else {
        float w = std::min(W * .68f, H * .73f * nw / std::max(1, nh)) * scale,
              h = w * nh / std::max(1, nw);
        float x = (W - w) / 2, y = (H - h) / 2, unit = w / std::max(1, nw);
        if (s.desktopSurround && scene.ready && level) {
            Uint8 oldR, oldG, oldB;
            SDL_GetTextureColorMod(level, &oldR, &oldG, &oldB);
            Uint8 dim = Uint8(s.questSurroundBrightness * 255 / 100);
            SDL_SetTextureColorMod(level, dim, dim, dim);
            image(r, level, x + (canvas.left - scene.cameraX) * unit,
                  y + (canvas.top - scene.cameraY) * unit, float(canvas.coverageW) * unit,
                  float(canvas.coverageH) * unit);
            SDL_SetTextureColorMod(level, oldR, oldG, oldB);
        }
        float frame = std::max(8.f, h * .065f);
        for (int i = 6; i > 0; --i)
            rect(r, x - frame - i * 3, y - frame - i * 3, w + 2 * frame + i * 6,
                 h + 2 * frame + i * 6, {18, 79, 102, Uint8(10 + i)});
        rect(r, x - frame, y - frame, w + frame * 2, h + frame * 2, metal);
        rect(r, x - frame * .9f, y - frame * .9f, w + frame * 1.8f, h + frame * 1.8f, body);
        rect(r, x - frame * .35f, y - frame * .35f, w + frame * .7f, h + frame * .7f, ink);
        image(r, native, x, y, w, h);
        auto key = std::array<int, 7>{
            {int(x), int(y), int(w), int(h), nh, s.desktopCrtStrength, int(s.desktopCrtMask)}};
        if (key != effectKey) {
            effectKey = key;
            effects.clear();
            float step = std::max(2.f, h / std::max(1, nh));
            for (float row = y + step * .65f; row < y + h; row += step)
                effectRect(effects, x, row, w, std::max(.7f, step * .26f),
                           {0, 0, 0, Uint8(s.desktopCrtStrength)});
            if (s.desktopCrtMask && w > nw * 2)
                for (float col = x; col < x + w; col += 3)
                    effectRect(effects, col, y, 1, h,
                               {20, 70, 95, Uint8(s.desktopCrtStrength / 7)});
        }
        if (!effects.empty())
            SDL_RenderGeometry(r, nullptr, effects.data(), int(effects.size()), nullptr, 0);
        rect(r, x + w - frame * .45f, y + h + frame * .35f, frame * .2f, frame * .13f, cyan);
        rect(r, x + w * .25f, y + h + frame * 1.1f, w * .5f, frame * .5f, ink);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
} // namespace scu
