# Supernova

Dies ist ein Universitäts Projekt.

Universität: Rheinische Friedrich-Wilhelms-Universität Bonn
Modul: BA-INF 105 - Einführung in die Computergrafik und Visualisierung
Semester: SS26

Die Idee ist [hier](./doc/idee/Supernova.excalidraw.png) skizziert. \
Der Projektbericht ist [hier](./doc/Bericht.md) zu finden.

## Dev environment mit devenv

> Läd alle Tools und Bibliotheken temporär herunter und erstellt temporär ein
> paar bash skripts. Außerdem werden noch git hooks hinzugefügt, die automatisch
> alles vor jedem Commit formattieren.

1. Programm [`devenv`](https://devenv.sh/getting-started/) installieren
2. `devenv shell` in diesem Ordner ausführen
    - Via [direnv](https://direnv.net/) oder [devenv selber](https://devenv.sh/auto-activation/) kann man diesen Befehl auch automatisch beim
    betreten dieses Ordners ausführen lassen.
3. Alle temporär installierten Bibliotheken und Tools benutzen
4. `exit` ausführen um alles wieder deinstallieren
    - bei direnv oder der auto activation von devenv wird das automatisch beim
    verlassen des Ordners getan

### Skripts

> devenv installiert zusätzlich noch ein paar Skripts

Kompiliere das Projekt:

```bash
build
```

Kompiliere und führe das Projekt aus

```bash
run
```

## Video Export

Rendert die komplette Kamerafahrt in eine Videodatei und beendet sich danach:

```bash
build && nixGLIntel ./build/supernova --export supernova.mp4
```

Ohne `--export` startet das Programm wie gehabt interaktiv, es wird also nichts
aufgenommen.

Das Ergebnis ist H.264 (libx264, CRF 18) in 3840x2160 bei 30 fps, mit dem
Soundtrack und allen Explosions-Effekten als AAC-Tonspur im selben mp4.

Ein paar Details, die für das Ergebnis wichtig sind:

- Gerendert wird nicht ins Fenster, sondern in einen Offscreen-Framebuffer in
  voller 2160p-Auflösung - das Fenster bleibt klein und die Auflösung ist
  unabhängig vom Monitor.
- Während des Exports läuft die Zeit nicht in Echtzeit, sondern in festen
  1/30-Sekunden-Schritten. Ein gerendertes Bild entspricht also exakt einem
  Bild im Video, egal wie lange die Grafikkarte dafür braucht.
- Der Raymarcher wird beim Export in Kacheln gezeichnet (Standard 256x256). Ein
  kompletter 4K-Frame in einem einzigen Draw-Call läuft auf der integrierten
  Intel-GPU so lange, dass der Treiber einen GPU-Hang meldet und den Kontext
  zurücksetzt (`i915 ... context reset due to GPU hang`) - das Ergebnis wäre
  ein komplett schwarzes Video. Nur die Scissor-Box wandert, der Viewport
  bleibt auf voller Auflösung, das Bild ist also identisch zu einem
  ungekachelten Render.
- Ein 4K-Frame dauert dadurch mehrere Sekunden; der Export der ~86 s langen
  Animation läuft mehrere Stunden. Das Fenster muss dabei offen bleiben (es
  hält den OpenGL-Kontext) und sollte nicht angeklickt werden - jeder
  Tastendruck löst laut `keyCallback` eine Explosion samt Sound aus, die dann
  mit im Video landet.

### Kachelgröße einstellen

Die richtige Kachelgröße hängt an der GPU, nicht am Projekt, und lässt sich
deshalb ohne Neukompilieren setzen:

```bash
./build/supernova --export supernova.mp4 --tile 1024
```

- Der Standardwert `256` ist auf die integrierte Intel-GPU ausgelegt.
- `--tile 0` schaltet das Kacheln ab und zeichnet jeden Frame in einem Zug.
- Auf einer dedizierten Grafikkarte ist ein deutlich größerer Wert (z.B. `1024`)
  sinnvoll: pro Kachel steht ein `glFinish()`, das bei ~3 s pro Frame nicht
  auffällt, bei ~0,25 s pro Frame aber spürbar Zeit kostet. Ganz abschalten
  würde ich es trotzdem nicht - Windows bricht mit seinem TDR-Watchdog jeden
  Draw-Call ab, der länger als 2 Sekunden läuft.

### Build unter Windows

FFmpeg wird über zwei Wege gesucht: zuerst per `pkg-config` (so liefert es
devenv unter Linux), sonst über das `FindFFMPEG`-Modul, das vcpkg mitbringt.
Unter Windows also:

```
vcpkg install ffmpeg
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
```

Zwei Punkte dazu:

- Der Windows-Pfad ist ungetestet - er wurde auf einem Linux-Rechner geschrieben.
- Die Fehlerbehandlung im Export besteht aus `assert()`. Ein Release-Build
  definiert `NDEBUG` und entfernt diese Prüfungen, ein fehlgeschlagener
  FFmpeg-Aufruf fällt dann nicht mehr auf. Für Exporte also lieber einen
  Debug-Build benutzen.
- Die Tonspur wird aus derselben Uhr gebaut: jeder Soundeffekt landet genau auf
  dem Zeitstempel des Frames, das ihn ausgelöst hat. Bild und Ton können
  dadurch nicht auseinanderlaufen. Die Live-Wiedergabe über die Lautsprecher
  ist während des Exports stummgeschaltet.

