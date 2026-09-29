/* Tests of the plcsim public API. Written in C on purpose: it also checks
   that the library can be consumed from C with nothing but plcsim.h.

   Usage: plcsim_test [directory with test.epg and kitt.epg] */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plcsim.h"

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            fprintf(stderr, "%s:%d: FAILED: %s\n", __FILE__, __LINE__,  \
                    #cond);                                             \
            failures++;                                                   \
        }                                                               \
    } while (0)

/* Fake clock, in centiseconds. */
static uint32_t now_cs = 1000;
static uint32_t fake_clock(void *user) { (void)user; return now_cs; }

static plcsim_t *new_plc(void)
{
    plcsim_t *p = plcsim_create();
    plcsim_set_clock(p, fake_clock, NULL);
    return p;
}

/* ------------------------------------------------------------------------ */
/* Builders of .epg programs in memory                                      */

static char program_text[16384];

static void begin_program(void)
{
    strcpy(program_text,
           "TEST\n2.12\nTEST\n*\n*\n*\n*\n*\n"
           "PGM-START\n0000:1969:004096:0065\nffffff09900000\n");
}

static unsigned step_number = 0;

/* Instruction: opcode (add 0x80 for a timer operand) and hex operand. */
static void inst(unsigned opcode, unsigned operand)
{
    char line[32];
    sprintf(line, "%04u%02x-010%04x\n", step_number++ % 10000, opcode, operand);
    strcat(program_text, line);
}

/* Function number (dot functions 0. to 9. with dot=1) and operand. */
static void func(unsigned number, int dot, unsigned operand)
{
    char line[32];
    sprintf(line, "%04u10%c%02u0%04x\n", step_number++ % 10000, dot ? '3' : '0',
            number, operand);
    strcat(program_text, line);
}

/* Ends the program. 'tc' is an optional TC section body (may be NULL). */
static int load_program(plcsim_t *p, const char *tc)
{
    strcat(program_text, "PGM-END\n");
    if (tc) {
        strcat(program_text, "TC-START\n000:137/137\n");
        strcat(program_text, tc);
        strcat(program_text, "TC-END\n");
    }
    step_number = 0;
    return plcsim_load_program_mem(p, program_text, strlen(program_text));
}

/* TC section line: octal index, preset, timer or counter. */
static const char *tc_line(unsigned number, unsigned preset, int counter)
{
    static char line[128];
    if (counter)
        sprintf(line, "%03o%-40s%04u004\n", number, "-", preset);
    else
        sprintf(line, "%03o%-40s%05u05\n", number, "-", preset);
    return line;
}

#define X(n)   PLCSIM_INPUT(n)
#define Y(n)   PLCSIM_OUTPUT(n)
#define R(n)   (0x140u + (n))
#define ALWAYS 0x35Eu

enum { ORG = 0x01, ORG_NOT = 0x41, STR = 0x02, AND = 0x04, AND_NOT = 0x44,
       OR = 0x20, AND_STR = 0x06, OR_STR = 0x22, OUT = 0x08, OUT_NOT = 0x48,
       TIMER = 0x80 };

/* ------------------------------------------------------------------------ */

