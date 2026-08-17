/*  FeatherCrawl.h
 *  Copyright (c) 2026 Eric
 *  介绍：FeatherCrawl 是由 Eric 开发的一款 C++ 网络库，采用 Apache 2.0 协议开源，适用于 C++11 及以上版本
 *  使用方式：#include "FeatherCrawl.h"
 *  编译：MSVC 可直接编译，MinGW/TDM-GCC 需加入 -lwinhttp 参数
 */

#pragma once

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>

namespace web
{

struct Response
{
    int status_code = 0;
    std::string body;
    std::string error_message;
};

struct Headers
{
    std::unordered_map<std::string, std::string> fields;

    void set(const std::string& key, const std::string& value)
    {
        fields[key] = value;
    }

    std::string get(const std::string& key) const
    {
        auto it = fields.find(key);
        return (it != fields.end()) ? it->second : "";
    }

    std::string to_winhttp_string() const
    {
        std::string result;
        for (const auto& pair : fields)
        {
            result += pair.first + ": " + pair.second + "\r\n";
        }
        return result;
    }
};

class WinHttpHandle
{
    HINTERNET handle_;

public:
    WinHttpHandle(HINTERNET h = nullptr) : handle_(h) {}

    ~WinHttpHandle()
    {
        if (handle_)
        {
            WinHttpCloseHandle(handle_);
        }
    }

    HINTERNET get() const
    {
        return handle_;
    }

    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    WinHttpHandle(WinHttpHandle&& other) noexcept : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }
    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept
    {
        if (this != &other)
        {
            if (handle_) WinHttpCloseHandle(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    explicit operator bool() const
    {
        return handle_ != nullptr;
    }
};

inline Response send_request(
    const std::wstring& method,
    const std::wstring& url,
    const std::string& body = "",
    const Headers& headers = Headers(),
    int timeout_ms = 0,
    bool follow_redirect = true,
    int retries = 0)
{
    Response resp;

    URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
    urlComp.dwHostNameLength = 1;
    urlComp.dwUrlPathLength = 1;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp))
    {
        resp.error_message = "Invalid URL";
        return resp;
    }

    std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
    std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    if (path.empty())
    {
        path = L"/";
    }

    for (int attempt = 0; attempt <= retries; ++attempt)
    {
        WinHttpHandle session(WinHttpOpen(L"FeatherCrawl/1.0",
                                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                          WINHTTP_NO_PROXY_NAME,
                                          WINHTTP_NO_PROXY_BYPASS, 0));
        if (!session)
        {
            resp.error_message = "WinHttpOpen failed";
            continue;
        }

        WinHttpHandle connect(WinHttpConnect(session.get(), host.c_str(), urlComp.nPort, 0));
        if (!connect)
        {
            resp.error_message = "WinHttpConnect failed";
            continue;
        }

        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        WinHttpHandle request(WinHttpOpenRequest(connect.get(),
                                                 method.c_str(),
                                                 path.c_str(),
                                                 NULL,
                                                 WINHTTP_NO_REFERER,
                                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 flags));
        if (!request)
        {
            resp.error_message = "WinHttpOpenRequest failed";
            continue;
        }

        if (timeout_ms > 0)
        {
            if (!WinHttpSetTimeouts(request.get(), timeout_ms, timeout_ms, timeout_ms, timeout_ms))
            {
                resp.error_message = "WinHttpSetTimeouts failed";
                continue;
            }
        }

        if (follow_redirect)
        {
            DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
            if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy)))
            {
                resp.error_message = "WinHttpSetOption (redirect) failed";
                continue;
            }
        }

        std::string header_str = headers.to_winhttp_string();
        if (!header_str.empty())
        {
            int wide_len = MultiByteToWideChar(CP_UTF8, 0, header_str.c_str(), -1, nullptr, 0);
            std::wstring wide_headers(wide_len, 0);
            MultiByteToWideChar(CP_UTF8, 0, header_str.c_str(), -1, &wide_headers[0], wide_len);
            if (!WinHttpAddRequestHeaders(request.get(), wide_headers.c_str(), (DWORD)-1L,
                                          WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE))
            {
                resp.error_message = "WinHttpAddRequestHeaders failed";
                continue;
            }
        }

        DWORD total_len = (DWORD)body.size();

        if (!WinHttpSendRequest(request.get(),
                                WINHTTP_NO_ADDITIONAL_HEADERS,
                                0,
                                WINHTTP_NO_REQUEST_DATA,
                                0,
                                total_len,
                                0))
        {
            resp.error_message = "WinHttpSendRequest failed";
            continue;
        }

        if (method == L"POST" && total_len > 0)
        {
            DWORD bytes_written = 0;
            if (!WinHttpWriteData(request.get(), body.c_str(), total_len, &bytes_written))
            {
                resp.error_message = "WinHttpWriteData failed";
                continue;
            }
            if (bytes_written != total_len)
            {
                resp.error_message = "WinHttpWriteData incomplete";
                continue;
            }
        }

        if (!WinHttpReceiveResponse(request.get(), NULL))
        {
            resp.error_message = "WinHttpReceiveResponse failed";
            continue;
        }

        DWORD status_code = 0;
        DWORD size = sizeof(status_code);
        if (WinHttpQueryHeaders(request.get(),
                                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                &status_code, &size, WINHTTP_NO_HEADER_INDEX))
        {
            resp.status_code = static_cast<int>(status_code);
        }
        else
        {
            resp.error_message = "WinHttpQueryHeaders failed";
            continue;
        }

        std::vector<char> buffer;
        DWORD bytes_read = 0;
        do
        {
            char chunk[4096];
            if (WinHttpReadData(request.get(), chunk, sizeof(chunk), &bytes_read))
            {
                if (bytes_read > 0)
                {
                    buffer.insert(buffer.end(), chunk, chunk + bytes_read);
                }
            }
            else
            {
                resp.error_message = "WinHttpReadData failed";
                break;
            }
        } while (bytes_read > 0);

        if (!buffer.empty())
        {
            resp.body.assign(buffer.data(), buffer.size());
        }

        resp.error_message.clear();
        return resp;
    }

    if (resp.error_message.empty())
    {
        resp.error_message = "All retries failed";
    }
    return resp;
}

