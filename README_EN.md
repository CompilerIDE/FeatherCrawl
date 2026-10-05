<h1 align="center">FeatherCrawl</h1>

<p align="center">
  <a href="README.md">Simplified Chinese</a> |
  <a href="README_EN.md">English</a>
</p>

<p align="center">
  <a href="https://github.com/CompilerIDE/FeatherCrawl/blob/main/LICENSE"><img src="https://img.shields.io/github/license/CompilerIDE/FeatherCrawl.svg?style=for-the-badge&new=1" alt="License"></a>
  <br>
</p>

FeatherCrawl is a lightweight C++ network request library. It is provided as a single header file and has a simple interface; however, **different platforms still need to link against the corresponding system or dependency libraries**. Before compiling, confirm the link parameters (see the "Features" and "Platform Dependencies and Compilation" sections below).

---

# Features

- Architecture:
  - Windows: Uses WinHTTP for HTTP communication, with optional Microsoft Edge WebView2 rendering
  - Linux: **Implements HTTP/TCP communication based on the Socket API, and uses OpenSSL to provide HTTPS/TLS support** (Socket handles the TCP layer, OpenSSL handles the TLS layer; the two are not substitutes for each other)
- Simple to call; only requires `#include <feathercrawl.h>`
- C++11 and above; supports running on Windows 10/11 and Linux
- Built-in Cookie management, redirection, retry, timeout, download progress, automatic character set detection, and more

---

# Platform Dependencies and Compilation

FeatherCrawl is a single-header library, but it **depends on system or third-party libraries**, which must be explicitly linked during compilation:

| Platform | Compilation Requirements |
|------|---------|
| Windows / MSVC | Compile directly. The header automatically links `winhttp.lib` via `#pragma comment(lib, ...)` (when WebView2 is enabled, it also automatically links `user32.lib` and `advapi32.lib`) |
| Windows / MinGW / TDM-GCC | Add `-lwinhttp` |
| Linux | Add `-lssl -lcrypto -pthread` (some distributions also require `-liconv` if using iconv) |

---

# Version and Compatibility

- Current version tag: `FeatherCrawl/2.0` (reflected in the default User-Agent)
- Last updated: 2026-09-25
- Language standard: C++11 and above
- Supported platforms: Windows 10/11, Linux

---

# Open Source License

FeatherCrawl is open source under the [Apache License 2.0](LICENSE).

---

# 1. Quick Start

## 1.1 Minimal Runnable Program

```cpp
#include <feathercrawl.h>
#include <iostream>
using namespace std;

int main()
{
    web::Session session;                                  // Create a session
    web::Response r = session.get("https://example.com");  // Send a GET request

    if (r.ok())                                            // Check whether the request succeeded
    {
        cout << r.body << endl;                            // Print the response body (scraped HTML, etc.)
    }
    else
    {
        cerr << r.error_message << endl;                   // Print the failure reason
    }
}
```

> Reminder: On Linux, you must link `-lssl -lcrypto -pthread`; on Windows / MinGW, you must link `-lwinhttp`. Otherwise, link errors will occur.

## 1.2 Three Core Objects

| Object | Purpose |
|------|------|
| `web::Session` | Manages the session, i.e. cookies, proxies, connection pool, etc. |
| `web::Headers` | The **request headers** sent by the user |
| `web::Response` | The result returned by the server (status code, response body, response headers, errors, etc.) |

> Note: Request headers `Headers` and response headers `ResponseHeaders` are **two different types**.
>
> - `Headers::fields` is `std::unordered_map<std::string, std::string>`
> - `ResponseHeaders::fields` is `std::unordered_map<std::string, std::vector<std::string>>`
>
> `Session`-related interfaces use `Headers`, while `Response::headers` is `ResponseHeaders`. They have different purposes and **cannot be assigned to each other**:

Example:
```cpp
web::Response r = session.get(url);
web::Headers h;
for (auto& kv : r.headers.fields)
{
    if (!kv.second.empty())
    {
        h.set(kv.first, kv.second.front());
    }
}
```

