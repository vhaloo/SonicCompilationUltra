#include "wear_display.h"
#include <algorithm>
#include <cmath>

namespace scu::wear {
View camera(double zoom, int stageW, int stageH, int nw, int nh, int cx, int cy, double aspect) {
    zoom = std::isfinite(zoom) ? std::clamp(zoom, 0., 1.) : 0.;
    nw = std::clamp(nw, 1, 4096);
    nh = std::clamp(nh, 1, 4096);
    stageW = std::max(nw, std::clamp(stageW, 1, 32768));
    stageH = std::max(nh, std::clamp(stageH, 1, 8192));
    aspect = std::isfinite(aspect) ? std::clamp(aspect, .1, 10.) : double(nw) / nh;
    double fullW = std::max(double(stageW), stageH * aspect), fullH = fullW / aspect;
    View view;
    view.width = std::max(double(nw), fullW * std::pow(nw / fullW, zoom));
    view.height = std::max(double(nh), fullH * std::pow(nh / fullH, zoom));
    double t = std::min(1., zoom * 4), follow = t * t * (3 - 2 * t);
    double fullLeft = (stageW - view.width) * .5, fullTop = (stageH - view.height) * .5;
    double nativeLeft = cx - (view.width - nw) * .5, nativeTop = cy - (view.height - nh) * .5;
    view.left = fullLeft + (nativeLeft - fullLeft) * follow;
    view.top = fullTop + (nativeTop - fullTop) * follow;
    // Never lose the live console viewport while approaching a player near a
    // stage edge. At closer zoom it is centered, even if stage margins are blank.
    view.left = std::clamp(view.left, cx + nw - view.width, double(cx));
    view.top = std::clamp(view.top, cy + nh - view.height, double(cy));
    if (zoom >= 1.) {
        view.left = cx;
        view.top = cy;
    }
    return view;
}
double crownZoom(double current, double delta, int speed, bool reverse) {
    current = std::isfinite(current) ? std::clamp(current, 0., 1000.) : 0.;
    if (!std::isfinite(delta))
        return current;
    double step = std::array<double, 3>{22, 40, 65}[std::clamp(speed, 0, 2)];
    return std::clamp(current + std::clamp(reverse ? -delta : delta, -30., 30.) * step, 0., 1000.);
}
LensMesh::LensMesh() {
    size_t at = 0;
    for (int y = 0; y < segments; ++y)
        for (int x = 0; x < segments; ++x) {
            int a = y * side + x, b = a + 1, c = a + side, d = c + 1;
            for (int i : {a, c, b, b, c, d})
                indices[at++] = i;
        }
}
Vertex LensMesh::project(float u, float v) const {
    u = std::clamp(u, 0.f, 1.f);
    v = std::clamp(v, 0.f, 1.f);
    float w = float(width), h = float(height), scale = std::min(w / aspect, h);
    float rw = scale * aspect, rh = scale;
    const float radius = std::min(w, h) * .48f;
    if (round) {
        float fit = std::min(1.f, radius / std::hypot(rw * .5f, rh * .5f));
        rw *= fit;
        rh *= fit;
    }
    float bx = w * .5f + (u - .5f) * rw, by = h * .5f + (v - .5f) * rh;
    if (!round || amount <= 0)
        return {std::clamp(bx, 0.f, w), std::clamp(by, 0.f, h), u, v};
    double x = 2 * u - 1, y = 2 * v - 1;
    double dx = x * std::sqrt(std::max(0., 1 - y * y * .5));
    double dy = y * std::sqrt(std::max(0., 1 - x * x * .5));
    double radial = std::hypot(dx, dy), gain = (1 + strength) / (1 + strength * radial);
    float lx = w * .5f + float(radius * dx * gain), ly = h * .5f + float(radius * dy * gain);
    return {bx + (lx - bx) * amount, by + (ly - by) * amount, u, v};
}
bool LensMesh::update(int w, int h, float sourceAspect, bool circular, float blend,
                      float magnification) {
    w = std::clamp(w, 1, 16384);
    h = std::clamp(h, 1, 16384);
    sourceAspect = std::isfinite(sourceAspect) ? std::clamp(sourceAspect, .1f, 10.f) : 320.f / 224;
    blend = std::isfinite(blend) ? std::clamp(blend, 0.f, 1.f) : 0;
    magnification = std::isfinite(magnification) ? std::clamp(magnification, 0.f, 1.25f) : 0;
    if (revision && width == w && height == h && aspect == sourceAspect && round == circular &&
        amount == blend && strength == magnification)
        return false;
    width = w;
    height = h;
    aspect = sourceAspect;
    round = circular;
    amount = blend;
    strength = magnification;
    for (int y = 0; y < side; ++y)
        for (int x = 0; x < side; ++x)
            vertices[y * side + x] = project(x / float(segments), y / float(segments));
    ++revision;
    return true;
}
} // namespace scu::wear
