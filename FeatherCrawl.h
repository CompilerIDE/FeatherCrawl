/*  feathercrawl.h
 *  Copyright (c) 2026 Eric
 *  Description: FeatherCrawl is a C++ network library developed by Eric, open-sourced under the Apache 2.0 license, suitable for C++11 and later versions.
 *  Usage: #include "feathercrawl.h"
 *  Compilation: On Windows/MSVC, it can be compiled directly; on Windows with MinGW/TDM-GCC, the -lwinhttp flag is required; on Linux, the flags -lssl -lcrypto -pthread are required.
 *  Implementation: Windows uses WinHTTP and Microsoft Edge WebView2 for rendering; Linux uses Socket API and OpenSSL.
*/

#pragma once

#if defined(_WIN32)

#include <windows.h>
#include <objbase.h>
#include <winreg.h>
#include <winhttp.h>

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <limits>
#include <cctype>
#include <mutex>
#include <memory>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <chrono>
#include <iostream>
#include <cwchar>

#if defined(FEATHERCRAWL_DISABLE_WEBVIEW2)
#define FEATHERCRAWL_HAS_WEBVIEW2 0
#else
#define FEATHERCRAWL_HAS_WEBVIEW2 1
#endif

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#if FEATHERCRAWL_HAS_WEBVIEW2
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#endif
#endif

namespace web
{

namespace detail
{

inline char ascii_lower(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
}

inline wchar_t ascii_lower(wchar_t ch)
{
    return (ch >= L'A' && ch <= L'Z') ? static_cast<wchar_t>(ch - L'A' + L'a') : ch;
}

inline std::string ascii_lower_copy(const std::string& value)
{
    std::string result = value;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = ascii_lower(result[i]);
    }
    return result;
}

inline std::wstring ascii_lower_copy(const std::wstring& value)
{
    std::wstring result = value;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = ascii_lower(result[i]);
    }
    return result;
}

inline bool ascii_iequals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (ascii_lower(a[i]) != ascii_lower(b[i]))
        {
            return false;
        }
    }
    return true;
}

inline bool ascii_iequals(const std::wstring& a, const std::wstring& b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (ascii_lower(a[i]) != ascii_lower(b[i]))
        {
            return false;
        }
    }
    return true;
}

inline std::string trim_ascii(const std::string& value)
{
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
    {
        ++first;
    }
    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
    {
        --last;
    }
    return value.substr(first, last - first);
}

inline std::wstring trim_ascii(const std::wstring& value)
{
    size_t first = 0;
    while (first < value.size() && (value[first] == L' ' || value[first] == L'\t' || value[first] == L'\r' || value[first] == L'\n'))
    {
        ++first;
    }
    size_t last = value.size();
    while (last > first)
    {
        wchar_t c = value[last - 1];
        if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n')
        {
            break;
        }
        --last;
    }
    return value.substr(first, last - first);
}

inline bool bytes_to_wide(const std::string& src, UINT codepage, std::wstring& out)
{
    out.clear();
    if (src.empty())
    {
        return true;
    }
    if (src.size() > static_cast<size_t>((std::numeric_limits<int>::max)()))
    {
        return false;
    }
    DWORD flags = codepage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
    int n = MultiByteToWideChar(codepage, flags, src.data(), static_cast<int>(src.size()), NULL, 0);
    if (n <= 0)
    {
        return false;
    }
    out.assign(static_cast<size_t>(n), L'\0');
    return MultiByteToWideChar(codepage, flags, src.data(), static_cast<int>(src.size()), &out[0], n) == n;
}

inline bool wide_to_codepage(const std::wstring& src, UINT codepage, std::string& out)
{
    out.clear();
    if (src.empty())
    {
        return true;
    }
    if (src.size() > static_cast<size_t>((std::numeric_limits<int>::max)()))
    {
        return false;
    }
    int n = WideCharToMultiByte(codepage, 0, src.data(), static_cast<int>(src.size()), NULL, 0, NULL, NULL);
    if (n <= 0)
    {
        return false;
    }
    out.assign(static_cast<size_t>(n), '\0');
    return WideCharToMultiByte(codepage, 0, src.data(), static_cast<int>(src.size()), &out[0], n, NULL, NULL) == n;
}

inline bool utf8_to_wide(const std::string& src, std::wstring& out)
{
    return bytes_to_wide(src, CP_UTF8, out);
}

inline std::string wide_to_utf8(const std::wstring& src)
{
    std::string out;
    if (!wide_to_codepage(src, CP_UTF8, out))
    {
        return "";
    }
    return out;
}

inline void ensure_utf8_console()
{
    static std::once_flag once;
    std::call_once(once, []()
    {
        if (GetConsoleOutputCP() != CP_UTF8)
        {
            SetConsoleOutputCP(CP_UTF8);
        }
    });
}

inline std::string& global_language()
{
    static std::string lang = "en_US";
    return lang;
}

static thread_local std::string* active_language = NULL;

inline std::string current_language()
{
    return active_language ? *active_language : global_language();
}

inline std::string normalize_language(const std::string& lang)
{
    std::string value = ascii_lower_copy(trim_ascii(lang));
    if (value == "zh" || value == "cn" || value == "zh_cn" || value == "zh-cn" || value == "chinese")
    {
        return "zh_CN";
    }
    return "en_US";
}

inline void set_default_language(const std::string& lang)
{
    global_language() = normalize_language(lang);
}

inline std::string text(const wchar_t* value)
{
    ensure_utf8_console();
    std::string original = wide_to_utf8(value ? std::wstring(value) : std::wstring());
    if (current_language() == "zh_CN")
    {
        return original;
    }

    static const std::map<std::string, std::string> translations = {
        {"HTTP 请求失败，状态码 ", "HTTP request failed: status code "},
        {"HTTP 方法不能为空", "HTTP method cannot be empty"},
        {"请求超时", "Request timed out"},
        {"无法解析服务器名称", "Unable to resolve server name"},
        {"无法连接到服务器", "Unable to connect to server"},
        {"与服务器的连接发生错误", "Connection error occurred with server"},
        {"HTTPS 安全连接失败", "HTTPS secure connection failed"},
        {"URL 无效", "Invalid URL"},
        {"不支持该 URL 协议", "Unsupported URL scheme"},
        {"身份验证失败", "Authentication failed"},
        {"操作已取消", "Operation was cancelled"},
        {"WinHTTP 请求执行失败", "WinHTTP request execution failed"},
        {"请求超时时间不能小于 0", "Timeout cannot be less than 0"},
        {"最大重定向次数不能小于 0", "Maximum redirects cannot be less than 0"},
        {"重试次数不能小于 0", "Retry count cannot be less than 0"},
        {"重试次数不能大于 100", "Retry count cannot exceed 100"},
        {"最大重定向次数不能大于 100", "Maximum redirects cannot exceed 100"},
        {"服务器响应超过允许的最大大小", "Server response exceeded the maximum allowed size"},
        {"写入下载文件失败", "Failed to write downloaded file"},
        {"已取消选择下载位置", "Download destination selection was cancelled"},
        {"无法打开文件保存对话框", "Unable to open the file save dialog"},
        {"文件保存对话框执行失败", "The file save dialog failed"},
        {"URL 或建议文件名不是有效的 UTF-8 编码", "URL or suggested filename is not valid UTF-8"},
        {"无", "None"}
    };

    auto it = translations.find(original);
    if (it != translations.end())
    {
        return it->second;
    }

    return "Operation failed";
}

inline void secure_clear(std::string& value)
{
    if (!value.empty())
    {
        volatile char* p = &value[0];
        for (size_t i = 0; i < value.size(); ++i)
        {
            p[i] = 0;
        }
    }
    value.clear();
}

class SecureStringGuard
{
    std::string* value_;
public:
    explicit SecureStringGuard(std::string& value) : value_(&value)
    {

        }
    ~SecureStringGuard()
    {
         if (value_)
    {
        secure_clear(*value_);
    }

    }
    SecureStringGuard(const SecureStringGuard&) = delete;
    SecureStringGuard& operator=(const SecureStringGuard&) = delete;
};

inline bool is_valid_utf8(const std::string& value)
{
    std::wstring temp;
    return utf8_to_wide(value, temp);
}

inline bool convert_codepage(const std::string& src, UINT from, UINT to, std::string& out)
{
    std::wstring wide;
    if (!bytes_to_wide(src, from, wide))
    {
        return false;
    }
    return wide_to_codepage(wide, to, out);
}

inline bool valid_header_name(const std::string& name)
{
    if (name.empty())
    {
        return false;
    }
    for (size_t i = 0; i < name.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(name[i]);
        bool alnum = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        bool punctuation = c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
        if (!alnum && !punctuation)
        {
            return false;
        }
    }
    return true;
}

inline bool valid_header_value(const std::string& value)
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\r' || value[i] == '\n' || value[i] == '\0')
        {
            return false;
        }
    }
    return true;
}

inline std::string normalize_encoding_name(const std::string& encoding)
{
    std::string value = trim_ascii(encoding);
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\'')))
    {
        value = value.substr(1, value.size() - 2);
    }
    value = ascii_lower_copy(value);
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '_')
        {
            value[i] = '-';
        }
    }
    return value;
}

inline bool encoding_to_codepage(const std::string& encoding, UINT& codepage)
{
    std::string value = normalize_encoding_name(encoding);
    if (value == "utf-8" || value == "utf8")
    {
         codepage = CP_UTF8; return true;
    }
    if (value == "gbk" || value == "gb2312" || value == "cp936")
    {
         codepage = 936; return true;
    }
    if (value == "gb18030" || value == "cp54936")
    {
         codepage = 54936; return true;
    }
    if (value == "big5" || value == "big-5" || value == "cp950")
    {
         codepage = 950; return true;
    }
    if (value == "shift-jis" || value == "shiftjis" || value == "sjis" || value == "cp932")
    {
         codepage = 932; return true;
    }
    if (value == "euc-kr" || value == "cp949")
    {
         codepage = 949; return true;
    }
    if (value == "iso-8859-1" || value == "latin1" || value == "latin-1")
    {
         codepage = 28591; return true;
    }
    if (value == "windows-1252" || value == "cp1252")
    {
         codepage = 1252; return true;
    }
    return false;
}

inline std::string extract_charset(const std::string& source)
{
    std::string lower = ascii_lower_copy(source);
    size_t pos = 0;
    while ((pos = lower.find("charset", pos)) != std::string::npos)
    {
        size_t p = pos + 7;
        while (p < lower.size() && std::isspace(static_cast<unsigned char>(lower[p])))
        {
            ++p;
        }
        if (p >= lower.size() || lower[p] != '=')
        {
             pos = p; continue;
        }
        ++p;
        while (p < source.size() && std::isspace(static_cast<unsigned char>(source[p])))
        {
            ++p;
        }
        if (p >= source.size())
        {
            return "";
        }
        char quote = 0;
        if (source[p] == '"' || source[p] == '\'')
        {
             quote = source[p]; ++p;
        }
        size_t start = p;
        while (p < source.size())
        {
            unsigned char ch = static_cast<unsigned char>(source[p]);
            if (quote != 0)
            {
                if (source[p] == quote)
                {
                    break;
                }
            }
            else if (std::isspace(ch) || source[p] == ';' || source[p] == '>' || source[p] == '/')
            {
                break;
            }
            ++p;
        }
        if (p > start)
        {
            return source.substr(start, p - start);
        }
        pos = p + 1;
    }
    return "";
}

inline std::string detect_html_charset(const std::string& body)
{
    size_t scan = (std::min)(body.size(), static_cast<size_t>(16384));
    return extract_charset(body.substr(0, scan));
}

inline bool is_json_content_type(const std::string& content_type)
{
    std::string value = ascii_lower_copy(content_type);
    return value.find("application/json") != std::string::npos || value.find("+json") != std::string::npos;
}

inline bool is_text_content_type(const std::string& content_type)
{
    if (content_type.empty())
    {
        return false;
    }
    std::string value = ascii_lower_copy(content_type);
    return value.find("text/") == 0 || value.find("application/json") != std::string::npos || value.find("+json") != std::string::npos || value.find("application/xml") != std::string::npos || value.find("+xml") != std::string::npos || value.find("application/javascript") != std::string::npos || value.find("application/x-javascript") != std::string::npos || value.find("application/x-www-form-urlencoded") != std::string::npos || value.find("application/graphql") != std::string::npos;
}

inline bool looks_like_text(const std::string& body)
{
    if (body.empty())
    {
        return false;
    }
    size_t i = 0;
    if (body.size() >= 3 && static_cast<unsigned char>(body[0]) == 0xEF && static_cast<unsigned char>(body[1]) == 0xBB && static_cast<unsigned char>(body[2]) == 0xBF)
    {
        i = 3;
    }
    while (i < body.size() && std::isspace(static_cast<unsigned char>(body[i])))
    {
        ++i;
    }
    if (i >= body.size())
    {
        return true;
    }
    char c = body[i];
    return c == '<' || c == '{' || c == '[' || c == '"' || c == '\'';
}

inline bool prepare_request_body(const std::string& body, const std::string& content_type, std::string& prepared, std::string& error)
{
    prepared = body;
    error.clear();
    if (body.empty() || !is_text_content_type(content_type))
    {
        return true;
    }
    if (is_json_content_type(content_type))
    {
        if (is_valid_utf8(body))
        {
            return true;
        }
        UINT acp = GetACP();
        if (acp != CP_UTF8 && convert_codepage(body, acp, CP_UTF8, prepared))
        {
            return true;
        }
        error = text(L"JSON \u8bf7\u6c42\u4f53\u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u6587\u672c\uff0c\u4e5f\u65e0\u6cd5\u4ece Windows \u5f53\u524d\u7cfb\u7edf\u7f16\u7801\u8f6c\u6362\u4e3a UTF-8");
        return false;
    }
    std::string charset = extract_charset(content_type);
    if (!charset.empty())
    {
        UINT cp = 0;
        if (encoding_to_codepage(charset, cp))
        {
            if (cp != CP_UTF8)
            {
                return true;
            }
            if (is_valid_utf8(body))
            {
                return true;
            }
            UINT acp = GetACP();
            if (acp != CP_UTF8 && convert_codepage(body, acp, CP_UTF8, prepared))
            {
                return true;
            }
            error = text(L"\u8bf7\u6c42\u4f53\u58f0\u660e\u4e3a UTF-8\uff0c\u4f46\u6570\u636e\u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u6587\u672c");
            return false;
        }
    }
    if (is_valid_utf8(body))
    {
        return true;
    }
    UINT acp = GetACP();
    if (acp != CP_UTF8 && convert_codepage(body, acp, CP_UTF8, prepared))
    {
        return true;
    }
    error = text(L"\u8bf7\u6c42\u4f53\u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u6587\u672c\uff0c\u4e5f\u65e0\u6cd5\u4ece Windows \u5f53\u524d\u7cfb\u7edf\u7f16\u7801\u8f6c\u6362\u4e3a UTF-8");
    return false;
}

inline std::string normalize_response_body(const std::string& body, const std::string& content_type)
{
    if (body.empty())
    {
        return body;
    }
    if (!is_text_content_type(content_type) && !looks_like_text(body))
    {
        return body;
    }
    std::string data = body;
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xEF && static_cast<unsigned char>(data[1]) == 0xBB && static_cast<unsigned char>(data[2]) == 0xBF)
    {
        data.erase(0, 3);
    }
    if (is_json_content_type(content_type))
    {
        if (is_valid_utf8(data))
        {
            return data;
        }
        UINT acp = GetACP();
        if (acp != 0 && acp != CP_UTF8)
        {
            std::wstring wide;
            if (bytes_to_wide(data, acp, wide))
            {
                std::string converted = wide_to_utf8(wide);
                if (!converted.empty())
                {
                    return converted;
                }
            }
        }
        return data;
    }
    std::string charset = extract_charset(content_type);
    if (charset.empty())
    {
        charset = detect_html_charset(data);
    }
    UINT source_cp = 0;
    if (!charset.empty() && encoding_to_codepage(charset, source_cp))
    {
        std::wstring wide;
        if (bytes_to_wide(data, source_cp, wide))
        {
            std::string converted = wide_to_utf8(wide);
            if (!converted.empty() || data.empty())
            {
                return converted;
            }
        }
    }
    if (is_valid_utf8(data))
    {
        return data;
    }
    UINT acp = GetACP();
    if (acp != 0 && acp != CP_UTF8)
    {
        std::wstring wide;
        if (bytes_to_wide(data, acp, wide))
        {
            std::string converted = wide_to_utf8(wide);
            if (!converted.empty() || data.empty())
            {
                return converted;
            }
        }
    }
    return data;
}

inline std::wstring normalize_url_for_crack(const std::wstring& url)
{
    size_t scheme = url.find(L"://");
    if (scheme == std::wstring::npos)
    {
        return url;
    }
    size_t authority = scheme + 3;
    size_t delimiter = url.find_first_of(L"/?#", authority);
    if (delimiter != std::wstring::npos && (url[delimiter] == L'?' || url[delimiter] == L'#'))
    {
        std::wstring result = url;
        result.insert(delimiter, 1, L'/');
        return result;
    }
    return url;
}

struct ParsedUrl
{
    INTERNET_SCHEME scheme;
    bool secure;
    std::wstring scheme_text;
    std::wstring host;
    INTERNET_PORT port;
    std::wstring path_query;
    std::wstring path_only;
    std::wstring absolute;
};

inline bool parse_url(const std::wstring& input, ParsedUrl& out, std::string& error, bool* unsupported_scheme = NULL)
{
    error.clear();
    if (unsupported_scheme)
    {
        *unsupported_scheme = false;
    }
    if (input.empty())
    {
        error = text(L"URL \u4e0d\u80fd\u4e3a\u7a7a");
        return false;
    }
    std::wstring url = normalize_url_for_crack(input);
    if (url.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
    {
        error = text(L"URL \u8fc7\u957f\uff0c\u8d85\u51fa WinHTTP \u53ef\u5904\u7406\u8303\u56f4");
        return false;
    }
    URL_COMPONENTS c = {};
    c.dwStructSize = sizeof(c);
    c.dwSchemeLength = 1;
    c.dwHostNameLength = 1;
    c.dwUrlPathLength = 1;
    c.dwExtraInfoLength = 1;
    c.dwUserNameLength = 1;
    c.dwPasswordLength = 1;
    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &c))
    {
        error = text(L"URL \u65e0\u6548");
        return false;
    }
    if (c.nScheme != INTERNET_SCHEME_HTTP && c.nScheme != INTERNET_SCHEME_HTTPS)
    {
        if (unsupported_scheme)
        {
            *unsupported_scheme = true;
        }
        error = text(L"\u4ec5\u652f\u6301 HTTP \u548c HTTPS URL");
        return false;
    }
    if (!c.lpszHostName || c.dwHostNameLength == 0)
    {
        error = text(L"URL \u4e2d\u6ca1\u6709\u6709\u6548\u7684\u670d\u52a1\u5668\u5730\u5740");
        return false;
    }
    if (c.dwUserNameLength > 0 || c.dwPasswordLength > 0)
    {
        error = text(L"\u4e3a\u4fdd\u62a4\u51ed\u636e\u5b89\u5168\uff0cURL \u4e2d\u4e0d\u5141\u8bb8\u5305\u542b\u7528\u6237\u540d\u6216\u5bc6\u7801");
        return false;
    }
    out.scheme = c.nScheme;
    out.secure = c.nScheme == INTERNET_SCHEME_HTTPS;
    out.scheme_text = out.secure ? L"https" : L"http";
    out.host.assign(c.lpszHostName, c.dwHostNameLength);
    out.host = ascii_lower_copy(out.host);
    out.port = c.nPort;
    out.path_only = L"/";
    if (c.lpszUrlPath && c.dwUrlPathLength > 0)
    {
        out.path_only.assign(c.lpszUrlPath, c.dwUrlPathLength);
    }
    if (out.path_only.empty())
    {
        out.path_only = L"/";
    }
    out.path_query = out.path_only;
    if (c.lpszExtraInfo && c.dwExtraInfoLength > 0)
    {
        std::wstring extra(c.lpszExtraInfo, c.dwExtraInfoLength);
        size_t fragment = extra.find(L'#');
        if (fragment != std::wstring::npos)
        {
            extra.erase(fragment);
        }
        out.path_query += extra;
    }
    out.absolute = out.scheme_text + L"://";
    if (out.host.find(L':') != std::wstring::npos)
    {
        out.absolute += L"[" + out.host + L"]";
    }
    else
    {
        out.absolute += out.host;
    }
    bool default_port = (out.secure && out.port == INTERNET_DEFAULT_HTTPS_PORT) || (!out.secure && out.port == INTERNET_DEFAULT_HTTP_PORT);
    if (!default_port)
    {
        std::wostringstream oss;
        oss << L":" << out.port;
        out.absolute += oss.str();
    }
    out.absolute += out.path_query;
    return true;
}

inline std::wstring origin_of(const ParsedUrl& u)
{
    std::wstring result = u.scheme_text + L"://";
    if (u.host.find(L':') != std::wstring::npos)
    {
        result += L"[" + u.host + L"]";
    }
    else
    {
        result += u.host;
    }
    bool default_port = (u.secure && u.port == INTERNET_DEFAULT_HTTPS_PORT) || (!u.secure && u.port == INTERNET_DEFAULT_HTTP_PORT);
    if (!default_port)
    {
        std::wostringstream oss;
        oss << L":" << u.port;
        result += oss.str();
    }
    return result;
}

inline std::wstring remove_dot_segments(const std::wstring& path)
{
    bool leading = !path.empty() && path[0] == L'/';
    bool trailing = !path.empty() && path[path.size() - 1] == L'/';
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (start <= path.size())
    {
        size_t slash = path.find(L'/', start);
        std::wstring part = path.substr(start, slash == std::wstring::npos ? std::wstring::npos : slash - start);
        if (part == L"..")
        {
            if (!parts.empty())
            {
                parts.pop_back();
            }
        }
        else if (!part.empty() && part != L".")
        {
            parts.push_back(part);
        }
        if (slash == std::wstring::npos)
        {
            break;
        }
        start = slash + 1;
    }
    std::wstring result = leading ? L"/" : L"";
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
        {
            result += L"/";
        }
        result += parts[i];
    }
    if (trailing && !result.empty() && result[result.size() - 1] != L'/')
    {
        result += L"/";
    }
    if (result.empty())
    {
        result = leading ? L"/" : L".";
    }
    return result;
}

inline bool resolve_redirect(const ParsedUrl& base, const std::string& location_utf8, std::wstring& result, std::string& error)
{
    std::wstring location;
    if (!utf8_to_wide(trim_ascii(location_utf8), location))
    {
        error = text(L"\u91cd\u5b9a\u5411 Location \u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u6587\u672c");
        return false;
    }
    if (location.empty())
    {
        error = text(L"\u91cd\u5b9a\u5411 Location \u4e3a\u7a7a");
        return false;
    }
    size_t fragment = location.find(L'#');
    if (fragment != std::wstring::npos)
    {
        location.erase(fragment);
    }
    if (location.find(L"://") != std::wstring::npos)
    {
        result = location;
        return true;
    }
    if (location.size() >= 2 && location[0] == L'/' && location[1] == L'/')
    {
        result = base.scheme_text + L":" + location;
        return true;
    }
    std::wstring origin = origin_of(base);
    if (!location.empty() && location[0] == L'/')
    {
        size_t q = location.find(L'?');
        std::wstring p = q == std::wstring::npos ? location : location.substr(0, q);
        std::wstring query = q == std::wstring::npos ? L"" : location.substr(q);
        result = origin + remove_dot_segments(p) + query;
        return true;
    }
    if (!location.empty() && location[0] == L'?')
    {
        result = origin + base.path_only + location;
        return true;
    }
    std::wstring base_dir = base.path_only;
    size_t slash = base_dir.find_last_of(L'/');
    if (slash == std::wstring::npos)
    {
        base_dir = L"/";
    }
    else
    {
        base_dir.erase(slash + 1);
    }
    size_t q = location.find(L'?');
    std::wstring rel_path = q == std::wstring::npos ? location : location.substr(0, q);
    std::wstring query = q == std::wstring::npos ? L"" : location.substr(q);
    result = origin + remove_dot_segments(base_dir + rel_path) + query;
    return true;
}

inline uint64_t now_filetime()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return static_cast<uint64_t>(u.QuadPart);
}

inline bool parse_http_time(const std::string& value, uint64_t& out)
{
    std::wstring wide;
    if (!utf8_to_wide(value, wide))
    {
        return false;
    }
    SYSTEMTIME st = {};
    if (!WinHttpTimeToSystemTime(wide.c_str(), &st))
    {
        return false;
    }
    FILETIME ft;
    if (!SystemTimeToFileTime(&st, &ft))
    {
        return false;
    }
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    out = static_cast<uint64_t>(u.QuadPart);
    return true;
}

inline bool parse_uint64(const std::string& value, uint64_t& out)
{
    std::string s = trim_ascii(value);
    if (s.empty())
    {
        return false;
    }
    uint64_t n = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] < '0' || s[i] > '9')
        {
            return false;
        }
        uint64_t digit = static_cast<uint64_t>(s[i] - '0');
        if (n > ((std::numeric_limits<uint64_t>::max)() - digit) / 10ULL)
        {
            return false;
        }
        n = n * 10ULL + digit;
    }
    out = n;
    return true;
}

inline bool parse_int64(const std::string& value, int64_t& out)
{
    std::string s = trim_ascii(value);
    if (s.empty())
    {
        return false;
    }
    bool neg = false;
    size_t i = 0;
    if (s[0] == '+' || s[0] == '-')
    {
        neg = s[0] == '-';
        i = 1;
        if (i == s.size())
        {
            return false;
        }
    }
    uint64_t n = 0;
    for (; i < s.size(); ++i)
    {
        if (s[i] < '0' || s[i] > '9')
        {
            return false;
        }
        uint64_t digit = static_cast<uint64_t>(s[i] - '0');
        if (n > ((std::numeric_limits<uint64_t>::max)() - digit) / 10ULL)
        {
            return false;
        }
        n = n * 10ULL + digit;
    }
    if (neg)
    {
        uint64_t limit = static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()) + 1ULL;
        if (n > limit)
        {
            return false;
        }
        out = n == limit ? (std::numeric_limits<int64_t>::min)() : -static_cast<int64_t>(n);
    }
    else
    {
        if (n > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()))
        {
            return false;
        }
        out = static_cast<int64_t>(n);
    }
    return true;
}

inline std::string winhttp_error_description(DWORD code)
{
    switch (code)
    {
    case ERROR_WINHTTP_TIMEOUT: return text(L"\u8bf7\u6c42\u8d85\u65f6");
    case ERROR_WINHTTP_NAME_NOT_RESOLVED: return text(L"\u65e0\u6cd5\u89e3\u6790\u670d\u52a1\u5668\u540d\u79f0");
    case ERROR_WINHTTP_CANNOT_CONNECT: return text(L"\u65e0\u6cd5\u8fde\u63a5\u5230\u670d\u52a1\u5668");
    case ERROR_WINHTTP_CONNECTION_ERROR: return text(L"\u4e0e\u670d\u52a1\u5668\u7684\u8fde\u63a5\u53d1\u751f\u9519\u8bef");
    case ERROR_WINHTTP_SECURE_FAILURE: return text(L"HTTPS \u5b89\u5168\u8fde\u63a5\u5931\u8d25");
    case ERROR_WINHTTP_INVALID_URL: return text(L"URL \u65e0\u6548");
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME: return text(L"\u4e0d\u652f\u6301\u8be5 URL \u534f\u8bae");
    case ERROR_WINHTTP_LOGIN_FAILURE: return text(L"\u8eab\u4efd\u9a8c\u8bc1\u5931\u8d25");
    case ERROR_WINHTTP_OPERATION_CANCELLED: return text(L"\u64cd\u4f5c\u5df2\u53d6\u6d88");
    default: return text(L"WinHTTP \u8bf7\u6c42\u6267\u884c\u5931\u8d25");
    }
}

inline std::string winhttp_error_message(const wchar_t* operation, DWORD code)
{
    std::string description = winhttp_error_description(code);

    if (current_language() == "zh_CN")
    {
        std::wstring desc;
        if (!utf8_to_wide(description, desc))
        {
            desc = L"WinHTTP 请求执行失败";
        }

        std::wostringstream oss;
        oss << operation << L"失败：" << desc
            << L"（WinHTTP 错误码 " << code << L"）";
        return wide_to_utf8(oss.str());
    }
    else
    {
        std::string op = wide_to_utf8(operation ? std::wstring(operation) : std::wstring());

        if (op == "打开 WinHTTP 会话")
        {
            op = "Open WinHTTP session";
        }
        else if (op == "关闭 WinHTTP 自动 Cookie")
        {
            op = "Close WinHTTP automatic cookies";
        }
        else if (op == "发送 HTTP 请求")
        {
            op = "Send HTTP request";
        }
        else if (op == "请求")
        {
            op = "Request";
        }

        if (op.empty())
        {
            op = "HTTP request";
        }

        return op + " failed: " + description +
            " (WinHTTP error code " + std::to_string(code) + ")";
    }
}

inline bool is_retryable_http_status(int code)
{
    return code == 408 || code == 425 || code == 429 || code == 500 || code == 502 || code == 503 || code == 504;
}

inline bool is_redirect_status(int code)
{
    return code == 301 || code == 302 || code == 303 || code == 307 || code == 308;
}

inline bool is_idempotent_method(const std::wstring& method)
{
    std::wstring m = ascii_lower_copy(method);
    return m == L"get" || m == L"head" || m == L"put" || m == L"delete" || m == L"options" || m == L"trace";
}

inline bool is_retryable_winhttp_error(DWORD code)
{
    return code == ERROR_WINHTTP_TIMEOUT || code == ERROR_WINHTTP_NAME_NOT_RESOLVED || code == ERROR_WINHTTP_CANNOT_CONNECT || code == ERROR_WINHTTP_CONNECTION_ERROR || code == ERROR_WINHTTP_RESEND_REQUEST;
}

inline DWORD capped_sleep_ms(double value, int max_ms)
{
    if (value < 0.0)
    {
        value = 0.0;
    }
    double cap = max_ms < 0 ? 0.0 : static_cast<double>(max_ms);
    if (value > cap)
    {
        value = cap;
    }
    if (value > static_cast<double>((std::numeric_limits<DWORD>::max)()))
    {
        value = static_cast<double>((std::numeric_limits<DWORD>::max)());
    }
    return static_cast<DWORD>(value);
}

}

enum class ErrorCode
{
    None,
    InvalidArgument,
    InvalidUrl,
    UnsupportedScheme,
    InvalidHeader,
    EncodingError,
    SessionOpenFailed,
    ConnectionFailed,
    RequestOpenFailed,
    Timeout,
    NameResolutionFailed,
    TlsFailure,
    SendFailed,
    ReceiveFailed,
    ReadFailed,
    HttpError,
    ResponseTooLarge,
    RedirectLimitExceeded,
    RedirectError,
    FileExists,
    FileOpenFailed,
    FileWriteFailed,
    FileCommitFailed,
    Cancelled,
    WinHttpError
};

struct Headers
{
    std::unordered_map<std::string, std::string> fields;

    Headers()
    {

        }
    Headers(const Headers& other) : fields(other.fields)
    {

        }
    Headers& operator=(const Headers& other)
    {
        if (this != &other)
        {
            clear();
            fields = other.fields;
        }
        return *this;
    }
    Headers(Headers&& other) noexcept : fields(std::move(other.fields))
    {
         other.fields.clear();
    }
    Headers& operator=(Headers&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            fields = std::move(other.fields);
            other.fields.clear();
        }
        return *this;
    }
    ~Headers()
    {
        clear();
    }

    void set(const std::string& key, const std::string& value)
    {
        for (std::unordered_map<std::string, std::string>::iterator it = fields.begin(); it != fields.end(); ++it)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                detail::secure_clear(it->second);
                it->second = value;
                return;
            }
        }
        fields[key] = value;
    }

    std::string get(const std::string& key) const
    {
        for (std::unordered_map<std::string, std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                return it->second;
            }
        }
        return "";
    }

    bool contains(const std::string& key) const
    {
        for (std::unordered_map<std::string, std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                return true;
            }
        }
        return false;
    }

    void erase(const std::string& key)
    {
        for (std::unordered_map<std::string, std::string>::iterator it = fields.begin(); it != fields.end();)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                detail::secure_clear(it->second);
                it = fields.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void clear()
    {
        for (std::unordered_map<std::string, std::string>::iterator it = fields.begin(); it != fields.end(); ++it)
        {
            detail::secure_clear(it->second);
        }
        fields.clear();
    }

    std::string to_winhttp_string() const
    {
        std::string result;
        for (std::unordered_map<std::string, std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it)
        {
            result += it->first + ": " + it->second + "\r\n";
        }
        return result;
    }
};

