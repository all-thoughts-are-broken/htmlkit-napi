const assert = require('node:assert/strict');
const htmlkit = require('..');

async function main() {
  htmlkit.initFontconfig();

  const image = await htmlkit.render(
    '<html><body><h1>Hello, World!</h1><p>htmlkit napi</p></body></html>',
    { width: 500 },
  );
  assert(Buffer.isBuffer(image));
  assert.equal(image.subarray(0, 8).toString('hex'), '89504e470d0a1a0a');

  const debug = await htmlkit.render('<html><body><p>debug</p></body></html>', {
    debug: true,
  });
  assert(Buffer.isBuffer(debug.image));
  assert.equal(typeof debug.debugHtml, 'string');
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