---

# 2. Language Settings

FeatherCrawl supports manual language switching, supports Chinese and English, and the default language is English.

## 2.1 `web::set_default_language(lang)`

Purpose: Sets the global default language. **This setting only affects Sessions created afterward, not already-created Sessions** (a Session copies the current global language when constructed).

Parameters: `lang` — `"zh_CN"` or `"en_US"` (also accepts aliases such as `"zh"`, `"cn"`, `"chinese"`).

Example:

```cpp
web::set_default_language("zh_CN");
web::Session a;               // a uses zh_CN

web::set_default_language("en_US");
web::Session b;               // b uses en_US; a is still zh_CN and is unaffected
```

## 2.2 `web::set_language(lang)`

Purpose: Equivalent to `web::set_default_language`; sets the global default language and only affects Sessions created afterward.

Example:
```cpp
web::set_language("zh_CN");
```

## 2.3 `Session::set_language(lang)`

Purpose: Sets the language of prompt messages for the current Session (including error messages and all internal prompt text), without affecting other Sessions.

Parameters: `std::string`.

Example:
```cpp
web::Session session;
session.set_language("zh_CN");
```

## 2.4 `Session::language()`

Purpose: Returns the language string used by the current Session.

Return value: `std::string`, either `"zh_CN"` or `"en_US"`.

Inheritance rules:
- A Session copies the global default language **at construction time**, and afterward it no longer changes with the global default language;
- If `Session::set_language()` is called later, the last set value is used.

Example:
```cpp
web::set_default_language("zh_CN");
web::Session s;                         // s.language() == "zh_CN"

web::set_default_language("en_US");
cout << s.language() << endl;           // Still zh_CN
```

---

# 3. Request Headers

`web::Headers` is used to store **request headers** and provides case-insensitive key-value lookup interfaces.

> `Headers` and `ResponseHeaders` are different structures; see section 1.2.

## 3.1 `set(key, value)`

Purpose: Adds or overwrites a request header (key comparison is case-insensitive).

Parameters: `key` and `value`. `key` is the request header name, such as `User-Agent`; `value` is the request header value.

Example:
```cpp
web::Headers headers;
headers.set("Content-Type", "application/json");
headers.set("Authorization", "Bearer 123456");
headers.set("User-Agent", "Crawler/1.0");
```

## 3.2 `get(key)`

Purpose: Reads the value of a request header, case-insensitively.

Returns: `std::string`, or an empty string if it does not exist.

Example:
```cpp
string key = headers.get("content-type");
```

## 3.3 `contains(key)`

Purpose: Checks whether a request header exists.

Returns: `bool`.

Example:
```cpp
if (headers.contains("Authorization")) { /* ... */ }
```

## 3.4 `erase(key)`

Purpose: Deletes the specified request header.

Example:
```cpp
headers.erase("Cookie");
```

## 3.5 `clear()`

Purpose: Clears all request headers.

Example:
```cpp
headers.clear();
```

## 3.6 `to_winhttp_string()`

Purpose: Converts request headers into a string joined as `"Key: Value\r\n"`.

Returns: `std::string`.

Example:
```cpp
string text = headers.to_winhttp_string();
```

---

# 4. Response Object

`web::Response` represents data returned by the server.

```cpp
web::Response r = session.get(url);
```

It returns a `Response` object containing:

- HTTP status code
- Response body
- Response headers (`ResponseHeaders`)
- Error code and error message
- Request statistics

## 4.1 `ok()`

Purpose: Checks whether the request succeeded.

Returns: `bool`.

Example:
```cpp
web::Response r = session.get("https://example.com");
if (r.ok())
{
    cout << "Request succeeded";
}
else
{
    cout << "Request failed";
}
```

## 4.2 `status_code`

Purpose: Gets the HTTP status code.

Returns: `int`.

Common values:

