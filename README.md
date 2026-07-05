# htmlkit-napi

[中文文档](README.zh-CN.md)

`htmlkit-napi` is a Node.js N-API binding for a litehtml based static HTML
renderer. It renders an HTML string to a PNG or JPEG `Buffer` without starting a
browser process.

The native addon is built with `cmake-js`, `node-addon-api`, CMake, and vcpkg.
CI produces one `.node` file per supported platform and architecture.

## What It Can Do

- Render HTML and CSS into PNG or JPEG buffers from Node.js.
- Use Pango/Cairo/fontconfig for text measurement and drawing.
- Load external images through an `imgFetch(url)` callback.
- Load imported CSS through a `cssFetch(url)` callback.
- Resolve relative URLs with `baseUrl` and an optional `urlJoin(baseUrl, url)`
  callback.
- Decode base64 `data:` URLs when `nativeDataScheme` is enabled.
- Decode image resources in PNG, JPEG, WebP, GIF, and AVIF formats.
- Return debug output with `{ debug: true }` for inspecting render layers.
- Run as an async N-API task, so `render()` returns a Promise.

It is a good fit for generated cards, dashboards, reports, receipts, previews,
message images, and other server-side images where you control the HTML and CSS.

It is not a browser screenshot tool. If you need JavaScript execution, exact
Chrome/WebKit layout behavior, canvas, video, or full modern web app rendering,
use a browser engine such as Playwright or Puppeteer instead.

## Supported Targets

GitHub Actions builds and uploads these native artifacts:

| Target | Artifact |
| --- | --- |
| Windows x64 | `htmlkit-napi.win32-x64-msvc.node` |
| Windows ARM64 | `htmlkit-napi.win32-arm64-msvc.node` |
| Linux x64 | `htmlkit-napi.linux-x64-gnu.node` |
| Linux ARM64 | `htmlkit-napi.linux-arm64-gnu.node` |
| macOS x64 | `htmlkit-napi.darwin-x64.node` |
| macOS ARM64 | `htmlkit-napi.darwin-arm64.node` |

The package loader searches in this order:

1. `HTMLKIT_NATIVE_LIBRARY_PATH`
2. The platform artifact in the package root, for example
   `htmlkit-napi.linux-x64-gnu.node`
3. `build/Release/htmlkit.node`
4. `build/Debug/htmlkit.node`

The CI build is configured for a single addon artifact. Third-party vcpkg
dependencies are linked statically into `htmlkit.node`; do not ship companion
vcpkg DLLs, shared objects, or dylibs with the package. macOS builds may still
depend on system dylibs and frameworks provided by macOS.

## Installation

This repository currently builds the native addon from source during install:

```bash
npm install
```

For a package that contains prebuilt CI artifacts, place the correct `.node`
file in the package root next to `index.js`. For example, on Linux x64:

```text
htmlkit-napi/
  index.js
  index.d.ts
  htmlkit-napi.linux-x64-gnu.node
```

You can also point the loader at an absolute native addon path:

```bash
HTMLKIT_NATIVE_LIBRARY_PATH=/opt/htmlkit/htmlkit.node node app.js
```

On Windows PowerShell:

```powershell
$env:HTMLKIT_NATIVE_LIBRARY_PATH = "C:\htmlkit\htmlkit.node"
node app.js
```

## Quick Start

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

`render()` returns a `Promise<Buffer>` by default. The buffer starts with PNG
bytes unless you request JPEG output.

## Rendering External Assets

The native renderer does not perform network or filesystem I/O by itself. This
is intentional: your JavaScript code owns fetching, authentication, caching,
timeouts, and security policy.

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

`imgFetch` must return `Buffer`, `Uint8Array`, `null`, `undefined`, or a Promise
for one of those values. `cssFetch` must return a string, `null`, `undefined`,
or a Promise for one of those values.

## Data URLs

Base64 `data:` URLs are decoded natively by default:

```html
<img src="data:image/png;base64,iVBORw0KGgo...">
```

The native data URL path only handles base64 payloads containing `;base64,`.
Disable it with `nativeDataScheme: false` if you want all resource loading to go
through `imgFetch` and `cssFetch`.

## Debug Output

Use `debug: true` when you need the image plus an HTML debug document:

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

With `debug: true`, `render()` returns:

```ts
{
  image: Buffer;
  debugHtml: string;
}
```

## API Reference

### `initFontconfig(): void`

Initializes/reinitializes fontconfig and the Pango font map. Call it once during
process startup before rendering, especially in CI or server environments where
font configuration can vary.

### `render(html, options): Promise<Buffer>`

Renders an HTML string to an image buffer.

```ts
render(html: string, options?: RenderOptions): Promise<Buffer>
```

### `render(html, { debug: true }): Promise<DebugRenderResult>`

Renders an HTML string and returns both the image buffer and debug HTML.

```ts
render(html: string, options: RenderOptions & { debug: true }): Promise<{
  image: Buffer;
  debugHtml: string;
}>
```

### Render Options

