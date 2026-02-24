# Code Map (Deutsch)

Diese Datei beschreibt den Code auf Funktions- und Datenstruktur-Ebene.

## 1. include/shared/buffer.hpp

## Typ: `enum class endian`

- Zweck: Plattform-Endianness abbilden.
- Hinweis: In der aktuellen Implementierung wird Endianness nicht wirklich transformiert.

## Klasse: `Buffer`

Interner Zustand:

- `_stream`: `std::deque<char>` als Byte-Puffer
- `_mutex`: Mutex fuer Zugriff auf `_stream`

Methoden:

- `size_t remaining()`
- Liefert Anzahl noch lesbarer Bytes.

- `void write_buffer(const char* buffer, size_t size)`
- Haengt `size` Bytes an `_stream` an.

- `void write_buffer(Buffer& buffer, size_t size)`
- Liest `size` Bytes aus anderem `Buffer` (destruktiv) und schreibt sie hier rein.

- `void peek_buffer(char* buffer, size_t size, bool reverse = false)`
- Kopiert Bytes ohne Entfernen.
- Parameter `reverse` wird nicht genutzt.

- `void read_buffer(char* buffer, size_t size, bool reverse = false)`
- Liest Bytes vorne aus `_stream` und entfernt sie.
- Parameter `reverse` wird nicht genutzt.

- `template<typename T> void write(T data)`
- Schreibt rohe Bytedarstellung von `T`.

- `template<typename T> T peek()`
- Liest `T` ohne Entfernen.

- `template<typename T> T read()`
- Liest `T` und entfernt die Bytes.

Wichtige Auswirkungen:

- Protokoll ist ABI-gekoppelt (`sizeof(int)`, `sizeof(bool)`, Endianness).

## 2. include/shared/udp.hpp

- `using udp = boost::asio::ip::udp`
- `enum class Ipv { V4, V6 }`

## 3. include/client/udp_client.hpp + src/client/udp_client.cpp

## Klasse: `udp_client`

Zweck:

- UDP-Kommunikation zum Server
- Async receive/send
- Callback in fachliche Clientlogik

Wichtige Felder:

- `_io_context`: Asio Eventloop
- `_socket`: UDP Socket, lokal an zufaelligem Port gebunden
- `_server`: aufgeloester Ziel-Endpoint (Host/Port)
- `_sender`: Endpoint des letzten eingehenden Pakets
- `_recv_buffer`, `_send_buffer`: statische Byte-Arrays
- `_to_send`: Queue aus ausgehenden `Buffer`-Objekten

Methoden:

- `udp_client(const std::string& host, const std::string& port, Ipv ipv)`
- Resolver fuer Zieladresse.
- Wirft `runtime_error`, wenn nicht aufloesbar.

- `set_read_callback(handle_packet_t callback)`
- Setzt Paket-Handler, wird auf jedem empfangenen Datagramm aufgerufen.

- `void receive()`
- Startet `async_receive_from`.
- Bei Paket: Bytes in `Buffer`, Callback aufrufen, dann erneut `receive()`.
- Bei Fehler: stoppt `io_context`.

- `void send()`
- Wenn Queue leer: postet sich selbst erneut.
- Wenn Daten da: nimmt Front, sendet per `async_send_to`, dann erneut `send()`.

- `void send(Buffer& buffer)`
- Kopiert Daten in neuen `Buffer` und queued ihn.

- `void start()`
- Aktiviert receive/send und blockiert in `_io_context.run()`.

- `void stop()`
- Stoppt Eventloop.

- `bool alive()`
- `true`, solange `_io_context` nicht gestoppt ist.

## 4. src/client/client.cpp

## Struct: `Pixel`

- Felder: `x, y, red, green, blue`
- Wird fuer Transfer und serverseitige Darstellung verwendet.

## Funktion: `create_request_work_packet(Buffer& buffer)`

- Schreibt Paket-ID `0x00`.
- Bedeutung: Work anfragen.

## Funktion: `send_pixels(Buffer& buffer, const std::vector<Pixel>& pixels)`

- Schreibt Paket-ID `0x01`.
- Schreibt Anzahl und dann jedes Pixel.

## Funktion: `inline double sqr(double v)`

- Kleiner Helfer fuer Quadrat.

## Funktion: `int calc_gray(float cx, float cy, int max_iter)`

- Mandelbrot-Escape-Iteration pro komplexem Punkt.
- Rueckgabe ist Iterationszahl bis Escape/Limit.

## Funktion: `int HSVToRGB(float h, float s, float v, float& r, float& g, float& b)`

- Manuelle HSV->RGB Umrechnung.
- Verwendet 6 Hue-Segmente und lineare Interpolation.

## Funktion: `calc_chunk(...)`

Parameter:

- Chunkregion: `x0, y0, chunk_height`
- Bild: `width, height`
- Kamera: `min_x, min_y, zoom`
- Render: `max_iterations, anti_aliasing`

Ablauf:

1. Iteriert Pixel des Chunks.
2. Berechnet ggf. Subpixel-Samples.
3. Ermittelt Hue aus Iterationszahlen.
4. Wandelt in RGB.
5. Schickt Pixel in 1200er Bloecken als `0x01`.

