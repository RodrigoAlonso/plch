/* =================================================
   |                                               |
   |                PLC Simulation                 |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//engine.cpp
//
// Simulation engine. Adapted from simular.cpp and the parts of plc.cpp and
// soporte.cpp of the original simulator that do not depend on the graphical
// interface.
//
// Differences from the original simulator (all of them fix port bugs):
//  - AR and ER are 16 bit, as in the PLC (and in the DOS compiler).
//  - Functions 15 (AND) and 16 (OR) operate on AR again; the port had turned
//    them into a plain load.
//  - Time is read once per scan, from a configurable clock.
//  - Invalid operands never access memory out of bounds.
//  - STR and STR NOT always push their value. In the original only those
//    that the loader replaced by STR AND / STR OR (etc.) for the ladder
//    listing did; a STR without AND STR / OR STR, such as the reset line of
//    a counter, was ignored.

#include <stdlib.h>
#include <string.h>
#include "engine.h"

namespace plcsim_detail {

//----------------------------------------------------------------------------
// BCD arithmetic
static uint32_t bcd_to_int(uint16_t bcd)
{
  return ((bcd >> 12) & 0xF) * 1000 +
         ((bcd >> 8)  & 0xF) * 100 +
         ((bcd >> 4)  & 0xF) * 10 +
         (bcd & 0xF);
}

static uint16_t int_to_bcd(uint32_t num)
{
  return ((num / 1000) << 12) |
         (((num / 100) % 10) << 8) |
         (((num / 10) % 10) << 4) |
         (num % 10);
}

bool bcd_add(uint16_t *bcd1, uint16_t bcd2)
{
  uint32_t sum = bcd_to_int(*bcd1) + bcd_to_int(bcd2);

  if (sum > 9999) return false;  // more than 4 BCD digits
  *bcd1 = int_to_bcd(sum);
  return true;
}

bool bcd_sub(uint16_t *bcd1, uint16_t bcd2)
{
  uint32_t val1 = bcd_to_int(*bcd1);
  uint32_t val2 = bcd_to_int(bcd2);

  if (val2 > val1) return false;  // negative result
  *bcd1 = int_to_bcd(val1 - val2);
  return true;
}

// Converts an UNSIGNED binary number into 4 BCD digits.
bool bin_to_bcd(unsigned bin, uint16_t *bcd)
{
  if (bin >= 10000) return false;
  *bcd = int_to_bcd(bin);
  return true;
}

// Converts 4 BCD digits into an UNSIGNED binary number.
bool bcd_to_bin(uint16_t *bin, uint16_t bcd)
{
  unsigned n,result=0,weight=1;

  for (n=0; n<4; n++)
  {
    unsigned digit=(bcd>>(n*4))&0x000f;
    if (digit>9) return false;
    result+=digit*weight;
    weight*=10;
  }
  *bin=result;
  return true;
}

//----------------------------------------------------------------------------
uint32_t now(const plcsim *p)
{
  return p->clock(p->clock_user);
}

// Puts the selector switches in their default position.
void apply_selector_defaults(plcsim *p)
{
  for (int i=0; i<p->wiring.selector_count; i++)
  {
    unsigned addr=p->wiring.selectors[i].positions[0];
    p->wiring.selectors[i].current=addr;
    p->mem[addr]=ON;
  }
}

void reset(plcsim *p)
{
  memcpy(p->steps,p->program.steps,sizeof(p->steps)); // state of edges
  memset(p->mem,0,sizeof(p->mem));
  memset(p->timer_state,0,sizeof(p->timer_state));
  for (int i=0; i<p->wiring.timer_count; i++)
    p->wiring.timers[i].state=WT_READY;
  p->AR=p->ER=0;
  p->CR=0;
  p->tick=0;
  p->scans=0;
  apply_selector_defaults(p);
  p->mem[R_FIRST_SCAN]=ON; // ON during the first scan.
}

//----------------------------------------------------------------------------
// Special relays
static void update_special_relays(plcsim *p, uint32_t t)
{
  unsigned char *mem=p->mem;

  mem[R_TOGGLE_SCAN]=!mem[R_TOGGLE_SCAN]?ON:0;
  if (t-p->tick>50)
  {
    mem[R_TOGGLE_50CS]=!mem[R_TOGGLE_50CS]?ON:0;
    p->tick=t;
  }
  mem[R_ALWAYS_ON]=ON;
}

//----------------------------------------------------------------------------
// Program scan
/*
 NOTE (14/03/97):
 ===============
   - Each relay has a single, current value.
   - Nothing is copied at the end of a scan.
   - A relay is updated right after its rung is evaluated.
   This matches how a real PLC works: it evaluates a chain of contacts and
   at the end (with an OUT instruction or a function) stores the result in
   some relay. The new value is available immediately, and the next time
   that contact is used in a chain the new value is used.
*/
static int run_program(plcsim *p, uint32_t t)
{
  step *steps=p->steps;
  unsigned char *mem=p->mem;
  timer_state *ts=p->timer_state;
  const timer_def *td=p->program.timers;
  uint16_t &AR=p->AR, &ER=p->ER;
  unsigned char &CR=p->CR;

  int stack[20];
  unsigned char opcode,value;
  unsigned char partial=0,result=0,out_value=0,top_value;
  unsigned char mc=ON;  // master control. NOTE: up to 3 (three) nested
                        //                       master controls.
  int mc_stack[3];
  int top=0,mc_top=0;
  unsigned i,addr;
  int warnings=0;

  for (i=0; i<p->program.step_count; i++)
  {
    opcode=steps[i].opcode;
    addr=steps[i].operand;
// Functions that do not use the operand as an address (master control,
// dot functions) may carry any value.
    if (steps[i].flags & F_RELAY_OPERAND || (steps[i].flags & F_FUNCTION))
      value=(addr<PLCSIM_MEMORY_SIZE) ? mem[addr] : 0;
     else value=(addr<PLCSIM_TIMER_COUNT) ? ts[addr].value : 0;
    if (steps[i].flags & F_INSTRUCTION)
    {
     switch (opcode)
     {
       case PLCSIM_OP_ORG:
                   top=0;      // flush the stack
                   [[fallthrough]];
       case PLCSIM_OP_STR:
                   push(stack,&top,20,value);
                   break;
       case PLCSIM_OP_ORG_NOT:
                   top=0;      // flush the stack
                   [[fallthrough]];
       case PLCSIM_OP_STR_NOT:
                   push(stack,&top,20,((!value)?ON:0));
                   break;
       case PLCSIM_OP_AND:
                   partial=pop(stack,&top) & value;
                   push(stack,&top,20,partial);
                   break;
       case PLCSIM_OP_AND_NOT:
                   partial=pop(stack,&top) & ((!value)?ON:0);
                   push(stack,&top,20,partial);
                   break;
       case PLCSIM_OP_OR:
                   partial=pop(stack,&top) | value;
                   push(stack,&top,20,partial);
                   break;
       case PLCSIM_OP_OR_NOT:
                   partial=pop(stack,&top) | ((!value)?ON:0);
                   push(stack,&top,20,partial);
                   break;
       case PLCSIM_OP_AND_STR:
                   top_value=pop(stack,&top);
                   partial=pop(stack,&top) & top_value;
                   push(stack,&top,20,partial);
                   break;
       case PLCSIM_OP_OR_STR:
                   top_value=pop(stack,&top);
                   partial=pop(stack,&top) | top_value;
                   push(stack,&top,20,partial);
                   break;
       case PLCSIM_OP_OUT:
                   out_value=peek(stack,&top);
                   break;
       case PLCSIM_OP_OUT_NOT:
                   out_value=(!peek(stack,&top)?ON:0);
                   break;
     }
    }
    else {        // Functions
           result=mc & peek(stack,&top); // 'peek' in case an OUT follows
           if (opcode==4)     // master control set
           {
             if (push(mc_stack,&mc_top,3,mc) == -1)
               warnings|=PLCSIM_WARN_MC_NESTING;
             mc=result;
           } else
           if (opcode==5)     // master control reset
             mc=pop(mc_stack,&mc_top);
           else
/*
 Edge functions (rising and trailing) keep their state in the flags of the
 step, to detect when an edge happens.
*/
           if (opcode==0)   // Rising edge
           {
             if (steps[i].flags & F_PULSE)     // goes OFF 1 scan later
             {
               mem[addr]=0;
               steps[i].flags&=~F_PULSE;
             } else
               if (!result) steps[i].flags&=~F_LOCKED;  // unlock
               else
                 if (result && !(steps[i].flags & F_LOCKED))
                 {
                   steps[i].flags|=F_LOCKED|F_PULSE;
                   mem[addr]=ON;
                 }
           } else
           if (opcode==1)   // Trailing edge
           {
             if (steps[i].flags & F_PULSE)     // goes OFF 1 scan later
             {
               mem[addr]=0;
               steps[i].flags&=~F_PULSE;
             } else
               if (result) steps[i].flags&=~F_LOCKED;  // unlock
               else
                 if (!result && !(steps[i].flags & F_LOCKED))
                 {
                   steps[i].flags|=F_LOCKED|F_PULSE;
                   mem[addr]=ON;
                 }
           } else
// The functions inside the following 'if (result)' are only evaluated when
// the preceding contacts are ON.
           if (result)
           {
/*
  2 byte words are made of the byte at the operand address as the high
  byte and the next one as the low byte.
  NOTE: the operand can NOT (yet) be a timer.
*/
             uint16_t word=0;
             if (addr+1<PLCSIM_MEMORY_SIZE)
               word=(uint16_t)((value<<8) | mem[addr+1]);

             if (opcode==10)   // load 2 bytes -> AR
             {
               AR=word;
             } else
             if (opcode==11)   // add: AR + bcd -> AR
             {
               CR=bcd_add(&AR,word) ? 0 : ON;
             } else
             if (opcode==12)   // sub: AR - bcd -> AR
             {
               CR=bcd_sub(&AR,word) ? 0 : ON;
             } else
             if (opcode==15)   // and: AR & 2 bytes -> AR
             {
               AR&=word;
             } else
             if (opcode==16)   // or: AR | 2 bytes -> AR
             {
               AR|=word;
             } else
             if (opcode==17)   // AR >= 2 bytes -> CR=1
             {
               CR=(AR>=word)? ON : 0;
             } else
             if (opcode==18)   // AR = 2 bytes -> CR=1
             {
               CR=(AR==word)? ON : 0;
             } else
             if (opcode==19)   // AR < 2 bytes -> CR=1
             {
               CR=(AR<word)? ON : 0;
             } else
/* NOTE: a relay is considered 'ON' when its MSB
         (leftmost bit) is set.
*/
             if (opcode==20)   // load 16 internal relays -> AR
             {
               unsigned bit;
               uint16_t mask;
               for (AR=0, bit=0, mask=1; bit<16; bit++, mask<<=1)
               {
                 if (mem[addr+bit]) AR|=mask;
               }
             } else
             if (opcode==21)   // out AR -> 2 bytes
             {
               mem[addr]=AR>>8;
               mem[addr+1]=AR & 0x00FF;
             } else
             if (opcode==22)   // out AR -> 16 internal relays
             {
               unsigned bit;
               uint16_t mask;
               for (bit=0, mask=1; bit<16; bit++, mask<<=1)
               {
                 if (AR & mask)
                   mem[addr+bit]=ON;
                 else
                   mem[addr+bit]=0;
               }
             } else
             if (opcode==23)   // output carry: CR -> I/O
             {
               mem[addr]=CR;
             } else
             if (opcode==24)   // AR(bin) ---> AR(bcd)
             {
               CR=bin_to_bcd(AR,&AR) ? 0 : ON;
             } else
             if (opcode==25)   // AR(bcd) ---> AR(bin)
             {
               CR=bcd_to_bin(&AR,AR) ? 0 : ON;
             } else
             if (opcode==26)   // left shift register
             {
               if (AR & 0x8000) CR=ON;
                 else CR=0;
               AR<<=1;
             } else
             if (opcode==27)   // right shift register
             {
               if (AR & 0x0001) CR=ON;
                 else CR=0;
               AR>>=1;
             } else
             if (opcode==50)   // load byte -> AR(low)
             {
               AR&=0xff00;
               AR|=value;
             } else
             if (opcode==61)   // add AR + 2 bytes -> AR
             {
               uint32_t sum=(uint32_t)AR+word;
               CR=(sum>0xFFFF)? ON : 0;
               AR=(uint16_t)sum;
             } else
             if (opcode==62)   // sub AR - 2 bytes -> AR
             {
               CR=(AR<word)? ON : 0;
               AR-=word;
             } else
             if (opcode==63)   // mul AR * 2 bytes -> AR
             {
               uint32_t product=(uint32_t)word*AR;
               AR=(uint16_t)(product & 0xFFFF);
               if (product>0xFFFF)
               {
                 CR=ON;
                 ER=(uint16_t)(product>>16);
               } else CR=0;
             } else
             if (opcode==80)   // swap
             {
               AR=(uint16_t)((AR<<8)|(AR>>8));
             } else
             if (opcode==82)   // exchange
             {
               uint16_t tmp=AR;
               AR=ER;
               ER=tmp;
             } else
             if (opcode==85)   // not. CHECK WHETHER A REAL PLC BEHAVES THE
                               // SAME BY DUMPING AR.
             {
               AR=AR^ON;
             } else
// 110 to 119 are the dot (.) functions 0. to 9.
             if (opcode==110)   // 0. : load BCD constant into AR
             {
               AR=addr;
             } else
             if (opcode==111)   // 1. : add BCD constant to AR
             {
               CR=bcd_add(&AR,addr) ? 0 : ON;
             } else
             if (opcode==112)   // 2. : subtract BCD constant from AR
             {
               CR=bcd_sub(&AR,addr) ? 0 : ON;
             } else
             if (opcode==115)   // 5. : AR & BCD constant -> AR
             {
               AR&=addr;
             } else
             if (opcode==116)   // 6. : AR | BCD constant -> AR
             {
               AR|=addr;
             } else
             if (opcode==117)   // 7. : AR >= BCD constant -> CR=1
             {
               CR=(AR>=addr)? ON : 0;
             } else
             if (opcode==118)   // 8. : AR = BCD constant -> CR=1
             {
               CR=(AR==addr)? ON : 0;
             } else
             if (opcode==119)   // 9. : AR < BCD constant -> CR=1
             {
               CR=(AR<addr)? ON : 0;
             }
           }
         }
// Outputs (OUT and OUT NOT of the program).
// (Functions 8 and 72 have the same numbers as these opcodes and are not
//  outputs.)
    if ((steps[i].flags & F_INSTRUCTION) &&
        (opcode==PLCSIM_OP_OUT || opcode==PLCSIM_OP_OUT_NOT))
    {
      result=mc & out_value;
      if (steps[i].flags & F_RELAY_OPERAND) mem[addr]=result;
        else
        if (!td[addr].is_counter)  // TIMERS
        {
         if (result)
         {
          if (!ts[addr].running)
          {
             ts[addr].running=1;
             ts[addr].start=t;
          }
          else if ((t-ts[addr].start)>td[addr].preset)
               {
                 ts[addr].value=result;
               }
         } else
           {
             ts[addr].value=result;
             ts[addr].running=0;
             ts[addr].start=0;
           }
        } else   // COUNTERS
          {
            pop(stack,&top);                  // reset
            out_value=mc && pop(stack,&top);  // count
            if (result)     // reset line of the counter
            {
              ts[addr].value=0;
              ts[addr].start=0;
              if (out_value) ts[addr].latch=1;
                 else ts[addr].latch=0;
            } else
              if (out_value)  // count line of the counter
              {
               if (ts[addr].latch==0)
               {
                 ts[addr].latch=1;
                 if (ts[addr].start<td[addr].preset)
                    ts[addr].start++;
                 if (ts[addr].start==td[addr].preset)
                    ts[addr].value=ON;
               }
              } else ts[addr].latch=0;
            push(stack,&top,20,result);
          }
    } // end_if
  } // end_for
  mem[R_FIRST_SCAN]=0; // ON during the first scan, then OFF.
  return warnings;
}

//----------------------------------------------------------------------------
// External wiring
static void run_wiring(plcsim *p, uint32_t t)
{
  unsigned addr;
  unsigned char level;

  for (int i=0; i<p->wiring.timer_count; i++)
  {
    wiring_timer &wt=p->wiring.timers[i];
    addr=abs(wt.output);
    level=(wt.output>0) ? ON : 0;
    if (p->mem[addr]==level)
    {
     if (wt.state == WT_READY)
     {
       wt.start=t;
       wt.state=WT_TIMING;
     }
     else if (wt.state == WT_TIMING)
            if (t-wt.start > wt.delay)
            {
              addr=abs(wt.input);
              level=(wt.input>0) ? ON : 0;
              p->mem[addr]=level;
              wt.state=WT_FIRED;
            }
    } else wt.state=WT_READY;
  }
}

//----------------------------------------------------------------------------
int run(plcsim *p, unsigned parts)
{
  uint32_t t=now(p);
  int warnings=0;

  if (parts & PLCSIM_RUN_PROGRAM)
  {
    update_special_relays(p,t);
    warnings=run_program(p,t);
    p->scans++;
  }
  if (parts & PLCSIM_RUN_WIRING) run_wiring(p,t);
  return warnings;
}

} // namespace plcsim_detail
