#pragma once

#include <string>

struct ServerParameters {
    std::string ip { "127.0.0.1" };
    std::string home_dir { "/tmp" };
    int port { 1235 };
};

class ArgsParser {
public:
    ArgsParser(int argc, char** argv);

    ServerParameters getServerParams() const { return m_server_params; }

private:
    ServerParameters m_server_params {};
};