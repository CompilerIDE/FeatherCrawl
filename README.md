<h1 align="center">FeatherCrawl</h1>
<p align="center">
  <a href="https://github.com/CompilerIDE/FeatherCrawl/blob/main/LICENSE"><img src="https://img.shields.io/github/license/CompilerIDE/FeatherCrawl.svg?style=for-the-badge&new=1" alt="License"></a>
  <br>
</p>
<p align="center">
  <img src="https://img.shields.io/badge/C++-11+-47848F?style=flat&logo=c%2B%2B&logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/Windows 10+-blue?style=flat" alt="Windows 10+">
  <img src="https://img.shields.io/badge/Miniblink M108+-blue?style=flat&logo=qt&logoColor=gree" alt="Miniblink M108+">
</p>

# FeatherCrawl

**A C++11 and later web crawler for Windows（一款适用于Windows平台、C++11及以上版本的网页爬虫库）**

FeatherCrawl 融合 **WinHTTP** 的高效网络请求与 **Miniblink M108** 的完整 JS 渲染引擎。一个头文件 + 一个 DLL，搞定静态与动态网页抓取。

---

## ✨ 特性

- 双引擎架构：WinHTTP + Miniblink M108 (Chromium)
- 支持 JavaScript 渲染、无头模式、HTTPS
- Header-only，仅需 `#include "feathercrawl.h"`
- C++11 及以上，Windows 10/11 原生支持

---

## 🚀 快速开始

```cpp
#include "feathercrawl.h"

int main() {
    feather::Initialize();

    // 静态页面（WinHTTP）
    auto resp = feather::Get("https://example.com");

    // 动态页面（Miniblink JS渲染）
    auto dyn = feather::GetDynamic("https://example.com/spa");

    feather::Shutdown();
    return 0;
}
```

编译时链接 `winhttp.lib`，将 `mb108.dll` 置于 exe 同目录即可。

---

## 📦 交付物

- `feathercrawl.h` —— 主头文件
- `mb108.dll` —— Miniblink M108 内核 (≈45MB)

---

## 📄 许可证

Apache License 2.0

---

**FeatherCrawl —— 让 C++ 爬虫变得简单而强大。🪶**

---

这个版本去掉了冗长的表格、详细的 API 列表、构建要求说明和贡献指南，只保留了最核心的：**它是什么、怎么用、需要什么文件**。用户 2 分钟就能读完并上手。

如果你觉得还需要保留某些部分（比如 API 概览），我可以再加回来，但整体会保持精简。😄
