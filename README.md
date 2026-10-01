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

---

<h3 align="center">FeatherCrawl —— 让 C++ 爬虫变得简单而强大。</h3>
