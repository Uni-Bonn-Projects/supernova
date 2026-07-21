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

