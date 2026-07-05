const fs = require('node:fs');
const path = require('node:path');
const { PNG } = require('pngjs');

function usage() {
  console.error([
    'Usage: node scripts/compare-png.js --baseline <png> --actual <png> --diff <png> [options]',
    '',
    'Options:',
    '  --report <json>                 Write a JSON summary.',
    '  --threshold <0-255>             Per-channel difference threshold. Default: 0.',
    '  --max-different-pixels <count>  Allowed changed pixels. Default: 0.',
    '  --max-different-ratio <ratio>   Allowed changed pixel ratio. Default: 0.',
  ].join('\n'));
}

function readArgs(argv) {
  const args = {
    threshold: 0,
    maxDifferentPixels: 0,
    maxDifferentRatio: 0,
  };

  for (let index = 2; index < argv.length; index += 1) {
    const key = argv[index];
    const value = argv[index + 1];
    if (!key.startsWith('--') || value === undefined) {
      usage();
      process.exit(2);
    }
    index += 1;

    if (key === '--baseline') args.baseline = value;
    else if (key === '--actual') args.actual = value;
    else if (key === '--diff') args.diff = value;
    else if (key === '--report') args.report = value;
    else if (key === '--threshold') args.threshold = Number(value);
    else if (key === '--max-different-pixels') args.maxDifferentPixels = Number(value);
    else if (key === '--max-different-ratio') args.maxDifferentRatio = Number(value);
    else {
      usage();
      process.exit(2);
    }
  }

  for (const required of ['baseline', 'actual', 'diff']) {
    if (!args[required]) {
      usage();
      process.exit(2);
    }
  }

  return args;
}

function readPng(file) {
  return PNG.sync.read(fs.readFileSync(file));
}

function ensureParentDir(file) {
  fs.mkdirSync(path.dirname(path.resolve(file)), { recursive: true });
}

function writeJson(file, value) {
  if (!file) return;
  ensureParentDir(file);
  fs.writeFileSync(file, `${JSON.stringify(value, null, 2)}\n`);
}

const args = readArgs(process.argv);
const baseline = readPng(args.baseline);
const actual = readPng(args.actual);
const sameSize = baseline.width === actual.width && baseline.height === actual.height;
const width = sameSize ? baseline.width : Math.max(baseline.width, actual.width);
const height = sameSize ? baseline.height : Math.max(baseline.height, actual.height);
const diff = new PNG({ width, height });

let differentPixels = 0;
let maxChannelDelta = 0;

for (let y = 0; y < height; y += 1) {
  for (let x = 0; x < width; x += 1) {
    const out = (y * width + x) * 4;
    const insideBaseline = x < baseline.width && y < baseline.height;
    const insideActual = x < actual.width && y < actual.height;

    if (!insideBaseline || !insideActual) {
      differentPixels += 1;
      diff.data[out] = 255;
      diff.data[out + 1] = 0;
      diff.data[out + 2] = 0;
      diff.data[out + 3] = 255;
      continue;
    }

    const base = (y * baseline.width + x) * 4;
    const act = (y * actual.width + x) * 4;
    let pixelDelta = 0;
    for (let channel = 0; channel < 4; channel += 1) {
      const channelDelta = Math.abs(baseline.data[base + channel] - actual.data[act + channel]);
      pixelDelta = Math.max(pixelDelta, channelDelta);
      maxChannelDelta = Math.max(maxChannelDelta, channelDelta);
    }

    if (pixelDelta > args.threshold) {
      differentPixels += 1;
      diff.data[out] = 255;
      diff.data[out + 1] = 0;
      diff.data[out + 2] = 0;
      diff.data[out + 3] = 255;
    } else {
      const gray = Math.round(
        actual.data[act] * 0.299 +
        actual.data[act + 1] * 0.587 +
        actual.data[act + 2] * 0.114,
      );
      diff.data[out] = gray;
      diff.data[out + 1] = gray;
      diff.data[out + 2] = gray;
      diff.data[out + 3] = 120;
    }
  }
}

ensureParentDir(args.diff);
fs.writeFileSync(args.diff, PNG.sync.write(diff));

const totalPixels = width * height;
const differenceRatio = totalPixels === 0 ? 0 : differentPixels / totalPixels;
const result = {
  baseline: path.resolve(args.baseline),
  actual: path.resolve(args.actual),
  diff: path.resolve(args.diff),
  sameSize,
  baselineSize: { width: baseline.width, height: baseline.height },
  actualSize: { width: actual.width, height: actual.height },
  totalPixels,
  differentPixels,
  differenceRatio,
  maxChannelDelta,
  threshold: args.threshold,
  maxDifferentPixels: args.maxDifferentPixels,
  maxDifferentRatio: args.maxDifferentRatio,
};

const passed =
  sameSize &&
  differentPixels <= args.maxDifferentPixels &&
  differenceRatio <= args.maxDifferentRatio;
result.passed = passed;
writeJson(args.report, result);

console.log(JSON.stringify(result, null, 2));
if (!passed) {
  process.exit(1);
}