| Status Code | Meaning |
|:-:|:---:|
| 200 | Success |
| 301 | Permanent redirect |
| 302 | Temporary redirect |
| 400 | Bad request |
| 403 | Forbidden |
| 404 | Not found |
| 500 | Server error |

Example:
```cpp
web::Response r = session.get("https://example.com");
cout << r.status_code;
```

## 4.3 `body`

Purpose: Returns the body content saved by the server (such as webpage source code).

Type: `std::string`.

Example:
```cpp
web::Response r = session.get("https://example.com");
cout << r.body;
```

## 4.4 `headers`

Purpose: Gets the response headers returned by the server.

Type: `ResponseHeaders`.

Example:
```cpp
web::Response r = session.get("https://example.com");
string type = r.headers.get("Content-Type");
```

Common `ResponseHeaders` interfaces:

| Method | Purpose |
|------|------|
| `get(key)` | Returns the first matching value |
| `get_all(key)` | Returns all values of headers with the same name |
| `contains(key)` | Whether a certain response header is included |
| `clear()` | Clears |

## 4.5 `error_code`

Purpose: Gets the error type.

Example:
```cpp
if (!r.ok())
{
    auto erc = r.error_code;
}
```

## 4.6 `error_message`

Purpose: Gets the error description.

Type: `std::string`.

Example:
```cpp
if (!r.ok())
{
    cerr << r.error_message;
}
```

## 4.7 `final_url`

Purpose: Gets the final visited URL, useful for HTTP redirects or webpages that automatically redirect.

Type: `std::wstring`.

Example:
```cpp
web::Response r = session.get("https://example.com");
std::wcout << r.final_url << std::endl;
```

## 4.8 `received_bytes`

Purpose: Gets the size of received data.

Type: `size_t`.

Example:
```cpp
cout << r.received_bytes << " bytes";
```

## 4.9 `attempts`

Purpose: Gets the actual number of requests.

Type: `int`.

Example:
```cpp
cout << "Attempts:" << r.attempts;
```

## 4.10 `redirect_count`

Purpose: Gets the number of redirects.

Type: `int`.

Example:
```cpp
cout << r.redirect_count;
```

---

# 5. Network Session

## 5.1 `Session()`

Purpose: Creates a default network session.

Example:
```cpp
web::Session session;
```

## 5.2 `Session(options)`

Purpose: Creates a Session using configuration.

Parameters: `SessionOptions`.

Example:
```cpp
web::SessionOptions opt;
opt.user_agent = L"MyCrawler/1.0";
web::Session session(opt);
```

## 5.3 `get(url)`

Purpose: Sends a GET request.

Parameters:

| Parameter | Type | Description |
|:-:|:----:|:--:|
| url | `std::string` or `std::wstring` | Request URL |

Returns: `Response`.

Example:
```cpp
web::Session session;
web::Response r = session.get("https://example.com");
```

## 5.4 `get(url, headers)`

Purpose: Sends a GET request with headers.

Example:
```cpp
web::Session session;
web::Headers h;
h.set("User-Agent", "MyBot");
web::Response r = session.get("https://example.com", h);
```

## 5.5 `get(url, headers, options)`

Purpose: Complete GET request.

Parameters:

| Parameter | Type |
| ------- | ---------------- |
| url | `std::string` or `std::wstring` |
| headers | `Headers` |
| options | `RequestOptions` |

Example:
```cpp
web::Session session;
web::RequestOptions opt;
opt.timeout_ms = 5000;
web::Response r = session.get("https://example.com", web::Headers(), opt);
```

## 5.6 `post(url, body, content_type, headers, options)`

Purpose: Sends a POST request.

Parameters:

| Parameter | Description |
| ------------ | ----- |
| url | Request URL |
| body | Data to send |
| content_type | Data type (default `application/x-www-form-urlencoded`) |
| headers | Request headers (optional) |
| options | `RequestOptions` (optional) |

Returns: `Response`.

`post()` provides multiple overloads and can be used like `get()` with request headers and `RequestOptions`.

