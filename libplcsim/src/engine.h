/* =================================================
   |                                               |
   |                PLC Simulation                 |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//engine.h
//
// Internal state of the simulation engine: the part of the original
// simulator's 'plc' class that does not depend on the graphical interface.
// All structures are POD: an instance is allocated with calloc and copied
// with memcpy.

#ifndef PLCSIM_ENGINE_H
#define PLCSIM_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include "plcsim.h"

namespace plcsim_detail {

// Value of an ON relay (bit 7 set).
#define ON 0x0080

// Wiring limits
#define MAX_WIRING_TIMERS      50
#define MAX_SELECTORS          10
#define MAX_SELECTOR_POSITIONS  7

// Flags of a program step
#define F_INSTRUCTION     0x0001
#define F_FUNCTION        0x0002
#define F_PULSE           0x0004  // edge function: relay goes OFF next scan
#define F_LOCKED          0x0008  // edge function: waiting for the input to
                                  // return to its rest state
#define F_RELAY_OPERAND   0x0010
#define F_TIMER_OPERAND   0x0020

// Special relays
#define R_TOGGLE_SCAN  834  // toggles on every scan
#define R_TOGGLE_50CS  836  // toggles every 50 centiseconds
#define R_FIRST_SCAN   839  // ON during the first scan, then OFF
#define R_ALWAYS_ON    862  // always ON

struct step {
  unsigned char flags;
  unsigned char opcode;   // for a function, its number (dot functions 0. to
                          // 9. are 110 to 119)
  unsigned operand;
};

struct io_symbol {
  char name[11];
  char comment[31];
};

struct timer_def {
  char name[11];
  unsigned char is_counter;
  uint32_t preset;        // timers: centiseconds; counters: count
};

struct timer_state {
  unsigned char value;    // contact state
  unsigned char running;  // timers: counting time
  uint32_t start;         // timers: time when started; counters: count
  unsigned char latch;    // counters: count input already seen ON
};

// Contents of a program file (.epg)
struct program {
  step steps[PLCSIM_MAX_STEPS];   // flags in their initial state
  unsigned step_count;
  io_symbol io[PLCSIM_MEMORY_SIZE];
  timer_def timers[PLCSIM_TIMER_COUNT];
};

//----------------
// External wiring
#define WT_READY  0  // ready to be triggered
#define WT_TIMING 1
#define WT_FIRED  2  // re-armed when the output leaves the trigger state
struct wiring_timer {
  int output;             // signed: negative => triggered by OFF
  uint32_t delay;
  int input;              // signed: negative => sets the input OFF
  int state;
  uint32_t start;
};

struct selector {
  int count;                                // number of positions
  int current;                              // position currently ON
  unsigned positions[MAX_SELECTOR_POSITIONS]; // the first one is the default
};

// Contents of an external wiring file (.sda)
struct wiring {
  wiring_timer timers[MAX_WIRING_TIMERS];
  int timer_count;
  selector selectors[MAX_SELECTORS];
  int selector_count;
  signed char selector_of[PLCSIM_MEMORY_SIZE]; // selector of each address, or -1
  int link;                                    // -1: not given, 0: absent, 1: present
};

} // namespace plcsim_detail

struct plcsim {
  plcsim_detail::program program;
  plcsim_detail::wiring wiring;

  // Simulation state
  plcsim_detail::step steps[PLCSIM_MAX_STEPS]; // copy of program.steps
  unsigned char mem[PLCSIM_MEMORY_SIZE];
  plcsim_detail::timer_state timer_state[PLCSIM_TIMER_COUNT];
  uint16_t AR, ER;        // internal registers of the PLC
  unsigned char CR;
  uint32_t tick;
  uint64_t scans;
  int linked;

  plcsim_clock_fn clock;
  void *clock_user;

  char error[256];
};

namespace plcsim_detail {

uint32_t system_clock(void *user);  // clock.cpp
uint32_t now(const plcsim *p);
void reset(plcsim *p);
void apply_selector_defaults(plcsim *p);
int run(plcsim *p, unsigned parts);  // PLCSIM_RUN_*

// Loaders. They fill the destination structure, which must be zeroed.
// Return PLCSIM_OK or a PLCSIM_ERR_* code, leaving a message in 'error'.
int parse_program(const char *data, size_t size, program *prog,
                  char *error, size_t error_size);
int parse_wiring(const char *data, size_t size, wiring *wir,
                 char *error, size_t error_size);

// Number of memory addresses, starting at the operand, used by each function
// (0 if the operand is not an address).
unsigned operand_width(unsigned function);

// Stack primitives. Push: store, then advance. Pop: go back, then take.
// Both return -1 on overflow/underflow.
inline int push(int *stack, int *top, int limit, int value)
{
  if (*top < limit) return stack[(*top)++]=value;
     else return -1;
}

inline int pop(int *stack, int *top)
{
  if (*top > 0) return stack[--(*top)];
     else return -1;
}

// Returns the top of the stack without removing it.
inline int peek(int *stack, int *top)
{
  if (*top > 0) return stack[(*top)-1];
     else return -1;
}

// 4 digit BCD arithmetic. Return false on overflow or invalid digits,
// leaving the result unchanged.
bool bcd_add(uint16_t *bcd1, uint16_t bcd2);
bool bcd_sub(uint16_t *bcd1, uint16_t bcd2);
bool bin_to_bcd(unsigned bin, uint16_t *bcd);
bool bcd_to_bin(uint16_t *bin, uint16_t bcd);

} // namespace plcsim_detail

#endif // PLCSIM_ENGINE_H