struct ResponseHeaders
{
    std::unordered_map<std::string, std::vector<std::string> > fields;

    ResponseHeaders()
    {

        }
    ResponseHeaders(const ResponseHeaders& other) : fields(other.fields)
    {

        }
    ResponseHeaders& operator=(const ResponseHeaders& other)
    {
        if (this != &other)
        {
            clear();
            fields = other.fields;
        }
        return *this;
    }
    ResponseHeaders(ResponseHeaders&& other) noexcept : fields(std::move(other.fields))
    {
         other.fields.clear();
    }
    ResponseHeaders& operator=(ResponseHeaders&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            fields = std::move(other.fields);
            other.fields.clear();
        }
        return *this;
    }
    ~ResponseHeaders()
    {
        clear();
    }

    void add(const std::string& key, const std::string& value)
    {
        fields[detail::ascii_lower_copy(key)].push_back(value);
    }

    std::string get(const std::string& key) const
    {
        std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = fields.find(detail::ascii_lower_copy(key));
        if (it == fields.end() || it->second.empty())
        {
            return "";
        }
        return it->second.front();
    }

    std::vector<std::string> get_all(const std::string& key) const
    {
        std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = fields.find(detail::ascii_lower_copy(key));
        return it == fields.end() ? std::vector<std::string>() : it->second;
    }

    bool contains(const std::string& key) const
    {
        return fields.find(detail::ascii_lower_copy(key)) != fields.end();
    }

    void clear()
    {
        for (std::unordered_map<std::string, std::vector<std::string> >::iterator it = fields.begin(); it != fields.end(); ++it)
        {
            for (size_t i = 0; i < it->second.size(); ++i)
            {
                detail::secure_clear(it->second[i]);
            }
        }
        fields.clear();
    }
};

struct RetryPolicy
{
    int retries = 0;
    int initial_delay_ms = 250;
    int max_delay_ms = 4000;
    double multiplier = 2.0;
    bool jitter = true;
    bool respect_retry_after = true;
    bool retry_non_idempotent = false;
    bool allow_automatic_authentication = false;
};

struct RequestOptions
{
    int timeout_ms = 0;
    bool follow_redirect = true;
    int max_redirects = -1;
    size_t max_response_size = 0;
    RetryPolicy retry;
};

struct SessionOptions
{
    std::wstring user_agent = L"FeatherCrawl/2.0";
    bool enable_cookies = true;
    DWORD access_type = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    std::wstring proxy;
    std::wstring proxy_bypass;
    size_t max_response_size = 64ULL * 1024ULL * 1024ULL;
    int max_redirects = 10;
    size_t max_connections = 64;
};

struct DownloadOptions
{
    RequestOptions request;
    uint64_t max_file_size = 1024ULL * 1024ULL * 1024ULL;
    bool overwrite = false;
    bool show_progress = true;
    size_t progress_bar_width = 20;
    unsigned int progress_refresh_ms = 100;
};

struct Response
{
    int status_code = 0;
    std::string body;
    ResponseHeaders headers;
    ErrorCode error_code = ErrorCode::None;
    DWORD native_error_code = 0;
    std::string error_message = detail::text(L"\u65e0");
    size_t received_bytes = 0;
    int attempts = 0;
    int redirect_count = 0;
    std::wstring final_url;

    bool ok() const
    {
        return error_code == ErrorCode::None && status_code >= 200 && status_code < 300;
    }
};

struct DownloadResult
{
    int status_code = 0;
    ResponseHeaders headers;
    ErrorCode error_code = ErrorCode::None;
    DWORD native_error_code = 0;
    std::string error_message = detail::text(L"\u65e0");
    uint64_t bytes_written = 0;
    uint64_t file_size = 0;
    bool file_size_known = false;
    int attempts = 0;
    int redirect_count = 0;
    std::wstring final_url;
    std::wstring destination;

    bool ok() const
    {
        return error_code == ErrorCode::None && status_code >= 200 && status_code < 300;
    }
};

class WinHttpHandle
{
    HINTERNET handle_;
public:
    explicit WinHttpHandle(HINTERNET handle = NULL) : handle_(handle)
    {

        }
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
    HINTERNET release()
    {
         HINTERNET h = handle_; handle_ = NULL; return h;
    }
    void reset(HINTERNET h = NULL)
    {
         if (handle_)
    {
        WinHttpCloseHandle(handle_);
    }
     handle_ = h;
    }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    WinHttpHandle(WinHttpHandle&& other) noexcept : handle_(other.handle_)
    {
         other.handle_ = NULL;
    }
    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept
    {
        if (this != &other)
        {
            reset(other.handle_);
            other.handle_ = NULL;
        }
        return *this;
    }
    explicit operator bool() const
    {
        return handle_ != NULL;
    }
};

class FileHandle
{
    HANDLE handle_;
public:
    explicit FileHandle(HANDLE handle = INVALID_HANDLE_VALUE) : handle_(handle)
    {

    }
    ~FileHandle()
    {
        if (handle_ != INVALID_HANDLE_VALUE && handle_ != NULL)
        {
            CloseHandle(handle_);
        }

    }
    HANDLE get() const
    {
        return handle_;
    }
    void reset(HANDLE handle = INVALID_HANDLE_VALUE)
    {
        if (handle_ != INVALID_HANDLE_VALUE && handle_ != NULL)
        {
            CloseHandle(handle_);
        }
        handle_ = handle;
    }
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    explicit operator bool() const
    {
        return handle_ != INVALID_HANDLE_VALUE && handle_ != NULL;
    }
};

class CookieJar
{
    struct Cookie
    {
        std::string name;
        std::string value;
        std::wstring domain;
        std::wstring path;
        bool host_only;
        bool secure;
        bool http_only;
        bool persistent;
        uint64_t expires;
        uint64_t sequence;
    };

    mutable std::mutex mutex_;
    std::vector<Cookie> cookies_;
    uint64_t sequence_;

    static bool is_ip_host(const std::wstring& host)
    {
        if (host.find(L':') != std::wstring::npos)
        {
            return true;
        }
        if (host.empty())
        {
            return false;
        }
        for (size_t i = 0; i < host.size(); ++i)
        {
            if ((host[i] < L'0' || host[i] > L'9') && host[i] != L'.')
            {
                return false;
            }
        }
        return true;
    }

    static bool domain_matches(const std::wstring& host, const std::wstring& domain)
    {
        if (detail::ascii_iequals(host, domain))
        {
            return true;
        }
        if (is_ip_host(host) || host.size() <= domain.size())
        {
            return false;
        }
        size_t start = host.size() - domain.size();
        return start > 0 && host[start - 1] == L'.' && detail::ascii_iequals(host.substr(start), domain);
    }

    static std::wstring default_path(const std::wstring& request_path)
    {
        if (request_path.empty() || request_path[0] != L'/')
        {
            return L"/";
        }
        size_t slash = request_path.find_last_of(L'/');
        if (slash == 0 || slash == std::wstring::npos)
        {
            return L"/";
        }
        return request_path.substr(0, slash);
    }

    static bool path_matches(const std::wstring& request_path, const std::wstring& cookie_path)
    {
        if (request_path == cookie_path)
        {
            return true;
        }
        if (request_path.size() < cookie_path.size())
        {
            return false;
        }
        if (request_path.compare(0, cookie_path.size(), cookie_path) != 0)
        {
            return false;
        }
        if (!cookie_path.empty() && cookie_path[cookie_path.size() - 1] == L'/')
        {
            return true;
        }
        return request_path.size() > cookie_path.size() && request_path[cookie_path.size()] == L'/';
    }

    static bool valid_cookie_name(const std::string& name)
    {
        return detail::valid_header_name(name);
    }

    static bool valid_cookie_value(const std::string& value)
    {
        for (size_t i = 0; i < value.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(value[i]);
            if (c < 0x20 || c == 0x7F || c == ';' || c == '\r' || c == '\n' || c == '\0')
            {
                return false;
            }
        }
        return true;
    }

    static void wipe_cookie(Cookie& cookie)
    {
        detail::secure_clear(cookie.name);
        detail::secure_clear(cookie.value);
        cookie.domain.assign(cookie.domain.size(), L'\0');
        cookie.domain.clear();
        cookie.path.assign(cookie.path.size(), L'\0');
        cookie.path.clear();
        cookie.expires = 0;
        cookie.sequence = 0;
    }

    void erase_at_locked(size_t index)
    {
        wipe_cookie(cookies_[index]);
        cookies_.erase(cookies_.begin() + static_cast<std::ptrdiff_t>(index));
    }

    void remove_expired_locked(uint64_t now)
    {
        for (size_t i = cookies_.size(); i > 0; --i)
        {
            if (cookies_[i - 1].persistent && cookies_[i - 1].expires <= now)
            {
                erase_at_locked(i - 1);
            }
        }
    }

    void remove_exact_locked(const std::string& name, const std::wstring& domain, const std::wstring& path)
    {
        for (size_t i = cookies_.size(); i > 0; --i)
        {
            const Cookie& c = cookies_[i - 1];
            if (c.name == name && detail::ascii_iequals(c.domain, domain) && c.path == path)
            {
                erase_at_locked(i - 1);
            }
        }
    }

public:
    CookieJar() : sequence_(0)
    {

    }
    ~CookieJar()
    {
        clear();
    }
    CookieJar(const CookieJar&) = delete;
    CookieJar& operator=(const CookieJar&) = delete;

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < cookies_.size(); ++i)
        {
            wipe_cookie(cookies_[i]);
        }
        cookies_.clear();
        sequence_ = 0;
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        remove_expired_locked(detail::now_filetime());
        return cookies_.size();
    }

    bool delete_cookie(const std::string& name, const std::string& domain_utf8 = "", const std::string& path_utf8 = "")
    {
        std::wstring domain;
        std::wstring path;
        if (!domain_utf8.empty() && !detail::utf8_to_wide(domain_utf8, domain))
        {
            return false;
        }
        if (!path_utf8.empty() && !detail::utf8_to_wide(path_utf8, path))
        {
            return false;
        }
        domain = detail::ascii_lower_copy(domain);
        while (!domain.empty() && domain[0] == L'.')
        {
            domain.erase(domain.begin());
        }
        std::lock_guard<std::mutex> lock(mutex_);
        bool removed = false;
        for (size_t i = cookies_.size(); i > 0; --i)
        {
            const Cookie& c = cookies_[i - 1];
            if (c.name != name)
            {
                continue;
            }
            if (!domain.empty() && !detail::ascii_iequals(c.domain, domain))
            {
                continue;
            }
            if (!path.empty() && c.path != path)
            {
                continue;
            }
            erase_at_locked(i - 1);
            removed = true;
        }
        return removed;
    }

    size_t clear_domain(const std::string& domain_utf8)
    {
        std::wstring domain;
        if (!detail::utf8_to_wide(domain_utf8, domain))
        {
            return 0;
        }
        domain = detail::ascii_lower_copy(domain);
        while (!domain.empty() && domain[0] == L'.')
        {
            domain.erase(domain.begin());
        }
        std::lock_guard<std::mutex> lock(mutex_);
        size_t removed = 0;
        for (size_t i = cookies_.size(); i > 0; --i)
        {
            if (detail::ascii_iequals(cookies_[i - 1].domain, domain))
            {
                erase_at_locked(i - 1);
                ++removed;
            }
        }
        return removed;
    }

    void store(const detail::ParsedUrl& origin, const std::string& set_cookie)
    {
        if (set_cookie.empty() || set_cookie.size() > 4096)
        {
            return;
        }
        std::vector<std::string> parts;
        size_t start = 0;
        while (start <= set_cookie.size())
        {
            size_t semi = set_cookie.find(';', start);
            parts.push_back(detail::trim_ascii(set_cookie.substr(start, semi == std::string::npos ? std::string::npos : semi - start)));
            if (semi == std::string::npos)
            {
                break;
            }
            start = semi + 1;
        }
        if (parts.empty())
        {
            return;
        }
        size_t eq = parts[0].find('=');
        if (eq == std::string::npos || eq == 0)
        {
            return;
        }
        Cookie cookie;
        cookie.name = detail::trim_ascii(parts[0].substr(0, eq));
        cookie.value = detail::trim_ascii(parts[0].substr(eq + 1));
        if (!valid_cookie_name(cookie.name) || !valid_cookie_value(cookie.value))
        {
            return;
        }
        cookie.domain = origin.host;
        cookie.path = default_path(origin.path_only);
        cookie.host_only = true;
        cookie.secure = false;
        cookie.http_only = false;
        cookie.persistent = false;
        cookie.expires = 0;
        cookie.sequence = 0;
        bool has_domain = false;
        bool has_path = false;
        bool has_max_age = false;
        int64_t max_age = 0;
        for (size_t i = 1; i < parts.size(); ++i)
        {
            if (parts[i].empty())
            {
                continue;
            }
            size_t aeq = parts[i].find('=');
            std::string name = detail::ascii_lower_copy(detail::trim_ascii(parts[i].substr(0, aeq)));
            std::string value = aeq == std::string::npos ? "" : detail::trim_ascii(parts[i].substr(aeq + 1));
            if (name == "secure")
            {
                cookie.secure = true;
            }
            else if (name == "httponly")
            {
                cookie.http_only = true;
            }
            else if (name == "domain" && !value.empty())
            {
                std::wstring d;
                if (!detail::utf8_to_wide(value, d))
                {
                    return;
                }
                d = detail::ascii_lower_copy(detail::trim_ascii(d));
                while (!d.empty() && d[0] == L'.')
                {
                    d.erase(d.begin());
                }
                if (d.empty() || !domain_matches(origin.host, d))
                {
                    return;
                }
                cookie.domain = d;
                cookie.host_only = false;
                has_domain = true;
            }
            else if (name == "path" && !value.empty())
            {
                std::wstring p;
                if (detail::utf8_to_wide(value, p) && !p.empty() && p[0] == L'/')
                {
                    cookie.path = p;
                    has_path = true;
                }
            }
            else if (name == "max-age")
            {
                int64_t v = 0;
                if (detail::parse_int64(value, v))
                {
                    has_max_age = true;
                    max_age = v;
                }
            }
            else if (name == "expires")
            {
                uint64_t expires = 0;
                if (!has_max_age && detail::parse_http_time(value, expires))
                {
                    cookie.persistent = true;
                    cookie.expires = expires;
                }
            }
        }
        if (has_max_age)
        {
            cookie.persistent = true;
            if (max_age <= 0)
            {
                cookie.expires = 0;
            }
            else
            {
                uint64_t now = detail::now_filetime();
                uint64_t delta = static_cast<uint64_t>(max_age) > ((std::numeric_limits<uint64_t>::max)() / 10000000ULL) ? (std::numeric_limits<uint64_t>::max)() : static_cast<uint64_t>(max_age) * 10000000ULL;
                cookie.expires = delta > (std::numeric_limits<uint64_t>::max)() - now ? (std::numeric_limits<uint64_t>::max)() : now + delta;
            }
        }
        if (cookie.secure && !origin.secure)
        {
            return;
        }
        if (cookie.name.find("__Secure-") == 0 && (!cookie.secure || !origin.secure))
        {
            return;
        }
        if (cookie.name.find("__Host-") == 0 && (!cookie.secure || !origin.secure || has_domain || cookie.path != L"/" || !has_path))
        {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        remove_expired_locked(detail::now_filetime());
        remove_exact_locked(cookie.name, cookie.domain, cookie.path);
        if (cookie.persistent && cookie.expires <= detail::now_filetime())
        {
            return;
        }
        size_t same_domain = 0;
        size_t oldest_domain_index = 0;
        uint64_t oldest_domain_sequence = (std::numeric_limits<uint64_t>::max)();
        for (size_t i = 0; i < cookies_.size(); ++i)
        {
            if (detail::ascii_iequals(cookies_[i].domain, cookie.domain))
            {
                ++same_domain;
                if (cookies_[i].sequence < oldest_domain_sequence)
                {
                    oldest_domain_sequence = cookies_[i].sequence;
                    oldest_domain_index = i;
                }
            }
        }
        if (same_domain >= 180 && !cookies_.empty())
        {
            erase_at_locked(oldest_domain_index);
        }
        if (cookies_.size() >= 3000)
        {
            erase_at_locked(0);
        }
        cookie.sequence = ++sequence_;
        cookies_.push_back(cookie);
        wipe_cookie(cookie);
    }

    std::string header_for(const detail::ParsedUrl& target)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        remove_expired_locked(detail::now_filetime());
        std::vector<const Cookie*> matches;
        for (size_t i = 0; i < cookies_.size(); ++i)
        {
            const Cookie& c = cookies_[i];
            bool domain_ok = c.host_only ? detail::ascii_iequals(target.host, c.domain) : domain_matches(target.host, c.domain);
            if (!domain_ok || !path_matches(target.path_only, c.path) || (c.secure && !target.secure))
            {
                continue;
            }
            matches.push_back(&c);
        }
        std::sort(matches.begin(), matches.end(), [](const Cookie* a, const Cookie* b)
        {
            if (a->path.size() != b->path.size())
            {
                return a->path.size() > b->path.size();
            }
            return a->sequence < b->sequence;
        });
        std::string result;
        for (size_t i = 0; i < matches.size(); ++i)
        {
            std::string pair = matches[i]->name + "=" + matches[i]->value;
            detail::SecureStringGuard pair_guard(pair);
            size_t extra = pair.size() + (result.empty() ? 0 : 2);
            if (extra > 65536 || result.size() > 65536 - extra)
            {
                break;
            }
            if (!result.empty())
            {
                result += "; ";
            }
            result += pair;
        }
        return result;
    }
};

namespace detail
{

inline bool validate_headers(const Headers& headers, std::string& error)
{
    for (std::unordered_map<std::string, std::string>::const_iterator it = headers.fields.begin(); it != headers.fields.end(); ++it)
    {
        if (!valid_header_name(it->first))
        {
            error = text(L"\u65e0\u6548\u7684 HTTP \u8bf7\u6c42\u5934\u540d\u79f0\uff1a") + it->first;
            return false;
        }
        if (!valid_header_value(it->second))
        {
            error = text(L"HTTP \u8bf7\u6c42\u5934\u7684\u503c\u65e0\u6548\uff1a") + it->first;
            return false;
        }
    }
    return true;
}

inline bool query_raw_headers(HINTERNET request, ResponseHeaders& headers)
{
    headers.clear();
    DWORD size = 0;
    WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &size, WINHTTP_NO_HEADER_INDEX);
    DWORD e = GetLastError();
    if (e != ERROR_INSUFFICIENT_BUFFER || size == 0)
    {
        return false;
    }
    std::vector<wchar_t> buffer(static_cast<size_t>(size / sizeof(wchar_t)) + 2, L'\0');
    DWORD actual = size;
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, buffer.data(), &actual, WINHTTP_NO_HEADER_INDEX))
    {
        return false;
    }
    std::wstring raw(buffer.data());
    size_t start = 0;
    bool first = true;
    while (start <= raw.size())
    {
        size_t end = raw.find(L"\r\n", start);
        std::wstring line = raw.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
        if (!first && !line.empty())
        {
            size_t colon = line.find(L':');
            if (colon != std::wstring::npos && colon > 0)
            {
                std::wstring name = trim_ascii(line.substr(0, colon));
                std::wstring value = trim_ascii(line.substr(colon + 1));
                headers.add(wide_to_utf8(name), wide_to_utf8(value));
            }
        }
        first = false;
        if (end == std::wstring::npos)
        {
            break;
        }
        start = end + 2;
    }
    return true;
}

inline ErrorCode map_winhttp_error(DWORD code, ErrorCode fallback)
{
    if (code == ERROR_WINHTTP_TIMEOUT)
    {
        return ErrorCode::Timeout;
    }
    if (code == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        return ErrorCode::NameResolutionFailed;
    }
    if (code == ERROR_WINHTTP_CANNOT_CONNECT || code == ERROR_WINHTTP_CONNECTION_ERROR)
    {
        return ErrorCode::ConnectionFailed;
    }
    if (code == ERROR_WINHTTP_SECURE_FAILURE)
    {
        return ErrorCode::TlsFailure;
    }
    if (code == ERROR_WINHTTP_OPERATION_CANCELLED)
    {
        return ErrorCode::Cancelled;
    }
    return fallback;
}

inline int retry_after_ms(const ResponseHeaders& headers, int max_ms)
{
    std::string value = trim_ascii(headers.get("Retry-After"));
    if (value.empty())
    {
        return -1;
    }
    uint64_t seconds = 0;
    if (parse_uint64(value, seconds))
    {
        uint64_t ms = seconds > (std::numeric_limits<uint64_t>::max)() / 1000ULL ? (std::numeric_limits<uint64_t>::max)() : seconds * 1000ULL;
        if (ms > static_cast<uint64_t>(max_ms < 0 ? 0 : max_ms))
        {
            ms = static_cast<uint64_t>(max_ms < 0 ? 0 : max_ms);
        }
        return static_cast<int>(ms);
    }
    uint64_t when = 0;
    if (parse_http_time(value, when))
    {
        uint64_t now = now_filetime();
        if (when <= now)
        {
            return 0;
        }
        uint64_t ms = (when - now) / 10000ULL;
        if (ms > static_cast<uint64_t>(max_ms < 0 ? 0 : max_ms))
        {
            ms = static_cast<uint64_t>(max_ms < 0 ? 0 : max_ms);
        }
        return static_cast<int>(ms);
    }
    return -1;
}

inline unsigned int retry_jitter_value(int retry_index)
{
    unsigned int x = static_cast<unsigned int>(GetTickCount());
    x ^= static_cast<unsigned int>(GetCurrentThreadId()) * 0x9E3779B9u;
    x ^= static_cast<unsigned int>(retry_index + 1) * 0x85EBCA6Bu;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

inline void sleep_retry(const RetryPolicy& policy, int retry_index, const ResponseHeaders* headers)
{
    int explicit_delay = -1;
    if (policy.respect_retry_after && headers)
    {
        explicit_delay = retry_after_ms(*headers, policy.max_delay_ms);
    }
    double delay = explicit_delay >= 0 ? static_cast<double>(explicit_delay) : static_cast<double>(policy.initial_delay_ms) * std::pow(policy.multiplier > 0.0 ? policy.multiplier : 1.0, static_cast<double>(retry_index));
    if (explicit_delay < 0 && policy.jitter && delay > 1.0)
    {
        double factor = 0.85 + static_cast<double>(retry_jitter_value(retry_index) % 301u) / 1000.0;
        delay *= factor;
    }
    Sleep(capped_sleep_ms(delay, policy.max_delay_ms));
}

inline std::string http_error_message(int status)
{
    std::ostringstream oss;
    oss << text(L"HTTP \u8bf7\u6c42\u5931\u8d25\uff0c\u72b6\u6001\u7801 ") << status;
    return oss.str();
}

inline std::string attempts_suffix(int attempts)
{
    if (attempts <= 1)
    {
        return "";
    }
    std::ostringstream oss;
    oss << text(L"\uff0c\u5df2\u5c1d\u8bd5 ") << attempts << text(L" \u6b21");
    return oss.str();
}

inline bool same_origin(const ParsedUrl& a, const ParsedUrl& b)
{
    return a.scheme == b.scheme && a.port == b.port && ascii_iequals(a.host, b.host);
}

inline std::wstring connection_key(const ParsedUrl& u)
{
    std::wostringstream oss;
    oss << u.scheme_text << L"|" << u.host << L"|" << u.port;
    return oss.str();
}

inline std::wstring format_download_bytes(uint64_t bytes)
{
    static const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    double value = static_cast<double>(bytes);
    size_t unit = 0;
    while (value >= 1024.0 && unit < 4)
    {
        value /= 1024.0;
        ++unit;
    }
    int precision = value >= 100.0 ? 0 : (value >= 10.0 ? 1 : 2);
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value << L" " << units[unit];
    return oss.str();
}

inline std::wstring format_download_seconds(double seconds)
{
    if (!(seconds >= 0.0) || seconds > 365.0 * 24.0 * 60.0 * 60.0)
    {
        return L"--:--";
    }
    uint64_t total = static_cast<uint64_t>(seconds + 0.5);
    uint64_t hours = total / 3600ULL;
    uint64_t minutes = (total % 3600ULL) / 60ULL;
    uint64_t secs = total % 60ULL;
    std::wostringstream oss;
    oss << std::setfill(L'0');
    if (hours > 0)
    {
        oss << std::setw(2) << hours << L":" << std::setw(2) << minutes << L":" << std::setw(2) << secs;
    }
    else
    {
        oss << std::setw(2) << minutes << L":" << std::setw(2) << secs;
    }
    return oss.str();
}

class DownloadProgressDisplay
{
    bool enabled_;
    size_t bar_width_;
    unsigned int refresh_ms_;
    bool active_;
    bool total_known_;
    uint64_t total_;
    uint64_t last_bytes_;
    std::chrono::steady_clock::time_point started_;
    std::chrono::steady_clock::time_point last_render_;
    HANDLE output_;
    bool console_;
    bool initialized_;
    COORD origin_;
    COORD after_;

    std::vector<std::wstring> make_lines(uint64_t bytes) const
    {
        bool zh = current_language() == "zh_CN";
        double fraction = 0.0;
        if (total_known_ && total_ > 0)
        {
            fraction = static_cast<double>((std::min)(bytes, total_)) / static_cast<double>(total_);
        }
        else if (total_known_ && total_ == 0)
        {
            fraction = 1.0;
        }
        if (fraction < 0.0)
        {
            fraction = 0.0;
        }
        if (fraction > 1.0)
        {
            fraction = 1.0;
        }

        size_t filled = total_known_ ? static_cast<size_t>(fraction * static_cast<double>(bar_width_) + 0.5) : 0;
        if (filled > bar_width_)
        {
            filled = bar_width_;
        }
        std::wstring bar = L"[";
        bar.append(filled, L'#');
        bar.append(bar_width_ - filled, L'-');
        bar += L"] ";
        if (total_known_)
        {
            int percent = static_cast<int>(fraction * 100.0 + 0.5);
            if (percent > 100)
            {
                percent = 100;
            }
            std::wostringstream ps;
            ps << percent << L"%";
            bar += ps.str();
        }
        else
        {
            bar += L"--%";
        }

        std::wstring size_line = format_download_bytes(bytes) + L" / " +
            (total_known_ ? format_download_bytes(total_) : (zh ? L"未知" : L"Unknown"));

        std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started_;
        double seconds = elapsed.count();
        double speed = seconds > 0.001 ? static_cast<double>(bytes) / seconds : 0.0;
        uint64_t speed_bytes = speed > 0.0 ? static_cast<uint64_t>(speed + 0.5) : 0;
        std::wstring speed_line = (zh ? L"下载速度 " : L"Download speed ") + format_download_bytes(speed_bytes) + L"/s";

        std::wstring eta = L"--:--";
        if (total_known_ && bytes >= total_)
        {
            eta = L"00:00";
        }
        else if (total_known_ && speed > 0.0 && total_ > bytes)
        {
            eta = format_download_seconds(static_cast<double>(total_ - bytes) / speed);
        }
        std::wstring eta_line = (zh ? L"剩余时间 " : L"ETA ") + eta;

        std::vector<std::wstring> lines;
        lines.push_back(bar);
        lines.push_back(size_line);
        lines.push_back(speed_line);
        lines.push_back(eta_line);
        return lines;
    }

    static void write_console_text(HANDLE output, const std::wstring& value)
    {
        if (value.empty())
        {
            return;
        }
        DWORD written = 0;
        WriteConsoleW(output, value.c_str(), static_cast<DWORD>(value.size()), &written, NULL);
    }

    void render_console(const std::vector<std::wstring>& lines)
    {
        CONSOLE_SCREEN_BUFFER_INFO info = {};
        if (!GetConsoleScreenBufferInfo(output_, &info))
        {
            return;
        }
        if (!initialized_)
        {
            if (info.dwCursorPosition.X != 0)
            {
                write_console_text(output_, L"\r\n");
            }
            for (size_t i = 0; i < lines.size(); ++i)
            {
                std::wstring line = lines[i];
                if (info.dwSize.X > 1 && line.size() > static_cast<size_t>(info.dwSize.X - 1))
                {
                    line.resize(static_cast<size_t>(info.dwSize.X - 1));
                }
                write_console_text(output_, line);
                write_console_text(output_, L"\r\n");
            }
            if (GetConsoleScreenBufferInfo(output_, &info))
            {
                after_ = info.dwCursorPosition;
                origin_.X = 0;
                origin_.Y = static_cast<SHORT>((std::max)(0, static_cast<int>(after_.Y) - static_cast<int>(lines.size())));
                initialized_ = true;
            }
            return;
        }

        SHORT width = info.dwSize.X;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            COORD pos = origin_;
            pos.Y = static_cast<SHORT>(origin_.Y + static_cast<SHORT>(i));
            if (pos.Y < 0 || pos.Y >= info.dwSize.Y)
            {
                continue;
            }
            DWORD cleared = 0;
            if (width > 1)
            {
                FillConsoleOutputCharacterW(output_, L' ', static_cast<DWORD>(width - 1), pos, &cleared);
            }
            SetConsoleCursorPosition(output_, pos);
            std::wstring line = lines[i];
            if (width > 1 && line.size() > static_cast<size_t>(width - 1))
            {
                line.resize(static_cast<size_t>(width - 1));
            }
            write_console_text(output_, line);
        }
        SetConsoleCursorPosition(output_, after_);
    }

    void render_stream(const std::vector<std::wstring>& lines, bool final)
    {
        if (!final)
        {
            return;
        }
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::cout << wide_to_utf8(lines[i]) << "\n";
        }
        std::cout.flush();
    }

    void render(uint64_t bytes, bool final, bool force)
    {
        if (!active_)
        {
            return;
        }
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (!force && refresh_ms_ > 0)
        {
            std::chrono::duration<double, std::milli> elapsed = now - last_render_;
            if (elapsed.count() < static_cast<double>(refresh_ms_))
            {
                return;
            }
        }
        last_render_ = now;
        last_bytes_ = bytes;
        std::vector<std::wstring> lines = make_lines(bytes);
        if (console_)
        {
            render_console(lines);
        }
        else
        {
            render_stream(lines, final);
        }
    }

public:
    DownloadProgressDisplay(bool enabled = false, size_t bar_width = 20, unsigned int refresh_ms = 100)
        : enabled_(enabled), bar_width_(bar_width), refresh_ms_(refresh_ms), active_(false), total_known_(false), total_(0), last_bytes_(0),
          output_(GetStdHandle(STD_OUTPUT_HANDLE)), console_(false), initialized_(false)
    {
        if (bar_width_ < 5)
        {
            bar_width_ = 5;
        }
        if (bar_width_ > 100)
        {
            bar_width_ = 100;
        }
        DWORD mode = 0;
        console_ = output_ != NULL && output_ != INVALID_HANDLE_VALUE && GetConsoleMode(output_, &mode) != 0;
        origin_.X = origin_.Y = 0;
        after_.X = after_.Y = 0;
    }

    void reset()
    {
        active_ = false;
        total_known_ = false;
        total_ = 0;
        last_bytes_ = 0;
    }

    void begin(int status_code, bool total_known, uint64_t total)
    {
        reset();
        if (!enabled_ || status_code < 200 || status_code >= 300 || status_code == 204)
        {
            return;
        }
        active_ = true;
        total_known_ = total_known;
        total_ = total;
        started_ = std::chrono::steady_clock::now();
        last_render_ = started_;
        render(0, false, true);
    }

    void update(uint64_t bytes)
    {
        render(bytes, false, false);
    }

    void end(uint64_t bytes)
    {
        render(bytes, true, true);
        active_ = false;
    }
};

 }

inline void set_default_language(const std::string& lang)
{
    detail::set_default_language(lang);
}

inline void set_language(const std::string& lang)
{
    detail::set_default_language(lang);
}

class MemorySink
{
    std::string data_;
    size_t limit_;
    bool too_large_;
public:
    explicit MemorySink(size_t limit) : limit_(limit), too_large_(false)
    {

        }
    bool reset()
    {
        data_.clear();
        too_large_ = false;
        return true;
    }
    bool reserve(uint64_t size)
    {
        if (limit_ > 0 && size > static_cast<uint64_t>(limit_))
        {
             too_large_ = true; return false;
        }
        if (size <= static_cast<uint64_t>((std::numeric_limits<size_t>::max)()))
        {
            data_.reserve(static_cast<size_t>(size));
        }
        return true;
    }
    bool write(const char* data, size_t size)
    {
        if (limit_ > 0 && (data_.size() > limit_ || size > limit_ - data_.size()))
        {
            too_large_ = true; return false;
        }
        data_.append(data, size);
        return true;
    }
    bool too_large() const
    {
        return too_large_;
    }
    const std::string& data() const
    {
        return data_;
    }
    uint64_t size() const
    {
        return static_cast<uint64_t>(data_.size());
    }
    void begin_response(int, bool, uint64_t)
    {
    }
    void end_response()
    {
    }
};

