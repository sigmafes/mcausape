// The multiplayer backend for the web build: a WebSocket relay (RakNet has no
// UDP in a browser) plus a tiny lobby (there is no LAN broadcast either).
//
// The wire protocol here is not designed in this file -- it is reverse
// specified by the client that already ships in the game:
//   - handheld/src/network/WebRakNetInstance.cpp (the RakNet socket override)
//   - handheld/project/web/shell.html (window.mcpeNet / window.mcpeLobby)
//   - tools/wiretest/relay.mjs (the conformance test for exactly this contract)
// Changing any message shape here means updating those too, and re-running
// the wiretest against the deployed Worker before trusting it.
//
// One Durable Object instance ("global") is the whole lobby: every /relay
// WebSocket, every /announce and every /list call lands on the same object,
// which is what makes `connected` a single shared count and makes a route a
// single global address space.

const CORS_HEADERS = {
	"Access-Control-Allow-Origin": "*",
	"Access-Control-Allow-Methods": "GET, POST, OPTIONS",
	"Access-Control-Allow-Headers": "Content-Type",
};

function json(body, status = 200) {
	return new Response(JSON.stringify(body), {
		status,
		headers: { "Content-Type": "application/json", ...CORS_HEADERS },
	});
}

// How long an /announce is good for without a follow-up heartbeat. The page's
// own heartbeat is every 10s (shell.html HEARTBEAT_MS); this just has to be
// comfortably longer than that, not exact.
const PLAYER_TTL_MS = 30000;

// How many worlds may be online (listed and joinable) at once, and how many
// players one of them holds. Together they cap the whole lobby at 100 people.
// The world cap is enforced here; the player cap is the host's to enforce (it
// is the one that accepts the login) and is only reported back by it.
const MAX_WORLDS = 10;
const MAX_PLAYERS_PER_WORLD = 10;

export class Lobby {
	constructor(state, env) {
		this.state = state;

		// route -> WebSocket. Hibernation can evict this whole object between
		// messages -- the sockets themselves survive it (that is the point of
		// acceptWebSocket), but any plain field here does not, so this map has
		// to be rebuilt from what the runtime actually still holds every time
		// this constructor runs, not just the first time.
		this.sockets = new Map();
		for (const ws of state.getWebSockets()) {
			const attachment = ws.deserializeAttachment();
			if (attachment)
				this.sockets.set(attachment.route, ws);
		}

		// id -> { id, name, world, route, ts }. In-memory only, and lost on
		// hibernation same as above: the lobby list is a snapshot of who is
		// here now, not a record worth persisting. A wake caused by a message
		// on one socket means every other player's listing goes stale until
		// their own tab's next heartbeat -- acceptable, since the page already
		// polls /list every 3s and treats a gap as "nobody new", not an error.
		this.players = new Map();
	}

	async fetch(request) {
		const url = new URL(request.url);

		if (request.method === "OPTIONS")
			return new Response(null, { headers: CORS_HEADERS });

		if (url.pathname === "/relay")
			return this.handleRelay(request);

		if (url.pathname === "/announce" && request.method === "POST")
			return this.handleAnnounce(request);

		if (url.pathname === "/list")
			return this.handleList();

		if (url.pathname === "/")
			return this.handleStatus();

		return json({ error: "not found" }, 404);
	}

	handleRelay(request) {
		if (request.headers.get("Upgrade") !== "websocket")
			return json({ error: "expected websocket" }, 400);

		const pair = new WebSocketPair();
		const [client, server] = Object.values(pair);

		// Hibernatable: this object can be evicted between datagrams and still
		// wake up to deliver the next one, which is the common case for a lobby
		// that is mostly two players standing in a world doing nothing.
		this.state.acceptWebSocket(server);

		const route = this.allocateRoute();
		const token = crypto.randomUUID().replace(/-/g, "");
		// Travels with the socket through hibernation; this is the only place
		// that remembers which route and token belong to it.
		server.serializeAttachment({ route, token });
		this.sockets.set(route, server);

		server.send(JSON.stringify({ type: "hello", route, token }));

		return new Response(null, { status: 101, webSocket: client });
	}

