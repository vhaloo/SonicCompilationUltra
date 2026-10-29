#pragma once
#ifdef SCU_QUEST
#include <SDL.h>
#include <openxr/openxr.h>
#include <memory>
#include <string>
#include "world.h"
namespace scu {
// The room scene has its own stereo projection. The options panel stays a sharp
// compositor quad, independent of the size of the game or its furniture.
struct XrStageContent {
    SDL_Texture *expanded = nullptr;
    const std::vector<uint32_t> *native = nullptr;
    const std::array<uint32_t, 320 * 224> *backdrop = nullptr;
    int nativeWidth = 320, nativeHeight = 224;
    uint64_t version = 0;
    bool loaded = false;
    Scene scene;
    int left = 0, top = 0;
    double coverageWidth = 320, coverageHeight = 224;
};
class XrStageRenderer {
    struct Impl;
    std::unique_ptr<Impl> impl;

  public:
    XrStageRenderer();
    ~XrStageRenderer();
    void initialise(XrInstance instance, XrSystemId system, XrSession session);
    const XrCompositionLayerProjection *render(XrSpace space, XrTime time, XrPosef origin,
                                               const Settings &settings, int mode,
                                               const XrStageContent &content, bool passthrough,
                                               const std::string &capture = {});
    uint64_t frames() const;
};
} // namespace scu
#endif
