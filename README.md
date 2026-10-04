<h1 align="center">FeatherCrawl</h1>

<p align="center">
  <a href="README.md">简体中文</a> |
  <a href="README_EN.md">English</a>
</p>

<p align="center">
  <a href="https://github.com/CompilerIDE/FeatherCrawl/blob/main/LICENSE"><img src="https://img.shields.io/github/license/CompilerIDE/FeatherCrawl.svg?style=for-the-badge&new=1" alt="License"></a>
  <br>
</p>

FeatherCrawl 是一款网络请求库。仅需一个头文件实现在 C++ 中进行网页抓取。

---

# 特性

- 架构：WinHTTP（Windows 平台），Socket API、OpenSSL（Linux 平台）
- 调用简单，仅需 `#include <feathercrawl.h>`
- C++11 及以上，支持在 Windows 10/11、Linux 上运行

---

# 开源许可证

FeatherCrawl 使用 [Apache License 2.0](LICENSE) 开源。

---

# 使用示例

## 1、快速上手

### 1.1 最小可运行程序
```cpp
#include <feathercrawl.h>
#include <iostream>
using namespace std;
int main()
{
    web::Session session; // 创建会话
    web::Response r = session.get("https://example.com"); // 发送 GET 请求
    if (r.ok()) // 判断请求是否成功  
    {
        cout << r.body << endl; // 打印响应体（即你要爬取的 HTML 等内容）
    }
    else
    {
        cerr << r.error_message << endl; // 打印失败原因
    }
}
```

**编译命令**

| 平台 | 命令 |
|------|------|
| Windows / MSVC | 直接编译，自动链接 `winhttp.lib` |
| Windows / MinGW | 需添加 `-lwinhttp` 编译参数 |
| Linux | 需添加 `-lssl -lcrypto -pthread` 编译参数 |

#### 1.2 三个核心函数（对象）
| 对象 | 作用 |
|------|--------|
| `web::Session` | 管理会话，即 Cookie、代理、连接池等 |
| `web::Headers` | 用户发送的请求头 |
| `web::Response` | 服务端返回的结果（状态码、响应体、响应头、错误等） |

## 二、语言设置
FeatherCrawl 支持手动切换语言，支持中文与英文，默认语言为英文。

### 2.1 `web::set_default_language(lang)`

作用：设置全局默认语言。

参数：lang —— "zh_CN" 或 "en_US"（也接受 "zh"、"cn"、"chinese" 等别名）。

示例：
```cpp
web::set_default_language("zh_CN");
web::set_default_language("en_US");
```

### 2.2 `Session::set_language(lang)`

作用：仅设置当前会话语言，不影响其他会话。

示例：
```cpp
web::Session session;
session.set_language("zh_CN");
```

### 2.3 `Session::language()`
作用：返回当前 Session 使用的语言字符串。

返回：string 类型，值为 "zh_CN" 或 "en_US"。

示例：
```cpp
cout << s.language() << endl;
```

## 三、请求头
`web::Headers` 是你要发送的请求头

### 3.1 `set(key, value)`
作用：添加一个请求头

参数：`key` 与 `value`。`key` 代表请求头的名字，例如：`User-Agent`；`value` 代表请求头的值。

示例：
```cpp
web::Headers headers;
headers.set("Content-Type", "application/json");
headers.set("Authorization", "Bearer 123456");
headers.set("User-Agent", "Crawler/1.0");
```

### 3.2 `get(key)`
作用：读取请求头的值，大小写不敏感

示例：
```cpp
string key = headers.get("content-type");
```

### 3.3 `contains(key)`
作用：判断请求头是否存在。

返回：bool 类型。

### 3.4 `erase(key)`
作用：删除指定请求头。

示例：
```cpp
headers.erase("Cookie");
```

### 3.5 `clear()`
作用：清除所有请求头

示例：
```cpp
headers.clear();
```

