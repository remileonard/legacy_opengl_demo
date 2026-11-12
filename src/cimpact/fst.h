/*
 * fst.h - loader for the Vega "fast" files (.fst) of Certain Impact.
 */
#ifndef FST_H
#define FST_H

#include "pf.h"

/* loads a .fst file (also .fst.Z and .fst.gz through zcat / gzip), NULL on error */
pfNode *fstLoadFile(const char *path);

#endif
