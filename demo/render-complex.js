const fs = require('node:fs/promises');
const path = require('node:path');
const htmlkit = require('..');

const html = `<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    body {
      margin: 0;
      padding: 0;
      background: #eef2f3;
      color: #1b1f24;
      font-family: Arial, sans-serif;
      font-size: 16px;
    }
    .page {
      width: 1080px;
      padding: 34px 40px 42px;
      background: #f7f9fa;
    }
    .header {
      padding: 24px 28px;
      background: #16313a;
      color: #ffffff;
      border-radius: 8px;
    }
    .brand-mark {
      float: left;
      width: 58px;
      height: 58px;
      margin-right: 18px;
      background: #f0c24b;
      border-radius: 7px;
      color: #102127;
      font-size: 30px;
      font-weight: bold;
      text-align: center;
      line-height: 58px;
    }
    h1 {
      margin: 0 0 5px;
      font-size: 32px;
      line-height: 38px;
      font-weight: 700;
    }
    .subtitle {
      color: #c7d2d6;
      font-size: 15px;
    }
    .clear {
      clear: both;
    }
    .tagline {
      margin-top: 22px;
      padding-top: 18px;
      border-top: 1px solid #42606a;
      color: #e7edef;
      font-size: 15px;
    }
    .pill {
      display: inline-block;
      margin-right: 8px;
      padding: 5px 10px;
      background: #234953;
      color: #d9e6e9;
      border-radius: 5px;
      font-size: 13px;
    }
    .section-title {
      margin: 28px 0 14px;
      font-size: 21px;
      font-weight: 700;
      color: #243039;
    }
    .metrics {
      width: 100%;
      border-spacing: 12px 0;
      margin-left: -12px;
    }
    .metric {
      padding: 18px 18px 17px;
      background: #ffffff;
      border: 1px solid #d8e0e3;
      border-radius: 8px;
      vertical-align: top;
    }
    .metric-name {
      color: #60717a;
      font-size: 13px;
      text-transform: uppercase;
    }
    .metric-value {
      margin-top: 9px;
      font-size: 30px;
      font-weight: bold;
      color: #16313a;
    }
    .metric-delta {
      margin-top: 6px;
      font-size: 14px;
      color: #2f7d5f;
    }
    .columns {
      width: 100%;
      border-spacing: 18px 0;
      margin-left: -18px;
    }
    .panel {
      background: #ffffff;
      border: 1px solid #d8e0e3;
      border-radius: 8px;
      padding: 20px;
      vertical-align: top;
    }
    .panel h2 {
      margin: 0 0 16px;
      font-size: 19px;
      color: #243039;
    }
    .bar-row {
      margin: 12px 0;
    }
    .bar-label {
      display: inline-block;
      width: 96px;
      color: #3c4950;
      font-size: 14px;
    }
    .bar-track {
      display: inline-block;
      width: 300px;
      height: 16px;
      background: #e7ecee;
      border-radius: 5px;
      vertical-align: middle;
    }
    .bar-fill {
      display: inline-block;
      height: 16px;
      border-radius: 5px;
      background: #3f8d7b;
    }
    .warn {
      background: #d98d42;
    }
    .blue {
      background: #3c79a8;
    }
    .note {
      margin-top: 18px;
      padding: 14px 16px;
      background: #f2f0e2;
      border-left: 5px solid #d6b447;
      color: #4d4b3b;
      line-height: 22px;
      border-radius: 5px;
    }
    .timeline {
      border-left: 4px solid #d0dadf;
      padding-left: 18px;
    }
    .event {
      margin: 0 0 15px;
      padding-bottom: 13px;
      border-bottom: 1px solid #e5eaec;
    }
    .event b {
      color: #16313a;
    }
    .event span {
      color: #68777f;
      font-size: 13px;
    }
    .data-table {
      width: 100%;
      border-collapse: collapse;
      background: #ffffff;
      border: 1px solid #d8e0e3;
      border-radius: 8px;
    }
    .data-table th {
      background: #e5ecef;
      color: #243039;
      text-align: left;
      padding: 12px 14px;
      border-bottom: 1px solid #cdd8dc;
      font-size: 14px;
    }
    .data-table td {
      padding: 13px 14px;
      border-bottom: 1px solid #edf1f2;
      font-size: 14px;
    }
    .badge {
      display: inline-block;
      padding: 4px 8px;
      border-radius: 4px;
      background: #dfeee7;
      color: #276247;
      font-size: 12px;
      font-weight: bold;
    }
    .badge.alert {
      background: #f4e4d1;
      color: #87511f;
    }
    .footer {
      margin-top: 24px;
      padding: 16px 20px;
      color: #596970;
      font-size: 13px;
      background: #ffffff;
      border: 1px solid #d8e0e3;
      border-radius: 8px;
    }
  </style>
</head>
<body>
  <div class="page">
    <div class="header">
      <div class="brand-mark">H</div>
      <h1>HTMLKit Render Validation</h1>
      <div class="subtitle">Native N-API output · CMake/vcpkg build · 1080px canvas</div>
      <div class="clear"></div>
      <div class="tagline">
        <span class="pill">tables</span>
        <span class="pill">inline blocks</span>
        <span class="pill">borders</span>
        <span class="pill">rounded panels</span>
        <span class="pill">nested typography</span>
      </div>
    </div>

    <div class="section-title">Daily service snapshot</div>
    <table class="metrics">
      <tr>
        <td class="metric">
          <div class="metric-name">Render jobs</div>
          <div class="metric-value">18,426</div>
          <div class="metric-delta">+12.8% from previous run</div>
        </td>
        <td class="metric">
          <div class="metric-name">Median latency</div>
          <div class="metric-value">84 ms</div>
          <div class="metric-delta">-9 ms after cache warmup</div>
        </td>
        <td class="metric">
          <div class="metric-name">Image success</div>
          <div class="metric-value">99.4%</div>
          <div class="metric-delta">PNG/JPEG/WebP decode path</div>
        </td>
        <td class="metric">
          <div class="metric-name">Queue depth</div>
          <div class="metric-value">37</div>
          <div class="metric-delta">steady under target</div>
        </td>
      </tr>
    </table>

    <div class="section-title">Layout stress area</div>
    <table class="columns">
      <tr>
        <td class="panel" width="55%">
          <h2>Module coverage</h2>
          <div class="bar-row"><span class="bar-label">HTML parse</span><span class="bar-track"><span class="bar-fill" style="width: 286px;"></span></span></div>
          <div class="bar-row"><span class="bar-label">CSS cascade</span><span class="bar-track"><span class="bar-fill blue" style="width: 238px;"></span></span></div>
          <div class="bar-row"><span class="bar-label">Text layout</span><span class="bar-track"><span class="bar-fill" style="width: 260px;"></span></span></div>
          <div class="bar-row"><span class="bar-label">Images</span><span class="bar-track"><span class="bar-fill warn" style="width: 205px;"></span></span></div>
          <div class="bar-row"><span class="bar-label">Tables</span><span class="bar-track"><span class="bar-fill blue" style="width: 276px;"></span></span></div>
          <div class="note">This sample intentionally mixes nested tables, inline-block charts, floated branding, multiple backgrounds, borders, rounded corners, and dense text to exercise the native renderer beyond a hello-world case.</div>
        </td>
        <td class="panel" width="45%">
          <h2>Execution timeline</h2>
          <div class="timeline">
            <div class="event"><b>Load addon</b><br><span>Resolve build/Release/htmlkit.node through CommonJS.</span></div>
            <div class="event"><b>Initialize fonts</b><br><span>Call fontconfig setup before rendering text-heavy content.</span></div>
            <div class="event"><b>Render worker</b><br><span>Native async worker calls litehtml and cairo.</span></div>
            <div class="event"><b>Encode image</b><br><span>Return a PNG buffer to JavaScript and write it to disk.</span></div>
          </div>
        </td>
      </tr>
    </table>

    <div class="section-title">Recent output checks</div>
    <table class="data-table">
      <tr>
        <th>Case</th>
        <th>Surface</th>
        <th>Status</th>
        <th>Notes</th>
      </tr>
      <tr>
        <td>Typography stack</td>
        <td>Headings, labels, paragraphs</td>
        <td><span class="badge">PASS</span></td>
        <td>Mixed size and weight text rendered on the same page.</td>
      </tr>
      <tr>
        <td>Dashboard cards</td>
        <td>Table cells with rounded panels</td>
        <td><span class="badge">PASS</span></td>
        <td>Four equal metrics with nested content and background colors.</td>
      </tr>
      <tr>
        <td>Chart-like bars</td>
        <td>Inline-block width controls</td>
        <td><span class="badge">PASS</span></td>
        <td>Several variable-width filled tracks use CSS dimensions.</td>
      </tr>
      <tr>
        <td>External assets</td>
        <td>Callback-backed fetch</td>
        <td><span class="badge alert">SKIPPED</span></td>
        <td>This smoke image keeps assets inline to isolate layout rendering.</td>
      </tr>
    </table>

    <div class="footer">Generated by demo/render-complex.js using the local N-API binding. The output file is demo/output/complex-dashboard.png.</div>
  </div>
</body>
</html>`;

async function main() {
  htmlkit.initFontconfig();

  const image = await htmlkit.render(html, {
    width: 1160,
    imageFormat: 'png',
    fontName: 'Arial',
    defaultFontSize: 16,
    allowRefit: true,
  });

  const outDir = path.join(__dirname, 'output');
  await fs.mkdir(outDir, { recursive: true });
  await fs.writeFile(path.join(outDir, 'complex-dashboard.png'), image);
  console.log(path.join(outDir, 'complex-dashboard.png'));
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