Example:
```cpp
web::Session session;

// Simple form
web::Response r1 = session.post("https://example.com/api", R"({"id":1})", "application/json");

// Complete form
web::Headers h;
h.set("Authorization", "Bearer 123456");
web::RequestOptions opt;
opt.timeout_ms = 8000;
web::Response r2 = session.post("https://example.com/api", R"({"id":1})", "application/json", h, opt);
```

## 5.7 `cookie_jar()`

Purpose: Gets the Cookie manager of the current session.

Returns: `CookieJar&`.

Example:
```cpp
web::Session s;
auto& jar = s.cookie_jar();
cout << jar.size();
```

## 5.8 `set_language(lang)`

Purpose: Sets the prompt message language of the current session.

Parameters: `std::string`.

Example:
```cpp
web::Session s;
s.set_language("zh_CN");
```

## 5.9 `language()`

Purpose: Gets the current language (the global default language copied at construction time, or the value explicitly set afterward).

Returns: `std::string`.

Example:
```cpp
web::Session s;
cout << s.language();
```

## 5.10 `Session::clear_cookies()`

Purpose: Clears the Cookies saved by the current Session.

Example:
```cpp
session.clear_cookies();
```

---

# 6. Request Options

`web::RequestOptions` is used to control the behavior of **a single HTTP request**.

It can set:

- Timeout
- Automatic redirect
- Maximum response size for a single request
- Retry policy

Example:
```cpp
web::RequestOptions options;
options.timeout_ms = 5000;
web::Response r = session.get("https://example.com", web::Headers(), options);
```

## 6.1 `timeout_ms`

Purpose: Sets the request timeout.

Unit: milliseconds. Default `0`.

Type: `int`.

Example:
```cpp
web::RequestOptions opt;
opt.timeout_ms = 10000;
```

## 6.2 `follow_redirect`

Purpose: Whether to automatically follow HTTP redirects.

Type: `bool`, default `true`.

Example:
```cpp
web::RequestOptions opt;
opt.follow_redirect = true;
```

## 6.3 `max_redirects`

Purpose: Sets the maximum number of redirects.

Type: `int`, default `-1`.

Example:
```cpp
web::RequestOptions opt;
opt.max_redirects = 10;
```

## 6.4 `max_response_size`

Purpose: Limits the size of data returned by the server for **a single request** (only for the in-memory read path).

Unit: bytes. Default `0`.

Type: `size_t`. When the value is `0`, `SessionOptions::max_response_size` is used (default 64 MiB).

Example:
```cpp
web::RequestOptions opt;
opt.max_response_size = 1024 * 1024; // Maximum 1 MiB
```

## 6.5 `retry`

Purpose: Sets the automatic retry policy.

Type: `RetryPolicy`.

Example:

```cpp
web::RequestOptions opt;
opt.retry.retries = 3;
```

---

# 7. Automatic Retry

`web::RetryPolicy` is used in unstable network environments.

For example:

- Network fluctuations
- Temporary server errors
- Timeouts

The fields and default values of `RetryPolicy` are as follows:

| Field | Type | Default Value | Purpose |
|------|------|-------|------|
| `retries` | `int` | `0` | Maximum number of retries |
| `initial_delay_ms` | `int` | `250` | Wait time before the first retry (milliseconds) |
| `max_delay_ms` | `int` | `4000` | Upper limit for a single wait time (milliseconds) |
| `multiplier` | `double` | `2.0` | Exponential growth multiplier for wait time |
| `jitter` | `bool` | `true` | Whether to add random jitter |
| `respect_retry_after` | `bool` | `true` | Whether to respect the `Retry-After` response header |
| `retry_non_idempotent` | `bool` | `false` | Whether to allow retries for non-idempotent methods (such as POST) |
| `allow_automatic_authentication` | `bool` | `false` | Whether to allow WinHTTP automatic authentication (**Windows/WinHTTP only**; the field exists on Linux but is unused) |

## 7.1 `retries`

Purpose: Sets the maximum number of retries.

Type: `int`.

Example:

