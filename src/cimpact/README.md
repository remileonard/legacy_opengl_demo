# cimpact_view - viewer for the Certain Impact models

Certain Impact! is a World War I air combat demo written by Paradigm
Simulation in 1996 with its Vega and AudioWorks toolkits, on top of IRIS
Performer. Its models (biplanes, blimp, trains, boats, bridges, balloons,
cockpit, explosions) and its terrain are stored in Vega's "fast" format,
`.fst`. `cimpact_view` loads and draws them with OpenGL 1.x and GLUT, using
the Performer layer written for Mekton (`src/mekton/pf.c`).

## The .fst format

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
state (lighting and texturing on, back faces culled).

## Usage

```
cimpact_view [data directory | model.fst ...]
```

With a directory, the viewer loads every `.fst`, `.fst.Z` or `.fst.gz` of
`<dir>/models_ogl` and `<dir>/contours_ogl` (compressed files need `gzip`).
The default directory is `data/cimpact`.

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
