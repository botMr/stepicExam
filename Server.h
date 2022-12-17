#pragma once

#include "ArgsParser.h"
#include "ThreadSafeQueue.h"

#include <iostream>
#include <thread>

class Server {
public:
    using socket_fd = int;
    static int SetNonblock(socket_fd fd);
    explicit Server(ServerParameters params);
    void Run();
    void HandleConnections();
    void SendResponse(socket_fd sock, const std::string& request) const;
    ~Server();

private:
    static bool IsValidContentRequested(const std::string& content_filename);
    std::thread m_worker;
    socket_fd m_acceptorSocket { -1 };
    ServerParameters m_serverParams {};
    ThreadSafeQueue<socket_fd> m_connectionEvents {};
    std::vector<socket_fd> m_slaves {};
};

class RequestParser {
public:
    static std::string GetTargetFilename(const std::string& http_request);
};
