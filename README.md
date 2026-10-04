<h1 align="center">FeatherCrawl</h1>

<p align="center">
  <a href="README.md">简体中文</a> |
  <a href="README_EN.md">English</a>
</p>

<p align="center">
  <a href="https://github.com/CompilerIDE/FeatherCrawl/blob/main/LICENSE"><img src="https://img.shields.io/github/license/CompilerIDE/FeatherCrawl.svg?style=for-the-badge&new=1" alt="License"></a>
  <br>
</p>

FeatherCrawl 是一款网络请求库。仅需一个头文件即可在 C++ 中进行网页抓取。

---

# 特性

- 架构：Windows 平台使用 WinHTTP（可选 Microsoft Edge WebView2 渲染），Linux 平台使用 Socket API + OpenSSL
- 调用简单，仅需 `#include <feathercrawl.h>`
- C++11 及以上，支持在 Windows 10/11、Linux 上运行
- 内建 Cookie 管理、重定向、重试、超时、下载进度、字符集自动识别等能力

---

# 开源许可证

FeatherCrawl 使用 [Apache License 2.0](LICENSE) 开源。

---

# 使用示例

## 一、快速上手

### 1.1 最小可运行程序

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

**编译命令**

| 平台 | 命令 |
|------|------|
| Windows / MSVC | 直接编译，自动链接 `winhttp.lib`（启用 WebView2 时还会链接 `user32.lib`、`advapi32.lib`） |
| Windows / MinGW / TDM-GCC | 需添加 `-lwinhttp` 编译参数 |
| Linux | 需添加 `-lssl -lcrypto -pthread`（部分发行版还需 `-liconv`） |

### 1.2 三个核心对象

| 对象 | 作用 |
|------|------|
| `web::Session` | 管理会话，即 Cookie、代理、连接池等 |
| `web::Headers` | 用户发送的请求头 |
| `web::Response` | 服务端返回的结果（状态码、响应体、响应头、错误等） |

---

## 二、语言设置

FeatherCrawl 支持手动切换语言，支持中文与英文，默认语言为英文。

### 2.1 `web::set_default_language(lang)`

作用：设置全局默认语言。

参数：`lang` —— `"zh_CN"` 或 `"en_US"`（也接受 `"zh"`、`"cn"`、`"chinese"` 等别名）。

示例：

```cpp
web::set_default_language("zh_CN");
web::set_default_language("en_US");
```

### 2.2 `web::set_language(lang)`

作用：与 `web::set_default_language` 等价，设置全局默认语言。

```cpp
web::set_language("zh_CN");
```

### 2.3 `Session::set_language(lang)`

作用：仅设置当前会话语言，不影响其他会话。

```cpp
web::Session session;
session.set_language("zh_CN");
```

### 2.4 `Session::language()`

作用：返回当前 Session 使用的语言字符串。

返回：`std::string` 类型，值为 `"zh_CN"` 或 `"en_US"`。

```cpp
web::Session s;
cout << s.language() << endl;
```

---

## 三、请求头

`web::Headers` 表示你要发送的请求头。内部以 `unordered_map<std::string, std::string>` 存储，键名大小写不敏感。

### 3.1 `set(key, value)`

作用：添加或覆盖一个请求头（键名比较大小写不敏感）。

参数：`key` 与 `value`。`key` 代表请求头的名字，例如 `User-Agent`；`value` 代表请求头的值。

```cpp
web::Headers headers;
headers.set("Content-Type", "application/json");
headers.set("Authorization", "Bearer 123456");
headers.set("User-Agent", "Crawler/1.0");
```

### 3.2 `get(key)`

作用：读取请求头的值，大小写不敏感。

返回：`std::string`，不存在时返回空串。

```cpp
string key = headers.get("content-type");
```

### 3.3 `contains(key)`

作用：判断请求头是否存在。

返回：`bool`。

```cpp
if (headers.contains("Authorization")) { /* ... */ }
```

### 3.4 `erase(key)`

作用：删除指定请求头。

```cpp
headers.erase("Cookie");
```

### 3.5 `clear()`

作用：清除所有请求头。

```cpp
headers.clear();
```

### 3.6 `to_winhttp_string()`

作用：将请求头转换为 `"Key: Value\r\n"` 拼接的字符串形式（Windows 端提交给 WinHTTP，Linux 端用于构造请求报文）。