## Globale Variable: `std::mutex working`

- Dient als grober Zustand: "Client rechnet gerade".

## `main(int argc, char** argv)`

Ablauf:

1. Liest `host`,`port`.
2. Erstellt `udp_client`.
3. Setzt Read-Callback.

Callback-Verhalten:

- Erwartet Paket-ID.
- Bei `id == 0x00`:
- Liest `has_work`.
- Wenn `true`: lockt `working`, liest Chunkdaten, ruft `calc_chunk`.

Zusatzthread `work_thread`:

- Solange `client->alive()`:
- versucht `working.try_lock()`.
- bei Erfolg: sendet Work-Request (`0x00`) und unlockt.
- schlaeft 150 ms.

Danach:

- `client->start()` startet Asio-Eventloop.

## 5. include/server/udp_server.hpp + src/server/udp_server.cpp

## Klasse: `udp_server`

Zweck:

- UDP Socket am festen Server-Port
- Async receive/send fuer alle Clients

Wichtige Felder:

- `_io_context`, `_socket`, `_sender`
- `_recv_buffer`, `_send_buffer`
- `_to_send`: Queue aus `(Buffer, endpoint)`

Methoden:

- `udp_server(Ipv ipv, int port)`
- Bindet Socket auf Port.

- `set_read_callback(...)`
- Registriert fachlichen Paket-Handler.

- `receive()`
- Wie Clientseite: empfange Datagramm, in `Buffer`, Callback, recurse.

- `send()`
- Nimmt naechstes Queue-Element, sendet async, recurse.

- `send_to(Buffer& buffer, const udp::endpoint& endpoint)`
- Queueing fuer ausgehendes Paket an bestimmten Client.

- `start()` / `stop()`
- Start/Stop Eventloop.

## 6. src/server/server.cpp

## Struct: `ImageChunk`

Felder:

- `x0, y0`
- `chunk_height`
- `width, height`
- `zoom, min_x, min_y`
- `max_iterations, anti_aliasing`

## Struct: `Pixel`

- Entspricht Transfer-Pixel vom Client.

## Globale Daten

- `std::deque<ImageChunk> to_generate`
- Jobs, die noch nicht an Clients ausgegeben wurden.

- `std::queue<Pixel> to_display`
- Empfangene Pixel, die noch nicht gezeichnet sind.

- `std::mutex to_display_mutex`
- Schutz fuer `to_display`.

## Funktion: `handle_submit_work_packet(const std::unique_ptr<Buffer>& data)`

- Liest `int size`.
- Liest `size` Pixel aus Buffer.
- Schiebt Pixel in `to_display`.

## Funktion: `request_work(const std::shared_ptr<udp_server>& server, const udp::endpoint& endpoint)`

- Baut Antwortpaket `id=0x00`.
- Wenn kein Job: `has_work=false`.
- Sonst: `has_work=true` + naechster Chunk aus `to_generate`.
- Sendet an den anfragenden Client.

## Funktion: `SetPixel(...)`

- Schreibt RGB-Bytes direkt in SDL-Surface-Speicher.

## Funktion: `server_main(...)`

- Setzt Netzwerk-Callback:
- `id=0x00` -> `request_work`
- `id=0x01` -> `handle_submit_work_packet`
- Startet `server->start()`.

## `main(int argc, char** argv)`

Ablauf:

1. Liest `width,height,port`.
2. Startet Netzwerkthread.
3. Initialisiert SDL und Surface.
4. Event/Render-Loop.

Loop-Verhalten:

- Bei `need_to_redraw`: fuellt `to_generate` mit horizontalen Chunks (`chunk_height=10`).
- Wenn `to_display` nicht leer: Pixel setzen und `SDL_UpdateRect`.
- Tastatur steuert Kamera und Renderparameter.

Shutdown:

- SDL freigeben.
- UDP Server stoppen.
- Thread join.

## 7. Build/Tooling Dateien

- `CMakeLists.txt`: Zieldefinitionen, Abhaengigkeiten (`Boost::thread`, `Boost::chrono`, `SDL::SDL`)
- `Makefile`: `build`, `run-server`, `run-client`
- `CMakePresets.json`: `debug` und `release`
- `cmake.toml`: cmkr-Quelle fuer Generierung von `CMakeLists.txt`

## 8. Call-Graph (vereinfacht)

Server:

1. `main`
2. `server_main` (im Thread)
3. `udp_server::receive` -> callback
4. callback ruft `request_work` oder `handle_submit_work_packet`

Client:

1. `main`
2. `work_thread` sendet `create_request_work_packet`
3. `udp_client::receive` -> callback
4. callback ruft `calc_chunk`
5. `calc_chunk` ruft `calc_gray` und `HSVToRGB`
6. `calc_chunk` ruft `send_pixels` -> `udp_client::send`

## 9. Stellen mit erhoehter Vorsicht beim Refactoring

1. `Buffer`-Format aendern bricht Protokollkompatibilitaet.
2. `SetPixel` geht von 24bpp-RGB Layout aus.
3. `to_generate` und `_to_send` sind threadkritisch.
4. Timing im Work-Thread beeinflusst Lastverteilung stark.
