# powerflip

Reconstruction of **powerflip**, the successor of flip on IRIX, from the
IRIX 6.5 binary (MIPS N32, C++). powerflip is flip whose objects are
described in **YAODL**, a scene description language: groups,
transformations, polygons, lines, colors, normals, and in the original also
textures, NURBS, lights and animation.

```
powerflip [-rsSCwWLRDhM] [-B blurfactor] [-Fdensity] [-f r,g,b]
          [-T title_message] [-m material,backmaterial]
          [-p x,y,z] [-o q1,q2,q3,q4] [-c r,g,b]
          [-b backgroundfile] model.ydl|model.defs [...]
```

| Option | |
|---|---|
| `-s` | stay still, no random rotation |
| `-w` / `-W` | the models are wireframe / wireframe while moving |
| `-L` | lines model: no lighting |
| `-r` | redraw even when nothing moves |
| `-b file` | a YAODL background, drawn in [-1, 1] without lighting nor z-buffer (`background*.defs`, `stars.defs`) |
| `-c r,g,b` | background color |
| `-F density`, `-f r,g,b` | fog of this density / color |
| `-C` | clipping plane on |
| `-B n` | motion blur keeping n % of the previous frames (with an accumulation buffer) |
| `-m m,b` | material and back material of the models (0 to 9, as in the menu) |
| `-p x,y,z`, `-o q1,q2,q3,q4` | position and orientation (quaternion) of the models |
| `-T text` | a line of the message shown while loading |
| `-S`, `-M`, `-R`, `-D`, `-z` | stereo, no multisampling, root window, debug, undocumented: accepted, no effect here |

Mouse: unlike flip, the left button rotates the selected objects (virtual
trackball) and the middle button moves them; both together zoom; the
right button opens the menu (`ui_swap_buttons` of libdemo).

The menu of flip gets the entries of powerflip:

- **Fog**: clear air, fog (grey or `-f`), haze, black fog (depthcue); the
  background takes the color of the fog; density, near and far distances,
  modes `vertex`/`pixel` x `exp`/`exp2`/`lin` (density 0.2, near 0.02,
  far 5 by default);
- **Blur and Sharpen**: motion blur, only with an accumulation buffer (not
  with Mesa/Xvfb, for example);
- **Clipping Planes**: the opaque objects keep one side of a plane, the
  transparent ones the other side; the plane turns with the mouse with
  "Move with Mouse";
- **Move Wireframe**: wireframe while moving.

## Files

| File | Role |
|---|---|
| `powerflip.c`, `light.c`, `powerflip.h` | the application, taken from flip (menus, mouse, lights, materials) |
| `pfopts.c` | the options and menus of powerflip: background, fog, clipping plane, motion blur, wireframe |
| `pfobj.c` | the objects of the application: YAODL models |
| `yaodl.c`, `yaodl.h` | parser and classes of YAODL |
| `image.c` | `readImageRGB`: the SGI images of the textures |
| `gen_envmap_yaodl.sh` | the IRIX script adding an environment map to a model |

The IRIS GL calls go through `src/porting` (`c3f`, `c4f`, `texgen`, the
NURBS functions, `clipplane`, `acsize` / `acbuf`, the back material, and
`lmcolor` and `fogvertex` were completed there for powerflip).

## YAODL

```
#YAODL
quad = (polygons (vertices -1 -1 0  1 -1 0  1 1 0  -1 1 0, : (normals 0 0 1 ...))),
(group quad quad,
    : (translates -1 0 0  1 0 0),
    : (colors 1 0 0  0 0 1)),
```

- `(class args)` is an object, `name = object` a definition reused by its
  name (also inside a list); the items after `:` are attributes of the
  enclosing object;
- numbers make lists of ints or floats (with a point), separated by commas;
- `.ydl` files hold binary blocks: `@float`, `@int` or `@indices`, a NUL,
  a big endian 32 bit size in bytes and the big endian data.