```cpp
web::RequestOptions opt;
opt.retry.retries = 5;
```

## 7.2 `initial_delay_ms`

Purpose: Wait time before the first retry.

Unit: milliseconds.

Example:

```cpp
opt.retry.initial_delay_ms = 500;
```

## 7.3 `max_delay_ms`

Purpose: Limits the maximum wait time.

Example:

```cpp
opt.retry.max_delay_ms = 5000;
```

## 7.4 `multiplier`

Purpose: Sets the wait time growth multiplier.

Type: `double`.

Example:

```cpp
opt.retry.multiplier = 2.0;
```

---

# 8. Cookie Management

`web::CookieJar` is used to manage Cookies saved in a Session.

Cookies are stored **in memory**, are valid only during the Session lifetime, and **are not written to disk**.

It is usually obtained via:

```cpp
Session::cookie_jar()
```

> **Thread safety note**:
> Although `CookieJar` internally adds a mutex to its own operations, **the entire `Session` is not suitable for multi-threaded concurrent use**.
> It is recommended that the same `Session` be operated in only one thread, or that each thread be assigned an independent `Session`. See section 5.12 for details.

## 8.1 `size()`

Purpose: Returns the number of Cookies.

Returns: `size_t`.

Example:

```cpp
web::Session session;
auto& jar = session.cookie_jar();
auto count = jar.size();
cout << count;
```

## 8.2 `clear()`

Purpose: Clears all Cookies.

Example:

```cpp
session.cookie_jar().clear();
```

Or use the Session method directly:

```cpp
session.clear_cookies();
```

## 8.3 `delete_cookie(name)`

Purpose: Deletes the specified Cookie.

Parameter `name` type: `std::string`.

Example:

```cpp
session.cookie_jar().delete_cookie("sessionid");
```

Or:

```cpp
session.delete_cookie("sessionid");
```

## 8.4 Automatic Cookie Saving

By default, a Session saves Cookies returned by the server.

Example:

```cpp
web::Session session;
session.post("https://example.com/login", "user=test", "application/x-www-form-urlencoded");
web::Response r = session.get("https://example.com/user");
```

---

# 9. Session Options

`web::SessionOptions` is used to set default behavior when creating a Session.

Example:

```cpp
web::SessionOptions opt;
web::Session session(opt);
```

The fields and default values of `SessionOptions` are as follows:

| Field | Type | Default Value | Purpose |
|------|------|-------|------|
| `user_agent` | `std::wstring` | `L"FeatherCrawl/2.0"` | Default User-Agent |
| `enable_cookies` | `bool` | `true` | Whether to enable Cookies |
| `access_type` | `DWORD` | `WINHTTP_ACCESS_TYPE_DEFAULT_PROXY` | WinHTTP access type (**Windows only**) |
| `proxy` | `std::wstring` | Empty | Proxy server |
| `proxy_bypass` | `std::wstring` | Empty | Proxy bypass list |
| `max_response_size` | `size_t` | 64 MiB | Default maximum response size |
| `max_redirects` | `int` | `10` | Default maximum number of redirects |
| `max_connections` | `size_t` | `64` | Connection pool upper limit (**Windows**; unused on Linux) |

## 9.1 `user_agent`

Purpose: Sets the default User-Agent.

Type: `std::wstring`.

Example:

```cpp
web::SessionOptions opt;
opt.user_agent = L"MyCrawler/1.0";
web::Session s(opt);
```

## 9.2 `enable_cookies`

Purpose: Whether to enable Cookies.

Type: `bool`.

Example:

```cpp
web::SessionOptions opt;
opt.enable_cookies = true;
```

## 9.3 `proxy`

Purpose: Sets the proxy server.

Type: `std::wstring`.

Example:

```cpp
web::SessionOptions opt;
opt.proxy = L"http://127.0.0.1:7890";
web::Session s(opt);
```

## 9.4 `max_response_size`

Purpose: Sets the default maximum response size for the session.