class FileSink
{
    std::wstring destination_;
    std::wstring temp_;
    uint64_t limit_;
    bool overwrite_;
    bool too_large_;
    uint64_t written_;
    FileHandle file_;
    bool committed_;
    detail::DownloadProgressDisplay progress_;

    static bool exists(const std::wstring& path)
    {
        DWORD attr = GetFileAttributesW(path.c_str());
        return attr != INVALID_FILE_ATTRIBUTES;
    }

public:
    FileSink(const std::wstring& destination, uint64_t limit, bool overwrite) : destination_(destination), limit_(limit), overwrite_(overwrite), too_large_(false), written_(0), committed_(false), progress_(false, 20, 100)
    {
    }

    FileSink(const std::wstring& destination, uint64_t limit, bool overwrite, bool show_progress, size_t progress_bar_width, unsigned int progress_refresh_ms)
        : destination_(destination), limit_(limit), overwrite_(overwrite), too_large_(false), written_(0), committed_(false), progress_(show_progress, progress_bar_width, progress_refresh_ms)
    {
    }

    ~FileSink()
    {
        file_.reset();
        if (!committed_ && !temp_.empty())
        {
            DeleteFileW(temp_.c_str());
        }
    }

    bool destination_exists() const
    {
        return exists(destination_);
    }

    bool reset()
    {
        static volatile LONG sequence = 0;
        file_.reset();
        if (!temp_.empty())
        {
            DeleteFileW(temp_.c_str());
        }
        temp_.clear();
        written_ = 0;
        too_large_ = false;
        progress_.reset();
        for (int attempt = 0; attempt < 64; ++attempt)
        {
            LONG value = InterlockedIncrement(&sequence);
            std::wostringstream oss;
            oss << destination_ << L".feathercrawl." << GetCurrentProcessId() << L"." << static_cast<unsigned long>(GetTickCount()) << L"." << static_cast<long>(value) << L".part";
            std::wstring candidate = oss.str();
            HANDLE h = CreateFileW(candidate.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h != INVALID_HANDLE_VALUE && h != NULL)
            {
                temp_ = candidate;
                file_.reset(h);
                return true;
            }
            DWORD error = GetLastError();
            if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
            {
                return false;
            }
        }
        return false;
    }

    bool reserve(uint64_t size)
    {
        if (limit_ > 0 && size > limit_)
        {
             too_large_ = true; return false;
        }
        return true;
    }

    bool write(const char* data, size_t size)
    {
        if (!file_)
        {
            return false;
        }
        if (limit_ > 0 && (written_ > limit_ || static_cast<uint64_t>(size) > limit_ - written_))
        {
             too_large_ = true; return false;
        }
        size_t offset = 0;
        while (offset < size)
        {
            DWORD chunk = static_cast<DWORD>((std::min)(size - offset, static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
            DWORD done = 0;
            if (!WriteFile(file_.get(), data + offset, chunk, &done, NULL) || done != chunk)
            {
                return false;
            }
            offset += done;
            written_ += done;
        }
        progress_.update(written_);
        return true;
    }

    void begin_response(int status_code, bool total_known, uint64_t total)
    {
        progress_.begin(status_code, total_known, total);
    }

    void end_response()
    {
        progress_.end(written_);
    }

    bool too_large() const
    {
        return too_large_;
    }
    uint64_t size() const
    {
        return written_;
    }

    bool commit()
    {
        if (!file_)
        {
            return false;
        }
        if (!FlushFileBuffers(file_.get()))
        {
            return false;
        }
        file_.reset();
        if (!overwrite_ && exists(destination_))
        {
            return false;
        }
        DWORD flags = MOVEFILE_WRITE_THROUGH;
        if (overwrite_)
        {
            flags |= MOVEFILE_REPLACE_EXISTING;
        }
        if (!MoveFileExW(temp_.c_str(), destination_.c_str(), flags))
        {
            return false;
        }
        committed_ = true;
        return true;
    }
};

inline std::wstring suggested_download_filename(const std::wstring& url)
{
    detail::ParsedUrl parsed;
    std::string error;
    std::wstring name;
    if (detail::parse_url(url, parsed, error))
    {
        size_t slash = parsed.path_only.find_last_of(L'/');
        name = slash == std::wstring::npos ? parsed.path_only : parsed.path_only.substr(slash + 1);
    }
    if (name.empty())
    {
        name = L"download.bin";
    }
    const wchar_t* invalid = L"<>:\\|?*\"/";
    for (size_t i = 0; i < name.size(); ++i)
    {
        if (name[i] < 32 || std::wcschr(invalid, name[i]) != NULL)
        {
            name[i] = L'_';
        }
    }
    if (name == L"." || name == L"..")
    {
        name = L"download.bin";
    }
    return name;
}

class Session
{
    SessionOptions options_;
    WinHttpHandle session_;
    std::unordered_map<std::wstring, std::shared_ptr<WinHttpHandle> > connections_;
    std::mutex connection_mutex_;
    CookieJar cookies_;
    mutable std::mutex option_mutex_;
    bool cookies_enabled_;
    std::string init_error_;
    DWORD init_native_error_;
    std::string language_;

    bool ensure_session()
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        if (session_)
        {
            return true;
        }
        HINTERNET h = WinHttpOpen(options_.user_agent.c_str(), options_.proxy.empty() ? options_.access_type : WINHTTP_ACCESS_TYPE_NAMED_PROXY, options_.proxy.empty() ? WINHTTP_NO_PROXY_NAME : options_.proxy.c_str(), options_.proxy_bypass.empty() ? WINHTTP_NO_PROXY_BYPASS : options_.proxy_bypass.c_str(), 0);
        if (!h)
        {
            init_native_error_ = GetLastError();
            init_error_ = detail::winhttp_error_message(L"\u6253\u5f00 WinHTTP \u4f1a\u8bdd", init_native_error_);
            return false;
        }
        session_.reset(h);
#ifdef WINHTTP_OPTION_DISABLE_GLOBAL_POOLING
        WinHttpSetOption(session_.get(), WINHTTP_OPTION_DISABLE_GLOBAL_POOLING, NULL, 0);
#endif
        init_error_.clear();
        init_native_error_ = 0;
        return true;
    }

    std::shared_ptr<WinHttpHandle> connection_for(const detail::ParsedUrl& url, std::string& error, DWORD& native)
    {
        if (!ensure_session())
        {
            std::lock_guard<std::mutex> lock(connection_mutex_);
            error = init_error_;
            native = init_native_error_;
            return std::shared_ptr<WinHttpHandle>();
        }
        std::wstring key = detail::connection_key(url);
        std::lock_guard<std::mutex> lock(connection_mutex_);
        std::unordered_map<std::wstring, std::shared_ptr<WinHttpHandle> >::iterator it = connections_.find(key);
        if (it != connections_.end())
        {
            return it->second;
        }
        HINTERNET h = WinHttpConnect(session_.get(), url.host.c_str(), url.port, 0);
        if (!h)
        {
            native = GetLastError();
            error = detail::winhttp_error_message(L"\u8fde\u63a5\u670d\u52a1\u5668", native);
            return std::shared_ptr<WinHttpHandle>();
        }
        std::shared_ptr<WinHttpHandle> handle(new WinHttpHandle(h));
        if (options_.max_connections > 0)
        {
            if (connections_.size() >= options_.max_connections && !connections_.empty())
            {
                connections_.erase(connections_.begin());
            }
            connections_[key] = handle;
        }
        return handle;
    }

    static bool validate_options(const RequestOptions& o, std::string& error)
    {
        if (o.timeout_ms < 0)
        {
            error = detail::text(L"\u8d85\u65f6\u65f6\u95f4\u4e0d\u80fd\u5c0f\u4e8e 0"); return false;
        }
        if (o.max_redirects < 0)
        {
            error = detail::text(L"\u6700\u5927\u91cd\u5b9a\u5411\u6b21\u6570\u4e0d\u80fd\u5c0f\u4e8e 0"); return false;
        }
        if (o.retry.retries < 0)
        {
            error = detail::text(L"\u91cd\u8bd5\u6b21\u6570\u4e0d\u80fd\u5c0f\u4e8e 0"); return false;
        }
        if (o.retry.retries > 100)
        {
            error = detail::text(L"\u91cd\u8bd5\u6b21\u6570\u4e0d\u80fd\u5927\u4e8e 100"); return false;
        }
        if (o.max_redirects > 100)
        {
            error = detail::text(L"\u6700\u5927\u91cd\u5b9a\u5411\u6b21\u6570\u4e0d\u80fd\u5927\u4e8e 100"); return false;
        }
        if (o.retry.initial_delay_ms < 0 || o.retry.max_delay_ms < 0)
        {
            error = detail::text(L"\u91cd\u8bd5\u5ef6\u8fdf\u65f6\u95f4\u4e0d\u80fd\u5c0f\u4e8e 0"); return false;
        }
        if (o.retry.multiplier <= 0.0)
        {
            error = detail::text(L"\u91cd\u8bd5\u500d\u7387\u5fc5\u987b\u5927\u4e8e 0"); return false;
        }
        return true;
    }

    template <class Sink>
    Response send_once(const std::wstring& method, const detail::ParsedUrl& url, const std::string& body, const Headers& headers, const RequestOptions& options, Sink& sink)
    {
        Response r;
        r.final_url = url.absolute;
        std::string error;
        DWORD native = 0;
        std::shared_ptr<WinHttpHandle> connect = connection_for(url, error, native);
        if (!connect)
        {
            r.error_code = detail::map_winhttp_error(native, session_ ? ErrorCode::ConnectionFailed : ErrorCode::SessionOpenFailed);
            r.native_error_code = native;
            r.error_message = error;
            return r;
        }
        DWORD flags = url.secure ? WINHTTP_FLAG_SECURE : 0;
        WinHttpHandle request(WinHttpOpenRequest(connect->get(), method.c_str(), url.path_query.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
        if (!request)
        {
            native = GetLastError();
            r.error_code = detail::map_winhttp_error(native, ErrorCode::RequestOpenFailed);
            r.native_error_code = native;
            r.error_message = detail::winhttp_error_message(L"\u521b\u5efa HTTP \u8bf7\u6c42", native);
            return r;
        }
        if (options.timeout_ms > 0 && !WinHttpSetTimeouts(request.get(), options.timeout_ms, options.timeout_ms, options.timeout_ms, options.timeout_ms))
        {
            native = GetLastError();
            r.error_code = detail::map_winhttp_error(native, ErrorCode::WinHttpError);
            r.native_error_code = native;
            r.error_message = detail::winhttp_error_message(L"\u8bbe\u7f6e\u8d85\u65f6\u65f6\u95f4", native);
            return r;
        }
        DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)))
        {
            native = GetLastError();
            r.error_code = ErrorCode::WinHttpError;
            r.native_error_code = native;
            r.error_message = detail::winhttp_error_message(L"\u8bbe\u7f6e\u91cd\u5b9a\u5411\u7b56\u7565", native);
            return r;
        }
        DWORD disable = WINHTTP_DISABLE_COOKIES;
#ifdef WINHTTP_DISABLE_AUTHENTICATION
        if (!options.retry.allow_automatic_authentication)
        {
            disable |= WINHTTP_DISABLE_AUTHENTICATION;
        }
#endif
        if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_DISABLE_FEATURE, &disable, sizeof(disable)))
        {
            native = GetLastError();
            r.error_code = ErrorCode::WinHttpError;
            r.native_error_code = native;
            r.error_message = detail::winhttp_error_message(L"\u5173\u95ed WinHTTP \u81ea\u52a8 Cookie", native);
            return r;
        }
        Headers actual = headers;
        bool cookies_enabled;
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            cookies_enabled = cookies_enabled_;
        }
        if (cookies_enabled && !actual.contains("Cookie"))
        {
            std::string cookie = cookies_.header_for(url);
            detail::SecureStringGuard cookie_guard(cookie);
            if (!cookie.empty())
            {
                actual.set("Cookie", cookie);
            }
        }
        std::string header_error;
        if (!detail::validate_headers(actual, header_error))
        {
            r.error_code = ErrorCode::InvalidHeader;
            r.error_message = header_error;
            return r;
        }
        std::string hs = actual.to_winhttp_string();
        detail::SecureStringGuard header_guard(hs);
        if (!hs.empty())
        {
            std::wstring wh;
            if (!detail::utf8_to_wide(hs, wh))
            {
                r.error_code = ErrorCode::EncodingError;
                r.error_message = detail::text(L"HTTP \u8bf7\u6c42\u5934\u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801");
                return r;
            }
            if (wh.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
            {
                r.error_code = ErrorCode::InvalidHeader;
                r.error_message = detail::text(L"HTTP \u8bf7\u6c42\u5934\u8fc7\u5927");
                return r;
            }
            if (!WinHttpAddRequestHeaders(request.get(), wh.c_str(), static_cast<DWORD>(wh.size()), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE))
            {
                native = GetLastError();
                r.error_code = detail::map_winhttp_error(native, ErrorCode::WinHttpError);
                r.native_error_code = native;
                r.error_message = detail::winhttp_error_message(L"\u6dfb\u52a0 HTTP \u8bf7\u6c42\u5934", native);
                return r;
            }
        }
        if (body.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
        {
            r.error_code = ErrorCode::InvalidArgument;
            r.error_message = detail::text(L"\u8bf7\u6c42\u4f53\u8fc7\u5927\uff0c\u8d85\u51fa\u5f53\u524d WinHTTP \u63a5\u53e3\u53ef\u5904\u7406\u8303\u56f4");
            return r;
        }
        DWORD body_size = static_cast<DWORD>(body.size());
        LPVOID data = body_size == 0 ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data());
        if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, data, body_size, body_size, 0))
        {
            native = GetLastError();
            r.error_code = detail::map_winhttp_error(native, ErrorCode::SendFailed);
            r.native_error_code = native;
            r.error_message = detail::winhttp_error_message(L"\u53d1\u9001 HTTP \u8bf7\u6c42", native);
            return r;
        }
        if (!WinHttpReceiveResponse(request.get(), NULL))
        {
            native = GetLastError();
            r.error_code = detail::map_winhttp_error(native, ErrorCode::ReceiveFailed);
            r.native_error_code = native;
            r.error_message = detail::winhttp_error_message(L"\u63a5\u6536\u670d\u52a1\u5668\u54cd\u5e94", native);
            return r;
        }
        DWORD status = 0;
        DWORD status_size = sizeof(status);
        if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX))
        {
            native = GetLastError();
            r.error_code = detail::map_winhttp_error(native, ErrorCode::WinHttpError);
            r.native_error_code = native;
            r.error_message = detail::winhttp_error_message(L"\u8bfb\u53d6 HTTP \u72b6\u6001\u7801", native);
            return r;
        }
        r.status_code = static_cast<int>(status);
        detail::query_raw_headers(request.get(), r.headers);
        if (cookies_enabled)
        {
            std::vector<std::string> set_cookies = r.headers.get_all("Set-Cookie");
            for (size_t i = 0; i < set_cookies.size(); ++i)
            {
                cookies_.store(url, set_cookies[i]);
            }
        }
        uint64_t content_length = 0;
        std::string cl = r.headers.get("Content-Length");
        bool content_length_known = !cl.empty() && detail::parse_uint64(cl, content_length);
        if (content_length_known)
        {
            if (!sink.reserve(content_length))
            {
                r.error_code = ErrorCode::ResponseTooLarge;
                r.error_message = detail::text(L"\u670d\u52a1\u5668\u54cd\u5e94\u8d85\u8fc7\u5141\u8bb8\u7684\u6700\u5927\u5927\u5c0f");
                return r;
            }
        }
        sink.begin_response(r.status_code, content_length_known, content_length);
        for (;;)
        {
            char buffer[32768];
            DWORD got = 0;
            if (!WinHttpReadData(request.get(), buffer, sizeof(buffer), &got))
            {
                sink.end_response();
                native = GetLastError();
                r.received_bytes = static_cast<size_t>((std::min)(sink.size(), static_cast<uint64_t>((std::numeric_limits<size_t>::max)())));
                r.error_code = detail::map_winhttp_error(native, ErrorCode::ReadFailed);
                r.native_error_code = native;
                r.error_message = detail::winhttp_error_message(L"\u8bfb\u53d6\u54cd\u5e94\u6570\u636e", native);
                return r;
            }
            if (got == 0)
            {
                break;
            }
            if (!sink.write(buffer, static_cast<size_t>(got)))
            {
                sink.end_response();
                r.received_bytes = static_cast<size_t>((std::min)(sink.size(), static_cast<uint64_t>((std::numeric_limits<size_t>::max)())));
                if (sink.too_large())
                {
                    r.error_code = ErrorCode::ResponseTooLarge;
                    r.error_message = detail::text(L"\u670d\u52a1\u5668\u54cd\u5e94\u8d85\u8fc7\u5141\u8bb8\u7684\u6700\u5927\u5927\u5c0f");
                }
                else
                {
                    r.error_code = ErrorCode::FileWriteFailed;
                    r.error_message = detail::text(L"\u5199\u5165\u4e0b\u8f7d\u6587\u4ef6\u5931\u8d25");
                }
                return r;
            }
        }
        sink.end_response();
        r.received_bytes = static_cast<size_t>((std::min)(sink.size(), static_cast<uint64_t>((std::numeric_limits<size_t>::max)())));
        if (r.status_code >= 400)
        {
            r.error_code = ErrorCode::HttpError;
            r.error_message = detail::http_error_message(r.status_code);
        }
        else
        {
            r.error_code = ErrorCode::None;
            r.error_message = detail::text(L"\u65e0");
        }
        return r;
    }

    template <class Sink>
    Response send_with_retry(const std::wstring& method, const detail::ParsedUrl& url, const std::string& body, const Headers& headers, const RequestOptions& options, Sink& sink)
    {
        int max_attempts = options.retry.retries + 1;
        bool method_retryable = detail::is_idempotent_method(method) || options.retry.retry_non_idempotent;
        for (int attempt = 0; attempt < max_attempts; ++attempt)
        {
            if (!sink.reset())
            {
                Response failure;
                failure.error_code = ErrorCode::FileOpenFailed;
                failure.error_message = detail::text(L"\u65e0\u6cd5\u521b\u5efa\u4e0b\u8f7d\u4e34\u65f6\u6587\u4ef6");
                failure.attempts = attempt + 1;
                return failure;
            }
            Response current = send_once(method, url, body, headers, options, sink);
            current.attempts = attempt + 1;
            bool network_retry = current.status_code == 0 && current.native_error_code != 0 && detail::is_retryable_winhttp_error(current.native_error_code);
            bool http_retry = detail::is_retryable_http_status(current.status_code);
            bool can_retry = method_retryable && attempt + 1 < max_attempts && (network_retry || http_retry);
            if (!can_retry)
            {
                if (current.attempts > 1 && (current.error_code != ErrorCode::None || detail::is_retryable_http_status(current.status_code)))
                {
                    current.error_message += detail::attempts_suffix(current.attempts);
                }
                return current;
            }
            detail::sleep_retry(options.retry, attempt, http_retry ? &current.headers : NULL);
        }
        Response failure;
        failure.error_code = ErrorCode::WinHttpError;
        failure.error_message = detail::text(L"\u91cd\u8bd5\u6d41\u7a0b\u5f02\u5e38\u7ed3\u675f");
        return failure;
    }

    template <class Sink>
    Response request_impl(const std::wstring& method, const std::wstring& url, const std::string& body, const Headers& headers, RequestOptions options, Sink& sink)
    {
        struct LanguageGuard
        {
            std::string* old;

            explicit LanguageGuard(std::string& language) : old(detail::active_language)
            {
                detail::active_language = &language;
            }

            ~LanguageGuard()
            {
                detail::active_language = old;
            }
        } guard(language_);
        Response failure;
        if (method.empty())
        {
            failure.error_code = ErrorCode::InvalidArgument;
            failure.error_message = detail::text(L"HTTP \u65b9\u6cd5\u4e0d\u80fd\u4e3a\u7a7a");
            return failure;
        }
        std::string option_error;
        if (!validate_options(options, option_error))
        {
            failure.error_code = ErrorCode::InvalidArgument;
            failure.error_message = option_error;
            return failure;
        }
        std::string header_error;
        if (!detail::validate_headers(headers, header_error))
        {
            failure.error_code = ErrorCode::InvalidHeader;
            failure.error_message = header_error;
            return failure;
        }
        Headers current_headers = headers;
        std::wstring current_method = method;
        std::string current_body = body;
        detail::SecureStringGuard current_body_guard(current_body);
        std::wstring current_url = url;
        int redirects = 0;
        for (;;)
        {
            detail::ParsedUrl parsed;
            std::string parse_error;
            bool unsupported_scheme = false;
            if (!detail::parse_url(current_url, parsed, parse_error, &unsupported_scheme))
            {
                failure.error_code = unsupported_scheme ? ErrorCode::UnsupportedScheme : ErrorCode::InvalidUrl;
                failure.error_message = parse_error;
                failure.redirect_count = redirects;
                return failure;
            }
            std::string prepared;
            detail::SecureStringGuard prepared_guard(prepared);
            std::string body_error;
            if (!detail::prepare_request_body(current_body, current_headers.get("Content-Type"), prepared, body_error))
            {
                failure.error_code = ErrorCode::EncodingError;
                failure.error_message = body_error;
                failure.redirect_count = redirects;
                return failure;
            }
            Response r = send_with_retry(current_method, parsed, prepared, current_headers, options, sink);
            r.redirect_count = redirects;
            r.final_url = parsed.absolute;
            if (!options.follow_redirect || !detail::is_redirect_status(r.status_code))
            {
                return r;
            }
            std::string location = r.headers.get("Location");
            if (location.empty())
            {
                return r;
            }
            if (redirects >= options.max_redirects)
            {
                r.error_code = ErrorCode::RedirectLimitExceeded;
                r.error_message = detail::text(L"\u91cd\u5b9a\u5411\u6b21\u6570\u8d85\u8fc7\u4e0a\u9650");
                return r;
            }
            std::wstring next_url;
            std::string redirect_error;
            if (!detail::resolve_redirect(parsed, location, next_url, redirect_error))
            {
                r.error_code = ErrorCode::RedirectError;
                r.error_message = redirect_error;
                return r;
            }
            detail::ParsedUrl next;
            if (!detail::parse_url(next_url, next, redirect_error))
            {
                r.error_code = ErrorCode::RedirectError;
                r.error_message = redirect_error;
                return r;
            }
            if (parsed.secure && !next.secure)
            {
                r.error_code = ErrorCode::RedirectError;
                r.error_message = detail::text(L"\u5df2\u963b\u6b62 HTTPS \u964d\u7ea7\u91cd\u5b9a\u5411\u5230 HTTP");
                return r;
            }
            if (!detail::same_origin(parsed, next))
            {
                current_headers.erase("Authorization");
                current_headers.erase("Proxy-Authorization");
                current_headers.erase("Cookie");
                current_headers.erase("Host");
            }
            if ((r.status_code == 303 && !detail::ascii_iequals(current_method, L"HEAD")) || ((r.status_code == 301 || r.status_code == 302) && detail::ascii_iequals(current_method, L"POST")))
            {
                current_method = L"GET";
                detail::secure_clear(current_body);
                current_headers.erase("Content-Type");
                current_headers.erase("Content-Length");
            }
            current_url = next.absolute;
            ++redirects;
        }
    }

public:
    explicit Session(const SessionOptions& options = SessionOptions()) : options_(options), cookies_enabled_(options.enable_cookies), init_native_error_(0), language_(detail::global_language())
    {

        }
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void set_language(const std::string& lang)
    {
        language_ = detail::normalize_language(lang);
    }

    std::string language() const
    {
        return language_;
    }

    void set_proxy(const std::string& proxy, const std::string& bypass = "")
    {
        std::wstring p, b;
        if (detail::utf8_to_wide(proxy, p))
        {
            options_.proxy = p;
        }
        if (detail::utf8_to_wide(bypass, b))
        {
            options_.proxy_bypass = b;
        }
        session_.reset();
        connections_.clear();
    }

    bool valid()
    {
        return ensure_session();
    }

    std::string error_message()
    {
        if (ensure_session())
        {
            return detail::text(L"\u65e0");
        }
        std::lock_guard<std::mutex> lock(connection_mutex_);
        return init_error_;
    }

    CookieJar& cookie_jar()
    {
        return cookies_;
    }

    void clear_cookies()
    {
        cookies_.clear();
    }

    bool delete_cookie(const std::string& name, const std::string& domain = "", const std::string& path = "")
    {
        return cookies_.delete_cookie(name, domain, path);
    }

    size_t clear_cookies_for_domain(const std::string& domain)
    {
        return cookies_.clear_domain(domain);
    }

    size_t cookie_count()
    {
        return cookies_.size();
    }

    void set_cookies_enabled(bool enabled, bool clear_when_disabled = true)
    {
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            cookies_enabled_ = enabled;
        }
        if (!enabled && clear_when_disabled)
        {
            cookies_.clear();
        }
    }

    bool cookies_enabled() const
    {
        std::lock_guard<std::mutex> lock(option_mutex_);
        return cookies_enabled_;
    }

    size_t connection_count()
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        return connections_.size();
    }

    Response request(const std::wstring& method, const std::wstring& url, const std::string& body = "", const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            if (options.max_response_size == 0)
            {
                options.max_response_size = options_.max_response_size;
            }
            if (options.max_redirects < 0)
            {
                options.max_redirects = options_.max_redirects;
            }
        }
        MemorySink sink(options.max_response_size);
        Response r = request_impl(method, url, body, headers, options, sink);
        if (r.error_code != ErrorCode::ResponseTooLarge && r.error_code != ErrorCode::FileWriteFailed)
        {
            std::string content_type = r.headers.get("Content-Type");
            r.body = detail::normalize_response_body(sink.data(), content_type);
            r.received_bytes = static_cast<size_t>((std::min)(sink.size(), static_cast<uint64_t>((std::numeric_limits<size_t>::max)())));
        }
        return r;
    }

    Response request(const std::wstring& method, const std::string& url_utf8, const std::string& body = "", const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        std::wstring url;
        if (!detail::utf8_to_wide(url_utf8, url))
        {
            Response r;
            r.error_code = ErrorCode::EncodingError;
            r.error_message = detail::text(L"URL \u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801");
            return r;
        }
        return request(method, url, body, headers, options);
    }

    Response get(const std::wstring& url, const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        return request(L"GET", url, "", headers, options);
    }

    Response get(const std::string& url, const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        return request(L"GET", url, "", headers, options);
    }

    Response post(const std::wstring& url, const std::string& body, const std::string& content_type = "application/x-www-form-urlencoded", const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        Headers h = headers;
        if (!h.contains("Content-Type"))
        {
            h.set("Content-Type", content_type);
        }
        return request(L"POST", url, body, h, options);
    }

    Response post(const std::string& url, const std::string& body, const std::string& content_type = "application/x-www-form-urlencoded", const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        Headers h = headers;
        if (!h.contains("Content-Type"))
        {
            h.set("Content-Type", content_type);
        }
        return request(L"POST", url, body, h, options);
    }

    Response post(const std::string& url, const std::string& body, const Headers& headers, RequestOptions options = RequestOptions())
    {
        return post(url, body, "application/x-www-form-urlencoded", headers, options);
    }

    Response post(const std::wstring& url, const std::string& body, const Headers& headers, RequestOptions options = RequestOptions())
    {
        return post(url, body, "application/x-www-form-urlencoded", headers, options);
    }

    DownloadResult download(const std::wstring& url, const std::wstring& destination, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        DownloadResult d;
        if (destination.empty())
        {
            d.error_code = ErrorCode::InvalidArgument;
            d.error_message = detail::text(L"\u4e0b\u8f7d\u6587\u4ef6\u8def\u5f84\u4e0d\u80fd\u4e3a\u7a7a");
            return d;
        }
        d.destination = destination;
        FileSink sink(destination, options.max_file_size, options.overwrite, options.show_progress, options.progress_bar_width, options.progress_refresh_ms);
        if (!options.overwrite && sink.destination_exists())
        {
            d.error_code = ErrorCode::FileExists;
            d.error_message = detail::text(L"\u76ee\u6807\u6587\u4ef6\u5df2\u5b58\u5728\uff0c\u672a\u5141\u8bb8\u8986\u76d6");
            return d;
        }
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            if (options.request.max_redirects < 0)
            {
                options.request.max_redirects = options_.max_redirects;
            }
            if (options.request.max_response_size == 0)
            {
                options.request.max_response_size = options_.max_response_size;
            }
        }
        Response r = request_impl(L"GET", url, "", headers, options.request, sink);
        d.status_code = r.status_code;
        d.headers = r.headers;
        d.error_code = r.error_code;
        d.native_error_code = r.native_error_code;
        d.error_message = r.error_message;
        d.bytes_written = sink.size();
        std::string content_length_text = d.headers.get("Content-Length");
        uint64_t reported_size = 0;
        if (!content_length_text.empty() && detail::parse_uint64(content_length_text, reported_size))
        {
            d.file_size = reported_size;
            d.file_size_known = true;
        }
        else
        {
            d.file_size = d.bytes_written;
            d.file_size_known = d.error_code == ErrorCode::None && d.status_code >= 200 && d.status_code < 300;
        }
        d.attempts = r.attempts;
        d.redirect_count = r.redirect_count;
        d.final_url = r.final_url;
        if (r.error_code == ErrorCode::None && r.status_code >= 200 && r.status_code < 300)
        {
            if (!sink.commit())
            {
                d.error_code = ErrorCode::FileCommitFailed;
                d.error_message = detail::text(L"\u4e0b\u8f7d\u5b8c\u6210\uff0c\u4f46\u5c06\u4e34\u65f6\u6587\u4ef6\u5b89\u5168\u66ff\u6362\u5230\u76ee\u6807\u8def\u5f84\u65f6\u5931\u8d25");
            }
        }
        return d;
    }

    DownloadResult download(const std::string& url_utf8, const std::string& destination_utf8, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        std::wstring url;
        std::wstring destination;
        if (!detail::utf8_to_wide(url_utf8, url) || !detail::utf8_to_wide(destination_utf8, destination))
        {
            DownloadResult d;
            d.error_code = ErrorCode::EncodingError;
            d.error_message = detail::text(L"URL \u6216\u4e0b\u8f7d\u6587\u4ef6\u8def\u5f84\u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801");
            return d;
        }
        return download(url, destination, headers, options);
    }

    DownloadResult download(const std::wstring& url, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        return download(url, suggested_download_filename(url), headers, options);
    }

    DownloadResult download(const std::string& url_utf8, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        std::wstring url;
        if (!detail::utf8_to_wide(url_utf8, url))
        {
            DownloadResult d;
            d.error_code = ErrorCode::EncodingError;
            d.error_message = detail::text(L"URL \u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801");
            return d;
        }
        return download(url, suggested_download_filename(url), headers, options);
    }


};

inline RequestOptions legacy_options(int timeout_ms, bool follow_redirect, int retries)
{
    RequestOptions options;
    options.timeout_ms = timeout_ms;
    options.follow_redirect = follow_redirect;
    options.retry.retries = retries;
    options.retry.retry_non_idempotent = retries > 0;
    return options;
}

inline Response send_request(const std::wstring& method, const std::wstring& url, const std::string& body = "", const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    SessionOptions so;
    so.enable_cookies = false;
    Session session(so);
    return session.request(method, url, body, headers, legacy_options(timeout_ms, follow_redirect, retries));
}

inline Response get(const std::wstring& url, const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    return send_request(L"GET", url, "", headers, timeout_ms, follow_redirect, retries);
}

inline Response get(const std::string& url_utf8, const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    std::wstring url;
    if (!detail::utf8_to_wide(url_utf8, url))
    {
        Response r;
        r.error_code = ErrorCode::EncodingError;
        r.error_message = detail::text(L"URL \u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801");
        return r;
    }
    return get(url, headers, timeout_ms, follow_redirect, retries);
}

inline Response post(const std::wstring& url, const std::string& body, const std::string& content_type = "application/x-www-form-urlencoded", const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    Headers h = headers;
    if (!h.contains("Content-Type"))
    {
        h.set("Content-Type", content_type);
    }
    return send_request(L"POST", url, body, h, timeout_ms, follow_redirect, retries);
}

inline Response post(const std::string& url_utf8, const std::string& body, const std::string& content_type = "application/x-www-form-urlencoded", const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    std::wstring url;
    if (!detail::utf8_to_wide(url_utf8, url))
    {
        Response r;
        r.error_code = ErrorCode::EncodingError;
        r.error_message = detail::text(L"URL \u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801");
        return r;
    }
    return post(url, body, content_type, headers, timeout_ms, follow_redirect, retries);
}

