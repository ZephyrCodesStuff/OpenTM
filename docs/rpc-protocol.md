# OpenTM server protocol

`opentm_server` holds console sessions so several clients can share them. The kit tracks protocol registration per TCP connection, so two processes cannot each open their own session to one console; anything wanting concurrent access goes through the server.

One server holds many sessions, so a single client can drive several consoles, and several clients can each drive any of them.

```
opentm_server --port 4300 [--local opentm] [--allow-mutating] [--quiet]
```

Listens on `127.0.0.1` by default. `--local` additionally opens a named pipe (Windows) or Unix domain socket, whichever the platform provides.

## Framing

One JSON object per line, both directions. No length prefix, so a socket tool or any language's standard library is enough.

**Request**

```json
{"id": 1, "method": "target.connect", "params": {"target": "decr"}}
```

`id` is echoed back and may be omitted for fire-and-forget. `params` is optional.

**Reply**

```json
{"id": 1, "ok": true,  "result": {...}}
{"id": 1, "ok": false, "error": "no open target named 'decr'"}
```

**Event** - unsolicited, no `id`, always tagged with its target:

```json
{"event": "tty", "target": "decr", "params": {"text": "..."}}
```

## Naming a target

`target.open` creates a session and returns its **handle**. Every subsequent call passes that handle as `target`:

```json
-> {"id":1,"method":"target.open","params":{"target":"decr","host":"10.0.0.5"}}
<- {"id":1,"ok":true,"result":{"target":"decr","name":"decr","host":"10.0.0.5","port":8530}}
-> {"id":2,"method":"target.connect","params":{"target":"decr"}}
```

Omitting `target` is allowed only when exactly one session is open; convenient for single-console scripting, an error otherwise rather than guess at which console to drive.

### Handles and names

A client that tracks its own identity passes `id`, and that becomes the handle; the display name travels separately as `target`/`name`. Clients that don't (the CLI, ad-hoc scripts) get the name as their handle.

The distinction matters because **the handle is stable and the name is not**. The GUI sends its stored target id, so renaming a target keeps driving the same session.

Resolution tries the handle first, then falls back to matching a display name, so `{"target":"decr"}` keeps working from a shell.

### Pushing a full record

`target.open` also takes a whole `record` object (the `target_record` JSON), which is how a front-end pushes edited properties into a live session:

```json
-> {"id":1,"method":"target.open","params":{"record":{
     "id":"6f1c7b2e-…","name":"decr","host":"10.0.0.5","port":8530,
     "type":"PS3_DEH_TCP","mac":"00:11:22:33:44:55",
     "reset_mode":3,"reset_boot_value":"81","reset_boot_mask":"17"}}}
```

The server drives resets, Wake on LAN and file serving from *its own* copy of the record, so anything omitted silently reverts to a default on the server side. That is what made a configured reset mode look ignored and Wake on LAN report no MAC when only the flat keys were sent. 64-bit reset values travel as strings because a `QJsonValue` only carries a double.

`target.open` defaults the name to `host:port` if none is given, defaults the port from `type` (`PS3_DEH_TCP` -> 8530, `PS3_DBG_DEX` -> 1000), and is idempotent: re-opening the same handle returns the existing session, updating its name and record on the way, which is how a restarted front-end re-attaches. A handle already bound to a *different* endpoint is an error.

`target.close` drops the session and disconnects it. Closing is explicit, a client disconnecting from the server does not end anyone's session.

## Requests acknowledge; data arrives as events

Anything that asks the *target* for data replies immediately with an acknowledgement, and the data follows as an event. That mirrors the underlying protocol, which is asynchronous throughout: a listing is a request on the wire and a reply frame some time later.

```json
-> {"id":7,"method":"fs.list","params":{"target":"decr","path":"/dev_hdd0/"}}
<- {"id":7,"ok":true,"result":{"requested":"/dev_hdd0/"}}
<- {"event":"directory","target":"decr","params":{"path":"/dev_hdd0/","entries":[...]}}
```

Correlate on `target` plus the event name, plus `pid` for the per-process queries.

## Power control does not need a session

`target.power_on`, `target.power_off`, `target.power_kill` and `target.reset` work on a target that has never been connected. The server brings up a control-only channel to the communications processor (DECR-1000 only), sends the verb and drops it again, which is what the vendor tool does - a console that is off never sees a debug session opened and closed on it.

The consequence for a client: after one of these the target is **not** connected, so a follow-up that needs a session (`fs.list`, `process.list`, ...) still has to `target.connect` first.

## Events

