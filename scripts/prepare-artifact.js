const fs = require('node:fs');
const path = require('node:path');
const { nativeFilename } = require('./native-name');

const rootDir = path.join(__dirname, '..');
const source = path.join(rootDir, 'build', 'Release', 'htmlkit.node');
const filename = nativeFilename();
const artifactDir = path.join(rootDir, 'artifacts');
const rootTarget = path.join(rootDir, filename);
const artifactTarget = path.join(artifactDir, filename);

if (!fs.existsSync(source)) {
  console.error(`Missing native addon: ${source}`);
  process.exit(1);
}

fs.mkdirSync(artifactDir, { recursive: true });
fs.copyFileSync(source, rootTarget);
fs.copyFileSync(source, artifactTarget);
console.log(artifactTarget);
