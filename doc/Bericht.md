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

### Exkurs: Multifile Shaders

Irgendwann wurde der Code dann im Fragment Shader immer mehr, wodurch man die
Übersicht verlor. Daher habe ich versucht, den Shader in mehere Dateien via
`#include` Instruktionen aufzuteilen. Leider gibt es im Framework irgendeinen Bug,
wodurch beim verändern der Shaderdateien (kein C++ Code!) und Kompilieren des Projektes
entweder dutzende Makros fehlschlagen oder vergessen wird, dass die glm Bibliothek
heruntergeladen wurde. Der einzige Fix dafür war es den build Ordner zu löschen
und alles noch mal neu zu kompilieren.

Damit ich, und mein Team, das nicht jedes mal machen mussten habe ich meinen
eigenen kleinen Shader preprocessor geschrieben, der einfach nur die ganzen
`#include`'s auflöst und das Resultat an den Shader Compiler vom Framework
weitergibt.

### Himmelskörper und Hintergründe

Nach diesem kleinem Exkurs habe ich noch die Erde und den Mond erstellt (wieder
algorithmisch erzeugte Kugeln) und sowohl den Stern- als auch Linearraumhintergrund
(das bunte Wabern) programmiert.

Erde und Mond mit Sternenhintergrund:

![](./img/earth_moon_star_background.png)

Bei der Bewegung der Kamera flackern die Sterne ein bisschen, jedoch konnte ich
das nie fixen ohne ein statisches Bild in die Skybox zu laden.

Linearraumhintergrund. Hier leider nur ein statisches Bild davon:

![](./img/linear_space.png)

### Partikelsystem

Jetzt wo es nichts mehr gab, mit dem ich mich weiter drücken konnte, habe ich
an das Partikelsystem setzen müssen. Wie ich befürchtet hatte, war dieses System
nicht gerade leicht umsetzbar.

Zu allererst habe ich mich auf die Suche gemacht, wie man sowas normalerweise
implementiert. Folgender Artikel hat dabei meine Implemention sehr stark inspiriert:
<https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/>

Damit ich mich aber erstmal aufs wesentliche konzentrieren konnte, habe ich erst
eine minimale Version, ohne OLDMAN und was auch immer wir schon gemacht hatten,
erstellt. In dieser konnte ich dann, via Instanzierung und Offsets in einem
`GL_ARRAY_BUFFER` folgende Explosionen implementieren.

![](./img/early_explosions.png)

Dies ist aber nur ein Standbild von Partikeln, welche in der Mitte spawnen und
sich gleichmäßig nach außen ausbreiten, bis sie despawnen. Die Explosion ist
zudem auch gestreckt, was später in der Integrierung, ohne mein zutun, weging.

Wie man sich bestimmt denken kann habe ich dann diese minimale Version, in Form
der `Explosions` Klasse in die `MainApp` Klasse integriert. `Explosions` hatte
dabei ihr eigenes Shaderprogramm, Mesh und Daten natürlich. In Prinzip liefen ab
diesem Zeitpunkt zwei Programme, welche nacheinander zum gleichen Bildschirm
rendern.

Nachdem ich noch zufällige Geschwindigkeiten und eine Kasinomäßige
Überlebensberechnung der Partikel hinzugefügt hatte sahen die Explosionen wie
folgt aus:

![](./img/final_explosions.png)

## Noah Neiseke

Meine Themen waren: 
1. Audio
2. Shadow Mapping
3. Volumetrisches Licht

### Audio

