#include "updates.h"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>
#if defined(__ANDROID__)
#include <jni.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

namespace scu {
namespace fs = std::filesystem;
#ifdef __ANDROID__
static std::string mobileUpdate(const char *action) {
    auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    auto activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity)
        return "error install";
    auto type = env->GetObjectClass(activity);
    auto method = env->GetMethodID(type, "scuUpdate", "(Ljava/lang/String;)Ljava/lang/String;");
    auto arg = env->NewStringUTF(action);
    auto result =
        method ? static_cast<jstring>(env->CallObjectMethod(activity, method, arg)) : nullptr;
    std::string value = "error install";
    if (env->ExceptionCheck())
        env->ExceptionClear();
    else if (result) {
        const char *text = env->GetStringUTFChars(result, nullptr);
        if (text) {
            value = text;
            env->ReleaseStringUTFChars(result, text);
        }
    }
    if (result)
        env->DeleteLocalRef(result);
    env->DeleteLocalRef(arg);
    env->DeleteLocalRef(type);
    env->DeleteLocalRef(activity);
    return value;
}
#else
#ifdef _WIN32
static std::wstring quoted(const std::wstring &arg) {
    std::wstring result = L"\"";
    size_t slash = 0;
    for (wchar_t ch : arg) {
        if (ch == L'\\') {
            ++slash;
            continue;
        }
        result.append(ch == L'"' ? slash * 2 + 1 : slash, L'\\');
        result += ch;
        slash = 0;
    }
    result.append(slash * 2, L'\\');
    return result + L'"';
}
#endif
static intptr_t spawn(const fs::path &exe, const std::vector<fs::path> &args) {
#ifdef _WIN32
    std::wstring command = quoted(exe.native());
    for (auto &a : args)
        command += L" " + quoted(a.native());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION info{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, nullptr, &startup, &info))
        return 0;
    CloseHandle(info.hThread);
    return reinterpret_cast<intptr_t>(info.hProcess);
#else
    std::vector<char *> argv{const_cast<char *>(exe.c_str())};
    for (auto &a : args)
        argv.push_back(const_cast<char *>(a.c_str()));
    argv.push_back(nullptr);
    pid_t pid = 0;
    return posix_spawn(&pid, exe.c_str(), nullptr, nullptr, argv.data(), environ) == 0 ? pid : 0;
#endif
}
static bool finished(intptr_t &process) {
    if (!process)
        return true;
#ifdef _WIN32
    auto handle = reinterpret_cast<HANDLE>(process);
    if (WaitForSingleObject(handle, 0) != WAIT_OBJECT_0)
        return false;
    CloseHandle(handle);
#else
    int status;
    if (waitpid(static_cast<pid_t>(process), &status, WNOHANG) == 0)
        return false;
#endif
    process = 0;
    return true;
}
static fs::path helperName() {
#ifdef _WIN32
    return "scu-update.exe";
#else
    return "scu-update";
#endif
}
static void cleanup(const fs::path &path) {
    std::error_code ec;
    for (const auto &name : std::array<fs::path, 4>{"status", "closed", "stop", helperName()})
        fs::remove(path / name, ec);
    fs::remove(path, ec);
}
#endif

void Updates::configure(const fs::path &base, const fs::path &data) {
    configured = true;
#ifndef __ANDROID__
    try {
        directory = fs::absolute(data);
        root = fs::absolute(base).lexically_normal();
#ifdef __APPLE__
        if (root.filename().empty())
            root = root.parent_path();
        if (root.filename() == "Resources")
            root = root.parent_path().parent_path();
        helper = root / "Contents" / "MacOS" / helperName();
#else
        helper = root / helperName();
#endif
        // Sessions live in the user's private data area. Only recognized files in
        // stale updater sessions are removed, including a prior Windows helper.
        const auto sessions = directory / "update-sessions";
        fs::create_directories(sessions);
        for (auto &entry : fs::directory_iterator(sessions)) {
            const auto name = entry.path().filename().string();
            if (entry.is_symlink() || !entry.is_directory() || name.size() != 32 ||
                name.find_first_not_of("0123456789abcdef") != std::string::npos)
                continue;
            if (fs::file_time_type::clock::now() - entry.last_write_time() > std::chrono::hours(24))
                cleanup(entry.path());
        }
        std::random_device random;
        const char digits[] = "0123456789abcdef";
        std::string token(32, '0');
        for (char &c : token)
            c = digits[random() % 16];
        session = sessions / token;
        if (!fs::create_directory(session))
            throw std::runtime_error("Update session already exists");
        if (fs::exists(directory / "update-install-error")) {
            state = "error";
            detail = "restored";
            fs::remove(directory / "update-install-error");
        }
    } catch (const std::exception &) {
        state = "error";
        detail = "permission";
    }
#else
    (void)base;
    (void)data;
#endif
}

