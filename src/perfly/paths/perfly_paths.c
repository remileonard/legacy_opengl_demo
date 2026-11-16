/*
 * perfly_paths.c - the vehicles of the later perfly, around the unchanged
 * perfly of Performer 2.0.
 *
 * The perfly of the town demo takes "-A model.flt,file.path,Name": a model
 * that moves along a path. In the perfly of Performer 2.0, -A is the
 * welcome text. This main takes out the -A options holding two commas
 * (model, path, name) before the original main (common/main.c, compiled
 * as perfly_main) reads the command line; the other -A stay the welcome
 * text. The vehicles are run by the Performer layer (pfu_path.c).
 */
#include <stdlib.h>
#include <string.h>

int perfly_main(int argc, char *argv[]);
int pfuiAddVehicle(const char *model, const char *pathfile, const char *name);

/* "model,path,name": 1 when it is a vehicle, added */
static int vehicle(const char *arg)
{
    char buf[1024], *c1, *c2;
    if (strlen(arg) >= sizeof(buf))
        return 0;
    strcpy(buf, arg);
    if ((c1 = strchr(buf, ',')) == NULL || (c2 = strchr(c1 + 1, ',')) == NULL)
        return 0;
    *c1 = *c2 = '\0';
    if (!buf[0] || !c1[1])
        return 0;
    return pfuiAddVehicle(buf, c1 + 1, c2 + 1) >= 0;
}

int main(int argc, char *argv[])
{
    char **args = (char **)malloc(sizeof(char *) * (argc + 1));
    int i, n = 0;
    for (i = 0; i < argc; i++) {
        if (i > 0 && strcmp(argv[i], "-A") == 0 && i + 1 < argc && vehicle(argv[i + 1])) {
            i++;
            continue;
        }
        if (i > 0 && strncmp(argv[i], "-A", 2) == 0 && argv[i][2] && vehicle(argv[i] + 2))
            continue;
        args[n++] = argv[i];
    }
    args[n] = NULL;
    return perfly_main(n, args);
}