inline DownloadResult download(const std::wstring& url, const std::wstring& destination, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
{
    SessionOptions so;
    so.enable_cookies = false;
    Session session(so);
    return session.download(url, destination, headers, options);
}

inline DownloadResult download(const std::string& url_utf8, const std::string& destination_utf8, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
{
    SessionOptions so;
    so.enable_cookies = false;
    Session session(so);
    return session.download(url_utf8, destination_utf8, headers, options);
}

inline DownloadResult download(const std::wstring& url, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
{
    SessionOptions so;
    so.enable_cookies = false;
    Session session(so);
    return session.download(url, headers, options);
}

inline DownloadResult download(const std::string& url_utf8, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
{
    SessionOptions so;
    so.enable_cookies = false;
    Session session(so);
    return session.download(url_utf8, headers, options);
}


inline std::string to_utf8(const std::string& src, int codepage)
{
    if (src.empty())
    {
        return std::string();
    }
    std::string out;
    if (!detail::convert_codepage(src, static_cast<UINT>(codepage), CP_UTF8, out))
    {
        return src;
    }
    return out;
}

inline std::string to_utf8(const std::string& src, const std::string& encoding)
{
    UINT cp = 0;
    if (detail::encoding_to_codepage(encoding, cp))
    {
        return to_utf8(src, static_cast<int>(cp));
    }
    return src;
}

inline std::string text_size(size_t bytes, const std::string& unit = "")
{
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int index = -1;
    if (!unit.empty())
    {
        std::string u = unit;
        std::transform(u.begin(), u.end(), u.begin(), [](unsigned char c)
        {
             return static_cast<char>(std::toupper(c));
        });
        if (u == "B")
        {
            index = 0;
        }
        else if (u == "KB")
        {
            index = 1;
        }
        else if (u == "MB")
        {
            index = 2;
        }
        else if (u == "GB")
        {
            index = 3;
        }
        else if (u == "TB")
        {
            index = 4;
        }
        else if (u == "AUTO")
        {
            index = -1;
        }
    }
    double size = static_cast<double>(bytes);
    int chosen = 0;
    if (index >= 0 && index <= 4)
    {
        for (int i = 0; i < index; ++i)
        {
            size /= 1024.0;
        }
        chosen = index;
    }
    else
    {
        while (size >= 1024.0 && chosen < 4)
        {
             size /= 1024.0; ++chosen;
        }
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[chosen];
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
    std::vector<std::string> values;
    std::istringstream stream(str);
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
        {
            line.erase(line.size() - 1);
        }
        values.push_back(line);
    }
    if (start_line < 1 || start_line > values.size() || start_line > end_line)
    {
        return "";
    }
    if (end_line > values.size())
    {
        end_line = values.size();
    }
    std::ostringstream result;
    for (size_t i = start_line - 1; i < end_line; ++i)
    {
        result << values[i];
        if (i + 1 < end_line)
        {
            result << '\n';
        }
    }
    return result.str();
}


enum class BrowserErrorCode
{
    None,
    UnsupportedPlatform,
    SdkUnavailable,
    LoaderNotFound,
    RuntimeNotFound,
    InvalidArgument,
    EncodingError,
    ComInitializationFailed,
    WindowClassFailed,
    WindowCreationFailed,
    EnvironmentCreationFailed,
    ControllerCreationFailed,
    NavigationFailed,
    MessageLoopFailed
};

struct BrowserOptions
{
    std::wstring title = L"FeatherCrawl";
    int width = 1100;
    int height = 760;
    bool resizable = true;
    bool script_enabled = true;
    bool devtools_enabled = true;
    bool context_menus_enabled = true;
    bool status_bar_enabled = true;
    bool default_script_dialogs_enabled = true;
    std::wstring user_data_folder;
    std::wstring browser_executable_folder;
};

struct BrowserResult
{
    BrowserErrorCode error_code = BrowserErrorCode::None;
    HRESULT native_error_code = S_OK;
    std::string error_message;
    int exit_code = 0;
    std::wstring runtime_version;

    bool ok() const
    {
        return error_code == BrowserErrorCode::None;
    }
};

namespace detail
{

inline std::string browser_text(const char* english, const wchar_t* chinese)
{
    if (current_language() == "zh_CN")
    {
        return wide_to_utf8(chinese ? std::wstring(chinese) : std::wstring());
    }
    return english ? std::string(english) : std::string();
}

#if FEATHERCRAWL_HAS_WEBVIEW2

typedef HRESULT (STDMETHODCALLTYPE *FeatherWebView2InternalCreateFunction)(bool, int, PCWSTR, IUnknown*, IUnknown*);

inline std::wstring webview2_architecture_name()
{
#if defined(_M_X64) || defined(__x86_64__)
    return L"x64";
#elif defined(_M_ARM64) || defined(__aarch64__)
    return L"arm64";
#elif defined(_M_IX86) || defined(__i386__)
    return L"x86";
#else
    return std::wstring();
#endif
}

inline bool webview2_file_exists(const std::wstring& path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

inline std::wstring webview2_join_path(const std::wstring& left, const std::wstring& right)
{
    if (left.empty())
    {
        return right;
    }
    std::wstring result = left;
    wchar_t last = result[result.size() - 1];
    if (last != L'\\' && last != L'/')
    {
        result += L'\\';
    }
    result += right;
    return result;
}

inline bool webview2_read_registry_string(HKEY root, const std::wstring& subkey, const wchar_t* value_name, std::wstring& value)
{
    value.clear();
    HKEY key = NULL;
    LONG open_result = RegOpenKeyExW(root, subkey.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &key);
    if (open_result != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD type = 0;
    DWORD size = 0;
    LONG query_result = RegQueryValueExW(key, value_name, NULL, &type, NULL, &size);
    if (query_result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || size < sizeof(wchar_t))
    {
        RegCloseKey(key);
        return false;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(size / sizeof(wchar_t)) + 2, L'\0');
    query_result = RegQueryValueExW(key, value_name, NULL, &type, reinterpret_cast<LPBYTE>(&buffer[0]), &size);
    RegCloseKey(key);
    if (query_result != ERROR_SUCCESS)
    {
        return false;
    }

    value.assign(&buffer[0]);
    if (type == REG_EXPAND_SZ && !value.empty())
    {
        DWORD required = ExpandEnvironmentStringsW(value.c_str(), NULL, 0);
        if (required > 0)
        {
            std::vector<wchar_t> expanded(static_cast<size_t>(required) + 1, L'\0');
            if (ExpandEnvironmentStringsW(value.c_str(), &expanded[0], required) > 0)
            {
                value.assign(&expanded[0]);
            }
        }
    }
    return !value.empty();
}

inline std::wstring webview2_environment_path(const wchar_t* name)
{
    DWORD required = GetEnvironmentVariableW(name, NULL, 0);
    if (required == 0)
    {
        return std::wstring();
    }
    std::vector<wchar_t> buffer(static_cast<size_t>(required) + 1, L'\0');
    DWORD written = GetEnvironmentVariableW(name, &buffer[0], required);
    if (written == 0 || written >= required)
    {
        return std::wstring();
    }
    return std::wstring(&buffer[0]);
}

inline std::wstring webview2_runtime_dll_from_base(const std::wstring& base)
{
    std::wstring architecture = webview2_architecture_name();
    if (base.empty() || architecture.empty())
    {
        return std::wstring();
    }
    return webview2_join_path(webview2_join_path(webview2_join_path(base, L"EBWebView"), architecture), L"EmbeddedBrowserWebView.dll");
}

inline std::wstring webview2_last_path_component(const std::wstring& path)
{
    if (path.empty())
    {
        return std::wstring();
    }
    size_t end = path.size();
    while (end > 0 && (path[end - 1] == L'\\' || path[end - 1] == L'/'))
    {
        --end;
    }
    if (end == 0)
    {
        return std::wstring();
    }
    size_t slash = path.find_last_of(L"\\/", end - 1);
    return path.substr(slash == std::wstring::npos ? 0 : slash + 1, end - (slash == std::wstring::npos ? 0 : slash + 1));
}

struct WebView2RuntimeLocation
{
    std::wstring dll_path;
    std::wstring version;
    int runtime_type;

    WebView2RuntimeLocation() : runtime_type(0)
    {
    }

    bool valid() const
    {
        return !dll_path.empty();
    }
};

inline bool webview2_try_installed_runtime(HKEY root, WebView2RuntimeLocation& location)
{
    static const wchar_t* runtime_guid = L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}";
    std::wstring state_key = std::wstring(L"SOFTWARE\\Microsoft\\EdgeUpdate\\ClientState\\") + runtime_guid;
    std::wstring clients_key = std::wstring(L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\") + runtime_guid;
    std::wstring base;
    std::wstring version;

    webview2_read_registry_string(root, state_key, L"EBWebView", base);
    webview2_read_registry_string(root, clients_key, L"pv", version);

    if (!base.empty())
    {
        std::wstring dll_path = webview2_runtime_dll_from_base(base);
        if (webview2_file_exists(dll_path))
        {
            location.dll_path = dll_path;
            location.version = version.empty() ? webview2_last_path_component(base) : version;
            location.runtime_type = 0;
            return true;
        }
    }

    if (!version.empty() && version != L"0.0.0.0")
    {
        std::vector<std::wstring> roots;
        if (root == HKEY_CURRENT_USER)
        {
            std::wstring local = webview2_environment_path(L"LOCALAPPDATA");
            if (!local.empty())
            {
                roots.push_back(webview2_join_path(local, L"Microsoft\\EdgeWebView\\Application"));
            }
        }
        else
        {
            std::wstring program_files_x86 = webview2_environment_path(L"ProgramFiles(x86)");
            if (!program_files_x86.empty())
            {
                roots.push_back(webview2_join_path(program_files_x86, L"Microsoft\\EdgeWebView\\Application"));
            }
            std::wstring program_files = webview2_environment_path(L"ProgramFiles");
            if (!program_files.empty())
            {
                roots.push_back(webview2_join_path(program_files, L"Microsoft\\EdgeWebView\\Application"));
            }
        }

        for (size_t i = 0; i < roots.size(); ++i)
        {
            std::wstring candidate_base = webview2_join_path(roots[i], version);
            std::wstring dll_path = webview2_runtime_dll_from_base(candidate_base);
            if (webview2_file_exists(dll_path))
            {
                location.dll_path = dll_path;
                location.version = version;
                location.runtime_type = 0;
                return true;
            }
        }
    }
    return false;
}

inline WebView2RuntimeLocation webview2_find_runtime(const std::wstring& browser_folder)
{
    WebView2RuntimeLocation location;
    if (!browser_folder.empty())
    {
        std::wstring direct = browser_folder;
        if (webview2_file_exists(direct))
        {
            location.dll_path = direct;
            location.version = webview2_last_path_component(browser_folder);
            location.runtime_type = 1;
            return location;
        }
        std::wstring dll_path = webview2_runtime_dll_from_base(browser_folder);
        if (webview2_file_exists(dll_path))
        {
            location.dll_path = dll_path;
            location.version = webview2_last_path_component(browser_folder);
            location.runtime_type = 1;
            return location;
        }
        return location;
    }

    if (webview2_try_installed_runtime(HKEY_LOCAL_MACHINE, location))
    {
        return location;
    }
    if (webview2_try_installed_runtime(HKEY_CURRENT_USER, location))
    {
        return location;
    }
    return location;
}

struct WebView2RuntimeApi
{
    HMODULE module;
    FeatherWebView2InternalCreateFunction create_environment;
    WebView2RuntimeLocation location;

    WebView2RuntimeApi() : module(NULL), create_environment(NULL)
    {
    }

    ~WebView2RuntimeApi()
    {
        if (module)
        {
            FreeLibrary(module);
            module = NULL;
        }
    }

    bool load(const std::wstring& browser_folder = std::wstring())
    {
        if (module && create_environment)
        {
            return true;
        }

        location = webview2_find_runtime(browser_folder);
        if (!location.valid())
        {
            return false;
        }

        HMODULE loaded = NULL;
#if defined(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR) && defined(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
        loaded = LoadLibraryExW(location.dll_path.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#endif
        if (!loaded)
        {
            loaded = LoadLibraryW(location.dll_path.c_str());
        }
        if (!loaded)
        {
            return false;
        }

        FARPROC create_proc = GetProcAddress(loaded, "CreateWebViewEnvironmentWithOptionsInternal");
        if (!create_proc)
        {
            FreeLibrary(loaded);
            return false;
        }

        module = loaded;
        create_environment = reinterpret_cast<FeatherWebView2InternalCreateFunction>(create_proc);
        return true;
    }

    HRESULT create(const std::wstring& user_data_folder, IUnknown* completed_handler)
    {
        if (!create_environment)
        {
            return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }
        return create_environment(true, location.runtime_type, user_data_folder.empty() ? NULL : user_data_folder.c_str(), NULL, completed_handler);
    }
};

inline void* webview2_method(void* object, size_t index)
{
    if (!object)
    {
        return NULL;
    }
    void*** object_pointer = reinterpret_cast<void***>(object);
    if (!object_pointer || !*object_pointer)
    {
        return NULL;
    }
    return (*object_pointer)[index];
}

inline HRESULT webview2_query_interface(void* object, REFIID interface_id, void** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = NULL;
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, REFIID, void**);
    Function function = reinterpret_cast<Function>(webview2_method(object, 0));
    return function ? function(object, interface_id, result) : E_NOINTERFACE;
}

inline ULONG webview2_add_ref(void* object)
{
    typedef ULONG (STDMETHODCALLTYPE *Function)(void*);
    Function function = reinterpret_cast<Function>(webview2_method(object, 1));
    return function ? function(object) : 0;
}

inline ULONG webview2_release(void* object)
{
    typedef ULONG (STDMETHODCALLTYPE *Function)(void*);
    Function function = reinterpret_cast<Function>(webview2_method(object, 2));
    return function ? function(object) : 0;
}

inline HRESULT webview2_environment_create_controller(void* environment, HWND window, IUnknown* handler)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, HWND, IUnknown*);
    Function function = reinterpret_cast<Function>(webview2_method(environment, 3));
    return function ? function(environment, window, handler) : E_NOINTERFACE;
}

inline HRESULT webview2_controller_put_visible(void* controller, BOOL visible)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, BOOL);
    Function function = reinterpret_cast<Function>(webview2_method(controller, 4));
    return function ? function(controller, visible) : E_NOINTERFACE;
}

inline HRESULT webview2_controller_put_bounds(void* controller, RECT bounds)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, RECT);
    Function function = reinterpret_cast<Function>(webview2_method(controller, 6));
    return function ? function(controller, bounds) : E_NOINTERFACE;
}

inline HRESULT webview2_controller_notify_position(void* controller)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*);
    Function function = reinterpret_cast<Function>(webview2_method(controller, 23));
    return function ? function(controller) : E_NOINTERFACE;
}

inline HRESULT webview2_controller_close(void* controller)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*);
    Function function = reinterpret_cast<Function>(webview2_method(controller, 24));
    return function ? function(controller) : E_NOINTERFACE;
}

inline HRESULT webview2_controller_get_webview(void* controller, void** webview)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, void**);
    Function function = reinterpret_cast<Function>(webview2_method(controller, 25));
    return function ? function(controller, webview) : E_NOINTERFACE;
}

inline HRESULT webview2_controller3_put_should_detect_monitor_scale_changes(void* controller3, BOOL value)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, BOOL);
    Function function = reinterpret_cast<Function>(webview2_method(controller3, 31));
    return function ? function(controller3, value) : E_NOINTERFACE;
}

inline HRESULT webview2_controller3_put_bounds_mode(void* controller3, int value)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, int);
    Function function = reinterpret_cast<Function>(webview2_method(controller3, 35));
    return function ? function(controller3, value) : E_NOINTERFACE;
}

inline HRESULT webview2_get_settings(void* webview, void** settings)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, void**);
    Function function = reinterpret_cast<Function>(webview2_method(webview, 3));
    return function ? function(webview, settings) : E_NOINTERFACE;
}

inline HRESULT webview2_navigate(void* webview, const wchar_t* url)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, const wchar_t*);
    Function function = reinterpret_cast<Function>(webview2_method(webview, 5));
    return function ? function(webview, url) : E_NOINTERFACE;
}

inline HRESULT webview2_navigate_to_string(void* webview, const wchar_t* html)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, const wchar_t*);
    Function function = reinterpret_cast<Function>(webview2_method(webview, 6));
    return function ? function(webview, html) : E_NOINTERFACE;
}

inline HRESULT webview2_get_browser_process_id(void* webview, UINT32* process_id)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, UINT32*);
    Function function = reinterpret_cast<Function>(webview2_method(webview, 37));
    return function ? function(webview, process_id) : E_NOINTERFACE;
}

inline HRESULT webview2_setting_put(void* settings, size_t index, BOOL value)
{
    typedef HRESULT (STDMETHODCALLTYPE *Function)(void*, BOOL);
    Function function = reinterpret_cast<Function>(webview2_method(settings, index));
    return function ? function(settings, value) : E_NOINTERFACE;
}

struct WebView2HostState
{
    HWND window;
    BrowserOptions options;
    std::wstring content;
    bool html;
    BrowserResult result;
    void* environment;
    void* controller;
    void* webview;
    DWORD browser_process_id;

    WebView2HostState() : window(NULL), html(false), environment(NULL), controller(NULL), webview(NULL), browser_process_id(0)
    {
    }

    ~WebView2HostState()
    {
        shutdown();
    }

    void shutdown()
    {
        if (controller)
        {
            webview2_controller_close(controller);
        }
        if (webview)
        {
            webview2_release(webview);
            webview = NULL;
        }
        if (controller)
        {
            webview2_release(controller);
            controller = NULL;
        }
        if (environment)
        {
            webview2_release(environment);
            environment = NULL;
        }
    }

    void fail(BrowserErrorCode code, HRESULT hr, const char* english, const wchar_t* chinese)
    {
        result.error_code = code;
        result.native_error_code = hr;
        result.error_message = browser_text(english, chinese);
        if (window)
        {
            PostMessageW(window, WM_CLOSE, 0, 0);
        }
    }

    void resize()
    {
        if (!controller || !window)
        {
            return;
        }
        RECT bounds = {};
        if (GetClientRect(window, &bounds))
        {
            webview2_controller_put_bounds(controller, bounds);
        }
    }
};

static const GUID feathercrawl_iid_iunknown = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID feathercrawl_iid_webview2_controller3 = { 0xF9614724, 0x5D2B, 0x41DC, { 0xAE, 0xF7, 0x73, 0xD6, 0x2B, 0x51, 0x54, 0x3B } };
static const GUID feathercrawl_iid_webview2_controller_completed = { 0x6C4819F3, 0xC9B7, 0x4260, { 0x81, 0x27, 0xC9, 0xF5, 0xBD, 0xE7, 0xF6, 0x8C } };
static const GUID feathercrawl_iid_webview2_environment_completed = { 0x4E8A3389, 0xC9D8, 0x4BD2, { 0xB6, 0xB5, 0x12, 0x4F, 0xEE, 0x6C, 0xC1, 0x4D } };

struct WebView2ControllerCompletedHandler;
struct WebView2EnvironmentCompletedHandler;

typedef HRESULT (STDMETHODCALLTYPE *WebView2HandlerQueryInterfaceFunction)(void*, REFIID, void**);
typedef ULONG (STDMETHODCALLTYPE *WebView2HandlerAddRefFunction)(void*);
typedef ULONG (STDMETHODCALLTYPE *WebView2HandlerReleaseFunction)(void*);
typedef HRESULT (STDMETHODCALLTYPE *WebView2ControllerInvokeFunction)(void*, HRESULT, void*);
typedef HRESULT (STDMETHODCALLTYPE *WebView2EnvironmentInvokeFunction)(void*, HRESULT, void*);

struct WebView2ControllerHandlerVTable
{
    WebView2HandlerQueryInterfaceFunction query_interface;
    WebView2HandlerAddRefFunction add_ref;
    WebView2HandlerReleaseFunction release;
    WebView2ControllerInvokeFunction invoke;
};

struct WebView2EnvironmentHandlerVTable
{
    WebView2HandlerQueryInterfaceFunction query_interface;
    WebView2HandlerAddRefFunction add_ref;
    WebView2HandlerReleaseFunction release;
    WebView2EnvironmentInvokeFunction invoke;
};

struct WebView2ControllerCompletedHandler
{
    WebView2ControllerHandlerVTable* vtable;
    volatile LONG references;
    WebView2HostState* state;

    explicit WebView2ControllerCompletedHandler(WebView2HostState* value) : vtable(table()), references(1), state(value)
    {
    }

    static HRESULT STDMETHODCALLTYPE query_interface(void* self, REFIID riid, void** object)
    {
        if (!object)
        {
            return E_POINTER;
        }
        *object = NULL;
        if (!IsEqualIID(riid, feathercrawl_iid_iunknown) && !IsEqualIID(riid, feathercrawl_iid_webview2_controller_completed))
        {
            return E_NOINTERFACE;
        }
        *object = self;
        add_ref(self);
        return S_OK;
    }

    static ULONG STDMETHODCALLTYPE add_ref(void* self)
    {
        WebView2ControllerCompletedHandler* handler = reinterpret_cast<WebView2ControllerCompletedHandler*>(self);
        return static_cast<ULONG>(InterlockedIncrement(&handler->references));
    }

    static ULONG STDMETHODCALLTYPE release(void* self)
    {
        WebView2ControllerCompletedHandler* handler = reinterpret_cast<WebView2ControllerCompletedHandler*>(self);
        ULONG value = static_cast<ULONG>(InterlockedDecrement(&handler->references));
        if (value == 0)
        {
            delete handler;
        }
        return value;
    }

    static HRESULT STDMETHODCALLTYPE invoke(void* self, HRESULT error_code, void* controller)
    {
        WebView2ControllerCompletedHandler* handler = reinterpret_cast<WebView2ControllerCompletedHandler*>(self);
        WebView2HostState* state = handler ? handler->state : NULL;
        if (!state)
        {
            return E_FAIL;
        }
        if (FAILED(error_code) || !controller)
        {
            state->fail(BrowserErrorCode::ControllerCreationFailed, error_code, "Unable to create the WebView2 controller", L"无法创建 WebView2 控制器");
            return S_OK;
        }

        state->controller = controller;
        webview2_add_ref(state->controller);

        void* controller3 = NULL;
        if (SUCCEEDED(webview2_query_interface(state->controller, feathercrawl_iid_webview2_controller3, &controller3)) && controller3)
        {
            webview2_controller3_put_should_detect_monitor_scale_changes(controller3, TRUE);
            webview2_controller3_put_bounds_mode(controller3, 0);
            webview2_release(controller3);
        }

        HRESULT hr = webview2_controller_get_webview(controller, &state->webview);
        if (FAILED(hr) || !state->webview)
        {
            state->fail(BrowserErrorCode::ControllerCreationFailed, hr, "Unable to obtain the WebView2 instance", L"无法获取 WebView2 实例");
            return S_OK;
        }

        UINT32 process_id = 0;
        if (SUCCEEDED(webview2_get_browser_process_id(state->webview, &process_id)))
        {
            state->browser_process_id = static_cast<DWORD>(process_id);
        }

        void* settings = NULL;
        if (SUCCEEDED(webview2_get_settings(state->webview, &settings)) && settings)
        {
            webview2_setting_put(settings, 4, state->options.script_enabled ? TRUE : FALSE);
            webview2_setting_put(settings, 6, TRUE);
            webview2_setting_put(settings, 8, state->options.default_script_dialogs_enabled ? TRUE : FALSE);
            webview2_setting_put(settings, 10, state->options.status_bar_enabled ? TRUE : FALSE);
            webview2_setting_put(settings, 12, state->options.devtools_enabled ? TRUE : FALSE);
            webview2_setting_put(settings, 14, state->options.context_menus_enabled ? TRUE : FALSE);
            webview2_release(settings);
        }

        state->resize();
        webview2_controller_put_visible(controller, TRUE);

        hr = state->html ? webview2_navigate_to_string(state->webview, state->content.c_str()) : webview2_navigate(state->webview, state->content.c_str());
        if (FAILED(hr))
        {
            state->fail(BrowserErrorCode::NavigationFailed, hr, "WebView2 navigation failed", L"WebView2 导航失败");
            return S_OK;
        }

        ShowWindow(state->window, SW_SHOW);
        UpdateWindow(state->window);
        return S_OK;
    }

    static WebView2ControllerHandlerVTable* table()
    {
        static WebView2ControllerHandlerVTable value = { query_interface, add_ref, release, invoke };
        return &value;
    }
};

struct WebView2EnvironmentCompletedHandler
{
    WebView2EnvironmentHandlerVTable* vtable;
    volatile LONG references;
    WebView2HostState* state;

    explicit WebView2EnvironmentCompletedHandler(WebView2HostState* value) : vtable(table()), references(1), state(value)
    {
    }

    static HRESULT STDMETHODCALLTYPE query_interface(void* self, REFIID riid, void** object)
    {
        if (!object)
        {
            return E_POINTER;
        }
        *object = NULL;
        if (!IsEqualIID(riid, feathercrawl_iid_iunknown) && !IsEqualIID(riid, feathercrawl_iid_webview2_environment_completed))
        {
            return E_NOINTERFACE;
        }
        *object = self;
        add_ref(self);
        return S_OK;
    }

    static ULONG STDMETHODCALLTYPE add_ref(void* self)
    {
        WebView2EnvironmentCompletedHandler* handler = reinterpret_cast<WebView2EnvironmentCompletedHandler*>(self);
        return static_cast<ULONG>(InterlockedIncrement(&handler->references));
    }

    static ULONG STDMETHODCALLTYPE release(void* self)
    {
        WebView2EnvironmentCompletedHandler* handler = reinterpret_cast<WebView2EnvironmentCompletedHandler*>(self);
        ULONG value = static_cast<ULONG>(InterlockedDecrement(&handler->references));
        if (value == 0)
        {
            delete handler;
        }
        return value;
    }

    static HRESULT STDMETHODCALLTYPE invoke(void* self, HRESULT error_code, void* environment)
    {
        WebView2EnvironmentCompletedHandler* handler = reinterpret_cast<WebView2EnvironmentCompletedHandler*>(self);
        WebView2HostState* state = handler ? handler->state : NULL;
        if (!state)
        {
            return E_FAIL;
        }
        if (FAILED(error_code) || !environment)
        {
            state->fail(BrowserErrorCode::EnvironmentCreationFailed, error_code, "Unable to create the WebView2 environment", L"无法创建 WebView2 环境");
            return S_OK;
        }

        state->environment = environment;
        webview2_add_ref(state->environment);

        WebView2ControllerCompletedHandler* controller_handler = new WebView2ControllerCompletedHandler(state);
        HRESULT hr = webview2_environment_create_controller(environment, state->window, reinterpret_cast<IUnknown*>(controller_handler));
        WebView2ControllerCompletedHandler::release(controller_handler);
        if (FAILED(hr))
        {
            state->fail(BrowserErrorCode::ControllerCreationFailed, hr, "Unable to start WebView2 controller creation", L"无法启动 WebView2 控制器创建");
        }
        return S_OK;
    }

    static WebView2EnvironmentHandlerVTable* table()
    {
        static WebView2EnvironmentHandlerVTable value = { query_interface, add_ref, release, invoke };
        return &value;
    }
};

inline bool webview2_remove_user_data_tree(const std::wstring& path)
{
    if (path.empty())
    {
        return true;
    }

    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND;
    }
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        return false;
    }

    std::wstring pattern = webview2_join_path(path, L"*");
    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find != INVALID_HANDLE_VALUE)
    {
        bool complete = true;
        do
        {
            if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
            {
                continue;
            }

            std::wstring child = webview2_join_path(path, data.cFileName);
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                {
                    SetFileAttributesW(child.c_str(), FILE_ATTRIBUTE_NORMAL);
                    if (!RemoveDirectoryW(child.c_str()))
                    {
                        complete = false;
                    }
                }
                else if (!webview2_remove_user_data_tree(child))
                {
                    complete = false;
                }
            }
            else
            {
                SetFileAttributesW(child.c_str(), FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFileW(child.c_str()))
                {
                    complete = false;
                }
            }
        }
        while (FindNextFileW(find, &data));
        FindClose(find);
        if (!complete)
        {
            return false;
        }
    }
    else
    {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND)
        {
            return false;
        }
    }

    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    if (RemoveDirectoryW(path.c_str()))
    {
        return true;
    }
    DWORD error = GetLastError();
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
}

inline bool webview2_temporary_user_data_process(const wchar_t* name, DWORD& process_id)
{
    process_id = 0;
    if (!name)
    {
        return false;
    }
    const wchar_t* prefix = L"FeatherCrawl.WebView2.";
    size_t prefix_length = wcslen(prefix);
    if (wcsncmp(name, prefix, prefix_length) != 0)
    {
        return false;
    }
    const wchar_t* current = name + prefix_length;
    if (*current < L'0' || *current > L'9')
    {
        return false;
    }
    unsigned long value = 0;
    while (*current >= L'0' && *current <= L'9')
    {
        unsigned long digit = static_cast<unsigned long>(*current - L'0');
        if (value > ((std::numeric_limits<unsigned long>::max)() - digit) / 10UL)
        {
            return false;
        }
        value = value * 10UL + digit;
        ++current;
    }
    if (*current != L'.' || value == 0 || value > static_cast<unsigned long>((std::numeric_limits<DWORD>::max)()))
    {
        return false;
    }
    process_id = static_cast<DWORD>(value);
    return true;
}

inline bool webview2_process_is_running(DWORD process_id)
{
    if (process_id == 0)
    {
        return false;
    }
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, process_id);
    if (!process)
    {
        DWORD error = GetLastError();
        return error == ERROR_ACCESS_DENIED;
    }
    DWORD result = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return result == WAIT_TIMEOUT;
}

inline void webview2_cleanup_stale_temporary_user_data_folders()
{
    wchar_t temp_path[MAX_PATH + 2] = {};
    DWORD length = GetTempPathW(MAX_PATH + 1, temp_path);
    if (length == 0 || length > MAX_PATH)
    {
        return;
    }
    std::wstring pattern = webview2_join_path(temp_path, L"FeatherCrawl.WebView2.*");
    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            continue;
        }
        DWORD process_id = 0;
        if (!webview2_temporary_user_data_process(data.cFileName, process_id))
        {
            continue;
        }
        if (webview2_process_is_running(process_id))
        {
            continue;
        }
        webview2_remove_user_data_tree(webview2_join_path(temp_path, data.cFileName));
    }
    while (FindNextFileW(find, &data));
    FindClose(find);
}

inline ULONGLONG webview2_tick_count()
{
    typedef ULONGLONG (WINAPI *GetTickCount64Function)(void);
    HMODULE module = GetModuleHandleW(L"kernel32.dll");
    if (module)
    {
        GetTickCount64Function get_tick_count64 = reinterpret_cast<GetTickCount64Function>(GetProcAddress(module, "GetTickCount64"));
        if (get_tick_count64)
        {
            return get_tick_count64();
        }
    }
    return static_cast<ULONGLONG>(GetTickCount());
}

inline std::wstring webview2_temporary_user_data_folder()
{
    webview2_cleanup_stale_temporary_user_data_folders();
    wchar_t temp_path[MAX_PATH + 2] = {};
    DWORD length = GetTempPathW(MAX_PATH + 1, temp_path);
    if (length == 0 || length > MAX_PATH)
    {
        return std::wstring();
    }

    ULONGLONG tick = webview2_tick_count();
    for (unsigned int attempt = 0; attempt < 64; ++attempt)
    {
        std::wostringstream name;
        name << L"FeatherCrawl.WebView2."
             << static_cast<unsigned long>(GetCurrentProcessId()) << L"."
             << static_cast<unsigned long>(GetCurrentThreadId()) << L"."
             << static_cast<unsigned long long>(tick) << L"."
             << attempt;
        std::wstring path = webview2_join_path(temp_path, name.str());
        if (CreateDirectoryW(path.c_str(), NULL))
        {
            return path;
        }
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS && error != ERROR_FILE_EXISTS)
        {
            return std::wstring();
        }
    }
    return std::wstring();
}

inline void webview2_wait_for_browser_process(DWORD process_id)
{
    if (process_id == 0)
    {
        return;
    }
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, process_id);
    if (!process)
    {
        return;
    }
    WaitForSingleObject(process, 5000);
    CloseHandle(process);
}

inline void webview2_cleanup_temporary_user_data_folder(const std::wstring& path)
{
    if (path.empty())
    {
        return;
    }
    for (unsigned int attempt = 0; attempt < 80; ++attempt)
    {
        if (webview2_remove_user_data_tree(path))
        {
            return;
        }
        Sleep(attempt < 10 ? 50 : 100);
    }
}

class WebView2DpiContext
{
    typedef HANDLE (WINAPI *SetThreadDpiAwarenessContextFunction)(HANDLE);

