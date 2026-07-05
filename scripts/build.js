const { spawnSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const rootDir = path.join(__dirname, '..');

function existingToolchain(root) {
  if (!root) return null;
  const file = path.join(root, 'scripts', 'buildsystems', 'vcpkg.cmake');
  return fs.existsSync(file) ? file : null;
}

function findFromCommand() {
  const command = process.platform === 'win32' ? 'where' : 'which';
  const result = spawnSync(command, ['vcpkg'], {
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'ignore'],
  });
  if (result.status !== 0) return null;
  const exe = result.stdout.split(/\r?\n/).find(Boolean);
  return exe ? path.dirname(exe.trim()) : null;
}

const commandRoot = findFromCommand();
const vcpkgRoot =
  existingToolchain(commandRoot) ? commandRoot : process.env.VCPKG_ROOT;
const toolchain = existingToolchain(vcpkgRoot);
if (!toolchain) {
  console.error('Unable to find vcpkg toolchain. Set VCPKG_ROOT or put vcpkg on PATH.');
  process.exit(1);
}

function defaultTriplet() {
  if (process.platform === 'win32') {
    if (process.arch === 'x64') return 'x64-windows-static';
    if (process.arch === 'arm64') return 'arm64-windows-static';
  }
  if (process.platform === 'linux') {
    if (process.arch === 'x64') return 'x64-linux';
    if (process.arch === 'arm64') return 'arm64-linux';
  }
  if (process.platform === 'darwin') {
    if (process.arch === 'x64') return 'x64-osx';
    if (process.arch === 'arm64') return 'arm64-osx';
  }
  console.error(`Unsupported platform/architecture: ${process.platform}/${process.arch}`);
  process.exit(1);
}

const triplet = process.env.VCPKG_TARGET_TRIPLET || defaultTriplet();
const overlayTriplets = path.join(rootDir, 'cmake', 'triplets');
const cmakeJs = path.join(rootDir, 'node_modules', 'cmake-js', 'bin', 'cmake-js');

function archFromTriplet(value) {
  if (value.startsWith('x64-')) return 'x64';
  if (value.startsWith('arm64-')) return 'arm64';
  return process.arch;
}

function cacheValue(cacheText, name) {
  const escaped = name.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  const match = cacheText.match(new RegExp(`^${escaped}:[^=]+=(.*)$`, 'm'));
  return match ? match[1].trim() : null;
}

function removeStaleBuildDirIfNeeded() {
  const buildDir = path.resolve(rootDir, 'build');
  const cacheFile = path.join(buildDir, 'CMakeCache.txt');
  if (!fs.existsSync(cacheFile)) return;

  const cacheText = fs.readFileSync(cacheFile, 'utf8');
  const cachedTriplet = cacheValue(cacheText, 'VCPKG_TARGET_TRIPLET');
  const cachedRuntime = cacheValue(cacheText, 'CMAKE_MSVC_RUNTIME_LIBRARY');
  const wantsStaticMsvcRuntime =
    process.platform === 'win32' && !triplet.includes('static-md');
  const hasDynamicMsvcRuntime =
    cachedRuntime ? cachedRuntime.includes('DLL') : false;

  if (
    cachedTriplet === triplet &&
    (!wantsStaticMsvcRuntime || !hasDynamicMsvcRuntime)
  ) {
    return;
  }

  const resolvedRoot = path.resolve(rootDir);
  if (!buildDir.startsWith(`${resolvedRoot}${path.sep}`)) {
    console.error(`Refusing to remove unexpected build directory: ${buildDir}`);
    process.exit(1);
  }

  console.log(`Removing stale CMake build directory for ${triplet}.`);
  fs.rmSync(buildDir, { recursive: true, force: true });
}

removeStaleBuildDirIfNeeded();

const args = [
  'compile',
  `--CDCMAKE_TOOLCHAIN_FILE=${toolchain}`,
  `--CDVCPKG_TARGET_TRIPLET=${triplet}`,
  '-a',
  archFromTriplet(triplet),
];
if (fs.existsSync(overlayTriplets)) {
  args.push(`--CDVCPKG_OVERLAY_TRIPLETS=${overlayTriplets}`);
}
if (process.platform === 'darwin') {
  args.push(
    `--CDCMAKE_OSX_ARCHITECTURES=${archFromTriplet(triplet) === 'x64' ? 'x86_64' : 'arm64'}`,
  );
}
if (process.env.CMAKE_GENERATOR_PLATFORM) {
  args.push('-A', process.env.CMAKE_GENERATOR_PLATFORM);
}

const result = spawnSync(process.execPath, [cmakeJs, ...args], { stdio: 'inherit' });
if (result.error) {
  console.error(result.error.message);
}
const status = result.status ?? 1;
if (status === 0 && process.platform === 'win32') {
  for (const config of ['Release', 'Debug', 'RelWithDebInfo']) {
    const dir = path.join(rootDir, 'build', config);
    for (const name of ['htmlkit.exp', 'htmlkit.lib']) {
      const file = path.join(dir, name);
      if (fs.existsSync(file)) {
        fs.rmSync(file, { force: true });
      }
    }
  }
}
process.exit(status);
