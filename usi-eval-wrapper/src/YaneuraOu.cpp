#include "YaneuraOu/YaneuraOu.hpp"

#include <stdexcept>
#include <cstring>
#include <iostream>

#include <sys/wait.h>
#include <unistd.h>

// -----------------------
// constructor
// -----------------------
YaneuraOuValue::YaneuraOuValue(
    const std::string &engine_path,
    bool debug_startup)
    : debug_startup_(debug_startup)
{
    int to_child[2];
    int from_child[2];

    if (pipe(to_child) == -1 || pipe(from_child) == -1)
        throw std::runtime_error("pipe() failed");

    child_pid = fork();
    if (child_pid == -1)
        throw std::runtime_error("fork() failed");

    if (child_pid == 0)
    {
        // ---- child ----
        dup2(to_child[0], STDIN_FILENO);
        dup2(from_child[1], STDOUT_FILENO);
        dup2(from_child[1], STDERR_FILENO);

        close(to_child[1]);
        close(from_child[0]);

        execl(engine_path.c_str(), engine_path.c_str(), nullptr);
        _exit(1);
    }

    // ---- parent ----
    close(to_child[0]);
    close(from_child[1]);

    write_fd = to_child[1];
    read_fd = from_child[0];

    if (debug_startup_)
        std::cerr << "[YaneuraOu] engine launched\n";

    // ---- USI init ----
    send("usi\n");
    wait_for("usiok");

    send("isready\n");
    wait_for("readyok");

    if (debug_startup_)
        std::cerr << "[YaneuraOu] ready\n";
}

// -----------------------
// destructor
// -----------------------
YaneuraOuValue::~YaneuraOuValue()
{
    if (write_fd != -1)
    {
        send("quit\n");
        close(write_fd);
    }

    if (read_fd != -1)
        close(read_fd);

    if (child_pid > 0)
        waitpid(child_pid, nullptr, 0);
}

// -----------------------
// send
// -----------------------
void YaneuraOuValue::send(const std::string &cmd)
{
    ::write(write_fd, cmd.c_str(), cmd.size());
}

// -----------------------
// readLine
// -----------------------
std::string YaneuraOuValue::readLine()
{
    std::string line;
    char c;

    while (true)
    {
        ssize_t n = ::read(read_fd, &c, 1);
        if (n <= 0)
            break;

        if (c == '\n')
            break;

        line.push_back(c);
    }
    return line;
}

// -----------------------
// wait_for token
// -----------------------
void YaneuraOuValue::wait_for(const std::string &token)
{
    while (true)
    {
        std::string line = readLine();
        if (line.find(token) != std::string::npos)
            return;
    }
}

// -----------------------
// evaluate
// -----------------------
int YaneuraOuValue::evaluate(const std::string &sfen)
{
    send("position sfen " + sfen + "\n");
    send("go depth 1\n");

    int cp = 0;

    while (true)
    {
        std::string line = readLine();

        // info depth 1 score cp xxx
        auto pos = line.find("score cp ");
        if (pos != std::string::npos)
        {
            cp = std::stoi(line.substr(pos + 9));
        }

        if (line.find("bestmove") != std::string::npos)
            break;
    }

    return cp;
}
