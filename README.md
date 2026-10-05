<h1 align="center">FeatherCrawl</h1>

<p align="center">
  <a href="README.md">简体中文</a> |
  <a href="README_EN.md">English</a>
</p>

<p align="center">
  <a href="https://github.com/CompilerIDE/FeatherCrawl/blob/main/LICENSE"><img src="https://img.shields.io/github/license/CompilerIDE/FeatherCrawl.svg?style=for-the-badge&new=1" alt="License"></a>
  <br>
</p>

FeatherCrawl 是一个轻量级 C++ 网络请求库，采用单头文件形式，接口使用简单；但**不同平台仍需要链接对应的系统库或依赖库**，编译前请确认链接参数（详见下文“特性”与“平台依赖”说明）。

---

# 特性

- 架构：
  - Windows：使用 WinHTTP 实现 HTTP 通信，可选 Microsoft Edge WebView2 渲染
  - Linux：**基于 Socket API 实现 HTTP/TCP 通信，并使用 OpenSSL 提供 HTTPS/TLS 支持**（Socket 负责 TCP 层，OpenSSL 负责 TLS 层，两者不是替代关系）
- 调用简单，仅需 `#include <feathercrawl.h>`
- C++11 及以上，支持在 Windows 10/11、Linux 上运行
- 内建 Cookie 管理、重定向、重试、超时、下载进度、字符集自动识别等能力

---

# 平台依赖与编译

FeatherCrawl 是单头文件库，但**依赖系统或第三方库**，编译时需要显式链接：

| 平台 | 编译要求 |
|------|---------|
| Windows / MSVC | 直接编译即可，头文件通过 `#pragma comment(lib, ...)` 自动链接 `winhttp.lib`（启用 WebView2 时还会自动链接 `user32.lib`、`advapi32.lib`） |
| Windows / MinGW / TDM-GCC | 需添加 `-lwinhttp` |
| Linux | 需添加 `-lssl -lcrypto -pthread`（部分发行版若使用 iconv 还需 `-liconv`） |

---

# 版本与兼容性

- 当前版本标记：`FeatherCrawl/2.0`（体现在默认 User-Agent 中）
- 最近更新：2026-09-25
- 语言标准：C++11 及以上
- 支持平台：Windows 10/11、Linux

---

# 开源许可证

FeatherCrawl 使用 [Apache License 2.0](LICENSE) 开源。

---

# 一、快速上手

## 1.1 最小可运行程序

```cpp
#include <feathercrawl.h>
#include <iostream>
using namespace std;

int main()
{
    web::Session session;                                  // 创建会话
    web::Response r = session.get("https://example.com");  // 发送 GET 请求

    if (r.ok())                                            // 判断请求是否成功
    {
        cout << r.body << endl;                            // 打印响应体（爬取到的 HTML 等）
    }
    else
    {
        cerr << r.error_message << endl;                   // 打印失败原因
    }
}
```

> 提醒：Linux 下必须链接 `-lssl -lcrypto -pthread`；Windows / MinGW 必须链接 `-lwinhttp`。否则会出现链接错误。

## 1.2 三个核心对象

| 对象 | 作用 |
|------|------|
| `web::Session` | 管理会话，即 Cookie、代理、连接池等 |
| `web::Headers` | 用户发送的**请求头** |
| `web::Response` | 服务端返回的结果（状态码、响应体、响应头、错误等） |

> 注意：请求头 `Headers` 与响应头 `ResponseHeaders` 是**两个不同类型**。
>
> - `Headers::fields` 是 `std::unordered_map<std::string, std::string>`
> - `ResponseHeaders::fields` 是 `std::unordered_map<std::string, std::vector<std::string>>`
>
> `Session` 相关接口使用 `Headers`，`Response::headers` 是 `ResponseHeaders`，两者用途不同，**不能相互赋值**：

示例：
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

# 二、语言设置

FeatherCrawl 支持手动切换语言，支持中文与英文，默认语言为英文。

