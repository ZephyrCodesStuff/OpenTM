# Internals

## Layers

| | |
|---|---|
| `tm_core` | protocol codecs and the socket. No session state, no UI. |
| `tm_session` | a session: handshake, dispatch, the controllers that own each feature |
| `tm_ui` | the Qt window, panels and dialogs |
| `tm_server` | exposes a session over line delimited JSON |
| `tm_launch` | single-instance handling and starting the tray/server |
| `tm_cli`, `tm_tray` | thin executables over the above |

`tm_core`: everything in it is a pure function over bytes, so it can be tested without hardware, and every wire format decoded lives there rather than being inlined at a call site. `deci3_codec` handles the outer envelope; `tsmp_codec`, `dfmp_codec`, `drfp_codec`, `dbgp_codec` and `dcmp_codec` the families inside it.

## The seam: `session_api`

`session_api` is an abstract console. Everything above it; the GUI, the CLI, the server - only ever talks to that interface, and there are two implementations:

```
session_api
 ├── target_session   owns a tcp_connection and drives a real console
 └── remote_session   forwards each call to opentm_server as JSON
```

That is why the GUI behaves identically whether it owns its sessions (`--standalone`) or shares them with a tray supervisor: `session_factory` picks the implementation, and nothing upstream knows which it got.

It also means a new capability has to be added in both, or the GUI works in one mode and not the other.

## How a frame gets to the screen

```
tcp_connection --> raw frames --> frame_dispatcher --> decoded --> controllers
                                                                │
                                                         session_api signals
                                                                │
                                                      panels / CLI / RPC events
``` 

`frame_dispatcher` decodes and fans out; it holds no state beyond what a decode needs. The controllers own a feature each:

| | |
|---|---|
| `session_controller` | handshake, session tokens, protocol registration |
| `target_actions` | power, reset, load, install, XMB settings |
| `file_explorer_controller` | directory listings and file operations |
| `kernel_explorer_controller` | processes and kernel objects |
| `host_file_server` | serves `/app_home/` to the console over DRFP |

Requests are asynchronous throughout, because the wire is: you send a request and a reply frame arrives some time later. Nothing blocks waiting for a console. A verb returns immediately and the answer comes back as a signal, which is also why the server protocol acknowledges a request and delivers the data as an event.

## Adding a verb end to end

Say the console gains an operation we want to expose. In order:

1. **`tm_core`** - a builder for the request body and a parser for the reply, plus a test in `tests/` that asserts against bytes from a capture rather than against your own encoder.
2. **A controller in `tm_session`** - send it, and turn the reply into a signal.
3. **`session_api`** - a virtual for the call, a signal for the completion.
4. **`target_session`** - forward to the controller.
5. **`remote_session` and `rpc_server`** - the JSON verb and the event, or the feature silently does nothing when the GUI runs against a server.
6. **The front ends** - a menu item or panel action, and a CLI flag.

## Tests

`tests/` is Catch2 over `tm_core` - codecs, path policy, the record format. These run without hardware and are the fast feedback loop.

`tools/test/rpc_contract.py` and `rpc_multiclient.py` start a real `opentm_server` and drive it, covering the JSON surface and multi-client behaviour. They need no console either: a target that never connects still exercises the verb table, the permission gate and the event fan-out.

Anything above that needs a devkit, and the protocol work is verified by captures plus hardware runs, not by unit tests.

## Things that are load-bearing and easy to break

**Protocol registration is per TCP connection.** A console gives its session to one connection at a time. Anything that opens a second connection to the same console will be refused, and the refusal is a status byte in a register reply that is easy to ignore - after which every request answers `NOPROTO` and the symptom is a hang. See [usage](usage.md#one-console-one-owner).

**Registrations survive a console reboot.** They belong to the connection, not to the LPAR, so re-registering after a reset earns "already registered" rather than fixing anything.

**The debug agent announces itself only when it starts.** Connect to a console that is already running and no announcement ever arrives, so anything that waits for one must also cope with never seeing it.

**Power control needs no session.** The communications processor answers with the console off, which is what makes power-on from cold work - and why the power verbs take a control-only channel that is dropped afterwards rather than opening a debug session on a console that is not up.
