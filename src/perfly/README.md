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
| `pfu_path.c` | the vehicles on `.path` files |
| `pfu_face.c` | the worlds of `perfly.face` (see below) |

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

- `.pfb`: Performer binary, e.g. the Mekton models in `data/mekton/models`.
  The later version 2 files (magic `0xDB0ACE01`, e.g. `earth.pfb`) have
  the same chunks, but their coordinates are scrambled: each word is
  xor'ed with a key of 3 words (`21b3c975 ad314ffb c7230d61`). I found the
  key on `earth.pfb`, a sphere whose coordinates are its normals times its
  radius. The key is applied when it gives plausible coordinates and the
  raw values do not;
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

A face is lit when it has a material. The material's ambient and diffuse
colors are then multiplied by the face color. Unlit (lighting off, or a
face without a material), the face shows its own color. Some materials are
emissive (the Mobil sign, the blimp of the town): they glow, with the same
color in every lighting mode.

Skipping the transform records and the light source palette loses
nothing in the toon and town databases:

- every transform record (translate, rotate...) comes with the matrix
  record that holds the whole transformation, which is what the loader
  uses;
- toon.flt holds a light source palette (one white infinite light), but no
  light source record uses it.

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

## Vehicles on paths

The `.path` files of `orig/pfpath` move models through the town, with the
syntax of a later perfly: `-A model.flt,file.path,Name`.

In the perfly of Performer 2.0, `-A` is the welcome text. So
`paths/perfly_paths.c` provides `main`. It takes out the `-A` options that
hold two commas, then calls the original `main` (`common/main.c`, compiled
as `perfly_main`). The other `-A` options are still the welcome text, and
the original sources stay unchanged. The Performer layer moves the models
(`src/porting/performer/pfu_path.c`).

A path is made of these lines:

- `speed s rate`: speed in units per second, and its rate of change; a
  negative speed runs the path backward;
- `line x1 y1 z1 x2 y2 z2`: a straight segment;
- `fillet r`: rounds the corner between the line before and the line after
  with an arc of radius r;
- `arc cx cy cz r a0 a1`: an arc;
- `close`: the path loops.

The models face +X and turn along their motion. For the town:

```
perfly -N -L -E tag -l 2 -d -p 2500,2500,2 -g 2 \
    -F orig/pftown:orig/pftexture:orig/pfmodel:orig/pfpath \
    -A esprit.flt,city.path,Lotus -A truck.flt,city2.path,Truck \
    -A blimp.flt,blimp.path,Blimp -A dashposx.flt,tour.path,Dash8 \
    orig/pftown/town.flt
```

`tour.path` was written for a `carpet.flt` that is not in the data. The
Dash 8 model turned toward +X (`dashposx.flt`) flies it instead.

Keys, taken before perfly sees them:

| Key | Action |
| --- | --- |
| `v` | follow the next vehicle, then none |
| `V` | chase view (behind and above) or on-board view |

The Motion menu of the GUI lists them too: Follow (chase view) and Ride
(on board) for each vehicle, after Trackball, Drive and Fly. The layer
extends the menu when perfly makes it, so `gui.c` is unchanged. The keys go
through the menu, so the menu shows the vehicle that the keys select.

The name of the followed vehicle is shown at the bottom of the view. When
it is left, trackball mode comes back to its view, and drive and fly modes
go on from where the eye is.

## The face demo (`perfly.face`)

`perfly.face` is the perfly of the "Earth to Matterhorn" demo: a zoom from
space down to a mountain through a chain of textured worlds. It is a
perfly built with extra code, and that code is not in the sources, so the
layer rebuilds it from the IRIX executable. As in the original, it turns on
only when a file named `paths` is in the current folder; without it,
perfly is the normal one.

The demo is started from its folder with the files of the worlds, in the
order of `paths`:

```
cd face
perfly earth.pfb 3010_mix.pfb trans_1.pfb trans_2.pfb europe.pfb ...
```

`paths` splits these files into worlds. Each world starts with
`world n`, followed by its tags:

| Tag | Meaning | Default |
| --- | --- | --- |
| `files n` | the next n files of the command line | |
| `numTextures n` | the next n textures of `texture.config` | |
| `textureOrder i ...` | the texture of each geoset, in the order of the graph; -1 means none | geoset k takes texture k |
| `lighting 0/1` | the lighting of the textured geosets | 0 |
| `center x y z` | the center of the tether | 0 0 0 |
| `minRadius`, `maxRadius` | the limits of the radius | 0, 5000 |
| `inRadius`, `outRadius` | with `inAround`... `outTilt`, the inner and outer positions the right button flies to; reaching the inner one enters the next world, the outer one the previous world | 20, 5000 |
| `inAround`, `inTilt`, `outAround`, `outTilt` | the eye when entering the world from the next world (in) or the previous one (out) | 0 |
| `nearMinAround` ... `farMaxTilt` | the limits of the angles at the smallest (near) and the largest (far) radius, interpolated in between | -180, 180 |
| `nearClip`, `farClip` | the clip planes | 1, 100 |
| `skyColor r g b` | the clear color | 0 0 0 |
| `scaleLinearVelocity` | the radius change per frame | 1 |
| `maxAngularVelocity` | the turning speed, in degrees per frame | 30 |
| `minRoll`, `maxRoll`, `maxRollVelocity` | the roll of the two-button motions | -180, 180, 30 |
| `tileSize` | read and unused (texture paging) | |

After the last world, flying in goes back to world 0. In a transition
world (two files, outer radius at most 3), the first geoset of the second
picture fades out as the eye goes in.

`texture.config` lists the textures: their count, the count of layouts,
the layouts (letter, width, height and mipmap levels), then one line per
texture with its file name and its layout. `%d` in the name is the
folder of a mipmap level, 1 being the largest
(`textures.%d/earthR.sgi.5551E`). The largest level found is loaded, and
OpenGL makes the smaller ones. When no level is found, the file is looked
up by its name alone, then without `.5551E` (`earthR.sgi`), so a picture
converted to a format the layer reads can stand in for it.

The `.5551E` images are 16-bit RGBA 5551 pictures. The original scrambles
them. The file starts with `SEn`, the count of inserted bytes, the width
and the height (big-endian integers). Every byte is XOR'ed with 13 plus
its position. Runs of junk bytes are inserted between runs of picture
bytes, and the picture is written from its last byte back to its first.
The layer decodes them as the original did.

The original pages its textures into texture memory a tile at a time. The
layer loads each texture whole, the first time its world is shown.

Controls of the tether. They come from the executable
(`pfiProcessTDFTetherMouse` and `pfiUpdate2DIXformTether`). As in the
other Performer motion models, the eye moves only while a button is held:

| Input | Action |
| --- | --- |
| left button | turn around and tilt; the farther the mouse is from the center, the faster |
| middle button (or Shift + left) | change the radius: mouse above the center goes in, below goes out |
| right button (or Ctrl + left) | fly to the inner position of the world (mouse above the center) or to its outer position (below), the radius and the angles arriving together; once there, enter the next or the previous world |
| left + middle, left + right | the same, with roll |
| Page Up / Page Down | fly in / out, like the right button |
| arrows | turn around and tilt |

Only the right button changes worlds, as in the original: the middle
button stops at the limits of the world. Speeds in `paths` are per frame
of the 60 Hz of the demo; the layer scales them to the actual frame time.
The keyboard keys are additions of the layer.

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
