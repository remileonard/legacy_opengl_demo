/*
 * pfb.h - loader for IRIS Performer binary files (.pfb), as written by
 * pfdStoreFile_pfb and read by pfdLoadFile_pfb (Performer 2.0, 1996).
 *
 * The layout was rebuilt from the pfdLoadFile_pfb code linked into the
 * IRIX mekton_player executable.
 */
#ifndef PFB_H
#define PFB_H

/* primitive types, in the order of the file codes */
enum {
    PFB_TRIFANS, PFB_TRISTRIPS, PFB_TRIS, PFB_POINTS, PFB_LINES,
    PFB_LINESTRIPS, PFB_FLAT_LINESTRIPS, PFB_QUADS, PFB_FLAT_TRISTRIPS,
    PFB_POLYS
};

/* attribute bindings, in the order of the file codes */
enum { PFB_BIND_NONE, PFB_OFF, PFB_PER_VERTEX, PFB_PER_PRIM, PFB_OVERALL };

/* node types, in the order of the file codes */
enum {
    PFB_NODE_TEXT = 1, PFB_NODE_GEODE, PFB_NODE_BBOARD, PFB_NODE_REF23,
    PFB_NODE_GROUP, PFB_NODE_SCS, PFB_NODE_DCS, PFB_NODE_PARTITION,
    PFB_NODE_SCENE, PFB_NODE_SWITCH, PFB_NODE_LOD, PFB_NODE_SEQUENCE,
    PFB_NODE_LAYER, PFB_NODE_MORPH
};

/* gstate keys */
enum {
    PFB_GS_TRANSPARENCY = 1, PFB_GS_ANTIALIAS, PFB_GS_DECAL,
    PFB_GS_ALPHAFUNC, PFB_GS_ENLIGHTING, PFB_GS_ENTEXTURE, PFB_GS_ENFOG,
    PFB_GS_CULLFACE, PFB_GS_ENLPOINTSTATE = 12, PFB_GS_FRONTMTL = 15,
    PFB_GS_BACKMTL, PFB_GS_TEXTURE, PFB_GS_TEXENV, PFB_GS_LIGHTS = 21,
    PFB_GS_LPOINTSTATE = 24
};

#define PFB_INHERIT (-1000)

typedef struct {
    int side;
    float alpha, shininess;
    float ambient[3], diffuse[3], specular[3], emission[3];
    int cmode_side, cmode;
} PfbMtl;

typedef struct {
    char *name;
    unsigned int gl_id;
    int width, height, comps;
} PfbTex;

typedef struct {
    int mode;
    float blend[4];
} PfbTEnv;

typedef struct {
    /* modes: PFB_INHERIT when the gstate does not set them */
    int transparency, alphafunc, lighting, texturing, fog, cullface, lpoint;
    /* attributes: index or -1 */
    int frontmtl, backmtl, texture, texenv, lpstate;
} PfbGState;

typedef struct {
    int count, comps;
    float *f;   /* coords, colors, normals, texcoords */
    int *i;     /* lengths */
} PfbList;

typedef struct {
    int bind, list, ilist;
} PfbAttr;

enum { PFB_COORD, PFB_COLOR, PFB_NORMAL, PFB_TEXCOORD };

typedef struct {
    int prim, nprims, lengths;
    PfbAttr attr[4];
    int flatshade, wireframe;
    int gstate;
    float linewidth, pointsize;
    unsigned int isectmask;
    float bbox[6];
} PfbGSet;

typedef struct {
    int type;
    char *name;
    int nchildren, *children;
    int ngsets, *gsets;
    float matrix[16];
    int ref;
    unsigned int travmask[4];
    int userdata;      /* index of a user data block, or -1 */
    /* switch */
    int switch_val;
    /* sequence */
    int nframes;
    float *times;
    int seq_mode, seq_begin, seq_end, seq_reps;
    float seq_speed;
} PfbNode;

typedef struct PfbFile {
    int nmtl, ntex, ntenv, ngstate, ngset, nnode;
    PfbMtl *mtl;
    PfbTex *tex;
    PfbTEnv *tenv;
    PfbGState *gstate;
    PfbGSet *gset;
    PfbNode *node;
    int nlist[9];
    PfbList *list[9];  /* index: chunk type 4..8 */
    int nmorph;
    int **morph;       /* raw pfMorph records */
    int *morph_len;
    int nud;
    unsigned int **ud; /* user data blocks, 32-bit words in host order */
    int *udsize;       /* in bytes */
} PfbFile;

PfbFile *pfb_load(const char *path);
void pfb_free(PfbFile *f);
int pfb_morph_sources(const PfbFile *f, int morph);
/* recompute the destination lists of a morph; NULL uses the file weights */
void pfb_morph(PfbFile *f, int morph, const float *weights);

#endif
