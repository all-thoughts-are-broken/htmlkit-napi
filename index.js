const path = require('node:path');
const { nativeFilename } = require('./scripts/native-name');

function loadBinding() {
  const platformNative = nativeFilename();
  const candidates = [
    process.env.HTMLKIT_NATIVE_LIBRARY_PATH,
    path.join(__dirname, platformNative),
    path.join(__dirname, 'build', 'Release', 'htmlkit.node'),
    path.join(__dirname, 'build', 'Debug', 'htmlkit.node'),
  ].filter(Boolean);

  const errors = [];
  for (const candidate of candidates) {
    try {
      return require(candidate);
    } catch (error) {
      errors.push(`${candidate}: ${error.message}`);
    }
  }

  throw new Error(`Unable to load htmlkit native addon:\n${errors.join('\n')}`);
}

module.exports = loadBinding();
