# htmlkit-napi

[English README](README.md)

`htmlkit-napi` 是一个基于 N-API 的 Node.js 原生 addon，用 litehtml 静态渲染
HTML，并通过 Pango/Cairo/fontconfig 输出 PNG 或 JPEG `Buffer`。它不启动浏览器
进程，适合在服务端直接把受控 HTML 模板渲染成图片。

项目使用 `cmake-js`、`node-addon-api`、CMake 和 vcpkg 构建。CI 会为每个支持的
平台和 CPU 架构生成一个独立的 `.node` 文件。

## 能做什么

- 在 Node.js 中把 HTML 和 CSS 渲染成 PNG 或 JPEG 图片。
- 使用 Pango/Cairo/fontconfig 进行文本测量、排版和绘制。
- 通过 `imgFetch(url)` 回调加载外部图片。
- 通过 `cssFetch(url)` 回调加载 `@import` 引入的 CSS。
- 通过 `baseUrl` 和可选的 `urlJoin(baseUrl, url)` 解析相对 URL。
- 默认支持 base64 `data:` URL 的原生解码。
- 能解码 PNG、JPEG、WebP、GIF、AVIF 图片资源。
- 通过 `{ debug: true }` 返回调试 HTML，便于检查渲染层。
- `render()` 以异步 N-API worker 执行，JavaScript 侧返回 Promise。

适合的场景包括卡片图、仪表盘、报表、收据、消息图片、预览图等服务端生成图片
任务，尤其是 HTML 和 CSS 由你自己控制的场景。

它不是浏览器截图工具。如果你需要执行 JavaScript、完全复刻 Chrome/WebKit 布局、
渲染 canvas/video，或者截图完整现代 Web 应用，应改用 Playwright 或 Puppeteer
这类浏览器引擎。

## 支持的平台

GitHub Actions 会构建并上传以下原生产物：

| 目标 | 产物 |
| --- | --- |
| Windows x64 | `htmlkit-napi.win32-x64-msvc.node` |
| Windows ARM64 | `htmlkit-napi.win32-arm64-msvc.node` |
| Linux x64 | `htmlkit-napi.linux-x64-gnu.node` |
| Linux ARM64 | `htmlkit-napi.linux-arm64-gnu.node` |
| macOS x64 | `htmlkit-napi.darwin-x64.node` |
| macOS ARM64 | `htmlkit-napi.darwin-arm64.node` |

加载器会按下面顺序查找 addon：

1. `HTMLKIT_NATIVE_LIBRARY_PATH`
2. 包根目录下的平台产物，例如 `htmlkit-napi.linux-x64-gnu.node`
3. `build/Release/htmlkit.node`
4. `build/Debug/htmlkit.node`

CI 构建目标是单文件 addon。第三方 vcpkg 依赖会静态链接进 `htmlkit.node`，发布
时不需要再携带 vcpkg 的 DLL、`.so` 或 `.dylib`。macOS 产物仍可能依赖系统自带的
dylib 和 framework。

## 安装

当前仓库默认在安装时从源码构建原生 addon：

```bash
npm install
```

如果你要发布一个带预编译产物的包，把对应平台的 `.node` 文件放到包根目录，也就
是和 `index.js` 同级。例如 Linux x64：

```text
htmlkit-napi/
  index.js
  index.d.ts
  htmlkit-napi.linux-x64-gnu.node
```

也可以用环境变量指定 addon 的绝对路径：

```bash
HTMLKIT_NATIVE_LIBRARY_PATH=/opt/htmlkit/htmlkit.node node app.js
```

Windows PowerShell：

```powershell
$env:HTMLKIT_NATIVE_LIBRARY_PATH = "C:\htmlkit\htmlkit.node"
node app.js
```

## 快速开始

```js
const fs = require('node:fs/promises');
const htmlkit = require('htmlkit-napi');

async function main() {
  htmlkit.initFontconfig();

  const image = await htmlkit.render(
    `<!doctype html>
    <html>
      <body style="font-family: Arial, sans-serif; padding: 32px;">
        <h1>Hello from htmlkit</h1>
        <p>This HTML was rendered by a native N-API addon.</p>
      </body>
    </html>`,
    {
      width: 800,
      imageFormat: 'png',
      fontName: 'Arial',
      defaultFontSize: 16,
    },
  );

  await fs.writeFile('hello.png', image);
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
```

默认情况下，`render()` 返回 `Promise<Buffer>`。除非显式请求 JPEG，否则返回的是
PNG 数据。

## 加载外部资源

原生渲染器本身不会主动访问网络或文件系统。这是有意设计：认证、缓存、超时、
代理、安全策略都由 JavaScript 回调负责。

