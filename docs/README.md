# Dokumentation

Diese Dokumentation erklaert das Projekt aus zwei Perspektiven:

1. **Systemverstaendnis**: Wie Server und Clients zusammenarbeiten, wie Jobs verteilt werden und wo die Mandelbrot-Berechnung passiert.
2. **Codeverstaendnis**: Welche Datei welche Verantwortung hat und wie die Daten durch den Code laufen.

## Empfohlene Lesereihenfolge

1. `docs/DEEP_DIVE_DE.md`
2. `docs/PROTOKOLL_DE.md`
3. `docs/CODE_MAP_DE.md`
4. `README.md` (kompakter Einstieg, Build und Start)

## Zielgruppe

- Du willst das Projekt benutzen: lies zuerst `README.md`.
- Du willst den Code bis ins Detail verstehen: lies `DEEP_DIVE_DE.md`, `PROTOKOLL_DE.md` und `CODE_MAP_DE.md`.

## Scope

Die Doku beschreibt den **aktuellen Ist-Zustand** der Implementierung, inklusive wichtiger technischer Einschraenkungen (z. B. UDP-Verlust, rohe Binairserialisierung, Threading-Risiken).
