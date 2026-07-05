const { spawnSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const rootDir = path.join(__dirname, '..');
const nativeFilename = process.env.NATIVE_FILENAME;
const nativeTarget = process.env.HTMLKIT_NATIVE_TARGET;
const expectedRuntime = process.env.EXPECTED_RUNTIME;
const visualDir = path.resolve(
  process.env.VISUAL_DIR || path.join(rootDir, 'visual-output', nativeTarget || 'native'),
);

function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: rootDir,
    env: process.env,
    stdio: 'inherit',
    ...options,
  });
  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    throw new Error(`${command} ${args.join(' ')} exited with ${result.status}`);
  }
}

function assertCleanConsumerTree() {
  const forbidden = [
    path.join(rootDir, 'build', 'Release', 'htmlkit.node'),
    path.join(rootDir, 'build', 'vcpkg_installed'),
    path.join(rootDir, 'vcpkg_installed'),
  ];
  for (const item of forbidden) {
    if (fs.existsSync(item)) {
      throw new Error(`Clean artifact verification must not use ${item}`);
    }
  }
}

if (!nativeFilename) {
  throw new Error('NATIVE_FILENAME is required.');
}
if (!nativeTarget) {
  throw new Error('HTMLKIT_NATIVE_TARGET is required.');
}

const runtime = `${process.platform}-${process.arch}`;
console.log(`Node runtime: ${runtime}`);
if (expectedRuntime && runtime !== expectedRuntime) {
  throw new Error(`Expected runtime ${expectedRuntime}, got ${runtime}.`);
}

assertCleanConsumerTree();

const downloadedNative = path.join(rootDir, 'native-artifact', nativeFilename);
const rootNative = path.join(rootDir, nativeFilename);
if (!fs.existsSync(downloadedNative)) {
  throw new Error(`Missing downloaded native artifact: ${downloadedNative}`);
}

fs.copyFileSync(downloadedNative, rootNative);
fs.rmSync(visualDir, { recursive: true, force: true });
fs.mkdirSync(visualDir, { recursive: true });

const reference = path.join(visualDir, 'reference.png');
const actual = path.join(visualDir, 'actual.png');
const diff = path.join(visualDir, 'diff.png');
const report = path.join(visualDir, 'report.json');

for (const output of [reference, actual]) {
  run(process.execPath, [path.join(rootDir, 'demo', 'render-complex.js')], {
    env: {
      ...process.env,
      HTMLKIT_RENDER_OUTPUT: output,
      HTMLKIT_NATIVE_TARGET: nativeTarget,
    },
  });
}

run(process.execPath, [
  path.join(rootDir, 'scripts', 'compare-png.js'),
  '--baseline',
  reference,
  '--actual',
  actual,
  '--diff',
  diff,
  '--report',
  report,
  '--threshold',
  '0',
  '--max-different-pixels',
  '0',
  '--max-different-ratio',
  '0',
]);
