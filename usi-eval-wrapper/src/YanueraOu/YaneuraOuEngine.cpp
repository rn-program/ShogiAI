#include "YaneuraOu/YaneuraOuEngine.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <cstring>
#include <stdexcept>
#include <iostream>

YaneuraOuEngine &YaneuraOuEngine::instance(const std::string &engine_path)
{
    static YaneuraOuEngine inst(engine_path);
    return inst;
}

YaneuraOuEngine::YaneuraOuEngine(const std::string &engine_path)
{
    int to_child[2];
    int from_child[2];

    if (pipe(to_child) == -1 || pipe(from_child) == -1)
        throw std::runtime_error("pipe() failed");

    pid_ = fork();
    if (pid_ < 0)
        throw std::runtime_error("fork() failed");

    if (pid_ == 0)
    {
        // ===== 子プロセス =====

        dup2(to_child[0], STDIN_FILENO);
        dup2(from_child[1], STDOUT_FILENO);
        dup2(from_child[1], STDERR_FILENO);

        close(to_child[1]);
        close(from_child[0]);

        execl(
            engine_path.c_str(),
            engine_path.c_str(),
            nullptr);

        // exec 失敗時のみここに来る
        std::perror("exec failed");
        _exit(1);
    }

    // ===== 親プロセス =====
    close(to_child[0]);
    close(from_child[1]);

    in_fd_ = to_child[1];
    out_fd_ = from_child[0];

    // 非ブロッキング解除（安全）
    fcntl(out_fd_, F_SETFL, fcntl(out_fd_, F_GETFL) & ~O_NONBLOCK);

    // ===== USI 初期化 =====
    send("usi");

    while (true)
    {
        std::string line = receive();
        if (line.find("usiok") != std::string::npos)
            break;
    }

    send("isready");
    while (true)
    {
        std::string line = receive();
        if (line.find("readyok") != std::string::npos)
            break;
    }
}

YaneuraOuEngine::~YaneuraOuEngine()
{
    if (pid_ > 0)
    {
        send("quit");
        close(in_fd_);
        close(out_fd_);
        waitpid(pid_, nullptr, 0);
    }
}

void YaneuraOuEngine::send(const std::string &cmd)
{
    std::string s = cmd + "\n";
    ssize_t n = write(in_fd_, s.c_str(), s.size());
    if (n < 0)
        throw std::runtime_error("write() failed");
}

std::string YaneuraOuEngine::receive()
{
    char buf[4096];
    ssize_t n = read(out_fd_, buf, sizeof(buf) - 1);
    if (n <= 0)
        throw std::runtime_error("read() failed");

    buf[n] = '\0';
    return std::string(buf);
}

int YaneuraOuEngine::evaluate(const std::string &sfen)
{
    std::lock_guard<std::mutex> lock(mtx_);

    send("position sfen " + sfen);
    send("go depth 1");

    while (true)
    {
        std::string line = receive();
        if (line.find("score cp") != std::string::npos)
        {
            auto pos = line.find("score cp");
            int cp = std::stoi(line.substr(pos + 9));
            return cp;
        }
    }
}
