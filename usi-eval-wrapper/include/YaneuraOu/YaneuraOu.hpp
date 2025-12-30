#pragma once

#include <string>
#include <unistd.h>
#include <sys/types.h>

class YaneuraOuValue
{
public:
    explicit YaneuraOuValue(
        const std::string &engine_path,
        bool debug_startup = false);
    ~YaneuraOuValue();

    // sfen を与えて cp を返す（手番視点）
    int evaluate(const std::string &sfen);

private:
    pid_t child_pid{-1};

    int write_fd{-1}; // 親 → 子 stdin
    int read_fd{-1};  // 子 → 親 stdout

    void send(const std::string &cmd);
    std::string readLine();

    void wait_for(const std::string &token);

    bool debug_startup_{false};
};
