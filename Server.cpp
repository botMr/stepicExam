#include "Server.h"
#include "HttpResponse.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

constexpr int nMaxConnect = 100;

int Server::SetNonblock(socket_fd fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, (unsigned)flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIONBIO, &flags);
#endif
}

Server::Server(ServerParameters params)
    : m_serverParams(std::move(params))
{
    m_acceptorSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_acceptorSocket < 0) {
        throw std::runtime_error("Cannot create acceptor socket: " + std::string(std::strerror(errno)));
    }
    int opt = 1;
    if (setsockopt(m_acceptorSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Cannot set sockopt on acceptor socket: " + std::string(std::strerror(errno)));
    }

    struct sockaddr_in sock_addr { };
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(m_serverParams.port);
    sock_addr.sin_addr.s_addr = inet_addr(m_serverParams.ip.c_str());

    if (bind(m_acceptorSocket, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
        throw std::runtime_error("Cannot bind acceptor socket: " + std::string(std::strerror(errno)));
    }

    if (SetNonblock(m_acceptorSocket) < 0) {
        throw std::runtime_error("Cannot set non blocking mode: " + std::string(std::strerror(errno)));
    }

    if (listen(m_acceptorSocket, SOMAXCONN) < 0) {
        throw std::runtime_error("Cannot listen on acceptor socket: " + std::string(std::strerror(errno)));
    }

    std::cerr << "Max available threads: " << std::thread::hardware_concurrency() << '\n';
    m_worker = std::thread(&Server::HandleConnections, this);
}

void Server::HandleConnections()
{
    std::cerr << "Worker thread successfully started..\n";
    constexpr int kReadBufMax = 1024;
    std::string recieved_msg;
    // int bytes_read = 0;
    while (true) {
        auto sock_with_event = m_connectionEvents.WaitAndPop();
        recieved_msg.clear();
        char read_buf[kReadBufMax];
        memset(read_buf, '\0', kReadBufMax);

        int bytes = recv(*sock_with_event, read_buf, kReadBufMax - 1, 0);
        // bytes_read += bytes;
        if (bytes <= 0 && errno != EAGAIN) {
            // std::cerr << "Connection closed or error ocurred: " << std::strerror(errno) << '\n';
            // shutdown(*sock_with_event, SHUT_RDWR);
            close(*sock_with_event);
        } else if (errno == EAGAIN) {
            SendResponse(*sock_with_event, recieved_msg);
            // std::cerr << "Msg received EAGAIN: " << recieved_msg << '\n';
        } else if (bytes > 0) {
            recieved_msg.append(read_buf, bytes);
            SendResponse(*sock_with_event, recieved_msg);
            // std::cerr << "Msg received: \n" << recieved_msg << '\n';
        }
    }
}

void Server::SendResponse(const socket_fd sock, const std::string& request) const
{
    std::string content_file = RequestParser::GetTargetFilename(request);
    std::string content_filename = m_serverParams.home_dir + "/" + content_file;
    HttpResponseType resp_type = IsValidContentRequested(content_filename) ? HttpResponseType::OK : HttpResponseType::ERROR;

    std::string response = ResponseFactory::create_response(resp_type, content_filename);
    // std::cout << response << "res\n\n";
    int bytes_sent = send(sock, response.c_str(), response.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "error cannot send response: " << std::strerror(errno) << '\n';
    }
}

void Server::Run()
{
    std::cerr << "Running server...\n";
    int epool_instance = epoll_create1(0);
    if (epool_instance < 0) {
        throw std::runtime_error("Cannot create epoll: " + std::string(std::strerror(errno)));
    }
    struct epoll_event epollEvent { };
    epollEvent.data.fd = m_acceptorSocket;
    epollEvent.events = EPOLLIN | EPOLLET;
    epoll_ctl(epool_instance, EPOLL_CTL_ADD, m_acceptorSocket, &epollEvent);

    while (true) {
        struct epoll_event events[kMaxConnections];
        int events_amount = epoll_wait(epool_instance, events, kMaxConnections, -1);
        for (int i = 0; i < events_amount; ++i) {
            if ((events[i].events & EPOLLERR) || events[i].events & EPOLLHUP) {
                throw std::runtime_error("EPOLL ERROR caught\n");
            }
            if (events[i].data.fd == m_acceptorSocket) {
                socket_fd slaveSock = accept(m_acceptorSocket, nullptr, nullptr);
                if (SetNonblock(m_acceptorSocket) < 0) {
                    throw std::runtime_error("Cannot set non blocking mode: " + std::string(std::strerror(errno)));
                }
                struct epoll_event event { };
                event.data.fd = slaveSock;
                event.events = EPOLLIN | EPOLLET;
                epoll_ctl(epool_instance, EPOLL_CTL_ADD, slaveSock, &event);
                m_slaves.push_back(slaveSock);

            } else {
                m_connectionEvents.Push(events[i].data.fd);
            }
        }
    }

    close(epool_instance);
}

Server::~Server()
{
    if (m_acceptorSocket > 0) {
        close(m_acceptorSocket);
    }
    for (const auto& sock : m_slaves) {
        close(sock);
    }
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

bool Server::IsValidContentRequested(const std::string& content_filename)
{
    std::ifstream f(content_filename.c_str());
    return f.good();
}

std::string RequestParser::GetTargetFilename(const std::string& http_request)
{
    std::size_t pos1 = http_request.find("GET /");
    std::size_t pos2 = http_request.find(" HTTP/1");
    if (pos1 == std::string::npos || pos2 == std::string::npos)
        return "";
    std::string ind = http_request.substr(pos1 + 5, pos2 - pos1 - 5);
    if (ind.empty())
        return "index.html";

    auto pos = ind.find('?');
    if (pos == std::string::npos)
        return ind;
    else
        return ind.substr(0, pos);
}