## 2.1 `web::set_default_language(lang)`

作用：设置全局默认语言。**该设置只影响之后创建的 Session，不影响已创建的 Session**（Session 在构造时会拷贝一份当前全局语言）。

参数：`lang` —— `"zh_CN"` 或 `"en_US"`（也接受 `"zh"`、`"cn"`、`"chinese"` 等别名）。

示例：

```cpp
web::set_default_language("zh_CN");
web::Session a;               // a 使用 zh_CN

web::set_default_language("en_US");
web::Session b;               // b 使用 en_US，a 仍然是 zh_CN，不受影响
```

## 2.2 `web::set_language(lang)`

作用：与 `web::set_default_language` 等价，设置全局默认语言，只影响之后创建的 Session。

示例：
```cpp
web::set_language("zh_CN");
```

## 2.3 `Session::set_language(lang)`

作用：设置当前 Session 的提示信息语言（包括错误信息以及所有内部提示文本），不影响其他 Session。

参数：`std::string`。

示例：
```cpp
web::Session session;
session.set_language("zh_CN");
```

## 2.4 `Session::language()`

作用：返回当前 Session 使用的语言字符串。

返回值：`std::string`，值为 `"zh_CN"` 或 `"en_US"`。

继承规则：
- Session 在**构造时**拷贝一份全局默认语言，之后不再随全局默认语言变化；
- 若之后调用过 `Session::set_language()`，则以最后一次设置值为准。

示例：
```cpp
web::set_default_language("zh_CN");
web::Session s;                         // s.language() == "zh_CN"

web::set_default_language("en_US");
cout << s.language() << endl;           // 仍为 zh_CN
```

---

# 三、请求头

`web::Headers` 用于保存**请求头**，提供大小写不敏感的键值查询接口。

> `Headers` 与 `ResponseHeaders` 是不同结构，详见 1.2 节。

## 3.1 `set(key, value)`

作用：添加或覆盖一个请求头（键名比较大小写不敏感）。

参数：`key` 与 `value`。`key` 代表请求头的名字，例如 `User-Agent`；`value` 代表请求头的值。

示例：
```cpp
web::Headers headers;
headers.set("Content-Type", "application/json");
headers.set("Authorization", "Bearer 123456");
headers.set("User-Agent", "Crawler/1.0");
```

## 3.2 `get(key)`

作用：读取请求头的值，大小写不敏感。

返回：`std::string`，不存在时返回空串。

示例：
```cpp
string key = headers.get("content-type");
```

## 3.3 `contains(key)`

作用：判断请求头是否存在。

返回：`bool`。

示例：
```cpp
if (headers.contains("Authorization")) { /* ... */ }
```

## 3.4 `erase(key)`

作用：删除指定请求头。

示例：
```cpp
headers.erase("Cookie");
```

## 3.5 `clear()`

作用：清除所有请求头。

示例：
```cpp
headers.clear();
```

## 3.6 `to_winhttp_string()`

作用：将请求头转换为 `"Key: Value\r\n"` 拼接的字符串形式。

返回：`std::string`。

示例：
```cpp
string text = headers.to_winhttp_string();
```

---

# 四、响应对象

`web::Response` 表示服务器返回的数据。

```cpp
web::Response r = session.get(url);
```

返回的就是一个 `Response` 对象，包含：

- HTTP 状态码
- 响应正文
- 响应头（`ResponseHeaders`）
- 错误码与错误信息
- 请求统计信息

## 4.1 `ok()`

作用：判断请求是否成功。

返回：`bool`。

示例：
```cpp
web::Response r = session.get("https://example.com");
if (r.ok())
{
    cout << "请求成功";
}
else
{
    cout << "请求失败";
}
```

## 4.2 `status_code`

作用：获取 HTTP 状态码。

返回：`int`。

常见值：

| 状态码 | 含义 |
|:-:|:---:|
| 200 | 成功 |
| 301 | 永久重定向 |
| 302 | 临时重定向 |
| 400 | 请求错误 |
| 403 | 禁止访问 |
| 404 | 不存在 |
| 500 | 服务器错误 |