static void test_basic(void)
{
    plcsim_t *p = new_plc();
    CHECK(plcsim_version() == PLCSIM_VERSION_MAJOR * 10000 +
                              PLCSIM_VERSION_MINOR * 100 + PLCSIM_VERSION_PATCH);
    CHECK(p != NULL);
    CHECK(plcsim_step_count(p) == 0);
    CHECK(plcsim_scan(p) == 0);
    CHECK(plcsim_scan_count(p) == 1);

    CHECK(plcsim_set(p, 5, 1) == PLCSIM_OK);
    CHECK(plcsim_get(p, 5) == 1);
    CHECK(plcsim_get_byte(p, 5) == 0x80);
    CHECK(plcsim_set_byte(p, 5, 0x7F) == PLCSIM_OK);
    CHECK(plcsim_get(p, 5) == 0);
    CHECK(plcsim_get(p, PLCSIM_MEMORY_SIZE) == PLCSIM_ERR_RANGE);
    CHECK(plcsim_set(p, PLCSIM_MEMORY_SIZE, 1) == PLCSIM_ERR_RANGE);
    CHECK(plcsim_get(NULL, 0) == PLCSIM_ERR_ARG);
    CHECK(plcsim_scan(NULL) == PLCSIM_ERR_ARG);

    plcsim_reset(p);
    CHECK(plcsim_get(p, 5) == 0);
    CHECK(plcsim_scan_count(p) == 0);

    CHECK(plcsim_addr_to_external(Y(0)) == 200);
    CHECK(plcsim_addr_to_external(17) == 21);
    CHECK(plcsim_addr_from_external(200) == (int)Y(0));
    CHECK(plcsim_addr_from_external(21) == 17);
    CHECK(plcsim_addr_from_external(16) == PLCSIM_ERR_RANGE);
    CHECK(plcsim_addr_to_external(PLCSIM_MEMORY_SIZE) == PLCSIM_ERR_RANGE);
    plcsim_destroy(p);
}

static void test_special_relays(void)
{
    plcsim_t *p = new_plc();
    int toggle;

    CHECK(plcsim_get(p, 839) == 1);   /* first scan */
    plcsim_scan(p);
    CHECK(plcsim_get(p, 839) == 0);
    CHECK(plcsim_get(p, ALWAYS) == 1);
    toggle = plcsim_get(p, 834);
    plcsim_scan(p);
    CHECK(plcsim_get(p, 834) == !toggle);

    /* 836 toggles when more than 50 cs elapsed since the last toggle. */
    toggle = plcsim_get(p, 836);
    now_cs += 50;
    plcsim_scan(p);
    CHECK(plcsim_get(p, 836) == toggle);
    now_cs += 1;
    plcsim_scan(p);
    CHECK(plcsim_get(p, 836) == !toggle);

    plcsim_reset(p);
    CHECK(plcsim_get(p, 839) == 1);
    plcsim_destroy(p);
}

static void test_contacts(void)
{
    plcsim_t *p = new_plc();
    /* Y0 = (X0 & !X1) | X2 ;  Y1 = !(X0 & X3) */
    begin_program();
    inst(ORG, X(0)); inst(AND_NOT, X(1)); inst(OR, X(2)); inst(OUT, Y(0));
    inst(ORG, X(0)); inst(AND, X(3)); inst(OUT_NOT, Y(1));
    CHECK(load_program(p, NULL) == PLCSIM_OK);
    CHECK(plcsim_step_count(p) == 7);
    {
        plcsim_step s;
        CHECK(plcsim_get_step(p, 1, &s) == PLCSIM_OK);
        CHECK(!s.is_function && s.opcode == PLCSIM_OP_AND_NOT &&
              s.operand == X(1) && !s.timer);
        CHECK(plcsim_get_step(p, 7, &s) == PLCSIM_ERR_RANGE);
    }

    for (unsigned v = 0; v < 16; v++) {
        int x0 = v & 1, x1 = (v >> 1) & 1, x2 = (v >> 2) & 1, x3 = (v >> 3) & 1;
        plcsim_set(p, X(0), x0);
        plcsim_set(p, X(1), x1);
        plcsim_set(p, X(2), x2);
        plcsim_set(p, X(3), x3);
        plcsim_scan(p);
        CHECK(plcsim_get(p, Y(0)) == ((x0 && !x1) || x2));
        CHECK(plcsim_get(p, Y(1)) == !(x0 && x3));
    }
    plcsim_destroy(p);
}