返回：`std::string`。

```cpp
string text = headers.to_winhttp_string();
```

### 3.7 `fields`

类型：`std::unordered_map<std::string, std::string>`

作用：直接访问底层字段，可用于遍历。

```cpp
for (auto& kv : headers.fields)
{
    cout << kv.first << ": " << kv.second << "\n";
}
```

---

## 四、响应对象

`web::Response` 表示服务器返回的数据。

```cpp
web::Response r = session.get(url);
```

返回的就是一个 `Response` 对象，包含：

- HTTP 状态码
- 响应正文
- 响应头
- 错误码与错误信息
- 请求统计信息

### 4.1 `ok()`

作用：判断请求是否成功。

返回：`bool`（`true` 表示请求成功且状态码在 `200`–`299` 之间）。

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

### 4.2 `status_code`

作用：获取 HTTP 状态码。

返回：`int` 类型。

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

```cpp
web::Response r = session.get("https://example.com");
cout << r.status_code;
```

### 4.3 `body`

作用：返回服务器保存的正文内容（例如网页源代码）。

类型：`std::string`。

```cpp
web::Response r = session.get("https://example.com");
cout << r.body;
```

### 4.4 `headers`

作用：获取服务器返回的响应头。

类型：`ResponseHeaders`。

```cpp
web::Response r = session.get("https://example.com");
string type = r.headers.get("Content-Type");
```

### 4.5 `error_code`

作用：获取错误类型。

类型：`ErrorCode` 枚举。

```cpp
if (!r.ok())
{
    auto erc = r.error_code;
}
```

### 4.6 `error_message`

作用：获取错误描述。

类型：`std::string`。

```cpp
if (!r.ok())
{
    cerr << r.error_message;
}
```

### 4.7 `final_url`

作用：获取最终访问地址，适用于 HTTP 重定向或会自动跳转的网页。

类型：`std::wstring`。

```cpp
web::Response r = session.get("https://example.com");
std::wcout << r.final_url << std::endl;
```

### 4.8 `received_bytes`

作用：获取收到的数据大小。

类型：`size_t`。

```cpp
cout << r.received_bytes << " bytes";
```

### 4.9 `attempts`

作用：获取实际请求次数。

类型：`int`。

```cpp
cout << "请求次数:" << r.attempts;
```

### 4.10 `redirect_count`

作用：获取重定向次数。

类型：`int`。

```cpp
cout << r.redirect_count;
```

---

## 五、网络会话

### 5.1 `Session()`

作用：创建一个默认网络会话。

```cpp
web::Session session;
```

### 5.2 `Session(options)`

作用：使用配置创建 Session。

参数：`SessionOptions`。

```cpp
web::SessionOptions opt;
opt.user_agent = L"MyCrawler/1.0";
web::Session session(opt);
```

### 5.3 `get(url)`

作用：发送 GET 请求。

参数：

| 参数 | 类型 | 说明 |
|:-:|:----:|:--:|
| url | `std::string` 或 `std::wstring` | 请求地址 |

返回：`Response`。

```cpp
web::Session session;
web::Response r = session.get("https://example.com");
```

### 5.4 `get(url, headers)`

作用：带请求头发送 GET。

```cpp
web::Session session;
web::Headers h;
h.set("User-Agent", "MyBot");
web::Response r = session.get("https://example.com", h);
```

### 5.5 `get(url, headers, options)`

作用：完整 GET 请求。

参数：

| 参数 | 类型 |
| ------- | ---------------- |
| url | `std::string` 或 `std::wstring` |
| headers | `Headers` |
| options | `RequestOptions` |

```cpp
web::Session session;
web::RequestOptions opt;
opt.timeout_ms = 5000;
web::Response r = session.get("https://example.com", web::Headers(), opt);
```

### 5.6 `post(url, body, content_type)`

作用：发送 POST 请求。

参数：

| 参数 | 说明 |
| ------------ | ----- |
| url | 请求地址 |
| body | 发送的数据 |
| content_type | 数据类型 |

返回：`Response`。

```cpp
web::Session session;
web::Response r = session.post(
    "https://example.com/api",
    R"({"id":1})",
    "application/json"
);
```

### 5.7 `cookie_jar()`

作用：获取当前会话的 Cookie 管理器。