示例：
```cpp
web::Response r = session.get("https://example.com");
cout << r.status_code;
```

## 4.3 `body`

作用：返回服务器保存的正文内容（例如网页源代码）。

类型：`std::string`。

示例：
```cpp
web::Response r = session.get("https://example.com");
cout << r.body;
```

## 4.4 `headers`

作用：获取服务器返回的响应头。

类型：`ResponseHeaders`。

示例：
```cpp
web::Response r = session.get("https://example.com");
string type = r.headers.get("Content-Type");
```

`ResponseHeaders` 常用接口：

| 方法 | 作用 |
|------|------|
| `get(key)` | 返回第一个匹配的值 |
| `get_all(key)` | 返回同名头的全部值 |
| `contains(key)` | 是否包含某个响应头 |
| `clear()` | 清空 |

## 4.5 `error_code`

作用：获取错误类型。

示例：
```cpp
if (!r.ok())
{
    auto erc = r.error_code;
}
```

## 4.6 `error_message`

作用：获取错误描述。

类型：`std::string`。

示例：
```cpp
if (!r.ok())
{
    cerr << r.error_message;
}
```

## 4.7 `final_url`

作用：获取最终访问地址，适用于 HTTP 重定向或会自动跳转的网页。

类型：`std::wstring`。

示例：
```cpp
web::Response r = session.get("https://example.com");
std::wcout << r.final_url << std::endl;
```

## 4.8 `received_bytes`

作用：获取收到的数据大小。

类型：`size_t`。

示例：
```cpp
cout << r.received_bytes << " bytes";
```

## 4.9 `attempts`

作用：获取实际请求次数。

类型：`int`。

示例：
```cpp
cout << "请求次数:" << r.attempts;
```

## 4.10 `redirect_count`

作用：获取重定向次数。

类型：`int`。

示例：
```cpp
cout << r.redirect_count;
```

---

# 五、网络会话

## 5.1 `Session()`

作用：创建一个默认网络会话。

示例：
```cpp
web::Session session;
```

## 5.2 `Session(options)`

作用：使用配置创建 Session。

参数：`SessionOptions`。

示例：
```cpp
web::SessionOptions opt;
opt.user_agent = L"MyCrawler/1.0";
web::Session session(opt);
```

## 5.3 `get(url)`

作用：发送 GET 请求。

参数：

| 参数 | 类型 | 说明 |
|:-:|:----:|:--:|
| url | `std::string` 或 `std::wstring` | 请求地址 |

返回：`Response`。

示例：
```cpp
web::Session session;
web::Response r = session.get("https://example.com");
```

## 5.4 `get(url, headers)`

作用：带请求头发送 GET。

示例：
```cpp
web::Session session;
web::Headers h;
h.set("User-Agent", "MyBot");
web::Response r = session.get("https://example.com", h);
```

## 5.5 `get(url, headers, options)`

作用：完整 GET 请求。

参数：

| 参数 | 类型 |
| ------- | ---------------- |
| url | `std::string` 或 `std::wstring` |
| headers | `Headers` |
| options | `RequestOptions` |

示例：
```cpp
web::Session session;
web::RequestOptions opt;
opt.timeout_ms = 5000;
web::Response r = session.get("https://example.com", web::Headers(), opt);
```

## 5.6 `post(url, body, content_type, headers, options)`

作用：发送 POST 请求。

参数：

| 参数 | 说明 |
| ------------ | ----- |
| url | 请求地址 |
| body | 发送的数据 |
| content_type | 数据类型（默认 `application/x-www-form-urlencoded`） |
| headers | 请求头（可选） |
| options | `RequestOptions`（可选） |

返回：`Response`。

`post()` 提供多个重载，可以像 `get()` 一样带请求头和 `RequestOptions`。

