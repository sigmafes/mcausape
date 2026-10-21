# Hosting: commands, access and limits

Commands are typed in the chat (`T`). The host can always use them; a visitor
can once the host has made them an operator. Game commands (`/give`, `/summon`,
`/gamemode`) also need Allow Cheats on the world.

| Command | What it does |
|---|---|
| `/gamemode <s\|c> [player]` | Survival or creative, for yourself or someone else in the world |
| `/op <player>` / `/deop <player>` | Give or take away command permission |
| `/whitelist` | Turns the whitelist on or off. On, only listed names (and operators) can join, and the Join list shows `[Whitelist]` |
| `/whitelist add\|remove <player>` | Edit the list; `/whitelist list` shows it |
| `/auth enable\|disable` | Turns authentication on or off |
| `/register <password>` / `/login <password>` | For visitors, when authentication is on |
| `/list` | Who is in the world |

Hold `Tab` for the player list.

## Authentication

The world tells one player from another by name only, so with it on a name has
to be registered (`/register`) and every later visit has to `/login`. Until they
do, a visitor cannot break, place, attack, use items, open containers or chat, is
not hurt, and is sent out after a minute. The host is exempt. Passwords are kept
as a salted SHA-256, never in clear.

Operators and the whitelist are by name too, so use authentication if the world
is public.

## Where it is kept

In the world's own folder: `server.txt` (whitelist flag, auth flag, listed
names, operators) and `auth.txt` (accounts). An exported world takes them along.

## Limits

- Ten online worlds at once. The lobby (`cloudflare/lobby`) refuses an
  eleventh: it keeps running, it is just not listed.
- Ten players in a world, the host included. That makes a hundred at most.
- A name is 1-16 letters, digits, `_` or `-`, and unique in the world.

The lobby is a Worker: deploy it with `.github/workflows/lobby.yml` (the API
token needs *Workers Scripts: Edit* as well as *Pages: Edit*) or `npx wrangler
deploy` in `cloudflare/lobby`.