inline Response get(const std::wstring& url,
                    const Headers& headers = Headers(),
                    int timeout_ms = 0,
                    bool follow_redirect = true,
                    int retries = 0)
{
    return send_request(L"GET", url, "", headers, timeout_ms, follow_redirect, retries);
}

inline Response get(const std::string& url_utf8,
                    const Headers& headers = Headers(),
                    int timeout_ms = 0,
                    bool follow_redirect = true,
                    int retries = 0)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, url_utf8.c_str(), -1, nullptr, 0);
    std::wstring wurl(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, url_utf8.c_str(), -1, &wurl[0], len);
    wurl.pop_back();
    return get(wurl, headers, timeout_ms, follow_redirect, retries);
}

inline Response post(const std::wstring& url,
                     const std::string& body,
                     const std::string& content_type = "application/x-www-form-urlencoded",
                     const Headers& headers = Headers(),
                     int timeout_ms = 0,
                     bool follow_redirect = true,
                     int retries = 0)
{
    Headers h = headers;
    if (h.fields.find("Content-Type") == h.fields.end())
    {
        h.set("Content-Type", content_type);
    }
    return send_request(L"POST", url, body, h, timeout_ms, follow_redirect, retries);
}

inline Response post(const std::string& url_utf8,
                     const std::string& body,
                     const std::string& content_type = "application/x-www-form-urlencoded",
                     const Headers& headers = Headers(),
                     int timeout_ms = 0,
                     bool follow_redirect = true,
                     int retries = 0)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, url_utf8.c_str(), -1, nullptr, 0);
    std::wstring wurl(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, url_utf8.c_str(), -1, &wurl[0], len);
    wurl.pop_back();
    return post(wurl, body, content_type, headers, timeout_ms, follow_redirect, retries);
}

inline std::string to_utf8(const std::string& src, int codepage)
{
    if (src.empty())
    {
        return {};
    }
    int len = MultiByteToWideChar(codepage, 0, src.c_str(), (int)src.size(), nullptr, 0);
    if (len == 0)
    {
        return src;
    }
    std::wstring wstr(len, 0);
    MultiByteToWideChar(codepage, 0, src.c_str(), (int)src.size(), &wstr[0], len);
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), len, nullptr, 0, nullptr, nullptr);
    if (utf8_len == 0)
    {
        return src;
    }
    std::string utf8(utf8_len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), len, &utf8[0], utf8_len, nullptr, nullptr);
    return utf8;
}

inline std::string to_utf8(const std::string& src, const std::string& encoding)
{
    static const std::unordered_map<std::string, int> codepage_map =
    {
        {"UTF-8",      CP_UTF8},
        {"GBK",        936},
        {"GB2312",     936},
        {"BIG5",       950},
        {"SHIFT-JIS",  932},
        {"EUC-KR",     949},
        {"ISO-8859-1", 28591},
        {"WINDOWS-1252", 1252},
    };
    auto it = codepage_map.find(encoding);
    if (it != codepage_map.end())
    {
        return to_utf8(src, it->second);
    }
    return src;
}

inline std::string text_size(size_t bytes, const std::string& unit = "")
{
    const char* units[] = { "B", "KB", "MB", "GB" };
    int unit_index = -1;

    if (!unit.empty())
    {
        std::string u = unit;
        std::transform(u.begin(), u.end(), u.begin(), ::toupper);
        if (u == "B")
        {
            unit_index = 0;
        }
        else if (u == "KB")
        {
            unit_index = 1;
        }
        else if (u == "MB")
        {
            unit_index = 2;
        }
        else if (u == "GB")
        {
            unit_index = 3;
        }
        else if (u == "AUTO")
        {
            unit_index = -1;
        }
    }

    double size = static_cast<double>(bytes);
    int chosen_index = 0;

    if (unit_index >= 0 && unit_index <= 3)
    {
        for (int i = 0; i < unit_index; i++)
        {
            size /= 1024.0;
        }
        chosen_index = unit_index;
    }
    else
    {
        while (size >= 1024.0 && chosen_index < 3)
        {
            size /= 1024.0;
            chosen_index++;
        }
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[chosen_index];
    return oss.str();
}

inline std::string substring(const std::string& str, size_t start, size_t end)
{
    if (start >= str.size() || end < start)
    {
        return "";
    }
    if (end >= str.size())
    {
        end = str.size() - 1;
    }
    return str.substr(start, end - start + 1);
}

inline std::string lines(const std::string& str, size_t start_line, size_t end_line)
{
    std::vector<std::string> lines_vec;
    std::istringstream stream(str);
    std::string line;
    while (std::getline(stream, line))
    {
        lines_vec.push_back(line);
    }

    if (start_line < 1 || start_line > lines_vec.size())
    {
        return "";
    }
    if (end_line > lines_vec.size())
    {
        end_line = lines_vec.size();
    }
    if (start_line > end_line)
    {
        return "";
    }

    std::ostringstream result;
    for (size_t i = start_line - 1; i < end_line; i++)
    {
        result << lines_vec[i];
        if (i != end_line - 1)
        {
            result << '\n';
        }
    }
    return result.str();
}

} // namespace web