Type: `size_t`. Default 64 MiB (`64ULL * 1024ULL * 1024ULL`).

Example:

```cpp
web::SessionOptions opt;
opt.max_response_size = 64 * 1024 * 1024; // 64 MiB
```

---

# 10. File Download System

FeatherCrawl provides dedicated download interfaces for:

- Downloading ordinary files
- Downloading large files
- Displaying download progress
- Obtaining download statistics

Core objects:

| Object | Purpose |
| --------------------- | ---- |
| `DownloadOptions` | Download configuration |
| `DownloadResult` | Download result |
| `download()` | Execute download |

> **The download file size limit uses `DownloadOptions::max_file_size`**.
> `DownloadOptions::request.max_response_size` is **not** used in the download path to limit file size. Do not confuse them (cross-reference section 6.4).

## 10.1 `download(url, filename, headers, options)`

Purpose: Downloads a file to local storage.

Parameters:

| Parameter | Type | Description |
| -------- | ----------------- | ---- |
| url | `std::string` or `std::wstring` | File URL |
| filename | `std::string` or `std::wstring` | Save path |
| headers | `Headers` | Request headers (optional) |
| options | `DownloadOptions` | Download configuration (optional) |

Returns: `DownloadResult`.

Example:

```cpp
#include <feathercrawl.h>
#include <iostream>
using namespace std;

int main()
{
    web::Session session;
    web::DownloadOptions opt;
    web::DownloadResult r = session.download(
        "https://example.com/test.zip",
        "test.zip",
        web::Headers(),
        opt
    );

    if (r.ok())
    {
        cout << "Download complete";
    }
    else
    {
        cerr << r.error_message << endl;
    }
}
```

If you do not need custom request headers, you can pass only the first two parameters:

```cpp
web::DownloadResult r = session.download("https://example.com/test.zip", "test.zip");
```

---

# 11. Download Options

`web::DownloadOptions` is used to control download behavior.

## 11.1 `show_progress`

Purpose: Whether to display download progress.

Type: `bool`, default `true`.

Example:

```cpp
web::DownloadOptions opt;
opt.show_progress = true;
```

## 11.2 `request.timeout_ms`

Purpose: Sets the download timeout.

Unit: milliseconds.

Type: `int`.

Example:

```cpp
web::DownloadOptions opt;
opt.request.timeout_ms = 30000;
```

## 11.3 `overwrite`

Purpose: Whether to overwrite an existing file.

Type: `bool`, default `false`.

Example:

```cpp
web::DownloadOptions opt;
opt.overwrite = true;
```

## 11.4 `request.retry`

Purpose: Sets retry on download failure.

Type: `RetryPolicy`.

Example:

```cpp
web::DownloadOptions opt;
opt.request.retry.retries = 5;
```

## 11.5 Other Download Options

| Field | Type | Default Value | Purpose |
|------|------|-------|------|
| `max_file_size` | `uint64_t` | 1 GiB (`1024ULL * 1024ULL * 1024ULL`) | Maximum allowed download file size (**use this field for the download size limit**) |
| `progress_bar_width` | `size_t` | `20` | Progress bar width |
| `progress_refresh_ms` | `unsigned int` | `100` | Progress refresh interval (milliseconds) |

---

# 12. Download Result

`web::DownloadResult` represents the result after a download completes.

## 12.1 `ok()`

Purpose: Checks whether the download succeeded.

Returns: `bool`.

Example:

```cpp
if (result.ok())
{
    cout << "Success";
}
```

## 12.2 `bytes_written`

Purpose: Actual number of bytes written to the file.

Type: `uint64_t`.

Example:

```cpp
cout << result.bytes_written;
```

## 12.3 `file_size`

Purpose: Total file size.

Type: `uint64_t`.

Example:

```cpp
cout << result.file_size;
```

## 12.4 `destination`

Purpose: Saved file path.

Type: `std::wstring`.

Example:

```cpp
std::wcout << result.destination << std::endl;
```

## 12.5 `error_message`

Purpose: Gets the reason for download failure.

