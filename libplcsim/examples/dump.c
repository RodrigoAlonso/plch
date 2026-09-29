/* Prints the ladder diagram and/or the instruction list of a PLC program.

   Usage: plcsim_dump [options] program.epg

     -l, --ladder   print the ladder diagram (default)
     -i, --ilist    print the instruction list
     -h, --help     show this help

   With both -i and -l, prints the instruction list and then the ladder. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plcsim.h"

static void usage(FILE *out, const char *program)
{
    fprintf(out,
            "Usage: %s [options] program.epg\n"
            "Prints the ladder diagram and/or the instruction list of a PLC "
            "program.\n"
            "\n"
            "  -l, --ladder   print the ladder diagram (default)\n"
            "  -i, --ilist    print the instruction list\n"
            "  -h, --help     show this help\n"
            "\n"
            "With both -i and -l, prints the instruction list and then the "
            "ladder.\n",
            program);
}

int main(int argc, char *argv[])
{
    unsigned what = 0;
    const char *file = NULL;
    plcsim_t *plc;
    int result;

    for (int n = 1; n < argc; n++) {
        const char *arg = argv[n];
        if (strcmp(arg, "-l") == 0 || strcmp(arg, "--ladder") == 0)
            what |= PLCSIM_DUMP_LADDER;
        else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--ilist") == 0)
            what |= PLCSIM_DUMP_LIST;
        else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        } else if (arg[0] == '-' && arg[1] != '\0') {
            fprintf(stderr, "%s: unknown option '%s'\n", argv[0], arg);
            usage(stderr, argv[0]);
            return EXIT_FAILURE;
        } else if (file == NULL)
            file = arg;
        else {
            fprintf(stderr, "%s: only one program file expected\n", argv[0]);
            usage(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (file == NULL) {
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }
    if (what == 0) what = PLCSIM_DUMP_LADDER;

    if ((plc = plcsim_create()) == NULL) return EXIT_FAILURE;
    if ((result = plcsim_load_program_file(plc, file)) == PLCSIM_OK)
        result = plcsim_dump(plc, stdout, what);
    else
        fprintf(stderr, "%s\n", plcsim_last_error(plc));
    plcsim_destroy(plc);
    return result == PLCSIM_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