示例：
```cpp
web::Session session;

// 简单形式
web::Response r1 = session.post("https://example.com/api", R"({"id":1})", "application/json");

// 完整形式
web::Headers h;
h.set("Authorization", "Bearer 123456");
web::RequestOptions opt;
opt.timeout_ms = 8000;
web::Response r2 = session.post("https://example.com/api", R"({"id":1})", "application/json", h, opt);
```

## 5.7 `cookie_jar()`

作用：获取当前会话的 Cookie 管理器。

返回：`CookieJar&`。

示例：
```cpp
web::Session s;
auto& jar = s.cookie_jar();
cout << jar.size();
```

## 5.8 `set_language(lang)`

作用：设置当前会话提示信息语言。

参数：`std::string`。

示例：
```cpp
web::Session s;
s.set_language("zh_CN");
```

## 5.9 `language()`

作用：获取当前语言（构造时拷贝的全局默认语言，或之后显式设置的值）。

返回：`std::string`。

示例：
```cpp
web::Session s;
cout << s.language();
```

## 5.10 `Session::clear_cookies()`

作用：清除当前 Session 保存的 Cookie。

示例：
```cpp
session.clear_cookies();
```

---

# 六、请求配置

`web::RequestOptions` 用于控制**一次 HTTP 请求**的行为。

可以设置：

- 超时时间
- 自动重定向
- 单次请求最大响应大小
- 重试策略

示例：
```cpp
web::RequestOptions options;
options.timeout_ms = 5000;
web::Response r = session.get("https://example.com", web::Headers(), options);
```

## 6.1 `timeout_ms`

作用：设置请求超时时间。

单位：毫秒。默认 `0`。

类型：`int`。

示例：
```cpp
web::RequestOptions opt;
opt.timeout_ms = 10000;
```

## 6.2 `follow_redirect`

作用：是否自动跟随 HTTP 重定向。

类型：`bool`，默认 `true`。

示例：
```cpp
web::RequestOptions opt;
opt.follow_redirect = true;
```

## 6.3 `max_redirects`

作用：设置最大重定向次数。

类型：`int`，默认 `-1`。

示例：
```cpp
web::RequestOptions opt;
opt.max_redirects = 10;
```

## 6.4 `max_response_size`

作用：**单次请求**限制服务器返回数据大小（仅内存读取路径）。

单位：字节。默认 `0`。

类型：`size_t`。当值为 `0` 时，使用 `SessionOptions::max_response_size`（默认 64 MiB）。

示例：
```cpp
web::RequestOptions opt;
opt.max_response_size = 1024 * 1024; // 最大 1 MiB
```

## 6.5 `retry`

作用：设置自动重试策略。

类型：`RetryPolicy`。

示例：

```cpp
web::RequestOptions opt;
opt.retry.retries = 3;
```

---

# 七、自动重试

`web::RetryPolicy` 用于网络不稳定环境。

例如：

- 网络波动
- 临时服务器错误
- 超时

`RetryPolicy` 字段与默认值如下：

| 字段 | 类型 | 默认值 | 作用 |
|------|------|-------|------|
| `retries` | `int` | `0` | 最大重试次数 |
| `initial_delay_ms` | `int` | `250` | 首次重试等待时间（毫秒） |
| `max_delay_ms` | `int` | `4000` | 单次等待时间上限（毫秒） |
| `multiplier` | `double` | `2.0` | 等待时间指数增长倍数 |
| `jitter` | `bool` | `true` | 是否加入随机抖动 |
| `respect_retry_after` | `bool` | `true` | 是否尊重 `Retry-After` 响应头 |
| `retry_non_idempotent` | `bool` | `false` | 是否允许非幂等方法（如 POST）重试 |
| `allow_automatic_authentication` | `bool` | `false` | 是否允许 WinHTTP 自动认证（**仅 Windows/WinHTTP 有效**，Linux 端字段存在但不使用） |

## 7.1 `retries`

作用：设置最大重试次数。

类型：`int`。

示例：

```cpp
web::RequestOptions opt;
opt.retry.retries = 5;
```

