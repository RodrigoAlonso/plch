/* =================================================
   |                                               |
   |                PLC Simulation                 |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
/*
 * plcsim.h - Public API of the Hitachi EC-series PLC simulation engine.
 *
 * This header is self-contained: it depends only on the C standard library.
 * The API is plain C (usable from C and C++) and every object is reached
 * through an opaque handle, so the library can evolve without breaking ABI.
 *
 * Typical use:
 *
 *     plcsim_t *plc = plcsim_create();
 *     if (plcsim_load_program_file(plc, "kitt.epg") != PLCSIM_OK)
 *         fprintf(stderr, "%s\n", plcsim_last_error(plc));
 *     plcsim_set(plc, PLCSIM_INPUT(0), 1);
 *     for (;;) {
 *         plcsim_scan(plc);
 *         int lamp = plcsim_get(plc, PLCSIM_OUTPUT(16));
 *         ...
 *     }
 *     plcsim_destroy(plc);
 *
 * Addresses
 * ---------
 * All I/O is addressed with *internal* addresses 0 .. PLCSIM_MEMORY_SIZE-1,
 * the same numbers used in the .epg program files (IO-START section):
 *
 *     0x000 - 0x09F   inputs           (PLCSIM_INPUT(n))
 *     0x050 - 0x05F   link read area   (PLCSIM_LINK_READ_BASE)
 *     0x0A0 - 0x13F   outputs          (PLCSIM_OUTPUT(n))
 *     0x0F0 - 0x0FF   link write area  (PLCSIM_LINK_WRITE_BASE)
 *     0x140 - 0x35F   internal relays / data registers
 *
 * The simulator GUI and the external wiring (.sda) files show a different
 * "external" numbering in which each 16-point word takes 20 numbers
 * (0..15, 20..35, ...). Use plcsim_addr_to_external() and
 * plcsim_addr_from_external() to convert.
 *
 * Special relays maintained by the engine on every scan:
 *     0x342 (834)  toggles on every scan
 *     0x344 (836)  toggles every 0.5 s
 *     0x347 (839)  ON during the first scan after load/reset, then OFF
 *     0x35E (862)  always ON
 *
 * Time
 * ----
 * Times are expressed in centiseconds (1/100 s), the resolution of the
 * timer presets in the program files. By default the engine reads a
 * monotonic system clock; plcsim_set_clock() installs a custom one, which
 * makes execution fully deterministic.
 *
 * Threads
 * -------
 * Distinct instances are independent and may be used from different
 * threads. A single instance must not be used concurrently.
 */

#ifndef PLCSIM_H
#define PLCSIM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined(_WIN32) && !defined(PLCSIM_STATIC)
#  if defined(PLCSIM_BUILDING)
#    define PLCSIM_API __declspec(dllexport)
#  else
#    define PLCSIM_API __declspec(dllimport)
#  endif
#elif defined(PLCSIM_BUILDING) && defined(__GNUC__) && __GNUC__ >= 4
#  define PLCSIM_API __attribute__((visibility("default")))
#else
#  define PLCSIM_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* Version                                                                  */

#define PLCSIM_VERSION_MAJOR 0
#define PLCSIM_VERSION_MINOR 1
#define PLCSIM_VERSION_PATCH 0

/* Runtime version of the library, encoded as major*10000 + minor*100 + patch. */
PLCSIM_API unsigned plcsim_version(void);

/* ------------------------------------------------------------------------ */
/* Limits of the simulated controller (Hitachi EC series)                   */

#define PLCSIM_MAX_STEPS        1950u  /* program instructions             */
#define PLCSIM_MEMORY_SIZE       864u  /* I/O and internal relay addresses */
#define PLCSIM_TIMER_COUNT        96u  /* timers + counters                */

#define PLCSIM_INPUT_BASE      0x000u
#define PLCSIM_OUTPUT_BASE     0x0A0u
#define PLCSIM_LINK_READ_BASE  0x050u
#define PLCSIM_LINK_WRITE_BASE 0x0F0u
#define PLCSIM_LINK_SIZE          16u

#define PLCSIM_INPUT(n)  (PLCSIM_INPUT_BASE + (unsigned)(n))
#define PLCSIM_OUTPUT(n) (PLCSIM_OUTPUT_BASE + (unsigned)(n))

/* ------------------------------------------------------------------------ */
/* Status codes                                                             */

typedef enum plcsim_status {
    PLCSIM_OK         =  0,
    PLCSIM_ERR_ARG    = -1,  /* NULL handle, bad argument                 */
    PLCSIM_ERR_IO     = -2,  /* file could not be opened or read          */
    PLCSIM_ERR_FORMAT = -3,  /* malformed program or wiring file          */
    PLCSIM_ERR_RANGE  = -4,  /* address or index out of range             */
    PLCSIM_ERR_NOMEM  = -5   /* out of memory                             */
} plcsim_status;