static void test_timer(void)
{
    plcsim_t *p = new_plc();
    plcsim_timer_info ti;
    /* T0 (preset 0.20 s) enabled by X0, Y0 follows T0. */
    begin_program();
    inst(ORG, X(0)); inst(OUT | TIMER, 0);
    inst(ORG | TIMER, 0); inst(OUT, Y(0));
    CHECK(load_program(p, tc_line(0, 20, 0)) == PLCSIM_OK);
    {
        plcsim_step s;
        CHECK(plcsim_get_step(p, 1, &s) == PLCSIM_OK);
        CHECK(!s.is_function && s.opcode == PLCSIM_OP_OUT && s.timer);
    }
    CHECK(plcsim_get_timer(p, 0, &ti) == PLCSIM_OK);
    CHECK(!ti.is_counter && ti.preset == 20 && ti.value == 0 && !ti.done);

    plcsim_set(p, X(0), 1);
    plcsim_scan(p);                         /* starts at t0 */
    now_cs += 20;
    plcsim_scan(p);                         /* exactly the preset: not yet */
    CHECK(plcsim_get(p, Y(0)) == 0);
    plcsim_get_timer(p, 0, &ti);
    CHECK(ti.value == 20);
    now_cs += 1;
    plcsim_scan(p);
    CHECK(plcsim_get(p, Y(0)) == 1);
    plcsim_get_timer(p, 0, &ti);
    CHECK(ti.done);

    plcsim_set(p, X(0), 0);
    plcsim_scan(p);
    CHECK(plcsim_get(p, Y(0)) == 0);
    plcsim_get_timer(p, 0, &ti);
    CHECK(ti.value == 0 && !ti.done);

    /* Clock wrap-around. */
    now_cs = 0xFFFFFFF0u;
    plcsim_set(p, X(0), 1);
    plcsim_scan(p);
    now_cs += 30;
    plcsim_scan(p);
    CHECK(plcsim_get(p, Y(0)) == 1);
    now_cs = 1000;
    plcsim_destroy(p);
}

static void test_counter(void)
{
    plcsim_t *p = new_plc();
    plcsim_timer_info ti;
    /* C1 (preset 3): count X0, reset X1. Y0 follows C1. */
    begin_program();
    inst(ORG, X(0)); inst(STR, X(1)); inst(OUT | TIMER, 1);
    inst(ORG | TIMER, 1); inst(OUT, Y(0));
    CHECK(load_program(p, tc_line(1, 3, 1)) == PLCSIM_OK);
    plcsim_get_timer(p, 1, &ti);
    CHECK(ti.is_counter && ti.preset == 3);

    for (int n = 1; n <= 4; n++) {
        plcsim_set(p, X(0), 1);
        plcsim_scan(p);
        plcsim_scan(p);                     /* held: counts only once */
        plcsim_set(p, X(0), 0);
        plcsim_scan(p);
        plcsim_get_timer(p, 1, &ti);
        CHECK(ti.value == (uint32_t)(n < 3 ? n : 3));
        CHECK(plcsim_get(p, Y(0)) == (n >= 3));
    }
    plcsim_set(p, X(1), 1);
    plcsim_scan(p);
    plcsim_get_timer(p, 1, &ti);
    CHECK(ti.value == 0 && !ti.done);
    plcsim_destroy(p);
}

static void test_edges(void)
{
    plcsim_t *p = new_plc();
    /* R0 = rising edge of X0, R1 = trailing edge of X0 */
    begin_program();
    inst(ORG, X(0)); func(0, 0, R(0));
    inst(ORG, X(0)); func(1, 0, R(1));
    CHECK(load_program(p, NULL) == PLCSIM_OK);

    /* Input already ON at start: no edge until it goes OFF first. */
    plcsim_set(p, X(0), 1);
    plcsim_scan(p);
    CHECK(plcsim_get(p, R(0)) == 0);
    plcsim_set(p, X(0), 0);
    plcsim_scan(p);
    CHECK(plcsim_get(p, R(0)) == 0);
    CHECK(plcsim_get(p, R(1)) == 1);
    plcsim_scan(p);
    CHECK(plcsim_get(p, R(1)) == 0);

    plcsim_set(p, X(0), 1);
    plcsim_scan(p);
    CHECK(plcsim_get(p, R(0)) == 1);
    plcsim_scan(p);
    CHECK(plcsim_get(p, R(0)) == 0);
    plcsim_scan(p);
    CHECK(plcsim_get(p, R(0)) == 0);

    /* Reset restores the initial edge lock. */
    plcsim_reset(p);
    plcsim_set(p, X(0), 1);
    plcsim_scan(p);
    CHECK(plcsim_get(p, R(0)) == 0);
    plcsim_destroy(p);
}

