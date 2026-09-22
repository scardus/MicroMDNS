# MicroMDNS

A small, heap-free mDNS responder for the ESP8266. It makes your device
reachable as `http://<name>.local/` from phones and PCs on the same network,
and optionally advertises one DNS-SD (Bonjour) service such as a web server.

It is a replacement for the core's `ESP8266mDNS` (LEAmDNS) when all you need is
"answer to my name, advertise my web server".

## Why

LEAmDNS parses every incoming mDNS packet inside the WiFi stack's receive
callback and allocates heap for each record it reads. On a busy network that
fragments the heap and can starve the chip long enough to trip the hardware
watchdog.

MicroMDNS does all of its work from `mdnsLoop()` in your main loop, out of
fixed buffers, and handles at most four packets per call. It never touches the
heap.

| | |
|---|---|
| Static RAM | about 1.7 KB (512 B receive + 768 B transmit buffer, names, TXT) |
| Heap | none, apart from the one UDP socket |
| Code | about 5.7 KB |

## Installation

**PlatformIO** - add it to `lib_deps` in your `platformio.ini`:

```ini
lib_deps =
    scardus/MicroMDNS@^0.1.0
```

**Arduino IDE** - download this repository as a ZIP and use
*Sketch > Include Library > Add .ZIP Library...*

## Quick start

```cpp
#include <ESP8266WiFi.h>
#include <MicroMDNS.h>

void setup() {
  // ... connect to WiFi first ...
  String name = "mydevice-" + String(ESP.getChipId(), HEX);
  mdnsBegin(name.c_str());
  mdnsAddService("http", "tcp", 80);
}

void loop() {
  mdnsLoop();
}
```

See `examples/Basic` for a complete sketch with a web server.

## API

| Function | What it does |
|---|---|
| `bool mdnsBegin(const char* hostname)` | Claim `<hostname>.local` and start answering. Call once WiFi is connected. Calling it again with a new name withdraws the old one first. |
| `bool mdnsAddService(const char* service, const char* proto, uint16_t port)` | Advertise one service, e.g. `("http", "tcp", 80)`. Only one service is supported. |
| `bool mdnsAddTxt(const char* entry)` | Add a `key=value` entry to the service's TXT record. Call it again for more entries (64 bytes in all). |
| `void mdnsLoop()` | Send announcements and answer queries. Call it every pass of `loop()`. |
| `void mdnsEnd()` | Withdraw the name (goodbye packets) and stop. Call it just before a restart. Blocks for about 100 ms. |
| `void mdnsSetLogger(MdnsLogger logger)` | Receive one line of text per event, e.g. to print to Serial. Silent by default. |

Names follow the usual rules, and a call that breaks them returns `false`:

- **host name:** 1-32 letters, digits and hyphens, no hyphen at either end,
  with no `.local` on the end.
- **service name:** 1-15 letters, digits and hyphens, at least one letter,
  no hyphen at either end and no two in a row, with no leading underscore.
- **protocol:** `"tcp"` or `"udp"`.

## The name must be unique

MicroMDNS does **not** probe the network for a device already using the name
before claiming it. If two devices use the same name, both will answer and
clients will see whichever replied last. Adding the chip ID, as in the
examples, makes a clash practically impossible:

```cpp
String name = "mydevice-" + String(ESP.getChipId(), HEX);
```

## What it implements

- A, PTR, SRV and TXT records for one host name and one service, plus the
  `_services._dns-sd._udp` service-type listing and a reverse (`in-addr.arpa`)
  lookup.
- Three announcements at start-up, 1 s and then 2 s apart.
- NSEC records saying the device has no IPv6 address, so clients that ask for
  both IPv4 and IPv6 get a quick answer instead of waiting for a timeout.
- Answers to "legacy" one-shot queries, which repeat the question as ordinary
  DNS expects. This is how Android resolves `.local` names.
- Direct (unicast) answers when a query asks for one.
- No record multicast more than once a second.
- Goodbye packets from `mdnsEnd()`, sent twice because multicast over WiFi gets
  no retries.

## Limitations

These are deliberate, to keep the responder small and the main loop fast:

- **ESP8266 only.**
- **One host name and at most one service.**
- **No conflict probing** - see "The name must be unique" above.
- **No known-answer suppression.** It may repeat an answer a client already
  has. Reading the extra records needed for this is exactly what made LEAmDNS
  allocate memory for every packet.
- **No random 20-120 ms delay** before answers that other devices may also
  give. Answers are sent straight away.
- **All records go in the Answer section**, rather than splitting extras into
  the Additional section. Clients accept this.
- **Station (STA) interface only.** Nothing is answered on the soft-AP, such as
  a WiFiManager setup portal.
- **IPv4 only.**
- **No service discovery client** - it answers, it never asks.

## Running the tests

The packet parsing and building is unit tested on your PC - no board needed:

```
pio test -e native
```

`pio check -e nodemcuv2` runs static analysis. GitHub Actions runs both, and
builds the example, on every push.

## Licence

MIT - see `LICENSE`.
