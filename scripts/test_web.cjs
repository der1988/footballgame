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
    assert.equal(await page.getByRole('radio').count(), 5, 'Five pre-match formations must be available');
    await page.getByRole('button', { name: 'Avvia partita' }).click();
    await page.waitForFunction(() => document.getElementById('cover').hidden, { timeout: 60000 });
    await page.waitForFunction(() => window.Module && Module.calledRun);
    await page.waitForTimeout(2600);
    assert(!messages.some(text => text.includes('entering state: play')), 'Human kickoff fired automatically');
    await page.locator('canvas').click();
    await page.keyboard.down('ArrowUp');
    await page.waitForTimeout(700);
    await page.keyboard.up('ArrowUp');
    await page.keyboard.down('s');
    await page.waitForTimeout(1500);
    assert(!messages.some(text => text.includes('RESTART delivered manual=1')), 'Full power fired before button release');
    await page.keyboard.up('s');
    await page.waitForTimeout(400);
    assert(messages.some(text => text.includes('RESTART delivered manual=1 power=1.00')), 'Release did not deliver a fully charged kickoff');
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
    assert(messages.some(text => text.includes('RESTART kickoff')), 'Opening kickoff was not prepared');
    assert(messages.some(text => text.includes('entering state: play')), 'Kickoff did not resume play');
    assert(messages.some(text => text.includes('RESTART delivered manual=1')), 'Manual release did not deliver kickoff');
    for (const control of [4, 5, 7]) {
      assert(messages.some(text => new RegExp(`control ${control} released`).test(text)),
        `Keyboard action ${control} did not reach the game input system`);
    }
    assert(!messages.some(text => /WARNING: SHADER|Failed to compile|Failed to link/.test(text)), 'Shader failure');
    assert.deepEqual(errors, [], 'Browser runtime errors');
    for (const [code, name] of [['541', '5-4-1'], ['532', '5-3-2'], ['343', '3-4-3'], ['3232', '3-2-3-2']]) {
      await page.goto(`http://127.0.0.1:${server.address().port}/`);
      await page.locator(`input[name="formation"][value="${code}"]`).check();
      assert.equal(await page.locator('.formation:has(input:checked) circle[fill="#d5f589"]').count(), 10);
      const firstMessage = messages.length;
      await page.getByRole('button', { name: 'Avvia partita' }).click();
      await page.waitForFunction(() => document.getElementById('cover').hidden, { timeout: 60000 });
      await page.waitForTimeout(500);
      assert(messages.slice(firstMessage).some(text => text.includes(`FORMATIONS home=${name} away=4-4-2 outfield=10/10`)),
        `Selection ${name} did not reach the actual match setup`);
      assert((await page.locator('#game-status').innerText()).includes(name));
      assert.deepEqual(errors, [], 'Browser runtime errors after formation selection');
    }
    console.log('PASS: WebAssembly startup, shaders, referee, keyboard actions and all five formation selections reaching eleven-player matches.');
  } finally {
    if (browser) await browser.close();
    await new Promise(resolve => server.close(resolve));
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
