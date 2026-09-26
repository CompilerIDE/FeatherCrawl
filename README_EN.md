<h1 align="center">FeatherCrawl</h1>

<p align="center">
  <a href="README.md">简体中文</a> |
  <a href="README_EN.md">English</a>
</p>

<p align="center">
  <a href="https://github.com/CompilerIDE/FeatherCrawl/blob/main/LICENSE"><img src="https://img.shields.io/github/license/CompilerIDE/FeatherCrawl.svg?style=for-the-badge&new=1" alt="License"></a>
  <br>
</p>

FeatherCrawl is a C++ network request library. It enables web scraping in C++ with a single header file.

---

## Features

- Architecture: WinHTTP
- Simple integration — only `#include "feathercrawl.h"` is required
- C++11 or later, supports running on Windows 10/11 and Linux.

---

## License

FeatherCrawl is open source under the [Apache License 2.0](LICENSE).

---
# Usage Examples

## 1. Quick Start

### 1.1 Minimal Runnable Program
```cpp
#include <feathercrawl.h>
#include <iostream>
using namespace std;
int main()
{
    web::Session session; // Create a session
    web::Response r = session.get("https://example.com"); // Send a GET request
    if (r.ok()) // Check whether the request succeeded
    {
        cout << r.body << endl; // Print the response body (i.e., the HTML, etc. you want to crawl)
    }
    else
    {
        cerr << r.error_message << endl; // Print the failure reason
    }
}
```

**Compilation Commands**

| Platform | Command |
|------|------|
| Windows / MSVC | Compile directly; automatically links `winhttp.lib` |
| Windows / MinGW | Add the `-lwinhttp` compilation flag |
| Linux | Add the `-lssl -lcrypto -pthread` compilation flags |

#### 1.2 Three Core Functions (Objects)
| Object | Purpose |
|------|--------|
| `web::Session` | Manages sessions, i.e., cookies, proxies, connection pools, etc. |
| `web::Headers` | Request headers sent by the user |
| `web::Response` | The result returned by the server (status code, response body, response headers, errors, etc.) |

## 2. Language Settings
FeatherCrawl supports manual language switching, supports Chinese and English, and the default language is English.

### 2.1 `web::set_default_language(lang)`

Purpose: Sets the global default language.

Parameters: lang — `"zh_CN"` or `"en_US"` (also accepts aliases such as `"zh"`, `"cn"`, and `"chinese"`).

Example:
```cpp
web::set_default_language("zh_CN");
web::set_default_language("en_US");
```

### 2.2 `Session::set_language(lang)`

Purpose: Sets the language only for the current session without affecting other sessions.

Example:
```cpp
web::Session session;
session.set_language("zh_CN");
```

### 2.3 `Session::language()`
Purpose: Returns the language string used by the current Session.

Returns: `string` type, with the value `"zh_CN"` or `"en_US"`.

Example:
```cpp
cout << s.language() << endl;
```

## 3. Request Headers
`web::Headers` represents the request headers you want to send.

### 3.1 `set(key, value)`
Purpose: Adds a request header.

Parameters: `key` and `value`. `key` represents the name of the request header, such as `User-Agent`; `value` represents the value of the request header.

Example:
```cpp
web::Headers headers;
headers.set("Content-Type", "application/json");
headers.set("Authorization", "Bearer 123456");
headers.set("User-Agent", "Crawler/1.0");
```

### 3.2 `get(key)`
Purpose: Reads the value of a request header; case-insensitive.

Example:
```cpp
string key = headers.get("content-type");
```

### 3.3 `contains(key)`
Purpose: Checks whether a request header exists.

Returns: `bool` type.

### 3.4 `clear()`
Purpose: Clears all request headers.

Example:
```cpp
headers.clear();
```

## 4. Session Object

---

<h3 align="center">FeatherCrawl — simple and powerful web scraping for C++.</h3>
