/* getopt.h - the POSIX command line parser, for Windows */
#ifndef PERFORMER_GETOPT_H
#define PERFORMER_GETOPT_H
extern char *optarg;
extern int optind, opterr, optopt;
int getopt(int argc, char *const argv[], const char *optstring);
#endif