static void test_master_control(void)
{
    plcsim_t *p = new_plc();
    /* MCS X1 { Y0 = X0 } MCR ; Y1 = X0 */
    begin_program();
    inst(ORG, X(1)); func(4, 0, 0xFFFF);
    inst(ORG, X(0)); inst(OUT, Y(0));
    func(5, 0, 0xFFFF);
    inst(ORG, X(0)); inst(OUT, Y(1));
    CHECK(load_program(p, NULL) == PLCSIM_OK);

    plcsim_set(p, X(0), 1);
    CHECK(plcsim_scan(p) == 0);
    CHECK(plcsim_get(p, Y(0)) == 0);
    CHECK(plcsim_get(p, Y(1)) == 1);
    plcsim_set(p, X(1), 1);
    plcsim_scan(p);
    CHECK(plcsim_get(p, Y(0)) == 1);

    /* More than 3 nested master controls. */
    begin_program();
    for (int n = 0; n < 4; n++) { inst(ORG, ALWAYS); func(4, 0, 0); }
    CHECK(load_program(p, NULL) == PLCSIM_OK);
    CHECK(plcsim_scan(p) & PLCSIM_WARN_MC_NESTING);
    plcsim_destroy(p);
}

static void test_functions(void)
{
    plcsim_t *p = new_plc();
    plcsim_registers r;

    /* 0. 123 (converted to BCD 0x0123), 1. +877 = 1000 -> R0..R1 (fn 21) */
    begin_program();
    inst(ORG, ALWAYS);
    func(0, 1, 123); func(1, 1, 877); func(21, 0, R(0));
    CHECK(load_program(p, NULL) == PLCSIM_OK);
    {
        plcsim_step s;
        CHECK(plcsim_get_step(p, 1, &s) == PLCSIM_OK);
        CHECK(s.is_function && s.dot && s.opcode == 0 && s.operand == 0x0123);
        CHECK(plcsim_get_step(p, 3, &s) == PLCSIM_OK);
        CHECK(s.is_function && !s.dot && s.opcode == 21);
    }
    plcsim_scan(p);
    plcsim_get_registers(p, &r);
    CHECK(r.ar == 0x1000 && !r.carry);
    CHECK(plcsim_get_byte(p, R(0)) == 0x10 && plcsim_get_byte(p, R(1)) == 0x00);

    /* Load a word, AND / OR with words, compare. */
    begin_program();
    inst(ORG, ALWAYS);
    func(10, 0, R(0));                       /* AR = 0xF0F0 */
    func(15, 0, R(2));                       /* AR &= 0x3C3C -> 0x3030 */
    func(16, 0, R(4));                       /* AR |= 0x0101 -> 0x3131 */
    func(21, 0, R(6));
    func(18, 0, R(6)); func(23, 0, R(8));    /* AR == R6 -> CR -> R8 */
    CHECK(load_program(p, NULL) == PLCSIM_OK);
    plcsim_set_byte(p, R(0), 0xF0); plcsim_set_byte(p, R(1), 0xF0);
    plcsim_set_byte(p, R(2), 0x3C); plcsim_set_byte(p, R(3), 0x3C);
    plcsim_set_byte(p, R(4), 0x01); plcsim_set_byte(p, R(5), 0x01);
    plcsim_scan(p);
    plcsim_get_registers(p, &r);
    CHECK(r.ar == 0x3131);
    CHECK(plcsim_get(p, R(8)) == 1);

    /* 16 bit registers: shift and multiply overflow. */
    begin_program();
    inst(ORG, ALWAYS);
    func(10, 0, R(0)); func(26, 0, 0); func(23, 0, R(8));  /* 0x8001 << 1 */
    func(22, 0, R(16));                                    /* AR -> 16 relays */
    func(10, 0, R(2)); func(63, 0, R(4));                  /* 0x1000 * 0x0100 */
    CHECK(load_program(p, NULL) == PLCSIM_OK);
    plcsim_set_byte(p, R(0), 0x80); plcsim_set_byte(p, R(1), 0x01);
    plcsim_set_byte(p, R(2), 0x10); plcsim_set_byte(p, R(3), 0x00);
    plcsim_set_byte(p, R(4), 0x01); plcsim_set_byte(p, R(5), 0x00);
    plcsim_scan(p);
    CHECK(plcsim_get(p, R(8)) == 1);
    CHECK(plcsim_get(p, R(16)) == 0 && plcsim_get(p, R(17)) == 1);
    for (unsigned b = 2; b < 16; b++) CHECK(plcsim_get(p, R(16 + b)) == 0);
    plcsim_get_registers(p, &r);
    CHECK(r.ar == 0x0000 && r.er == 0x0010 && r.carry);

    /* Functions are conditional on their rung. */
    begin_program();
    inst(ORG, X(0)); func(0, 1, 42);
    CHECK(load_program(p, NULL) == PLCSIM_OK);
    plcsim_scan(p);
    plcsim_get_registers(p, &r);
    CHECK(r.ar == 0);
    plcsim_set(p, X(0), 1);
    plcsim_scan(p);
    plcsim_get_registers(p, &r);
    CHECK(r.ar == 0x0042);
    plcsim_destroy(p);
}