    HMODULE module_;
    SetThreadDpiAwarenessContextFunction set_context_;
    HANDLE previous_;
    bool changed_;

public:
    WebView2DpiContext() : module_(NULL), set_context_(NULL), previous_(NULL), changed_(false)
    {
        module_ = LoadLibraryW(L"user32.dll");
        if (!module_)
        {
            return;
        }
        set_context_ = reinterpret_cast<SetThreadDpiAwarenessContextFunction>(GetProcAddress(module_, "SetThreadDpiAwarenessContext"));
        if (!set_context_)
        {
            return;
        }

        HANDLE per_monitor_v2 = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4));
        previous_ = set_context_(per_monitor_v2);
        if (!previous_)
        {
            HANDLE per_monitor = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-3));
            previous_ = set_context_(per_monitor);
        }
        changed_ = previous_ != NULL;
    }

    ~WebView2DpiContext()
    {
        if (changed_ && set_context_)
        {
            set_context_(previous_);
        }
        if (module_)
        {
            FreeLibrary(module_);
        }
    }

    WebView2DpiContext(const WebView2DpiContext&) = delete;
    WebView2DpiContext& operator=(const WebView2DpiContext&) = delete;
};

inline LRESULT CALLBACK webview2_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    WebView2HostState* state = reinterpret_cast<WebView2HostState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        state = reinterpret_cast<WebView2HostState*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        if (state)
        {
            state->window = window;
        }
    }

    if (state)
    {
        if (message == WM_SIZE)
        {
            state->resize();
            return 0;
        }
        if (message == 0x02E0U)
        {
            RECT* suggested = reinterpret_cast<RECT*>(lparam);
            if (suggested)
            {
                SetWindowPos(window, NULL, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOACTIVATE | SWP_NOZORDER);
            }
            state->resize();
            if (state->controller)
            {
                webview2_controller_notify_position(state->controller);
            }
            return 0;
        }
        if (message == WM_MOVE || message == WM_MOVING)
        {
            if (state->controller)
            {
                webview2_controller_notify_position(state->controller);
            }
        }
    }

    if (message == WM_CLOSE)
    {
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

inline bool register_webview2_window_class(HINSTANCE instance)
{
    const wchar_t* name = L"FeatherCrawlWebView2HostWindow";
    WNDCLASSEXW existing = {};
    existing.cbSize = sizeof(existing);
    if (GetClassInfoExW(instance, name, &existing))
    {
        return true;
    }

    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = webview2_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    window_class.lpszClassName = name;
    if (RegisterClassExW(&window_class) != 0)
    {
        return true;
    }
    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

class WebView2ComApartment
{
    typedef HRESULT (WINAPI *InitializeFunction)(LPVOID, DWORD);
    typedef void (WINAPI *UninitializeFunction)(void);

    HMODULE module_;
    InitializeFunction initialize_;
    UninitializeFunction uninitialize_;
    bool initialized_;

public:
    WebView2ComApartment() : module_(NULL), initialize_(NULL), uninitialize_(NULL), initialized_(false)
    {
    }

    ~WebView2ComApartment()
    {
        if (initialized_ && uninitialize_)
        {
            uninitialize_();
        }
        if (module_)
        {
            FreeLibrary(module_);
        }
    }

    HRESULT initialize(DWORD apartment_type)
    {
        module_ = LoadLibraryW(L"ole32.dll");
        if (!module_)
        {
            DWORD error = GetLastError();
            return HRESULT_FROM_WIN32(error ? error : ERROR_MOD_NOT_FOUND);
        }

        initialize_ = reinterpret_cast<InitializeFunction>(GetProcAddress(module_, "CoInitializeEx"));
        uninitialize_ = reinterpret_cast<UninitializeFunction>(GetProcAddress(module_, "CoUninitialize"));
        if (!initialize_ || !uninitialize_)
        {
            return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }

        HRESULT result = initialize_(NULL, apartment_type);
        if (SUCCEEDED(result))
        {
            initialized_ = true;
        }
        return result;
    }

    WebView2ComApartment(const WebView2ComApartment&) = delete;
    WebView2ComApartment& operator=(const WebView2ComApartment&) = delete;
};

inline BrowserResult run_webview2_window(const std::wstring& content, bool html, const BrowserOptions& options)
{
    BrowserResult result;
    if (content.empty())
    {
        result.error_code = BrowserErrorCode::InvalidArgument;
        result.error_message = browser_text("Browser content cannot be empty", L"浏览器内容不能为空");
        return result;
    }
    if (html && content.size() > (2ULL * 1024ULL * 1024ULL) / sizeof(wchar_t))
    {
        result.error_code = BrowserErrorCode::InvalidArgument;
        result.error_message = browser_text("HTML content exceeds the WebView2 NavigateToString limit", L"HTML 内容超过 WebView2 NavigateToString 的大小限制");
        return result;
    }

    WebView2RuntimeApi runtime;
    if (!runtime.load(options.browser_executable_folder))
    {
        result.error_code = BrowserErrorCode::RuntimeNotFound;
        result.native_error_code = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        result.error_message = browser_text("Microsoft Edge WebView2 Runtime was not found or its runtime entry point is unavailable", L"未找到 Microsoft Edge WebView2 Runtime，或运行时入口不可用");
        return result;
    }
    result.runtime_version = runtime.location.version;

    WebView2DpiContext dpi_context;

    WebView2ComApartment com_apartment;
    HRESULT com = com_apartment.initialize(COINIT_APARTMENTTHREADED);
    if (FAILED(com))
    {
        result.error_code = BrowserErrorCode::ComInitializationFailed;
        result.native_error_code = com;
        result.error_message = browser_text("WebView2 requires a single-threaded COM apartment", L"WebView2 需要单线程 COM Apartment");
        return result;
    }

    HINSTANCE instance = GetModuleHandleW(NULL);
    if (!register_webview2_window_class(instance))
    {
        result.error_code = BrowserErrorCode::WindowClassFailed;
        result.native_error_code = HRESULT_FROM_WIN32(GetLastError());
        result.error_message = browser_text("Unable to register the WebView2 host window class", L"无法注册 WebView2 宿主窗口类");
        return result;
    }

    std::wstring temporary_user_data_folder;
    std::wstring active_user_data_folder = options.user_data_folder;
    if (active_user_data_folder.empty())
    {
        temporary_user_data_folder = webview2_temporary_user_data_folder();
        if (temporary_user_data_folder.empty())
        {
            result.error_code = BrowserErrorCode::EnvironmentCreationFailed;
            result.native_error_code = HRESULT_FROM_WIN32(GetLastError());
            result.error_message = browser_text("Unable to create a temporary WebView2 user data folder", L"无法创建 WebView2 临时用户数据目录");
            return result;
        }
        active_user_data_folder = temporary_user_data_folder;
    }

    BrowserResult final_result;
    DWORD browser_process_id = 0;
    {
        WebView2HostState state;
        state.options = options;
        state.content = content;
        state.html = html;
        state.result.runtime_version = result.runtime_version;

        DWORD style = WS_OVERLAPPEDWINDOW;
        if (!options.resizable)
        {
            style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }

        RECT desired = { 0, 0, options.width > 0 ? options.width : 1100, options.height > 0 ? options.height : 760 };
        AdjustWindowRect(&desired, style, FALSE);
        HWND window = CreateWindowExW(0, L"FeatherCrawlWebView2HostWindow", options.title.empty() ? L"FeatherCrawl" : options.title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT, desired.right - desired.left, desired.bottom - desired.top, NULL, NULL, instance, &state);
        if (!window)
        {
            result.error_code = BrowserErrorCode::WindowCreationFailed;
            result.native_error_code = HRESULT_FROM_WIN32(GetLastError());
            result.error_message = browser_text("Unable to create the WebView2 host window", L"无法创建 WebView2 宿主窗口");
            webview2_cleanup_temporary_user_data_folder(temporary_user_data_folder);
            return result;
        }

        WebView2EnvironmentCompletedHandler* handler = new WebView2EnvironmentCompletedHandler(&state);
        HRESULT create_hr = runtime.create(active_user_data_folder, reinterpret_cast<IUnknown*>(handler));
        WebView2EnvironmentCompletedHandler::release(handler);
        if (FAILED(create_hr))
        {
            DestroyWindow(window);
            state.result.error_code = BrowserErrorCode::EnvironmentCreationFailed;
            state.result.native_error_code = create_hr;
            state.result.error_message = browser_text("Unable to start WebView2 environment creation", L"无法启动 WebView2 环境创建");
            final_result = state.result;
        }
        else
        {
            MSG message = {};
            BOOL message_result = 0;
            while ((message_result = GetMessageW(&message, NULL, 0, 0)) > 0)
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }

            if (message_result < 0 && state.result.error_code == BrowserErrorCode::None)
            {
                state.result.error_code = BrowserErrorCode::MessageLoopFailed;
                state.result.native_error_code = HRESULT_FROM_WIN32(GetLastError());
                state.result.error_message = browser_text("The WebView2 window message loop failed", L"WebView2 窗口消息循环失败");
            }
            state.result.exit_code = static_cast<int>(message.wParam);
            final_result = state.result;
        }
        browser_process_id = state.browser_process_id;
        state.shutdown();
    }

    if (!temporary_user_data_folder.empty())
    {
        webview2_wait_for_browser_process(browser_process_id);
        webview2_cleanup_temporary_user_data_folder(temporary_user_data_folder);
    }
    return final_result;
}

#endif

}

inline bool webview2_available()
{
#if FEATHERCRAWL_HAS_WEBVIEW2
    detail::WebView2RuntimeApi runtime;
    return runtime.load();
#else
    return false;
#endif
}

inline std::wstring webview2_runtime_version()
{
#if FEATHERCRAWL_HAS_WEBVIEW2
    detail::WebView2RuntimeApi runtime;
    if (!runtime.load())
    {
        return std::wstring();
    }
    return runtime.location.version;
#else
    return std::wstring();
#endif
}

inline BrowserResult browse(const std::wstring& url, const BrowserOptions& options = BrowserOptions())
{
#if FEATHERCRAWL_HAS_WEBVIEW2
    return detail::run_webview2_window(url, false, options);
#else
    BrowserResult result;
    result.error_code = BrowserErrorCode::SdkUnavailable;
    result.error_message = detail::browser_text("WebView2 support is disabled at compile time", L"WebView2 支持已在编译时禁用");
    return result;
#endif
}

inline BrowserResult browse(const std::string& url_utf8, const BrowserOptions& options = BrowserOptions())
{
    std::wstring url;
    if (!detail::utf8_to_wide(url_utf8, url))
    {
        BrowserResult result;
        result.error_code = BrowserErrorCode::EncodingError;
        result.error_message = detail::browser_text("URL is not valid UTF-8", L"URL 不是有效的 UTF-8 编码");
        return result;
    }
    return browse(url, options);
}

inline BrowserResult render_html(const std::wstring& html, const BrowserOptions& options = BrowserOptions())
{
#if FEATHERCRAWL_HAS_WEBVIEW2
    return detail::run_webview2_window(html, true, options);
#else
    BrowserResult result;
    result.error_code = BrowserErrorCode::SdkUnavailable;
    result.error_message = detail::browser_text("WebView2 support is disabled at compile time", L"WebView2 支持已在编译时禁用");
    return result;
#endif
}

inline BrowserResult render_html(const std::string& html_utf8, const BrowserOptions& options = BrowserOptions())
{
    if (html_utf8.size() > 2ULL * 1024ULL * 1024ULL)
    {
        BrowserResult result;
        result.error_code = BrowserErrorCode::InvalidArgument;
        result.error_message = detail::browser_text("HTML content exceeds the WebView2 NavigateToString limit", L"HTML 内容超过 WebView2 NavigateToString 的大小限制");
        return result;
    }
    std::wstring html;
    if (!detail::utf8_to_wide(html_utf8, html))
    {
        BrowserResult result;
        result.error_code = BrowserErrorCode::EncodingError;
        result.error_message = detail::browser_text("HTML content is not valid UTF-8", L"HTML 内容不是有效的 UTF-8 编码");
        return result;
    }
    return render_html(html, options);
}

}

#elif defined(__linux__)

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <limits>
#include <cctype>
#include <mutex>
#include <memory>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <chrono>
#include <iostream>
#include <cwchar>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <cstdio>
#include <atomic>
#include <thread>
#include <locale>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <termios.h>
#include <iconv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

namespace web
{

namespace detail
{

inline char ascii_lower(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
}

inline wchar_t ascii_lower(wchar_t ch)
{
    return (ch >= L'A' && ch <= L'Z') ? static_cast<wchar_t>(ch - L'A' + L'a') : ch;
}

inline std::string ascii_lower_copy(const std::string& value)
{
    std::string result = value;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = ascii_lower(result[i]);
    }
    return result;
}

inline std::wstring ascii_lower_copy(const std::wstring& value)
{
    std::wstring result = value;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = ascii_lower(result[i]);
    }
    return result;
}

inline bool ascii_iequals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (ascii_lower(a[i]) != ascii_lower(b[i]))
        {
            return false;
        }
    }
    return true;
}

inline bool ascii_iequals(const std::wstring& a, const std::wstring& b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (ascii_lower(a[i]) != ascii_lower(b[i]))
        {
            return false;
        }
    }
    return true;
}

inline std::string trim_ascii(const std::string& value)
{
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
    {
        ++first;
    }
    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
    {
        --last;
    }
    return value.substr(first, last - first);
}

inline std::wstring trim_ascii(const std::wstring& value)
{
    size_t first = 0;
    while (first < value.size() && (value[first] == L' ' || value[first] == L'\t' || value[first] == L'\r' || value[first] == L'\n'))
    {
        ++first;
    }
    size_t last = value.size();
    while (last > first)
    {
        wchar_t c = value[last - 1];
        if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n')
        {
            break;
        }
        --last;
    }
    return value.substr(first, last - first);
}

inline bool append_utf8_codepoint(uint32_t cp, std::string& out)
{
    if (cp <= 0x7F)
    {
        out.push_back(static_cast<char>(cp));
        return true;
    }
    if (cp <= 0x7FF)
    {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return true;
    }
    if (cp >= 0xD800 && cp <= 0xDFFF)
    {
        return false;
    }
    if (cp <= 0xFFFF)
    {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return true;
    }
    if (cp <= 0x10FFFF)
    {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return true;
    }
    return false;
}

inline bool decode_utf8_codepoint(const std::string& src, size_t& index, uint32_t& cp)
{
    if (index >= src.size())
    {
        return false;
    }
    unsigned char c0 = static_cast<unsigned char>(src[index]);
    if (c0 <= 0x7F)
    {
        cp = c0;
        ++index;
        return true;
    }
    int count = 0;
    uint32_t value = 0;
    uint32_t minimum = 0;
    if ((c0 & 0xE0) == 0xC0)
    {
        count = 2;
        value = c0 & 0x1F;
        minimum = 0x80;
    }
    else if ((c0 & 0xF0) == 0xE0)
    {
        count = 3;
        value = c0 & 0x0F;
        minimum = 0x800;
    }
    else if ((c0 & 0xF8) == 0xF0)
    {
        count = 4;
        value = c0 & 0x07;
        minimum = 0x10000;
    }
    else
    {
        return false;
    }
    if (index + static_cast<size_t>(count) > src.size())
    {
        return false;
    }
    for (int i = 1; i < count; ++i)
    {
        unsigned char cx = static_cast<unsigned char>(src[index + static_cast<size_t>(i)]);
        if ((cx & 0xC0) != 0x80)
        {
            return false;
        }
        value = (value << 6) | (cx & 0x3F);
    }
    if (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
    {
        return false;
    }
    index += static_cast<size_t>(count);
    cp = value;
    return true;
}

inline bool utf8_to_wide(const std::string& src, std::wstring& out)
{
    out.clear();
    size_t index = 0;
    while (index < src.size())
    {
        uint32_t cp = 0;
        if (!decode_utf8_codepoint(src, index, cp))
        {
            out.clear();
            return false;
        }
#if WCHAR_MAX <= 0xFFFF
        if (cp <= 0xFFFF)
        {
            out.push_back(static_cast<wchar_t>(cp));
        }
        else
        {
            cp -= 0x10000;
            out.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
            out.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
        }
#else
        out.push_back(static_cast<wchar_t>(cp));
#endif
    }
    return true;
}

inline std::string wide_to_utf8(const std::wstring& src)
{
    std::string out;
    for (size_t i = 0; i < src.size(); ++i)
    {
        uint32_t cp = static_cast<uint32_t>(src[i]);
#if WCHAR_MAX <= 0xFFFF
        if (cp >= 0xD800 && cp <= 0xDBFF)
        {
            if (i + 1 >= src.size())
            {
                return std::string();
            }
            uint32_t low = static_cast<uint32_t>(src[++i]);
            if (low < 0xDC00 || low > 0xDFFF)
            {
                return std::string();
            }
            cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
        }
        else if (cp >= 0xDC00 && cp <= 0xDFFF)
        {
            return std::string();
        }
#endif
        if (!append_utf8_codepoint(cp, out))
        {
            return std::string();
        }
    }
    return out;
}

inline bool is_valid_utf8(const std::string& value)
{
    std::wstring temp;
    return utf8_to_wide(value, temp);
}

inline std::string normalize_language(const std::string& lang)
{
    std::string value = ascii_lower_copy(trim_ascii(lang));
    if (value == "zh" || value == "cn" || value == "zh_cn" || value == "zh-cn" || value == "chinese")
    {
        return "zh_CN";
    }
    return "en_US";
}

inline std::string& global_language()
{
    static std::string value = "en_US";
    return value;
}

inline std::string*& active_language_storage()
{
    static thread_local std::string* value = NULL;
    return value;
}

inline std::string current_language()
{
    std::string* active = active_language_storage();
    return active ? *active : global_language();
}

inline void set_default_language(const std::string& lang)
{
    global_language() = normalize_language(lang);
}

class ScopedLanguage
{
    std::string* old_;

public:
    explicit ScopedLanguage(std::string& language) : old_(active_language_storage())
    {
        active_language_storage() = &language;
    }

    ~ScopedLanguage()
    {
        active_language_storage() = old_;
    }
};

inline std::string text_pair(const char* english, const wchar_t* chinese)
{
    if (current_language() == "zh_CN")
    {
        return wide_to_utf8(chinese ? std::wstring(chinese) : std::wstring());
    }
    return english ? std::string(english) : std::string();
}

inline std::string text(const wchar_t* value)
{
    std::string original = wide_to_utf8(value ? std::wstring(value) : std::wstring());
    if (current_language() == "zh_CN")
    {
        return original;
    }
    static const std::map<std::string, std::string> translations = {
        {"HTTP 请求失败，状态码 ", "HTTP request failed: status code "},
        {"HTTP 方法不能为空", "HTTP method cannot be empty"},
        {"请求超时", "Request timed out"},
        {"无法解析服务器名称", "Unable to resolve server name"},
        {"无法连接到服务器", "Unable to connect to server"},
        {"与服务器的连接发生错误", "Connection error occurred with server"},
        {"HTTPS 安全连接失败", "HTTPS secure connection failed"},
        {"URL 无效", "Invalid URL"},
        {"不支持该 URL 协议", "Unsupported URL scheme"},
        {"身份验证失败", "Authentication failed"},
        {"操作已取消", "Operation was cancelled"},
        {"请求执行失败", "Request execution failed"},
        {"请求超时时间不能小于 0", "Timeout cannot be less than 0"},
        {"最大重定向次数不能小于 0", "Maximum redirects cannot be less than 0"},
        {"重试次数不能小于 0", "Retry count cannot be less than 0"},
        {"重试次数不能大于 100", "Retry count cannot exceed 100"},
        {"最大重定向次数不能大于 100", "Maximum redirects cannot exceed 100"},
        {"服务器响应超过允许的最大大小", "Server response exceeded the maximum allowed size"},
        {"写入下载文件失败", "Failed to write downloaded file"},
        {"URL 或下载文件路径不是有效的 UTF-8 编码", "URL or destination is not valid UTF-8"},
        {"URL 不是有效的 UTF-8 编码", "URL is not valid UTF-8"},
        {"下载文件路径不能为空", "Download destination cannot be empty"},
        {"目标文件已存在，未允许覆盖", "Destination file already exists"},
        {"下载完成，但将临时文件安全替换到目标路径时失败", "Download completed but committing the temporary file failed"},
        {"无", "None"}
    };
    std::map<std::string, std::string>::const_iterator it = translations.find(original);
    if (it != translations.end())
    {
        return it->second;
    }
    return "Operation failed";
}

inline void secure_clear(std::string& value)
{
    if (!value.empty())
    {
        volatile char* p = &value[0];
        for (size_t i = 0; i < value.size(); ++i)
        {
            p[i] = 0;
        }
    }
    value.clear();
}

class SecureStringGuard
{
    std::string* value_;

public:
    explicit SecureStringGuard(std::string& value) : value_(&value)
    {
    }

    ~SecureStringGuard()
    {
        if (value_)
        {
            secure_clear(*value_);
        }
    }

    SecureStringGuard(const SecureStringGuard&) = delete;
    SecureStringGuard& operator=(const SecureStringGuard&) = delete;
};

inline bool valid_header_name(const std::string& name)
{
    if (name.empty())
    {
        return false;
    }
    for (size_t i = 0; i < name.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(name[i]);
        bool alnum = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        bool punctuation = c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
        if (!alnum && !punctuation)
        {
            return false;
        }
    }
    return true;
}

inline bool valid_header_value(const std::string& value)
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\r' || value[i] == '\n' || value[i] == '\0')
        {
            return false;
        }
    }
    return true;
}

inline std::string normalize_encoding_name(const std::string& encoding)
{
    std::string value = trim_ascii(encoding);
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\'')))
    {
        value = value.substr(1, value.size() - 2);
    }
    value = ascii_lower_copy(value);
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '_')
        {
            value[i] = '-';
        }
    }
    return value;
}

inline const char* encoding_iconv_name(const std::string& encoding)
{
    std::string value = normalize_encoding_name(encoding);
    if (value == "utf-8" || value == "utf8")
    {
        return "UTF-8";
    }
    if (value == "gbk" || value == "gb2312" || value == "cp936")
    {
        return "GBK";
    }
    if (value == "gb18030" || value == "cp54936")
    {
        return "GB18030";
    }
    if (value == "big5" || value == "big-5" || value == "cp950")
    {
        return "BIG5";
    }
    if (value == "shift-jis" || value == "shiftjis" || value == "sjis" || value == "cp932")
    {
        return "SHIFT-JIS";
    }
    if (value == "euc-kr" || value == "cp949")
    {
        return "EUC-KR";
    }
    if (value == "iso-8859-1" || value == "latin1" || value == "latin-1")
    {
        return "ISO-8859-1";
    }
    if (value == "windows-1252" || value == "cp1252")
    {
        return "WINDOWS-1252";
    }
    return NULL;
}

inline const char* codepage_iconv_name(int codepage)
{
    switch (codepage)
    {
    case 65001:
        return "UTF-8";
    case 936:
        return "GBK";
    case 54936:
        return "GB18030";
    case 950:
        return "BIG5";
    case 932:
        return "SHIFT-JIS";
    case 949:
        return "EUC-KR";
    case 28591:
        return "ISO-8859-1";
    case 1252:
        return "WINDOWS-1252";
    default:
        return NULL;
    }
}

inline bool iconv_convert(const std::string& src, const char* from, const char* to, std::string& out)
{
    out.clear();
    if (!from || !to)
    {
        return false;
    }
    iconv_t converter = iconv_open(to, from);
    if (converter == reinterpret_cast<iconv_t>(-1))
    {
        return false;
    }
    size_t input_left = src.size();
    char* input = input_left ? const_cast<char*>(src.data()) : NULL;
    std::vector<char> buffer((std::max)(static_cast<size_t>(64), src.size() * 4 + 32));
    char* output = buffer.data();
    size_t output_left = buffer.size();
    while (input_left > 0)
    {
        size_t rc = iconv(converter, &input, &input_left, &output, &output_left);
        if (rc != static_cast<size_t>(-1))
        {
            continue;
        }
        if (errno != E2BIG)
        {
            iconv_close(converter);
            return false;
        }
        size_t used = static_cast<size_t>(output - buffer.data());
        buffer.resize(buffer.size() * 2 + 64);
        output = buffer.data() + used;
        output_left = buffer.size() - used;
    }
    size_t used = static_cast<size_t>(output - buffer.data());
    out.assign(buffer.data(), used);
    iconv_close(converter);
    return true;
}

inline std::string extract_charset(const std::string& source)
{
    std::string lower = ascii_lower_copy(source);
    size_t pos = 0;
    while ((pos = lower.find("charset", pos)) != std::string::npos)
    {
        size_t p = pos + 7;
        while (p < lower.size() && std::isspace(static_cast<unsigned char>(lower[p])))
        {
            ++p;
        }
        if (p >= lower.size() || lower[p] != '=')
        {
            pos = p;
            continue;
        }
        ++p;
        while (p < source.size() && std::isspace(static_cast<unsigned char>(source[p])))
        {
            ++p;
        }
        if (p >= source.size())
        {
            return std::string();
        }
        char quote = 0;
        if (source[p] == '"' || source[p] == '\'')
        {
            quote = source[p];
            ++p;
        }
        size_t start = p;
        while (p < source.size())
        {
            unsigned char ch = static_cast<unsigned char>(source[p]);
            if (quote != 0)
            {
                if (source[p] == quote)
                {
                    break;
                }
            }
            else if (std::isspace(ch) || source[p] == ';' || source[p] == '>' || source[p] == '/')
            {
                break;
            }
            ++p;
        }
        if (p > start)
        {
            return source.substr(start, p - start);
        }
        pos = p + 1;
    }
    return std::string();
}

inline std::string detect_html_charset(const std::string& body)
{
    size_t scan = (std::min)(body.size(), static_cast<size_t>(16384));
    return extract_charset(body.substr(0, scan));
}

inline bool is_json_content_type(const std::string& content_type)
{
    std::string value = ascii_lower_copy(content_type);
    return value.find("application/json") != std::string::npos || value.find("+json") != std::string::npos;
}

inline bool is_text_content_type(const std::string& content_type)
{
    if (content_type.empty())
    {
        return false;
    }
    std::string value = ascii_lower_copy(content_type);
    return value.find("text/") == 0 || value.find("application/json") != std::string::npos || value.find("+json") != std::string::npos || value.find("application/xml") != std::string::npos || value.find("+xml") != std::string::npos || value.find("application/javascript") != std::string::npos || value.find("application/x-javascript") != std::string::npos || value.find("application/x-www-form-urlencoded") != std::string::npos || value.find("application/graphql") != std::string::npos;
}

inline bool looks_like_text(const std::string& body)
{
    if (body.empty())
    {
        return false;
    }
    size_t i = 0;
    if (body.size() >= 3 && static_cast<unsigned char>(body[0]) == 0xEF && static_cast<unsigned char>(body[1]) == 0xBB && static_cast<unsigned char>(body[2]) == 0xBF)
    {
        i = 3;
    }
    while (i < body.size() && std::isspace(static_cast<unsigned char>(body[i])))
    {
        ++i;
    }
    if (i >= body.size())
    {
        return true;
    }
    char c = body[i];
    return c == '<' || c == '{' || c == '[' || c == '"' || c == '\'';
}

inline bool prepare_request_body(const std::string& body, const std::string& content_type, std::string& prepared, std::string& error)
{
    prepared = body;
    error.clear();
    if (body.empty() || !is_text_content_type(content_type))
    {
        return true;
    }
    if (is_json_content_type(content_type))
    {
        if (is_valid_utf8(body))
        {
            return true;
        }
        error = text_pair("JSON request body must be valid UTF-8", L"JSON 请求体必须是有效的 UTF-8 文本");
        return false;
    }
    std::string charset = extract_charset(content_type);
    if (!charset.empty())
    {
        const char* source = encoding_iconv_name(charset);
        if (source && std::strcmp(source, "UTF-8") != 0)
        {
            if (iconv_convert(body, source, "UTF-8", prepared))
            {
                return true;
            }
        }
    }
    if (is_valid_utf8(body))
    {
        return true;
    }
    error = text_pair("Request body is not valid UTF-8", L"请求体不是有效的 UTF-8 文本");
    return false;
}

inline std::string normalize_response_body(const std::string& body, const std::string& content_type)
{
    if (body.empty())
    {
        return body;
    }
    if (!is_text_content_type(content_type) && !looks_like_text(body))
    {
        return body;
    }
    std::string data = body;
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xEF && static_cast<unsigned char>(data[1]) == 0xBB && static_cast<unsigned char>(data[2]) == 0xBF)
    {
        data.erase(0, 3);
    }
    if (is_valid_utf8(data))
    {
        return data;
    }
    std::string charset = extract_charset(content_type);
    if (charset.empty())
    {
        charset = detect_html_charset(data);
    }
    const char* source = encoding_iconv_name(charset);
    std::string converted;
    if (source && iconv_convert(data, source, "UTF-8", converted))
    {
        return converted;
    }
    return data;
}

inline bool parse_uint64(const std::string& value, uint64_t& out)
{
    std::string s = trim_ascii(value);
    if (s.empty())
    {
        return false;
    }
    uint64_t n = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] < '0' || s[i] > '9')
        {
            return false;
        }
        uint64_t digit = static_cast<uint64_t>(s[i] - '0');
        if (n > ((std::numeric_limits<uint64_t>::max)() - digit) / 10ULL)
        {
            return false;
        }
        n = n * 10ULL + digit;
    }
    out = n;
    return true;
}

inline bool parse_int64(const std::string& value, int64_t& out)
{
    std::string s = trim_ascii(value);
    if (s.empty())
    {
        return false;
    }
    bool negative = false;
    size_t index = 0;
    if (s[0] == '+' || s[0] == '-')
    {
        negative = s[0] == '-';
        index = 1;
        if (index == s.size())
        {
            return false;
        }
    }
    uint64_t n = 0;
    for (; index < s.size(); ++index)
    {
        if (s[index] < '0' || s[index] > '9')
        {
            return false;
        }
        uint64_t digit = static_cast<uint64_t>(s[index] - '0');
        if (n > ((std::numeric_limits<uint64_t>::max)() - digit) / 10ULL)
        {
            return false;
        }
        n = n * 10ULL + digit;
    }
    if (negative)
    {
        uint64_t limit = static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()) + 1ULL;
        if (n > limit)
        {
            return false;
        }
        out = n == limit ? (std::numeric_limits<int64_t>::min)() : -static_cast<int64_t>(n);
    }
    else
    {
        if (n > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()))
        {
            return false;
        }
        out = static_cast<int64_t>(n);
    }
    return true;
}

struct ParsedUrl
{
    bool secure;
    std::wstring scheme_text;
    std::wstring host;
    uint16_t port;
    std::wstring path_query;
    std::wstring path_only;
    std::wstring absolute;
};

inline bool valid_port_number(const std::wstring& text, uint16_t& port)
{
    if (text.empty())
    {
        return false;
    }
    unsigned long value = 0;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] < L'0' || text[i] > L'9')
        {
            return false;
        }
        value = value * 10UL + static_cast<unsigned long>(text[i] - L'0');
        if (value > 65535UL)
        {
            return false;
        }
    }
    if (value == 0)
    {
        return false;
    }
    port = static_cast<uint16_t>(value);
    return true;
}