### 3.6 `to_winhttp_string()`
作用：转换为 WinHTTP 使用的请求头字符串格式。

返回：string 类型

示例：
```cpp
string text = headers.to_winhttp_string();
```

## 四、响应对象
`web::Response` 表示服务器返回的数据。

```cpp
web::Response r = session.get(url);
```

返回的就是一个 Response 对象。

包含：
- HTTP 状态码
- 响应正文
- 响应头
- 错误信息
- 请求统计信息

### 4.1 `ok()`
作用：判断请求是否成功。

返回：bool 类型（true 表示请求成功，false 表示请求失败）

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

### 4.2 `status_code`
作用：获取 HTTP 状态码。

返回：int 类型

常见值：
|状态码|含义   |
|:-:|:---:|
|200|成功   |
|301|永久重定向|
|302|临时重定向|
|400|请求错误 |
|403|禁止访问 |
|404|不存在  |
|500|服务器错误|

示例：
```cpp
web::Response r = session.get("https://example.com");
cout << r.status_code;
```

### 4.3 `body`
作用：返回服务器保存的正文内容（例如网页源代码）。

类型：string 类型

示例：
```cpp
web::Response r = session.get("https://example.com");
cout << r.body;
```

### 4.4 `headers`
作用：获取服务器返回的响应头。

类型：ResponseHeaders

示例：
```cpp
web::Response r = session.get("https://example.com");
string type = r.headers.get("Content-Type");
```

### 4.5 `error_code`
作用：获取错误类型。

示例：
```cpp
if(!r.ok())
{
    auto erc = r.error_code;
}
```

### 4.6 `error_message`
作用：获取错误描述。

类型：string 类型

示例：
```cpp
if(!r.ok())
{
    cerr << r.error_message;
}
```

### 4.7 `final_url`
作用：获取最终访问地址，适用于 HTTP 重定向或会自动跳转的网页。

类型：string 类型

示例：
```cpp
web::Response r = session.get("https://example.com");
cout << r.final_url;
```

### 4.8 `received_bytes`
作用：获取收到的数据大小。

类型：size_t 类型

示例：
```cpp
cout << r.received_bytes << " bytes";
```

### 4.9 `attempts`
作用：获取实际请求次数。

类型：int 类型

示例：
```cpp
cout << "请求次数:" << r.attempts;
```

### 4.10 `redirect_count`
作用：获取重定向次数

类型：int 类型

示例：
```cpp
cout << r.redirect_count;
```

## 五、网络会话

### 5.1 `Session()`
作用：创建一个默认网络会话。

示例：
```cpp
web::Session session;
```

### 5.2 `Session(options)`
作用：使用配置创建 Session。

参数：SessionOptions

示例：
```cpp
web::SessionOptions opt;
opt.user_agent = L"MyCrawler/1.0";
web::Session session(opt);
```

### 5.3 `get(url)`
作用：发送 GET 请求。

参数：
|参数 |类型    |说明  |
|:-:|:----:|:--:|
|url|string|请求地址|

返回：Response

示例：
```cpp
web::Session session;
web::Response r = session.get("https://example.com");
```

### 5.4 `get(url, headers)`
作用：带请求头发送 GET。

示例：
```cpp
web::Headers h;
h.set("User-Agent", "MyBot");
web::Response r = Session.get("https://example.com", h);
```

### 5.5 `get(url, headers, options)`

作用：完整 GET 请求。

参数：
| 参数      | 类型               |
| ------- | ---------------- |
| url     | `std::string`    |
| headers | `Headers`        |
| options | `RequestOptions` |

示例：
```cpp
web::RequestOptions opt;
opt.timeout_ms = 5000;
web::Response r = session.get("https://example.com", web::Headers(), opt);
```

### 5.6 `post(url, body, content_type)`

作用：发送 POST 请求。

参数：
| 参数           | 说明    |
| ------------ | ----- |
| url          | 请求地址  |
| body         | 发送的数据 |
| content_type | 数据类型  |