返回：`CookieJar&`。

```cpp
web::Session s;
auto& jar = s.cookie_jar();
cout << jar.size();
```

### 5.8 `set_language(lang)`

作用：设置当前会话错误语言。

参数：`std::string`。

```cpp
web::Session s;
s.set_language("zh_CN");
```

### 5.9 `language()`

作用：获取当前语言。

返回：`std::string`。

```cpp
web::Session s;
cout << s.language();
```

### 5.10 `Session::clear_cookies()`

作用：清除当前 Session 保存的 Cookie。

```cpp
session.clear_cookies();
```

### 5.11 其他会话方法

| 方法 | 作用 |
|------|------|
| `bool valid()` | 检查会话是否可用 |
| `std::string error_message()` | 获取会话初始化错误信息 |
| `bool delete_cookie(name, domain, path)` | 删除指定 Cookie |
| `size_t clear_cookies_for_domain(domain)` | 清除某个域下的全部 Cookie |
| `size_t cookie_count()` | 返回 Cookie 数量 |
| `void set_cookies_enabled(bool enabled, bool clear_when_disabled = true)` | 启用或禁用 Cookie |
| `bool cookies_enabled() const` | 返回是否启用 Cookie |
| `size_t connection_count()` | 返回连接池中的连接数（Linux 端始终为 0） |
| `void set_proxy(proxy, bypass = "")` | 设置代理 |

---

## 六、请求配置

`web::RequestOptions` 用于控制一次 HTTP 请求的行为。

可以设置：

- 超时时间
- 自动重定向
- 最大响应大小
- 重试策略

示例：

```cpp
web::RequestOptions options;
options.timeout_ms = 5000;
web::Response r = session.get("https://example.com", web::Headers(), options);
```

### 6.1 `timeout_ms`

作用：设置请求超时时间。

单位：毫秒。

类型：`int`。

```cpp
web::RequestOptions opt;
opt.timeout_ms = 10000;
```

### 6.2 `follow_redirect`

作用：是否自动跟随 HTTP 重定向。

类型：`bool`。

```cpp
web::RequestOptions opt;
opt.follow_redirect = true;
```

### 6.3 `max_redirects`

作用：设置最大重定向次数。

类型：`int`。

```cpp
web::RequestOptions opt;
opt.max_redirects = 10;
```

### 6.4 `max_response_size`

作用：限制服务器返回数据大小。

单位：字节。

类型：`size_t`。

```cpp
web::RequestOptions opt;
opt.max_response_size = 1024 * 1024; // 最大 1MB
```

超过限制时，请求失败。

### 6.5 `retry`

作用：设置自动重试策略。

类型：`RetryPolicy`。

```cpp
web::RequestOptions opt;
opt.retry.retries = 3;
```

---

## 七、自动重试

`web::RetryPolicy` 用于网络不稳定环境。

例如：

- 网络波动
- 临时服务器错误
- 超时

### 7.1 `retries`

作用：设置最大重试次数。

类型：`int`。

```cpp
web::RequestOptions opt;
opt.retry.retries = 5;
```

### 7.2 `initial_delay_ms`

作用：第一次重试等待时间。

单位：毫秒。

```cpp
opt.retry.initial_delay_ms = 500;
```

### 7.3 `max_delay_ms`

作用：限制最大等待时间。

```cpp
opt.retry.max_delay_ms = 5000;
```

### 7.4 `multiplier`

作用：设置等待时间增长倍数。

类型：`double`。

```cpp
opt.retry.multiplier = 2.0;
```

### 7.5 其他重试字段

| 字段 | 类型 | 作用 |
|------|------|------|
| `jitter` | `bool` | 是否加入随机抖动 |
| `respect_retry_after` | `bool` | 是否尊重 `Retry-After` 响应头 |
| `retry_non_idempotent` | `bool` | 是否允许非幂等方法重试 |
| `allow_automatic_authentication` | `bool` | 是否允许 WinHTTP 自动认证 |

---

## 八、Cookie 管理

`web::CookieJar` 用于管理 Session 中保存的 Cookie。

通常由以下方式获得：

```cpp
Session::cookie_jar()
```

### 8.1 `size()`

作用：返回 Cookie 数量。

返回：`size_t`。

```cpp
web::Session session;
auto& jar = session.cookie_jar();
auto count = jar.size();
cout << count;
```

