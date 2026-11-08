# VORTEX — bar-locked granular portal (VST3)
create by flmd

## Сборка (Windows, для FL Studio)
Нужны: Visual Studio 2022 (Desktop C++), CMake 3.22+, Git.

    cmake -B build -G "Visual Studio 17 2022"
    cmake --build build --config Release

Готовый плагин: `build/Vortex_artefacts/Release/VST3/VORTEX.vst3`
Скопируйте папку/файл в `C:\Program Files\Common Files\VST3`, затем в FL Studio:
Options > Manage plugins > Find more plugins.

Также собирается Standalone (`build/Vortex_artefacts/Release/Standalone`) — удобно смотреть UI без DAW
(часы тогда идут от внутреннего темпа 120 BPM).

macOS: `cmake -B build -G Xcode` и `cmake --build build --config Release`.

## Ручки
RATE — сетка гранул 1/1 … 1/64 (с точками и триолями), строго по BPM хоста
SIZE — длина гранулы, % от шага сетки
SCAN — центр: позиция стоит, вправо/влево: скан вперёд/назад (скорость привязана к темпу)
HARMONY — 0: оригинал, выше: гранулы питчатся по ступеням гаммы и разбрасываются по октавам
CHAOS — случайность позиции, питча, панорамы, реверса, лишние «призрачные» гранулы (сид повторяется каждый цикл)
MIX — dry/wet
Списки: KEY (транспонирование от C), SCALE, BUFFER (1/2/4 такта). Кнопка FREEZE замораживает буфер.
Двойной клик по ручке возвращает значение по умолчанию.

## Советы
- Ставьте плагин на дорожку с семплом/луп/мелодией и играйте проект: буфер пишется по сетке такта.
- Для мелодий: HARMONY 50–80%, SCALE под тональность семпла, CHAOS 20–40%.
- FREEZE + SCAN даёт «протягивание» замороженного момента по такту.
