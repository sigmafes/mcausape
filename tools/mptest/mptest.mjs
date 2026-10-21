// Two headless tabs, a host and a visitor, through the real relay: the checks
// in docs/multiplayer-plan.md (phase 0). Drives the game through the test hooks
// in handheld/src/main_sdl.h (mcpe_test_*), not through the menus.
//
//   node tools/mptest/mptest.mjs [url]
//
// The url defaults to the Cloudflare deployment. Prints one line per check and
// a JSON summary at the end; exits non-zero if a check failed.

import { createRequire } from 'module';
import { execSync } from 'child_process';

const require = createRequire(import.meta.url);
const globalRoot = execSync('npm root -g').toString().trim();
const { chromium } = require(globalRoot + '/playwright');

const URL = (process.argv[2] || 'https://mcausape.pages.dev/').replace(/\/?$/, '/') + '?touch=0';
const SEED = 1000 + Math.floor(Math.random() * 100000);
const GLOWSTONE = 89, WOOL = 35, STONE = 1, NETHER = 1, OVERWORLD = 0;

const results = {};
let failed = 0;
function check(name, ok, detail) {
	results[name] = { ok, detail };
	if (!ok) failed++;
	console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}  ${detail}`);
}
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function openTab(browser, label) {
	const context = await browser.newContext({ viewport: { width: 854, height: 480 } });
	const page = await context.newPage();
	page.on('console', (m) => { if (process.env.MPTEST_VERBOSE) console.log(`[${label}] ${m.text()}`); });
	page.on('pageerror', (e) => console.log(`[${label}] page error: ${e.message}`));
	await page.goto(URL);
	await page.waitForFunction(() => window.mcpeReady && Module._mcpe_test_state
		&& Module.UTF8ToString(Module._mcpe_test_state()).split(' ')[0] !== '-1', null, { timeout: 120000 });
	await sleep(2000);
	return page;
}

const call = (page, fn, ...args) => page.evaluate(([fn, args]) => {
	const r = Module['_' + fn](...args);
	return typeof r === 'number' && fn !== 'mcpe_test_set' && fn !== 'mcpe_test_send' && fn !== 'mcpe_test_summon' ? Module.UTF8ToString(r) : r;
}, [fn, args]);

async function state(page) {
	const s = (await call(page, 'mcpe_test_state')).split(' ');
	return { ready: s[0] === '1', client: s[1] === '1', dim: +s[2], x: +s[3], y: +s[4], z: +s[5], chunks: +s[6], players: +s[7] };
}
async function block(page, dim, x, y, z) {
	const r = await call(page, 'mcpe_test_block', dim, x, y, z);
	if (r === '-1') return null;
	const [id, data, bl, sl] = r.split(' ').map(Number);
	return { id, data, bl, sl };
}
async function waitFor(fn, timeoutMs, stepMs = 100) {
	const start = Date.now();
	while (Date.now() - start < timeoutMs) {
		const v = await fn();
		if (v) return { value: v, ms: Date.now() - start };
		await sleep(stepMs);
	}
	return { value: null, ms: timeoutMs };
}

const browser = await chromium.launch({
	headless: true,
	args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--autoplay-policy=no-user-gesture-required'],
});

try {
	// Host: a new world, and the edits a visitor must see when it arrives.
	const host = await openTab(browser, 'host');
	await call(host, 'mcpe_test_host', SEED, 0);
	const hostReady = await waitFor(async () => (await state(host)).ready, 180000, 250);
	check('host world ready', !!hostReady.value, `${hostReady.ms} ms`);
	const route = await host.waitForFunction(() => window.mcpeNet && window.mcpeNet.route(), null, { timeout: 30000 }).then((h) => h.jsonValue());
	await sleep(1000);

	const [sx, sy, sz] = (await call(host, 'mcpe_test_spawn')).split(' ').map(Number);
	const ow = { glow: [sx + 3, sy + 3, sz + 3], wool: [sx + 4, sy + 1, sz + 3], stone: [sx + 5, sy + 1, sz + 3] };
	await call(host, 'mcpe_test_set', OVERWORLD, ...ow.glow, GLOWSTONE, 0);
	await call(host, 'mcpe_test_set', OVERWORLD, ...ow.wool, WOOL, 5);
	await call(host, 'mcpe_test_set', OVERWORLD, ...ow.stone, STONE, 0);
	// The Nether is made by the first edit, written out, and dropped again
	// when nobody is in it: the visitor's trip has to read it back.
	const nx = Math.max(8, Math.min(500, sx)), nz = Math.max(8, Math.min(500, sz));
	const ne = { glow: [nx + 2, 70, nz + 2], stone: [nx + 3, 70, nz + 2] };
	await call(host, 'mcpe_test_set', NETHER, ...ne.glow, GLOWSTONE, 0);
	await call(host, 'mcpe_test_set', NETHER, ...ne.stone, STONE, 0);
	await sleep(1500);
	const hostGlowLight = await block(host, OVERWORLD, ow.glow[0], ow.glow[1] - 1, ow.glow[2]);

	// Visitor joins.
	const visitor = await openTab(browser, 'visitor');
	const t0 = Date.now();
	await call(visitor, 'mcpe_test_join', route);
	const moved = await waitFor(async () => { const s = await state(visitor); return s.ready && s.client; }, 300000, 250);
	check('visitor can play', !!moved.value, `${moved.ms} ms after join`);
	const allChunks = await waitFor(async () => (await state(visitor)).chunks >= 1024, 600000, 500);
	check('visitor has every chunk', !!allChunks.value, `${Date.now() - t0} ms after join`);
	await sleep(2000);

	const vGlow = await block(visitor, OVERWORLD, ...ow.glow);
	const vWool = await block(visitor, OVERWORLD, ...ow.wool);
	const vStone = await block(visitor, OVERWORLD, ...ow.stone);
	check('overworld edit (block)', vStone && vStone.id === STONE, JSON.stringify(vStone));
	check('overworld edit (data)', vWool && vWool.id === WOOL && vWool.data === 5, JSON.stringify(vWool));
	check('overworld edit (glowstone)', vGlow && vGlow.id === GLOWSTONE, JSON.stringify(vGlow));
	const vLight = await block(visitor, OVERWORLD, ow.glow[0], ow.glow[1] - 1, ow.glow[2]);
	check('overworld light under glowstone', vLight && hostGlowLight && vLight.bl === hostGlowLight.bl,
		`visitor ${vLight && vLight.bl} host ${hostGlowLight && hostGlowLight.bl}`);

	// Through to the Nether.
	const t1 = Date.now();
	await call(host, 'mcpe_test_send', NETHER);
	const inNether = await waitFor(async () => { const s = await state(visitor); return s.ready && s.dim === NETHER; }, 300000, 250);
	check('visitor in the Nether', !!inNether.value, `${inNether.ms} ms`);
	const netherChunks = await waitFor(async () => { const s = await state(visitor); return s.dim === NETHER && s.chunks >= 1024; }, 600000, 500);
	check('visitor has every Nether chunk', !!netherChunks.value, `${Date.now() - t1} ms after the trip`);
	await sleep(3000);
	const nGlow = await block(visitor, NETHER, ...ne.glow);
	const nStone = await block(visitor, NETHER, ...ne.stone);
	check('Nether edit (block)', nStone && nStone.id === STONE, JSON.stringify(nStone));
	check('Nether edit (glowstone)', nGlow && nGlow.id === GLOWSTONE, JSON.stringify(nGlow));
	const nLight = await block(visitor, NETHER, ne.glow[0], ne.glow[1] - 1, ne.glow[2]);
	const hostNLight = await block(host, NETHER, ne.glow[0], ne.glow[1] - 1, ne.glow[2]);
	check('Nether light under glowstone', nLight && nLight.bl >= 13,
		`visitor ${nLight && nLight.bl} host ${hostNLight && hostNLight.bl}`);

	// Back, then the delay of a live edit with a crowd of mobs about.
	await call(host, 'mcpe_test_send', OVERWORLD);
	const back = await waitFor(async () => { const s = await state(visitor); return s.ready && s.dim === OVERWORLD && s.chunks >= 1024; }, 600000, 500);
	check('visitor back in the Overworld', !!back.value, `${back.ms} ms`);
	const pigs = await call(host, 'mcpe_test_summon', 100);
	await sleep(8000);
	const live = [sx + 6, sy + 1, sz + 6];
	await call(host, 'mcpe_test_set', OVERWORLD, ...live, STONE, 0);
	const seen = await waitFor(async () => { const b = await block(visitor, OVERWORLD, ...live); return b && b.id === STONE; }, 120000, 50);
	check('live edit with mobs about', !!seen.value && seen.ms < 2000, `${seen.ms} ms with ${pigs} pigs`);
} finally {
	await browser.close();
}

console.log(JSON.stringify({ seed: SEED, failed, results }, null, 2));
process.exit(failed ? 1 : 0);