返回：Response

示例：
```cpp
web::Session session;
web::Response r = session.post("https://example.com/api", R"({"id":1})", "application/json");
```

### 5.7 `cookies()`

作用：获取当前会话的 Cookie 管理器。

返回：`CookieJar&`

示例：
```cpp
web::Session s;
auto& jar = s.cookies();
cout << jar.size();
```

### 5.8 `set_language(lang)`

作用：设置当前会话错误语言。

参数：string

示例：

```cpp
web::Session s;
s.set_language("zh_CN");
```

### 5.9 `language()`

作用：获取当前语言。

返回：string 类型

示例：
```cpp
cout << s.language();
```

---

### 5.10 `Session::clear_cookies()

作用：

清除当前 Session 保存的 Cookie。

示例：

```cpp
session.clear_cookies();
```

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

---

### 6.1 `timeout_ms`

作用：设置请求超时时间。

单位：毫秒。

类型：int 类型

示例：

```cpp
web::RequestOptions opt;
opt.timeout_ms = 10000;
```

### 6.2 `follow_redirect`

作用：是否自动跟随 HTTP 重定向。

类型：bool 类型

示例：
```cpp
web::RequestOptions opt;
opt.follow_redirect = true;
```

### 6.3 `max_redirects`

作用：设置最大重定向次数。

类型：int 类型

示例：
```cpp
web::RequestOptions opt;
opt.max_redirects = 10;
```

### 6.4 `max_response_size`

作用：限制服务器返回数据大小。

单位：字节。

类型：size_t 类型

示例：

```cpp
web::RequestOptions opt;
opt.max_response_size = 1024 * 1024; // 最大 1MB
```

超过限制时，请求失败。

### 6.5 `retry`

作用：设置自动重试策略。

类型：RetryPolicy

示例：
```cpp
web::RequestOptions opt;
opt.retry.retries = 3;
```

## 七、自动重试

`web::RetryPolicy` 用于网络不稳定环境。

例如：

- 网络波动
- 临时服务器错误
- 超时

---

### 7.1 `retries`

作用：设置最大重试次数。

类型：int 类型

示例：
```cpp
web::RequestOptions opt;
opt.retry.retries = 5;
```


### 7.2 `initial_delay_ms`

作用：第一次重试等待时间。

单位：毫秒。

示例：
```cpp
opt.retry.initial_delay_ms = 500;
```

### 7.3 `max_delay_ms`

作用：限制最大等待时间。

示例：
```cpp
opt.retry.max_delay_ms = 5000;
```

### 7.4 `multiplier`

作用：设置等待时间增长倍数。

类型：double 类型

示例：

```cpp
opt.retry.multiplier = 2.0;
```

## 八、Cookie 管理

`web::CookieJar` 用于管理 Session 中保存的 Cookie。

通常由：

```cpp
Session::cookies()
```

获得。

---

### 8.1 `size()`

作用：返回 Cookie 数量。

返回：size_t 类型

示例：
```cpp
auto count = session.cookies().size();
cout << count;
```

### 8.2 `clear()`

作用：清空所有 Cookie。

示例：
```cpp
session.cookies().clear();
```

### 8.3 `delete_cookie(name)`

作用：删除指定 Cookie。

参数 name 类型：string 类型

示例：
```cpp
session.cookies().delete_cookie("sessionid");
```

### 8.4 Cookie 自动保存

Session 默认会保存服务器返回的 Cookie。

示例：

```cpp
web::Session session;
// 登录
session.post("https://example.com/login", "user=test", "application/x-www-form-urlencoded");
// 后续请求自动携带 Cookie
web::Response r = session.get("https://example.com/user");
```

## 九、会话配置

`web::SessionOptions` 用于创建 Session 时设置默认行为。

示例：

```cpp
web::SessionOptions opt;
web::Session session(opt);
```

