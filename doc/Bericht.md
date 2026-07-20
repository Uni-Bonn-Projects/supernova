# Projektbericht

## Florian

Ich hatte mit einer simplen, linearen Kamerafahrt begonnen. Danach habe ich einen Asset Manager erstellt, welcher alle Objekte verwaltet. Über diesen Asset Manager kann man bestimmen wann welche Objekte wo spawnen und despawnen. Die Kamerafahrten sahen mir aber zu fade aus. Deswegen setzte Keyframe-Interpolation(Spline) um, weswegen ich mithilfe von Catmull-Rom eine Methode erstellt habe, welche aus 4 Punkten eine Kurve machen. Danach habe ich keyframes eingeführt, welche die Position und Tatget der Kamera und deren Zeiten enthalten. Diese werden als Punkte auf der Kurve genutzt, um die gesamte Kamerafahrten zu einer smoothen Kurve zu machen. Die Kameraposition und -target besitzen ihre eigene Kurve. Mit der Funktion moveCamera gibt man an wohin sich die Kamera bewegen soll. Es startet bei unserer gewählten Startposition fightPos. Bei moveCamera kann man außerdem die Geschwindigkeit wählen, wie schnell sich die Kamera auf die angegebene Position und Target bewegen soll. 

So sah es in main.cpp umgesetzt aus:

![](image-3.png)

![alt text](image-4.png)

Damit konnte man zwar alles machen, aber um Szenen einfach zu bauen und die Reihenfolge der Szenen frei wählen zu können, haben wir in cinematic.cpp Szenen hinzugefügt. Mit diesen kann man einstellen, wie die Kamera sich verhält, welche Objekte spawnen und despawnen und auch die Zeiten genau einstellen für ein passendes Timing. Auch die Laser und Explosionen können in diesen festgelegt werden. Hier die erste Szene als Beispiel:

![alt text](image-5.png)

Nachdem dann alle Szenen erstellt sind kann man diese mit push_back in die Liste der durchlaufenden Szenen hinzufügen und somit das Video wie gewünscht aufbauen.

Als Filter hatten wir uns einen CRT Effekt ausgesucht, um dem Zuschauer glauben zu lassen, dass eine echte Kamera die Aufnahme macht. Für CRT Effekt suchte ich auf shadertoy.com  nach Inspiration. Meine erste Wahl war schwer umsetzbar, weil ich einen eigenen Buffer dafür zusätzlich erstellen musste. Nach weiterer Suche fand ich eine noch bessere Inspiration: Fast CRT von kbjwes77. 

![alt text](image-2.png)

Ich habe in unserer main.cpp Regler für den Warp und Scandes CRT Effekts eingefügt, sodass wir die Effekte testen und passende Werte finden können. Danach habe ich den Code der Internetseite auf unser Projekt angepasst und diesen in den Shader gepackt. Die Ränder wurden Schwarz und das Bild anhand der Distanz zur Mitte des Bildes gewarped, sodass die Ränder mehr gewölbt sind als die Mitte. Die Scanlines werden in einer Sinuskurve, die senkrecht verläuft, gewählt, sodass die horizontale immer die gleiche Farbe haben. Die Spitzen sind dabei dunkler. Der Wert Scan gibt an, wie dunkel diese Linien sind. Das Ergebnis sieht damit aus wie horizontale Scanlines einer alten Monitors.

![alt text](image-1.png)

Danach wollte ich Bewegungsunschärfe umsetzen. Doch unsere Objekte waren noch nicht fertig und bewegten sich noch nicht mit der Zeit. 

![alt text](image.png)

