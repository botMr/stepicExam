#include "ArgsParser.h"

#include <getopt.h>
#include <iostream>

ArgsParser::ArgsParser(int argc, char** argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "h:p:d:")) != -1) {
        switch (opt) {
        case 'h': {
            std::string ip = std::string(optparse);
            if (!ip.empty()) {
                m_server_params.ip = std::move(ip);
            }
            break;
        }
        case 'p': {
            int port = std::atoi(optparse);
            if (port != -1) {
                m_server_params.port = port;
            }
            break;
        }
        case 'd': {
            std::string dir = std::string(optparse);
            if (!dir.empty()) {
                m_server_params.home_dir = std::move(dir);
            }
            break;
        }
        default:
            std::cerr << "Usage: " << char(opt) << "  [-t nsecs] [-n] name\n";
            exit(EXIT_FAILURE);
        }
    }
}