#pragma once
#include <string>
#include <mutex>

class YaneuraOuEngine
{
public:
    static YaneuraOuEngine &instance(const std::string &engine_path);

    int evaluate(const std::string &sfen);

private:
    YaneuraOuEngine(const std::string &engine_path);
    ~YaneuraOuEngine();

    YaneuraOuEngine(const YaneuraOuEngine &) = delete;
    YaneuraOuEngine &operator=(const YaneuraOuEngine &) = delete;

    void send(const std::string &cmd);
    std::string receive();

    int in_fd_;  // 親 → 子（stdin）
    int out_fd_; // 子 → 親（stdout）
    pid_t pid_;

    std::mutex mtx_;
};
