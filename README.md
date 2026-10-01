# Cubiomes Viewer

Cubiomes Viewer provides a graphical interface for the efficient and flexible
seed-finding utilities provided by [cubiomes](https://github.com/Cubitect/cubiomes)
and a map viewer for the Minecraft biomes and structure generation.

The tool is designed for high performance and supports Minecraft Java Edition
main releases up to 1.21.


## Tectonic build

This branch (`tectonic`) shows worlds of the [Tectonic](https://modrinth.com/mod/tectonic) world generation mod
(3.0.31 mod with its default config, or the 3.0.29 datapack) on Minecraft 26.3: biome map, base surface height,
climate layers.

It is built on [GlowingGavin's 26.3 port](https://github.com/GlowingGavin/cubiomes-viewer) of Cubiomes Viewer 4.1.2
(abandoned camps, 26.x biome names, extra structure conditions) and uses
[xpple's cubiomes fork](https://github.com/xpple/cubiomes) as the library (it knows Minecraft 26.x; the upstream
library pinned by Cubiomes Viewer does not), with the Tectonic module from
[xu-shawn/cubiomes, branch `tectonic`](https://github.com/xu-shawn/cubiomes/tree/tectonic) as the `cubiomes` submodule.
Tectonic itself is by Apollo (MIT).

### Build and run

Needs Qt 5 development files, `qmake`, `cmake`, a C/C++ compiler and `make` (Debian: `qtbase5-dev cmake build-essential`).

```sh
./run-tectonic.sh              # builds into build/ if needed (JOBS=3 by default), then starts the viewer
./run-tectonic.sh --help       # options are passed on to the viewer
```

or by hand: `git submodule update --init && mkdir -p build && cd build && qmake .. && make -j3 && ./cubiomes-viewer`.
The library is built with CMake as part of `make` (`build/cubiomes-lib`).

This build keeps its settings and sessions in `~/.config/cubiomes-viewer-tectonic/`, apart from a regular Cubiomes
Viewer (`~/.config/cubiomes-viewer/`) and from the Flatpak (`~/.var/app/com.github.cubitect.cubiomes-viewer/`), so it
cannot disturb existing sessions. Session files written with Tectonic on contain a line `#Tectonic: 1` (mod) or
`#Tectonic: 2` (datapack), which other builds ignore.

### Using it

* **Terrain** (below "Large biomes"): *Vanilla*, *Tectonic 3.0.31 (mod, default config)* or *Tectonic 3.0.29
  (datapack)*. Choosing Tectonic switches the version to 26.3 (Tectonic's data is for 26.3); choosing another version
  switches back to Vanilla. The choice is saved with the settings and in session files, like "Large biomes".
* The biome map then shows Tectonic's biomes at every zoom level, and the position readout shows
  `base Y <height> <biome>` for the block under the cursor.
* **Layer > Height** (or *Layer display options*, Height) has two new display modes that work for any world:
  *Hypsometric tints* (the palette of the Tectonic explorer, with relief shading) and *Tints + outline Y>=150* (red
  outline around the mountains). With Tectonic the height is the base surface: the terrain before Tectonic's peaks
  and 3D noise, which on mountain crests rise up to ~130 blocks higher. The existing height modes (grayscale,
  shading, contours) use it as well.
* The climate layers (temperature, humidity, continentalness, erosion, weirdness, depth) show Tectonic's values, as
  the biome source sees them; "depth" is the terrain offset. Their per-octave sub-options do not apply.
* `--grab=map.png [--grab-window=window.png] [--layer=mode,disp] [--grab-size=WxH]` renders the map of the saved
  settings to an image and exits; with `QT_QPA_PLATFORM=offscreen` no window appears. Layer 0 is biomes, 12 is
  height (display 4 = tints, 5 = tints + outline).

### What is exact, what is not

* **Exact**: base surface height and the six climate parameters are bit-identical to Minecraft 26.3 with Tectonic
  (validated in the library on 200,000 positions per path, see `cubiomes/tectonic/README.md`). Biomes are identical to
  the game on 99.95% of columns at 1:4 and at 1:1; the rest are exact ties between two biomes that the game and the
  library's biome tree resolve differently. Rendered maps of this build compared pixel by pixel with the game's
  biomes: 1,736,093 of 1,736,520 pixels identical (5 maps, both variants, 1 and 4 blocks per pixel).
* **Approximate**: the height is the *base* surface, not the generated surface with peaks, overhangs and caves.
  Maps at 16 blocks per pixel and coarser sample one position per pixel.
* **Unverified**: structures, spawn, strongholds, slime chunks and all search conditions. They use vanilla placement
  logic; with Tectonic on, their biome checks see Tectonic's biomes, but terrain checks and conditions that read the
  climate noise directly still use vanilla noise. Treat structure positions in a Tectonic world as hints.
* **Missing**: the real surface (3D terrain), other Tectonic versions and non-default mod configs, and biomes added
  by other mods.


## Download

Precompiled binaries can be found in the [releases section](https://github.com/Cubitect/cubiomes-viewer/releases)
on GitHub. This includes single file executables, which are statically
linked against [Qt](https://www.qt.io).

A Flatpak for the tool is available on
[Flathub](https://flathub.org/apps/details/com.github.cubitect.cubiomes-viewer).

For Arch Linux users, the tool may be found in the
[AUR](https://aur.archlinux.org/packages/cubiomes-viewer) thanks to
[JakobDev](https://github.com/JakobDev).

Non-PC platforms, such as macOS, are not formally supported, but you can check
[here](https://github.com/Cubitect/cubiomes-viewer/issues/107) for more
information on this issue.


## Build from source

Build instructions can be found in the [buildguide](buildguide.md).


## Basic feature overview

The tool features a map viewer that outlines the biomes of the Overworld,
Nether and End dimensions, with a wide zoom range and with toggles for each
supported structure type. The active game version and seed can be changed
on the fly while a matching seeds list stores a working buffer of seeds for
examination.

The integrated seed finder is highly customizable, utilizing a hierarchical
condition system that allows the user to look for features that are relative to
one another. Conditions can be based on a varity of criteria, including
structure placement, world spawn point and requirements for the biomes of an
area. The search supports Quad-Hut and Quad-Monument seed generators, which can
quickly look for seeds that include extremely rare structure constellations.
For more complex searches, the tool provides logic gates in the form of helper
conditions and can integrate Lua scripts to create custom filters that can be
edited right inside the tool.

It is also possible to find Locations in a fixed seed. In this mode, the
conditions are checked against a list of trial positions instead of the
world origin. Each location that passes the conditions is then collected
with additional information on where each individual condition was triggered.

An analysis of the biomes and structures can be performed in their respective
tabs. This provides information on the amount of biomes and structures that
are available in an area, as well as their size and positions.


## Screenshots

Screenshots were taken of Cubiomes Viewer v4.0.

![seeds](etc/screenshot_seeds-fs8.png
"Searching for a quad-hut near a stronghold with a good biome variety")

![locations](etc/screenshot_locations-fs8.png
"Locations in a given seed while viewing the world's height map")

![structures](etc/screenshot_structures-fs8.png
"Examining structures in the nether")


## Languages

The active language can be selected under `Edit preferences`, which currently includes translations for:

- English
- German
- Chinese

Chinese translations are provided by [SunnySlopes](https://github.com/SunnySlopes)
and are maintained at [his fork](https://github.com/SunnySlopes/cubiomes-viewer).


## Known issues

Desert Pyramids, Jungle Temples and, to a lesser extent, Woodland Mansions can
fail to generate in 1.18+ due to unsuitable terrain. Cubiomes will make an
attempt to estimate the terrain based on the biomes and climate noise. However,
expect some inaccurate results.

The World Spawn point for pre-1.18 versions can sometimes be off because it
depends on the presence of a grass block, that cubiomes cannot test for.


## Legal information

The main code is under the GPLv3, see [LICENSE](LICENSE), while other
components are released under their respective author licenses:

- Biome and structure generation from cubiomes, licensed under MIT.
- Cross platform [Qt](https://www.qt.io/licensing) GUI toolkit, available under (L)GPLv3.
- Dark Qt theme derived from [QDarkStyleSheet](https://github.com/ColinDuquesnoy/QDarkStyleSheet), licensed under MIT.
- Biome colors and icons are inspired by [Amidst](https://github.com/toolbox4minecraft/amidst), licensed under GPLv3.
- [Lua](https://www.lua.org/license.html) is distributed under the terms of the MIT license.

NOT AN OFFICIAL MINECRAFT PRODUCT.
NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT.