static void test_errors(void)
{
    plcsim_t *p = new_plc();
    const char *bad;

    begin_program();
    inst(ORG, X(0)); inst(OUT, Y(0));
    CHECK(load_program(p, NULL) == PLCSIM_OK);

    bad = "not a program\n";
    CHECK(plcsim_load_program_mem(p, bad, strlen(bad)) == PLCSIM_ERR_FORMAT);
    CHECK(strlen(plcsim_last_error(p)) > 0);
    CHECK(plcsim_step_count(p) == 2);      /* previous program kept */

    begin_program();
    inst(ORG, X(0));
    strcat(program_text, "TC-START\n");        /* no PGM-END */
    CHECK(plcsim_load_program_mem(p, program_text, strlen(program_text)) ==
          PLCSIM_ERR_FORMAT);

    begin_program();
    inst(ORG, PLCSIM_MEMORY_SIZE); inst(OUT, Y(0));
    CHECK(load_program(p, NULL) == PLCSIM_ERR_FORMAT);
    begin_program();
    inst(ORG, X(0)); inst(OUT | TIMER, PLCSIM_TIMER_COUNT);
    CHECK(load_program(p, NULL) == PLCSIM_ERR_FORMAT);
    begin_program();
    inst(ORG, X(0)); func(22, 0, PLCSIM_MEMORY_SIZE - 8);
    CHECK(load_program(p, NULL) == PLCSIM_ERR_FORMAT);
    CHECK(plcsim_step_count(p) == 2);

    CHECK(plcsim_load_program_file(p, "/does/not/exist.epg") == PLCSIM_ERR_IO);
    CHECK(strstr(plcsim_last_error(p), "/does/not/exist.epg") != NULL);
    CHECK(plcsim_load_wiring_mem(p, "[unknown]", 9) == PLCSIM_ERR_FORMAT);
    CHECK(plcsim_load_wiring_mem(p, "", 0) == PLCSIM_ERR_FORMAT);
    plcsim_destroy(p);
}