| event | when |
|---|---|
| `target_opened` / `target_closed` | a session appeared or went away |
| `target_renamed` | display name changed - `name`; the handle is unchanged |
| `server_shutdown` | the server is exiting - `reason`; sent untagged, before the socket closes |
| `conn_state` | socket state changed - `state` is the `tcp_connection` enum |
| `session_ready` | handshake complete; carries `token`, `sub_token` |
| `session_lost` | session reaped or connection dropped |
| `debug_agent_ready` | the kit's debug agent came up |
| `sdk_version` / `cp_version` | versions harvested during the handshake |
| `directory` | reply to `fs.list` - `path`, `entries[]` |
| `processes` | reply to `process.list` |
| `objects` | reply to any `process.*` drill-down - `kind`, `pid`, `items[]` |
| `tty` | console output as it arrives |
| `tty_stream` | the same, split by stream number |
| `clear_console` | a load asked for the console to be cleared |
| `load_reply` / `install_reply` | LV2 status of a load or install |
| `install_progress` / `install_finished` | percent, then the install path |
| `log` / `wire` | human-readable session log / frame log line |
| `status` | short status text, as the GUI status bar shows |
| `error` | something failed outside a request |

`objects.kind` is one of `threads`, `modules`, `mutexes`, `lwmutexes`, `conds`, `event_queues`, `containers`.

Events go to every connected client. Filter on `target` if you only care about one console.

## Methods

Call `server.methods` for the live list - each entry reports whether it mutates, whether it needs an open target, and whether that target needs a handshaked session. That is enough to build a tool manifest without hardcoding anything.

**Always available**

| method | notes |
|---|---|
| `server.methods` | introspection |
| `server.status` | target count, client count, mutation permission |
| `server.own` | claim ownership; with `--exit-with-owner` the server quits when this client disconnects |
| `target.open` | `host`, `[target\|name]`, `[port]`, `[type]`, `[serve_dir]` |
| `target.list` | every open session and its state |

**Need an open target**

| method | notes |
|---|---|
| `target.close` | |
| `target.status` | host/port/type/state, tty cursor |
| `target.connect` | |
| `target.disconnect` | |
| `target.serve_dir` | `dir` - the host directory exposed as `/app_home/` |
| `tty.read` | `since` -> `lines[]`, `cursor` |

**Need a session** (open *and* handshaked)

`fs.list`, `process.list`, `process.threads`, `process.modules`, `process.mutexes`, `process.lwmutexes`, `process.conds`, `process.event_queues`, `process.containers`.

The per-process calls take `pid`, accepted either as a number or as a `"0x01010400"` string.

**Mutating** - refused unless the server was started with `--allow-mutating`

`process.load`, `package.install`, `process.pause`, `process.resume`, `process.terminate`, `process.core_dump`, `target.reset`, `target.power_on`, `target.power_off`, `target.power_kill`, `target.wake_on_lan`, `settings.refresh`, `settings.apply`, `settings.commit`.

`target.reset` uses the target's configured reset mode; `mode` accepts
only `current`.

## Safety

Mutating verbs are off by default. This drives real hardware - a loop with a bug can power-cycle a console or reset it mid-write - so the permission is opt-in per server process rather than per request. `format_hdd` is deliberately not exposed at all.

Both listeners are local-only. The TCP listener binds to loopback, never `0.0.0.0`: this is a control channel for hardware and does not belong on a network.

## TTY

TTY is a stream, but the request/response surface would make a blocking tail hang a client. Lines are buffered per target (5000 max) and read by cursor:

```json
-> {"id":1,"method":"tty.read","params":{"target":"decr","since":0}}
<- {"id":1,"ok":true,"result":{"lines":["..."],"cursor":42}}
```

Pass the returned `cursor` as the next `since`. Clients that prefer push can ignore this and consume `tty` events instead.

## Who starts the server?

Normally `opentm_tray` does, and owns it: it starts the server with `--allow-mutating --exit-with-owner`, then calls `server.own` so being killed outright still takes the server down rather than leaving it holding consoles. Its control channel is a separate local socket (`opentm-supervisor-<user>`) taking one line of JSON per connection - `status`, `ensure_server`, `stop_server`, `open_gui`, `quit`.

`opentm_app --server 127.0.0.1:4300` (or `--server-local <name>`) points the window at a server you started yourself instead. `opentm_app --standalone` runs with no server at all, owning its sessions in-process.

## Testing

`tools/test/rpc_contract.py` exercises the surface without a console - introspection, both guards, argument validation, target resolution, the cursor. `tools/test/rpc_multiclient.py` covers session sharing between clients. Both start the server themselves; set `OPENTM_SERVER` to point at a specific binary.
