## How the API works

The API is [JSON-RPC 2.0](https://www.jsonrpc.org/specification) served over three transports:

- **WebSocket** - default port 9090; carries request/response and server-initiated notifications.
- **Raw TCP** - port 9090; a newline-free stream of JSON objects over a plain socket, carrying the same two.
- **HTTP** - POST the request envelope to `/jsonrpc` (default port 8080). Request/response only: notifications are never delivered over HTTP, so a client on this transport can only learn that something happened by asking again. Requires the 'Allow remote control via HTTP' setting to be enabled.

**Which to use.** Prefer a socket transport for anything that outlives a single call. Kodi announces what it is doing - playback starting, a library item changing, the skin finishing its load - and a client holding a socket open is told, while a client on HTTP has to poll for the same thing and will miss anything that starts and finishes between two polls. HTTP suits one-shot calls and environments where keeping a connection open is impractical. A client on HTTP that needs to know when the interface is up reads `ready` on `GUI.GetProperties`; the web server answering proves only that the JSON-RPC service is running.

Preferring a socket does not mean turning the web server off. It also serves the artwork and file endpoints (`/image/`, `/vfs/`) that several methods hand back URLs for, so a client doing its calls over a socket still fetches those over HTTP.

There is a single endpoint: the method is selected by the `method` member of the request envelope, not by the URL. Authentication over HTTP is basic auth when a username and password are configured. A permissions model gates the methods; each method page states the permission it requires.
