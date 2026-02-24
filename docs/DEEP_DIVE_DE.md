# Mandelbrot Distributed Renderer - Deep Dive (Deutsch)

Diese Datei erklaert den Code im Detail: Architektur, Datenfluss, Threading, Mandelbrot-Mathematik, Paketformat und Dateirollen.

## 1. High-Level Architektur

Das Projekt hat zwei Binaries:

1. **Server** (`src/server/server.cpp` + `src/server/udp_server.cpp`)
2. **Client** (`src/client/client.cpp` + `src/client/udp_client.cpp`)

Rollen:

- **Server** erstellt Render-Jobs (Chunks), verteilt sie per UDP und zeichnet empfangene Pixel in ein SDL-Fenster.
- **Client** fragt Jobs an, berechnet Mandelbrot-Pixel und schickt Pixelpakete zurueck.

Die Mandelbrotmenge wird **nicht** auf dem Server berechnet, sondern im Client in:

- `calc_gray(...)`
- `calc_chunk(...)`

## 2. Datei-fuer-Datei Verantwortung

## Shared

- `include/shared/udp.hpp`
1. Alias `using udp = boost::asio::ip::udp`
2. Enum `Ipv` fuer IPv4/IPv6

- `include/shared/buffer.hpp`
1. Klasse `Buffer` als Byte-Queue (`std::deque<char>`)
2. Rohes Schreiben/Lesen arithmetischer Typen (`int`, `float`, `bool`, `char`)
3. Kein echtes Endianness-Handling trotz `endian` Enum

## Client

- `include/client/udp_client.hpp`
1. API fuer UDP-Client
2. Async receive/send Schleifen
3. Callback fuer Paketverarbeitung

- `src/client/udp_client.cpp`
1. Resolver (`host`,`port`) -> Server-Endpoint
2. `async_receive_from` fuer eingehende UDP-Pakete
3. `async_send_to` fuer Warteschlange `_to_send`

- `src/client/client.cpp`
1. Paketbau (`create_request_work_packet`, `send_pixels`)
2. Mandelbrot/Color-Berechnung (`calc_gray`, `HSVToRGB`, `calc_chunk`)
3. Callback fuer Work-Response
4. Zusatztread, der periodisch Arbeit anfragt

## Server

- `include/server/udp_server.hpp`
1. API fuer UDP-Server
2. Async receive/send Schleifen

- `src/server/udp_server.cpp`
1. Bind auf UDP-Port
2. Paketempfang + Callback
3. Paketversand aus `_to_send`

- `src/server/server.cpp`
1. Job-Queue `to_generate`
2. Pixel-Queue `to_display`
3. SDL Event-Loop und Darstellung
4. Netzwerk-Callback fuer IDs `0x00` und `0x01`

## Build

- `CMakeLists.txt`: Targets `server` und `client`
- `Makefile`: bequeme Befehle fuer Build/Run

## 3. End-to-End Datenfluss (Ablauf)

## Schritt A: Server startet

1. Startparameter: `width height port`
2. Startet UDP-Serverthread (`server_main`)
3. Initialisiert SDL-Fenster
4. Bei `need_to_redraw=true` zerlegt er das Bild in horizontale Chunks (`chunk_height=10`) und fuellt `to_generate`

Chunk-Inhalt:

- Startpixel `x0,y0`
- Bildgroesse `width,height`
- Kamera/Renderparameter `zoom,min_x,min_y,max_iterations,anti_aliasing`

## Schritt B: Client startet

1. Startparameter: `host port`
2. Startet UDP receive/send in `client->start()`
3. Startet zusaetzlichen Worker-Thread
4. Dieser sendet etwa alle 150 ms ein Work-Request Paket (ID `0x00`)

## Schritt C: Jobverteilung

1. Server bekommt `0x00` und ruft `request_work(...)`.
2. Wenn `to_generate` leer: Antwort `id=0x00`, `has_work=false`.
3. Sonst: Antwort `id=0x00`, `has_work=true` + Chunkdaten.

## Schritt D: Mandelbrotberechnung im Client

1. Callback liest Chunkdaten.
2. `calc_chunk(...)` berechnet Pixel fuer genau diesen horizontalen Bereich.
3. Pixel werden in Bloecken (`max_chunk=1200`) als `0x01` zurueckgesendet.

## Schritt E: Darstellung im Server