	allocateRoute() {
		// The client does not keep a route as a bare number -- RakNet keys
		// connections by SystemAddress, so WebRakNetInstance.cpp's
		// addressFromRoute() packs it into the low 24 bits of a synthetic
		// 10.0.0.0/8 address (see that file for why: never 0.0.0.0 or a
		// broadcast address, and it reads back out byte for byte). A route
		// with anything set above bit 23 round-trips fine within the relay
		// here, but gets silently truncated the moment a joining client turns
		// it into a connection target, which sends it dialing a different,
		// almost certainly empty, route -- a connect that times out with no
		// error anywhere, because nothing was ever technically wrong, just
		// aimed at nobody. So routes here are drawn from the same 24-bit
		// space the client actually has room for, 1..0xFFFFFF, not the full
		// 32 bits the wire format's header could otherwise carry.
		let route;
		do {
			route = crypto.getRandomValues(new Uint32Array(1))[0] & 0x00ffffff;
		} while (route === 0 || this.sockets.has(route));
		return route;
	}

	// The Hibernation API's message/close/error handlers -- named exactly this
	// way, they are how the runtime wakes this object for a specific socket
	// instead of replaying every event on every wake.
	async webSocketMessage(ws, message) {
		// Only the relay datagram shape travels as binary from the client; any
		// stray text frame has nothing to mean here.
		if (typeof message === "string")
			return;

		const data = new Uint8Array(message);
		if (data.length < 4)
			return;

		const view = new DataView(data.buffer, data.byteOffset, data.length);
		const dest = view.getUint32(0, false);
		const target = this.sockets.get(dest);
		if (!target)
			return; // Addressed to nobody here anymore; drop, not fatal.

		const { route: srcRoute } = ws.deserializeAttachment();

		// Rewrite the header to the sender's own route before forwarding, so the
		// receiver's RakNet sees who it came from, not where it was going.
		const out = new Uint8Array(data.length);
		new DataView(out.buffer).setUint32(0, srcRoute, false);
		out.set(data.subarray(4), 4);
		target.send(out);
	}

	async webSocketClose(ws) {
		this.freeSocket(ws);
	}

	async webSocketError(ws) {
		this.freeSocket(ws);
	}

	freeSocket(ws) {
		const attachment = ws.deserializeAttachment();
		if (!attachment)
			return;

		this.sockets.delete(attachment.route);

		// A player's route dies with the socket; the lobby listing keeps the
		// player (world list still shows the name) but marks it unjoinable
		// until the next announce, if any, republishes a live route.
		for (const player of this.players.values())
			if (player.route === attachment.route)
				player.route = 0;
	}

	async handleAnnounce(request) {
		let body;
		try {
			body = await request.json();
		} catch (e) {
			return json({ error: "bad json" }, 400);
		}

		const { id, name, world, route, token, gone, whitelist, players, motd, deploy } = body;
		if (!id)
			return json({ error: "missing id" }, 400);

		if (gone) {
			this.players.delete(id);
			return json({ ok: true });
		}

		// A world that is already listed keeps its place; a new one needs room.
		const now = Date.now();
		const known = this.players.get(id);
		const alive = known && now - known.ts < PLAYER_TTL_MS;
		if (!alive) {
			let active = 0;
			for (const p of this.players.values())
				if (p.id !== id && now - p.ts < PLAYER_TTL_MS)
					active++;
			if (active >= MAX_WORLDS) {
				this.players.delete(id);
				return json({ ok: false, error: "limit", max: MAX_WORLDS }, 429);
			}
		}

		// The route is only published if its own socket vouches for the token --
		// otherwise anyone could claim someone else's route by guessing a number.
		const socket = this.sockets.get(route);
		const attachment = socket && socket.deserializeAttachment();
		const vouched = attachment && attachment.token === token;

		this.players.set(id, {
			id,
			name: name || "Player",
			world: world || "",
			route: vouched ? route >>> 0 : 0,
			whitelist: whitelist ? 1 : 0,
			players: Math.max(1, Math.min(MAX_PLAYERS_PER_WORLD, players | 0 || 1)),
			// The host's /motd (a line under the world) and its build, so a
			// client can tell a world it cannot join apart.
			motd: String(motd || "").replace(/[\t\n\r]/g, " ").slice(0, 64),
			deploy: deploy | 0,
			ts: Date.now(),
		});

		return json({ ok: true });
	}

	handleList() {
		const now = Date.now();
		const players = [...this.players.values()]
			.filter((p) => now - p.ts < PLAYER_TTL_MS)
			.map(({ id, name, world, route, whitelist, players, motd, deploy }) => ({ id, name, world, route, whitelist, players, motd, deploy }));
		return json({ players, maxWorlds: MAX_WORLDS, maxPlayers: MAX_PLAYERS_PER_WORLD });
	}

	handleStatus() {
		return json({
			connected: this.state.getWebSockets().length,
			carriesGameTraffic: true,
		});
	}
}

export default {
	async fetch(request, env) {
		const id = env.LOBBY.idFromName("global");
		const stub = env.LOBBY.get(id);
		return stub.fetch(request);
	},
};
