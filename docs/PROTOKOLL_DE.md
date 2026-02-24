# UDP-Protokoll Referenz (Ist-Zustand)

Diese Referenz beschreibt exakt, welche Pakettypen zwischen Client und Server ausgetauscht werden.

## Grundprinzip

- Transport: UDP (Boost.Asio, `udp::socket`)
- Paketbeginn: 1 Byte `id` (`char`)
- Serialisierung: rohe C++-Typbytes via `Buffer::write/read`
- Byte-Reihenfolge: Host-Endianness (kein Network Byte Order)
- Typgroessen: ABI-abhaengig (`int`, `bool`, `float`)

## Pakettypen

## 1) Work Request (Client -> Server)

ID: `0x00`

Payload-Felder:

- `char id` = `0x00`

Semantik:

- Client signalisiert: "Bitte gib mir den naechsten Render-Chunk".

Typische Groesse (Linux x86_64):

- `1` Byte

## 2) Work Response (Server -> Client)

ID: `0x00`

Payload-Felder:

- `char id` = `0x00`
- `bool has_work`
- falls `has_work == true`, folgen:
- `int x0`
- `int y0`
- `int chunk_height`
- `int width`
- `int height`
- `float zoom`
- `float min_x`
- `float min_y`
- `int max_iterations`
- `int anti_aliasing`

Semantik:

- `has_work == false`: keine Arbeit verfuegbar.
- `has_work == true`: komplette Parameter fuer einen horizontalen Chunk.

Typische Groesse mit Arbeit (Linux x86_64):

- `1 + 1 + (7 * 4) + (3 * 4) = 42` Bytes

## 3) Pixel Submit (Client -> Server)

ID: `0x01`

Payload-Felder:

- `char id` = `0x01`
- `int pixel_count`
- dann `pixel_count` mal:
- `int x`
- `int y`
- `float red`
- `float green`
- `float blue`

Semantik:

- Client uebertraegt berechnete Pixel eines Chunks.
- Farben liegen als Float-Werte im Bereich ca. `0..255` vor.

Typische Groesse:

- `1 + 4 + pixel_count * 20` Bytes

Beispiel:

- `pixel_count = 1200` -> `1 + 4 + 1200 * 20 = 24005` Bytes

## Verarbeitung

Server:

- `id == 0x00`: `request_work(...)` gibt den naechsten Chunk aus `to_generate` aus.
- `id == 0x01`: `handle_submit_work_packet(...)` liest Pixel und schreibt sie in `to_display`.

Client:

- `id == 0x00`: bei `has_work == true` startet `calc_chunk(...)`.
- Andere IDs: werden als "Unknown packet ID" geloggt.

## Wichtige Protokoll-Limits

1. Keine Sequenznummern.
2. Kein ACK/Retry.
3. Keine Versionierung des Paketformats.
4. Keine Integritaetspruefung (CRC/HMAC).
5. Keine Authentifizierung oder Verschluesselung.

Folge:

- Geeignet fuer lokale/trusted Umgebungen.
- Nicht robust fuer unzuverlaessige oder unsichere Netze.