## 7.2 `initial_delay_ms`

作用：第一次重试等待时间。

单位：毫秒。

示例：

```cpp
opt.retry.initial_delay_ms = 500;
```

## 7.3 `max_delay_ms`

作用：限制最大等待时间。

示例：

```cpp
opt.retry.max_delay_ms = 5000;
```

## 7.4 `multiplier`

作用：设置等待时间增长倍数。

类型：`double`。

示例：

```cpp
opt.retry.multiplier = 2.0;
```

---

# 八、Cookie 管理

`web::CookieJar` 用于管理 Session 中保存的 Cookie。

Cookie 保存在**内存**中，仅在 Session 生命周期内有效，**不会写入磁盘**。

通常由以下方式获得：

```cpp
Session::cookie_jar()
```

> **线程安全提示**：
> `CookieJar` 内部虽然对自身操作加了互斥锁，但**整个 `Session` 并不适合多线程并发使用**。
> 建议同一个 `Session` 只在一个线程中操作，或给每个线程分配独立的 `Session`。详见 5.12 节。

## 8.1 `size()`

作用：返回 Cookie 数量。

返回：`size_t`。

示例：

```cpp
web::Session session;
auto& jar = session.cookie_jar();
auto count = jar.size();
cout << count;
```

## 8.2 `clear()`

作用：清空所有 Cookie。

示例：

```cpp
session.cookie_jar().clear();
```

或直接使用 Session 方法：

```cpp
session.clear_cookies();
```

## 8.3 `delete_cookie(name)`

作用：删除指定 Cookie。

参数 `name` 类型：`std::string`。

示例：

```cpp
session.cookie_jar().delete_cookie("sessionid");
```

或：

```cpp
session.delete_cookie("sessionid");
```

## 8.4 Cookie 自动保存

Session 默认会保存服务器返回的 Cookie。

示例：

```cpp
web::Session session;
session.post("https://example.com/login", "user=test", "application/x-www-form-urlencoded");
web::Response r = session.get("https://example.com/user");
```

---

# 九、会话配置

`web::SessionOptions` 用于创建 Session 时设置默认行为。

示例：

```cpp
web::SessionOptions opt;
web::Session session(opt);
```

`SessionOptions` 字段与默认值如下：

| 字段 | 类型 | 默认值 | 作用 |
|------|------|-------|------|
| `user_agent` | `std::wstring` | `L"FeatherCrawl/2.0"` | 默认 User-Agent |
| `enable_cookies` | `bool` | `true` | 是否启用 Cookie |
| `access_type` | `DWORD` | `WINHTTP_ACCESS_TYPE_DEFAULT_PROXY` | WinHTTP 访问类型（**仅 Windows**） |
| `proxy` | `std::wstring` | 空 | 代理服务器 |
| `proxy_bypass` | `std::wstring` | 空 | 代理绕过列表 |
| `max_response_size` | `size_t` | 64 MiB | 默认最大响应大小 |
| `max_redirects` | `int` | `10` | 默认最大重定向次数 |
| `max_connections` | `size_t` | `64` | 连接池上限（**Windows**；Linux 端未使用） |

## 9.1 `user_agent`

作用：设置默认 User-Agent。

类型：`std::wstring`。

示例：

```cpp
web::SessionOptions opt;
opt.user_agent = L"MyCrawler/1.0";
web::Session s(opt);
```

## 9.2 `enable_cookies`

作用：是否启用 Cookie。

类型：`bool`。

示例：

```cpp
web::SessionOptions opt;
opt.enable_cookies = true;
```

## 9.3 `proxy`

作用：设置代理服务器。

类型：`std::wstring`。

示例：

```cpp
web::SessionOptions opt;
opt.proxy = L"http://127.0.0.1:7890";
web::Session s(opt);
```

## 9.4 `max_response_size`

作用：设置会话默认最大响应大小。

类型：`size_t`。默认 64 MiB（`64ULL * 1024ULL * 1024ULL`）。

示例：

