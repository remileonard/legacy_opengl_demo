/*
 * dwb.h - loader of the Designer's Workbench files (.dwb) of the F-18 demo
 *
 * Designer's Workbench is the modeler of Coryphaeus Software; the demo
 * (Performer 2.2 and Coryphaeus EasyScene) loads its models, its terrains
 * and its cockpit with pfdLoadFile_dwb.  This loader builds the same kind
 * of graph on the Performer layer of Mekton (src/mekton/pf.c).
 */
#ifndef DWB_H
#define DWB_H

#include "pf.h"

/* statistics of the last file loaded */
typedef struct DwbStats {
    int groups, lods, layers, polygons, vertices;
    int textures, textures_loaded, textures_encrypted, materials;
    float version;
} DwbStats;

/* reads a .dwb file, NULL on error; the textures are searched next to the
 * file, in ../textures and in the directories of pfFilePath */
pfNode *pfdLoadFile_dwb(const char *file);
const DwbStats *dwbGetStats(void);

#endif
