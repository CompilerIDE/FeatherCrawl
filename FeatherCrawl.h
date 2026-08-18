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
#include <sstream>
#include <iomanip>
#include <algorithm>
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

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
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

inline std::string text(const wchar_t* value)
{
    ensure_utf8_console();
    return wide_to_utf8(value ? std::wstring(value) : std::wstring());
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
    std::wostringstream oss;
    oss << operation << L"\u5931\u8d25\uff1a";
    std::wstring desc;
    std::string d = winhttp_error_description(code);
    if (utf8_to_wide(d, desc))
    {
        oss << desc;
    }
    else
    {
        oss << L"WinHTTP \u8bf7\u6c42\u6267\u884c\u5931\u8d25";
    }
    oss << L"\uff08WinHTTP \u9519\u8bef\u7801 " << code << L"\uff09";
    return wide_to_utf8(oss.str());
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
    size_t max_response_size = 64ULL * 1024ULL * 1024ULL;
    int max_redirects = 10;
    size_t max_connections = 64;
};

struct DownloadOptions
{
    RequestOptions request;
    uint64_t max_file_size = 1024ULL * 1024ULL * 1024ULL;
    bool overwrite = false;
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
    int attempts = 0;
    int redirect_count = 0;
    std::wstring final_url;

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
    HINTERNET get() const { return handle_; }
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
    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept { if (this != &other)
    {
         reset(other.handle_); other.handle_ = NULL;
    }
     return *this; }
    explicit operator bool() const { return handle_ != NULL; }
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
    HANDLE get() const { return handle_; }
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
    explicit operator bool() const { return handle_ != INVALID_HANDLE_VALUE && handle_ != NULL; }
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
    bool too_large() const { return too_large_; }
    const std::string& data() const { return data_; }
    uint64_t size() const { return static_cast<uint64_t>(data_.size()); }
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

    static bool exists(const std::wstring& path)
    {
        DWORD attr = GetFileAttributesW(path.c_str());
        return attr != INVALID_FILE_ATTRIBUTES;
    }

public:
    FileSink(const std::wstring& destination, uint64_t limit, bool overwrite) : destination_(destination), limit_(limit), overwrite_(overwrite), too_large_(false), written_(0), committed_(false)
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

    bool destination_exists() const { return exists(destination_); }

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
        return true;
    }

    bool too_large() const { return too_large_; }
    uint64_t size() const { return written_; }

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

    bool ensure_session()
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        if (session_)
        {
            return true;
        }
        HINTERNET h = WinHttpOpen(options_.user_agent.c_str(), options_.access_type, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
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
        if (!cl.empty() && detail::parse_uint64(cl, content_length))
        {
            if (!sink.reserve(content_length))
            {
                r.error_code = ErrorCode::ResponseTooLarge;
                r.error_message = detail::text(L"\u670d\u52a1\u5668\u54cd\u5e94\u8d85\u8fc7\u5141\u8bb8\u7684\u6700\u5927\u5927\u5c0f");
                return r;
            }
        }
        for (;;)
        {
            char buffer[32768];
            DWORD got = 0;
            if (!WinHttpReadData(request.get(), buffer, sizeof(buffer), &got))
            {
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
    explicit Session(const SessionOptions& options = SessionOptions()) : options_(options), cookies_enabled_(options.enable_cookies), init_native_error_(0)
    {

        }
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

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

    DownloadResult download(const std::wstring& url, const std::wstring& destination, const Headers& headers = Headers(), DownloadOptions options = DownloadOptions())
    {
        DownloadResult d;
        if (destination.empty())
        {
            d.error_code = ErrorCode::InvalidArgument;
            d.error_message = detail::text(L"\u4e0b\u8f7d\u6587\u4ef6\u8def\u5f84\u4e0d\u80fd\u4e3a\u7a7a");
            return d;
        }
        FileSink sink(destination, options.max_file_size, options.overwrite);
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

}
