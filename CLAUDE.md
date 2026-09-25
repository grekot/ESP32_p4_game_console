# CLAUDE.md - LakeMarioGame

Mini konsola do gier na **ESP32-P4** (płytka Guition JC4880P443C_I_W_Y: ekran 4,3" 480x800 MIPI-DSI,
dotyk GT911, ESP32-C6 jako radio) plus **emulator na Windows**, który uruchamia ten sam kod.
Pierwsza gra: platformówka „Lake Mario". UI konsoli w LVGL 9.5, gry na własnym rendererze 2D.

Kod i komentarze po polsku, **BEZ znaków diakrytycznych w plikach źródłowych** (kodowanie w toolchainie);
dokumentacja w `docs/`, README i ten plik z polskimi znakami.

## Stan na 2026-09-23

- **Płytka jeszcze nie dotarła** (zamówiona 22.09.2026, dostawa ok. 27.09-01.10). Cały kod powstał bez
  sprzętu. Po przyjściu płytki zacząć od listy „Do zweryfikowania na sprzęcie" w `docs/HARDWARE.md`.
- Firmware kompiluje się (24.09 rano, po przeróbce wizualnej Karta): **1 266 kB flash (30,2 % z 4 MB)**, 78,4 kB RAM
  statycznie (23.09 noc: 1 250 kB / 77,6 kB); siatki, bufor 14 000 trójkątów ekranowych, cache wierzchołków (100 kB, SRAM) i PNG nieba (~1,5 MB PSRAM) alokowane
  w czasie działania.
  Wcześniej: 1 208 kB / 54 kB (Labirynt + Kosmos), 1 186 kB / 46 kB (przed grami pokazowymi). Dekoder PNG i komponent spiffs to ~70 kB. Wzrost z 832 kB to czcionki Montserrat 24/32/40/48 (~280 kB) — jeśli flash zacznie brakować,
  wyłączyć 40 i 48 w `lv_conf.h` (używa ich tylko `console::text` w rozmiarze 4 i etykiety pada). Emulator kompiluje się i działa.
- Wszystkie mechaniki Lake Mario zweryfikowane skryptami w emulatorze (lista niżej). Bez testu skryptowego,
  tylko przegląd kodu: meta `F`, śmierć w przepaści, koniec czasu.
- **Platforma nauki C++ dla syna (11-13 lat, po Scratchu) — gotowa 23.09:** warstwa `src/console/`, 13 lekcji
  w `src/games/lekcje/` (00 szablon … 12 mini mario, 13 własny projekt) z README, `testy.txt` i rozwiązaniami; każda
  sprawdzona: kod startowy pada, rozwiązanie przechodzi. Regresja Mario (`tools/testy.ps1 mario`) 9/9 po refaktorze
  silnika (math2d, palette, ParticlePool, TileMap wycięte z `mario_game.cpp`: 641 -> 538 linii). Dokumentacja:
  `docs/NAUKA.md` (rodzic), `docs/DLA_UCZNIA.md` (uczeń). **Niesprawdzone:** `tools/setup_kid_pc.ps1` na czystej
  maszynie (tylko parser), gałąź `lekcje` nieutworzona, prace **niezacommitowane**.