inline bool parse_url(const std::wstring& input, ParsedUrl& out, std::string& error, bool* unsupported_scheme = NULL)
{
    error.clear();
    if (unsupported_scheme)
    {
        *unsupported_scheme = false;
    }
    if (input.empty())
    {
        error = text_pair("URL cannot be empty", L"URL 不能为空");
        return false;
    }
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] < 0x20 || input[i] == 0x7F)
        {
            error = text(L"URL 无效");
            return false;
        }
    }
    size_t scheme_end = input.find(L"://");
    if (scheme_end == std::wstring::npos)
    {
        error = text(L"URL 无效");
        return false;
    }
    std::wstring scheme = ascii_lower_copy(input.substr(0, scheme_end));
    if (scheme != L"http" && scheme != L"https")
    {
        if (unsupported_scheme)
        {
            *unsupported_scheme = true;
        }
        error = text_pair("Only HTTP and HTTPS URLs are supported", L"仅支持 HTTP 和 HTTPS URL");
        return false;
    }
    size_t authority_start = scheme_end + 3;
    size_t authority_end = input.find_first_of(L"/?#", authority_start);
    if (authority_end == std::wstring::npos)
    {
        authority_end = input.size();
    }
    std::wstring authority = input.substr(authority_start, authority_end - authority_start);
    if (authority.empty() || authority.find(L'@') != std::wstring::npos)
    {
        error = text_pair("URL has an invalid authority", L"URL 服务器地址无效");
        return false;
    }
    std::wstring host;
    uint16_t port = scheme == L"https" ? 443 : 80;
    if (authority[0] == L'[')
    {
        size_t close = authority.find(L']');
        if (close == std::wstring::npos || close == 1)
        {
            error = text(L"URL 无效");
            return false;
        }
        host = authority.substr(1, close - 1);
        if (close + 1 < authority.size())
        {
            if (authority[close + 1] != L':' || !valid_port_number(authority.substr(close + 2), port))
            {
                error = text(L"URL 无效");
                return false;
            }
        }
    }
    else
    {
        size_t colon = authority.find_last_of(L':');
        if (colon != std::wstring::npos)
        {
            if (authority.find(L':') != colon)
            {
                error = text_pair("IPv6 URL hosts must use brackets", L"IPv6 URL 主机必须使用方括号");
                return false;
            }
            host = authority.substr(0, colon);
            if (!valid_port_number(authority.substr(colon + 1), port))
            {
                error = text(L"URL 无效");
                return false;
            }
        }
        else
        {
            host = authority;
        }
    }
    host = ascii_lower_copy(host);
    if (host.empty())
    {
        error = text_pair("URL has no valid host", L"URL 中没有有效的服务器地址");
        return false;
    }
    std::wstring rest = authority_end < input.size() ? input.substr(authority_end) : L"/";
    size_t fragment = rest.find(L'#');
    if (fragment != std::wstring::npos)
    {
        rest.erase(fragment);
    }
    if (rest.empty())
    {
        rest = L"/";
    }
    if (rest[0] == L'?')
    {
        rest.insert(rest.begin(), L'/');
    }
    if (rest[0] != L'/')
    {
        rest.insert(rest.begin(), L'/');
    }
    size_t query = rest.find(L'?');
    std::wstring path_only = query == std::wstring::npos ? rest : rest.substr(0, query);
    if (path_only.empty())
    {
        path_only = L"/";
    }
    out.secure = scheme == L"https";
    out.scheme_text = scheme;
    out.host = host;
    out.port = port;
    out.path_query = rest;
    out.path_only = path_only;
    out.absolute = scheme + L"://";
    if (host.find(L':') != std::wstring::npos)
    {
        out.absolute += L"[" + host + L"]";
    }
    else
    {
        out.absolute += host;
    }
    bool default_port = (out.secure && port == 443) || (!out.secure && port == 80);
    if (!default_port)
    {
        std::wostringstream stream;
        stream << L":" << port;
        out.absolute += stream.str();
    }
    out.absolute += rest;
    return true;
}

inline std::wstring origin_of(const ParsedUrl& url)
{
    std::wstring result = url.scheme_text + L"://";
    if (url.host.find(L':') != std::wstring::npos)
    {
        result += L"[" + url.host + L"]";
    }
    else
    {
        result += url.host;
    }
    bool default_port = (url.secure && url.port == 443) || (!url.secure && url.port == 80);
    if (!default_port)
    {
        std::wostringstream stream;
        stream << L":" << url.port;
        result += stream.str();
    }
    return result;
}

inline std::wstring remove_dot_segments(const std::wstring& path)
{
    bool leading = !path.empty() && path[0] == L'/';
    bool trailing = !path.empty() && path[path.size() - 1] == L'/';
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (start <= path.size())
    {
        size_t slash = path.find(L'/', start);
        std::wstring part = path.substr(start, slash == std::wstring::npos ? std::wstring::npos : slash - start);
        if (part == L"..")
        {
            if (!parts.empty())
            {
                parts.pop_back();
            }
        }
        else if (!part.empty() && part != L".")
        {
            parts.push_back(part);
        }
        if (slash == std::wstring::npos)
        {
            break;
        }
        start = slash + 1;
    }
    std::wstring result = leading ? L"/" : L"";
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
        {
            result += L"/";
        }
        result += parts[i];
    }
    if (trailing && !result.empty() && result[result.size() - 1] != L'/')
    {
        result += L"/";
    }
    if (result.empty())
    {
        result = leading ? L"/" : L".";
    }
    return result;
}

inline bool resolve_redirect(const ParsedUrl& base, const std::string& location_utf8, std::wstring& result, std::string& error)
{
    std::wstring location;
    if (!utf8_to_wide(trim_ascii(location_utf8), location) || location.empty())
    {
        error = text_pair("Redirect Location is invalid UTF-8", L"重定向 Location 不是有效的 UTF-8 文本");
        return false;
    }
    size_t fragment = location.find(L'#');
    if (fragment != std::wstring::npos)
    {
        location.erase(fragment);
    }
    if (location.find(L"://") != std::wstring::npos)
    {
        result = location;
        return true;
    }
    if (location.size() >= 2 && location[0] == L'/' && location[1] == L'/')
    {
        result = base.scheme_text + L":" + location;
        return true;
    }
    std::wstring origin = origin_of(base);
    if (!location.empty() && location[0] == L'/')
    {
        size_t q = location.find(L'?');
        std::wstring path = q == std::wstring::npos ? location : location.substr(0, q);
        std::wstring query = q == std::wstring::npos ? L"" : location.substr(q);
        result = origin + remove_dot_segments(path) + query;
        return true;
    }
    if (!location.empty() && location[0] == L'?')
    {
        result = origin + base.path_only + location;
        return true;
    }
    std::wstring base_dir = base.path_only;
    size_t slash = base_dir.find_last_of(L'/');
    if (slash == std::wstring::npos)
    {
        base_dir = L"/";
    }
    else
    {
        base_dir.erase(slash + 1);
    }
    size_t q = location.find(L'?');
    std::wstring relative = q == std::wstring::npos ? location : location.substr(0, q);
    std::wstring query = q == std::wstring::npos ? L"" : location.substr(q);
    result = origin + remove_dot_segments(base_dir + relative) + query;
    return true;
}

inline bool same_origin(const ParsedUrl& a, const ParsedUrl& b)
{
    return a.secure == b.secure && a.port == b.port && ascii_iequals(a.host, b.host);
}

inline bool is_redirect_status(int code)
{
    return code == 301 || code == 302 || code == 303 || code == 307 || code == 308;
}

inline bool is_retryable_http_status(int code)
{
    return code == 408 || code == 425 || code == 429 || code == 500 || code == 502 || code == 503 || code == 504;
}

inline bool is_idempotent_method(const std::wstring& method)
{
    std::wstring value = ascii_lower_copy(method);
    return value == L"get" || value == L"head" || value == L"put" || value == L"delete" || value == L"options" || value == L"trace";
}

inline time_t timegm_portable(struct tm* value)
{
    return timegm(value);
}

inline bool parse_http_time(const std::string& value, time_t& out)
{
    static const char* formats[] = {
        "%a, %d %b %Y %H:%M:%S GMT",
        "%A, %d-%b-%y %H:%M:%S GMT",
        "%a %b %e %H:%M:%S %Y"
    };
    for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i)
    {
        struct tm parsed = {};
        char* end = strptime(value.c_str(), formats[i], &parsed);
        if (end && *trim_ascii(end).c_str() == '\0')
        {
            time_t result = timegm_portable(&parsed);
            if (result != static_cast<time_t>(-1))
            {
                out = result;
                return true;
            }
        }
    }
    return false;
}

inline std::string http_error_message(int status)
{
    std::ostringstream stream;
    if (current_language() == "zh_CN")
    {
        stream << "HTTP 请求失败，状态码 ";
    }
    else
    {
        stream << "HTTP request failed: status code ";
    }
    stream << status;
    return stream.str();
}

inline std::string attempts_suffix(int attempts)
{
    if (attempts <= 1)
    {
        return std::string();
    }
    std::ostringstream stream;
    if (current_language() == "zh_CN")
    {
        stream << "，尝试次数 " << attempts;
    }
    else
    {
        stream << ", attempts " << attempts;
    }
    return stream.str();
}

inline std::wstring format_download_bytes(uint64_t bytes)
{
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB" };
    double value = static_cast<double>(bytes);
    size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < sizeof(units) / sizeof(units[0]))
    {
        value /= 1024.0;
        ++unit;
    }
    std::wostringstream stream;
    if (unit == 0)
    {
        stream << bytes;
    }
    else if (value >= 100.0)
    {
        stream << std::fixed << std::setprecision(0) << value;
    }
    else if (value >= 10.0)
    {
        stream << std::fixed << std::setprecision(1) << value;
    }
    else
    {
        stream << std::fixed << std::setprecision(2) << value;
    }
    stream << L" " << units[unit];
    return stream.str();
}

inline std::wstring format_download_seconds(double seconds)
{
    if (!(seconds >= 0.0) || seconds > static_cast<double>((std::numeric_limits<uint64_t>::max)()))
    {
        return L"--:--";
    }
    uint64_t total = static_cast<uint64_t>(seconds + 0.5);
    uint64_t hours = total / 3600ULL;
    uint64_t minutes = (total % 3600ULL) / 60ULL;
    uint64_t secs = total % 60ULL;
    std::wostringstream stream;
    stream << std::setfill(L'0');
    if (hours > 0)
    {
        stream << std::setw(2) << hours << L":" << std::setw(2) << minutes << L":" << std::setw(2) << secs;
    }
    else
    {
        stream << std::setw(2) << minutes << L":" << std::setw(2) << secs;
    }
    return stream.str();
}

}

using DWORD = unsigned long;

static const DWORD WINHTTP_ACCESS_TYPE_DEFAULT_PROXY = 0;

enum class ErrorCode
{
    None,
    InvalidArgument,
    InvalidUrl,
    UnsupportedScheme,
    InvalidHeader,
    EncodingError,
    SessionOpenFailed,
    ConnectionFailed,
    RequestOpenFailed,
    Timeout,
    NameResolutionFailed,
    TlsFailure,
    SendFailed,
    ReceiveFailed,
    ReadFailed,
    HttpError,
    ResponseTooLarge,
    RedirectLimitExceeded,
    RedirectError,
    FileExists,
    FileOpenFailed,
    FileWriteFailed,
    FileCommitFailed,
    Cancelled,
    WinHttpError
};

struct Headers
{
    std::unordered_map<std::string, std::string> fields;

    Headers()
    {
    }

    Headers(const Headers& other) : fields(other.fields)
    {
    }

    Headers& operator=(const Headers& other)
    {
        if (this != &other)
        {
            clear();
            fields = other.fields;
        }
        return *this;
    }

    Headers(Headers&& other) noexcept : fields(std::move(other.fields))
    {
        other.fields.clear();
    }

    Headers& operator=(Headers&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            fields = std::move(other.fields);
            other.fields.clear();
        }
        return *this;
    }

    ~Headers()
    {
        clear();
    }

    void set(const std::string& key, const std::string& value)
    {
        for (std::unordered_map<std::string, std::string>::iterator it = fields.begin(); it != fields.end(); ++it)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                detail::secure_clear(it->second);
                it->second = value;
                return;
            }
        }
        fields[key] = value;
    }

    std::string get(const std::string& key) const
    {
        for (std::unordered_map<std::string, std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                return it->second;
            }
        }
        return std::string();
    }

    bool contains(const std::string& key) const
    {
        for (std::unordered_map<std::string, std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                return true;
            }
        }
        return false;
    }

    void erase(const std::string& key)
    {
        for (std::unordered_map<std::string, std::string>::iterator it = fields.begin(); it != fields.end();)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                detail::secure_clear(it->second);
                it = fields.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void clear()
    {
        for (std::unordered_map<std::string, std::string>::iterator it = fields.begin(); it != fields.end(); ++it)
        {
            detail::secure_clear(it->second);
        }
        fields.clear();
    }

    std::string to_winhttp_string() const
    {
        std::string result;
        for (std::unordered_map<std::string, std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it)
        {
            result += it->first + ": " + it->second + "\r\n";
        }
        return result;
    }
};

struct ResponseHeaders
{
    std::unordered_map<std::string, std::vector<std::string> > fields;

    ResponseHeaders()
    {
    }

    ResponseHeaders(const ResponseHeaders& other) : fields(other.fields)
    {
    }

    ResponseHeaders& operator=(const ResponseHeaders& other)
    {
        if (this != &other)
        {
            clear();
            fields = other.fields;
        }
        return *this;
    }

    ResponseHeaders(ResponseHeaders&& other) noexcept : fields(std::move(other.fields))
    {
        other.fields.clear();
    }

    ResponseHeaders& operator=(ResponseHeaders&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            fields = std::move(other.fields);
            other.fields.clear();
        }
        return *this;
    }

    ~ResponseHeaders()
    {
        clear();
    }

    void add(const std::string& key, const std::string& value)
    {
        fields[detail::ascii_lower_copy(key)].push_back(value);
    }

    std::string get(const std::string& key) const
    {
        std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = fields.find(detail::ascii_lower_copy(key));
        if (it == fields.end() || it->second.empty())
        {
            return std::string();
        }
        return it->second.front();
    }

    std::vector<std::string> get_all(const std::string& key) const
    {
        std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = fields.find(detail::ascii_lower_copy(key));
        return it == fields.end() ? std::vector<std::string>() : it->second;
    }

    bool contains(const std::string& key) const
    {
        return fields.find(detail::ascii_lower_copy(key)) != fields.end();
    }

    void clear()
    {
        for (std::unordered_map<std::string, std::vector<std::string> >::iterator it = fields.begin(); it != fields.end(); ++it)
        {
            for (size_t i = 0; i < it->second.size(); ++i)
            {
                detail::secure_clear(it->second[i]);
            }
        }
        fields.clear();
    }
};

struct RetryPolicy
{
    int retries = 0;
    int initial_delay_ms = 250;
    int max_delay_ms = 4000;
    double multiplier = 2.0;
    bool jitter = true;
    bool respect_retry_after = true;
    bool retry_non_idempotent = false;
    bool allow_automatic_authentication = false;
};

struct RequestOptions
{
    int timeout_ms = 0;
    bool follow_redirect = true;
    int max_redirects = -1;
    size_t max_response_size = 0;
    RetryPolicy retry;
};

struct SessionOptions
{
    std::wstring user_agent = L"FeatherCrawl/2.0";
    bool enable_cookies = true;
    DWORD access_type = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    std::wstring proxy;
    std::wstring proxy_bypass;
    size_t max_response_size = 64ULL * 1024ULL * 1024ULL;
    int max_redirects = 10;
    size_t max_connections = 64;
};

struct DownloadOptions
{
    RequestOptions request;
    uint64_t max_file_size = 1024ULL * 1024ULL * 1024ULL;
    bool overwrite = false;
    bool show_progress = true;
    size_t progress_bar_width = 20;
    unsigned int progress_refresh_ms = 100;
};

struct Response
{
    int status_code = 0;
    std::string body;
    ResponseHeaders headers;
    ErrorCode error_code = ErrorCode::None;
    DWORD native_error_code = 0;
    std::string error_message = detail::text(L"无");
    size_t received_bytes = 0;
    int attempts = 0;
    int redirect_count = 0;
    std::wstring final_url;

    bool ok() const
    {
        return error_code == ErrorCode::None && status_code >= 200 && status_code < 300;
    }
};

struct DownloadResult
{
    int status_code = 0;
    ResponseHeaders headers;
    ErrorCode error_code = ErrorCode::None;
    DWORD native_error_code = 0;
    std::string error_message = detail::text(L"无");
    uint64_t bytes_written = 0;
    uint64_t file_size = 0;
    bool file_size_known = false;
    int attempts = 0;
    int redirect_count = 0;
    std::wstring final_url;
    std::wstring destination;

    bool ok() const
    {
        return error_code == ErrorCode::None && status_code >= 200 && status_code < 300;
    }
};

class CookieJar
{
    struct Cookie
    {
        std::string name;
        std::string value;
        std::wstring domain;
        std::wstring path;
        bool host_only;
        bool secure;
        bool http_only;
        bool persistent;
        time_t expires;
        uint64_t sequence;
    };

    mutable std::mutex mutex_;
    std::vector<Cookie> cookies_;
    uint64_t sequence_;

    static bool is_ip_host(const std::wstring& host)
    {
        if (host.find(L':') != std::wstring::npos)
        {
            return true;
        }
        if (host.empty())
        {
            return false;
        }
        bool dot = false;
        for (size_t i = 0; i < host.size(); ++i)
        {
            if (host[i] == L'.')
            {
                dot = true;
                continue;
            }
            if (host[i] < L'0' || host[i] > L'9')
            {
                return false;
            }
        }
        return dot;
    }

    static bool domain_matches(const std::wstring& host, const std::wstring& domain)
    {
        if (detail::ascii_iequals(host, domain))
        {
            return true;
        }
        if (is_ip_host(host) || host.size() <= domain.size())
        {
            return false;
        }
        size_t start = host.size() - domain.size();
        return start > 0 && host[start - 1] == L'.' && detail::ascii_iequals(host.substr(start), domain);
    }

    static std::wstring default_path(const std::wstring& request_path)
    {
        if (request_path.empty() || request_path[0] != L'/')
        {
            return L"/";
        }
        size_t slash = request_path.find_last_of(L'/');
        if (slash == 0 || slash == std::wstring::npos)
        {
            return L"/";
        }
        return request_path.substr(0, slash);
    }

    static bool path_matches(const std::wstring& request_path, const std::wstring& cookie_path)
    {
        if (request_path == cookie_path)
        {
            return true;
        }
        if (request_path.size() < cookie_path.size())
        {
            return false;
        }
        if (request_path.compare(0, cookie_path.size(), cookie_path) != 0)
        {
            return false;
        }
        if (!cookie_path.empty() && cookie_path[cookie_path.size() - 1] == L'/')
        {
            return true;
        }
        return request_path.size() > cookie_path.size() && request_path[cookie_path.size()] == L'/';
    }

    static bool valid_cookie_name(const std::string& name)
    {
        return detail::valid_header_name(name);
    }

    static bool valid_cookie_value(const std::string& value)
    {
        for (size_t i = 0; i < value.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(value[i]);
            if (c < 0x20 || c == 0x7F || c == ';' || c == '\r' || c == '\n' || c == '\0')
            {
                return false;
            }
        }
        return true;
    }

    static void wipe_cookie(Cookie& cookie)
    {
        detail::secure_clear(cookie.name);
        detail::secure_clear(cookie.value);
        cookie.domain.assign(cookie.domain.size(), L'\0');
        cookie.domain.clear();
        cookie.path.assign(cookie.path.size(), L'\0');
        cookie.path.clear();
        cookie.expires = 0;
        cookie.sequence = 0;
    }

    void erase_at_locked(size_t index)
    {
        wipe_cookie(cookies_[index]);
        cookies_.erase(cookies_.begin() + static_cast<std::ptrdiff_t>(index));
    }

    void remove_expired_locked(time_t now)
    {
        for (size_t i = cookies_.size(); i > 0; --i)
        {
            if (cookies_[i - 1].persistent && cookies_[i - 1].expires <= now)
            {
                erase_at_locked(i - 1);
            }
        }
    }

    void remove_exact_locked(const std::string& name, const std::wstring& domain, const std::wstring& path)
    {
        for (size_t i = cookies_.size(); i > 0; --i)
        {
            const Cookie& cookie = cookies_[i - 1];
            if (cookie.name == name && detail::ascii_iequals(cookie.domain, domain) && cookie.path == path)
            {
                erase_at_locked(i - 1);
            }
        }
    }

public:
    CookieJar() : sequence_(0)
    {
    }

    ~CookieJar()
    {
        clear();
    }

    CookieJar(const CookieJar&) = delete;
    CookieJar& operator=(const CookieJar&) = delete;

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < cookies_.size(); ++i)
        {
            wipe_cookie(cookies_[i]);
        }
        cookies_.clear();
        sequence_ = 0;
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        remove_expired_locked(std::time(NULL));
        return cookies_.size();
    }

    bool delete_cookie(const std::string& name, const std::string& domain_utf8 = std::string(), const std::string& path_utf8 = std::string())
    {
        std::wstring domain;
        std::wstring path;
        if (!domain_utf8.empty() && !detail::utf8_to_wide(domain_utf8, domain))
        {
            return false;
        }
        if (!path_utf8.empty() && !detail::utf8_to_wide(path_utf8, path))
        {
            return false;
        }
        domain = detail::ascii_lower_copy(domain);
        while (!domain.empty() && domain[0] == L'.')
        {
            domain.erase(domain.begin());
        }
        std::lock_guard<std::mutex> lock(mutex_);
        bool removed = false;
        for (size_t i = cookies_.size(); i > 0; --i)
        {
            const Cookie& cookie = cookies_[i - 1];
            if (cookie.name != name)
            {
                continue;
            }
            if (!domain.empty() && !detail::ascii_iequals(cookie.domain, domain))
            {
                continue;
            }
            if (!path.empty() && cookie.path != path)
            {
                continue;
            }
            erase_at_locked(i - 1);
            removed = true;
        }
        return removed;
    }

    size_t clear_domain(const std::string& domain_utf8)
    {
        std::wstring domain;
        if (!detail::utf8_to_wide(domain_utf8, domain))
        {
            return 0;
        }
        domain = detail::ascii_lower_copy(domain);
        while (!domain.empty() && domain[0] == L'.')
        {
            domain.erase(domain.begin());
        }
        std::lock_guard<std::mutex> lock(mutex_);
        size_t removed = 0;
        for (size_t i = cookies_.size(); i > 0; --i)
        {
            if (detail::ascii_iequals(cookies_[i - 1].domain, domain))
            {
                erase_at_locked(i - 1);
                ++removed;
            }
        }
        return removed;
    }

    void store(const detail::ParsedUrl& origin, const std::string& set_cookie)
    {
        if (set_cookie.empty() || set_cookie.size() > 4096)
        {
            return;
        }
        std::vector<std::string> parts;
        size_t start = 0;
        while (start <= set_cookie.size())
        {
            size_t semicolon = set_cookie.find(';', start);
            parts.push_back(detail::trim_ascii(set_cookie.substr(start, semicolon == std::string::npos ? std::string::npos : semicolon - start)));
            if (semicolon == std::string::npos)
            {
                break;
            }
            start = semicolon + 1;
        }
        if (parts.empty())
        {
            return;
        }
        size_t equals = parts[0].find('=');
        if (equals == std::string::npos || equals == 0)
        {
            return;
        }
        Cookie cookie;
        cookie.name = detail::trim_ascii(parts[0].substr(0, equals));
        cookie.value = detail::trim_ascii(parts[0].substr(equals + 1));
        if (!valid_cookie_name(cookie.name) || !valid_cookie_value(cookie.value))
        {
            return;
        }
        cookie.domain = origin.host;
        cookie.path = default_path(origin.path_only);
        cookie.host_only = true;
        cookie.secure = false;
        cookie.http_only = false;
        cookie.persistent = false;
        cookie.expires = 0;
        cookie.sequence = 0;
        bool has_domain = false;
        bool has_path = false;
        bool has_max_age = false;
        int64_t max_age = 0;
        for (size_t i = 1; i < parts.size(); ++i)
        {
            if (parts[i].empty())
            {
                continue;
            }
            size_t attribute_equals = parts[i].find('=');
            std::string name = detail::ascii_lower_copy(detail::trim_ascii(parts[i].substr(0, attribute_equals)));
            std::string value = attribute_equals == std::string::npos ? std::string() : detail::trim_ascii(parts[i].substr(attribute_equals + 1));
            if (name == "secure")
            {
                cookie.secure = true;
            }
            else if (name == "httponly")
            {
                cookie.http_only = true;
            }
            else if (name == "domain")
            {
                std::wstring domain;
                if (!detail::utf8_to_wide(value, domain))
                {
                    return;
                }
                domain = detail::ascii_lower_copy(detail::trim_ascii(domain));
                while (!domain.empty() && domain[0] == L'.')
                {
                    domain.erase(domain.begin());
                }
                if (domain.empty() || !domain_matches(origin.host, domain))
                {
                    return;
                }
                cookie.domain = domain;
                cookie.host_only = false;
                has_domain = true;
            }
            else if (name == "path")
            {
                std::wstring path;
                if (detail::utf8_to_wide(value, path) && !path.empty() && path[0] == L'/')
                {
                    cookie.path = path;
                    has_path = true;
                }
            }
            else if (name == "max-age")
            {
                if (detail::parse_int64(value, max_age))
                {
                    has_max_age = true;
                }
            }
            else if (name == "expires" && !has_max_age)
            {
                time_t expires = 0;
                if (detail::parse_http_time(value, expires))
                {
                    cookie.persistent = true;
                    cookie.expires = expires;
                }
            }
        }
        if (has_max_age)
        {
            cookie.persistent = true;
            if (max_age <= 0)
            {
                cookie.expires = 0;
            }
            else
            {
                time_t now = std::time(NULL);
                if (max_age > static_cast<int64_t>((std::numeric_limits<time_t>::max)() - now))
                {
                    cookie.expires = (std::numeric_limits<time_t>::max)();
                }
                else
                {
                    cookie.expires = now + static_cast<time_t>(max_age);
                }
            }
        }
        if (cookie.secure && !origin.secure)
        {
            return;
        }
        if (cookie.name.find("__Secure-") == 0 && (!cookie.secure || !origin.secure))
        {
            return;
        }
        if (cookie.name.find("__Host-") == 0 && (!cookie.secure || !origin.secure || has_domain || cookie.path != L"/" || !has_path))
        {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        time_t now = std::time(NULL);
        remove_expired_locked(now);
        remove_exact_locked(cookie.name, cookie.domain, cookie.path);
        if (cookie.persistent && cookie.expires <= now)
        {
            return;
        }
        size_t same_domain = 0;
        size_t oldest_domain_index = 0;
        uint64_t oldest_domain_sequence = (std::numeric_limits<uint64_t>::max)();
        for (size_t i = 0; i < cookies_.size(); ++i)
        {
            if (detail::ascii_iequals(cookies_[i].domain, cookie.domain))
            {
                ++same_domain;
                if (cookies_[i].sequence < oldest_domain_sequence)
                {
                    oldest_domain_sequence = cookies_[i].sequence;
                    oldest_domain_index = i;
                }
            }
        }
        if (same_domain >= 180 && !cookies_.empty())
        {
            erase_at_locked(oldest_domain_index);
        }
        if (cookies_.size() >= 3000)
        {
            erase_at_locked(0);
        }
        cookie.sequence = ++sequence_;
        cookies_.push_back(cookie);
        wipe_cookie(cookie);
    }

    std::string header_for(const detail::ParsedUrl& target)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        remove_expired_locked(std::time(NULL));
        std::vector<const Cookie*> matches;
        for (size_t i = 0; i < cookies_.size(); ++i)
        {
            const Cookie& cookie = cookies_[i];
            bool domain_ok = cookie.host_only ? detail::ascii_iequals(target.host, cookie.domain) : domain_matches(target.host, cookie.domain);
            if (!domain_ok || !path_matches(target.path_only, cookie.path) || (cookie.secure && !target.secure))
            {
                continue;
            }
            matches.push_back(&cookie);
        }
        std::sort(matches.begin(), matches.end(), [](const Cookie* a, const Cookie* b)
        {
            if (a->path.size() != b->path.size())
            {
                return a->path.size() > b->path.size();
            }
            return a->sequence < b->sequence;
        });
        std::string result;
        for (size_t i = 0; i < matches.size(); ++i)
        {
            if (!result.empty())
            {
                result += "; ";
            }
            result += matches[i]->name + "=" + matches[i]->value;
        }
        return result;
    }
};

namespace detail
{

inline bool validate_headers(const Headers& headers, std::string& error)
{
    for (std::unordered_map<std::string, std::string>::const_iterator it = headers.fields.begin(); it != headers.fields.end(); ++it)
    {
        if (!valid_header_name(it->first) || !valid_header_value(it->second))
        {
            error = text_pair("Invalid HTTP header", L"HTTP 请求头无效");
            return false;
        }
    }
    return true;
}

inline bool is_ip_literal(const std::string& host)
{
    struct in_addr ipv4;
    struct in6_addr ipv6;
    return inet_pton(AF_INET, host.c_str(), &ipv4) == 1 || inet_pton(AF_INET6, host.c_str(), &ipv6) == 1;
}

inline bool parse_proxy(const std::wstring& proxy_text, std::string& host, uint16_t& port)
{
    host.clear();
    port = 0;
    if (proxy_text.empty())
    {
        return false;
    }
    std::wstring value = trim_ascii(proxy_text);
    size_t scheme = value.find(L"://");
    if (scheme != std::wstring::npos)
    {
        std::wstring prefix = ascii_lower_copy(value.substr(0, scheme));
        if (prefix != L"http")
        {
            return false;
        }
        value.erase(0, scheme + 3);
    }
    size_t slash = value.find(L'/');
    if (slash != std::wstring::npos)
    {
        value.erase(slash);
    }
    std::wstring whost;
    if (!value.empty() && value[0] == L'[')
    {
        size_t close = value.find(L']');
        if (close == std::wstring::npos)
        {
            return false;
        }
        whost = value.substr(1, close - 1);
        if (close + 1 >= value.size() || value[close + 1] != L':')
        {
            return false;
        }
        if (!valid_port_number(value.substr(close + 2), port))
        {
            return false;
        }
    }
    else
    {
        size_t colon = value.find_last_of(L':');
        if (colon == std::wstring::npos || colon == 0)
        {
            return false;
        }
        whost = value.substr(0, colon);
        if (!valid_port_number(value.substr(colon + 1), port))
        {
            return false;
        }
    }
    host = wide_to_utf8(whost);
    return !host.empty();
}

class SocketHandle
{
    int fd_;

public:
    explicit SocketHandle(int fd = -1) : fd_(fd)
    {
    }

    ~SocketHandle()
    {
        reset();
    }

    SocketHandle(const SocketHandle&) = delete;
    SocketHandle& operator=(const SocketHandle&) = delete;

    SocketHandle(SocketHandle&& other) noexcept : fd_(other.fd_)
    {
        other.fd_ = -1;
    }

    SocketHandle& operator=(SocketHandle&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const
    {
        return fd_;
    }

    int release()
    {
        int value = fd_;
        fd_ = -1;
        return value;
    }

    void reset(int fd = -1)
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
        }
        fd_ = fd;
    }

    explicit operator bool() const
    {
        return fd_ >= 0;
    }
};

inline bool set_socket_blocking(int fd, bool blocking)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        return false;
    }
    int updated = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(fd, F_SETFL, updated) == 0;
}