Type: `std::string`.

Example:

```cpp
if (!result.ok())
{
    cerr << result.error_message;
}
```

## 12.6 Other Download Result Fields

| Field | Type | Purpose |
|------|------|------|
| `status_code` | `int` | HTTP status code |
| `headers` | `ResponseHeaders` | Response headers |
| `error_code` | `ErrorCode` | Error code |
| `native_error_code` | `DWORD` | Underlying error code (on Linux, `DWORD` is an alias of `unsigned long`; its value may come from `errno`, a `getaddrinfo` return value, or an OpenSSL error code) |
| `file_size_known` | `bool` | Whether the total file size is known |
| `attempts` | `int` | Actual number of requests |
| `redirect_count` | `int` | Number of redirects |
| `final_url` | `std::wstring` | Final download URL |

---

# 13. Utility Functions

FeatherCrawl also provides some convenient utility functions.

## 13.1 `web::to_utf8(src, codepage)`

Purpose: Converts a string in the specified code page to UTF-8.

Example:

```cpp
std::string utf8 = web::to_utf8(gbk_text, 936);
```

## 13.2 `web::to_utf8(src, encoding)`

Purpose: Converts a string with the specified encoding name to UTF-8.

Example:

```cpp
std::string utf8 = web::to_utf8(gbk_text, "GBK");
```

## 13.3 `web::text_size(bytes, unit)`

Purpose: Formats a byte count into a readable string.

Example:

```cpp
cout << web::text_size(1024 * 1024); // 1.00 MB
```

## 13.4 `web::substring(str, start, end)`

Purpose: Extracts a substring by index (inclusive of `start` and `end`).

Example:

```cpp
std::string part = web::substring("Hello World", 0, 4); // "Hello"
```

## 13.5 `web::lines(str, start_line, end_line)`

Purpose: Extracts a substring by lines; line numbers start from 1.

Example:

```cpp
std::string part = web::lines("a\nb\nc", 2, 3); // "b\nc"
```

---

# 14. Browser and HTML Rendering (Windows Only)

On the Windows platform, FeatherCrawl optionally supports Microsoft Edge WebView2 for opening webpages or rendering HTML.

> **Platform limitation**: `browse()` and `render_html()` are only actually implemented on the **Windows** platform.

> **Blocking semantics**: On Windows, `browse()` and `render_html()` create a window and enter a message loop, **blocking the current thread until the window is closed before returning**. If concurrency is needed, call them from a separate thread.

## 14.1 `webview2_available()`

Purpose: Checks whether WebView2 is available in the current environment.

Returns: `bool`.

Example:

```cpp
if (web::webview2_available())
{
    cout << "WebView2 available";
}
```

## 14.2 `webview2_runtime_version()`

Purpose: Gets the WebView2 runtime version.

Returns: `std::wstring`.

Example:

```cpp
std::wstring version = web::webview2_runtime_version();
std::wcout << version << std::endl;
```

## 14.3 `browse(url, options)`

Purpose: Opens a window to browse the specified URL. **Blocks until the window is closed before returning.**

Example:

```cpp
web::BrowserOptions opt;
opt.title = L"My Browser";
opt.width = 1200;
opt.height = 800;

web::BrowserResult r = web::browse("https://example.com", opt);
if (!r.ok())
{
    cerr << r.error_message << endl;
}
```

## 14.4 `render_html(html, options)`

Purpose: Renders an HTML string. **Blocks until the window is closed before returning.**

Size limit: about 2 MiB.

Example:

```cpp
web::BrowserResult r = web::render_html("<h1>Hello</h1>");
if (!r.ok())
{
    cerr << r.error_message << endl;
}
```

## 14.5 `BrowserOptions`

