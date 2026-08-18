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

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#endif

namespace web
{

namespace detail
{

inline char ascii_lower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
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

    DWORD flags = 0;
    if (codepage == CP_UTF8)
    {
        flags = MB_ERR_INVALID_CHARS;
    }

    const int src_len = static_cast<int>(src.size());
    const int wide_len = MultiByteToWideChar(
        codepage,
        flags,
        src.data(),
        src_len,
        NULL,
        0);

    if (wide_len <= 0)
    {
        return false;
    }

    out.assign(static_cast<size_t>(wide_len), L'\0');
    const int converted = MultiByteToWideChar(
        codepage,
        flags,
        src.data(),
        src_len,
        &out[0],
        wide_len);

    if (converted != wide_len)
    {
        out.clear();
        return false;
    }

    return true;
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

    const int src_len = static_cast<int>(src.size());
    const int out_len = WideCharToMultiByte(
        codepage,
        0,
        src.data(),
        src_len,
        NULL,
        0,
        NULL,
        NULL);

    if (out_len <= 0)
    {
        return false;
    }

    out.assign(static_cast<size_t>(out_len), '\0');
    const int converted = WideCharToMultiByte(
        codepage,
        0,
        src.data(),
        src_len,
        &out[0],
        out_len,
        NULL,
        NULL);

    if (converted != out_len)
    {
        out.clear();
        return false;
    }