---

### 9.1 `user_agent`

作用：设置默认 User-Agent。

类型：wstring 类型

示例：
```cpp
web::SessionOptions opt;
opt.user_agent = L"MyCrawler/1.0";
web::Session s(opt);
```

### 9.2 `enable_cookies`

作用：是否启用 Cookie。

类型：bool 类型
```

示例：
```cpp
web::SessionOptions opt;
opt.enable_cookies = true;
```

### 9.3 `proxy`

作用：设置代理服务器。

类型：wstring 类型

示例：

```cpp
web::SessionOptions opt;
opt.proxy = L"http://127.0.0.1:7890";
web::Session s(opt);
```

### 9.4 `max_response_size`

作用：设置默认最大响应大小。

类型：size_t 类型

示例：
```cpp
web::SessionOptions opt;
opt.max_response_size = 64 * 1024 * 1024; // 表示最大 64MB
```

## 十、文件下载系统

`FeatherCrawl` 提供专用下载接口，用于：

- 下载普通文件
- 下载大文件
- 显示下载进度
- 获取下载统计信息

核心对象：

| 对象                    | 作用   |
| --------------------- | ---- |
| `DownloadOptions`     | 下载配置 |
| `DownloadResult`      | 下载结果 |
| `download()` | 执行下载 |

---

### 10.1 `download(url, filename, options)`

作用：下载文件到本地。

参数：
| 参数       | 类型                | 说明   |
| -------- | ----------------- | ---- |
| url      | `std::string`     | 文件地址 |
| filename | `std::string`     | 保存路径 |
| options  | `DownloadOptions` | 下载配置 |

返回：DownloadResult

示例：
```cpp
#include <feathercrawl.h>
#include <iostream>
using namespace std;
int main()
{
    web::Session session;
    web::DownloadOptions opt;
    web::DownloadResult r = session.download("https://example.com/test.zip", "test.zip", opt);
    if(r.ok())
    {
        cout << "下载完成";
    }
}
```

---

## 十一、下载配置

### 11.1 `show_progress`

作用：是否显示下载进度。

类型：bool 类型

示例：
```cpp
web::DownloadOptions opt;
opt.show_progress = true;
```

### 11.2 `timeout_ms`

作用：设置下载超时时间。

单位：毫秒。

类型：int 类型

示例：
```cpp
web::DownloadOptions opt;
opt.timeout_ms = 30000;
```

### 11.3 `overwrite`

作用：是否覆盖已有文件。

类型：bool 类型

示例：
```cpp
web::DownloadOptions opt;
opt.overwrite = true;
```

### 12.4 `buffer_size`

作用：设置下载缓冲区大小。

单位：字节。

类型：size_t 类

示例：
```cpp
web::DownloadOptions opt;
opt.buffer_size = 1024 * 1024;
```

### 12.5 `retry`

作用：设置下载失败重试。

类型：RetryPolicy

示例：
```cpp
web::DownloadOptions opt;
opt.retry.retries = 5;
```

## 十二、下载结果

### 12.1 `ok()`

作用：判断下载是否成功。

返回：bool 类型

示例：
```cpp
if (result.ok())
{
    cout << "成功";
}
```

### 12.2 `bytes_written`

作用：实际写入文件大小。

类型：size_t 类型

示例：
```cpp
cout << result.bytes_written;
```

### 12.3 `file_size`

作用：文件总大小。

类型：size_t 类型

示例：
```cpp
cout << result.file_size;
```

### 12.4 `destination`

作用：保存文件路径。

类型：string 类型

示例：
```cpp
cout << result.destination;
```

### 12.5 `error_message`

作用：获取下载失败原因。

类型：string 类型

示例：
```cpp
if (!result.ok())
{
    cerr << result.error_message;
}
```
---

<h3 align="center">FeatherCrawl —— 让 C++ 爬虫变得简单而强大。</h3>
