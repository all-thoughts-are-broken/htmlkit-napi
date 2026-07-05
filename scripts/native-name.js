const packageName = 'htmlkit-napi';

function targetFromTriplet(triplet) {
  if (!triplet) return null;
  if (triplet === 'x64-windows-static') return 'win32-x64-msvc';
  if (triplet === 'arm64-windows-static') return 'win32-arm64-msvc';
  if (triplet === 'x64-linux') return 'linux-x64-gnu';
  if (triplet === 'arm64-linux') return 'linux-arm64-gnu';
  if (triplet === 'x64-osx') return 'darwin-x64';
  if (triplet === 'arm64-osx') return 'darwin-arm64';
  return null;
}

function targetFromRuntime(platform = process.platform, arch = process.arch) {
  if (platform === 'win32') {
    if (arch === 'x64') return 'win32-x64-msvc';
    if (arch === 'arm64') return 'win32-arm64-msvc';
  }
  if (platform === 'linux') {
    if (arch === 'x64') return 'linux-x64-gnu';
    if (arch === 'arm64') return 'linux-arm64-gnu';
  }
  if (platform === 'darwin') {
    if (arch === 'x64') return 'darwin-x64';
    if (arch === 'arm64') return 'darwin-arm64';
  }
  return null;
}

function nativeTarget() {
  return (
    process.env.HTMLKIT_NATIVE_TARGET ||
    targetFromTriplet(process.env.VCPKG_TARGET_TRIPLET) ||
    targetFromRuntime()
  );
}

function nativeFilename(target = nativeTarget()) {
  if (!target) {
    throw new Error(`Unsupported platform/architecture: ${process.platform}/${process.arch}`);
  }
  return `${packageName}.${target}.node`;
}

if (require.main === module) {
  process.stdout.write(`${nativeFilename()}\n`);
}

module.exports = {
  nativeFilename,
  nativeTarget,
  targetFromRuntime,
  targetFromTriplet,
};
