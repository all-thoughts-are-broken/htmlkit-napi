# htmlkit-napi

N-API binding for the litehtml based HTML renderer.

## Platform

- Windows x64/arm64
- Linux x64/arm64
- macOS x64/arm64

The build is configured for a single addon artifact. Third-party vcpkg
dependencies are linked statically into `htmlkit.node`; do not ship companion
vcpkg DLLs, shared objects, or dylibs with the package. macOS builds may still
depend on system dylibs and frameworks provided by macOS.

GitHub Actions uploads one `.node` artifact per target:

- `htmlkit-napi.win32-x64-msvc.node`
- `htmlkit-napi.win32-arm64-msvc.node`
- `htmlkit-napi.linux-x64-gnu.node`
- `htmlkit-napi.linux-arm64-gnu.node`
- `htmlkit-napi.darwin-x64.node`
- `htmlkit-napi.darwin-arm64.node`

## Prerequisites

- Node.js 18+
- CMake
- A C++17 compiler
- vcpkg

Set `VCPKG_ROOT` to your vcpkg checkout, or put `vcpkg` on `PATH`.

## Build

```bash
npm install
npm run build
```

For cross architecture builds, set the vcpkg triplet before building:

```bash
VCPKG_TARGET_TRIPLET=x64-windows-static npm run build
VCPKG_TARGET_TRIPLET=arm64-windows-static npm run build
VCPKG_TARGET_TRIPLET=x64-linux npm run build
VCPKG_TARGET_TRIPLET=arm64-linux npm run build
VCPKG_TARGET_TRIPLET=x64-osx npm run build
VCPKG_TARGET_TRIPLET=arm64-osx npm run build
```

If `VCPKG_TARGET_TRIPLET` is omitted, `scripts/build.js` selects a static
triplet for the current platform and architecture.

To prepare a napi-rs-style native artifact after building:

```bash
npm run artifact
```

## Usage

```js
const htmlkit = require('htmlkit-napi');

htmlkit.initFontconfig();

const image = await htmlkit.render('<h1>Hello</h1>', {
  width: 800,
  imageFormat: 'png',
  imgFetch: async (url) => null,
  cssFetch: async (url) => null,
});
```

`render()` returns a `Promise<Buffer>` by default. With `{ debug: true }`, it returns
`Promise<{ image: Buffer, debugHtml: string }>`.

## API

- `initFontconfig(): void`
- `render(html: string, options?: RenderOptions): Promise<Buffer | DebugRenderResult>`

Important options:

- `baseUrl`
- `width` / `maxWidth`
- `height` / `deviceHeight`
- `dpi`
- `defaultFontSize`
- `fontName`
- `allowRefit`
- `imageFormat`: `"png"` or `"jpeg"`
- `jpegQuality`
- `nativeDataScheme`
- `imgFetch(url)`
- `cssFetch(url)`
- `urlJoin(baseUrl, url)`

`imgFetch`, `cssFetch`, and `urlJoin` may return values directly or return promises.