inline void set_socket_timeouts(int fd, int timeout_ms)
{
    if (timeout_ms <= 0)
    {
        return;
    }
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

inline bool connect_socket(const std::string& host, uint16_t port, int timeout_ms, SocketHandle& socket_handle, ErrorCode& error_code, DWORD& native_error, std::string& message)
{
    std::ostringstream service;
    service << port;
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo* addresses = NULL;
    int gai = getaddrinfo(host.c_str(), service.str().c_str(), &hints, &addresses);
    if (gai != 0)
    {
        error_code = ErrorCode::NameResolutionFailed;
        native_error = static_cast<DWORD>(gai);
        message = text_pair("Unable to resolve server name", L"无法解析服务器名称");
        return false;
    }

    int last_errno = 0;
    bool timeout_seen = false;
    for (struct addrinfo* address = addresses; address; address = address->ai_next)
    {
        SocketHandle candidate(::socket(address->ai_family, address->ai_socktype, address->ai_protocol));
        if (!candidate)
        {
            last_errno = errno;
            continue;
        }
        if (!set_socket_blocking(candidate.get(), false))
        {
            last_errno = errno;
            continue;
        }
        int rc = ::connect(candidate.get(), address->ai_addr, address->ai_addrlen);
        if (rc != 0 && errno != EINPROGRESS)
        {
            last_errno = errno;
            continue;
        }
        if (rc != 0)
        {
            struct pollfd descriptor;
            descriptor.fd = candidate.get();
            descriptor.events = POLLOUT;
            descriptor.revents = 0;
            int poll_timeout = timeout_ms > 0 ? timeout_ms : 30000;
            int poll_result = poll(&descriptor, 1, poll_timeout);
            if (poll_result == 0)
            {
                timeout_seen = true;
                last_errno = ETIMEDOUT;
                continue;
            }
            if (poll_result < 0)
            {
                last_errno = errno;
                continue;
            }
            int socket_error = 0;
            socklen_t socket_error_size = sizeof(socket_error);
            if (getsockopt(candidate.get(), SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_size) != 0 || socket_error != 0)
            {
                last_errno = socket_error ? socket_error : errno;
                continue;
            }
        }
        if (!set_socket_blocking(candidate.get(), true))
        {
            last_errno = errno;
            continue;
        }
        set_socket_timeouts(candidate.get(), timeout_ms);
        socket_handle = std::move(candidate);
        freeaddrinfo(addresses);
        return true;
    }
    freeaddrinfo(addresses);
    native_error = static_cast<DWORD>(last_errno);
    if (timeout_seen || last_errno == ETIMEDOUT)
    {
        error_code = ErrorCode::Timeout;
        message = text(L"请求超时");
    }
    else
    {
        error_code = ErrorCode::ConnectionFailed;
        message = text(L"无法连接到服务器");
    }
    return false;
}

inline bool socket_send_all(int fd, const char* data, size_t size, DWORD& native_error)
{
    size_t offset = 0;
    while (offset < size)
    {
        ssize_t sent = ::send(fd, data + offset, size - offset, MSG_NOSIGNAL);
        if (sent > 0)
        {
            offset += static_cast<size_t>(sent);
            continue;
        }
        if (sent < 0 && errno == EINTR)
        {
            continue;
        }
        native_error = static_cast<DWORD>(errno);
        return false;
    }
    return true;
}

inline ssize_t socket_receive(int fd, char* data, size_t size, DWORD& native_error)
{
    for (;;)
    {
        ssize_t result = ::recv(fd, data, size, 0);
        if (result < 0 && errno == EINTR)
        {
            continue;
        }
        if (result < 0)
        {
            native_error = static_cast<DWORD>(errno);
        }
        return result;
    }
}

struct TlsState
{
    SSL_CTX* context;
    SSL* ssl;

    TlsState() : context(NULL), ssl(NULL)
    {
    }

    ~TlsState()
    {
        if (ssl)
        {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
        if (context)
        {
            SSL_CTX_free(context);
        }
    }

    TlsState(const TlsState&) = delete;
    TlsState& operator=(const TlsState&) = delete;
};

inline void initialize_openssl()
{
    static std::once_flag once;
    std::call_once(once, []()
    {
        OPENSSL_init_ssl(0, NULL);
    });
}

inline bool start_tls(int fd, const std::string& host, TlsState& tls, DWORD& native_error, std::string& message)
{
    initialize_openssl();
    tls.context = SSL_CTX_new(TLS_client_method());
    if (!tls.context)
    {
        native_error = ERR_get_error();
        message = text(L"HTTPS 安全连接失败");
        return false;
    }
    SSL_CTX_set_verify(tls.context, SSL_VERIFY_PEER, NULL);
    if (SSL_CTX_set_default_verify_paths(tls.context) != 1)
    {
        native_error = ERR_get_error();
        message = text_pair("Unable to load system certificate authorities", L"无法加载系统证书颁发机构");
        return false;
    }
    tls.ssl = SSL_new(tls.context);
    if (!tls.ssl)
    {
        native_error = ERR_get_error();
        message = text(L"HTTPS 安全连接失败");
        return false;
    }
    if (SSL_set_fd(tls.ssl, fd) != 1)
    {
        native_error = ERR_get_error();
        message = text(L"HTTPS 安全连接失败");
        return false;
    }
    bool ip = is_ip_literal(host);
    if (ip)
    {
        X509_VERIFY_PARAM* param = SSL_get0_param(tls.ssl);
        if (!param || X509_VERIFY_PARAM_set1_ip_asc(param, host.c_str()) != 1)
        {
            native_error = ERR_get_error();
            message = text(L"HTTPS 安全连接失败");
            return false;
        }
    }
    else
    {
        if (SSL_set_tlsext_host_name(tls.ssl, host.c_str()) != 1)
        {
            native_error = ERR_get_error();
            message = text(L"HTTPS 安全连接失败");
            return false;
        }
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        if (SSL_set1_host(tls.ssl, host.c_str()) != 1)
        {
            native_error = ERR_get_error();
            message = text(L"HTTPS 安全连接失败");
            return false;
        }
#else
        X509_VERIFY_PARAM* param = SSL_get0_param(tls.ssl);
        if (!param || X509_VERIFY_PARAM_set1_host(param, host.c_str(), 0) != 1)
        {
            native_error = ERR_get_error();
            message = text(L"HTTPS 安全连接失败");
            return false;
        }
#endif
    }
    int rc = SSL_connect(tls.ssl);
    if (rc != 1)
    {
        int ssl_error = SSL_get_error(tls.ssl, rc);
        unsigned long openssl_error = ERR_get_error();
        native_error = openssl_error ? openssl_error : static_cast<DWORD>(ssl_error);
        message = text(L"HTTPS 安全连接失败");
        return false;
    }
    long verify = SSL_get_verify_result(tls.ssl);
    if (verify != X509_V_OK)
    {
        native_error = static_cast<DWORD>(verify);
        message = text(L"HTTPS 安全连接失败");
        return false;
    }
    return true;
}

inline bool tls_send_all(SSL* ssl, const char* data, size_t size, DWORD& native_error)
{
    size_t offset = 0;
    while (offset < size)
    {
        int chunk = static_cast<int>((std::min)(size - offset, static_cast<size_t>((std::numeric_limits<int>::max)())));
        int sent = SSL_write(ssl, data + offset, chunk);
        if (sent > 0)
        {
            offset += static_cast<size_t>(sent);
            continue;
        }
        int ssl_error = SSL_get_error(ssl, sent);
        if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
        {
            continue;
        }
        unsigned long openssl_error = ERR_get_error();
        native_error = openssl_error ? openssl_error : static_cast<DWORD>(ssl_error);
        return false;
    }
    return true;
}

inline ssize_t tls_receive(SSL* ssl, char* data, size_t size, DWORD& native_error)
{
    for (;;)
    {
        int chunk = static_cast<int>((std::min)(size, static_cast<size_t>((std::numeric_limits<int>::max)())));
        int received = SSL_read(ssl, data, chunk);
        if (received > 0)
        {
            return received;
        }
        int ssl_error = SSL_get_error(ssl, received);
        if (ssl_error == SSL_ERROR_ZERO_RETURN)
        {
            return 0;
        }
        if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
        {
            continue;
        }
        unsigned long openssl_error = ERR_get_error();
        native_error = openssl_error ? openssl_error : static_cast<DWORD>(ssl_error);
        return -1;
    }
}

class Connection
{
    SocketHandle socket_;
    std::unique_ptr<TlsState> tls_;

public:
    bool connect_to(const std::string& host, uint16_t port, int timeout_ms, bool secure, const std::string& tls_host, ErrorCode& error_code, DWORD& native_error, std::string& message)
    {
        if (!connect_socket(host, port, timeout_ms, socket_, error_code, native_error, message))
        {
            return false;
        }
        if (secure)
        {
            tls_.reset(new TlsState());
            if (!start_tls(socket_.get(), tls_host, *tls_, native_error, message))
            {
                error_code = ErrorCode::TlsFailure;
                return false;
            }
        }
        return true;
    }

    int fd() const
    {
        return socket_.get();
    }

    bool start_tls_layer(const std::string& tls_host, ErrorCode& error_code, DWORD& native_error, std::string& message)
    {
        tls_.reset(new TlsState());
        if (!start_tls(socket_.get(), tls_host, *tls_, native_error, message))
        {
            error_code = ErrorCode::TlsFailure;
            return false;
        }
        return true;
    }

    bool send_all(const std::string& data, DWORD& native_error)
    {
        if (tls_ && tls_->ssl)
        {
            return tls_send_all(tls_->ssl, data.data(), data.size(), native_error);
        }
        return socket_send_all(socket_.get(), data.data(), data.size(), native_error);
    }

    ssize_t receive(char* data, size_t size, DWORD& native_error)
    {
        if (tls_ && tls_->ssl)
        {
            return tls_receive(tls_->ssl, data, size, native_error);
        }
        return socket_receive(socket_.get(), data, size, native_error);
    }
};

inline bool send_connect_tunnel(Connection& connection, const std::string& host, uint16_t port, const Headers& headers, int timeout_ms, ErrorCode& error_code, DWORD& native_error, std::string& message)
{
    (void)timeout_ms;
    std::ostringstream request;
    request << "CONNECT ";
    if (host.find(':') != std::string::npos)
    {
        request << "[" << host << "]";
    }
    else
    {
        request << host;
    }
    request << ":" << port << " HTTP/1.1\r\nHost: ";
    if (host.find(':') != std::string::npos)
    {
        request << "[" << host << "]";
    }
    else
    {
        request << host;
    }
    request << ":" << port << "\r\nProxy-Connection: keep-alive\r\n";
    std::string proxy_authorization = headers.get("Proxy-Authorization");
    if (!proxy_authorization.empty())
    {
        request << "Proxy-Authorization: " << proxy_authorization << "\r\n";
    }
    request << "\r\n";
    if (!connection.send_all(request.str(), native_error))
    {
        error_code = native_error == ETIMEDOUT || native_error == EAGAIN || native_error == EWOULDBLOCK ? ErrorCode::Timeout : ErrorCode::SendFailed;
        message = text_pair("Unable to send proxy CONNECT request", L"无法发送代理 CONNECT 请求");
        return false;
    }
    std::string response;
    char buffer[4096];
    while (response.find("\r\n\r\n") == std::string::npos)
    {
        ssize_t received = connection.receive(buffer, sizeof(buffer), native_error);
        if (received <= 0)
        {
            error_code = received < 0 && (native_error == ETIMEDOUT || native_error == EAGAIN || native_error == EWOULDBLOCK) ? ErrorCode::Timeout : ErrorCode::ReceiveFailed;
            message = text_pair("Unable to receive proxy CONNECT response", L"无法接收代理 CONNECT 响应");
            return false;
        }
        response.append(buffer, static_cast<size_t>(received));
        if (response.size() > 65536)
        {
            error_code = ErrorCode::ReceiveFailed;
            message = text_pair("Proxy CONNECT response headers are too large", L"代理 CONNECT 响应头过大");
            return false;
        }
    }
    size_t line_end = response.find("\r\n");
    if (line_end == std::string::npos)
    {
        error_code = ErrorCode::ReceiveFailed;
        message = text_pair("Invalid proxy CONNECT response", L"代理 CONNECT 响应无效");
        return false;
    }
    std::istringstream status(response.substr(0, line_end));
    std::string version;
    int code = 0;
    status >> version >> code;
    if (code < 200 || code >= 300)
    {
        error_code = ErrorCode::ConnectionFailed;
        std::ostringstream error_stream;
        if (current_language() == "zh_CN")
        {
            error_stream << "代理 CONNECT 失败，状态码 " << code;
        }
        else
        {
            error_stream << "Proxy CONNECT failed: status code " << code;
        }
        message = error_stream.str();
        return false;
    }
    return true;
}

}

namespace detail
{

class DownloadProgressDisplay
{
    bool enabled_;
    size_t bar_width_;
    unsigned int refresh_ms_;
    bool active_;
    bool total_known_;
    uint64_t total_;
    bool terminal_;
    bool rendered_;
    std::chrono::steady_clock::time_point started_;
    std::chrono::steady_clock::time_point last_render_;

    std::vector<std::wstring> make_lines(uint64_t bytes) const
    {
        bool zh = current_language() == "zh_CN";
        double fraction = 0.0;
        if (total_known_ && total_ > 0)
        {
            fraction = static_cast<double>((std::min)(bytes, total_)) / static_cast<double>(total_);
        }
        else if (total_known_ && total_ == 0)
        {
            fraction = 1.0;
        }
        if (fraction < 0.0)
        {
            fraction = 0.0;
        }
        if (fraction > 1.0)
        {
            fraction = 1.0;
        }
        size_t filled = total_known_ ? static_cast<size_t>(fraction * static_cast<double>(bar_width_) + 0.5) : 0;
        if (filled > bar_width_)
        {
            filled = bar_width_;
        }
        std::wstring bar = L"[";
        bar.append(filled, L'#');
        bar.append(bar_width_ - filled, L'-');
        bar += L"] ";
        if (total_known_)
        {
            int percent = static_cast<int>(fraction * 100.0 + 0.5);
            if (percent > 100)
            {
                percent = 100;
            }
            std::wostringstream percent_stream;
            percent_stream << percent << L"%";
            bar += percent_stream.str();
        }
        else
        {
            bar += L"--%";
        }
        std::wstring size_line = format_download_bytes(bytes) + L" / " + (total_known_ ? format_download_bytes(total_) : (zh ? L"未知" : L"Unknown"));
        std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started_;
        double seconds = elapsed.count();
        double speed = seconds > 0.001 ? static_cast<double>(bytes) / seconds : 0.0;
        uint64_t speed_bytes = speed > 0.0 ? static_cast<uint64_t>(speed + 0.5) : 0;
        std::wstring speed_line = (zh ? L"下载速度 " : L"Download speed ") + format_download_bytes(speed_bytes) + L"/s";
        std::wstring eta_line = zh ? L"剩余时间 " : L"ETA ";
        if (total_known_ && bytes >= total_)
        {
            eta_line += L"00:00";
        }
        else if (total_known_ && speed > 0.001 && total_ > bytes)
        {
            eta_line += format_download_seconds(static_cast<double>(total_ - bytes) / speed);
        }
        else
        {
            eta_line += L"--:--";
        }
        std::vector<std::wstring> lines;
        lines.push_back(bar);
        lines.push_back(size_line);
        lines.push_back(speed_line);
        lines.push_back(eta_line);
        return lines;
    }

    size_t terminal_width() const
    {
        struct winsize size = {};
        if (terminal_ && ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
        {
            return static_cast<size_t>(size.ws_col);
        }
        return 80;
    }

    std::string clip_line(const std::wstring& line) const
    {
        std::string utf8 = wide_to_utf8(line);
        size_t width = terminal_width();
        if (utf8.size() < width)
        {
            return utf8;
        }
        if (width <= 1)
        {
            return std::string();
        }
        return utf8.substr(0, width - 1);
    }

    void render(uint64_t bytes, bool force, bool final)
    {
        if (!enabled_ || !active_)
        {
            return;
        }
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (!force && last_render_ != std::chrono::steady_clock::time_point())
        {
            std::chrono::duration<double, std::milli> elapsed = now - last_render_;
            if (elapsed.count() < refresh_ms_)
            {
                return;
            }
        }
        last_render_ = now;
        if (!terminal_ && !final)
        {
            return;
        }
        std::vector<std::wstring> lines = make_lines(bytes);
        if (terminal_ && rendered_)
        {
            std::cout << "\033[4A";
        }
        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (terminal_)
            {
                std::cout << "\r\033[2K";
            }
            std::cout << clip_line(lines[i]) << '\n';
        }
        std::cout.flush();
        rendered_ = true;
    }

public:
    DownloadProgressDisplay(bool enabled = false, size_t bar_width = 20, unsigned int refresh_ms = 100)
        : enabled_(enabled), bar_width_(bar_width ? bar_width : 20), refresh_ms_(refresh_ms ? refresh_ms : 100), active_(false), total_known_(false), total_(0), terminal_(isatty(STDOUT_FILENO) != 0), rendered_(false)
    {
    }

    void reset()
    {
        active_ = false;
        total_known_ = false;
        total_ = 0;
        rendered_ = false;
        started_ = std::chrono::steady_clock::now();
        last_render_ = std::chrono::steady_clock::time_point();
    }

    void begin(int status_code, bool total_known, uint64_t total)
    {
        if (!enabled_ || status_code < 200 || status_code >= 300)
        {
            return;
        }
        active_ = true;
        total_known_ = total_known;
        total_ = total;
        started_ = std::chrono::steady_clock::now();
        last_render_ = std::chrono::steady_clock::time_point();
        rendered_ = false;
        render(0, true, false);
    }

    void update(uint64_t bytes)
    {
        render(bytes, false, false);
    }

    void end(uint64_t bytes)
    {
        render(bytes, true, true);
        active_ = false;
    }
};

}

inline void set_default_language(const std::string& lang)
{
    detail::set_default_language(lang);
}

inline void set_language(const std::string& lang)
{
    detail::set_default_language(lang);
}

class MemorySink
{
    std::string data_;
    size_t limit_;
    bool too_large_;

public:
    explicit MemorySink(size_t limit) : limit_(limit), too_large_(false)
    {
    }

    bool reset()
    {
        data_.clear();
        too_large_ = false;
        return true;
    }

    bool reserve(uint64_t size)
    {
        if (limit_ > 0 && size > static_cast<uint64_t>(limit_))
        {
            too_large_ = true;
            return false;
        }
        if (size <= static_cast<uint64_t>((std::numeric_limits<size_t>::max)()))
        {
            data_.reserve(static_cast<size_t>(size));
        }
        return true;
    }

    bool write(const char* data, size_t size)
    {
        if (limit_ > 0 && (data_.size() > limit_ || size > limit_ - data_.size()))
        {
            too_large_ = true;
            return false;
        }
        data_.append(data, size);
        return true;
    }

    void begin_response(int, bool, uint64_t)
    {
    }

    void end_response()
    {
    }

    bool too_large() const
    {
        return too_large_;
    }

    bool file_sink() const
    {
        return false;
    }

    uint64_t size() const
    {
        return static_cast<uint64_t>(data_.size());
    }

    const std::string& data() const
    {
        return data_;
    }
};

class FileSink
{
    std::wstring destination_;
    std::string destination_utf8_;
    std::string temp_utf8_;
    uint64_t limit_;
    bool overwrite_;
    bool too_large_;
    uint64_t written_;
    int fd_;
    bool committed_;
    detail::DownloadProgressDisplay progress_;

    static bool exists_utf8(const std::string& path)
    {
        struct stat info;
        return stat(path.c_str(), &info) == 0;
    }

    void close_file()
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
            fd_ = -1;
        }
    }

public:
    FileSink(const std::wstring& destination, uint64_t limit, bool overwrite, bool show_progress, size_t progress_width, unsigned int progress_refresh_ms)
        : destination_(destination), destination_utf8_(detail::wide_to_utf8(destination)), limit_(limit), overwrite_(overwrite), too_large_(false), written_(0), fd_(-1), committed_(false), progress_(show_progress, progress_width, progress_refresh_ms)
    {
    }

    ~FileSink()
    {
        close_file();
        if (!committed_ && !temp_utf8_.empty())
        {
            unlink(temp_utf8_.c_str());
        }
    }

    bool destination_exists() const
    {
        return !destination_utf8_.empty() && exists_utf8(destination_utf8_);
    }

    bool reset()
    {
        close_file();
        if (!temp_utf8_.empty())
        {
            unlink(temp_utf8_.c_str());
        }
        temp_utf8_.clear();
        too_large_ = false;
        written_ = 0;
        committed_ = false;
        progress_.reset();
        if (destination_utf8_.empty())
        {
            return false;
        }
        static std::atomic<unsigned long> sequence(0);
        for (int attempt = 0; attempt < 64; ++attempt)
        {
            std::ostringstream name;
            name << destination_utf8_ << ".feathercrawl." << static_cast<unsigned long>(getpid()) << "." << static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count()) << "." << ++sequence << ".part";
            temp_utf8_ = name.str();
            fd_ = open(temp_utf8_.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
            if (fd_ >= 0)
            {
                return true;
            }
            if (errno != EEXIST)
            {
                temp_utf8_.clear();
                return false;
            }
        }
        return false;
    }

    bool reserve(uint64_t size)
    {
        if (limit_ > 0 && size > limit_)
        {
            too_large_ = true;
            return false;
        }
        return true;
    }

    bool write(const char* data, size_t size)
    {
        if (fd_ < 0)
        {
            return false;
        }
        if (limit_ > 0 && (written_ > limit_ || static_cast<uint64_t>(size) > limit_ - written_))
        {
            too_large_ = true;
            return false;
        }
        size_t offset = 0;
        while (offset < size)
        {
            ssize_t count = ::write(fd_, data + offset, size - offset);
            if (count > 0)
            {
                offset += static_cast<size_t>(count);
                written_ += static_cast<uint64_t>(count);
                continue;
            }
            if (count < 0 && errno == EINTR)
            {
                continue;
            }
            return false;
        }
        progress_.update(written_);
        return true;
    }

    void begin_response(int status_code, bool total_known, uint64_t total)
    {
        progress_.begin(status_code, total_known, total);
    }

    void end_response()
    {
        progress_.end(written_);
    }

    bool too_large() const
    {
        return too_large_;
    }

    bool file_sink() const
    {
        return true;
    }

    uint64_t size() const
    {
        return written_;
    }

    bool commit()
    {
        if (fd_ < 0)
        {
            return false;
        }
        if (fsync(fd_) != 0)
        {
            return false;
        }
        close_file();
        if (overwrite_)
        {
            if (rename(temp_utf8_.c_str(), destination_utf8_.c_str()) != 0)
            {
                return false;
            }
        }
        else
        {
            if (link(temp_utf8_.c_str(), destination_utf8_.c_str()) != 0)
            {
                return false;
            }
            if (unlink(temp_utf8_.c_str()) != 0)
            {
                unlink(destination_utf8_.c_str());
                return false;
            }
        }
        committed_ = true;
        return true;
    }
};

inline std::wstring suggested_download_filename(const std::wstring& url)
{
    detail::ParsedUrl parsed;
    std::string error;
    std::wstring name;
    if (detail::parse_url(url, parsed, error))
    {
        size_t slash = parsed.path_only.find_last_of(L'/');
        name = slash == std::wstring::npos ? parsed.path_only : parsed.path_only.substr(slash + 1);
    }
    if (name.empty())
    {
        name = L"download.bin";
    }
    const wchar_t* invalid = L"/\\\0";
    for (size_t i = 0; i < name.size(); ++i)
    {
        if (name[i] < 32 || std::wcschr(invalid, name[i]) != NULL)
        {
            name[i] = L'_';
        }
    }
    if (name == L"." || name == L"..")
    {
        name = L"download.bin";
    }
    return name;
}

namespace detail
{

class BufferedReader
{
    Connection& connection_;
    std::string buffer_;
    DWORD& native_error_;

public:
    BufferedReader(Connection& connection, DWORD& native_error) : connection_(connection), native_error_(native_error)
    {
    }

    bool read_more()
    {
        char data[32768];
        ssize_t received = connection_.receive(data, sizeof(data), native_error_);
        if (received <= 0)
        {
            return false;
        }
        buffer_.append(data, static_cast<size_t>(received));
        return true;
    }

    bool read_until(const std::string& marker, size_t max_size, std::string& output)
    {
        for (;;)
        {
            size_t position = buffer_.find(marker);
            if (position != std::string::npos)
            {
                size_t end = position + marker.size();
                output.assign(buffer_.data(), end);
                buffer_.erase(0, end);
                return true;
            }
            if (buffer_.size() >= max_size)
            {
                return false;
            }
            if (!read_more())
            {
                return false;
            }
        }
    }

    bool read_line(std::string& line, size_t max_size = 65536)
    {
        std::string raw;
        if (!read_until("\r\n", max_size, raw))
        {
            return false;
        }
        line.assign(raw.data(), raw.size() - 2);
        return true;
    }

    template <class Sink>
    bool consume_exact(uint64_t count, Sink& sink)
    {
        while (count > 0)
        {
            if (buffer_.empty() && !read_more())
            {
                return false;
            }
            size_t chunk = static_cast<size_t>((std::min)(count, static_cast<uint64_t>(buffer_.size())));
            if (!sink.write(buffer_.data(), chunk))
            {
                return false;
            }
            buffer_.erase(0, chunk);
            count -= static_cast<uint64_t>(chunk);
        }
        return true;
    }

    bool consume_exact_string(uint64_t count, std::string& output)
    {
        output.clear();
        if (count > static_cast<uint64_t>((std::numeric_limits<size_t>::max)()))
        {
            return false;
        }
        output.reserve(static_cast<size_t>(count));
        while (count > 0)
        {
            if (buffer_.empty() && !read_more())
            {
                return false;
            }
            size_t chunk = static_cast<size_t>((std::min)(count, static_cast<uint64_t>(buffer_.size())));
            output.append(buffer_.data(), chunk);
            buffer_.erase(0, chunk);
            count -= static_cast<uint64_t>(chunk);
        }
        return true;
    }

    template <class Sink>
    bool consume_until_eof(Sink& sink)
    {
        if (!buffer_.empty())
        {
            if (!sink.write(buffer_.data(), buffer_.size()))
            {
                return false;
            }
            buffer_.clear();
        }
        char data[32768];
        for (;;)
        {
            ssize_t received = connection_.receive(data, sizeof(data), native_error_);
            if (received == 0)
            {
                return true;
            }
            if (received < 0)
            {
                return false;
            }
            if (!sink.write(data, static_cast<size_t>(received)))
            {
                return false;
            }
        }
    }
};

inline bool parse_status_and_headers(const std::string& block, int& status, ResponseHeaders& headers)
{
    headers.clear();
    size_t first_end = block.find("\r\n");
    if (first_end == std::string::npos)
    {
        return false;
    }
    std::string status_line = block.substr(0, first_end);
    std::istringstream status_stream(status_line);
    std::string version;
    status = 0;
    status_stream >> version >> status;
    if (version.size() < 7 || version.compare(0, 5, "HTTP/") != 0 || status < 100 || status > 999)
    {
        return false;
    }
    size_t start = first_end + 2;
    while (start < block.size())
    {
        size_t end = block.find("\r\n", start);
        if (end == std::string::npos || end == start)
        {
            break;
        }
        std::string line = block.substr(start, end - start);
        if (!line.empty() && (line[0] == ' ' || line[0] == '\t'))
        {
            return false;
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos || colon == 0)
        {
            return false;
        }
        std::string name = line.substr(0, colon);
        std::string value = trim_ascii(line.substr(colon + 1));
        if (!valid_header_name(name) || !valid_header_value(value))
        {
            return false;
        }
        headers.add(name, value);
        start = end + 2;
    }
    return true;
}

inline bool parse_content_length_headers(const ResponseHeaders& headers, bool& known, uint64_t& length)
{
    known = false;
    length = 0;
    std::vector<std::string> values = headers.get_all("Content-Length");
    bool have = false;
    uint64_t expected = 0;
    for (size_t i = 0; i < values.size(); ++i)
    {
        size_t start = 0;
        while (start <= values[i].size())
        {
            size_t comma = values[i].find(',', start);
            std::string part = trim_ascii(values[i].substr(start, comma == std::string::npos ? std::string::npos : comma - start));
            uint64_t value = 0;
            if (part.empty() || !parse_uint64(part, value))
            {
                return false;
            }
            if (!have)
            {
                expected = value;
                have = true;
            }
            else if (expected != value)
            {
                return false;
            }
            if (comma == std::string::npos)
            {
                break;
            }
            start = comma + 1;
        }
    }
    known = have;
    length = expected;
    return true;
}

inline bool transfer_is_chunked(const ResponseHeaders& headers, bool& present, bool& supported)
{
    present = false;
    supported = true;
    std::vector<std::string> values = headers.get_all("Transfer-Encoding");
    if (values.empty())
    {
        return false;
    }
    present = true;
    std::vector<std::string> codings;
    for (size_t i = 0; i < values.size(); ++i)
    {
        size_t start = 0;
        while (start <= values[i].size())
        {
            size_t comma = values[i].find(',', start);
            std::string coding = ascii_lower_copy(trim_ascii(values[i].substr(start, comma == std::string::npos ? std::string::npos : comma - start)));
            if (coding.empty())
            {
                supported = false;
                return false;
            }
            codings.push_back(coding);
            if (comma == std::string::npos)
            {
                break;
            }
            start = comma + 1;
        }
    }
    if (codings.empty())
    {
        supported = false;
        return false;
    }
    bool chunked = codings.back() == "chunked";
    for (size_t i = 0; i + 1 < codings.size(); ++i)
    {
        if (codings[i] != "identity")
        {
            supported = false;
        }
    }
    if (!chunked)
    {
        supported = false;
    }
    return chunked;
}

inline bool parse_hex_size(const std::string& line, uint64_t& size)
{
    std::string value = line.substr(0, line.find(';'));
    value = trim_ascii(value);
    if (value.empty())
    {
        return false;
    }
    uint64_t result = 0;
    for (size_t i = 0; i < value.size(); ++i)
    {
        int digit = -1;
        if (value[i] >= '0' && value[i] <= '9')
        {
            digit = value[i] - '0';
        }
        else if (value[i] >= 'a' && value[i] <= 'f')
        {
            digit = value[i] - 'a' + 10;
        }
        else if (value[i] >= 'A' && value[i] <= 'F')
        {
            digit = value[i] - 'A' + 10;
        }
        if (digit < 0 || result > ((std::numeric_limits<uint64_t>::max)() - static_cast<uint64_t>(digit)) / 16ULL)
        {
            return false;
        }
        result = result * 16ULL + static_cast<uint64_t>(digit);
    }
    size = result;
    return true;
}

inline bool no_response_body(const std::wstring& method, int status)
{
    return ascii_iequals(method, L"HEAD") || (status >= 100 && status < 200) || status == 204 || status == 304;
}

template <class Sink>
inline bool consume_chunked_body(BufferedReader& reader, Sink& sink, ResponseHeaders& headers, ErrorCode& error_code, std::string& message)
{
    for (;;)
    {
        std::string line;
        if (!reader.read_line(line))
        {
            error_code = ErrorCode::ReadFailed;
            message = text_pair("Invalid chunked response", L"分块响应无效");
            return false;
        }
        uint64_t chunk_size = 0;
        if (!parse_hex_size(line, chunk_size))
        {
            error_code = ErrorCode::ReadFailed;
            message = text_pair("Invalid chunk size", L"分块大小无效");
            return false;
        }
        if (chunk_size == 0)
        {
            for (;;)
            {
                std::string trailer;
                if (!reader.read_line(trailer))
                {
                    error_code = ErrorCode::ReadFailed;
                    message = text_pair("Invalid chunk trailer", L"分块尾部无效");
                    return false;
                }
                if (trailer.empty())
                {
                    return true;
                }
                size_t colon = trailer.find(':');
                if (colon == std::string::npos || colon == 0)
                {
                    error_code = ErrorCode::ReadFailed;
                    message = text_pair("Invalid chunk trailer header", L"分块尾部请求头无效");
                    return false;
                }
                std::string name = trailer.substr(0, colon);
                std::string value = trim_ascii(trailer.substr(colon + 1));
                if (!valid_header_name(name) || !valid_header_value(value))
                {
                    error_code = ErrorCode::ReadFailed;
                    message = text_pair("Invalid chunk trailer header", L"分块尾部请求头无效");
                    return false;
                }
                std::string lower = ascii_lower_copy(name);
                if (lower != "content-length" && lower != "transfer-encoding" && lower != "host")
                {
                    headers.add(name, value);
                }
            }
        }
        if (!reader.consume_exact(chunk_size, sink))
        {
            error_code = sink.too_large() ? ErrorCode::ResponseTooLarge : ErrorCode::ReadFailed;
            message = sink.too_large() ? text(L"服务器响应超过允许的最大大小") : text_pair("Unable to read chunked response body", L"无法读取分块响应体");
            return false;
        }
        std::string crlf;
        if (!reader.consume_exact_string(2, crlf) || crlf != "\r\n")
        {
            error_code = ErrorCode::ReadFailed;
            message = text_pair("Invalid chunk terminator", L"分块结束符无效");
            return false;
        }
    }
}

inline void sleep_retry(const RetryPolicy& policy, int retry_index, const ResponseHeaders* headers)
{
    double delay = static_cast<double>(policy.initial_delay_ms) * std::pow(policy.multiplier > 0.0 ? policy.multiplier : 1.0, static_cast<double>(retry_index));
    if (delay > policy.max_delay_ms)
    {
        delay = policy.max_delay_ms;
    }
    if (policy.respect_retry_after && headers)
    {
        std::string retry_after = headers->get("Retry-After");
        uint64_t seconds = 0;
        if (!retry_after.empty() && parse_uint64(retry_after, seconds))
        {
            double requested = static_cast<double>(seconds) * 1000.0;
            if (requested > delay)
            {
                delay = requested;
            }
        }
        else if (!retry_after.empty())
        {
            time_t date = 0;
            if (parse_http_time(retry_after, date))
            {
                time_t now = std::time(NULL);
                if (date > now)
                {
                    double requested = static_cast<double>(date - now) * 1000.0;
                    if (requested > delay)
                    {
                        delay = requested;
                    }
                }
            }
        }
    }
    if (policy.max_delay_ms >= 0 && delay > policy.max_delay_ms)
    {
        delay = policy.max_delay_ms;
    }
    if (delay < 0.0)
    {
        delay = 0.0;
    }
    if (policy.jitter && delay > 1.0)
    {
        double fraction = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
        delay *= 0.75 + fraction * 0.5;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(delay)));
}

inline bool retryable_native_error(ErrorCode code, DWORD native_error)
{
    if (code == ErrorCode::Timeout || code == ErrorCode::ConnectionFailed || code == ErrorCode::NameResolutionFailed)
    {
        return true;
    }
    return native_error == ECONNRESET || native_error == ECONNREFUSED || native_error == EPIPE || native_error == ETIMEDOUT || native_error == EHOSTUNREACH || native_error == ENETUNREACH;
}

}

class Session
{
    SessionOptions options_;
    CookieJar cookies_;
    mutable std::mutex option_mutex_;
    bool cookies_enabled_;
    std::string language_;