An object has N elements (children of a group, vertices, polygons); the
element i of each attribute applies to its element i. The transformations
are restored between two elements.

| Class | Arguments | Elements |
|---|---|---|
| `group` | objects | its children |
| `lines`, `polygons` | `vertices` objects | a line / polygon per list of vertices |
| `indexpolygons` | `vertices`, `indices` | the polygons of the indices |
| `indices` | a list of ints per polygon (text), `npolys, sizes..., indices...` (binary) | |
| `vertices`, `normals`, `colors` | floats, 3 per element | `v3f`, `n3f`, `c3f` |
| `colors4f` | floats, 4 per element | `c4f` |
| `translates`, `scales` | floats, 3 per element | `translate`, `scale` |
| `rotates` | angle (degrees) and axis, 4 per element | rotation |
| `untransform` | none | back to the viewing matrix |
| `nofliplighting` | none | no lighting |
| `textures` | file names (strings) | `texbind` of the texture i |
| `texcoords` | floats, 2 per element | `t2f` |
| `sContours`, `tContours` | mode (`contour`, `linear`, `spheremap`, `off`), space (`eye`, `object`), plane (4 floats, not for `spheremap`), repeated | `texgen` of s / t |
| `regularMesh` | 2 ints (columns, rows), `vertices` | triangle strips between the rows |
| `subdivides` | | accepted, no effect (IRIX screen subdivision) |
| `nurbs` | s knots, t knots, `ns nt`, ns*nt points of 3 or 4 floats (s varying fastest), repeated | a NURBS surface, with its attributes (trimming curves) inside it |
| `trimcurves` | knots, number of points, points `u v w`, repeated | a rational trimming curve of the surface i |

The colors replace the diffuse color of the material (`lmcolor(LMC_DIFFUSE)`).
A texture is read once, modulates the lit color, and is searched in the
current directory then in the directory of the model. A texture bound by
an enclosing object wins over the inner ones. In `eye` space the plane of a
contour is given with the identity as viewing matrix.

A NURBS surface also gets a bilinear patch of texture coordinates over the
parameters [-1, 1] x [-1, 1]; it is drawn filled, or as outline polygons
in wireframe.

```
sh gen_envmap_yaodl.sh {LINEAR|CONTOUR|SPHEREMAP} model.ydl image.rgb > model_env.ydl
```

On Windows, `data/powerflip/gen_envmap_yaodl.cmd` (also in the Windows
package) does the same without bash; the output is a file, since a binary
`.ydl` cannot go through the console:

```
cd data\powerflip
gen_envmap_yaodl.cmd SPHEREMAP ranger.ydl flowers.rgb ranger_env.ydl
..\..\interactive\powerflip.exe ranger_env.ydl
```
Polygons and lines are compiled in display lists, as in the original.

## Status

1. scene engine: groups, transformations, polygons, lines, colors, normals - **done**
2. textures, texture coordinates, `regularMesh`, `subdivides` - **done**
3. lights: the light sources and lighting models are options of the menus,
   the same as in flip (the binary has the same names: White Infinite, Red
   Local...); the YAODL classes `light`, `lightPosition`, `lightColor`,
   `lightAmbient`, `spotlight`, `spotdirection` are used by none of the
   models and are parsed and ignored
4. NURBS and trimming curves - **done**
5. animation (`advanceTime`, `seconds`, `spline`, `transVector`, `rotVector`)
6. the menus and options of powerflip (background, fog, motion blur,
   clipping planes, wireframe...) - **done**

The classes of the later steps are parsed and ignored with a message.

## Data

The models and images of powerflip are in `data/powerflip` (the
`background*.defs` files are the backgrounds of the `-b` option, step 6).
`bignose.defs` and `marble.defs` are fragments using definitions of
another file, and `re_can.ydl` uses `re_coke.rgb`, which is missing: it is
drawn without its texture. The textures given with an IRIX absolute path
(`/usr/demos/...`) are searched in the directory of the model.