bool Updates::launch(bool apply) {
#ifdef __ANDROID__
    acceptStatus(mobileUpdate(apply ? "install" : "check"));
    return state != "error";
#else
    try {
        if (!finished(process) || session.empty() || !fs::is_regular_file(helper))
            return false;
        std::error_code ec;
        fs::remove(session / "status", ec);
        fs::path exe = helper;
        std::vector<fs::path> args{apply ? "install" : "check", SCU_VERSION, session, root};
        if (apply) {
            exe = session / helperName();
            fs::copy_file(helper, exe, fs::copy_options::overwrite_existing);
            args.emplace_back(version);
            args.push_back(directory);
            args.push_back(fs::current_path());
#ifdef _WIN32
            args.emplace_back(std::to_string(GetCurrentProcessId()));
#else
            args.emplace_back(std::to_string(getpid()));
#endif
        }
        process = spawn(exe, args);
        return process != 0;
    } catch (const std::exception &) {
        return false;
    }
#endif
}
void Updates::check() {
    if (!configured || state == "checking" || state == "downloading" || state == "verifying" ||
        state == "system")
        return;
    state = "checking";
    detail.clear();
    version.clear();
    nextPoll = 0;
    deadline = SDL_GetTicks64() + 25000;
    if (!launch(false)) {
        state = "error";
        detail = "check";
    }
}
void Updates::install() {
    if (state != "available")
        return;
    state = "downloading";
    progress = 0;
    nextPoll = 0;
    deadline = SDL_GetTicks64() + 330000;
    if (!launch(true)) {
        state = "error";
        detail = "install";
    }
}
void Updates::acceptStatus(const std::string &value) {
    std::istringstream in(value);
    std::string phase;
    in >> phase;
    if (phase == "available") {
        std::string label;
        in >> label;
        if (label.empty() || label.size() > 32)
            return;
        version = label;
    } else if (phase == "downloading") {
        int n = 0;
        in >> n;
        progress = std::clamp(n, 0, 100);
    } else if (phase == "error") {
        in >> detail;
    } else if (phase != "checking" && phase != "current" && phase != "ready" &&
               phase != "verifying" && phase != "system" && phase != "later")
        return;
    state = phase;
}
void Updates::poll() {
    const auto now = SDL_GetTicks64();
    if (now < nextPoll)
        return;
    nextPoll = now + 250;
#ifndef __ANDROID__
    const bool done = finished(process);
#endif
    if (state != "checking" && state != "downloading" && state != "verifying" && state != "system")
        return;
#ifdef __ANDROID__
    acceptStatus(mobileUpdate("status"));
#else
    std::ifstream f(session / "status");
    std::string line;
    if (std::getline(f, line) && (done || line.rfind("available ", 0) != 0))
        acceptStatus(line);
    if (done && (state == "checking" || state == "downloading" || state == "verifying")) {
        detail = state == "checking" ? "check" : "install";
        state = "error";
    }
#endif
    if (state != "system" &&
        (state == "checking" || state == "downloading" || state == "verifying") &&
        now >= deadline) {
        detail = state == "checking" ? "check" : "install";
        state = "error";
    }
    if (state == "ready")
        handedOff = true;
}
void Updates::later() {
    state = "later";
    detail.clear();
}
Updates::~Updates() {
#ifndef __ANDROID__
    if (session.empty())
        return;
    if (handedOff)
        std::ofstream(session / "closed") << "1";
    else if (finished(process))
        cleanup(session);
    else
        std::ofstream(session / "stop") << "1";
#ifdef _WIN32
    if (process)
        CloseHandle(reinterpret_cast<HANDLE>(process));
#endif
#endif
}
} // namespace scu