### 8.2 `clear()`

作用：清空所有 Cookie。

```cpp
session.cookie_jar().clear();
```

或直接使用 Session 方法：

```cpp
session.clear_cookies();
```

### 8.3 `delete_cookie(name)`

作用：删除指定 Cookie。

参数 `name` 类型：`std::string`。

```cpp
session.cookie_jar().delete_cookie("sessionid");
```

或：

```cpp
session.delete_cookie("sessionid");
```

### 8.4 Cookie 自动保存

Session 默认会保存服务器返回的 Cookie。

```cpp
web::Session session;
// 登录
session.post(
    "https://example.com/login",
    "user=test",
    "application/x-www-form-urlencoded"
);
// 后续请求自动携带 Cookie
web::Response r = session.get("https://example.com/user");
```

---

## 九、会话配置

`web::SessionOptions` 用于创建 Session 时设置默认行为。

```cpp
web::SessionOptions opt;
web::Session session(opt);
```

### 9.1 `user_agent`

作用：设置默认 User-Agent。

类型：`std::wstring`。

```cpp
web::SessionOptions opt;
opt.user_agent = L"MyCrawler/1.0";
web::Session s(opt);
```

### 9.2 `enable_cookies`

作用：是否启用 Cookie。

类型：`bool`。

```cpp
web::SessionOptions opt;
opt.enable_cookies = true;
```

### 9.3 `proxy`

作用：设置代理服务器。

类型：`std::wstring`。

```cpp
web::SessionOptions opt;
opt.proxy = L"http://127.0.0.1:7890";
web::Session s(opt);
```

### 9.4 `max_response_size`

作用：设置默认最大响应大小。

类型：`size_t`。

```cpp
web::SessionOptions opt;
opt.max_response_size = 64 * 1024 * 1024; // 最大 64MB
```

### 9.5 其他会话配置

| 字段 | 类型 | 作用 |
|------|------|------|
| `proxy_bypass` | `std::wstring` | 代理绕过列表 |
| `max_redirects` | `int` | 默认最大重定向次数 |
| `max_connections` | `size_t` | 最大连接池大小（Windows 端） |
| `access_type` | `DWORD` | WinHTTP 访问类型（Windows 端） |

---

## 十、文件下载系统

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

### 10.1 `download(url, filename, headers, options)`

作用：下载文件到本地。

参数：

