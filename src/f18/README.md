# F-18

The F-18 demo was written in 1996 with IRIS Performer 2.2 and Coryphaeus
EasyScene (IRIX executable `F18.pf22_ogl62`). Its models, its terrains and
its cockpit were made with Coryphaeus' Designer's Workbench and are stored
in its binary format, `.dwb`, loaded by `pfdLoadFile_dwb`. This directory
holds a loader for that format, rebuilt from the loader linked in the
executable, and a viewer (`f18_view`) using the Performer layer written for
Mekton (`src/mekton/pf.c`).

## The viewer

    f18_view [data directory | model.dwb ...]

Without arguments the models are looked for in `data/f18/models` (the
`models` directory of the original distribution); a directory or `.dwb`
files can be given instead.

| Input | Action |
|---|---|
| left drag, arrows | turn around the model |
| right drag, wheel, `+` `-` | zoom |
| middle drag, shift + left drag | pan |
| `n` `p`, page down / up | next / previous model |
| `w` | wireframe |
| `t` | textures |
| `l` | lighting |
| `d` | levels of detail: on the eye distance, or always the most detailed |
| `r` | reset the view |
| `h` | information and help |
| `q`, Esc | quit |

The textures are looked for, by the base name of the file name stored in
the model, next to the model, in `../textures` and in `textures`, with
the extensions `.rgb`, `.rgba`, `.int`, `.inta` and `.bw` when the name
has none.

Most images of the demo were encrypted for its distribution: their SGI
header is valid but a word after the colormap (offset 0x6c) is 4 for the
verbatim images and 5 for the run-length encoded ones, and the pixels are
scrambled. They are recognized and not drawn: the textured polygons of the
terrains are then white. Their decryption is the next step.

## The Designer's Workbench format

A file is a sequence of big-endian records: a 16-bit opcode, a 16-bit
length of the data, then the data. `0x17ca` (push) and `0x17cb` (pop)
enclose the children of the record before them.

| Opcode | Record | Fields used |
|---|---|---|
| `0x1771` | header (320 bytes) | `+0x04` float version, `+0x08` date, `+0x110` short units (0 m, 1 ft, 2 in, 3 cm, 4 nautical miles...) |
| `0x1772` | group (160 bytes) | `+0x34` flags: bit 26, the children are decals on the first child (`pfLayer`) |
| `0x1773` | level of detail (96 bytes) | `+0x08` switch out, `+0x0c` switch in, `+0x10` center |
| `0x1775` | polygon (76 bytes) | `+0x00` color index, `+0x04` vertices, `+0x10` texture (-1 none), `+0x2c` color `0xAABBGGRR`, `+0x36` material (-1 none) |
| `0x177a`, `0x177c` | vertices of the polygon | 44 bytes each: color index, x y z, normal, s t, 2 floats |
| `0x1798` | name of the record before it | |
| `0x179c` | file name of the texture before it | |
| `0x17ca`, `0x17cb` | push, pop | |
| `0x17ea` | material (160 bytes) | `+0x04` diffuse, `+0x10` ambient, `+0x1c` shininess, `+0x20` specular, `+0x2c` emission, `+0x38` alpha |
| `0x17eb` | texture | `+0x00` width, `+0x02` height, `+0x04` components |

The other records (`0x1790` color table, `0x1791` eyepoints, `0x17e8`,
`0x17ec` and `0x17ed` settings, `0x17ee` texture mappings, `0x1799`,
`0x179a` and `0x17c0` texts of the cockpit) do not change the geometry and
are skipped.

The loader builds a `pfGroup` per group, a `pfLayer` per decal group, a
`pfLOD` (with a group child) per level of detail, and gathers the polygons
of a group sharing a texture, a material and a transparency into one
geoset of `PFGS_POLYS`. The polygon colors are kept with the materials
(color material on ambient and diffuse), as Performer does.

For this loader the Performer layer (`src/mekton/pf.c`) gained LOD ranges
and centers (`pfNewLOD`, `pfLODRange`, `pfLODCenter`, `pfLODScale`) and
decal layers (`pfNewLayer`, `pfLayerMode`, drawn with a polygon offset);
the nodes loaded from the Mekton and Certain Impact files keep their
behavior.
