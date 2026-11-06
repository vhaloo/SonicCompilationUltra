#pragma once
#ifdef SCU_QUEST
#include <openxr/openxr.h>
#include <array>
#include <algorithm>

namespace scu {
// Optional OpenXR profiles. Failure disables this optimization, never startup.
// Profiles are cached; no eye buffer or profile is reallocated while adapting.
class XrFoveation {
    XrSession session = XR_NULL_HANDLE;
    PFN_xrCreateFoveationProfileFB create = nullptr;
    PFN_xrDestroyFoveationProfileFB destroy = nullptr;
    PFN_xrUpdateSwapchainFB update = nullptr;
    std::array<XrFoveationProfileFB, 4> profiles{};

  public:
    bool available = false;
    void initialise(XrInstance instance, XrSession value, bool supported) {
        session = value;
        if (!supported)
            return;
        auto get = [&](const char *name, auto &target) {
            return XR_SUCCEEDED(xrGetInstanceProcAddr(
                       instance, name, reinterpret_cast<PFN_xrVoidFunction *>(&target))) &&
                   target;
        };
        available = get("xrCreateFoveationProfileFB", create) &&
                    get("xrDestroyFoveationProfileFB", destroy) &&
                    get("xrUpdateSwapchainFB", update);
    }
    bool apply(XrSwapchain chain, int level) {
        if (!available)
            return false;
        level = std::clamp(level, 0, 3);
        auto &profile = profiles[level];
        if (!profile) {
            XrFoveationLevelProfileCreateInfoFB detail{
                XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB};
            detail.level = static_cast<XrFoveationLevelFB>(level);
            detail.dynamic = XR_FOVEATION_DYNAMIC_DISABLED_FB;
            XrFoveationProfileCreateInfoFB info{XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB};
            if (level)
                info.next = &detail;
            if (XR_FAILED(create(session, &info, &profile))) {
                available = false;
                return false;
            }
        }
        XrSwapchainStateFoveationFB state{XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB};
        state.profile = profile;
        if (XR_FAILED(
                update(chain, reinterpret_cast<const XrSwapchainStateBaseHeaderFB *>(&state)))) {
            available = false;
            return false;
        }
        return true;
    }
    ~XrFoveation() {
        if (destroy)
            for (auto p : profiles)
                if (p)
                    destroy(p);
    }
};
} // namespace scu
#endif
