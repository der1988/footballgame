const { chromium } = require('playwright');
const assert = require('node:assert/strict');
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');

// Serve the actual deploy bundle; do not substitute a mock page or game.
const root = path.resolve(__dirname, '../dist');
const server = http.createServer((request, response) => {
  const name = new URL(request.url, 'http://localhost').pathname;
  const file = path.join(root, name === '/' ? 'index.html' : name);
  if (!file.startsWith(root + path.sep) || !fs.existsSync(file) || !fs.statSync(file).isFile()) {
    response.writeHead(404); response.end(); return;
  }
  const types = { '.wasm': 'application/wasm', '.js': 'application/javascript', '.html': 'text/html' };
  response.setHeader('Content-Type', types[path.extname(file)] || 'application/octet-stream');
  fs.createReadStream(file).pipe(response);
});

(async () => {
  let browser;
  try {
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
    browser = await chromium.launch({
      executablePath: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH || undefined,
      args: ['--enable-unsafe-swiftshader', '--use-angle=swiftshader']
    });
    const page = await browser.newPage({ viewport: { width: 1440, height: 1100 } });
    const errors = [], messages = [];
    page.on('pageerror', error => errors.push(error.message));
    page.on('console', message => {
      messages.push(message.text());
      if (message.type() === 'error') errors.push(message.text());
    });
    await page.goto(`http://127.0.0.1:${server.address().port}/`);
    await page.getByRole('button', { name: 'Avvia partita' }).click();
    await page.waitForFunction(() => document.getElementById('cover').hidden, { timeout: 60000 });
    await page.waitForFunction(() => window.Module && Module.calledRun);
    await page.waitForTimeout(1000);
    await page.locator('canvas').click();
    await page.keyboard.down('ArrowUp');
    await page.waitForTimeout(700);
    await page.keyboard.up('ArrowUp');
    for (const key of ['a', 's', 'd']) {
      await page.keyboard.down(key);
      await page.waitForTimeout(200);
      await page.keyboard.up(key);
      // Let the game's existing charge-lock cooldown expire before the next action.
      await page.waitForTimeout(1800);
    }
    await page.keyboard.press('q');
    await page.keyboard.press('F1'); // Disabled replay must not crash the reduced-memory build.
    await page.waitForTimeout(400);
    assert(messages.some(text => text.includes('gameplay initialisation complete')), 'Game did not start');
    assert(messages.some(text => text.includes('referee_')), 'Referee did not spawn');
    for (const control of [4, 5, 7]) {
      assert(messages.some(text => new RegExp(`control ${control} released`).test(text)),
        `Keyboard action ${control} did not reach the game input system`);
    }
    assert(!messages.some(text => /WARNING: SHADER|Failed to compile|Failed to link/.test(text)), 'Shader failure');
    assert.deepEqual(errors, [], 'Browser runtime errors');
    console.log('PASS: WebAssembly startup, WebGL shaders, referee, keyboard actions and disabled replay.');
  } finally {
    if (browser) await browser.close();
    await new Promise(resolve => server.close(resolve));
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
