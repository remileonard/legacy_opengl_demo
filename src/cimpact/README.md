# Certain Impact

Certain Impact! is a World War I air combat demo written by Paradigm
Simulation in 1996 with its Vega and AudioWorks toolkits, on top of IRIS
Performer. This directory holds a port of the game (`cimpact`) and a viewer
for its models (`cimpact_view`), both with OpenGL 1.x and GLUT, using the
Performer layer written for Mekton (`src/mekton/pf.c`).

## The game

The game was rebuilt from the IRIX executable `cimpact_ogl`, whose C++
classes kept their names: `mardi` (the application), `vgAirPlane` (the
flight model of the player), `vgEnemyAirPlane`, `vgBattleRegion`,
`vgBalloon`, `vgCannon`, `vgShip`, `vgBridge`, `vgBunker`, `vgTrain`,
`vgBlimp`, `vgMachGunPair`, `vgBombs`, `vgCockPit`, `vgSelections`... Their
logic is in `ci_main.c`, `ci_plane.c` and `ci_world.c`.

`vg.c` is a small compatibility layer for the parts of Vega the game uses:
the application definition file (`cimpact.adf`: objects, files, positions,
isect classes, DIS appearance mappings, scenes, environment), copies of
objects, parts, intersections against the scenes (segments with class
masks, ground height, pitch and roll), the paths of the ships and of the
train (`vgPath`: lines, fillets, speeds, delays) and a stroke font.
`jpeg.c` decodes the intro pages.

### Playing

Pick an airplane and a skill level in the hangar with the mouse, then click
*Start Game* (or press F9). German airplanes start from the German airfield
and must protect the supply train: the Allied blockades along the railway
destroy the cars that reach them. Allied airplanes start from the British
airfield and must stop the train, for instance by bombing the bridge.
Ships, the blimp, houses, crates and artillery are targets too; enemy
fighters and flak defend the battle regions, where balloons give fuel,
ammunition and bombs, or repairs when you fly into them. The score
counts in Cadet and doubles in Ace; the Invincible level does not count.

The airplane rolls on the ground until it is fast enough: pull the stick
(move the mouse down) to take off. A too slow airplane stalls; landing
needs a gentle descent with the wings level. After a crash the game starts
again.

| Input | Action |
| --- | --- |
| Mouse position | stick (in the cockpit view, the middle of the upper part of the screen is neutral) |
| Left button, `a`, Page Up | throttle up |
| Right button, `z`, Page Down | throttle down |
| Middle button, left + right buttons, `x`, Enter | machine guns |
| Space | drop a bomb |
| F1 | cockpit, follow and train views |
| F2 | distance of the follow view |
| F3, `m` | map on/off |
| `f` | fog on/off (`-nofog` on the command line starts without fog) |
| F9 | back to the hangar / start |
| Esc | quit |

Like the original, the game runs at 30 frames per second: its flight model
works frame by frame.

The original also used a flight box (joystick) and played its sounds with
AudioWorks; the port has no sound and no network (DIS) support.

### Data

```
cimpact [-noinstruct] [-nofog] [data directory] [file.adf]
```

The default data directory is `data/cimpact`, or `Certain` when the
original distribution is next to the program. The game looks for its files
in that directory and in its `bin`, `models_ogl`, `contours_ogl`, `img`,
`visual_data/...` and `data/visual_data/...` subdirectories, so the
directory of the original distribution (`Certain`) can be given as is, or
its files copied like this:

```
data/cimpact/bin/cimpact.adf, trainPath, AlliedShipPath1, ... (from Certain/bin)
data/cimpact/models_ogl/*.fst                                (from Certain/data/visual_data)
data/cimpact/contours_ogl/*.fst
data/cimpact/img/*.jpg
```

The `.fst.Z` files of the distribution (Unix `compress`) are decompressed
by the loader itself; `.fst.gz` files are read through `gzip`.

## The viewer

The models (biplanes, blimp, trains, boats, bridges, balloons, cockpit,
explosions) and the terrain are stored in Vega's "fast" format, `.fst`.
`cimpact_view` loads and draws them.

### The .fst format

The format was never documented. It was rebuilt from `vgLoadFast` and the
`vgFast*::Read` methods linked into the IRIX `cimpact_ogl` executable.

A file is a memory image of a scene graph, big-endian:

- a 556 byte header: magic `0xf1d05bed`, version `2.0` (float), then sizes
  and offsets; word 5 is the offset of the directory, word 6 its size and
  word 7 the start of the records;
- a directory of `{ type, offset }` pairs, one per record, in the order the
  reader walks the graph. Each `Read` takes the next entry, checks its type
  and reads the record at its offset. A record met twice (a shared geode or
  geostate) is recognized by its offset and shared;
- the records.

| Type | Record |
| --- | --- |
| 1 | object (root) |
| 3 | billboard: geode plus one position per geoset |
| 9, 10 | SCS, DCS: matrix |
| 11 | geoset: primitive type, lengths, draw modes, geostate, then 4 attributes (colors, normals, texture coordinates, coordinates) with their binding |
| 12 | geostate: inherit mask, 12 modes (transparency, alpha function, lighting, texturing, culling, ...) and up to 10 attributes |
| 13 | geode |
| 14 | group |
| 15 | layer |
| 16 | light point |
| 17 | LOD: center and ranges |
| 18 | sequence: frames, mode, interval, speed, repetitions, frame times (doubles) |
| 19 | switch |
| 20 | texture: name, size, filters, wraps and the texels (bytes, bottom row first) |
| 21 | texture environment |
| 22 | material |

Group-like nodes start with their name and bounding sphere and end with
their children. Inherited geostate modes take the defaults of Vega's global
state (lighting and texturing on, no face culling: `backface off` in the
gfx of `cimpact.adf`).

### Usage

```
cimpact_view [data directory | model.fst ...]
```

With a directory, the viewer loads every `.fst`, `.fst.Z` or `.fst.gz` of
`<dir>/models_ogl` and `<dir>/contours_ogl`, or of the same directories in
`<dir>/data/visual_data` for the original distribution (`.gz` files need
`gzip`). The default directory is `data/cimpact`, then `Certain`.

| Input | Action |
| --- | --- |
| `n` / `p` (or Page Down / Page Up) | next / previous model |
| Left drag, arrow keys | turn around the model |
| Right drag, mouse wheel, `+` / `-` | zoom |
| Shift + left drag, middle drag | pan |
| `w` | wireframe |
| `t` | textures on/off |
| `l` | lighting on/off |
| Space | pause the animations |
| `r` | reset the view |
| `h` | show or hide the help line |
| `Esc` / `q` | quit |

The Certain Impact data files are not part of this repository.