static void test_wiring(void)
{
    plcsim_t *p = new_plc();
    /* Y0 = X0. External numbering: 200 = Y0, 5 = X5, 21 = X17. */
    const char *wiring_text =
        "// test wiring\n"
        "[Temporizadores]\n"
        "  200, 30, 5      /* Y0 ON 0.30 s -> X5 ON */\n"
        "  -200 10 -5\n"
        "[selectoras]\n"
        "  inicio 1 2 21 fin\n"
        "[link] presente\n"
        "[colores]\n"
        "  200 verde\n"
        "  1 rojo 0\n";

    begin_program();
    inst(ORG, X(0)); inst(OUT, Y(0));
    CHECK(load_program(p, NULL) == PLCSIM_OK);
    CHECK(plcsim_load_wiring_mem(p, wiring_text, strlen(wiring_text)) == PLCSIM_OK);
    CHECK(plcsim_is_linked(p));

    CHECK(plcsim_is_selector(p, X(17)) == 1);
    CHECK(plcsim_is_selector(p, X(3)) == 0);
    CHECK(plcsim_is_selector(p, PLCSIM_MEMORY_SIZE) == PLCSIM_ERR_RANGE);

    /* Selector: default position, exclusive positions. */
    CHECK(plcsim_get(p, X(1)) == 1);
    plcsim_set(p, X(17), 1);
    CHECK(plcsim_get(p, X(1)) == 0 && plcsim_get(p, X(17)) == 1);
    plcsim_reset(p);
    CHECK(plcsim_get(p, X(1)) == 1 && plcsim_get(p, X(17)) == 0);

    /* Delayed feedback from Y0 to X5. */
    plcsim_set(p, X(0), 1);
    plcsim_scan(p);
    now_cs += 30;
    plcsim_scan(p);
    CHECK(plcsim_get(p, X(5)) == 0);
    now_cs += 1;
    plcsim_scan(p);
    CHECK(plcsim_get(p, X(5)) == 1);

    plcsim_set(p, X(0), 0);
    plcsim_scan(p);
    now_cs += 11;
    plcsim_scan(p);
    CHECK(plcsim_get(p, X(5)) == 0);

    /* Wiring alone, with the program stopped. */
    plcsim_set(p, X(0), 1);
    plcsim_run(p, PLCSIM_RUN_PROGRAM);
    plcsim_set(p, X(0), 0);
    plcsim_run(p, PLCSIM_RUN_WIRING);
    now_cs += 31;
    plcsim_run(p, PLCSIM_RUN_WIRING);
    CHECK(plcsim_get(p, Y(0)) == 1 && plcsim_get(p, X(5)) == 1);
    /* Program alone: the wiring does not react. */
    plcsim_set(p, X(5), 0);
    plcsim_run(p, PLCSIM_RUN_PROGRAM);
    now_cs += 31;
    plcsim_run(p, PLCSIM_RUN_PROGRAM);
    CHECK(plcsim_get(p, Y(0)) == 0 && plcsim_get(p, X(5)) == 0);

    /* The program survives loading wiring and vice versa. */
    CHECK(plcsim_step_count(p) == 2);
    plcsim_clear_wiring(p);
    plcsim_set(p, X(2), 1);
    CHECK(plcsim_get(p, X(1)) == 1);
    plcsim_destroy(p);
}

static void test_link(void)
{
    plcsim_t *a = new_plc(), *b = new_plc(), *c = new_plc();
    plcsim_t *plcs[3];

    plcs[0] = a; plcs[1] = b; plcs[2] = c;
    plcsim_set_linked(a, 1);
    plcsim_set_linked(b, 1);
    plcsim_set_byte(a, PLCSIM_LINK_WRITE_BASE, 0x81);
    plcsim_set_byte(b, PLCSIM_LINK_WRITE_BASE, 0x02);
    plcsim_set_byte(b, PLCSIM_LINK_WRITE_BASE + 15, 0x80);
    plcsim_set_byte(c, PLCSIM_LINK_WRITE_BASE, 0x04);  /* not linked */
    plcsim_link_update(plcs, 3);
    CHECK(plcsim_get_byte(a, PLCSIM_LINK_READ_BASE) == 0x83);
    CHECK(plcsim_get_byte(b, PLCSIM_LINK_READ_BASE) == 0x83);
    CHECK(plcsim_get(a, PLCSIM_LINK_READ_BASE + 15) == 1);
    CHECK(plcsim_get_byte(c, PLCSIM_LINK_READ_BASE) == 0);
    plcsim_destroy(a);
    plcsim_destroy(b);
    plcsim_destroy(c);
}