Da mir schon fast klar war, dass es im Internet Modules gibt die Audio für GLSL Projekte implementiert habe ich mit dieser Aufgabe begonnen. Nach kurzer Suche bin ich auf [Miniaudio](https://miniaud.io/docs/manual/) gestoßen. Da es sich hier um eine Single-Header-File handelt und auch die Dokumentation mir das Gefühl gegeben hat, dass ich keine großen Probleme haben werde Audio einzubinden habe ich mich für Miniaudio entschieden. Direkt am ersten Tag war ich auch schon in der Lage einen Backgroundtrack hinzuzufügen, da der Code dafür sehr simple ist. Nach einfügen des Headers und einer kleinen Wrapper Implemenation als "AudioEngine" muss man nur 
```glsl
AudioEngine audio;
...
audio.init();
audio.playMusic("src/audio/Soundtrack.wav", 0.35f);
```
in main.cpp einfügen. `0.35f` ist hier die Lautstärke des Soundtracks. Der Soundtrack selbst kommt aus diesem Copyright Free [Youtube Video](https://www.youtube.com/watch?v=J0YZKSwe2qU). Runtergeladen hab ichs via meiner YouTube Premium Mitgliedschaft und dann einfach mit ffmpeg die Audio extrahiert. Da wir diesen nicht zu laut haben wollten hab ich es auf 35% gesetzt. Weitergehend wollten wir auch noch Explosionssounds haben. Hier habe ich leider keinen frei verfüglichen Effekt gefunden und musste daher recherchieren wie man sich selbst so einen Soundeffekt erstellen könnte. Ich kam also dazu irgendwie mit ffmpeg diesen Sound zusammenzubauen. Nach 10-15 unterschiedlichen Versuchen unterschiedliche Sounds zu produzieren kam leider kein befriedigendes Ergebnis zustande. Mir war aber zumindest klar, dass ffmpeg dafür reicht und Brown Noise ein guter Ansatz ist und somit auch meine Basis bilden sollte. Also suchte ich Hilfe bei KI-Tools. Nach einem kurzen Chat mit dem KI-Tool Claude wurde mir dieser Befehl vorgeschlagen:
```bash
ffmpeg -f lavfi -i "anoisesrc=d=1.4:color=brown:r=44100:a=1" -af "afade=t=out:st=0:d=1.4:curve=exp,lowpass=f=600" -ar 44100 -ac 2 src/audio/explosion.wav
```
Da ich sehr zufrieden mit diesem Ergebnis war habe ich es so übernommen. Danach war beim Thema Sound erstmal eine große Pause. Ohne die Szenen wo die Angreifer explodieren ist es natürlich nicht möglich die Sounds zeitlich abzustimmen. Zu Beginn dachte ich auch es wäre viel schwieriger den Sound auf die Explosion abzustimmen (wie soll ich wissen zu welchem t die Explosion passiert? Was wenn eine Änderung im Code die Zeit verändert?), doch unsere Implementation der Szenen mit Lambda-Funktionen hat es um einiges erleichter. Immer wenn das OneShotEvent für eine Explosion des Angreifers läuft habe ich an die Lambda-Funktion einfach noch den Soundeffekt angehängt. Damit ist der Sound fertig. 

### Shadow Mapping
Shadow Mapping hatte unterschiedliche Ansätze und Probleme. Zumal war es das erste Thema wo ich wirklich etwas "machen" musste (Audio war ja basierend auf einem bereits verbreiteten Modul) aber auch die Zusammenarbeit hatte hier kleine Störungen gebracht. Auch hier musste ich erstmal auf meine Teamkollegen warten: Ohne Old Man/Mond Objekte gibt es natürlich nichts was einen Schatten werfen kann, oder auf dem der Schatten landen könnte. Als diese Objekte dann vorlagen und Dennis auch bereits die ersten Rays implementiert hat lag auch ein Basic Schatten vor. ![](FirstMoonShadow.png)
Dies reichte meines Erachtens aber noch nicht daher fing ich an klassisches Shadow Mapping zu implementieren. Nach 1-2 Tagen rumprobieren und vielen nicht funktionierenden Prototypen habe ich mal gegoogled wie denn überhaupt Raytracing, Formelbasierende Geometrie aus dem Shader und Shadow Mapping zusammen funktionieren. Dort ist mir aufgefallen, dass für Shadow Mapping "echte" Geometrie über boolesche Geometrie benötigt wird. Da nur der Old Man mit boolescher Geometrie, also Dreiecksgeometrie, gebaut wird ist das also kaum möglich. Ich hätte sonst die Szene nochmal parallel komplett neu aufbauen müssen und als Meshes triangulisieren nur um diesen Shadow-Map-Pass zu ermöglichen. Ebenso hat die Skala unserer Szene auch ein Problem dargestellt. Da hier alles in Kilometern ist und z.B. zwischen dem Old Man und dem Mond 600km liegen wäre die Berechnung fast unmöglich. (und das ohne uns die Distanz von Sonne->Old Man->Mond anzuschauen!) Daher habe ich zu der Raytracing Alternative gegriffen: Shadow Rays. Von jedem mit Licht getroffenen Punkt schieße ich einen Ray zurück Richtung Lichtquelle. Erreicht er diese _**nicht**_ muss irgendwas den Ray abgefangen haben, ergo hier muss ein Schatten hin. Diese Intersection Logik funktioniert so:
_src/shaders/raytracing.glsl_
```glsl
// Input: pos-Punkt der gecheckt wird normal-Vektor Richtung Licht
bool inShadow(vec3 pos, vec3 normal) {
  vec3 shadowOrigin = pos + normal * max(1e-3, length(pos) * 1e-6);
  // nutzt das bereits etablierte Raytracing Protokoll
  RaytraceResult result = RaytraceResult(
      vec3(Inf),
      vec3(Inf),
      vec3(0.0),
      uFar,
      true
    );
  result = raytraceOldman(shadowOrigin, uLightDir, result);
  result = proceduralScene(shadowOrigin, uLightDir, result);
  // Wenn ein Ergebnis eine Intersection vor uFar gefunden hat wird True zurückgeben
  return result.distance < uFar;
}
```
```glsl
// Input: RaytraceResult 
vec3 calcLighting(RaytraceResult x) {
  vec3 intensity;
  if (x.glowing || uInLinearSpace) {
    intensity = vec3(1.0);
  } else {
    vec3 lighting = vec3(max(dot(x.normal, uLightDir), 0.0));
    // Überprüft durch inShadow ob etwas im Schatten liegen sollte
    float shadow = inShadow(x.hitPos, x.normal) ? 0.0 : 1.0;
    intensity = lighting * shadow;
  }
  // Berechnet die wirkliche Farbe die der Schatten haben sollte
  // Hier ist es eigentlich immer Schwarz, aber sicherheitshalber
  // ordentlich implementiert. ;)
  return intensity * x.objectColor;
}
```
Zuletzt verbindet rayTrace() diese Funktionen um ein finales Raytracing Produkt zu implementieren. Diese beiden Funktionen beschränken sich auch nicht nur auf den Old Man, sondern führen auch dazu, dass die Angreifer immer einen Schatten werfen. 

### Volumetrisches Licht
Vielleicht der schwerste Teil, da hier besonders Probleme in der Planung aufgetreten sind und auch die technischen Schwierigkeiten kein Ende genommen haben. In unserem Plan ging es ja um den "Big Blue Laser" der vom Old Man auf die Sonne geschossen wird, dieser sollte dann volumetrisches Licht ausstrahlen. Zu Beginn war mir schon klar es wird schwer, dass dieses Licht überhaupt sichtbar wird, da alle Quellen zu volumetrischen Licht immer als Beispiel Licht durch ein Fesnter oder durch eine Menge an Hochhäusern schien, und weil die Größe unserer Szene einzelne Lightrays viel zu kleine ausschauen lässt. Erste Versuche ließen auch zu wünschen übrig: ![](Stand3-7_Laser.png) Diese erste Version, von uns liebevoll die "Wurst" getauft, enstand dadurch dass wir nicht richtig kommuniziert haben wie dieser Laser letztendlich aussehen soll. Während ich an eine Art Laserschwert dachte, das eigentlich nur an den Ränden transparent ist, waren die anderen eher für durchsichtigere Strahlen. Da ich auch nicht wusste wie man es anders macht als ein festes Objekt zu kreieren welches dann Licht ausstrahlt kam die Wurst zustande. Andere Versuche führten dazu, dass die Wurst zwar weg war, aber ich dafür nurnoch eine unsichtbare Lichtquelle hatte. Dieses Licht konnte man aber kaum erkennen, da es erstes von der globalen Lichtquelle überstrahlt wurde und zweitens keine sichtbaren Rays produzierte, sondern nur Strahlen auf dem Old Man. Nachdem wir uns als Gruppe dann nochmal ausgetauscht haben musste ich im Prinzip meine ganze bisherige Arbeit scrappen und neu anfangen, jetzt war ein Lichtstrahl ohne feste Mitte gewollt. Dennis und andere Kommilitonen mit denen wir über dieses Projekt geredet haben gaben mir dafür auch direkt einen Tipp: Alpha-Blending.  
Als ich mich an Alpha-Blending probierte stoß ich direkt auf positiveres Feedback vom Rest der Gruppe, ich war zwar noch nicht die Wurst selbst los, aber immerhin sah man endlich das volumetrische Licht. ![](NichtFesterLaserv1.png) Nach weiterem Rumprobieren mit der Art wie ich den Laser weiter konstruieren will kam ich endlich zu diesem Ergebnis: 
![](NIchtFesterLasterv2.png)
Hier haben zwar viele Aspekte noch falsche Werte gehabt (GlowRadius, GlowIntensity und auch Länge des Lasers), aber da das eigentlich alles nur Umschreiben von Variablen war, bin ich trotzdem sehr zufrieden mit dem Ergebnis gewesen. 
Als letztes "neues" Feature für den Strahl habe ich noch probiert den Laser pulsieren zu lassen. Leider geling mir das garnicht. Alle Versuche dieses Pulsieren zu implementieren führten entweder zu keiner Änderung, oder dazu, dass der Strahl teilweise komplett verschwand oder sogar das Programm crashen lies. Das sehe ich auch noch immer als größtes Versagen des ganzen Projekts, da ich der Meinung bin, dass dieses Pulsieren noch einen riesen visuellen Unterschied gemacht hätte. 
Die Implementation befindet sich in den Funktionen _src/shaders/raytracing.glsl_, laserSegmentGlow() und laserGlow(). Während laserGlow im Prinzip nur ein Wrapper Funktion die nur überprüft ob der Laser überhaupt aktiv ist, hat laserSegmetGlow den wirklichen Arbeitsaufwand. 
Zuerst wird berechnet wo der nächste Punkt des Lasers ist um zu verhindern, dass der ganze Stral überprüft werden muss. (103-115) Darauf folgt die Auswahl des Raymarch-Fensters (117-120) und zuletzt wird der ganze Strahl abgetastet und die Farbe wird aufsummiert (122-138). Der senkrechte Abstand zur Laserachse gibt uns die Deckkraft des Lasers (Gauß-Abfall damit je weiter außen desto weicher) und die Farbe (innen weiß, außen blau). Diese Beiträge werden dann mit Front-To-Back Alpha Rendering aufaddiert. Zurückgegeben wird eine RGBA-Farbe die dann in main.cpp weiterverwendet wird.
Vorherige Versionen haben sich erstmal nur auf den großen Laser beschränkt und waren daher hardcoded mit blauer Farbe und auch LaserIntensity/Glow waren hardcoded. Da wird aber noch die roten Laser für die Angreifer und die grünen Laser für die Verteidigungsangriffe brauchten, hab ich lieber alles durch Parameter geregelt. Damit konnten wir den Laser für diese Angriffe wiederverwenden. 