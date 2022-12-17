#include "HttpResponse.h"

#include <fstream>
#include <sstream>

Http200Response::Http200Response(const std::string& filename)
{
    std::ifstream f(filename.c_str());
    if (f.good()) {
        std::stringstream buffer;
        buffer << f.rdbuf();
        m_body = buffer.str();
    }
}
std::string Http200Response::GetResponse() const
{
    std::stringstream ss;
    ss << "HTTP/1.0 200 OK"
       << "\r\n"
       << "Content-length: "
       << m_body.size()
       << "\r\n"
       << "Content-Type: text/html"
       << "\r\n\r\n"
       << m_body;

    return ss.str();
}

std::string Http404Response::GetResponse() const
{
    std::stringstream ss;
    ss << "HTTP/1.0 404 NOT FOUND"
       << "\r\n"
       << "Content-length: "
       << 0
       << "\r\n"
       << "Content-Type: text/html"
       << "\r\n\r\n";
    return ss.str();
}

std::string ResponseFactory::create_response(const HttpResponseType type, const std::string& content_filename)
{
    Http404Response response404 {};
    std::string response = response404.GetResponse();
    switch (type) {
    case HttpResponseType::OK: {
        Http200Response response200 { content_filename };
        response = response200.GetResponse();
        break;
    }
    case HttpResponseType::ERROR: {
        response = response404.GetResponse();
        break;
    }
    }

    return response;
}