| Field | Type | Default Value | Purpose |
|------|------|--------|------|
| `title` | `std::wstring` | `L"FeatherCrawl"` | Window title |
| `width` | `int` | `1100` | Window width |
| `height` | `int` | `760` | Window height |
| `resizable` | `bool` | `true` | Whether resizable |
| `script_enabled` | `bool` | `true` | Whether scripts are enabled |
| `devtools_enabled` | `bool` | `true` | Whether developer tools are enabled |
| `context_menus_enabled` | `bool` | `true` | Whether context menus are enabled |
| `status_bar_enabled` | `bool` | `true` | Whether the status bar is enabled |
| `default_script_dialogs_enabled` | `bool` | `true` | Whether default script dialogs are enabled |
| `user_data_folder` | `std::wstring` | Empty | User data directory. **When empty, the library creates a temporary directory and automatically cleans it up after the window closes** |
| `browser_executable_folder` | `std::wstring` | Empty | Browser executable folder |

## 14.6 `BrowserResult`

| Field | Type | Purpose |
|------|------|------|
| `error_code` | `BrowserErrorCode` | Error code |
| `native_error_code` | On Windows: `HRESULT`; on Linux: `long` | Underlying error code |
| `error_message` | `std::string` | Error description |
| `exit_code` | `int` | Window exit code |
| `runtime_version` | `std::wstring` | WebView2 runtime version |
| `ok()` | `bool` | Whether successful |

---

# 15. Frequently Asked Questions (FAQ)

**Q1: Linux compilation fails, reporting that OpenSSL symbols cannot be found?**

A: Add `-lssl -lcrypto -pthread` to the compilation parameters. When using CMake, use `find_package(OpenSSL REQUIRED)` and link `OpenSSL::SSL` and `OpenSSL::Crypto`.

**Q2: Windows / MinGW compilation fails, reporting that `WinHttpOpen` and others are undefined?**

A: Add `-lwinhttp` to the compilation parameters. Under MSVC, because `feathercrawl.h` uses `#pragma comment(lib, "winhttp.lib")`, manual linking is not required.

**Q3: File download fails, reporting "destination file already exists"?**

A: `DownloadOptions::overwrite` defaults to `false`. Set:

```cpp
web::DownloadOptions opt;
opt.overwrite = true;
```

**Q4: HTTPS request returns `TlsFailure`?**

A: Possible reasons include: incorrect system time, incomplete system CA certificates, or an expired target server certificate. First calibrate the system time and check the system CA. The current version does not provide a switch to skip certificate verification.

**Q5: `final_url` and `destination` cannot be output with `cout`?**

A: These two fields are of type `std::wstring`; use `std::wcout`:

```cpp
std::wcout << r.final_url << std::endl;
std::wcout << result.destination << std::endl;
```

**Q6: `Session::cookies()` cannot be called?**

A: The current interface is `Session::cookie_jar()`; use:

```cpp
auto& jar = session.cookie_jar();
```

**Q7: `DownloadOptions::timeout_ms` is not recognized?**

A: `timeout_ms` is located in the `DownloadOptions::request` sub-object:

```cpp
opt.request.timeout_ms = 30000;
```

**Q8: `browse()` / `render_html()` return `UnsupportedPlatform` on Linux?**

A: WebView2 rendering is **supported only on Windows**. On Linux, these two functions only return an error code and do not open a window.

**Q9: I changed the global default language, why has the language of an existing Session not changed?**

A: A Session copies the global default language at construction time. To change the language of an existing Session, call `Session::set_language()`.

**Q10: Can it be used on macOS?**

A: No. macOS is not "unsupported"; rather, **including the header directly triggers `#error` and compilation fails**.

**Q11: Do `browse()` / `render_html()` block the main thread?**

A: Yes. On Windows, these two functions enter a message loop, **blocking the current thread until the window is closed**. If concurrency is needed, call them from a separate thread.

**Q12: Can the same `Session` be used simultaneously in multiple threads?**

A: Not recommended. `Session` is not designed for multi-threaded concurrency. Issuing multiple requests at the same time or modifying state at the same time is undefined behavior. Assign an independent `Session` to each thread.

---

<h3 align="center">FeatherCrawl — Simple, Efficient C++ Network Request Library.</h3>
