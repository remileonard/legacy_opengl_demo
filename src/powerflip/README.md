# powerflip

Reconstruction of **powerflip**, the successor of flip on IRIX, from the
IRIX 6.5 binary (MIPS N32, C++). powerflip is flip whose objects are
described in **YAODL**, a scene description language: groups,
transformations, polygons, lines, colors, normals, and in the original also
textures, NURBS, lights and animation.

```
powerflip [-sh] [-W[xorg,yorg,]width,height] model.ydl|model.defs [...]
```

## Files

| File | Role |
|---|---|
| `powerflip.c`, `light.c`, `powerflip.h` | the application, taken from flip (menus, mouse, lights, materials) |
| `pfobj.c` | the objects of the application: YAODL models |
| `yaodl.c`, `yaodl.h` | parser and classes of YAODL |
| `image.c` | `readImageRGB`: the SGI images of the textures |
| `gen_envmap_yaodl.sh` | the IRIX script adding an environment map to a model |

The IRIS GL calls go through `src/porting` (`c3f`, `c4f`, `texgen` and
`lmcolor` were completed there for powerflip).

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

The colors replace the diffuse color of the material (`lmcolor(LMC_DIFFUSE)`).
A texture is read once, modulates the lit color, and is searched in the
current directory then in the directory of the model. A texture bound by
an enclosing object wins over the inner ones. In `eye` space the plane of a
contour is given with the identity as viewing matrix.

```
sh gen_envmap_yaodl.sh {LINEAR|CONTOUR|SPHEREMAP} model.ydl image.rgb > model_env.ydl
```
Polygons and lines are compiled in display lists, as in the original.

## Status

1. scene engine: groups, transformations, polygons, lines, colors, normals - **done**
2. textures, texture coordinates, `regularMesh`, `subdivides` - **done**
3. YAODL lights
4. NURBS and trimming curves
5. animation (`advanceTime`, `seconds`, `spline`, `transVector`, `rotVector`)
6. the menus and options of powerflip (fog, motion blur, clipping planes...)

The classes of the later steps are parsed and ignored with a message.