- **Przejście na 800x480 (23.09 po południu, decyzja użytkownika „nowocześnie, nie retro"):** płótno natywne, LVGL bez
  schodków, nowy design menu/pauzy (`ui/theme.h`), wygładzony tekst w grach (`gfx/text.h`), Mario jako pixel-art x2 przez
  `canvas_scale()`, lekcje przeliczone; regresja Mario 9/9 (ślady identyczne, zrzuty 800x480 = x2 starych), 12 lekcji
  zweryfikowanych (start pada, rozwiązanie przechodzi). Emulator: okno 1:1, zrzuty 800x480 (1,15 MB BMP).
- **Gry pokazowe (23.09 wieczorem, na prośbę „przykład z fajną grafiką" + pytanie o 3D):** `src/games/labirynt3d/`
  (raycasting 400 kolumn, tekstury PNG 64x64, drzwi otwierane podejściem, monety/portal jako sprite'y z z-buforem,
  mini-mapa; płótno 400x240 x2) i `src/games/kosmos/` (800x480, PNG, 3 warstwy gwiazd, asteroidy rozpadające się,
  wybuchy 4 klatki PNG). Grafika z `python tools/gen_demo_assets.py` (deterministyczne PNG do `assets/`). Pełne 3D
  odrzucone (DECYZJE 23). Regresja 14/14 (`tests/scenarios.txt`: + `labirynt_route`, `labirynt_door`, `kosmos_play` x2).
  `--bench`: labirynt 0,62 ms/klatkę na PC vs 0,52 ms pusta gra; szacunek na P4 2-4 ms — **do zmierzenia na sprzęcie**.
- **Silnik 3D `src/gfx3d/` (23.09 noc, po ocenie użytkownika „słaba ta gra, czy ogranicza nas silnik?”):** software'owy
  rasteryzator low-poly na `gfx::Canvas` – `math3d.h` (Vec3, Mat4, `heading()`), `mesh` (siatka: kolor na trójkąt, bryły:
  box/klin/koło/walec/stożek/kula/dysk, `smooth` = Gouraud, `unlit`), `renderer` (kamera, światło kierunkowe, mgła, cull po
  normalnej, przycinanie z ≥ 1, sortowanie malarskie 1024 kubełków × 2 warstwy z FIFO, bez Z-bufora; `project()`,
  `horizon_y()`, statystyki). Bufory w PSRAM przez `platform::alloc_pixels`. Max 4096 wierzchołków na siatkę. DECYZJE 27.
- **Kart (23.09 noc; „wypasiona gra w stylu Mario Kart”, „żadnej pikselozy”, „coś, co da lepsze możliwości”):**
  `src/games/kart/` (kart_game.cpp logika 2D: tor Catmull-Rom, mapa nawierzchni 128x128, fizyka, AI z pasami i „gumą”,
  przedmioty, ranking; kart_render.cpp: scena 3D na gfx3d) – tor z wzniesieniami (256 przekrojów × 11 wierzchołków:
  4 pasy asfaltu, krawężniki, pobocza; siatka terenu 32x32 bez komórek pod drogą; wysokość = suma sinusów, tylko wizualna),
  gokarty jako modele (17 brył + kask Gouraud + 4 koła: obrót z prędkości, skręt przednich, przechył z `yaw_rate_` i terenu),
  drzewa/skrzynki (unlit)/przedmioty/flaga jako bryły, kamera za gokartem podążająca za terenem, niebo+chmury+góry (pasy PNG
  z alfą, `gfx::Image`/`load_png_rgba`), dym driftu, kurz, płomień turbo (`draw_image` z alfą przez `r3d_.project`).
  4 gokarty, 3 okrążenia, skrzynki → grzyb/banan/skorupa, drift + mini-turbo, pola przyspieszenia, wirowanie, mini-mapa,
  tabela na mecie; **Y = autopilot** (demo + testy). Jakość adaptacyjna (`quality_` 0/1/2: zasięg mgły i chmury, próg 13 ms).
  Na PC 2,7 ms/klatkę (~7 tys. trójkątów). Regresja: `kart_auto`, `kart_race`, `kart_player` (wzorce nagrane po zmianie
  mapy nawierzchni na komórkową). Nazwy w UI („Kart”, „Niebieski/Zielony/Zolty”) robocze – użytkownik nie zatwierdzał.
  `gen_kart_assets.py` (Pillow) generuje już tylko elementy 2D: ikony, chmury, góry, dym, poświatę, płomień.
- **Kart – przeróbka wizualna (24.09 rano, „wygląda słabo, ma być naprawdę atrakcyjnie”):** koniec z szachownicą asfaltu
  i trawy – `road_` i `terrain_` to siatki Gouraud z **kolorami wierzchołków** (`Mesh::vertex_colors`, `add_vertex(p, kolor)`):
  asfalt z szumem i jaśniejszą osią, trawa z koloru wysokości (doliny ciemne, wzniesienia jasne) + szum. Oznaczenia w osobnej
  płaskiej siatce `marks_` wypełniającej luki w `road_` (krawężniki czerwono-białe co 3 przekroje, linie boczne, przerywana
  oś, szachownica startu 8 kolumn, szewrony pól przyspieszenia). Statyczne obiekty w jednej siatce `props_` w współrzędnych
  świata: brama startowa (szachownicowy baner z obu stron, czerwona belka), trybuna z kolorową publicznością i dachem (przy
  przekrojach 0-18 po lewej; drzewa stamtąd usunięte), stosy opon (2 poziomy) po zewnętrznej ostrych zakrętów (`path_curvature`
  > 0,17), banery na prostych. Krzaki (`bushes_`, 40, dwa odcienie) za krawężnikiem, drzewa ze skalą losową i **cieniami**
  (dysk, `depth_bias` 60 w `draw_mesh` → zawsze po podłożu). **Bolid (druga iteracja po „bolidy wyglądają bardzo słabo”):**
  kadłub jako łańcuch brył ściętych o wspólnych przekrojach (ogon → kokpit → nos → szpic, `Mesh::add_loft`/`add_hexa`),
  pontony z czarnymi wlotami i białym pasem, lusterka, airbox z wlotem za głową, silnik, wydechy, płyta podłogowa i dyfuzor
  (karbon), tylne skrzydło z dwu płatów + białe płytki + dwa środkowe słupki, przednie skrzydło z klapą i płytkami,
  **wahacze** (widelce z przodu, drążki, wahacze i oś z tyłu – `add_rod`: pręt o kwadratowym przekroju między dwoma
  punktami, ogólny helper), kierowca: fotel, tułów-loft, barki, kark, ramiona-pręty do kierownicy, rękawice, kierownica
  ze środkiem i kolumną; kask 12×6 z wizjerem i pasem. **Koła z felgą**: `add_wheel(..., rim)` – bok koła = ściana boczna opony
  (jaśniejsza czerń) + **wklęsła** felga w ciemnym graficie z fasetami na przemian („szprychy”) + mały jasny kapsel na osi
  (pierwsza wersja z płaską jasnoszarą tarczą na pół boku wyglądała jak „bębny” – uwaga użytkownika), promienie 2,1/2,4,
  12 boków. 648+384 trójkątów
  na gokart (było 360+192). Światło zgodne z tarczą słońca (`SUN_ANGLE` 0,9 rad),
  niebo 3-progowe, cień gokarta szary. Kamera: FOV rozszerza się przy turbo (`fov_vis_`), na tytule kołysze się za polami
  startowymi. HUD: zaokrąglone panele z cieniem (`hud_panel`, `fill_round_rect_alpha`), medal miejsca, kolejność 4 gokartów
  (kropki), pasek prędkości z gradientem i km/h, etykieta DRIFT/MINI-TURBO, komunikat „OKRAZENIE n / OSTATNIE OKRAZENIE!”,
  odliczanie w pulsującym kole, tabela mety z czasami. Tryb oszczędny `Renderer::set_flat_only` przy `quality_ ≥ 1`
  (Gouraud → płasko), `quality_ ≥ 2` bez krzaków i cieni drzew. **PC: 2,4-3,4 ms/klatkę (było 2,05; rozrzut między
  uruchomieniami)**, ~14 tys. trójkątów w scenie przed odrzucaniem (droga 3040+2672, teren 930, obiekty 3024, gokart
  4×(648+384)); bufor renderera 18 000.
- **Kart – tekstury, cienie, odblask, ślady, Z-bufor (25.09 rano; użytkownik: „nadal wiele do życzenia, czy silnik nie
  da ładniej?” → ocena + plan A-E, „działaj po kolei od A do E”):**
  A. **Teksturowanie w gfx3d:** atlas 512×512 **8-bit z paletą** (`assets/kart/atlas.png` z `tools/gen_kart_atlas.py`,
  Pillow; indeks 0 = przezroczysty; `gfx::load_png_indexed` dekoduje lodepng do indeksów + paleta) i **colormapa**
  16 odcieni × 12 poziomów mgły × 256 (96 kB, `Renderer::set_texture`) – piksel = 2 odczyty (atlas, colormapa), bez
  mnożenia kolorów (jak w Quake). Rasteryzacja afiniczna z **korekcją perspektywy co 16 px** (u/z, v/z, 1/z liniowe),
  Gouraud = interpolowany wiersz colormapy, `alpha_test` (indeks 0) dla billboardów. `Tri` ma `u[3], v[3], tex`;
  `add_quad_uv/add_tri_uv/set_tri_uv`, `add_hexa/add_loft(..., top_uv)`, `add_wheel(..., tread_uv)`, `add_billboard`.
  Kart: asfalt (kafelek 128 na 64 jednostki w poprzek, 6 przekrojów wzdłuż), trawa (pobocze szczegółowa, dalsze pobocze
  i teren wygładzona – bez mipmap ostra tekstura migocze w oddali), komórki terenu z losowym obrotem UV i wyborem
  sucha/soczysta z wysokości, malowania bolidów 64×128 (numer, pasy, sponsorzy) na wierzchu kadłuba, bieżnik opon,
  publiczność trybuny, banery (KART/TURBO/LAKE) i baner bramy z tekstu (Arial Bold; fallback DejaVu/domyślna) – dwustronne
  `add_sign`. **Drzewa i krzaki = billboardy** (1 prostokąt obracany do kamery, `unlit + alpha_test`, 2 trójkąty zamiast
  ~100 – i wyglądają lepiej). Uwaga: `add_billboard` – prawo widza = `cross(-n, up)`; pierwsza wersja odbijała napisy.
  B. **Cienie rzutowane:** `Mat4::shadow_onto_plane(kierunek do słońca, y)` + `Renderer::draw_shadow(mesh, model)` →
  maska 1 B/piksel, po warstwie 0 piksele maski przyciemniane (×0,69, `darken565`), maska czyszczona tylko w bbox.
  Półprzezroczysty cień o kształcie bolidu (kadłub + 4 koła), bez podwójnego przyciemnienia nakładających się trójkątów.
  Słońce podniesione (`SUN_ELEV` 1,15 ≈ 50°), bo przy 33° cień był dłuższy od bolidu. Cienie drzew = dysk w masce.
  C. **Odblask:** `Mesh::specular` (wykładnik 16, półwektor L+V liczony raz na `draw_mesh`), kadłub `smooth` z normalnymi
  uśrednionymi w narożach (zaokrąglony lakier), kask 0,8, kadłub 0,38 (0,55 przepalało do bieli).
  D. **Ślady opon:** `update_skids` w logice (tylne koła przy drifcie/wirowaniu na asfalcie, segment co 1,5 jednostki,
  bufor 240 czworokątów, 30 s zanikania) rysowane `draw_tri(unlit, depth_bias 30)`. Nie wchodzą do `debug_line`.
  E. **Z-bufor 16-bit** (768 kB PSRAM, `enable_zbuffer`, 1/z jako 8.16 w spanie) przy `quality_ == 0`; sortowanie
  malarskie zostaje. Koszt na PC +0,3 ms. `KART_NOZ=1` (zmienna środowiskowa) wyłącza go do pomiaru.
  **Wynik: PC ~5 ms/klatkę (było 2,5)**; firmware 1 275 kB flash, 88,7 kB RAM statycznie (+10 kB: `KartGame` jest
  `static` w `registry.cpp`, ślady opon 9,6 kB). Na P4 koszt teksturowania NIEZMIERZONY – cel projektowy 30 FPS,
  awaryjnie `quality_` (płasko, bez Z-bufora, bez krzaków/cieni). **Testy:** adaptacyjna jakość mierzy zegar, więc pod
  obciążeniem CPU zrzut `kart_race` różnił się między uruchomieniami → `engine::set_deterministic()` (ustawiane przez
  `app::set_fixed_dt`) zamraża jakość w trybie `--frames`. Opis Karta w menu poprawiony („Wyscigi 3D…”, był „Mode 7”)
  → wzorzec `menu` nagrany na nowo. DECYZJE 29.
  **Bolidy „ginęły pod asfaltem” (zgłoszenie użytkownika po Z-buforze):** jezdnia jest wypłaszczona w poprzek (wysokość
  środka toru), a obiekty stały na `ground_height` swojego punktu – niżej od nawierzchni na przechyle terenu; malarz to
  maskował (warstwa 1 zawsze nad 0), Z-bufor uczciwie chował koła. Rozwiązanie: `surface_height(x, z)` (najbliższy odcinek
  linii środkowej, płasko do 38, przejście do terenu jak w siatce `road_`) dla bolidów (wysokość, pochylenie, przechył),
  przedmiotów, dymu, iskier, śladów (wysokość zapamiętana w `Skid::h`), kamery; drzewa i krzaki cachują wysokość przy
  budowie (`tree_h_`, `Bush::h`). Zasada: **co stoi na drodze, pyta o `surface_height`, nie `ground_height`.**
  To nie wystarczyło (użytkownik: „już na starcie koła wchodzą w asfalt”). Pomiar (tymczasowy log spodu koła vs wysokość
  siatki pod nim) pokazał trzy źródła: (1) przechył w zakrętach `yaw_rate_ * 0.09` (do 0,16 rad) obracał CAŁY bolid z kołami
  wokół punktu na ziemi → zewnętrzne koła 1,1 jednostki pod drogą; (2) `prev_angle_` niezerowane w `new_race` → skok kąta
  → przechył 0,16 przez ~1 s na starcie (i na tytule); (3) krzywizna terenu (`2·sin((x−2z)·0,027)`) daje ~0,13 jednostki
  ugięcia na pół rozstawu osi. Rozwiązanie w `draw_kart_3d`: **każde koło na `surface_height` swojego punktu**, nadwozie
  z pochyleniem i przechyłem z czterech kół (średnie przód/tył, lewo/prawo), przechył kosmetyczny (max 0,09) obraca tylko
  nadwozie wokół osi kół (y = 2,3); `prev_angle_`/`yaw_rate_` zerowane w `new_race`. Weryfikacja: wycinki kół z Z-buforem
  i bez (`KART_NOZ=1`) identyczne.
  **Bolidy były lustrzanym odbiciem (użytkownik: „przednie koła skręcają się przeciwnie”).** Rachunek na macierzach
  i rzut czubka koła na ekran mówiły „w prawo”, obraz mówił „w lewo”. Przyczyna: `Mat4::heading` (X = prawo, Y = góra,
  Z = przód) ma **wyznacznik −1** – to odbicie, nie obrót (prawoskrętna baza z Z do przodu ma X w lewo). Odbicie odwraca
  nawinięcie trójkątów w świecie, więc `cross(b−a, c−a)` wskazywało do środka bryły: cull odrzucał BLIŻSZE ściany, widać było
  dalsze = lustrzane odbicie modelu (koła „skręcone” odwrotnie, malowanie z numerem na masce w ogóle niewidoczne, bo górna
  ściana odrzucana). Malarz i brak Z-bufora maskowały to od początku (23.09). Naprawa w `Renderer::draw_mesh`: wyznacznik
  macierzy modelu < 0 → normalna geometryczna mnożona przez −1 (cull i oświetlenie płaskie); normalne gładkie
  (`apply_dir(V.n)`) bez zmian, bo izometria zachowuje „na zewnątrz”. **Zasada: każda macierz modelu z odbiciem jest OK
  dla renderera, ale nie budować na tym dalszych założeń o kolejności wierzchołków.**
  **Kolizje z obiektami przy torze (użytkownik: „można przejechać przez drzewo”):** `add_obstacle(x, y, r)` wołane przy
  budowie sceny (pnie drzew 2,6, stosy opon 2,7, słupy bramy 2,2, słupki banerów 1, trybuna jako 9 okręgów po 5),
  `obstacle_collisions` w fizyce po `kart_collisions`: wypchnięcie z okręgu `r + KART_RADIUS`, prędkość × (1 − 0,75·into),
  gdzie `into` = składowa kierunku jazdy w stronę przeszkody. Deterministyczne; ślady Karta bez zmian, bo AI nie zjeżdża
  z toru (sprawdzone regresją). Ślady `kart_auto`/`kart_player` bez zmian (fizyka
  nietknięta), wzorzec `kart_race` nagrany na nowo. DECYZJE 28.
- **Kart – 4 tory i motywy, krok 2 tekstur (25.09 wieczorem; „wprowadzaj krok 2, weź pod uwagę kilka różnych torów”):**
  `src/games/kart/kart_tracks.h`: `TRACKS[]` (16 punktów kontrolnych, pola przyspieszenia, skrzynki, motyw, `hills`/`phase`
  terenu) i `THEMES[]` (kolory nieba i mgły). Tory: **Jezioro** (tor 0 = pierwotny, na nim `kart_auto/race/player`),
  **Kanion** (pustynia), **Zimowa przełęcz** (śnieg), **Jesienny las**. Wybór LEWO/PRAWO na ekranie tytułowym
  (`select_track` → `build_track` + `build_track_scene`: atlas i panorama motywu wczytywane raz na motyw, `Mesh::init`
  używa ponownie pamięci), rekord okrążenia per tor w `kart_best` (tylko jazda ręczna, w `RECORD_KEYS`). Nowy tor:
  `python tools/kart_tracks_check.py --png podglad.png` (margines 90, odstęp odcinków ≥ 120, zakręt ≤ 1,9 rad/8 próbek
  jak tor 0, trybuna przy próbce 9). Tekstury z Gemini (nowy czat „Generowanie tekstury asfaltu…” – stary przestał
  generować: „I don't seem to have access”): `asphalt`, `ground_<motyw>`, `crowd`, `trees_sheet_<motyw>`,
  `mountains_<motyw>` w `assets_src/kart/`; `gen_kart_gemini.py` robi bezszwowe kafelki (przenikanie z kopią przesuniętą
  o pół kafelka, wycinek `crop` = większe elementy), wersję daleką (rozmycie z zawinięciem), `gen_kart_atlas.py` →
  `assets/kart/atlas_<motyw>.png` (paleta `quantize(kmeans=2)`); jesień: `sky_flood` (Gemini dorysował różowe obłoki).
  **Pomiar migotania** `python tools/kart_shimmer.py [--hold RIGHT 3 4]` (średnia różnica kolejnych klatek w pasie pod
  horyzontem): stara łąka 12,05 → nowa 10,62; kanion 7,85, zima 8,22, jesień 7,83. **Naprawiony błąd fizyki**
  (`kart_collisions`): kara ×0,92 co klatkę styku sklejała dwa gokarty jadące tym samym torem (~19 km/h, na torze 0
  dwaj rywale stali tak ~1000 klatek po starcie) – teraz kara tylko przy zbliżaniu, proporcjonalna do prędkości
  zbliżania; wzorce `kart_*` nagrane na nowo. PC 5,3/6,9/5,6/5,9 ms/klatkę; firmware 1 387 kB, 91,6 kB RAM; assets
  5,41 MB z 11,94. Regresja 33/33 (`kart_kanion` ślad+zrzut, `kart_zima`, `kart_jesien`, `kart_title_zima`).
  `tools/testy.ps1`: stderr emulatora nie przerywa już skryptu (PowerShell 5.1 + „Stop”).
- **Kart – grafika z Gemini, krok 1 (25.09 wieczorem; użytkownik: „czy generując w Gemini tekstury uatrakcyjnimy
  Karta?” → ocena → „zacznij od kroku 1”):** niebo i drzewa zamiast rysowanych skryptem. `assets_src/kart/`: arkusz drzew
  4x2 (dąb, sosna, palma, brzoza, krzak, krzak z kwiatami, klon, cyprys), dwie panoramy gór, pas chmur, ikony 3x2 (górny
  rząd: banan, skorupa, grzyb). `python tools/gen_kart_gemini.py` (po `gen_kart_assets.py`, przed `gen_kart_atlas.py`):
  góry 2048x160 = A B A B, chmury 1024x160 = jedna pętla; szwy łączone z **sumowaniem sylwetek** (alfa = max, zanik tylko
  w dalszej połowie szwu – zwykłe przenikanie dawało półprzezroczyste „duchy” szczytów), dół gór przechodzi w FOG_COL;
  chmury obcięte nad dorysowanymi przez Gemini wzgórzami i odfiltrowane z kolorowych pikseli. Drzewa w atlasie 8-bit
  (alpha-test, próg 128): `gen_kart_atlas.py` bierze `atlas_tiles()` z Gemini, gdy jest arkusz; **4 rodzaje drzew**
  (nowe kafelki brzoza 256,256 i klon 384,256; `T_TREE[4]`, `TREE_SIZE`), rodzaj = `kind` z logiki (0/1, bez zmian –
  ślady) + para z `noise01(i, 13)`. Wycinanie z magenty: `key_magenta(im, holes=True)` usuwa też zamkniętą magentę
  (szczeliny w koronach). Ślady Karta bez zmian, nowy wzorzec `kart_race`; PC 4,9-5,05 ms/klatkę (jak przed). Krok 2
  (trawa, asfalt, publiczność: bezszwowość, wspólna paleta, wersje bliska/daleka, migotanie) – do zrobienia.
- **Menu konsoli „KOTARBA / GAME CONSOLE” (25.09 po południu, użytkownik: „menu mało atrakcyjne, trzeba też opcji
  konfiguracji”; wybrał wariant A = karuzela okładek; nazwa od „Kotarba Game Console” – do ewentualnej zmiany,
  stałe `theme::BRAND_NAME/BRAND_SUB`):** `src/ui/menu.cpp` przepisane – pasek z logo i 4 zakładkami (Gry, Lekcje,
  Ustawienia, O konsoli); Gry = karuzela okładek 440x248 (boczne ×0,62 przez `lv_image_set_scale`, stała ramka
  zaznaczenia na środku, tło = okładka rozmyta w kodzie i przyciemniona, przenikanie dwóch warstw), Lekcje = lista +
  podgląd (okładka generowana: kolor z nazwy + numer z opisu „Lekcja NN:”), Ustawienia (`app/settings`: jasność PWM,
  wygaszanie ekranu, podpowiedzi dotykowe auto/zawsze/nigdy, licznik FPS, kolor akcentu, wyczyść rekordy, domyślne),
  O konsoli (wersja, pamięć, czas pracy, test przycisków), ekran startowy z logo. Klawisze obsługuje `menu::update()`
  (zbocza liczone w menu), nie grupa LVGL; X/Y = następna/poprzednia zakładka, GÓRA = pasek zakładek. Zapis trwały:
  `platform::load_blob/save_blob/erase_blob` (NVS „console” / `sim/build/save_<key>.bin`) przez `engine::load_data/
  save_data` – **wyłączone w trybie deterministycznym**, więc testy startują z ustawień domyślnych; `RECORD_KEYS` w
  `engine/storage.cpp` (Snake zapisuje `snake_top`). Polskie litery: `tools/gen_pl_fonts.py` → `src/ui/fonts/
  console_fonts_pl.c` (18 znaków Montserrat Medium 12-48 px z `fallback` na wbudowane czcionki – ASCII bez zmian,
  zrzuty starych testów identyczne); w kodzie jako `\uXXXX`. Okładki: Gemini → `assets_src/covers/*.jpg` →
  `python tools/gen_covers.py` → `assets/covers/<id>.png` (Snake z ilustracji tytułowej); `GameEntry` ma pola `cover`
  i `lesson` (lekcje przez `CONSOLE_ADD_GAME`). LVGL w trybie `--frames` ma **zegar wirtualny** (1/60 s na `ui::tick`) –
  inaczej odświeżanie co 16 ms nie nadążało i zrzuty pokazywały starą klatkę. Firmware 1 384 kB flash, 98,7 kB RAM;
  partycja assets 4,55 MB z 11,94. Regresja 28/28 (`menu_carousel`, `menu_lessons`, `menu_settings`, `menu_about`,
  `menu_nav`). **Na sprzęcie niesprawdzone:** PWM podświetlenia (GPIO23 może nie znosić PWM), płynność karuzeli
  (skalowanie okładek programowo), czas dekodowania 5 okładek przy starcie, NVS.
- **Snake (25.09, prośba „nowa gra snake na ładnych grafikach z mojego Gemini”; lekcja 07 `waz` zostaje):** `src/games/snake/`
  (snake_game.cpp: 10 plansz ASCII 25x14 w 5 światach, ruch po kratkach z kolejką 3 skrętów, przedmioty, combo, portal,
  tryb Bez końca, autopilot BFS; snake_render.cpp: tło wypalane raz na poziom do `bake_`, ciało z kulek generowanych w kodzie
  + obrys + cień przez maskę, głowa z Gemini obracana, HUD, ekrany). Grafika: Gemini w **Chrome użytkownika** (Claude in
  Chrome, gemini.google.com, konto z subskrypcją; wbudowana przeglądarka nie jest zalogowana) – arkusze 4x3 na magencie,
  tła, ilustracja tytułowa; surowe JPG w `assets_src/snake/` (19 MB, **nie** idą na SPIFFS), obróbka
  `python tools/gen_snake_assets.py` → `assets/snake/` (2,8 MB). Pobieranie z Gemini: przycisk „Pobierz obraz w pełnym
  rozmiarze” (widoczny po najechaniu na obraz) → `~/Downloads/Gemini_Generated_Image_*.jpg`; gdy kolejne pobrania nie
  przychodzą, pomaga odświeżenie strony. Sterowanie: krzyżak albo „2 przyciski” (wiersz Ruch na tytule, `two_buttons_`: Lewo/Prawo = skręt względem głowy, tylko zbocza), A = turbo, X na tytule = poziom startowy, Y = autopilot (w autopilocie
  poziomy przechodzą same). PC 0,84 ms/klatkę; firmware 1 317 kB flash, 97,2 kB RAM statycznie. Rekordy tylko w RAM.
  Regresja: `snake_auto`, `snake_title`, `snake_desert`, `snake_gameover`, `snake_player`, `snake_twobtn` (23/23 z menu nagranym na nowo).
  DECYZJE 30. **Nieoceniona przez użytkownika, na sprzęcie niesprawdzona** (koszt dekodowania tła PNG przy starcie poziomu).
- **Pacman (25.09 wieczorem; użytkownik: „gra packman”, grafika z jego Gemini, równolegle z drugim agentem przy Karcie –
  praca w osobnym worktree gita `../LakeMarioGame_pacman`, gałąź `pacman`):** `src/games/pacman/` (pacman_game.cpp: ruch po
  kratkach z decyzją raz na kratkę, skręt „przed czasem” 6 px, zawrócenie natychmiast, 4 duchy z celami i harmonogramem
  rozproszenie/pościg jak w oryginale, strach z łańcuchem 200…1600, oczy wracają do domu, owoce po 70/170 kulkach, dodatkowe
  życie za 10 000, autopilot BFS; pacman_render.cpp: tło z Gemini + ściany z pola odległości od korytarza wypalane raz na
  poziom, sprite'y PNG z alfa, panel; pacman_mazes.h: 4 plansze 27×19, „Klasyk” ręcznie + 3 z `tools/pacman_maze_gen.py`,
  sprawdzane `tools/pacman_maze_check.py`). 4 światy (neon, cukierki, dżungla, lawa). Grafika: `assets_src/pacman/`
  (5 obrazów Gemini) → `python tools/gen_pacman_assets.py` → `assets/pacman/` (1,5 MB); okładka z ilustracji tytułowej.
  Rekord `pacman_top` w NVS (`RECORD_KEYS`). PC 0,30 ms/klatkę. Regresja 39/39 (`pacman_title`, `pacman_auto`,
  `pacman_play`, `pacman_player`, `pacman_lava`, `pacman_gameover`; wzorce `menu`, `menu_carousel`, `menu_about` nagrane na
  nowo – 6 gier w karuzeli). Zintegrowany z głównym drzewem 25.09 wieczorem (worktree można usunąć: `git worktree remove ../LakeMarioGame_pacman`);
  firmware z Pacmanem: **1 413 kB flash (33,7 %), 98,1 kB RAM statycznie** (pierwszy `pio run` po `clean` padł na
  `ninja: failed recompaction: Permission denied` – wyścig o `.pio` z innym procesem; drugi przebieg OK).
  Nazwa „Pacman” i nazwy plansz/światów robocze. DECYZJE 34.
- **Lekcja 14 `14_obrazki` (23.09 noc):** PNG z `load_image`, klatki animacji w tablicy `Sprite hero[2]`, Piskel, drzewa jako
  przeszkody z cofaniem ruchu; gra „sad” (jabłka, pszczoła). Kod startowy pada na 2 testy, zad4/zad5 przechodzą (zad3 tylko test 1).
  Lekcja dodatkowa po 10. Grafika z `gen_demo_assets.py` (`assets/obrazki/`).
- Repozytorium: **https://github.com/grekot/ESP32_p4_game_console.git**, gałąź `main`, pierwszy commit 23.09.2026.
  Commit i push tylko na wyraźne polecenie użytkownika. `.gitattributes` wymusza LF w repozytorium.
  Uwaga historyczna: repozytorium bez żadnego commita wywala build ESP-IDF (woła `git describe`).
- Około 11 500 linii własnego kodu (bez sterownika ST7701 i lodepng), w tym ~1500 w `src/console` + lekcje + `tools/` oraz ~1100 w grach pokazowych.

## Komendy

```bash
pio run                              # firmware (pierwszy raz ~15 min: pobiera ESP-IDF i toolchain); po nowych plikach: pio run -t clean
pio run -t upload -t monitor         # wgranie + logi 115200
pio run -t uploadfs                  # katalog assets/ (PNG gier) -> partycja assets (SPIFFS); po kazdej zmianie obrazkow
cmake -S sim --preset mingw          # konfiguracja emulatora (raz; nowe .cpp wykrywa sam build - CONFIGURE_DEPENDS)
cmake --build sim/build              # emulator (kilka sekund) - ZAMKNIJ dzialajacy console_sim.exe!
./sim/build/console_sim.exe             # emulator okienkowy
./sim/build/console_sim.exe --list      # gry: numer, id, nazwa
./sim/build/console_sim.exe --game pilka --hold RIGHT 10 70 --frames 90 --trace 15      # po id/nazwie/katalogu lekcji
./sim/build/console_sim.exe --game 0 --hold B 3 4 --hold RIGHT 20 90 --frames 90 --trace 15   # test skryptowany
./sim/build/console_sim.exe --game labirynt3d --hold A 3 4 --hold UP 10 600 --frames 600 --bench   # sredni/max czas klatki (PC)
python tools/gen_demo_assets.py                              # PNG dla labirynt3d i kosmos (assets/), deterministyczne
python tools/gen_kart_atlas.py                               # atlas tekstur Karta (assets/kart/atlas.png, Pillow, deterministyczny)
python tools/gen_snake_assets.py                             # Snake: assets_src/snake/*.jpg (Gemini) -> assets/snake/*.png (Pillow)
python tools/gen_kart_gemini.py                              # Kart: assets_src/kart (Gemini) -> gory per motyw, chmury, ikony; potem gen_kart_atlas.py
python tools/gen_kart_atlas.py                               # Kart: assets/kart/atlas_<motyw>.png (tekstury, drzewa z Gemini)
python tools/kart_tracks_check.py --png p.png                # Kart: walidacja ukladow torow z kart_tracks.h
python tools/kart_shimmer.py                                 # Kart: pomiar migotania tekstur w oddali (emulator)
python tools/gen_covers.py                                   # okladki menu: assets_src/covers/*.jpg -> assets/covers/<id>.png 440x248
python tools/gen_pl_fonts.py                                 # polskie litery dla LVGL -> src/ui/fonts/console_fonts_pl.c
powershell -File tools/build_installer.ps1 -Version 1.0.0     # instalator Windows -> dist/ (Inno Setup 6); wydanie: git tag v1.0.0 + push = CI
powershell -File tools/testy.ps1 mario                       # regresja: 8 sladow + 6 zrzutow bajt w bajt (Mario, api_demo, labirynt, kosmos)
powershell -File tools/testy.ps1 src/games/lekcje/02_pilka   # testy zadan jednej lekcji (kod startowy PADA, rozwiazanie przechodzi)
powershell -File tools/testy.ps1 mario -Update               # nowe wzorce - tylko po swiadomej zmianie (np. menu po nowej lekcji)
```

Ścieżki: PlatformIO `C:\Users\grzegorz.kotarba\.platformio\penv\Scripts\pio.exe`, kompilator
`C:\msys64\mingw64\bin\g++.exe` (musi być w PATH razem ze swoimi DLL-ami), CMake 4.x w `C:\Program Files\CMake`,
Ninja w `C:\Prg\ninja-win`. Z Git Basha: `export PATH="/c/msys64/mingw64/bin:$PATH"` przed cmake.
**`pio run` uruchamiać z PowerShella, nie z Git Basha** — instalator narzędzi pioarduino odmawia pracy pod MSYS.

## Dwa cele, jeden kod

| warstwa | płytka | emulator |
|---|---|---|
| `src/platform/platform.h` | `src/platform/platform_esp.cpp` | `sim/platform_win32.cpp` |
| obraz | MIPI-DSI + PPA (skala x2, obrót) | okno Win32 GDI, `StretchBlt` |
| wejście | krzyżak + gałka + przyciski (JP1), GT911, BOOT | klawisze PC (`keymap.cfg`), pad USB (winmm), mysz |
| czas | `esp_timer` | `QueryPerformanceCounter`; w trybie `--frames` stały krok 1/60 s |

Zasada: **nic nad `platform::` nie może zawierać `#include "esp_*"` ani FreeRTOS.** Logowanie przez
`core/log.h` (`CONSOLE_LOGI/W/E`), nie `ESP_LOGx`. Sprawdzenie: `grep -rn "esp_\|freertos" src/app src/engine
src/gfx src/input src/ui --include=*.h --include=*.cpp | grep include` ma nic nie zwracać.

## Struktura

```
platformio.ini          firmware: platforma pioarduino 55.03.312 (ESP-IDF 5.5.5), board esp32-p4, CONSOLE_DISPLAY_ROTATION
CMakeLists.txt          projekt ESP-IDF; dodaje src/ui (lv_conf.h) do include WSZYSTKICH komponentów
sdkconfig.defaults      ESP-IDF: rewizja P4, PSRAM HEX, flash 16 MB DIO, LVGL (CONF_SKIP=n, bez dem/przykładów)
partitions.csv          nvs, phy, factory 4 MB @0x10000, assets (spiffs) ~12 MB
src/main.cpp            wejście płytki: NVS -> platform::init -> task "console" (rdzeń 1) -> app::run
src/platform/           interfejs platformy + implementacja ESP
src/board/              sterowniki płytki: pins.h, display (DSI+PPA), touch (GT911), keypad, joystick (ADC2), buttons, st7701/
src/core/log.h          logowanie zależne od celu
src/gfx/                Canvas RGB565 (blit, blit_scaled, blit_upscale2x, line, circle), Sprite z ASCII-artu (make_sprite, domyślna paleta),
                        png.h: load_png (RGB565 z kolorem-kluczem) i load_png_rgba → gfx::Image (RGB565 + alfa 0..255, PSRAM) dla gier
                        z mieszaniem alfa (Kart),
                        text.h (wygładzony tekst Montserrat 12-48 px z glifów LVGL - menu i gry ucznia),
                        palette.h (19 kolorów gfx::pal::* + DEFAULT_PALETTE, wspólna dla Mario i console), czcionka 5x7 (wielkie+małe)
src/input/              keys.h (14 klawiszy), pad.h (PadState + osie, held/pressed), virtual_pad (dotyk + klawisze -> PadState, zbocza)
src/engine/             Game (init/update/render/canvas_scale/debug_line), screen.h (800x480 + PIXEL_CANVAS 400x240), stats, game_registry (GameEntry{id,name,desc,create},
                        find_game), rng (xorshift32, seed_rng), math2d.h, particles.h (ParticlePool<N>), tilemap (TileMap: ASCII,
                        solid_at, move_x/move_y AABB, draw z kamerą - wycięte 1:1 z Mario)
src/ui/                 menu.cpp (karuzela/lekcje/ustawienia/o konsoli, update(pad), debug_line), fonts/ (polskie litery, generowane),
                        lv_conf.h (WSPÓLNY), lvgl_glue (PARTIAL, flush do płótna, indev dotyk+klawisze, grupa, zegar wirtualny w testach), dawniej menu (36 px/pozycja,
                        4 widoczne, pasek przewijania), pause
src/app/                maszyna stanów konsoli Menu -> Playing -> Paused; set_fixed_dt; debug_line; seed_rng przy starcie gry;
                        settings (ustawienia w NVS, apply = jasność), wygaszanie ekranu, licznik FPS, tryb podpowiedzi dotykowych
src/console/               API dla ucznia: console.h (using namespace console + makro CONSOLE_ADD_GAME), console_api.h (deklaracje z opisami),
                        console_runtime.cpp (implementacja, arena sprite'ów 64 kB, watch, mapa poziom 2), simple_game (adapter -> engine::Game)
src/games/registry.cpp  lista gier: Mario, Labirynt 3D, Kosmos + lekcje z lekcje/lista.h (X-makro)
src/games/mario/        assets (sprite'y ASCII na wspólnej palecie), level (6 segmentów 25x15), game (fizyka, HUD; mapa = engine::TileMap)
src/games/labirynt3d/   raycasting (DDA po mapie 32x24 z liter, własna tablica - TileMap ma max 16 wierszy), tekstury/sprite'y PNG,
                        zbuf_[400], gradient sufit/podłoga, drzwi 'D' znikają przy podejściu; debug_line: x y ang coins doors time
src/games/kosmos/       strzelanka 800x480: pule Bullet/Asteroid/Boom/Spark, gwiazdy 3 warstwy, PNG z assets/kosmos/; debug_line:
                        ship score lives asteroids bullets
src/gfx3d/              math3d.h (Vec3, Mat4 wierszowa, heading, shadow_onto_plane), mesh.h/.cpp (Mesh + bryły: box/hexa/loft/klin/
                        koło/walec/stożek/kula/dysk/billboard, UV na trójkąt, smooth/unlit/vertex_colors/alpha_test/specular),
                        renderer.h/.cpp (Camera, Light, Fog, Renderer: set_texture (atlas 8-bit + colormapa), enable_zbuffer,
                        begin/draw_mesh(depth_bias)/draw_tri/draw_shadow/end, project, horizon_y, set_flat_only)
src/games/snake/        snake_game (plansze, ruch, przedmioty, autopilot BFS; debug_line: stan lvl len head dir apples score lives items
                        portal pw=SGMXO ap), snake_render (bake_field, kulki ciała, głowy obrócone, portal 8 faz, HUD, tytuł, nakładki)
src/games/pacman/       pacman_game (plansza z pacman_mazes.h, ruch po kratkach, AI duchów, strach, owoce, autopilot BFS; debug_line:
                        stan lvl maze pac dir pel score lives g=HLNFEI ph fr fruit ap), pacman_render (bake_maze: tło PNG + ściany
                        z pola odległości, maska do migania; sprite'y z obrotami; panel; tytuł), pacman_mazes.h (4 plansze ASCII)
src/games/kart/         kart_game (tor Catmull-Rom → path_ + surf_, fizyka 2D, AI, przedmioty, ranking, dym, obrót kół, ślady opon),
                        kart_render (atlas + colormapa; build_road: road_ teksturowana Gouraud + marks_ płaskie; build_terrain;
                        build_props: brama/trybuna/opony/banery z teksturami; build_decor: billboardy drzew i krzaków;
                        build_kart_models z malowaniem; draw_scene: kamera, światło, mgła, Z-bufor, cienie rzutowane, ślady, jakość;
                        draw_sky: gradient 3-progowy/słońce/pasy; draw_effects: dym/płomień/iskry przez project(); HUD:
                        draw_hud_title/race/finish, draw_minimap); debug_line: lap place x y ang spd idx item surf t boost spin ai haz sh
assets/                 PNG gier: bohater/, api_demo/, labirynt3d/ (brick stone door exit coin portal), kosmos/ (ship asteroid_s/m/l
                        bullet boom0-3), obrazki/ (lekcja 14) - tools/gen_demo_assets.py; kart/ (banana/shell/mushroom 48 = ikony HUD,
                        clouds 1024x160, mountains 2048x160, smoke, glow, flame - tools/gen_kart_assets.py; atlas.png 512x512 8-bit
                        z paletą: asfalt, trawa ×2, publiczność, banery, drzewa, krzaki, bieżnik, brama, 4 malowania -
                        tools/gen_kart_atlas.py, układ kafelków w docstringu i w tabeli T_* w kart_render.cpp); gokarty, skrzynki,
                        brama, trybuna to modele 3D w kodzie. Podmiana pliku = nowa grafika
assets/pacman/          hero0-3, die0-3, ghost_<kolor>0/1, scared0/1, eyes, 8 owoców + ikony, bg_<świat> 648x456, title 800x480
                        (tools/gen_pacman_assets.py z assets_src/pacman/); okładka assets/covers/pacman.png (gen_covers.py)
src/games/lekcje/       lista.h (LEKCJA(id) na lekcję) + 00_szablon … 13_twoja_gra, 14_obrazki (dodatkowa, PNG): gra.cpp, README.md,
                        testy.txt, rozwiazania/*.cpp.txt
sim/                    emulator: CMakeLists (LVGL: managed_components → third_party/lvgl → FetchContent zip), CMakePresets,
                        platform_win32 (set_unthrottled), keymap_win32 + keymap.cfg, gamepad_win32 (winmm), main (--list, --game id,
                        --bench, --hold do 12 wpisów; set_fixed_dt PRZED start_game = stałe ziarno)
installer/              console.iss (Inno Setup, per uzytkownik, bez UAC), STEROWANIE.txt; program startowy sim/launcher_win32.cpp
                        (KotarbaConsole.exe: aktualizacje z GitHub Releases, SHA-256, /SILENT /RELAUNCH), ikona sim/app.ico
                        (tools/gen_icon.py) + sim/app.rc; CI .github/workflows/release.yml (tag v* -> wydanie). DECYZJE 35
tests/                  scenarios.txt + expected/ (ślady i BMP: Mario nagrane 23.09 przed refaktorem; api_demo, labirynt, kosmos 23.09 wieczorem)
tools/                  testy.ps1 (regresja + testy lekcji), setup_kid_pc.ps1 (PC ucznia), fetch_lvgl.ps1, bmp2png.ps1,
                        md2pdf.py (Markdown -> PDF przez Edge headless; python Windows + pakiet markdown),
                        gen_demo_assets.py (PNG gier pokazowych i lekcji 14, czysty Python/zlib), gen_kart_assets.py (model 3D gokarta
                        rzutowany w 16 kierunkach, przedmioty, drzewa)
docs/API.md             opis wszystkich funkcji console dla ucznia + plakat docs/images/api_plakat.png (gra 99_api_demo)
docs/pdf/               PDF-y z md2pdf.py: API, DLA_UCZNIA, kazda lekcja (odswiezac po zmianie README)
third_party/            (gitignore) LVGL z fetch_lvgl.ps1 na komputerze bez PlatformIO
docs/HARDWARE.md        płytka, pinout, JP1, kontroler, kalibracja gałki, lista do sprawdzenia na sprzęcie, źródła
docs/EMULATOR.md        budowanie, sterowanie, keymap, tryby CLI, testy skryptowane, źródła LVGL
docs/VSCODE.md          rozszerzenia, zadania (LEKCJA:/SIM:/PIO:), debug, na co odpowiadać przy pierwszym otwarciu
docs/NAUKA.md           nauka C++ (dla rodzica): założenia, program 13 lekcji, testy zadań, komputer ucznia, git
docs/DLA_UCZNIA.md      instrukcja dla ucznia: start, klawisze, ściągawka API, czytanie błędów, zasady
docs/lekcje/SZABLON_README.md  wzór README lekcji
docs/DECYZJE.md         dziennik decyzji projektowych z uzasadnieniami (wpisy 15-19: warstwa console, rejestracja, RNG, regresja, LVGL)
.vscode/                settings (CMake Tools -> sim/), tasks (LEKCJA:/SIM:/PIO:; domyślne = LEKCJA: Uruchom), extensions;
                        c_cpp_properties i launch GENERUJE PlatformIO
```

## Toolchain - pułapki

- Oficjalna platforma `platformio/espressif32` nie wspiera P4; używamy **pioarduino** (wymaga PlatformIO Core ≥ 6.2.0;
  Core zaktualizowano z 6.1.18). `pio upgrade` uruchomione przez `pio.exe` psuje własne środowisko — aktualizować
  przez `penv\Scripts\python.exe -m pip install -U platformio`.
- Konfiguracja ESP-IDF: `sdkconfig.defaults` to źródło prawdy; wygenerowany `sdkconfig.jc4880p443c` jest w
  .gitignore. Po zmianie defaults skasować wygenerowany plik i `pio run -t clean`.
- Listy plików są wyliczane przy konfiguracji (GLOB bez CONFIGURE_DEPENDS — ESP-IDF wykonuje CMakeLists
  komponentu także w trybie skryptu). Po dodaniu pliku: `pio run -t clean` oraz `cmake -S sim --preset mingw`.
- Kompilator MinGW nie startuje bez `C:\msys64\mingw64\bin` w PATH (brak DLL); CMake raportuje to mylnie jako
  „kompilator nie jest w stanie skompilować prostego programu".
- **Przed `cmake --build sim/build` zamknąć działający emulator** — Windows nie pozwala nadpisać uruchomionego exe,
  linker kończy się błędem `collect2` bez czytelnego komunikatu.
- `espressif/esp_hosted` ≥ 3.0 nie kompiluje się pod PlatformIO na Windows (`SHELL:-include`); przy dodawaniu
  Wi-Fi przypiąć `>=2.11,<3.0`.
- VS Code: PlatformIO IDE = firmware, CMake Tools = emulator (`cmake.sourceDirectory` -> `sim/`, preset `mingw`,
  debug przez `cmake.debugConfig` z gdb MSYS2). **Nie instalować pioarduino IDE obok PlatformIO IDE.**
  PlatformIO dopisuje `pioarduino.pioarduino-ide` do `extensions.json` przy każdym buildzie — jest też na liście
  `unwantedRecommendations`, co neutralizuje podpowiedź; nie walczyć z tym ręcznie.
- **Końce linii.** Bloby w repozytorium są LF (`.gitattributes` `eol=lf`), ale `core.autocrlf=true` na tym komputerze
  wypisywał część plików do drzewa roboczego z CRLF. 23.09 drzewo robocze przepisano na LF (`sed -i 's/\r$//'` po
  `git ls-files`) — dla gita to zero różnicy. Uwaga dla narzędzi: `perl -0pi` z wzorcem `\n` nie trafia w pliki CRLF,
  a `git diff --quiet` ignoruje `--ignore-cr-at-eol` przy kodzie wyjścia (używać `--name-only`).
- Emulator w trybie `--frames` nie czeka na 60 FPS (`sim::set_unthrottled`), stąd 600 klatek w ułamku sekundy; wyniki bez zmian.
- **Emulator w `--frames` ignoruje prawdziwą klawiaturę i mysz** (`sim::set_ignore_real_input`, 25.09). Wcześniej stan
  klawiszy z okna dokładał się do `--hold`, więc Enter (= START) wciśnięty przez osobę piszącą przy komputerze w trakcie
  `testy.ps1` pauzował grę w teście: ślad `kart_player` pokazywał `PAUSED`, a wzorzec nagrany `-Update` w takiej chwili był
  zły. Objaw: test raz przechodzi, raz nie, bez zmian w kodzie. Od tego samego dnia okno emulatora w `--frames` jest
  **ukryte** (`SW_HIDE`; zrzuty idą z bitmapy w pamięci) – wcześniej wyskakiwało na pierwszy plan i kradło fokus
  osobie piszącej w innym edytorze (zgłoszenie użytkownika).
- **Nie ruszać `managed_components/` podczas `pio run`.** Menedżer komponentów ESP-IDF przy każdym buildzie sprawdza hash
  katalogu; gdy 23.09 przemianowałem `managed_components` na czas testu ścieżek LVGL emulatora, a w tle szedł `pio run`,
  komponent `lvgl__lvgl` został uznany za uszkodzony i wyczyszczony (zostały `tests/` i `zephyr/`). Naprawa: `rm -rf
  managed_components/lvgl__lvgl` i `pio run` (pobiera wg `dependencies.lock`). Testy ścieżek LVGL emulatora robić przy
  zatrzymanym PlatformIO albo przez `-DCONSOLE_LVGL_DIR=`.
- **Partycja assets: `board_build.filesystem = spiffs` + `board_build.spiffs.obj_name_len = 64` w `platformio.ini`
  (dodane 25.09).** Bez nich pioarduino budował obraz **LittleFS**, a potem SPIFFS z nazwami 32 znaki – firmware montuje
  SPIFFS z `CONFIG_SPIFFS_OBJ_NAME_LEN=64` i `format_if_mount_failed`, więc `uploadfs` skończyłby się skasowaniem obrazków.
  Sprawdzenie bez płytki: `pio run -t buildfs` → `.pio/build/jc4880p443c/spiffs.bin` (25.09: 3,68 MB zajęte z 11,94 MB,
  w tym 2,96 MB plików; Snake 2,84 MB). Montowanie na sprzęcie nadal niesprawdzone.
- **Nie zmieniać `platformio.ini`, gdy otwarte jest VS Code z PlatformIO IDE.** Rozszerzenie po zapisie pliku samo
  rekonfiguruje projekt; równoległa konfiguracja ESP-IDF z terminala ściga się o `managed_components` i komponent
  `lvgl__lvgl` zostaje „corrupted” (25.09: dwa razy z rzędu). Naprawa jak niżej: `rm -rf managed_components/lvgl__lvgl`,
  odczekać, aż nie działa żaden `pio`/`python` z VS Code, i jeden `pio run`.
- **Narzędzie Bash (Claude Code) zjada podwójne ukośniki w heredocu**, także w `<<'EOF'`: `'\\'` w Pythonie z heredoca
  zrobiło się `'\'`, a `'\0'` bajtem NUL w `sim/platform_win32.cpp`. Skrypty z ukośnikami zapisywać do pliku i uruchamiać.
  Po edycji sprawdzić, że źródła C/C++ są w ASCII: `LC_ALL=C grep -c '[^ -~<TAB>]' plik` ma dać 0.
- Skrypty PowerShell w `tools/` są pod Windows PowerShell 5.1 (bez `&&`, bez `?:`); składnię sprawdza
  `[System.Management.Automation.Language.Parser]::ParseFile`.

## LVGL - pułapki, które już kosztowały czas

- Na ESP-IDF LVGL domyślnie **ignoruje `lv_conf.h`** (`CONFIG_LV_CONF_SKIP=y`). `sdkconfig.defaults` ustawia `n`.
- `src/ui` musi być na include **wszystkich** komponentów: `idf_build_set_property(COMPILE_OPTIONS "-I.../src/ui" APPEND)`
  w głównym `CMakeLists.txt`. Flagi z `platformio.ini` nie docierają do kompilacji LVGL.
- `lv_conf.h` musi mieć strażnik **`LV_CONF_H`**, inaczej LVGL ostrzega o nieudanym include.
- `CONFIG_LV_BUILD_EXAMPLES` i `CONFIG_LV_BUILD_DEMOS` są domyślnie włączone (258 zbędnych plików) — wyłączone
  w `sdkconfig.defaults`; to opcje Kconfiga, nie `lv_conf.h`.
- Emulator bierze LVGL z `managed_components/lvgl__lvgl` (identyczna wersja co firmware), a bez niego pobiera
  tag `v9.5.0`; obie ścieżki sprawdzone. Katalog z `lv_conf.h` wskazuje `LV_BUILD_CONF_DIR`.
- LVGL w trybie **PARTIAL**: rysuje do bufora 400x80, `flush_cb` kopiuje na płótno. Tryb DIRECT z przezroczystym
  tłem **nie działa** jako nakładka na grę (LVGL zamalowuje tło na biało). Nakładka = zamrozić klatkę gry i podać
  przez `ui::set_background_frame()` jako widget canvas (robi tak pauza).
- Podczas rozgrywki `ui::tick()` nie jest wołane — gra włada całym płótnem.
- `lv_obj_clear_flag` istnieje tylko w mapie zgodności z v8 — używać `lv_obj_remove_flag`.
- Domyślna obsługa asercji LVGL to `while(1)` — emulator „wisi" bez słowa. `lv_conf.h` ustawia `LV_ASSERT_HANDLER abort();`
  (kod wyjścia + stos w gdb: `gdb -p PID -batch -ex "thread 1" -ex bt`). Tak znaleziono brak `handlers` w `lv_draw_buf_t`.
- Wbudowane czcionki Montserrat mają tylko ASCII — napisy w UI i grach bez polskich liter (rysują się jako `?`).

## Architektura

- Płótno konsoli **800x480 RGB565** (768 kB, PSRAM) w natywnej rozdzielczości logicznego ekranu — od 23.09 po południu
  (wcześniej 400x240 skalowane x2, co dawało schodki na czcionkach; decyzja użytkownika: „nowocześnie, nie retro").
  Panel jest pionowy 480x800; `display::present()` obraca przez **PPA** (skala 1) do tylnego z dwóch buforów DPI,
  `esp_lcd_panel_draw_bitmap` z adresem własnego bufora sterownika przełącza bufory bez kopiowania, czekamy na
  `on_frame_buf_complete`. `CONSOLE_DISPLAY_ROTATION` (90/270) steruje obrotem i mapowaniem dotyku jednocześnie.
  `display::present()` nadal obsługuje dowolną całkowitą skalę, więc powrót do 400x240 to zmiana w `engine/screen.h`.
- **Gry pixel-art** (Lake Mario, sprite'y 16x16) deklarują `engine::Game::canvas_scale() == 2`: `app.cpp` daje im pod-płótno
  `PIXEL_CANVAS_W x H` = 400x240 (192 kB, SRAM) i po `render()` powiększa x2 na płótno (`Canvas::blit_upscale2x`, CPU).
  Ślady Mario są identyczne z czasów 400x240, zrzuty to dokładnie powiększenie x2 (sprawdzone piksel w piksel).
  Menu, pauza, gry ucznia rysują natywnie w 800x480.
- **Tekst w grach:** `gfx/text.h` (`draw_text_px`, `text_width_px`, `text_height_px`) miesza glify A8 z czcionek Montserrat
  LVGL (`lv_font_get_glyph_bitmap` do własnego `lv_draw_buf_t` przez `lv_draw_buf_init` — bez tego LVGL zatrzymuje się
  na asercji). `console::text(..., scale)` mapuje 1..4 -> 16/24/32/48 px. Czcionka 5x7 (`gfx/font.h`) zostaje dla HUD Mario
  i innych gier pixel-art. UI: `ui/theme.h` (paleta slate, czcionki, wymiary), menu z kartami 64 px, pauza z panelem.
- `console`: `sprite(s, x, y, flip, scale)` (16x16 rysuje się x3), kafelek mapy `TILE = 32` (15 wierszy = 480 px, 25 kolumn =
  ekran), `draw_tiles` powiększa sprite do kafelka. Lekcje przeliczone na 800x480 (rozmiary i prędkości x2), testy
  zaktualizowane (np. `square_x=241`).
- **Pliki i PNG (decyzja użytkownika: dekodowanie w grze, nie konwersja przy budowaniu):** `platform::read_file(path)` —
  emulator: `assets/<path>` (cwd, potem `<exe>/../../assets`, `<exe>/assets`); płytka: `/assets/<path>` z partycji `assets`
  (SPIFFS, montowana w `platform::init`, `format_if_mount_failed`, `CONFIG_SPIFFS_OBJ_NAME_LEN=64`). Dekoder lodepng
  w `src/gfx/lodepng/` (flagi `LODEPNG_NO_COMPILE_ENCODER/DISK/ANCILLARY_CHUNKS/CPP` w obu buildach), `gfx::load_png`
  → RGB565, alfa < 128 = TRANSPARENT. Uczeń: `load_image("hero.png")` → `assets/<id gry>/hero.png` (id z `CONSOLE_ADD_GAME`
  przez `SimpleGame`), nazwa z `/` = od `assets/`. Arena obrazków 256 kB. Wgrywanie na płytkę: `pio run -t uploadfs`
  (`data_dir = assets`). Regresja: scenariusz `api_demo` (zrzut z PNG). **Na sprzęcie niesprawdzone.**
- Kontroler: krzyżak + gałka analogowa + A/B/X/Y + START (układ jak w padzie Switch). Płytka: przełączniki
  wprost na GPIO, odkłócanie 8 ms w zadaniu 1 kHz (`board/keypad.cpp`); gałka na ADC2 z kalibracją środka przy
  starcie, strefa martwa 25 %, próg kierunku 0,5 (`board/joystick.cpp`). Jedno wejście: `platform::controller()`
  zwraca klawisze ORAZ osie -1..1; wychylenie gałki jest już przełożone na kierunki. **SELECT nie ma pinu**
  (`pins::KEY_SELECT = GPIO_NUM_NC`, sterownik pomija NC); w menu cofa B.
- Na JP1 ADC mają **tylko GPIO49-52** — osie gałki muszą być na dwóch z nich (49 = X, 50 = Y).
- `app/app.cpp`: Menu (LVGL) -> Playing -> Paused (LVGL na zamrożonej klatce). **Pauza należy do konsoli**, gra
  nie może reagować na START (obie warstwy widziały ten sam klawisz). Ekran tytułowy reaguje na A, B albo dotyk.
- Podpowiedzi dotykowe (wirtualny pad) znikają po pierwszym użyciu klawiszy (`VirtualPad::keys_used`).
- Menu i pauza mają nawigację klawiszami przez grupę LVGL (`ui::nav_group()`, `mark_focusable`).
- `engine::Game`: init/update/render + `debug_line()` (linia stanu do `--trace`). Rejestr gier w `games/registry.cpp`:
  `GameEntry{id, name, description, create}`; `find_game()` rozumie numer, id, nazwę i ścieżkę katalogu lekcji (`NN_id`).
- **Warstwa `console` (nauka C++ syna, 11-13 lat, po Scratchu):** uczeń pisze `setup()`/`frame()` w `namespace {}` i kończy plik
  `CONSOLE_ADD_GAME(id, "Nazwa", "opis")`; jedna linia `LEKCJA(id)` w `games/lekcje/lista.h` (X-makro w `registry.cpp`, jawnie —
  ESP-IDF linkuje komponent statycznie, samorejestracja by przepadła). API po angielsku (`rect`, `held(LEFT)`, `random`,
  `watch`, `load_sprite`, `load_map`/`move_box`), komentarze po polsku, matematyka całkowita przy 60 FPS. Adapter
  `console::SimpleGame` -> `engine::Game`, więc menu, pauza, `--trace` działają bez zmian. START/SELECT dla ucznia zawsze false.
  Zmienne globalne ucznia żyją między wejściami z menu — wartości startowe nadaje `setup()`. `watch()` (max 8) rysuje panel
  w rogu i trafia do `debug_line` → testy zadań w `testy.txt` (`argumenty | regex`) czytają je z linii TRACE.
- Losowość: `engine::rng()` (xorshift32); `app::start_game` seeduje stałą przy `s_fixed_dt > 0`, zegarem w oknie. Nie `rand()`.
- Sprite'y i czcionka to ASCII-art zamieniany na bitmapy przy starcie (`gfx::make_sprite`), kolor-klucz magenta.
  Wspólna paleta 19 kolorów w `gfx/palette.h` (litery k w e E r R o y Y g G b B t s u U p P); sprite'y ucznia idą do areny 64 kB
  zerowanej przy `setup()` (`platform::alloc_pixels(fast=false)` = PSRAM).
- **Pule obiektów (Kosmos, nauczka):** funkcja `spawn_*` szukająca wolnego slotu może zająć slot, który właśnie zwolniliśmy
  w tej samej iteracji — wszystko, co jest potrzebne po `alive = false` (rozmiar, pozycja), skopiować do lokalnych
  zmiennych **przed** spawnowaniem. Objaw był: „statek-widmo" (indeks -1 w tablicy sprite'ów) i niedeterministyczny ślad.
- **Testy lekcji: `(?m)`.** `testy.ps1` dopina `(?m)` do regexa, więc `^`/`$` działają na LINII, nie na całym wyjściu
  (wcześniej `lives=2( |$)` nie trafiało, bo za śladem są jeszcze logi). Przy podmianie `gra.cpp` na rozwiązanie i z powrotem
  przez `mv` **ninja nie przebuduje pliku** (stary mtime) – po przywróceniu `touch gra.cpp`, inaczej binarka nadal ma rozwiązanie.
- **gfx3d – układ i konwencje:** X prawo, Y góra, Z „w głąb”; mapa 2D gry (x, y) = 3D (X, Z). `Mat4::heading(a)` ustawia
  lokalne +Z modelu na (cos a, 0, sin a) i lokalne +X na prawo gokarta (jawna baza, nie `rotation_y` – ta mirrorowała X).
  Nawinięcie trójkątów: budować przez `add_tri_out/add_quad_out` z wektorem „na zewnątrz” (normalna = cross(b−a, c−a));
  cull odrzuca `dot(n, a − kamera) > 0`. Sortowanie malarskie: warstwa 0 (teren, cienie) przed 1 (obiekty); w kubełku
  kolejność zgłaszania (FIFO) – cień zgłaszać po drodze, gokart po cieniu. Skrzynki `unlit`, bo boki w cieniu wyglądały
  jak ciemne płytki „pływające” nad drogą (to nie był błąd geometrii). `draw_mesh` ma cache 4096 wierzchołków (alokowany w `Renderer::init`, SRAM z awaryjnym PSRAM) – większa siatka = podział na kilka.
  Zmiana mapy nawierzchni (stemplowanie komórek 8x8 zamiast tekseli) zmienia ślady Karta – wzorce nagrane 23.09 noc.
  **Kolory wierzchołków** (`vertex_colors`) wymagają `smooth`; wierzchołek dzielony między pasami o różnych kolorach się rozmyje –
  pasy o ostrej granicy (asfalt | trawa) mają własne wierzchołki, a płaskie oznaczenia (krawężniki, linie) siedzą w LUKACH siatki
  Gouraud, nie na niej (nakładanie dwu współpłaszczyznowych siatek = migotanie sortowania malarskiego). **Cienie na podłożu**:
  `draw_mesh(..., layer 0, cull, depth_bias = 60)` – bez przesunięcia duży trójkąt terenu o dalszym środku zamalowywał dysk cienia.
  Kamera nie może wjeżdżać w bryły (przycinanie z ≥ 1 rozrywa geometrię) – na tytule kołysze się za gokartami zamiast okrążać.
  **Stos zadania konsoli na płytce to 16 kB** (`main.cpp`): tablice robocze budowy siatek (indeksy pierścieni toru 10 kB, siatka
  terenu 4 kB) idą przez `new[]`/`delete[]`, nie na stos (wersja z 23.09 miała 15,6 kB na stosie w `build_scene` – na sprzęcie
  groziło przepełnieniem) i nie jako `static` (zjadałyby SRAM na stałe).
- Labirynt 3D: mapa 32x24 w własnej tablicy (`TileMap::MAX_ROWS` = 16; podniesienie limitu = więcej RAM w każdej instancji).
  Po zmianie mapy sprawdzić osiągalność BFS-em (skrypt jednorazowy; 4 zamknięte pokoje wyszły przy pierwszej wersji).
- Struktury ESP-IDF inicjalizować przez `= {}` + przypisania pól (kolejność pól w makrach IDF bywa niezgodna z C++).
- Duże bufory jawnie w PSRAM (`platform::alloc_pixels(..., fast=false)`), wyrównane do 128 B (PPA/cache).
  Płótno 192 kB próbuje najpierw SRAM. Bez wyjątków C++: nieudany `new` = abort.

## Weryfikacja gier w emulatorze (bez człowieka)

`console_sim.exe --game N --hold KLAWISZ OD DO ... --frames M --trace K` (do 12 wpisów `--hold`; także `--pause-at`,
`--shot plik.bmp`, `--keymap`, `--bench`). W trybie `--frames` krok czasu jest stały (`app::set_fixed_dt(1/60)`) i ustawiany
**przed** startem gry (stałe ziarno RNG), więc ten sam skrypt daje **identyczny** ślad także w grach losujących.
Sprawdzenie determinizmu: dwa uruchomienia `| grep TRACE | md5sum` muszą dać ten sam skrót. Na ekranie tytułowym najpierw wcisnąć B (`--hold B 3 4`), bo tytuł nie reaguje na START.

Zweryfikowane 23.09.2026 (Lake Mario): podłoże stabilne co klatkę; chód 100 px/s; bieg 165 px/s; ściana x=0;
skok: tap 28 px, 3 klatki 40 px, pełny 52 px, bufor 120 ms działa, poza oknem nie; uderzenie `?` (+200, moneta)
i rozbicie `B` (+50); przeskok nad przeciwnikiem; zadeptanie (+100, odbicie, przeciwnik znika); śmierć od
przeciwnika -> respawn z lives-1; 4 śmierci -> GAME OVER -> nowa gra; monety na platformie; pauza konsoli
i powrót (czas dalej liczy); START na tytule nie startuje gry. **Zmieniając fizykę, powtórz te scenariusze.**

**Siatka regresji (od 23.09 po południu):** `tests/scenarios.txt` = 6 śladów Mario (chód, skok tap/pełny, bieg, 600 klatek,
pauza) + 3 zrzuty BMP (tytuł, gra, menu), wzorce w `tests/expected/` nagrane z binarki **sprzed** refaktoru silnika
(math2d, palette, ParticlePool, TileMap); od wieczora także `api_demo` (zrzut), `labirynt_route` (ślad 640 klatek: skręty,
drzwi, moneta), `labirynt_door` (zrzut), `kosmos_play` (ślad + zrzut, losowość ze stałym ziarnem). od nocy `kart_auto` (ślad 1800 klatek autopilotem), `kart_race` (zrzut), `kart_player` (ślad). `tools/testy.ps1 mario`
musi dać 39/39 (od 25.09: sześć testów Snake, pięć testów menu, pięć testów torów Karta, sześć testów Pacmana) po każdej zmianie w `src/engine`, `src/gfx`, `src/games/*`. Zrzut `menu` zmienia się po dodaniu lekcji — wtedy `-Update`. Testy lekcji: kod startowy w `gra.cpp`
**ma padać**, `rozwiazania/zadN.cpp.txt` skopiowane do `gra.cpp` **ma przechodzić** (sprawdzone dla 01-12).

## Lake Mario - fizyka i poziom

- Podłoże: sprawdzać kafelek pod **dolną krawędzią** (`y + h`), nie ostatni piksel hitboxa. Wersja `y + h - 1`
  dawała migotanie sprite'a (stanie/skok na przemian co 2-3 klatki).
- `GRAVITY 1100`, `JUMP_V -350` / `-390` z biegu, ucięcie po puszczeniu do `-240` (min. 28 px), `JUMP_BUFFER 0.12 s`,
  `COYOTE 0.10 s`, chód 100, bieg 165 px/s. Wysokości: tap 1,8 kafelka, pełny 3,3, z biegu 4,3; dal 4 / 7 kafelków.
- Poziom 1 do przejścia **skokiem z chodu**: ściany i rury na trasie max 3 kafelki, przepaście max 3, platformy
  max 3 kafelki nad powierzchnią startu, bloki `?` w wierszu ≥ 9 (gracz stoi w wierszu 12). Bieg tylko do bonusów.
- Hitboxy: gracz 12x16 (sprite 16, offset x 2), przeciwnik 14x13 (offset x 1, y 3). Kafelek 16 px, 25x15 widocznych.

## Sprzęt - fakty krytyczne (szczegóły w docs/HARDWARE.md)

- `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` + `CONFIG_ESP32P4_REV_MIN_0=y` — P4 rev 1.x; zły wybór = crash w bootloaderze.
- Taktowanie **360 MHz** (400 MHz tylko rev ≥ 3.0 albo chipy kwalifikowane przez Espressif).
- LDO kanał 3 (2,5 V) zasila DSI-PHY — włączyć przed `esp_lcd_new_dsi_bus`. Kanał 4 (3,3 V) zasila kartę SD.
- Podświetlenie GPIO23 = zwykły GPIO, stan wysoki. Sterownik ST7701 producenta w `src/board/st7701/` (komponent
  z rejestru daje czarny ekran). Timingi: 2 linie DSI 500 Mbps, DPI 34 MHz, 480x800, h 12/42/42, v 2/8/166.
- GT911: I2C0 SDA 7 / SCL 8, adres 0x5D lub 0x14, RST/INT nieużywane. Magistrala wspólna z kodekiem ES8311 (0x18).
- JP1 (2x13): wolne GPIO 52, 51, 50, 49, 34, 33, 32, 31, 30, 29, 28 (+35 = BOOT, nie używać jako klawisza gry:
  przytrzymany przy starcie = tryb programowania; służy jako zapasowy START). Pełna tabela pinów w HARDWARE.md.
- Wi-Fi/BT tylko przez ESP32-C6 (ESP-Hosted, SDIO slot 1). Fabryczne firmware C6 (2.3.0) nie współpracuje
  z hostem 2.12 — trzeba je wgrać przez UART na JP1. Nieużywane w grze.

## Otwarte decyzje użytkownika (nie podejmować za niego)

1. **Gałka analogowa czy SELECT.** Gałka zjada dokładnie 2 piny, których brakuje na pełny pad. Bez gałki: SELECT + 1 pin
   wolny. Rekomendacja z 23.09: zbudować najpierw bez gałki, zostawić dla niej miejsce w obudowie; kod obsługuje oba
   warianty (`pins.h`). Użytkownik nie zdecydował.
2. **Obudowa.** Przełączniki MX (raster 19 mm) dają krzyżak 57x57 mm — dwa bloki nie mieszczą się pod ekranem
   (płytka ~110 mm). Opcje: osobny pad na kablu (rekomendacja), format Game Boya (~130x150 mm, przełączniki
   niskoprofilowe), format Switcha (~230 mm). Płytkę zmierzyć po przyjściu.

## Następne kroki

1. **Komputer syna:** `tools/setup_kid_pc.ps1` przetestować na czystej maszynie (VM / konto bez narzędzi) — składnia
   sprawdzona, przebieg nie. Utworzyć gałąź `lekcje` przed klonowaniem u syna. Prawdziwy test: lekcja 01 bez pomocy.
2. Płytka: uruchomienie wg listy w `docs/HARDWARE.md` (koniec ramki DPI co klatkę, kierunek obrotu vs USB,
   mapowanie dotyku, rewizja krzemu, kalibracja gałki, odkłócanie klawiszy). Lekcje pojawią się w menu konsoli.
3. Testy skryptowe brakujących mechanik Mario: meta `F` (LevelClear), przepaść, koniec czasu (dopisać do `tests/scenarios.txt`).
   Labirynt 3D: scenariusz dojścia do portalu (stan WON). Kart: test trafienia skorupą i mini-turbo.
4. Zrzuty lekcji do README (`LEKCJA: Zrzut ekranu` + `tools/bmp2png.ps1`), ewentualnie `docs/images/lekcje/`.
5. Dźwięk: ES8311 przez I2S (`espressif/esp_codec_dev`, adres 8-bitowy 0x30, I2S stereo slot mimo mono, jedna instancja IN_OUT);
   dla ucznia `console::beep()`.
6. Assety z partycji SPIFFS (narzędzie PNG -> RGB565) zamiast ASCII-artu.
7. Ekran ustawień, wyniki w NVS (rekordy lekcji), ekran „o konsoli".
8. IntelliSense dla plików `sim/` (dziś pokazuje błąd na `<windows.h>` — tylko kosmetyka).
9. Gamepad BLE przez C6 (drugi gracz), ekspander I2C PCF8574 na JP1 pin 23/25, jeśli zabraknie klawiszy.

## Zasady współpracy

- Użytkownik pisze po polsku; odpowiadać po polsku, konkretnie, z liczbami i tabelami, bez lania wody.
- Gdy pyta „co myślisz?" — najpierw ocena z uzasadnieniem i rekomendacją, implementacja dopiero po decyzji.
- Weryfikować pomiarem, nie na oko: `--trace`, zrzuty `--shot`, porównania śladów. Mówić wprost, co NIE jest przetestowane.
- Commit/push tylko na wyraźne polecenie. Nie robić `git init` bez commita.
- Przed każdym `cmake --build sim/build` zamknąć emulator; po zakończeniu pracy zostawić emulator uruchomiony,
  jeśli użytkownik o to prosił.
- Lekcje: kod startowy ma być grywalny od pierwszej sekundy i **celowo** pozbawiony jednej mechaniki, którą uczeń dopisuje;
  jedna nowa koncepcja na lekcję; README wg `docs/lekcje/SZABLON_README.md`; test w `testy.txt` czyta `watch()`;
  rozwiązanie w `rozwiazania/*.cpp.txt` zbudować raz (podmiana `gra.cpp`) i przywrócić kod startowy. Nazwy zmiennych ucznia
  nie mogą kolidować z libc (`time`, `random`, `abs` — `abs`/`min`/`max` są w `console`, unikać `time`).