1. Server liest `0x01`, entpackt Pixel und legt sie in `to_display`.
2. SDL-Loop holt Pixel aus `to_display`, schreibt sie in Surface via `SetPixel(...)`.
3. `SDL_UpdateRect(...)` aktualisiert das Fenster.

Ergebnis:

- Das Bild entsteht inkrementell, sobald Clients Ergebnisse liefern.

## 4. Mandelbrot-Berechnung im Detail

Zentrale Funktion:

- `int calc_gray(float cx, float cy, int max_iter)`

Interpretation:

- Komplexe Zahl `c = cx + i*cy`
- Start: `z0 = 0`
- Iteration: `z_{n+1} = z_n^2 + c`

Implementierung:

- `x` und `y` repraesentieren Real/Imag-Teil von `z`.
- Update pro Schritt:
1. `xt = x*x - y*y + cx`
2. `yt = 2*x*y + cy`
3. `x = xt`, `y = yt`
4. `square = x*x + y*y`
5. `iter++`

Abbruch:

- wenn `square > max_square` oder `iter == max_iter`
- `max_square` ist hier `3.0` (klassisch waere oft `4.0`)

Rueckgabe:

- Anzahl Iterationen bis Escape oder Limit

## Pixel -> komplexe Ebene

In `calc_chunk(...)`:

1. `factor = zoom / min(width, height)`
2. Fuer jedes Pixel `(x,y)`:
1. `cx = min_x + (x + subpixel_x) * factor`
2. `cy = min_y + (y + subpixel_y) * factor`

Damit bilden `zoom`, `min_x`, `min_y` den sichtbaren Ausschnitt.

## Anti-Aliasing

`anti_aliasing` ist ein Radius:

- `0` -> 1 Sample
- `1` -> 3x3 = 9 Samples

Sampling erfolgt ueber Schleifen `dx,dy` von `-aa` bis `+aa`.
Der gemittelte Wert wird ueber `sqr(2*aa+1)` normiert.

## Farbmodell

1. Pro Sample wird `iter = calc_gray(...)` bestimmt.
2. Hue-Bildung:
- `hue += 360 * log(iter) / log(max_iterations)`
3. Mittelung ueber Samples
4. Wenn `hue > 359`, dann `value = 0` (schwarz)
5. `HSVToRGB(hue, 1.0, value, r, g, b)`

Wichtig:

- `HSVToRGB` ist eine manuelle Segment-Interpolation ueber 6 Hue-Bereiche.
- Rueckgabewerte sind Floats im Bereich um `0..255`.

## 5. Verteiltes Rendering (warum es skaliert)

Das System ist pull-basiert:

1. Client fragt aktiv Arbeit an.
2. Server gibt den naechsten Chunk aus einer Queue zurueck.
3. Mehr Clients bedeuten mehr parallele Chunk-Berechnung.

Vorteil:

- Einfaches Lastbalancing: schnelle Clients fragen haeufiger, langsame seltener.

Nachteil:

- Es gibt keine Wiederholung verlorener Chunks/Pixelpakete.

## 6. UDP- und Asio-Layer

## `udp_server` und `udp_client`

Beide Klassen haben dasselbe Muster:

1. `receive()` startet `async_receive_from`.
2. Nach Empfang wird Callback mit `Buffer` aufgerufen.
3. Danach ruft sich `receive()` erneut auf.

Und beim Senden:

1. `send_to`/`send` legt Daten in Queue.
2. `send()` nimmt Front-Element, sendet async.
3. Completion-Handler ruft erneut `send()`.

Damit laufen receive/send als dauerhafte Eventschleifen.

## 7. Shared Buffer im Detail

`Buffer` kapselt einen Byte-Stream als `std::deque<char>`.

Kernmethoden:

1. `write_buffer(const char*, size_t)`
2. `read_buffer(char*, size_t)`
3. `template<typename T> write(T)`
4. `template<typename T> read()`

Wichtige Eigenschaften:

- Speicherung ist roh und unverpackt.
- Keine explizite Protokollversion.
- Kein explizites Endianness-Mapping auf Netzwerkformat.

Folge:

- Praktisch fuer homogene Systeme, aber unportabel zwischen unterschiedlichen ABIs/Architekturen.

## 8. SDL-Darstellung

Pixel setzen erfolgt hier:

- `SetPixel(SDL_Surface* screen, short x, short y, unsigned char red, unsigned char green, unsigned char blue)`

Speicherzugriff:

- `pixel = screen->pixels + x*3 + y*screen->pitch`
- Dann drei Byte fuer RGB.

Serverloop:

1. nimmt alle Pixel aus `to_display`
2. schreibt auf Surface
3. `SDL_UpdateRect(screen, 0, 0, width, height)`

## 9. Eingaben und Neuberechnung

Server verarbeitet Tastatur:

1. Pfeile: Pan (`min_x`,`min_y`)
2. `PageUp`/`m`: zoom in
3. `PageDown`/`p`: zoom out
4. `+/-`: Iterationen rauf/runter
5. `Return`/`q`: Anti-Aliasing toggeln
6. `Esc`: Ende

Bei Aenderung:

- `need_to_redraw = true`
- Jobqueue wird neu aufgebaut

## 10. Threading und Synchronisation

### Server-Seite

Globale Daten:

- `to_generate` (`std::deque<ImageChunk>`) fuer Jobs
- `to_display` (`std::queue<Pixel>`) fuer Anzeige

Absicherung:

- `to_display` ist via `to_display_mutex` geschuetzt.
- `to_generate` wird aktuell **ohne Mutex** von Main-Thread und Netzwerkthread genutzt.

### Client-Seite

- `working` ist ein globaler Mutex.
- Worker-Thread sendet Work-Requests nur, wenn `working.try_lock()` erfolgreich ist.
- Callback lockt `working` waehrend der Chunkberechnung (`calc_chunk`).

Effekt:

- Solange gerechnet wird, sendet der Worker-Thread keine neuen Requests.
- Das begrenzt parallele lokale Chunkarbeit auf effektiv 1 Chunk pro Client.

## 11. Wichtige technische Einschraenkungen (Ist-Zustand)

1. UDP ist unzuverlaessig: Paketverlust moeglich.
2. Keine Chunk-Retry-Logik.
3. Keine Protokollversion oder Integritaetschecks.
4. Rohe Typserialisierung (`int/float/bool`) ist ABI-abhaengig.
5. Mehrere Queues werden ohne durchgaengige Thread-Sicherheit verwendet (`to_generate`, `_to_send`).
6. Viele Debug-Prints verursachen hohe Konsole-Last bei vielen Paketen.

## 12. Praktische Lesestrategie fuer den Code

Wenn du den Code komplett verstehen willst, lies in dieser Reihenfolge:

1. `src/server/server.cpp`
- Verstehe: Joberzeugung, Eventloop, Paketdispatch

2. `src/client/client.cpp`
- Verstehe: Work-Request, Work-Response, `calc_chunk`, `calc_gray`

3. `src/server/udp_server.cpp` und `src/client/udp_client.cpp`
- Verstehe: Asio Eventschleifen fuer receive/send

4. `include/shared/buffer.hpp`
- Verstehe: Serialisierung und ihre Grenzen

5. `CMakeLists.txt` und `Makefile`
- Verstehe: Targets, Abhaengigkeiten, Startkommandos

## 13. Kurze Begriffsklaerung

- **Chunk**: horizontaler Bildstreifen (`height = 10` Pixel)
- **Work Request**: Client fragt neuen Chunk an
- **Work Response**: Server sendet Chunkparameter
- **Pixel Submit**: Client sendet berechnete RGB-Werte
- **need_to_redraw**: Server soll alle Chunks neu erzeugen

## 14. Was "wo" passiert (Kurzantwort)

- **Mandelbrotmenge wird erstellt in**: `src/client/client.cpp` (`calc_gray`, `calc_chunk`)
- **Verteilung passiert in**: `src/server/server.cpp` (`to_generate`, `request_work`)
- **Empfang und Darstellung passieren in**: `src/server/server.cpp` (`handle_submit_work_packet`, `SetPixel`, SDL loop)

## 15. Ausfuehren und Beobachten

Server starten:

```bash
make run-server WIDTH=1280 HEIGHT=720 PORT=5000
```

Client starten (ein oder mehrfach):

```bash
make run-client HOST=127.0.0.1 PORT=5000
```

Beobachtung:

- Mit mehreren Clients sollte die Zeit bis zur vollen Bilddarstellung sinken.
- Bei Paketverlust koennen Bildbereiche spaeter oder gar nicht erscheinen.