| Option | Type | Default | Description |
| --- | --- | --- | --- |
| `baseUrl` | `string` | `""` | Base URL used to resolve relative image and CSS URLs. |
| `width` | `number` | `800` | Layout viewport width. If omitted, `maxWidth` is used before falling back to `800`. |
| `maxWidth` | `number` | `800` | Backward-compatible alias used when `width` is not set. |
| `height` | `number` | `600` | Viewport/media height for layout and media queries. Output height is based on rendered content height. |
| `deviceHeight` | `number` | `600` | Backward-compatible alias used when `height` is not set. |
| `dpi` | `number` | `96` | Resolution exposed to the renderer and media features. |
| `defaultFontSize` | `number` | `12` | Default font size in points. |
| `fontName` | `string` | `"sans-serif"` | Default font family. The actual font depends on installed system fonts. |
| `allowRefit` | `boolean` | `true` | Allows the output width to shrink to the best content width when content is narrower than `width`. |
| `imageFormat` | `"png"`, `"jpeg"`, `"jpg"` | `"png"` | Output image format. |
| `jpegQuality` | `number` | `100` | JPEG quality when `imageFormat` is `jpeg` or `jpg`. |
| `imageFlag` | `number` | unset | Low-level override. Values from `0` to `100` force JPEG encoding with that quality; other values use PNG. |
| `lang` | `string` | `"zh"` | Language exposed to litehtml. |
| `culture` | `string` | `"CN"` | Culture exposed to litehtml. |
| `nativeDataScheme` | `boolean` | `true` | Enables native base64 `data:` URL decoding for images and imported CSS. |
| `debug` | `boolean` | `false` | Returns `{ image, debugHtml }` instead of only `Buffer`. |
| `imgFetch` | `function` | unset | Fetches image bytes for external image URLs. |
| `cssFetch` | `function` | unset | Fetches CSS text for `@import` URLs. |
| `urlJoin` | `function` | unset | Resolves `(baseUrl, url)` to an absolute or canonical URL. |

## Rendering Model and Limitations

`htmlkit-napi` renders with litehtml, Pango, and Cairo. It is a deterministic
static renderer, not a full browser engine. The main limitations are:

- JavaScript is not executed. There is no DOM runtime, no browser event loop,
  and no post-load layout mutation.
- Browser APIs such as canvas, video, audio, WebGL, workers, storage, cookies,
  and navigation are not available.
- HTML and CSS support follows litehtml's static layout engine. Classic
  document layout, text, tables, inline/block layout, floats, borders,
  backgrounds, gradients, and many typography features work well. Modern browser
  layout features such as flexbox, grid, complex transforms, filters,
  animations, and interactive form controls should be tested before relying on
  them.
- External resources are not fetched automatically. Images and imported CSS only
  load when you provide callbacks.
- SVG images are not decoded by the native image loader. Use PNG, JPEG, WebP,
  GIF, AVIF, or pre-rasterize SVG content before rendering.
- Output is a single image. There is no pagination, PDF generation, selection,
  accessibility tree, or incremental painting API.
- `height` is a viewport/media value. The generated image height follows the
  rendered document content height.
- Text output depends on available fonts and fontconfig/Pango behavior. Install
  the same fonts on every runner or server when you need pixel-stable output.
- Base64 `data:` support is intentionally narrow and expects `;base64,`.
- Untrusted HTML/CSS should still be handled carefully. The renderer is native
  code, and user-provided callbacks can perform arbitrary I/O. Use process
  isolation, input limits, and timeouts for hostile input.

## Build From Source

Prerequisites:

- Node.js 18+
- CMake 3.25+
- A C++17 compiler
- vcpkg
- Platform build tools:
  - Windows: Visual Studio 2022 C++ tools
  - Linux: `build-essential`, `autoconf`, `autoconf-archive`, `automake`,
    `libtool`, `nasm`, `ninja-build`
  - macOS: Xcode command line tools, `autoconf`, `autoconf-archive`,
    `automake`, `libtool`, `nasm`, `ninja`

Set `VCPKG_ROOT` to your vcpkg checkout, or put `vcpkg` on `PATH`.

```bash
npm ci
npm run build
npm test
```

If `VCPKG_TARGET_TRIPLET` is omitted, `scripts/build.js` selects a static
triplet for the current platform and architecture.

Explicit triplets:

```bash
VCPKG_TARGET_TRIPLET=x64-windows-static npm run build
VCPKG_TARGET_TRIPLET=arm64-windows-static npm run build
VCPKG_TARGET_TRIPLET=x64-linux npm run build
VCPKG_TARGET_TRIPLET=arm64-linux npm run build
VCPKG_TARGET_TRIPLET=x64-osx npm run build
VCPKG_TARGET_TRIPLET=arm64-osx npm run build
```

PowerShell examples:

```powershell
$env:VCPKG_TARGET_TRIPLET = "x64-windows-static"
npm run build

$env:VCPKG_TARGET_TRIPLET = "arm64-windows-static"
$env:CMAKE_GENERATOR_PLATFORM = "ARM64"
npm run build
```

Create the napi-rs-style native artifact name after building:

```bash
npm run artifact
```

This copies `build/Release/htmlkit.node` to both:

```text
htmlkit-napi.<target>.node
artifacts/htmlkit-napi.<target>.node
```

## CI Verification

The GitHub Actions workflow:

1. Builds all six supported targets.
2. Verifies that Windows, Linux, and macOS outputs do not depend on third-party
   vcpkg dynamic libraries.
3. Uploads one `.node` artifact per target.
4. Downloads each artifact on a clean runner for that platform/architecture.
5. Runs `demo/render-complex.js` twice.
6. Compares the two generated PNG files with a zero-pixel-difference threshold.
7. Uploads `render-complex.<target>` artifacts containing `reference.png`,
   `actual.png`, `diff.png`, and `report.json`.

Run the same verification script locally when you have a prepared native
artifact:

```bash
NATIVE_FILENAME=htmlkit-napi.linux-x64-gnu.node \
HTMLKIT_NATIVE_TARGET=linux-x64-gnu \
EXPECTED_RUNTIME=linux-x64 \
npm run verify:render-artifact
```

## Demo

Render the complex dashboard sample:

```bash
npm run build
node demo/render-complex.js
```

The default output is:

```text
demo/output/complex-dashboard.png
```

## License

The package is licensed as `MIT AND LGPL-3.0-or-later`. The bundled renderer
core carries its own LGPL license text in `core/LICENSE`.