```cpp
web::SessionOptions opt;
opt.max_response_size = 64 * 1024 * 1024; // 64 MiB
```

---

# 十、文件下载系统

FeatherCrawl 提供专用下载接口，用于：

- 下载普通文件
- 下载大文件
- 显示下载进度
- 获取下载统计信息

核心对象：

| 对象 | 作用 |
| --------------------- | ---- |
| `DownloadOptions` | 下载配置 |
| `DownloadResult` | 下载结果 |
| `download()` | 执行下载 |

> **下载文件大小限制使用 `DownloadOptions::max_file_size`**。
> `DownloadOptions::request.max_response_size` 在下载路径中**不会**被用于限制文件大小，请勿混淆（交叉参见 6.4 节）。

## 10.1 `download(url, filename, headers, options)`

作用：下载文件到本地。

参数：

| 参数 | 类型 | 说明 |
| -------- | ----------------- | ---- |
| url | `std::string` 或 `std::wstring` | 文件地址 |
| filename | `std::string` 或 `std::wstring` | 保存路径 |
| headers | `Headers` | 请求头（可选） |
| options | `DownloadOptions` | 下载配置（可选） |

返回：`DownloadResult`。

示例：

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
        cout << "下载完成";
    }
    else
    {
        cerr << r.error_message << endl;
    }
}
```

如果不需要自定义请求头，可以只传前两个参数：

```cpp
web::DownloadResult r = session.download("https://example.com/test.zip", "test.zip");
```

---

# 十一、下载配置

`web::DownloadOptions` 用于控制下载行为。

## 11.1 `show_progress`

作用：是否显示下载进度。

类型：`bool`，默认 `true`。

示例：

```cpp
web::DownloadOptions opt;
opt.show_progress = true;
```

## 11.2 `request.timeout_ms`

作用：设置下载超时时间。

单位：毫秒。

类型：`int`。

示例：

```cpp
web::DownloadOptions opt;
opt.request.timeout_ms = 30000;
```

## 11.3 `overwrite`

作用：是否覆盖已有文件。

类型：`bool`，默认 `false`。

示例：

```cpp
web::DownloadOptions opt;
opt.overwrite = true;
```

## 11.4 `request.retry`

作用：设置下载失败重试。

类型：`RetryPolicy`。

示例：

```cpp
web::DownloadOptions opt;
opt.request.retry.retries = 5;
```

## 11.5 其他下载配置

| 字段 | 类型 | 默认值 | 作用 |
|------|------|-------|------|
| `max_file_size` | `uint64_t` | 1 GiB（`1024ULL * 1024ULL * 1024ULL`） | 允许下载的最大文件大小（**下载大小限制请使用此字段**） |
| `progress_bar_width` | `size_t` | `20` | 进度条宽度 |
| `progress_refresh_ms` | `unsigned int` | `100` | 进度刷新间隔（毫秒） |

---

# 十二、下载结果

`web::DownloadResult` 表示下载完成后的结果。

## 12.1 `ok()`

作用：判断下载是否成功。

返回：`bool`。

示例：

```cpp
if (result.ok())
{
    cout << "成功";
}
```

## 12.2 `bytes_written`

作用：实际写入文件大小。

类型：`uint64_t`。

示例：

```cpp
cout << result.bytes_written;
```

## 12.3 `file_size`

作用：文件总大小。

类型：`uint64_t`。

示例：

```cpp
cout << result.file_size;
```

## 12.4 `destination`

作用：保存文件路径。

类型：`std::wstring`。

示例：

```cpp
std::wcout << result.destination << std::endl;
```

## 12.5 `error_message`

作用：获取下载失败原因。

类型：`std::string`。

示例：

```cpp
if (!result.ok())
{
    cerr << result.error_message;
}
```

## 12.6 其他下载结果字段

| 字段 | 类型 | 作用 |
|------|------|------|
| `status_code` | `int` | HTTP 状态码 |
| `headers` | `ResponseHeaders` | 响应头 |
| `error_code` | `ErrorCode` | 错误码 |
| `native_error_code` | `DWORD` | 底层错误码（Linux 端 `DWORD` 是 `unsigned long` 的别名，其值可能来自 `errno`、`getaddrinfo` 返回值或 OpenSSL 错误码） |
| `file_size_known` | `bool` | 文件总大小是否已知 |
| `attempts` | `int` | 实际请求次数 |
| `redirect_count` | `int` | 重定向次数 |
| `final_url` | `std::wstring` | 最终下载地址 |

---

# 十三、工具函数

FeatherCrawl 还提供了一些便捷工具函数。

## 13.1 `web::to_utf8(src, codepage)`

作用：将指定代码页的字符串转换为 UTF-8。

示例：

```cpp
std::string utf8 = web::to_utf8(gbk_text, 936);
```

## 13.2 `web::to_utf8(src, encoding)`

作用：将指定编码名的字符串转换为 UTF-8。

示例：

```cpp
std::string utf8 = web::to_utf8(gbk_text, "GBK");
```

## 13.3 `web::text_size(bytes, unit)`

作用：将字节数格式化为可读字符串。

示例：

```cpp
cout << web::text_size(1024 * 1024); // 1.00 MB
```

## 13.4 `web::substring(str, start, end)`

作用：按索引截取字符串（包含 `start` 和 `end`）。

示例：

```cpp
std::string part = web::substring("Hello World", 0, 4); // "Hello"
```

## 13.5 `web::lines(str, start_line, end_line)`

作用：按行截取字符串，行号从 1 开始。

示例：

```cpp
std::string part = web::lines("a\nb\nc", 2, 3); // "b\nc"
```

---

# 十四、浏览器与 HTML 渲染（仅 Windows）

在 Windows 平台上，FeatherCrawl 可选支持 Microsoft Edge WebView2，用于打开网页或渲染 HTML。

> **平台限制**：`browse()` 与 `render_html()` 只在 **Windows** 平台上真正实现。

> **阻塞语义**：`browse()` 与 `render_html()` 在 Windows 端会创建窗口并进入消息循环，**阻塞当前线程直到窗口关闭才返回**。如需并发，请放到独立线程中调用。

## 14.1 `webview2_available()`

作用：检查当前环境是否可用 WebView2。

返回：`bool`。

示例：

```cpp
if (web::webview2_available())
{
    cout << "WebView2 可用";
}
```

## 14.2 `webview2_runtime_version()`

作用：获取 WebView2 运行时版本。

返回：`std::wstring`。

示例：

```cpp
std::wstring version = web::webview2_runtime_version();
std::wcout << version << std::endl;
```

## 14.3 `browse(url, options)`

作用：打开一个窗口浏览指定 URL。**阻塞到窗口关闭后返回。**

示例：

```cpp
web::BrowserOptions opt;
opt.title = L"我的浏览器";
opt.width = 1200;
opt.height = 800;

