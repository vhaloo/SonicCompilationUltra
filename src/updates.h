#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

namespace scu {
class Updates {
  public:
    void configure(const std::filesystem::path &base, const std::filesystem::path &data);
    void check();
    void install();
    void poll();
    void later();
    ~Updates();
    std::string state = "idle", version, detail;
    int progress = 0;
    bool configured = false;

  private:
    std::filesystem::path root, directory, session, helper;
    uint64_t nextPoll = 0, deadline = 0;
    intptr_t process = 0;
    bool handedOff = false;
    void acceptStatus(const std::string &value);
    bool launch(bool apply);
};
} // namespace scu