    return true;
}

inline bool utf8_to_wide(const std::string& src, std::wstring& out)
{
    return bytes_to_wide(src, CP_UTF8, out);
}

inline std::string wide_to_utf8(const std::wstring& src)
{
    std::string result;
    if (!wide_to_codepage(src, CP_UTF8, result))
    {
        return "";
    }
    return result;
}

inline UINT output_codepage()
{
    UINT codepage = GetConsoleOutputCP();
    if (codepage == 0)
    {
        codepage = GetACP();
    }
    if (codepage == 0)
    {
        codepage = CP_UTF8;
    }
    return codepage;
}

inline std::string wide_to_output(const std::wstring& src)
{
    std::string result;
    if (wide_to_codepage(src, output_codepage(), result))
    {
        return result;
    }
    if (wide_to_codepage(src, CP_UTF8, result))
    {
        return result;
    }
    return "";
}

inline std::string text(const wchar_t* value)
{
    return wide_to_output(value ? std::wstring(value) : std::wstring());
}

inline bool is_valid_utf8(const std::string& value)
{
    std::wstring temp;
    return utf8_to_wide(value, temp);
}

inline bool convert_codepage(const std::string& src, UINT from_codepage, UINT to_codepage, std::string& out)
{
    std::wstring wide;
    if (!bytes_to_wide(src, from_codepage, wide))
    {
        return false;
    }
    return wide_to_codepage(wide, to_codepage, out);
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

inline std::string normalize_encoding_name(const std::string& encoding)
{
    std::string value = trim_ascii(encoding);
    if (value.size() >= 2)
    {
        if ((value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\''))
        {
            value = value.substr(1, value.size() - 2);
        }
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
    const std::string value = normalize_encoding_name(encoding);

    if (value == "utf-8" || value == "utf8")
    {
        codepage = CP_UTF8;
        return true;
    }
    if (value == "gbk" || value == "gb2312" || value == "cp936")
    {
        codepage = 936;
        return true;
    }
    if (value == "gb18030" || value == "cp54936")
    {
        codepage = 54936;
        return true;
    }
    if (value == "big5" || value == "big-5" || value == "cp950")
    {
        codepage = 950;
        return true;
    }
    if (value == "shift-jis" || value == "shiftjis" || value == "sjis" || value == "cp932")
    {
        codepage = 932;
        return true;
    }
    if (value == "euc-kr" || value == "cp949")
    {
        codepage = 949;
        return true;
    }
    if (value == "iso-8859-1" || value == "latin1" || value == "latin-1")
    {
        codepage = 28591;
        return true;
    }
    if (value == "windows-1252" || value == "cp1252")
    {
        codepage = 1252;
        return true;
    }

    return false;
}

inline std::string extract_charset(const std::string& source)
{
    const std::string lower = ascii_lower_copy(source);
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
            return "";
        }

        char quote = 0;
        if (source[p] == '"' || source[p] == '\'')
        {
            quote = source[p];
            ++p;
        }

        const size_t start = p;
        while (p < source.size())
        {
            const unsigned char ch = static_cast<unsigned char>(source[p]);
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
    if (body.empty())
    {
        return "";
    }

    const size_t scan_size = (std::min)(body.size(), static_cast<size_t>(16384));
    return extract_charset(body.substr(0, scan_size));
}

inline bool is_text_content_type(const std::string& content_type)
{
    if (content_type.empty())
    {
        return false;
    }

    const std::string value = ascii_lower_copy(content_type);
    if (value.find("text/") == 0)
    {
        return true;
    }
    if (value.find("application/json") != std::string::npos ||
        value.find("+json") != std::string::npos ||
        value.find("application/xml") != std::string::npos ||
        value.find("+xml") != std::string::npos ||
        value.find("application/javascript") != std::string::npos ||
        value.find("application/x-javascript") != std::string::npos ||
        value.find("application/x-www-form-urlencoded") != std::string::npos ||
        value.find("application/graphql") != std::string::npos)
    {
        return true;
    }

    return false;
}

inline bool looks_like_text(const std::string& body)
{
    if (body.empty())
    {
        return false;
    }

    size_t i = 0;
    if (body.size() >= 3 &&
        static_cast<unsigned char>(body[0]) == 0xEF &&
        static_cast<unsigned char>(body[1]) == 0xBB &&
        static_cast<unsigned char>(body[2]) == 0xBF)
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

    const char ch = body[i];
    return ch == '<' || ch == '{' || ch == '[' || ch == '"' || ch == '\'';
}

inline bool prepare_request_body(
    const std::string& body,
    const std::string& content_type,
    std::string& prepared,
    std::string& error)
{
    prepared = body;
    error.clear();

    if (body.empty() || !is_text_content_type(content_type))
    {
        return true;
    }

    const std::string charset = extract_charset(content_type);
    if (!charset.empty())
    {
        UINT declared_codepage = 0;
        if (encoding_to_codepage(charset, declared_codepage))
        {
            if (declared_codepage != CP_UTF8)
            {
                return true;
            }

            if (is_valid_utf8(body))
            {
                return true;
            }

            const UINT acp = GetACP();
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

    const UINT acp = GetACP();
    if (acp != CP_UTF8 && convert_codepage(body, acp, CP_UTF8, prepared))
    {
        return true;
    }

    error = text(L"\u8bf7\u6c42\u4f53\u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u6587\u672c\uff0c\u4e5f\u65e0\u6cd5\u4ece Windows \u5f53\u524d\u7cfb\u7edf\u7f16\u7801\u8f6c\u6362\u4e3a UTF-8");
    return false;
}

inline std::string normalize_response_body(
    const std::string& body,
    const std::string& content_type)
{
    if (body.empty())
    {
        return body;
    }

    const bool text_response = is_text_content_type(content_type) || looks_like_text(body);
    if (!text_response)
    {
        return body;
    }

    std::string data = body;
    if (data.size() >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF)
    {
        data.erase(0, 3);
    }

    std::string charset = extract_charset(content_type);
    if (charset.empty())
    {
        charset = detect_html_charset(data);
    }

    UINT source_codepage = 0;
    if (!charset.empty() && encoding_to_codepage(charset, source_codepage))
    {
        std::wstring wide;
        if (bytes_to_wide(data, source_codepage, wide))
        {
            const std::string converted = wide_to_output(wide);
            if (!converted.empty() || data.empty())
            {
                return converted;
            }
        }
    }

    if (is_valid_utf8(data))
    {
        std::wstring wide;
        if (utf8_to_wide(data, wide))
        {
            const std::string converted = wide_to_output(wide);
            if (!converted.empty() || data.empty())
            {
                return converted;
            }
        }
    }

    const UINT acp = GetACP();
    if (acp != 0)
    {
        std::wstring wide;
        if (bytes_to_wide(data, acp, wide))
        {
            const std::string converted = wide_to_output(wide);
            if (!converted.empty() || data.empty())
            {
                return converted;
            }
        }
    }

    return data;
}

inline bool query_header_utf8(HINTERNET request, DWORD query, std::string& value)
{
    value.clear();

    DWORD size = 0;
    if (WinHttpQueryHeaders(
            request,
            query,
            WINHTTP_HEADER_NAME_BY_INDEX,
            WINHTTP_NO_OUTPUT_BUFFER,
            &size,
            WINHTTP_NO_HEADER_INDEX))
    {
        return true;
    }

    const DWORD error_code = GetLastError();
    if (error_code == ERROR_WINHTTP_HEADER_NOT_FOUND)
    {
        return true;
    }
    if (error_code != ERROR_INSUFFICIENT_BUFFER || size == 0)
    {
        return false;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(size / sizeof(wchar_t)) + 2, L'\0');
    DWORD actual_size = size;
    if (!WinHttpQueryHeaders(
            request,
            query,
            WINHTTP_HEADER_NAME_BY_INDEX,
            buffer.data(),
            &actual_size,
            WINHTTP_NO_HEADER_INDEX))
    {
        return false;
    }

    value = wide_to_utf8(std::wstring(buffer.data()));
    return true;
}

inline std::wstring winhttp_error_description(DWORD error_code)
{
    switch (error_code)
    {
    case ERROR_WINHTTP_TIMEOUT:
        return L"\u8bf7\u6c42\u8d85\u65f6";
    case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        return L"\u65e0\u6cd5\u89e3\u6790\u670d\u52a1\u5668\u540d\u79f0";
    case ERROR_WINHTTP_CANNOT_CONNECT:
        return L"\u65e0\u6cd5\u8fde\u63a5\u5230\u670d\u52a1\u5668";
    case ERROR_WINHTTP_CONNECTION_ERROR:
        return L"\u4e0e\u670d\u52a1\u5668\u7684\u8fde\u63a5\u53d1\u751f\u9519\u8bef";
    case ERROR_WINHTTP_SECURE_FAILURE:
        return L"HTTPS \u5b89\u5168\u8fde\u63a5\u5931\u8d25";
    case ERROR_WINHTTP_INVALID_URL:
        return L"URL \u65e0\u6548";
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
        return L"\u4e0d\u652f\u6301\u8be5 URL \u534f\u8bae";
    case ERROR_WINHTTP_LOGIN_FAILURE:
        return L"\u8eab\u4efd\u9a8c\u8bc1\u5931\u8d25";
    case ERROR_WINHTTP_OPERATION_CANCELLED:
        return L"\u64cd\u4f5c\u5df2\u53d6\u6d88";
    case ERROR_WINHTTP_HEADER_NOT_FOUND:
        return L"\u672a\u627e\u5230\u9700\u8981\u7684 HTTP \u54cd\u5e94\u5934";
    default:
        return L"WinHTTP \u8bf7\u6c42\u6267\u884c\u5931\u8d25";
    }
}

inline std::string winhttp_error(const wchar_t* operation, DWORD error_code)
{
    std::wostringstream oss;
    oss << operation
        << L"\u5931\u8d25\uff1a"
        << winhttp_error_description(error_code)
        << L"\uff08WinHTTP \u9519\u8bef\u7801 "
        << error_code
        << L"\uff09";
    return wide_to_output(oss.str());
}

inline std::wstring normalize_url_for_crack(const std::wstring& url)
{
    const size_t scheme_pos = url.find(L"://");
    if (scheme_pos == std::wstring::npos)
    {
        return url;
    }

    const size_t authority_start = scheme_pos + 3;
    const size_t first_delimiter = url.find_first_of(L"/?#", authority_start);
    if (first_delimiter != std::wstring::npos &&
        (url[first_delimiter] == L'?' || url[first_delimiter] == L'#'))
    {
        std::wstring normalized = url;
        normalized.insert(first_delimiter, 1, L'/');
        return normalized;
    }

    return url;
}

inline bool valid_header_name(const std::string& name)
{
    if (name.empty())
    {
        return false;
    }

    for (size_t i = 0; i < name.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        const bool alpha_num =
            (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z');

        const bool punctuation =
            c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
            c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
            c == '^' || c == '_' || c == '`' || c == '|' || c == '~';

        if (!alpha_num && !punctuation)
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

}

struct Response
{
    int status_code = 0;
    std::string body;
    std::string error_message = detail::text(L"\u65e0");
};

struct Headers
{
    std::unordered_map<std::string, std::string> fields;

    void set(const std::string& key, const std::string& value)
    {
        for (auto it = fields.begin(); it != fields.end(); ++it)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                it->second = value;
                return;
            }
        }
        fields[key] = value;
    }

    std::string get(const std::string& key) const
    {
        for (auto it = fields.begin(); it != fields.end(); ++it)
        {
            if (detail::ascii_iequals(it->first, key))
            {
                return it->second;
            }
        }
        return "";
    }

    std::string to_winhttp_string() const
    {
        std::string result;
        for (auto it = fields.begin(); it != fields.end(); ++it)
        {
            result += it->first;
            result += ": ";
            result += it->second;
            result += "\r\n";
        }
        return result;
    }
};

namespace detail
{

inline bool headers_contains(const Headers& headers, const std::string& key)
{
    for (auto it = headers.fields.begin(); it != headers.fields.end(); ++it)
    {
        if (ascii_iequals(it->first, key))
        {
            return true;
        }
    }
    return false;
}

inline bool validate_headers(const Headers& headers, std::string& error)
{
    for (auto it = headers.fields.begin(); it != headers.fields.end(); ++it)
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

inline Response make_error_response(const std::string& message)
{
    Response resp;
    resp.error_message = message;
    return resp;
}

inline bool is_retryable_http_status(int status_code)
{
    switch (status_code)
    {
    case 408:
    case 425:
    case 429:
    case 500:
    case 502:
    case 503:
    case 504:
        return true;
    default:
        return false;
    }
}

inline std::string http_error_message(int status_code)
{
    std::ostringstream oss;
    oss << text(L"HTTP \u8bf7\u6c42\u5931\u8d25\uff0c\u72b6\u6001\u7801\uff1a") << status_code;
    return oss.str();
}

inline void retry_delay(unsigned long long attempt_index)
{
    unsigned long long delay_ms = 200ULL * (attempt_index + 1ULL);
    if (delay_ms > 1000ULL)
    {
        delay_ms = 1000ULL;
    }
    Sleep(static_cast<DWORD>(delay_ms));
}

}

class WinHttpHandle
{
    HINTERNET handle_;

public:
    explicit WinHttpHandle(HINTERNET h = nullptr) : handle_(h) {}

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
            if (handle_)
            {
                WinHttpCloseHandle(handle_);
            }
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
    if (method.empty())
    {
        return detail::make_error_response(detail::text(L"HTTP \u65b9\u6cd5\u4e0d\u80fd\u4e3a\u7a7a"));
    }
    if (url.empty())
    {
        return detail::make_error_response(detail::text(L"URL \u4e0d\u80fd\u4e3a\u7a7a"));
    }
    if (timeout_ms < 0)
    {
        return detail::make_error_response(detail::text(L"\u8d85\u65f6\u65f6\u95f4\u4e0d\u80fd\u5c0f\u4e8e 0"));
    }
    if (retries < 0)
    {
        return detail::make_error_response(detail::text(L"\u91cd\u8bd5\u6b21\u6570\u4e0d\u80fd\u5c0f\u4e8e 0"));
    }
    if (url.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
    {
        return detail::make_error_response(detail::text(L"URL \u8fc7\u957f\uff0c\u8d85\u51fa WinHTTP \u53ef\u5904\u7406\u8303\u56f4"));
    }

    std::string header_validation_error;
    if (!detail::validate_headers(headers, header_validation_error))
    {
        return detail::make_error_response(header_validation_error);
    }

    std::string prepared_body;
    std::string body_error;
    if (!detail::prepare_request_body(body, headers.get("Content-Type"), prepared_body, body_error))
    {
        return detail::make_error_response(body_error);
    }

    if (prepared_body.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
    {
        return detail::make_error_response(detail::text(L"\u8bf7\u6c42\u4f53\u8fc7\u5927\uff0c\u8d85\u51fa\u5f53\u524d FeatherCrawl API \u53ef\u5904\u7406\u8303\u56f4"));
    }

    const std::wstring normalized_url = detail::normalize_url_for_crack(url);
    if (normalized_url.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
    {
        return detail::make_error_response(detail::text(L"URL \u8fc7\u957f\uff0c\u8d85\u51fa WinHTTP \u53ef\u5904\u7406\u8303\u56f4"));
    }

    URL_COMPONENTS url_comp = {};
    url_comp.dwStructSize = static_cast<DWORD>(sizeof(url_comp));
    url_comp.dwHostNameLength = 1;
    url_comp.dwUrlPathLength = 1;
    url_comp.dwExtraInfoLength = 1;

    if (!WinHttpCrackUrl(
            normalized_url.c_str(),
            static_cast<DWORD>(normalized_url.size()),
            0,
            &url_comp))
    {
        return detail::make_error_response(detail::winhttp_error(L"\u89e3\u6790 URL", GetLastError()));
    }

    if (url_comp.nScheme != INTERNET_SCHEME_HTTP &&
        url_comp.nScheme != INTERNET_SCHEME_HTTPS)
    {
        return detail::make_error_response(detail::text(L"\u4ec5\u652f\u6301 HTTP \u548c HTTPS URL"));
    }
    if (url_comp.lpszHostName == NULL || url_comp.dwHostNameLength == 0)
    {
        return detail::make_error_response(detail::text(L"URL \u4e2d\u6ca1\u6709\u6709\u6548\u7684\u670d\u52a1\u5668\u5730\u5740"));
    }

    std::wstring host(url_comp.lpszHostName, url_comp.dwHostNameLength);
    std::wstring path;

    if (url_comp.lpszUrlPath != NULL && url_comp.dwUrlPathLength > 0)
    {
        path.assign(url_comp.lpszUrlPath, url_comp.dwUrlPathLength);
    }
    if (path.empty())
    {
        path = L"/";
    }

    if (url_comp.lpszExtraInfo != NULL && url_comp.dwExtraInfoLength > 0)
    {
        std::wstring extra(url_comp.lpszExtraInfo, url_comp.dwExtraInfoLength);
        const size_t fragment_pos = extra.find(L'#');
        if (fragment_pos != std::wstring::npos)
        {
            extra.erase(fragment_pos);
        }
        if (!extra.empty())
        {
            path += extra;
        }
    }

    std::wstring wide_headers;
    const std::string header_string = headers.to_winhttp_string();
    if (!header_string.empty() && !detail::utf8_to_wide(header_string, wide_headers))
    {
        return detail::make_error_response(detail::text(L"HTTP \u8bf7\u6c42\u5934\u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801"));
    }

    const DWORD total_len = static_cast<DWORD>(prepared_body.size());
    const unsigned long long max_attempts = static_cast<unsigned long long>(retries) + 1ULL;
    std::string last_error = detail::text(L"\u8bf7\u6c42\u5931\u8d25");

    for (unsigned long long attempt = 0; attempt < max_attempts; ++attempt)
    {
        WinHttpHandle session(WinHttpOpen(
            L"FeatherCrawl/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0));

        if (!session)
        {
            last_error = detail::winhttp_error(L"\u6253\u5f00 WinHTTP \u4f1a\u8bdd", GetLastError());
            if (attempt + 1ULL < max_attempts)
            {
                detail::retry_delay(attempt);
            }
            continue;
        }

        WinHttpHandle connect(WinHttpConnect(
            session.get(),
            host.c_str(),
            url_comp.nPort,
            0));

        if (!connect)
        {
            last_error = detail::winhttp_error(L"\u8fde\u63a5\u670d\u52a1\u5668", GetLastError());
            if (attempt + 1ULL < max_attempts)
            {
                detail::retry_delay(attempt);
            }
            continue;
        }

        DWORD flags = 0;
        if (url_comp.nScheme == INTERNET_SCHEME_HTTPS)
        {
            flags |= WINHTTP_FLAG_SECURE;
        }

        WinHttpHandle request(WinHttpOpenRequest(
            connect.get(),
            method.c_str(),
            path.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags));

        if (!request)
        {
            last_error = detail::winhttp_error(L"\u521b\u5efa HTTP \u8bf7\u6c42", GetLastError());
            if (attempt + 1ULL < max_attempts)
            {
                detail::retry_delay(attempt);
            }
            continue;
        }

        if (timeout_ms > 0)
        {
            if (!WinHttpSetTimeouts(
                    request.get(),
                    timeout_ms,
                    timeout_ms,
                    timeout_ms,
                    timeout_ms))
            {
                return detail::make_error_response(
                    detail::winhttp_error(L"\u8bbe\u7f6e\u8d85\u65f6\u65f6\u95f4", GetLastError()));
            }
        }

        DWORD redirect_policy = follow_redirect
            ? WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP
            : WINHTTP_OPTION_REDIRECT_POLICY_NEVER;

        if (!WinHttpSetOption(
                request.get(),
                WINHTTP_OPTION_REDIRECT_POLICY,
                &redirect_policy,
                sizeof(redirect_policy)))
        {
            return detail::make_error_response(
                detail::winhttp_error(L"\u8bbe\u7f6e\u91cd\u5b9a\u5411\u7b56\u7565", GetLastError()));
        }

        if (!wide_headers.empty())
        {
            if (wide_headers.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
            {
                return detail::make_error_response(detail::text(L"HTTP \u8bf7\u6c42\u5934\u8fc7\u5927\uff0c\u8d85\u51fa WinHTTP \u53ef\u5904\u7406\u8303\u56f4"));
            }

            if (!WinHttpAddRequestHeaders(
                    request.get(),
                    wide_headers.c_str(),
                    static_cast<DWORD>(wide_headers.size()),
                    WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE))
            {
                return detail::make_error_response(
                    detail::winhttp_error(L"\u6dfb\u52a0 HTTP \u8bf7\u6c42\u5934", GetLastError()));
            }
        }

        LPVOID optional_data = WINHTTP_NO_REQUEST_DATA;
        if (total_len > 0)
        {
            optional_data = const_cast<char*>(prepared_body.data());
        }

        if (!WinHttpSendRequest(
                request.get(),
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0,
                optional_data,
                total_len,
                total_len,
                0))
        {
            last_error = detail::winhttp_error(L"\u53d1\u9001 HTTP \u8bf7\u6c42", GetLastError());
            if (attempt + 1ULL < max_attempts)
            {
                detail::retry_delay(attempt);
            }
            continue;
        }

        if (!WinHttpReceiveResponse(request.get(), NULL))
        {
            last_error = detail::winhttp_error(L"\u63a5\u6536\u670d\u52a1\u5668\u54cd\u5e94", GetLastError());
            if (attempt + 1ULL < max_attempts)
            {
                detail::retry_delay(attempt);
            }
            continue;
        }

        DWORD status_code = 0;
        DWORD status_size = sizeof(status_code);
        if (!WinHttpQueryHeaders(
                request.get(),
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &status_code,
                &status_size,
                WINHTTP_NO_HEADER_INDEX))
        {
            last_error = detail::winhttp_error(L"\u8bfb\u53d6 HTTP \u72b6\u6001\u7801", GetLastError());
            if (attempt + 1ULL < max_attempts)
            {
                detail::retry_delay(attempt);
            }
            continue;
        }

        Response current;
        current.status_code = static_cast<int>(status_code);

        DWORD content_length = 0;
        DWORD content_length_size = sizeof(content_length);
        if (WinHttpQueryHeaders(
                request.get(),
                WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &content_length,
                &content_length_size,
                WINHTTP_NO_HEADER_INDEX))
        {
            current.body.reserve(static_cast<size_t>(content_length));
        }

        bool read_failed = false;
        for (;;)
        {
            char chunk[16384];
            DWORD bytes_read = 0;

            if (!WinHttpReadData(request.get(), chunk, sizeof(chunk), &bytes_read))
            {
                last_error = detail::winhttp_error(L"\u8bfb\u53d6\u54cd\u5e94\u6570\u636e", GetLastError());
                read_failed = true;
                break;
            }

            if (bytes_read == 0)
            {
                break;
            }

            current.body.append(chunk, static_cast<size_t>(bytes_read));
        }

        if (read_failed)
        {
            if (attempt + 1ULL < max_attempts)
            {
                detail::retry_delay(attempt);
            }
            continue;
        }

        std::string content_type;
        if (!detail::query_header_utf8(request.get(), WINHTTP_QUERY_CONTENT_TYPE, content_type))
        {
            content_type.clear();
        }

        current.body = detail::normalize_response_body(current.body, content_type);

        if (current.status_code >= 400)
        {
            current.error_message = detail::http_error_message(current.status_code);
        }
        else
        {
            current.error_message = detail::text(L"\u65e0");
        }

        if (detail::is_retryable_http_status(current.status_code) &&
            attempt + 1ULL < max_attempts)
        {
            detail::retry_delay(attempt);
            continue;
        }

        if (detail::is_retryable_http_status(current.status_code) &&
            max_attempts > 1ULL)
        {
            std::ostringstream oss;
            oss << current.error_message
                << detail::text(L"\uff0c\u5df2\u5c1d\u8bd5 ")
                << max_attempts
                << detail::text(L" \u6b21");
            current.error_message = oss.str();
        }

        return current;
    }

    Response resp;
    resp.error_message = last_error;
    if (max_attempts > 1ULL)
    {
        std::ostringstream oss;
        oss << resp.error_message
            << detail::text(L"\uff0c\u5df2\u5c1d\u8bd5 ")
            << max_attempts
            << detail::text(L" \u6b21");
        resp.error_message = oss.str();
    }
    return resp;
}

inline Response get(
    const std::wstring& url,
    const Headers& headers = Headers(),
    int timeout_ms = 0,
    bool follow_redirect = true,
    int retries = 0)
{
    return send_request(L"GET", url, "", headers, timeout_ms, follow_redirect, retries);
}

inline Response get(
    const std::string& url_utf8,
    const Headers& headers = Headers(),
    int timeout_ms = 0,
    bool follow_redirect = true,
    int retries = 0)
{
    std::wstring wurl;
    if (!detail::utf8_to_wide(url_utf8, wurl))
    {
        return detail::make_error_response(detail::text(L"URL \u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801"));
    }
    return get(wurl, headers, timeout_ms, follow_redirect, retries);
}

inline Response post(
    const std::wstring& url,
    const std::string& body,
    const std::string& content_type = "application/x-www-form-urlencoded",
    const Headers& headers = Headers(),
    int timeout_ms = 0,
    bool follow_redirect = true,
    int retries = 0)
{
    Headers h = headers;
    if (!detail::headers_contains(h, "Content-Type"))
    {
        h.set("Content-Type", content_type);
    }
    return send_request(L"POST", url, body, h, timeout_ms, follow_redirect, retries);
}

inline Response post(
    const std::string& url_utf8,
    const std::string& body,
    const std::string& content_type = "application/x-www-form-urlencoded",
    const Headers& headers = Headers(),
    int timeout_ms = 0,
    bool follow_redirect = true,
    int retries = 0)
{
    std::wstring wurl;
    if (!detail::utf8_to_wide(url_utf8, wurl))
    {
        return detail::make_error_response(detail::text(L"URL \u4e0d\u662f\u6709\u6548\u7684 UTF-8 \u7f16\u7801"));
    }
    return post(wurl, body, content_type, headers, timeout_ms, follow_redirect, retries);
}

inline std::string to_utf8(const std::string& src, int codepage)
{
    if (src.empty())
    {
        return {};
    }

    std::string result;
    if (!detail::convert_codepage(src, static_cast<UINT>(codepage), CP_UTF8, result))
    {
        return src;
    }
    return result;
}

inline std::string to_utf8(const std::string& src, const std::string& encoding)
{
    UINT codepage = 0;
    if (detail::encoding_to_codepage(encoding, codepage))
    {
        return to_utf8(src, static_cast<int>(codepage));
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
        std::transform(
            u.begin(),
            u.end(),
            u.begin(),
            [](unsigned char c) -> char
            {
                return static_cast<char>(std::toupper(c));
            });

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
        for (int i = 0; i < unit_index; ++i)
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
            ++chosen_index;
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
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
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
    for (size_t i = start_line - 1; i < end_line; ++i)
    {
        result << lines_vec[i];
        if (i != end_line - 1)
        {
            result << '\n';
        }
    }
    return result.str();
}

}