web::BrowserResult r = web::browse("https://example.com", opt);
if (!r.ok())
{
    cerr << r.error_message << endl;
}
```

## 14.4 `render_html(html, options)`

作用：渲染一段 HTML 字符串。**阻塞到窗口关闭后返回。**

大小限制：约 2 MiB。

示例：

```cpp
web::BrowserResult r = web::render_html("<h1>Hello</h1>");
if (!r.ok())
{
    cerr << r.error_message << endl;
}
```

## 14.5 `BrowserOptions`

| 字段 | 类型 | 默认值 | 作用 |
|------|------|--------|------|
| `title` | `std::wstring` | `L"FeatherCrawl"` | 窗口标题 |
| `width` | `int` | `1100` | 窗口宽度 |
| `height` | `int` | `760` | 窗口高度 |
| `resizable` | `bool` | `true` | 是否可调整大小 |
| `script_enabled` | `bool` | `true` | 是否启用脚本 |
| `devtools_enabled` | `bool` | `true` | 是否启用开发者工具 |
| `context_menus_enabled` | `bool` | `true` | 是否启用右键菜单 |
| `status_bar_enabled` | `bool` | `true` | 是否启用状态栏 |
| `default_script_dialogs_enabled` | `bool` | `true` | 是否启用默认脚本对话框 |
| `user_data_folder` | `std::wstring` | 空 | 用户数据目录。**为空时库会创建临时目录，并在窗口关闭后自动清理** |
| `browser_executable_folder` | `std::wstring` | 空 | 浏览器可执行文件目录 |

## 14.6 `BrowserResult`

| 字段 | 类型 | 作用 |
|------|------|------|
| `error_code` | `BrowserErrorCode` | 错误码 |
| `native_error_code` | Windows 下 `HRESULT`；Linux 下 `long` | 底层错误码 |
| `error_message` | `std::string` | 错误描述 |
| `exit_code` | `int` | 窗口退出码 |
| `runtime_version` | `std::wstring` | WebView2 运行时版本 |
| `ok()` | `bool` | 是否成功 |

---

# 十五、常见问题（FAQ）

**Q1：Linux 编译失败，提示找不到 OpenSSL 符号？**

A：在编译参数中加入 `-lssl -lcrypto -pthread`。使用 CMake 时通过 `find_package(OpenSSL REQUIRED)` 并链接 `OpenSSL::SSL` 和 `OpenSSL::Crypto`。

**Q2：Windows / MinGW 编译失败，提示 `WinHttpOpen` 等未定义？**

A：在编译参数中加入 `-lwinhttp`。MSVC 下因为 `feathercrawl.h` 使用了 `#pragma comment(lib, "winhttp.lib")`，无需手动链接。