| 参数 | 类型 | 说明 |
| -------- | ----------------- | ---- |
| url | `std::string` 或 `std::wstring` | 文件地址 |
| filename | `std::string` 或 `std::wstring` | 保存路径 |
| headers | `Headers` | 请求头（可省略） |
| options | `DownloadOptions` | 下载配置（可省略） |

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
}
```

如果不需要自定义请求头，可以只传前两个参数：

```cpp
web::DownloadResult r = session.download(
    "https://example.com/test.zip",
    "test.zip"
);
```

---

## 十一、下载配置

`web::DownloadOptions` 用于控制下载行为。

### 11.1 `show_progress`

作用：是否显示下载进度。

类型：`bool`。

```cpp
web::DownloadOptions opt;
opt.show_progress = true;
```

### 11.2 `request.timeout_ms`

作用：设置下载超时时间。

单位：毫秒。

类型：`int`。

注意：`timeout_ms` 位于 `DownloadOptions::request` 子对象中，而不是直接挂在 `DownloadOptions` 上。

```cpp
web::DownloadOptions opt;
opt.request.timeout_ms = 30000;
```

### 11.3 `overwrite`

作用：是否覆盖已有文件。

类型：`bool`。

```cpp
web::DownloadOptions opt;
opt.overwrite = true;
```

### 11.4 `request.retry`

作用：设置下载失败重试。

类型：`RetryPolicy`。

注意：`retry` 位于 `DownloadOptions::request` 子对象中，而不是直接挂在 `DownloadOptions` 上。

```cpp
web::DownloadOptions opt;
opt.request.retry.retries = 5;
```

### 11.5 其他下载配置

| 字段 | 类型 | 作用 |
|------|------|------|
| `max_file_size` | `uint64_t` | 允许下载的最大文件大小 |
| `progress_bar_width` | `size_t` | 进度条宽度 |
| `progress_refresh_ms` | `unsigned int` | 进度刷新间隔（毫秒） |

---

## 十二、下载结果

`web::DownloadResult` 表示下载完成后的结果。

### 12.1 `ok()`

作用：判断下载是否成功。

返回：`bool`。

```cpp
if (result.ok())
{
    cout << "成功";
}
```

### 12.2 `bytes_written`

作用：实际写入文件大小。

类型：`uint64_t`。

```cpp
cout << result.bytes_written;
```

### 12.3 `file_size`

作用：文件总大小。

类型：`uint64_t`。

```cpp
cout << result.file_size;
```

### 12.4 `destination`

作用：保存文件路径。

类型：`std::wstring`。

```cpp
std::wcout << result.destination << std::endl;
```

### 12.5 `error_message`

作用：获取下载失败原因。

类型：`std::string`。

```cpp
if (!result.ok())
{
    cerr << result.error_message;
}
```

### 12.6 其他下载结果字段

| 字段 | 类型 | 作用 |
|------|------|------|
| `status_code` | `int` | HTTP 状态码 |
| `headers` | `ResponseHeaders` | 响应头 |
| `error_code` | `ErrorCode` | 错误码 |
| `native_error_code` | `DWORD` | 底层错误码 |
| `file_size_known` | `bool` | 文件总大小是否已知 |
| `attempts` | `int` | 实际请求次数 |
| `redirect_count` | `int` | 重定向次数 |
| `final_url` | `std::wstring` | 最终下载地址 |

---

## 十三、工具函数

FeatherCrawl 还提供了一些便捷工具函数。

### 13.1 `web::to_utf8(src, codepage)`

作用：将指定代码页的字符串转换为 UTF-8。

```cpp
std::string utf8 = web::to_utf8(gbk_text, 936);
```

### 13.2 `web::to_utf8(src, encoding)`

作用：将指定编码名的字符串转换为 UTF-8。

```cpp
std::string utf8 = web::to_utf8(gbk_text, "GBK");
```

### 13.3 `web::text_size(bytes, unit)`

作用：将字节数格式化为可读字符串。

```cpp
cout << web::text_size(1024 * 1024); // 1.00 MB
```

### 13.4 `web::substring(str, start, end)`

作用：按索引截取字符串（包含 `start` 和 `end`）。

```cpp
std::string part = web::substring("Hello World", 0, 4); // "Hello"
```

### 13.5 `web::lines(str, start_line, end_line)`

作用：按行截取字符串，行号从 1 开始。

```cpp
std::string part = web::lines("a\nb\nc", 2, 3); // "b\nc"
```

---

## 十四、浏览器与 HTML 渲染（Windows）

在 Windows 平台上，FeatherCrawl 可选支持 Microsoft Edge WebView2，用于打开网页或渲染 HTML。

### 14.1 `webview2_available()`

作用：检查当前环境是否可用 WebView2。

返回：`bool`。

```cpp
if (web::webview2_available())
{
    cout << "WebView2 可用";
}
```

### 14.2 `webview2_runtime_version()`

作用：获取 WebView2 运行时版本。

返回：`std::wstring`。

```cpp
std::wstring version = web::webview2_runtime_version();
std::wcout << version << std::endl;
```

### 14.3 `browse(url, options)`

作用：打开一个窗口浏览指定 URL。

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

### 14.4 `render_html(html, options)`

作用：渲染一段 HTML 字符串。

```cpp
web::BrowserResult r = web::render_html("<h1>Hello</h1>");
if (!r.ok())
{
    cerr << r.error_message << endl;
}
```

### 14.5 `BrowserOptions`

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
| `user_data_folder` | `std::wstring` | 空 | 用户数据目录 |
| `browser_executable_folder` | `std::wstring` | 空 | 浏览器可执行文件目录 |

### 14.6 `BrowserResult`

| 字段 | 类型 | 作用 |
|------|------|------|
| `error_code` | `BrowserErrorCode` | 错误码 |
| `native_error_code` | `HRESULT` | 底层错误码 |
| `error_message` | `std::string` | 错误描述 |
| `exit_code` | `int` | 窗口退出码 |
| `runtime_version` | `std::wstring` | WebView2 运行时版本 |
| `ok()` | `bool` | 是否成功 |

---

<h3 align="center">FeatherCrawl —— 让 C++ 爬虫变得简单而强大。</h3>