/* Collects the text of a dump. */
static char dump_text[16384];

static void collect(void *user, const char *text)
{
    (void)user;
    if (strlen(dump_text) + strlen(text) < sizeof(dump_text))
        strcat(dump_text, text);
}

static void test_dump(void)
{
    plcsim_t *p = new_plc();
    static const char expected[] =
        "Instruction list (10 steps)\n"
        "\n"
        "Step  Instruction  Operand  Comment\n"
        "0000  ORG          000\n"
        "0001  OUT          200\n"
        "0002  AND          001\n"
        "0003  OUT          201\n"
        "0004  AND NOT      002\n"
        "0005  OUT          202\n"
        "0006  OUT          203\n"
        "0007  ORG          006\n"
        "0008  STR          007\n"
        "0009  OUT          C001     count 3\n"
        "\n"
        "Ladder diagram\n"
        "\n"
        "0000  |-[ 000 ]-+----------------------( 200 )\n"
        "      |         +-[ 001 ]-+------------( 201 )\n"
        "      |                   +-[/002 ]--+-( 202 )\n"
        "      |                              +-( 203 )\n"
        "      |\n"
        "0007  |-[ 006 ]------------------------(CNT C001)\n"
        "      |-[ 007 ]------------------------(RST C001)\n"
        "      |\n";

    dump_text[0] = '\0';
    CHECK(plcsim_dump_to(p, collect, NULL, PLCSIM_DUMP_ALL) == PLCSIM_OK);
    CHECK(strcmp(dump_text, "No program loaded\n") == 0);

    /* Outputs branching off each other, stacked coils and a counter. */
    begin_program();
    inst(ORG, X(0)); inst(OUT, Y(0));
    inst(AND, X(1)); inst(OUT, Y(1));
    inst(AND_NOT, X(2)); inst(OUT, Y(2)); inst(OUT, Y(3));
    inst(ORG, X(6)); inst(STR, X(7)); inst(OUT | TIMER, 1);
    CHECK(load_program(p, tc_line(1, 3, 1)) == PLCSIM_OK);
    dump_text[0] = '\0';
    CHECK(plcsim_dump_to(p, collect, NULL, PLCSIM_DUMP_ALL) == PLCSIM_OK);
    if (strcmp(dump_text, expected) != 0) {
        fprintf(stderr, "dump:\n%s", dump_text);
        CHECK(strcmp(dump_text, expected) == 0);
    }

    /* Each part on its own. */
    dump_text[0] = '\0';
    plcsim_dump_to(p, collect, NULL, PLCSIM_DUMP_LIST);
    CHECK(strncmp(dump_text, "Instruction list", 16) == 0);
    CHECK(strstr(dump_text, "Ladder") == NULL);
    dump_text[0] = '\0';
    plcsim_dump_to(p, collect, NULL, PLCSIM_DUMP_LADDER);
    CHECK(strncmp(dump_text, "Ladder diagram", 14) == 0);
    CHECK(strstr(dump_text, "Instruction") == NULL);

    CHECK(plcsim_dump_to(NULL, collect, NULL, PLCSIM_DUMP_ALL) == PLCSIM_ERR_ARG);
    CHECK(plcsim_dump_to(p, NULL, NULL, PLCSIM_DUMP_ALL) == PLCSIM_ERR_ARG);
    CHECK(plcsim_dump(p, NULL, PLCSIM_DUMP_ALL) == PLCSIM_ERR_ARG);
    plcsim_destroy(p);
}

/* ------------------------------------------------------------------------ */
/* Sample programs of the simulator                                         */

static void join_path(char *dest, const char *dir, const char *file)
{
    sprintf(dest, "%s/%s", dir, file);
}

