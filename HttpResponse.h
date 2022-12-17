#pragma once

#include <string>

enum class HttpResponseType {
    OK = 200,
    ERROR = 404
};

class IHttpResponse {
public:
    virtual std::string GetResponse() const = 0;
    virtual ~IHttpResponse() = default;
};

class Http200Response final : public IHttpResponse {
public:
    explicit Http200Response(const std::string& filename);
    std::string GetResponse() const override;

private:
    std::string m_body { "" };
};

class Http404Response final : public IHttpResponse {
public:
    std::string GetResponse() const override;

private:
};

class ResponseFactory {
public:
    static std::string create_response(HttpResponseType type, const std::string& content_filename);
};