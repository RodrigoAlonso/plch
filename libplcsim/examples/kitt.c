/* Runs a PLC program in real time and shows its outputs in the terminal.

   Usage: plcsim_kitt program.epg [wiring.sda] [seconds]

   With the kitt.epg sample it turns on the KITT input and shows the
   Knight Rider light sweeping across outputs Y16..Y23. */

#include <stdio.h>
#include <stdlib.h>
#include "plcsim.h"

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(unsigned ms) { Sleep(ms); }
#else
#include <time.h>
static void sleep_ms(unsigned ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}
#endif

int main(int argc, char *argv[])
{
    const char *sda = NULL;
    double seconds = 5;
    plcsim_t *plc;
    int kitt;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s program.epg [wiring.sda] [seconds]\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    for (int n = 2; n < argc; n++) {
        char *end;
        double value = strtod(argv[n], &end);
        if (*argv[n] != '\0' && *end == '\0') seconds = value;
        else sda = argv[n];
    }

    if ((plc = plcsim_create()) == NULL) return EXIT_FAILURE;
    if (plcsim_load_program_file(plc, argv[1]) != PLCSIM_OK ||
        (sda && plcsim_load_wiring_file(plc, sda) != PLCSIM_OK)) {
        fprintf(stderr, "%s\n", plcsim_last_error(plc));
        plcsim_destroy(plc);
        return EXIT_FAILURE;
    }
    printf("%u steps loaded\n", plcsim_step_count(plc));

    kitt = plcsim_find_io(plc, "KITT");
    if (kitt >= 0) plcsim_set(plc, (unsigned)kitt, 1);

    /* One scan every millisecond, display refreshed every 20 ms. */
    for (long ms = 0; ms < (long)(seconds * 1000); ms++) {
        plcsim_scan(plc);
        if (ms % 20 == 0) {
            printf("\r Y16..Y23 [");
            for (unsigned b = 16; b < 24; b++)
                putchar(plcsim_get(plc, PLCSIM_OUTPUT(b)) ? '#' : ' ');
            printf("]  scans: %llu",
                   (unsigned long long)plcsim_scan_count(plc));
            fflush(stdout);
        }
        sleep_ms(1);
    }
    printf("\n");
    plcsim_destroy(plc);
    return EXIT_SUCCESS;
}