static void test_sample_test(const char *dir)
{
    plcsim_t *p = new_plc();
    char path[1024];

    join_path(path, dir, "test.epg");
    CHECK(plcsim_load_program_file(p, path) == PLCSIM_OK);
    CHECK(plcsim_step_count(p) == 20);

    /* The program computes
         Y0 = X0 & (series | (branch & (X9 | !X12))) & X5 & !X6
         series = X1 & X2 & X3 & X4
         branch = X7 & (X8 | ((X10 | X13) & X11))
       Check it for every combination of the 14 inputs. */
    for (unsigned combination = 0; combination < (1u << 14); combination++) {
        int in[14];
        for (int n = 0; n < 14; n++) {
            in[n] = (combination >> n) & 1;
            plcsim_set(p, X(n), in[n]);
        }
        int series = in[1] && in[2] && in[3] && in[4];
        int branch = in[7] && (in[8] || ((in[10] || in[13]) && in[11]));
        int expected = in[0] && (series || (branch && (in[9] || !in[12])))
                       && in[5] && !in[6];
        plcsim_scan(p);
        if (plcsim_get(p, Y(0)) != expected) {
            fprintf(stderr, "test.epg: inputs %04x\n", combination);
            CHECK(plcsim_get(p, Y(0)) == expected);
            break;
        }
    }
    plcsim_destroy(p);
}

static void test_sample_kitt(const char *dir)
{
    plcsim_t *p = new_plc();
    char path[1024];
    int seen_on = 0, came_back = 0;

    join_path(path, dir, "kitt.epg");
    CHECK(plcsim_load_program_file(p, path) == PLCSIM_OK);
    CHECK(strcmp(plcsim_io_name(p, 0), "KITT") == 0);
    CHECK(strcmp(plcsim_io_name(p, Y(0)), "") == 0);
    CHECK(plcsim_find_io(p, "ida") == 0x141);
    CHECK(plcsim_find_io(p, "nothing") == PLCSIM_ERR_RANGE);

    dump_text[0] = '\0';
    CHECK(plcsim_dump_to(p, collect, NULL, PLCSIM_DUMP_ALL) == PLCSIM_OK);
    CHECK(strstr(dump_text, "0000  ORG          000      KITT\n") != NULL);
    CHECK(strstr(dump_text, "0005  STR          T000     0.10 s\n") != NULL);
    CHECK(strstr(dump_text, "0098  FUN 05                master control reset\n")
          != NULL);
    CHECK(strstr(dump_text, "0094  |-+-[ T007]-+--[/T000]---") != NULL);

    for (int n = 0; n < 10; n++) { plcsim_scan(p); now_cs += 5; }
    for (unsigned b = 0; b < 8; b++) CHECK(plcsim_get(p, Y(16 + b)) == 0);

    /* The light sweeps Y16 -> Y23 and back (going dark for one scan when
       it changes direction). */
    plcsim_set(p, plcsim_find_io(p, "KITT"), 1);
    for (int n = 0; n < 200; n++) {
        int lit = 0;
        plcsim_scan(p);
        now_cs += 5;
        for (unsigned b = 0; b < 8; b++)
            if (plcsim_get(p, Y(16 + b))) {
                lit++;
                seen_on |= 1 << b;
                if (b == 0 && (seen_on & 0x80)) came_back = 1;
            }
        CHECK(lit <= 2);
    }
    CHECK(seen_on == 0xFF);
    CHECK(came_back);
    plcsim_destroy(p);
}

int main(int argc, char *argv[])
{
    test_basic();
    test_special_relays();
    test_contacts();
    test_timer();
    test_counter();
    test_edges();
    test_master_control();
    test_functions();
    test_errors();
    test_wiring();
    test_link();
    test_dump();
    if (argc > 1) {
        test_sample_test(argv[1]);
        test_sample_kitt(argv[1]);
    } else
        printf("No data directory given: skipping sample programs\n");

    if (failures) {
        printf("%d check(s) FAILED\n", failures);
        return EXIT_FAILURE;
    }
    printf("All tests passed\n");
    return EXIT_SUCCESS;
}
