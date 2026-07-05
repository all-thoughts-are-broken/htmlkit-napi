export type ImageFormat = "png" | "jpeg" | "jpg";

export interface RenderOptions {
  baseUrl?: string;
  dpi?: number;
  width?: number;
  maxWidth?: number;
  height?: number;
  deviceHeight?: number;
  defaultFontSize?: number;
  fontName?: string;
  allowRefit?: boolean;
  imageFormat?: ImageFormat;
  jpegQuality?: number;
  imageFlag?: number;
  lang?: string;
  culture?: string;
  nativeDataScheme?: boolean;
  debug?: boolean;
  imgFetch?: (url: string) => Buffer | Uint8Array | null | undefined | Promise<Buffer | Uint8Array | null | undefined>;
  cssFetch?: (url: string) => string | null | undefined | Promise<string | null | undefined>;
  urlJoin?: (baseUrl: string, url: string) => string | Promise<string>;
}

export interface DebugRenderResult {
  image: Buffer;
  debugHtml: string;
}

export function initFontconfig(): void;
export function render(html: string, options?: RenderOptions & { debug?: false }): Promise<Buffer>;
export function render(html: string, options: RenderOptions & { debug: true }): Promise<DebugRenderResult>;