    bool proxy_bypassed(const detail::ParsedUrl& url) const
    {
        std::wstring bypass;
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            bypass = options_.proxy_bypass;
        }
        if (bypass.empty())
        {
            return false;
        }
        std::wstring host = detail::ascii_lower_copy(url.host);
        size_t start = 0;
        while (start <= bypass.size())
        {
            size_t separator = bypass.find_first_of(L";,", start);
            std::wstring token = detail::ascii_lower_copy(detail::trim_ascii(bypass.substr(start, separator == std::wstring::npos ? std::wstring::npos : separator - start)));
            if (!token.empty())
            {
                if (token == L"<local>" && host.find(L'.') == std::wstring::npos)
                {
                    return true;
                }
                if (token[0] == L'*')
                {
                    token.erase(token.begin());
                }
                if (!token.empty() && token[0] == L'.')
                {
                    if (host.size() >= token.size() && host.compare(host.size() - token.size(), token.size(), token) == 0)
                    {
                        return true;
                    }
                }
                else if (detail::ascii_iequals(host, token))
                {
                    return true;
                }
            }
            if (separator == std::wstring::npos)
            {
                break;
            }
            start = separator + 1;
        }
        return false;
    }

    template <class Sink>
    Response send_once(const std::wstring& method, const detail::ParsedUrl& url, const std::string& body, Headers headers, const RequestOptions& options, Sink& sink)
    {
        Response response;
        response.final_url = url.absolute;
        std::string header_error;
        if (!detail::validate_headers(headers, header_error))
        {
            response.error_code = ErrorCode::InvalidHeader;
            response.error_message = header_error;
            return response;
        }

        std::string host = detail::wide_to_utf8(url.host);
        if (host.empty())
        {
            response.error_code = ErrorCode::EncodingError;
            response.error_message = detail::text_pair("URL host cannot be represented as UTF-8", L"URL 主机无法转换为 UTF-8");
            return response;
        }

        std::wstring proxy_value;
        std::wstring user_agent_value;
        bool use_cookies = false;
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            proxy_value = options_.proxy;
            user_agent_value = options_.user_agent;
            use_cookies = cookies_enabled_;
        }
        bool use_proxy = !proxy_value.empty() && !proxy_bypassed(url);
        std::string proxy_host;
        uint16_t proxy_port = 0;
        if (use_proxy && !detail::parse_proxy(proxy_value, proxy_host, proxy_port))
        {
            response.error_code = ErrorCode::InvalidArgument;
            response.error_message = detail::text_pair("Invalid HTTP proxy address", L"HTTP 代理地址无效");
            return response;
        }

        if (!headers.contains("User-Agent"))
        {
            std::string user_agent = detail::wide_to_utf8(user_agent_value);
            if (!user_agent.empty())
            {
                headers.set("User-Agent", user_agent);
            }
        }
        if (!headers.contains("Accept"))
        {
            headers.set("Accept", "*/*");
        }
        if (!headers.contains("Connection"))
        {
            headers.set("Connection", "close");
        }
        if (use_cookies && !headers.contains("Cookie"))
        {
            std::string cookie = cookies_.header_for(url);
            if (!cookie.empty())
            {
                headers.set("Cookie", cookie);
            }
        }
        headers.erase("Transfer-Encoding");
        if (!body.empty() || headers.contains("Content-Length"))
        {
            std::ostringstream length;
            length << body.size();
            headers.set("Content-Length", length.str());
        }
        else
        {
            headers.erase("Content-Length");
        }

        std::ostringstream host_header;
        if (host.find(':') != std::string::npos)
        {
            host_header << "[" << host << "]";
        }
        else
        {
            host_header << host;
        }
        bool default_port = (url.secure && url.port == 443) || (!url.secure && url.port == 80);
        if (!default_port)
        {
            host_header << ":" << url.port;
        }
        headers.set("Host", host_header.str());

        detail::Connection connection;
        ErrorCode connection_error = ErrorCode::None;
        DWORD native_error = 0;
        std::string connection_message;
        if (use_proxy)
        {
            if (!connection.connect_to(proxy_host, proxy_port, options.timeout_ms, false, std::string(), connection_error, native_error, connection_message))
            {
                response.error_code = connection_error;
                response.native_error_code = native_error;
                response.error_message = connection_message;
                return response;
            }
            if (url.secure)
            {
                if (!detail::send_connect_tunnel(connection, host, url.port, headers, options.timeout_ms, connection_error, native_error, connection_message))
                {
                    response.error_code = connection_error;
                    response.native_error_code = native_error;
                    response.error_message = connection_message;
                    return response;
                }
                headers.erase("Proxy-Authorization");
                headers.erase("Proxy-Connection");
                if (!connection.start_tls_layer(host, connection_error, native_error, connection_message))
                {
                    response.error_code = connection_error;
                    response.native_error_code = native_error;
                    response.error_message = connection_message;
                    return response;
                }
            }
        }
        else if (!connection.connect_to(host, url.port, options.timeout_ms, url.secure, host, connection_error, native_error, connection_message))
        {
            response.error_code = connection_error;
            response.native_error_code = native_error;
            response.error_message = connection_message;
            return response;
        }

        std::string method_utf8 = detail::wide_to_utf8(method);
        std::string target;
        if (use_proxy && !url.secure)
        {
            target = detail::wide_to_utf8(url.absolute);
        }
        else
        {
            target = detail::wide_to_utf8(url.path_query);
        }
        if (method_utf8.empty() || target.empty())
        {
            response.error_code = ErrorCode::EncodingError;
            response.error_message = detail::text_pair("Unable to encode the HTTP request", L"无法编码 HTTP 请求");
            return response;
        }

        std::ostringstream request;
        request << method_utf8 << " " << target << " HTTP/1.1\r\n";
        for (std::unordered_map<std::string, std::string>::const_iterator it = headers.fields.begin(); it != headers.fields.end(); ++it)
        {
            request << it->first << ": " << it->second << "\r\n";
        }
        request << "\r\n";
        std::string wire = request.str();
        wire.append(body);
        if (!connection.send_all(wire, native_error))
        {
            response.native_error_code = native_error;
            if (native_error == ETIMEDOUT || native_error == EAGAIN || native_error == EWOULDBLOCK)
            {
                response.error_code = ErrorCode::Timeout;
                response.error_message = detail::text(L"请求超时");
            }
            else
            {
                response.error_code = ErrorCode::SendFailed;
                response.error_message = detail::text_pair("Unable to send HTTP request", L"无法发送 HTTP 请求");
            }
            return response;
        }

        detail::BufferedReader reader(connection, native_error);
        for (;;)
        {
            std::string header_block;
            if (!reader.read_until("\r\n\r\n", 1024ULL * 1024ULL, header_block))
            {
                response.native_error_code = native_error;
                if (native_error == ETIMEDOUT || native_error == EAGAIN || native_error == EWOULDBLOCK)
                {
                    response.error_code = ErrorCode::Timeout;
                    response.error_message = detail::text(L"请求超时");
                }
                else
                {
                    response.error_code = ErrorCode::ReceiveFailed;
                    response.error_message = detail::text_pair("Unable to read HTTP response headers", L"无法读取 HTTP 响应头");
                }
                return response;
            }
            int status = 0;
            ResponseHeaders parsed_headers;
            if (!detail::parse_status_and_headers(header_block, status, parsed_headers))
            {
                response.error_code = ErrorCode::ReceiveFailed;
                response.error_message = detail::text_pair("Invalid HTTP response headers", L"HTTP 响应头无效");
                return response;
            }
            if (status >= 100 && status < 200 && status != 101)
            {
                continue;
            }
            response.status_code = status;
            response.headers = parsed_headers;
            break;
        }

        if (use_cookies)
        {
            std::vector<std::string> set_cookies = response.headers.get_all("Set-Cookie");
            for (size_t i = 0; i < set_cookies.size(); ++i)
            {
                cookies_.store(url, set_cookies[i]);
            }
        }

        bool transfer_present = false;
        bool transfer_supported = true;
        bool chunked = detail::transfer_is_chunked(response.headers, transfer_present, transfer_supported);
        if (transfer_present && !transfer_supported)
        {
            response.error_code = ErrorCode::ReceiveFailed;
            response.error_message = detail::text_pair("Unsupported HTTP Transfer-Encoding", L"不支持该 HTTP Transfer-Encoding");
            return response;
        }
        bool length_known = false;
        uint64_t content_length = 0;
        if (!detail::parse_content_length_headers(response.headers, length_known, content_length))
        {
            response.error_code = ErrorCode::ReceiveFailed;
            response.error_message = detail::text_pair("Conflicting or invalid Content-Length headers", L"Content-Length 响应头冲突或无效");
            return response;
        }
        if (transfer_present)
        {
            length_known = false;
            content_length = 0;
        }
        if (length_known && !sink.reserve(content_length))
        {
            response.error_code = ErrorCode::ResponseTooLarge;
            response.error_message = detail::text(L"服务器响应超过允许的最大大小");
            return response;
        }
        sink.begin_response(response.status_code, length_known, content_length);

        if (!detail::no_response_body(method, response.status_code))
        {
            bool body_ok = true;
            ErrorCode body_error = ErrorCode::None;
            std::string body_message;
            if (chunked)
            {
                body_ok = detail::consume_chunked_body(reader, sink, response.headers, body_error, body_message);
            }
            else if (length_known)
            {
                body_ok = reader.consume_exact(content_length, sink);
                if (!body_ok)
                {
                    if (sink.too_large())
                    {
                        body_error = ErrorCode::ResponseTooLarge;
                        body_message = detail::text(L"服务器响应超过允许的最大大小");
                    }
                    else if (sink.file_sink())
                    {
                        body_error = ErrorCode::FileWriteFailed;
                        body_message = detail::text(L"写入下载文件失败");
                    }
                    else
                    {
                        body_error = ErrorCode::ReadFailed;
                        body_message = detail::text_pair("HTTP response ended before Content-Length bytes were received", L"HTTP 响应在接收完 Content-Length 指定字节前结束");
                    }
                }
            }
            else
            {
                body_ok = reader.consume_until_eof(sink);
                if (!body_ok)
                {
                    if (sink.too_large())
                    {
                        body_error = ErrorCode::ResponseTooLarge;
                        body_message = detail::text(L"服务器响应超过允许的最大大小");
                    }
                    else if (sink.file_sink())
                    {
                        body_error = ErrorCode::FileWriteFailed;
                        body_message = detail::text(L"写入下载文件失败");
                    }
                    else if (native_error == ETIMEDOUT || native_error == EAGAIN || native_error == EWOULDBLOCK)
                    {
                        body_error = ErrorCode::Timeout;
                        body_message = detail::text(L"请求超时");
                    }
                    else
                    {
                        body_error = ErrorCode::ReadFailed;
                        body_message = detail::text_pair("Unable to read HTTP response body", L"无法读取 HTTP 响应体");
                    }
                }
            }
            if (!body_ok)
            {
                response.error_code = body_error;
                response.native_error_code = native_error;
                response.error_message = body_message;
                return response;
            }
        }

        sink.end_response();
        response.received_bytes = static_cast<size_t>((std::min)(sink.size(), static_cast<uint64_t>((std::numeric_limits<size_t>::max)())));
        if (response.status_code >= 400)
        {
            response.error_code = ErrorCode::HttpError;
            response.error_message = detail::http_error_message(response.status_code);
        }
        else
        {
            response.error_code = ErrorCode::None;
            response.error_message = detail::text(L"无");
        }
        return response;
    }

    template <class Sink>
    Response send_with_retry(const std::wstring& method, const detail::ParsedUrl& url, const std::string& body, const Headers& headers, const RequestOptions& options, Sink& sink)
    {
        int retries = options.retry.retries;
        if (retries < 0)
        {
            retries = 0;
        }
        if (retries > 100)
        {
            retries = 100;
        }
        int maximum_attempts = retries + 1;
        bool method_retryable = detail::is_idempotent_method(method) || options.retry.retry_non_idempotent;
        for (int attempt = 0; attempt < maximum_attempts; ++attempt)
        {
            if (!sink.reset())
            {
                Response response;
                response.error_code = ErrorCode::FileOpenFailed;
                response.error_message = detail::text_pair("Unable to create the download temporary file", L"无法创建下载临时文件");
                response.attempts = attempt + 1;
                return response;
            }
            Response response = send_once(method, url, body, headers, options, sink);
            response.attempts = attempt + 1;
            bool retry_status = detail::is_retryable_http_status(response.status_code);
            bool retry_transport = detail::retryable_native_error(response.error_code, response.native_error_code);
            bool should_retry = method_retryable && attempt + 1 < maximum_attempts && (retry_status || retry_transport);
            if (!should_retry)
            {
                if (response.error_code != ErrorCode::None && response.error_code != ErrorCode::HttpError)
                {
                    response.error_message += detail::attempts_suffix(response.attempts);
                }
                return response;
            }
            detail::sleep_retry(options.retry, attempt, &response.headers);
        }
        Response response;
        response.error_code = ErrorCode::WinHttpError;
        response.error_message = detail::text_pair("Retry sequence ended unexpectedly", L"重试流程异常结束");
        return response;
    }

    template <class Sink>
    Response request_impl(const std::wstring& method, const std::wstring& input_url, const std::string& body, const Headers& headers, RequestOptions options, Sink& sink)
    {
        Response failure;
        if (method.empty())
        {
            failure.error_code = ErrorCode::InvalidArgument;
            failure.error_message = detail::text(L"HTTP 方法不能为空");
            return failure;
        }
        if (options.timeout_ms < 0)
        {
            failure.error_code = ErrorCode::InvalidArgument;
            failure.error_message = detail::text(L"请求超时时间不能小于 0");
            return failure;
        }
        if (options.max_redirects < 0)
        {
            options.max_redirects = options_.max_redirects;
        }
        if (options.max_redirects < 0 || options.max_redirects > 100)
        {
            failure.error_code = ErrorCode::InvalidArgument;
            failure.error_message = options.max_redirects < 0 ? detail::text(L"最大重定向次数不能小于 0") : detail::text(L"最大重定向次数不能大于 100");
            return failure;
        }
        if (options.max_response_size == 0)
        {
            options.max_response_size = options_.max_response_size;
        }
        if (options.retry.retries < 0 || options.retry.retries > 100)
        {
            failure.error_code = ErrorCode::InvalidArgument;
            failure.error_message = options.retry.retries < 0 ? detail::text(L"重试次数不能小于 0") : detail::text(L"重试次数不能大于 100");
            return failure;
        }

        std::wstring current_url = input_url;
        std::wstring current_method = method;
        std::string current_body = body;
        Headers current_headers = headers;
        int redirects = 0;
        for (;;)
        {
            detail::ParsedUrl parsed;
            std::string parse_error;
            bool unsupported_scheme = false;
            if (!detail::parse_url(current_url, parsed, parse_error, &unsupported_scheme))
            {
                failure.error_code = unsupported_scheme ? ErrorCode::UnsupportedScheme : ErrorCode::InvalidUrl;
                failure.error_message = parse_error;
                failure.redirect_count = redirects;
                return failure;
            }
            std::string prepared;
            detail::SecureStringGuard prepared_guard(prepared);
            std::string body_error;
            if (!detail::prepare_request_body(current_body, current_headers.get("Content-Type"), prepared, body_error))
            {
                failure.error_code = ErrorCode::EncodingError;
                failure.error_message = body_error;
                failure.redirect_count = redirects;
                return failure;
            }
            Response response = send_with_retry(current_method, parsed, prepared, current_headers, options, sink);
            response.redirect_count = redirects;
            response.final_url = parsed.absolute;
            if (!options.follow_redirect || !detail::is_redirect_status(response.status_code))
            {
                return response;
            }
            std::string location = response.headers.get("Location");
            if (location.empty())
            {
                return response;
            }
            if (redirects >= options.max_redirects)
            {
                response.error_code = ErrorCode::RedirectLimitExceeded;
                response.error_message = detail::text_pair("Redirect limit exceeded", L"重定向次数超过上限");
                return response;
            }
            std::wstring next_url;
            std::string redirect_error;
            if (!detail::resolve_redirect(parsed, location, next_url, redirect_error))
            {
                response.error_code = ErrorCode::RedirectError;
                response.error_message = redirect_error;
                return response;
            }
            detail::ParsedUrl next;
            if (!detail::parse_url(next_url, next, redirect_error))
            {
                response.error_code = ErrorCode::RedirectError;
                response.error_message = redirect_error;
                return response;
            }
            if (parsed.secure && !next.secure)
            {
                response.error_code = ErrorCode::RedirectError;
                response.error_message = detail::text_pair("Blocked HTTPS downgrade redirect to HTTP", L"已阻止 HTTPS 降级重定向到 HTTP");
                return response;
            }
            if (!detail::same_origin(parsed, next))
            {
                current_headers.erase("Authorization");
                current_headers.erase("Proxy-Authorization");
                current_headers.erase("Cookie");
                current_headers.erase("Host");
            }
            if ((response.status_code == 303 && !detail::ascii_iequals(current_method, L"HEAD")) || ((response.status_code == 301 || response.status_code == 302) && detail::ascii_iequals(current_method, L"POST")))
            {
                current_method = L"GET";
                detail::secure_clear(current_body);
                current_headers.erase("Content-Type");
                current_headers.erase("Content-Length");
            }
            current_url = next.absolute;
            ++redirects;
        }
    }

public:
    explicit Session(const SessionOptions& options = SessionOptions()) : options_(options), cookies_enabled_(options.enable_cookies), language_(detail::global_language())
    {
    }

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void set_language(const std::string& lang)
    {
        language_ = detail::normalize_language(lang);
    }

    std::string language() const
    {
        return language_;
    }

    void set_proxy(const std::string& proxy, const std::string& bypass = std::string())
    {
        std::wstring proxy_wide;
        std::wstring bypass_wide;
        if (detail::utf8_to_wide(proxy, proxy_wide))
        {
            options_.proxy = proxy_wide;
        }
        if (detail::utf8_to_wide(bypass, bypass_wide))
        {
            options_.proxy_bypass = bypass_wide;
        }
    }

    bool valid()
    {
        return true;
    }

    std::string error_message()
    {
        detail::ScopedLanguage guard(language_);
        return detail::text(L"无");
    }

    CookieJar& cookie_jar()
    {
        return cookies_;
    }

    void clear_cookies()
    {
        cookies_.clear();
    }

    bool delete_cookie(const std::string& name, const std::string& domain = std::string(), const std::string& path = std::string())
    {
        return cookies_.delete_cookie(name, domain, path);
    }

    size_t clear_cookies_for_domain(const std::string& domain)
    {
        return cookies_.clear_domain(domain);
    }

    size_t cookie_count()
    {
        return cookies_.size();
    }

    void set_cookies_enabled(bool enabled, bool clear_when_disabled = true)
    {
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            cookies_enabled_ = enabled;
        }
        if (!enabled && clear_when_disabled)
        {
            cookies_.clear();
        }
    }

    bool cookies_enabled() const
    {
        std::lock_guard<std::mutex> lock(option_mutex_);
        return cookies_enabled_;
    }

    size_t connection_count()
    {
        return 0;
    }

    Response request(const std::wstring& method, const std::wstring& url, const std::string& body = std::string(), const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        detail::ScopedLanguage guard(language_);
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            if (options.max_response_size == 0)
            {
                options.max_response_size = options_.max_response_size;
            }
            if (options.max_redirects < 0)
            {
                options.max_redirects = options_.max_redirects;
            }
        }
        MemorySink sink(options.max_response_size);
        Response response = request_impl(method, url, body, headers, options, sink);
        if (response.error_code != ErrorCode::ResponseTooLarge && response.error_code != ErrorCode::FileWriteFailed)
        {
            response.body = detail::normalize_response_body(sink.data(), response.headers.get("Content-Type"));
            response.received_bytes = static_cast<size_t>((std::min)(sink.size(), static_cast<uint64_t>((std::numeric_limits<size_t>::max)())));
        }
        return response;
    }

    Response request(const std::wstring& method, const std::string& url_utf8, const std::string& body = std::string(), const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        detail::ScopedLanguage guard(language_);
        std::wstring url;
        if (!detail::utf8_to_wide(url_utf8, url))
        {
            Response response;
            response.error_code = ErrorCode::EncodingError;
            response.error_message = detail::text(L"URL 不是有效的 UTF-8 编码");
            return response;
        }
        return request(method, url, body, headers, options);
    }

    Response get(const std::wstring& url, const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        return request(L"GET", url, std::string(), headers, options);
    }

    Response get(const std::string& url, const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        return request(L"GET", url, std::string(), headers, options);
    }

    Response post(const std::wstring& url, const std::string& body, const std::string& content_type = "application/x-www-form-urlencoded", const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        Headers updated = headers;
        if (!updated.contains("Content-Type"))
        {
            updated.set("Content-Type", content_type);
        }
        return request(L"POST", url, body, updated, options);
    }

    Response post(const std::string& url, const std::string& body, const std::string& content_type = "application/x-www-form-urlencoded", const Headers& headers = Headers(), RequestOptions options = RequestOptions())
    {
        Headers updated = headers;
        if (!updated.contains("Content-Type"))
        {
            updated.set("Content-Type", content_type);
        }
        return request(L"POST", url, body, updated, options);
    }

    Response post(const std::string& url, const std::string& body, const Headers& headers, RequestOptions options = RequestOptions())
    {
        return post(url, body, "application/x-www-form-urlencoded", headers, options);
    }

    Response post(const std::wstring& url, const std::string& body, const Headers& headers, RequestOptions options = RequestOptions())
    {
        return post(url, body, "application/x-www-form-urlencoded", headers, options);
    }

    DownloadResult download(const std::wstring& url, const std::wstring& destination, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        detail::ScopedLanguage guard(language_);
        DownloadResult result;
        if (destination.empty())
        {
            result.error_code = ErrorCode::InvalidArgument;
            result.error_message = detail::text(L"下载文件路径不能为空");
            return result;
        }
        result.destination = destination;
        FileSink sink(destination, options.max_file_size, options.overwrite, options.show_progress, options.progress_bar_width, options.progress_refresh_ms);
        if (!options.overwrite && sink.destination_exists())
        {
            result.error_code = ErrorCode::FileExists;
            result.error_message = detail::text(L"目标文件已存在，未允许覆盖");
            return result;
        }
        {
            std::lock_guard<std::mutex> lock(option_mutex_);
            if (options.request.max_redirects < 0)
            {
                options.request.max_redirects = options_.max_redirects;
            }
            if (options.request.max_response_size == 0)
            {
                options.request.max_response_size = options_.max_response_size;
            }
        }
        Response response = request_impl(L"GET", url, std::string(), headers, options.request, sink);
        result.status_code = response.status_code;
        result.headers = response.headers;
        result.error_code = response.error_code;
        result.native_error_code = response.native_error_code;
        result.error_message = response.error_message;
        result.bytes_written = sink.size();
        bool known = false;
        uint64_t reported = 0;
        if (detail::parse_content_length_headers(result.headers, known, reported) && known && !result.headers.contains("Transfer-Encoding"))
        {
            result.file_size = reported;
            result.file_size_known = true;
        }
        else
        {
            result.file_size = result.bytes_written;
            result.file_size_known = result.error_code == ErrorCode::None && result.status_code >= 200 && result.status_code < 300;
        }
        result.attempts = response.attempts;
        result.redirect_count = response.redirect_count;
        result.final_url = response.final_url;
        if (response.error_code == ErrorCode::None && response.status_code >= 200 && response.status_code < 300)
        {
            if (!sink.commit())
            {
                result.error_code = ErrorCode::FileCommitFailed;
                result.error_message = detail::text(L"下载完成，但将临时文件安全替换到目标路径时失败");
            }
        }
        return result;
    }

    DownloadResult download(const std::string& url_utf8, const std::string& destination_utf8, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        detail::ScopedLanguage guard(language_);
        std::wstring url;
        std::wstring destination;
        if (!detail::utf8_to_wide(url_utf8, url) || !detail::utf8_to_wide(destination_utf8, destination))
        {
            DownloadResult result;
            result.error_code = ErrorCode::EncodingError;
            result.error_message = detail::text(L"URL 或下载文件路径不是有效的 UTF-8 编码");
            return result;
        }
        return download(url, destination, headers, options);
    }

    DownloadResult download(const std::wstring& url, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        return download(url, suggested_download_filename(url), headers, options);
    }

    DownloadResult download(const std::string& url_utf8, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        detail::ScopedLanguage guard(language_);
        std::wstring url;
        if (!detail::utf8_to_wide(url_utf8, url))
        {
            DownloadResult result;
            result.error_code = ErrorCode::EncodingError;
            result.error_message = detail::text(L"URL 不是有效的 UTF-8 编码");
            return result;
        }
        return download(url, suggested_download_filename(url), headers, options);
    }
};

inline RequestOptions legacy_options(int timeout_ms, bool follow_redirect, int retries)
{
    RequestOptions options;
    options.timeout_ms = timeout_ms;
    options.follow_redirect = follow_redirect;
    options.retry.retries = retries;
    options.retry.retry_non_idempotent = retries > 0;
    return options;
}

inline Response send_request(const std::wstring& method, const std::wstring& url, const std::string& body = std::string(), const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    SessionOptions options;
    options.enable_cookies = false;
    Session session(options);
    return session.request(method, url, body, headers, legacy_options(timeout_ms, follow_redirect, retries));
}

inline Response get(const std::wstring& url, const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    return send_request(L"GET", url, std::string(), headers, timeout_ms, follow_redirect, retries);
}

inline Response get(const std::string& url_utf8, const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    std::wstring url;
    if (!detail::utf8_to_wide(url_utf8, url))
    {
        Response response;
        response.error_code = ErrorCode::EncodingError;
        response.error_message = detail::text(L"URL 不是有效的 UTF-8 编码");
        return response;
    }
    return get(url, headers, timeout_ms, follow_redirect, retries);
}

inline Response post(const std::wstring& url, const std::string& body, const std::string& content_type = "application/x-www-form-urlencoded", const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    Headers updated = headers;
    if (!updated.contains("Content-Type"))
    {
        updated.set("Content-Type", content_type);
    }
    return send_request(L"POST", url, body, updated, timeout_ms, follow_redirect, retries);
}

inline Response post(const std::string& url_utf8, const std::string& body, const std::string& content_type = "application/x-www-form-urlencoded", const Headers& headers = Headers(), int timeout_ms = 0, bool follow_redirect = true, int retries = 0)
{
    std::wstring url;
    if (!detail::utf8_to_wide(url_utf8, url))
    {
        Response response;
        response.error_code = ErrorCode::EncodingError;
        response.error_message = detail::text(L"URL 不是有效的 UTF-8 编码");
        return response;
    }
    return post(url, body, content_type, headers, timeout_ms, follow_redirect, retries);
}

inline DownloadResult download(const std::wstring& url, const std::wstring& destination, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
{
    SessionOptions session_options;
    session_options.enable_cookies = false;
    Session session(session_options);
    return session.download(url, destination, headers, options);
}

inline DownloadResult download(const std::string& url_utf8, const std::string& destination_utf8, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
{
    SessionOptions session_options;
    session_options.enable_cookies = false;
    Session session(session_options);
    return session.download(url_utf8, destination_utf8, headers, options);
}

inline DownloadResult download(const std::wstring& url, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
{
    SessionOptions session_options;
    session_options.enable_cookies = false;
    Session session(session_options);
    return session.download(url, headers, options);
}

inline DownloadResult download(const std::string& url_utf8, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
{
    SessionOptions session_options;
    session_options.enable_cookies = false;
    Session session(session_options);
    return session.download(url_utf8, headers, options);
}

inline std::string to_utf8(const std::string& src, int codepage)
{
    if (src.empty())
    {
        return std::string();
    }
    const char* source = detail::codepage_iconv_name(codepage);
    std::string output;
    if (source && detail::iconv_convert(src, source, "UTF-8", output))
    {
        return output;
    }
    return src;
}

inline std::string to_utf8(const std::string& src, const std::string& encoding)
{
    if (src.empty())
    {
        return std::string();
    }
    const char* source = detail::encoding_iconv_name(encoding);
    std::string output;
    if (source && detail::iconv_convert(src, source, "UTF-8", output))
    {
        return output;
    }
    return src;
}

inline std::string text_size(size_t bytes, const std::string& unit = std::string())
{
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int index = -1;
    if (!unit.empty())
    {
        std::string value = unit;
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::toupper(c));
        });
        if (value == "B")
        {
            index = 0;
        }
        else if (value == "KB")
        {
            index = 1;
        }
        else if (value == "MB")
        {
            index = 2;
        }
        else if (value == "GB")
        {
            index = 3;
        }
        else if (value == "TB")
        {
            index = 4;
        }
    }
    double size = static_cast<double>(bytes);
    int chosen = 0;
    if (index >= 0 && index <= 4)
    {
        for (int i = 0; i < index; ++i)
        {
            size /= 1024.0;
        }
        chosen = index;
    }
    else
    {
        while (size >= 1024.0 && chosen < 4)
        {
            size /= 1024.0;
            ++chosen;
        }
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << size << " " << units[chosen];
    return stream.str();
}

inline std::string substring(const std::string& str, size_t start, size_t end)
{
    if (start >= str.size() || end < start)
    {
        return std::string();
    }
    if (end >= str.size())
    {
        end = str.size() - 1;
    }
    return str.substr(start, end - start + 1);
}

inline std::string lines(const std::string& str, size_t start_line, size_t end_line)
{
    std::vector<std::string> values;
    std::istringstream stream(str);
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
        {
            line.erase(line.size() - 1);
        }
        values.push_back(line);
    }
    if (start_line < 1 || start_line > values.size() || start_line > end_line)
    {
        return std::string();
    }
    if (end_line > values.size())
    {
        end_line = values.size();
    }
    std::ostringstream result;
    for (size_t i = start_line - 1; i < end_line; ++i)
    {
        result << values[i];
        if (i + 1 < end_line)
        {
            result << '\n';
        }
    }
    return result.str();
}

enum class BrowserErrorCode
{
    None,
    UnsupportedPlatform,
    SdkUnavailable,
    LoaderNotFound,
    RuntimeNotFound,
    InvalidArgument,
    EncodingError,
    ComInitializationFailed,
    WindowClassFailed,
    WindowCreationFailed,
    EnvironmentCreationFailed,
    ControllerCreationFailed,
    NavigationFailed,
    MessageLoopFailed
};

struct BrowserOptions
{
    std::wstring title = L"FeatherCrawl";
    int width = 1100;
    int height = 760;
    bool resizable = true;
    bool script_enabled = true;
    bool devtools_enabled = true;
    bool context_menus_enabled = true;
    bool status_bar_enabled = true;
    bool default_script_dialogs_enabled = true;
    std::wstring user_data_folder;
    std::wstring browser_executable_folder;
};

struct BrowserResult
{
    BrowserErrorCode error_code = BrowserErrorCode::None;
    long native_error_code = 0;
    std::string error_message;
    int exit_code = 0;
    std::wstring runtime_version;

    bool ok() const
    {
        return error_code == BrowserErrorCode::None;
    }
};

inline bool webview2_available()
{
    return false;
}

inline std::wstring webview2_runtime_version()
{
    return std::wstring();
}

inline BrowserResult browse(const std::wstring&, const BrowserOptions& = BrowserOptions())
{
    BrowserResult result;
    result.error_code = BrowserErrorCode::UnsupportedPlatform;
    result.error_message = detail::text_pair("WebView2 browsing is only available on Windows", L"WebView2 浏览功能仅支持 Windows");
    return result;
}

inline BrowserResult browse(const std::string& url_utf8, const BrowserOptions& options = BrowserOptions())
{
    std::wstring url;
    if (!detail::utf8_to_wide(url_utf8, url))
    {
        BrowserResult result;
        result.error_code = BrowserErrorCode::EncodingError;
        result.error_message = detail::text_pair("URL is not valid UTF-8", L"URL 不是有效的 UTF-8 编码");
        return result;
    }
    return browse(url, options);
}

inline BrowserResult render_html(const std::wstring&, const BrowserOptions& = BrowserOptions())
{
    BrowserResult result;
    result.error_code = BrowserErrorCode::UnsupportedPlatform;
    result.error_message = detail::text_pair("WebView2 HTML rendering is only available on Windows", L"WebView2 HTML 渲染功能仅支持 Windows");
    return result;
}

inline BrowserResult render_html(const std::string& html_utf8, const BrowserOptions& options = BrowserOptions())
{
    std::wstring html;
    if (!detail::utf8_to_wide(html_utf8, html))
    {
        BrowserResult result;
        result.error_code = BrowserErrorCode::EncodingError;
        result.error_message = detail::text_pair("HTML content is not valid UTF-8", L"HTML 内容不是有效的 UTF-8 编码");
        return result;
    }
    return render_html(html, options);
}

}

#else
#error FeatherCrawl supports Windows and Linux only.
#endif
