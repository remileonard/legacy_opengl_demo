/* getopt.c - the POSIX command line parser, for Windows */
#include <stdio.h>
#include <string.h>
#include "getopt.h"

char *optarg = NULL;
int optind = 1, opterr = 1, optopt = 0;

int getopt(int argc, char *const argv[], const char *optstring)
{
    static int pos = 1;
    const char *arg, *o;
    if (optind >= argc || !argv[optind] || argv[optind][0] != '-' || !argv[optind][1])
        return -1;
    if (strcmp(argv[optind], "--") == 0) {
        optind++;
        return -1;
    }
    arg = argv[optind];
    optopt = arg[pos];
    o = strchr(optstring, optopt);
    if (!o || optopt == ':') {
        if (opterr)
            fprintf(stderr, "%s: illegal option -- %c\n", argv[0], optopt);
        if (!arg[++pos]) {
            optind++;
            pos = 1;
        }
        return '?';
    }
    if (o[1] == ':') {
        if (arg[pos + 1]) {
            optarg = (char *)arg + pos + 1;
        } else if (optind + 1 < argc) {
            optarg = argv[++optind];
        } else {
            if (opterr)
                fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], optopt);
            optind++;
            pos = 1;
            return '?';
        }
        optind++;
        pos = 1;
    } else {
        optarg = NULL;
        if (!arg[++pos]) {
            optind++;
            pos = 1;
        }
    }
    return optopt;
}