```js
const { Buffer } = require('node:buffer');
const htmlkit = require('htmlkit-napi');

async function renderWithAssets() {
  htmlkit.initFontconfig();

  return htmlkit.render(
    `<!doctype html>
    <html>
      <head>
        <link rel="stylesheet" href="/styles/card.css">
      </head>
      <body>
        <img src="/images/logo.png">
        <h1>External assets</h1>
      </body>
    </html>`,
    {
      baseUrl: 'https://example.com/reports/weekly.html',
      width: 960,
      imgFetch: async (url) => {
        const response = await fetch(url);
        if (!response.ok) return null;
        return Buffer.from(await response.arrayBuffer());
      },
      cssFetch: async (url) => {
        const response = await fetch(url);
        if (!response.ok) return null;
        return response.text();
      },
      urlJoin: (baseUrl, url) => new URL(url, baseUrl).href,
    },
  );
}
```

`imgFetch` 必须返回 `Buffer`、`Uint8Array`、`null`、`undefined`，或这些值的
Promise。`cssFetch` 必须返回字符串、`null`、`undefined`，或这些值的 Promise。

## Data URL

默认会原生解码 base64 `data:` URL：

```html
<img src="data:image/png;base64,iVBORw0KGgo...">
```

当前原生 data URL 路径只处理包含 `;base64,` 的 payload。若你希望所有资源都走
`imgFetch` 和 `cssFetch`，可以设置 `nativeDataScheme: false`。

## 调试输出

需要同时拿到图片和调试 HTML 时，传入 `debug: true`：

```js
async function renderDebug(htmlkit, fs) {
  const result = await htmlkit.render('<h1>Debug</h1>', {
    width: 600,
    debug: true,
  });

  await fs.writeFile('debug.png', result.image);
  await fs.writeFile('debug.html', result.debugHtml);
}
```

`debug: true` 时，`render()` 返回：

```ts
{
  image: Buffer;
  debugHtml: string;
}
```

## API 参考

### `initFontconfig(): void`

初始化或重新初始化 fontconfig 和 Pango font map。建议在进程启动后、首次渲染前调用
一次，尤其是在 CI 或字体环境不固定的服务器上。

### `render(html, options): Promise<Buffer>`

把 HTML 字符串渲染成图片 Buffer。

```ts
render(html: string, options?: RenderOptions): Promise<Buffer>
```

### `render(html, { debug: true }): Promise<DebugRenderResult>`

把 HTML 字符串渲染成图片，并额外返回调试 HTML。

```ts
render(html: string, options: RenderOptions & { debug: true }): Promise<{
  image: Buffer;
  debugHtml: string;
}>
```

### 渲染选项

| 选项 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `baseUrl` | `string` | `""` | 用于解析相对图片 URL 和 CSS URL 的基础 URL。 |
| `width` | `number` | `800` | 布局视口宽度。未设置时先使用 `maxWidth`，再回退到 `800`。 |
| `maxWidth` | `number` | `800` | 兼容旧用法；仅在 `width` 未设置时生效。 |
| `height` | `number` | `600` | 布局和媒体查询使用的视口高度。最终图片高度由实际内容高度决定。 |
| `deviceHeight` | `number` | `600` | 兼容旧用法；仅在 `height` 未设置时生效。 |
| `dpi` | `number` | `96` | 暴露给渲染器和媒体特性的分辨率。 |
| `defaultFontSize` | `number` | `12` | 默认字体大小，单位为 point。 |
| `fontName` | `string` | `"sans-serif"` | 默认字体族。实际字体取决于系统已安装字体。 |
| `allowRefit` | `boolean` | `true` | 当内容比 `width` 窄时，允许输出宽度收缩到内容最佳宽度。 |
| `imageFormat` | `"png"`, `"jpeg"`, `"jpg"` | `"png"` | 输出图片格式。 |
| `jpegQuality` | `number` | `100` | `imageFormat` 为 JPEG 时的质量，范围通常为 `0` 到 `100`。 |
| `imageFlag` | `number` | 未设置 | 底层覆盖项。`0` 到 `100` 会强制按该质量输出 JPEG，其他值输出 PNG。 |
| `lang` | `string` | `"zh"` | 传给 litehtml 的语言。 |
| `culture` | `string` | `"CN"` | 传给 litehtml 的区域文化。 |
| `nativeDataScheme` | `boolean` | `true` | 启用图片和导入 CSS 的 base64 `data:` URL 原生解码。 |
| `debug` | `boolean` | `false` | 返回 `{ image, debugHtml }`，而不是只返回 `Buffer`。 |
| `imgFetch` | `function` | 未设置 | 为外部图片 URL 返回图片字节。 |
| `cssFetch` | `function` | 未设置 | 为 `@import` URL 返回 CSS 文本。 |
| `urlJoin` | `function` | 未设置 | 把 `(baseUrl, url)` 解析成绝对或规范化 URL。 |

## 渲染模型和局限性