**Q3：下载文件失败，提示“目标文件已存在”？**

A：`DownloadOptions::overwrite` 默认为 `false`。请设置：

```cpp
web::DownloadOptions opt;
opt.overwrite = true;
```

**Q4：HTTPS 请求返回 `TlsFailure`？**

A：可能原因包括：系统时间不正确、系统 CA 证书不完整、目标服务器证书过期。请先校准系统时间并检查系统 CA。当前版本不提供跳过证书校验的开关。

**Q5：`final_url` 与 `destination` 无法用 `cout` 输出？**

A：这两个字段的类型是 `std::wstring`，请使用 `std::wcout`：

```cpp
std::wcout << r.final_url << std::endl;
std::wcout << result.destination << std::endl;
```

**Q6：`Session::cookies()` 无法调用？**

A：当前接口是 `Session::cookie_jar()`，请使用：

```cpp
auto& jar = session.cookie_jar();
```

**Q7：`DownloadOptions::timeout_ms` 无法识别？**

A：`timeout_ms` 位于 `DownloadOptions::request` 子对象中：

```cpp
opt.request.timeout_ms = 30000;
```

**Q8：`browse()` / `render_html()` 在 Linux 上返回 `UnsupportedPlatform`？**

A：WebView2 渲染功能**仅支持 Windows**。Linux 上这两个函数只返回错误码，不会打开窗口。

**Q9：修改了全局默认语言，为什么已有 Session 的语言没变？**

A：Session 在构造时拷贝当时全局默认语言。要改变已有 Session 的语言，请调用 `Session::set_language()`。

**Q10：macOS 上能用吗？**

A：不能。macOS 不是“未支持”，而是**包含头文件会直接 `#error` 编译失败**。

**Q11：`browse()` / `render_html()` 会卡住主线程吗？**

A：会。这两个函数在 Windows 端会进入消息循环，**阻塞当前线程直到窗口关闭**。如需并发请放到独立线程中调用。

**Q12：同一个 `Session` 可以在多个线程中同时使用吗？**

A：不建议。`Session` 不是为多线程并发设计的，同时发起多个请求或同时修改状态属于未定义行为。请为每个线程分配独立的 `Session`。

---

<h3 align="center">FeatherCrawl —— 简洁、高效的 C++ 网络请求库。</h3>