/* Warning flags returned by plcsim_scan(). The scan always runs to the end. */
#define PLCSIM_WARN_MC_NESTING 0x1  /* more than 3 nested Master Controls */

/* ------------------------------------------------------------------------ */
/* Lifecycle                                                                */

typedef struct plcsim plcsim_t;

/* Creates a PLC with no program loaded. Returns NULL if out of memory. */
PLCSIM_API plcsim_t *plcsim_create(void);
PLCSIM_API void plcsim_destroy(plcsim_t *plc);

/* Human readable description of the last error reported by this instance
   (never NULL; empty string if there was none). */
PLCSIM_API const char *plcsim_last_error(const plcsim_t *plc);

/* ------------------------------------------------------------------------ */
/* Loading                                                                  */

/* Loads an ACTSIP-E program (.epg). On success the program replaces the
   current one and the PLC is reset; on failure the PLC is left untouched.
   The external wiring, if any, is kept. */
PLCSIM_API int plcsim_load_program_file(plcsim_t *plc, const char *path);
PLCSIM_API int plcsim_load_program_mem(plcsim_t *plc, const char *data,
                                       size_t size);

/* Loads an external wiring description (.sda): delayed feedback from
   outputs to inputs ([temporizadores]), selector switches ([selectoras])
   and link presence ([link]). The [colores] section is accepted and
   ignored. Replaces any previously loaded wiring; on failure the PLC is
   left untouched. */
PLCSIM_API int plcsim_load_wiring_file(plcsim_t *plc, const char *path);
PLCSIM_API int plcsim_load_wiring_mem(plcsim_t *plc, const char *data,
                                      size_t size);
PLCSIM_API void plcsim_clear_wiring(plcsim_t *plc);

/* 1 if the address is a position of a selector switch of the wiring,
   0 if not, PLCSIM_ERR_* on error. */
PLCSIM_API int plcsim_is_selector(const plcsim_t *plc, unsigned addr);

/* Number of instructions of the loaded program (0 if none). */
PLCSIM_API unsigned plcsim_step_count(const plcsim_t *plc);

/* Program listing, e.g. to display the program as ladder. */
#define PLCSIM_OP_ORG     0x01u
#define PLCSIM_OP_ORG_NOT 0x41u
#define PLCSIM_OP_STR     0x02u
#define PLCSIM_OP_STR_NOT 0x42u
#define PLCSIM_OP_AND     0x04u
#define PLCSIM_OP_AND_NOT 0x44u
#define PLCSIM_OP_OR      0x20u
#define PLCSIM_OP_OR_NOT  0x60u
#define PLCSIM_OP_AND_STR 0x06u
#define PLCSIM_OP_OR_STR  0x22u
#define PLCSIM_OP_OUT     0x08u
#define PLCSIM_OP_OUT_NOT 0x48u

typedef struct plcsim_step {
    int      is_function; /* 0 = instruction, 1 = function                */
    unsigned opcode;      /* instruction: PLCSIM_OP_*; function: number    */
    unsigned operand;     /* address, timer index, or constant (BCD) of a
                             dot function                                  */
    int      timer;       /* instruction whose operand is a timer/counter  */
    int      dot;         /* dot function (0. to 9.)                       */
} plcsim_step;

PLCSIM_API int plcsim_get_step(const plcsim_t *plc, unsigned index,
                               plcsim_step *out);

/* ------------------------------------------------------------------------ */
/* Execution                                                                */

/* Returns all I/O, relays, timers, counters and registers to their
   power-on state, and puts selector switches in their default position. */
PLCSIM_API void plcsim_reset(plcsim_t *plc);

/* Executes one complete scan of the program followed by one step of the
   external wiring. Returns 0 or a combination of PLCSIM_WARN_* flags, or
   PLCSIM_ERR_ARG. Same as plcsim_run(plc, PLCSIM_RUN_ALL). */
PLCSIM_API int plcsim_scan(plcsim_t *plc);

/* Runs only some parts of a scan, e.g. to pause the program while the
   external wiring keeps working. Returns like plcsim_scan(). */
#define PLCSIM_RUN_PROGRAM 0x1  /* program scan and special relays */
#define PLCSIM_RUN_WIRING  0x2  /* external wiring step            */
#define PLCSIM_RUN_ALL     (PLCSIM_RUN_PROGRAM | PLCSIM_RUN_WIRING)
PLCSIM_API int plcsim_run(plcsim_t *plc, unsigned what);

/* Number of program scans executed since the last load or reset. */
PLCSIM_API uint64_t plcsim_scan_count(const plcsim_t *plc);

/* Clock returning the current time in centiseconds. It only has to be
   monotonic; wrap-around of the 32-bit value is handled. */