`htmlkit-napi` 使用 litehtml、Pango 和 Cairo。它是确定性的静态渲染器，不是完整
浏览器引擎。主要局限如下：

- 不执行 JavaScript。没有 DOM 运行时、浏览器事件循环，也没有加载后的动态布局变更。
- 不提供 canvas、video、audio、WebGL、worker、storage、cookie、导航等浏览器 API。
- HTML/CSS 支持范围取决于 litehtml 的静态布局引擎。传统文档布局、文本、表格、
  inline/block、float、边框、背景、渐变和大量排版能力比较适合。flexbox、grid、
  复杂 transform、filter、动画、交互式表单控件等现代浏览器特性不要默认假设可用，
  需要用目标模板实际验证。
- 不会自动获取外部资源。图片和 `@import` CSS 只有在提供回调后才会加载。
- 原生图片加载器不解码 SVG 图片。请使用 PNG、JPEG、WebP、GIF、AVIF，或在渲染前
  先把 SVG 栅格化。
- 输出是单张图片。没有分页、PDF 生成、文本选择、无障碍树或增量绘制 API。
- `height` 是视口和媒体查询参数，不是固定裁剪高度。最终图片高度跟随文档内容高度。
- 文本输出依赖系统字体和 fontconfig/Pango 行为。需要像素稳定输出时，应在所有 runner
  或服务器上安装同一套字体。
- base64 `data:` 支持范围刻意较窄，要求 URL 中包含 `;base64,`。
- 处理不可信 HTML/CSS 时仍需谨慎。渲染器是原生代码，且用户提供的回调可以执行任意
  I/O。面对恶意输入时建议加进程隔离、输入大小限制和超时。

## 从源码构建

前置要求：

- Node.js 18+
- CMake 3.25+
- C++17 编译器
- vcpkg
- 平台构建工具：
  - Windows：Visual Studio 2022 C++ 工具链
  - Linux：`build-essential`、`autoconf`、`autoconf-archive`、`automake`、
    `libtool`、`nasm`、`ninja-build`
  - macOS：Xcode command line tools、`autoconf`、`autoconf-archive`、
    `automake`、`libtool`、`nasm`、`ninja`

把 `VCPKG_ROOT` 指向你的 vcpkg 目录，或者确保 `vcpkg` 在 `PATH` 中。

```bash
npm ci
npm run build
npm test
```

如果没有设置 `VCPKG_TARGET_TRIPLET`，`scripts/build.js` 会根据当前平台和架构选择
默认 triplet。

显式指定 triplet：

```bash
VCPKG_TARGET_TRIPLET=x64-windows-static npm run build
VCPKG_TARGET_TRIPLET=arm64-windows-static npm run build
VCPKG_TARGET_TRIPLET=x64-linux npm run build
VCPKG_TARGET_TRIPLET=arm64-linux npm run build
VCPKG_TARGET_TRIPLET=x64-osx npm run build
VCPKG_TARGET_TRIPLET=arm64-osx npm run build
```

PowerShell 示例：

```powershell
$env:VCPKG_TARGET_TRIPLET = "x64-windows-static"
npm run build

$env:VCPKG_TARGET_TRIPLET = "arm64-windows-static"
$env:CMAKE_GENERATOR_PLATFORM = "ARM64"
npm run build
```

构建后生成类似 napi-rs 命名风格的原生产物：

```bash
npm run artifact
```

该命令会把 `build/Release/htmlkit.node` 复制到：

```text
htmlkit-napi.<target>.node
artifacts/htmlkit-napi.<target>.node
```

## CI 验证

GitHub Actions workflow 会执行以下步骤：

1. 构建 6 个支持目标。
2. 检查 Windows、Linux、macOS 产物没有依赖第三方 vcpkg 动态库。
3. 为每个目标上传一个 `.node` 产物。
4. 在对应平台和架构的干净 runner 上下载该产物。
5. 执行两次 `demo/render-complex.js`。
6. 用零像素差异阈值比较两次生成的 PNG。
7. 上传 `render-complex.<target>` artifact，里面包含 `reference.png`、`actual.png`、
   `diff.png` 和 `report.json`。

已有原生产物时，也可以本地执行同一套验证脚本：

```bash
NATIVE_FILENAME=htmlkit-napi.linux-x64-gnu.node \
HTMLKIT_NATIVE_TARGET=linux-x64-gnu \
EXPECTED_RUNTIME=linux-x64 \
npm run verify:render-artifact
```

## Demo

渲染复杂仪表盘示例：

```bash
npm run build
node demo/render-complex.js
```

默认输出路径：

```text
demo/output/complex-dashboard.png
```

## 许可证

包许可证为 `MIT AND LGPL-3.0-or-later`。内置 renderer core 的 LGPL 许可证文本位于
`core/LICENSE`。