Außerdem lief unser Projekt bereits recht langsam, sodass Bewegungsunschärfe dieses Problem nur vergrößern würde. Deshalb hatte ich mich dafür entschlossen, dass ich stattdessen Fokusunschärfe mache, aber diese nur während den Explosionen aktiviere. Damit leiden unsere FPS nicht so sehr und gibt den Explosionen mehr Wucht. Für die Fokusunschärfe habe ich einen Offset im Shader erstellt, welcher von innen nach außen sich vergrößert. Danach habe ich jeden Pixel so oft wie unsere Sample Menge gerendert, wobei jeder durchlauf leicht verschoben war. Danach wurde der Durchschnitt der Farbe jedes Pixels gemacht, sodass die Pixel nicht n-mal so hell sind. Die Distanz der Fokusunschärfe ist immer die Distanz der Kamera zu dessen Target, sodass das Ziel immer im Fokus ist. Zum Schluss habe ich den Explosionen eine boolische Funktion angehangen, wenn es eine Explosion gibt und die Fokusunschärfe immer dann aktiviert. Nach jeder Explosion gehen die Werte der Unschärfe langsam und nicht auf einmal wieder runter.

## Dennis

Meine Themen waren:
1. Algorithmisch erzeugte Geometrie
2. Boolesche Geometrie
3. Partikelsysteme

### Der Anfang

Da ich als erstes Zeit gefunden hatte um mit dem Projekt anzufangen, habe ich
zuerst unsere ganze Devumgebung erstellt. Darunter zählt:
- das repository erstellen
- via [devenv](https://devenv.sh/) alle Abhängigkeiten und Skripts definieren,
damit ich und meine Gruppen alles gleich installiert haben
- den Code von Aufgabe 4a kopieren
- und die README erstellen, damit mein Team weiß wie man devenv benutzt

Als dann der erste, Florian, versucht hatte die Devumgebung zu benutzen, hatte
er einige Probleme mit OpenGL. Das lag daran, dass devenv auf Nix basiert, wo
dynamische Bibliotheken, wie OpenGL, ein wenig anders funktionieren. Da er (und
Noah) kein NixOS benutzt, mussten wir erst einmal recherchieren, wie man das
Problem behebt. Am Ende ging das jedoch recht leicht, indem man das Program mit
[nixGL](https://github.com/nix-community/nixgl) startet. Das ganze haben wir dann
im `build` und `run` Skript versteckt.

### Die ersten 2 Features

Der Grund warum ich den Code der 4a in dieses Projekt am Anfang kopiert hatte war,
dass ich dort schon Algorithmisch erzeugte Geometrie via Raymarching implementiert
hatte. Dadurch konnte ich Feature Nummer 1 abhaken. Zufälligerweise war dort
der Code fürs Shadow Mapping, was Noah eigentlich hätte implemtieren müssen,
schon vorgegeben gewesen.

Danach habe ich mich an die Boolesche Geometrie gesetzt. Anfangs dachte ich, dass
es relativ schwer werden würde zu implementieren, jedoch habe ich nach kurzer
Google suche folgende Webseite gefunden: <https://iquilezles.org/articles/distfunctions/>

Nachdem ich die folgenden 8 relevanten Zeilen kopiert hatte, konnte ich den
OLDMAN (das 200km große Raumschiff aus den Perry Rhodan büchern) recht leicht
zusammenbauen und rendern:

```glsl
float intersectSDF(float distA, float distB) {
  return max(distA, distB);
}
float unionSDF(float distA, float distB) {
  return min(distA, distB);
}
float differenceSDF(float distA, float distB) {
  return max(distA, -distB);
}
```

Damit ich auch sehen konnte, was ich überhaupt programmiere, habe ich auch noch
ganz schnell ein paar Controls für die Kamera hinzugefügt. (WASD Controls und
Hoch- und Runterfliegen)

Das Resultat zu diesem Zeitpunkt sah so aus:

![](./raymarching_oldman.png)

Rückblickend waren die 12 Sektionen nicht weit genug draußen. (Man kann hier
nur die ersten 25km von denen sehen)

Ab da hatte ich gemerkt, dass es ein paar Performance Probleme gibt. Um diese zu
mitigieren, habe ich zuerst alle 12 Sektionen aufeinmal rendern lassen, indem
ich eine Box 11 weitere mal im Kreis spiegele. Die Mathematik dafür verdanke ich
wieder einem Artikel von <https://iquilezles.org/>. Als zweites habe ich dann
alles um ein Faktor 1000 geschrumpft.

Bevor Florian und Noah dann richtig angefangen haben am Projekt zu arbeiten,
habe ich noch die Angreifer, einfache, algorithmisch erzeugte Kugeln, erstellt.