typedef uint32_t (*plcsim_clock_fn)(void *user);

/* Installs a custom clock; NULL restores the default monotonic clock. */
PLCSIM_API void plcsim_set_clock(plcsim_t *plc, plcsim_clock_fn clock,
                                 void *user);

/* ------------------------------------------------------------------------ */
/* I/O and memory                                                           */

/* Logical state of an address: 1 = ON, 0 = OFF, PLCSIM_ERR_* on error. */
PLCSIM_API int plcsim_get(const plcsim_t *plc, unsigned addr);

/* Forces an address ON (on != 0) or OFF. Turning ON a position of a
   selector switch turns OFF the other positions of the same selector. */
PLCSIM_API int plcsim_set(plcsim_t *plc, unsigned addr, int on);

/* Raw byte stored at an address (data functions store full bytes; a relay
   is ON when bit 7 is set). Returns 0..255 or PLCSIM_ERR_*. */
PLCSIM_API int plcsim_get_byte(const plcsim_t *plc, unsigned addr);
PLCSIM_API int plcsim_set_byte(plcsim_t *plc, unsigned addr, uint8_t value);

/* Internal registers: arithmetic (AR), extension (ER) and carry (CR). */
typedef struct plcsim_registers {
    uint16_t ar;
    uint16_t er;
    int      carry;
} plcsim_registers;

PLCSIM_API int plcsim_get_registers(const plcsim_t *plc, plcsim_registers *out);

/* ------------------------------------------------------------------------ */
/* Timers and counters                                                      */

typedef struct plcsim_timer_info {
    int      is_counter; /* 0 = timer, 1 = counter                         */
    uint32_t preset;     /* timers: centiseconds; counters: count          */
    uint32_t value;      /* timers: elapsed cs while running; counters:
                            current count                                  */
    int      done;       /* contact state: 1 = ON                          */
} plcsim_timer_info;

PLCSIM_API int plcsim_get_timer(const plcsim_t *plc, unsigned index,
                                plcsim_timer_info *out);

/* ------------------------------------------------------------------------ */
/* Symbols from the program file                                            */

/* Name (up to 10 chars) and comment (up to 30 chars) of an address, with
   trailing blanks removed. Return "" if undefined, NULL if out of range. */
PLCSIM_API const char *plcsim_io_name(const plcsim_t *plc, unsigned addr);
PLCSIM_API const char *plcsim_io_comment(const plcsim_t *plc, unsigned addr);
PLCSIM_API const char *plcsim_timer_name(const plcsim_t *plc, unsigned index);

/* Address of the I/O with the given name (case-insensitive), or
   PLCSIM_ERR_RANGE if not found. */
PLCSIM_API int plcsim_find_io(const plcsim_t *plc, const char *name);

/* Conversion between internal addresses and the external numbering used by
   the GUI and wiring files. Return PLCSIM_ERR_RANGE if out of range. */
PLCSIM_API int plcsim_addr_to_external(unsigned addr);
PLCSIM_API int plcsim_addr_from_external(unsigned external);

/* ------------------------------------------------------------------------ */
/* Program listings                                                         */

#define PLCSIM_DUMP_LIST   0x1  /* instruction list */
#define PLCSIM_DUMP_LADDER 0x2  /* ladder diagram   */
#define PLCSIM_DUMP_ALL    (PLCSIM_DUMP_LIST | PLCSIM_DUMP_LADDER)

/* Writes a text listing of the loaded program: its instruction list and/or
   its ladder diagram. I/O addresses use the external numbering; timers and
   counters are shown as T or C followed by their octal number, as in
   program files. Returns PLCSIM_OK or PLCSIM_ERR_*. */
PLCSIM_API int plcsim_dump(const plcsim_t *plc, FILE *out, unsigned what);

/* Receives successive pieces of text (each one NUL terminated). */
typedef void (*plcsim_write_fn)(void *user, const char *text);

/* Same as plcsim_dump(), delivering the text to a callback. On Windows use
   this one when the program and the library may use different C runtimes,
   which cannot share a FILE *. */
PLCSIM_API int plcsim_dump_to(const plcsim_t *plc, plcsim_write_fn write,
                              void *user, unsigned what);

/* ------------------------------------------------------------------------ */
/* Link between PLCs                                                        */

/* Marks a PLC as connected (or not) to the link. Wiring files can also set
   it through their [link] section. Disconnected by default. */
PLCSIM_API void plcsim_set_linked(plcsim_t *plc, int linked);
PLCSIM_API int  plcsim_is_linked(const plcsim_t *plc);

/* Exchanges link data among the connected PLCs of the array: the link read
   area of each one receives the OR of the link write areas of all of them.
   Call it between scans. */
PLCSIM_API void plcsim_link_update(plcsim_t *const *plcs, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* PLCSIM_H */
