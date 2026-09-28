# WiFi Scale – ESP32-alapú vezeték nélküli mérleg és erőmérő

A projekt egy egyszerűen megépíthető, olcsó, Wi-Fi-képes digitális mérleget és dinamikus erőmérőt mutat be, amely elsősorban fizikaórai kísérletekhez készült.

Az eszköz saját Wi-Fi-hálózatot hoz létre, így internetkapcsolat és külön mobilalkalmazás nélkül használható. A mérési eredmények mobiltelefonon, tableten vagy számítógépen, közvetlenül a böngészőben jelennek meg.

## Hardver

- **Mikrovezérlő:** Wemos LOLIN32 Lite (ESP32)
- **Mérőerősítő:** HX711, gyors mintavételi üzemmódban
- **Mérőcella:** 1 kg-os nyúlásmérő bélyeges erőérzékelő
- **Mechanika:** plexilapokból kialakított mérőfelület

**HX711 csatlakoztatása:**

| HX711 | ESP32 |
|---|---|
| DOUT | GPIO22 |
| SCK | GPIO19 |

## A program főbb funkciói

- Közel 96 Hz-es tényleges mintavételi frekvencia.
- Tömegmérés és az erő kiszámítása newtonban.
- Tárázás a böngészőből.
- Valós idejű erő-idő grafikon, 10 másodperces gördülő időablakkal.
- Választható erőskálák: 0,5 / 1 / 5 / 10 N.
- Több kliens egyidejű kiszolgálása WebSocket-kommunikációval.
- Kliensenként független grafikonkezelés.

## Telepítés és használat

A program Arduino IDE környezetben készült. A fordításhoz az ESP32-kártyacsomag, valamint a **HX711** és a Markus Sattler-féle **WebSockets** könyvtár szükséges.

A főprogramot (`.ino`) és a böngészős kezelőfelületet (`webpage.h`) ugyanabba a projektmappába kell helyezni.

A program feltöltése után csatlakozzunk a mérleg saját Wi-Fi-hálózatához:

- **SSID:** MERLEG
- **Jelszó:** 12345678
- **Webes kezelőfelület:** http://192.168.4.1

A mérleg használat előtt tárázható, és szükség esetén ismert tömeggel újrakalibrálható.

## Oktatási alkalmazások

A mérleg alkalmas statikus erőmérésre, a tömeg és a súly kapcsolatának vizsgálatára, valamint lassabb dinamikus erőhatások tanulmányozására.

A gyors ütközések során mért erőcsúcsok pontosságát a mérőcella mechanikai tulajdonságai és a mintavételi rendszer korlátozzák.

**Fejlesztő:** Piláth Károly · 2026# Smart-scale
