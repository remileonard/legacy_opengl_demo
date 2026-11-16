# perfly

`perfly` is the database fly-through program of IRIS Performer 2.0
(Silicon Graphics, 1993-1996). It loads one or more scene files and lets
you look at them. You can turn them with a trackball, or drive or fly
through them, and a GUI panel changes the rendering.

## The sources

- `perfly/` holds the program: `perfly.c`, `gui.c`, `keybd.c`,
  `cmdline.c`, their headers and the IRIX `Makefile`.
- `common/` holds the code shared by the Performer samples: `main.c`
  (process setup and the frame loop), `generic.c` (shared memory, pipes,
  windows, channels and the draw callbacks) and `env.c` (Earth/sky, fog,
  time of day).

Both directories are the original 1996 files, built **unchanged**. Only
the Performer compatibility layer in
[`src/porting/performer`](../porting/performer) is new code.

## The Performer compatibility layer

Performer is replaced by a layer on top of OpenGL 1.x and freeglut.
`include/` provides the headers the samples include:

- `Performer/pf.h`, `pfutil.h`, `pfui.h`, `pfdu.h` and `prmath.h`;
- IRIX and X11 stand-ins: `bstring.h`, `sys/sysmp.h`, `sys/statfs.h` and
  `X11/*.h`. The X11 display queries are answered by freeglut.

On Windows, `win32/` adds a `getopt`.

The scene graph, the geosets, the textures, the intersections and the
`.pfb` loader are those of Mekton, in [`src/mekton/pf.c`](../mekton/pf.c).
The `.dwb` loader is the one of the F-18 demo,
[`src/f18/dwb.c`](../f18/dwb.c).

The layer is made of these files:

| File | Contents |
| --- | --- |
| `pf_runtime.c` | `pfInit`/`pfConfig`, pipes, pipe windows (one freeglut window each), channels (viewports, field of view, near/far, view and view offsets, channel groups and their shared attributes), the frame loop (`pfSync`, `pfFrame`: APP, CULL and DRAW callbacks of each channel), Earth/sky (clear, sky dome, sky and ground, horizon band), fog, frustums, libpr state (materials, light model, lights, matrix stack), statistics, strings, notification |
| `pf_scene.c` | bounding spheres, `pfSCS`, `pfFlatten`, partitions (plain groups) |
| `pfd_util.c` | loaders chosen by file extension, builder modes, scene geostate, scene-graph statistics, and a Wavefront `.obj` loader |
| `pfd_flt.c` | an OpenFlight `.flt` loader |
| `pfu_util.c` | window input as Performer events (`pfuMouse`, `pfuEventStream`), messages, draw styles, scene-graph display, SGI image snapshots, texture lists, side-by-side channels (MCO) |
| `pfu_gui.c` | the GUI panel and its widgets: buttons, switches, menu buttons, radio buttons, linear and log sliders, message bar |
| `pfi_xformer.c` | the `pfiTDFXformer`: trackball, drive and fly, with collisions |

Everything runs in one process. `pfConfig` forks nothing. `pfFrame` calls
the APP, CULL and DRAW callbacks of the channels one after the other,
swaps the windows and handles the window events. In free-run phase,
frames are paced to the 60 Hz video rate, like Performer's swaps waiting
for the retrace. In the other phases they are paced to the target frame
rate.

## Running

```
perfly [options] file.ext ...
```

Supported formats:

- `.pfb`: Performer binary, e.g. the Mekton models in `data/mekton/models`;
- `.dwb`: Designer's Workbench, e.g. the F-18 models and terrains in
  `data/f18/models`;
- `.obj`: Wavefront, e.g. the models in `data/smooth`. They are turned
  from Y up to Performer's Z up;
- `.flt`: OpenFlight, in the 14.2 format and the older format of version
  12. See below.

The folder of each file and the `../textures` folder next to it are added
to the search path for textures.

The window takes the whole screen. To choose its size, use `-W 1024,768`.

Examples:

```
perfly data/smooth/cow.obj
perfly -E skygrnd -f -p -25887,-30000,4000 -e 0,-20,0 data/f18/models/all_terrain.dwb
perfly -c 3 -E skygrnd data/mekton/models/colony_city.pfb
```

`perfly -h` lists all the options. These are the useful ones:

| Option | Effect |
| --- | --- |
| `-W x,y` | window size |
| `-c n` | n channels side by side |
| `-d` / `-f` | start in drive / fly mode (trackball by default) |
| `-p x,y,z` / `-e h,p,r` | initial position and orientation |
| `-E clear\|sky\|skygrnd\|skyclear` | Earth/sky mode |
| `-b r,g,b` | clear color |
| `-z near,far` | clip planes |
| `-l x,y,z,r,g,b` | add a light |
| `-g 0\|1\|2` | GUI off, vertical, horizontal |
| `-r hz` | target frame rate |
| `-P phase` | frame phase |
| `-X radius` | explode the files |
| `-F path` | file search path |
| `-I n` | print the statistics and exit after n frames |

## OpenFlight

The `.flt` loader reads:

- the header and its units;
- the color, material and texture palettes;
- the vertex palette with its vertex lists (14.2), and the old vertices
  given inside the faces, with integer coordinates (version 12);
- groups, objects, levels of detail and matrices;
- external references, each file loaded once and shared;
- faces with their colors, materials, textures and draw types (solid,
  two-sided, wireframe, points);
- subfaces, drawn as decals;
- billboard faces, turned toward the eye around Z.

It skips the transform records that only describe a matrix and the light
source palette.

The texture attribute files (`name.rgb.attr`) give the wrap of the
textures. The mountain walls of the toon and town skies clamp their v
coordinate, which goes slightly past 1. With the default repeat, a dark
band from the bottom of the image showed along the top of the walls.

Textures are searched in this order:

1. the name as written (absolute, or relative to the `.flt` file);
2. its base name on the file path (`-F`).

So the absolute paths of the machines where the databases were made
(`/usr/mg/Flt/Txt/...`, `/show/town/pftexture/...`) still find their
image in the texture folder. Images shared by several files are loaded
once.

A face is lit when it has a material: its color is then the face color
times the diffuse color of the material. Faces without a material keep
their color, unlit. Textures with an alpha channel (`.rgba`, `.inta`) are
cut out on their alpha.

The *toon* town of the Performer "friends" demos runs with its original
command, from the folder that holds `toon/` (its data is not in this
repository):

```
perfly -N -L -E tag -l 0 -d -s 3.75 -d -P 3 -p 2500,2500,2 -g 2 \
    -F toon/pftoon:toon/pftexture:toon/pfmodel toon/pftoon/toon.flt
```

The town of the same demos (`orig/`, OpenFlight version 11 and 12) runs
the same way:

```
perfly -N -L -E tag -l 2 -d -p 2500,2500,2 -g 2 \
    -F orig/pftown:orig/pftexture:orig/pfmodel orig/pftown/town.flt
```

The `.path` files of `orig/pfpath` are motion paths, for a later perfly
(`perfly -A model.flt,tour.path,Tour`). The perfly of Performer 2.0 has no
paths: its `-A` is the welcome text.

## Controls

Mouse, in the scene:

| Mode | Left | Middle | Right |
| --- | --- | --- | --- |
| trackball | rotate | pan | zoom |
| drive | faster, the mouse across the scene steers | height | slower, then backward |
| fly | faster, the mouse steers and climbs | steer only | slower, then backward |

For trackpads without a middle button: Shift + left acts as the middle
button, Ctrl + left as the right button.

Keys (the help is printed with `?`):

| Key | Action |
| --- | --- |
| Esc | quit |
| Space | stop |
| F1 | GUI on/off |
| F12 | print the eye position |
| `r` | reset the position |
| `s` / `g` | statistics |
| `S` / `G` | print the statistics |
| `l` | lighting: eye, sun, off |
| `t` | textures |
| `w` / `W` | wireframe / cycle the draw styles |
| `b` | back-face culling |
| `c` | collisions |
| `C` | cull mode |
| `f` / `F` | frame rate |
| `p` | frame phase |
| `h` | scene-graph display |
| `z` | culling frustum box |
| `x` / `X` | snapshot to `perfly.N.rgb` / `.rgba`, with a `.info` file |

The GUI panel holds:

- buttons: Quit, GUI Off, Reposition, Center, Snap Screen, Reset All;
- menus: draw style, highlight, texture, background, motion model, cull
  mode, fog, lighting;
- switches: collisions, scene-graph display, stress;
- statistics modes;
- sliders: field of view, clip planes, fog ranges, time of day, LOD scale,
  tree pan and scale, stress.

## What differs from Performer

- **Multiprocessing.** None: one process for APP, CULL and DRAW. The
  processor locking, priorities and phases (besides frame pacing) do
  nothing.
- **Culling.** Nothing is culled. The cull modes, the culling-frustum box
  (`z`) and the stress settings are kept and shown, but have no effect.
  Neither do the LOD fade, the highlights, the display lists, the dynamic
  video resolution (`k`) and the arrow keys (clip-texture border).
- **Statistics.** The statistics display was redrawn by me: frame rate,
  process times, and the geosets, primitives and vertices drawn, with a
  graph of the last 120 frames (APP green, CULL yellow, DRAW red).
- **Optimizations.** The scene-graph optimizations of libpfdu (sharing,
  flattening, layers, billboards) keep the graph as it is.
- **Writing files.** Only the ASCII listing (`-w file.out`) can be written.
- **Welcome text.** The 3D text needs Performer's `.of` font files, which
  are not present. The welcome message is drawn as text, which is what
  perfly itself does when the font is missing.
- **Anti-aliasing.** The fill statistics, multisampling and anti-aliasing
  widgets are not offered. The queried features say there is no
  multisampling.
- **A guarded uninitialized pointer.** `updateStats` (perfly.c) can pass
  an uninitialized `pfFrameStats` pointer when the statistics are printed
  while already shown. On IRIX the pointer happened to hold a valid value.
  The layer reads only the statistics of a channel and ignores any other
  pointer.
