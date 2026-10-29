#pragma once
#ifdef SCU_QUEST
#include <memory>
namespace scu {
class App;
// Android-only presenter. Simulation and the original SDL menu remain in App.
class QuestPresentation {
    struct Impl;
    std::unique_ptr<Impl> impl;

  public:
    explicit QuestPresentation(App &app);
    ~QuestPresentation();
    void initialise();
    bool beginFrame();
    void endFrame();
    void prepareUi();
    void drawReticle();
    void options();
    void recenter();
    void chooseRom();
    void logicalSize(int &width, int &height) const;
};
} // namespace scu
#endif
