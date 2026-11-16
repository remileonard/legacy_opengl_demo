/*
 * Performer/pfdu.h - the libpfdu of IRIS Performer 2.0 used by the sample
 * programs: the loaders of the database files chosen on their extension,
 * the builder modes and the scene graph optimizations (which keep the
 * graph as it is), the fonts.
 *
 * The loaders: .pfb (Performer binary, the loader of Mekton), .dwb
 * (Designer's Workbench, the loader of the F-18 demo) and .obj (Wavefront).
 */
#ifndef PERFORMER_PFDU_H
#define PERFORMER_PFDU_H

#include <Performer/pf.h>

int pfdInitConverter(const char *file);
int pfdExitConverter(const char *file);
void pfdConverterMode(const char *ext, int mode, float value);
void pfdAddExtAlias(const char *ext, const char *alias);
int pfdStoreFile(pfNode *root, const char *file);
void pfdFreeFileLoaders(void);

#define PFDBLDR_MESH_ENABLE 0
#define PFDBLDR_MESH_SHOW_TSTRIPS 1
#define PFDBLDR_MESH_INDEXED 2
#define PFDBLDR_MESH_MAX_TRIS 3
#define PFDBLDR_MESH_RETESSELLATE 4
#define PFDBLDR_MESH_LOCAL_LIGHTING 5
#define PFDBLDR_AUTO_COLORS 10
#define PFDBLDR_AUTO_NORMALS 11
#define PFDBLDR_AUTO_TEXTURE 12
#define PFDBLDR_AUTO_ORIENT 13
#define PFDBLDR_AUTO_DISABLE_TCOORDS_BY_STATE 15
#define PFDBLDR_AUTO_DISABLE_NCOORDS_BY_STATE 16
#define PFDBLDR_AUTO_LIGHTING_STATE_BY_NCOORDS 17
#define PFDBLDR_AUTO_LIGHTING_STATE_BY_MATERIALS 18
#define PFDBLDR_AUTO_TEXTURE_STATE_BY_TEXTURES 19
#define PFDBLDR_AUTO_TEXTURE_STATE_BY_TCOORDS 20
#define PFDBLDR_BREAKUP 30
#define PFDBLDR_BREAKUP_SIZE 31
#define PFDBLDR_BREAKUP_BRANCH 32
#define PFDBLDR_BREAKUP_STRIP_LENGTH 33
#define PFDBLDR_SHARE_MASK 34
#define PFDBLDR_ATTACH_NODE_NAMES 35
#define PFDBLDR_DESTROY_DATA_UPON_BUILD 36
#define PFDBLDR_PF12_STATE_COMPATIBLE 37
#define PFDBLDR_BUILD_LIMIT 38

#define PFDBLDR_COLORS_PRESERVE 0
#define PFDBLDR_COLORS_MISSING 1
#define PFDBLDR_COLORS_GENERATE 2
#define PFDBLDR_COLORS_DISCARD 3
#define PFDBLDR_NORMALS_PRESERVE 0
#define PFDBLDR_NORMALS_MISSING 1
#define PFDBLDR_NORMALS_GENERATE 2
#define PFDBLDR_NORMALS_DISCARD 3
#define PFDBLDR_TEXTURE_PRESERVE 0
#define PFDBLDR_TEXTURE_MISSING 1
#define PFDBLDR_TEXTURE_GENERATE 2
#define PFDBLDR_TEXTURE_DISCARD 3
#define PFDBLDR_ORIENT_PRESERVE 0
#define PFDBLDR_ORIENT_NORMALS 1
#define PFDBLDR_ORIENT_VERTICES 2

void pfdBldrMode(int mode, int val);
int pfdGetBldrMode(int mode);
void *pfdGetDefaultGState(void);

pfNode *pfdCleanTree(pfNode *node, void *keep);
pfNode *pfdFreezeTransforms(pfNode *node, void *keep);
void pfdMakeShared(pfNode *node);
void pfdMakeSharedScene(pfScene *scene);
void pfdCombineLayers(pfNode *node);
void pfdCombineBillboards(pfNode *node, int size);
void pfdPrintSceneGraphStats(pfNode *node, double elapsed);

#define PFDFONT_TEXTURED 0
#define PFDFONT_OUTLINED 1
#define PFDFONT_FILLED 2
#define PFDFONT_EXTRUDED 3
#define PFDOBJFNT_TEXTURED PFDFONT_TEXTURED
#define PFDOBJFNT_OUTLINED PFDFONT_OUTLINED
#define PFDOBJFNT_FILLED PFDFONT_FILLED
#define PFDOBJFNT_EXTRUDED PFDFONT_EXTRUDED
/* the 3D fonts (.of files) of Performer are not read: NULL */
pfFont *pfdLoadFont_type1(const char *name, int type);

#endif
