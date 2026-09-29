//api.cpp
//
// Implementation of the public interface (plcsim.h) on top of the engine.
// The C++ runtime is not used (no new, exceptions or RTTI), so the static
// library can be linked from C programs.

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"

using namespace plcsim_detail;

static int fail(plcsim_t *plc, int code, const char *message,
                const char *detail = NULL)
{
  snprintf(plc->error,sizeof(plc->error),detail ? "%s: %s" : "%s",
           message,detail);
  return code;
}

static bool valid_address(unsigned addr)
{
  return addr<PLCSIM_MEMORY_SIZE;
}

// Reads a whole file. The caller frees the buffer.
static int read_file(plcsim_t *plc, const char *path, char **data,
                     size_t *size)
{
  FILE *f;
  long length;

  if (path==NULL) return fail(plc,PLCSIM_ERR_ARG,"Null file name");
  if ((f=fopen(path,"rb"))==NULL)
    return fail(plc,PLCSIM_ERR_IO,path,strerror(errno));
  if (fseek(f,0,SEEK_END)!=0 || (length=ftell(f))<0 ||
      fseek(f,0,SEEK_SET)!=0)
  {
    fclose(f);
    return fail(plc,PLCSIM_ERR_IO,path,"cannot read");
  }
  if ((*data=(char *)malloc(length ? length : 1))==NULL)
  {
    fclose(f);
    return fail(plc,PLCSIM_ERR_NOMEM,"Out of memory");
  }
  *size=fread(*data,1,length,f);
  bool error=ferror(f);
  fclose(f);
  if (error || *size!=(size_t)length)
  {
    free(*data);
    return fail(plc,PLCSIM_ERR_IO,path,"cannot read");
  }
  return PLCSIM_OK;
}

//----------------------------------------------------------------------------
// Lifecycle

extern "C" unsigned plcsim_version(void)
{
  return PLCSIM_VERSION_MAJOR*10000 + PLCSIM_VERSION_MINOR*100 +
         PLCSIM_VERSION_PATCH;
}

extern "C" plcsim_t *plcsim_create(void)
{
  plcsim_t *plc=(plcsim_t *)calloc(1,sizeof(plcsim_t));

  if (plc==NULL) return NULL;
  plc->clock=system_clock;
  plcsim_clear_wiring(plc);
  reset(plc);
  return plc;
}

extern "C" void plcsim_destroy(plcsim_t *plc)
{
  free(plc);
}

extern "C" const char *plcsim_last_error(const plcsim_t *plc)
{
  return plc ? plc->error : "Null instance";
}

//----------------------------------------------------------------------------
// Loading

extern "C" int plcsim_load_program_mem(plcsim_t *plc, const char *data,
                                       size_t size)
{
  if (plc==NULL) return PLCSIM_ERR_ARG;
  if (data==NULL && size!=0) return fail(plc,PLCSIM_ERR_ARG,"Null data");
  program *prog=(program *)calloc(1,sizeof(program));
  if (prog==NULL) return fail(plc,PLCSIM_ERR_NOMEM,"Out of memory");
  int result=parse_program(data,size,prog,plc->error,sizeof(plc->error));
  if (result==PLCSIM_OK)
  {
    plc->program=*prog;
    plc->error[0]='\0';
    reset(plc);
  }
  free(prog);
  return result;
}

extern "C" int plcsim_load_program_file(plcsim_t *plc, const char *path)
{
  char *data;
  size_t size;
  int result;

  if (plc==NULL) return PLCSIM_ERR_ARG;
  if ((result=read_file(plc,path,&data,&size))!=PLCSIM_OK)
    return result;
  result=plcsim_load_program_mem(plc,data,size);
  free(data);
  return result;
}

extern "C" int plcsim_load_wiring_mem(plcsim_t *plc, const char *data,
                                      size_t size)
{
  if (plc==NULL) return PLCSIM_ERR_ARG;
  if (data==NULL && size!=0) return fail(plc,PLCSIM_ERR_ARG,"Null data");
  wiring *wir=(wiring *)calloc(1,sizeof(wiring));
  if (wir==NULL) return fail(plc,PLCSIM_ERR_NOMEM,"Out of memory");
  int result=parse_wiring(data,size,wir,plc->error,sizeof(plc->error));
  if (result==PLCSIM_OK)
  {
    plc->wiring=*wir;
    plc->error[0]='\0';
    if (plc->wiring.link!=-1) plc->linked=plc->wiring.link;
    apply_selector_defaults(plc);
  }
  free(wir);
  return result;
}

extern "C" int plcsim_load_wiring_file(plcsim_t *plc, const char *path)
{
  char *data;
  size_t size;
  int result;

  if (plc==NULL) return PLCSIM_ERR_ARG;
  if ((result=read_file(plc,path,&data,&size))!=PLCSIM_OK)
    return result;
  result=plcsim_load_wiring_mem(plc,data,size);
  free(data);
  return result;
}

extern "C" void plcsim_clear_wiring(plcsim_t *plc)
{
  if (plc==NULL) return;
  memset(&plc->wiring,0,sizeof(plc->wiring));
  memset(plc->wiring.selector_of,-1,sizeof(plc->wiring.selector_of));
  plc->wiring.link=-1;
}

extern "C" int plcsim_is_selector(const plcsim_t *plc, unsigned addr)
{
  if (plc==NULL) return PLCSIM_ERR_ARG;
  if (!valid_address(addr)) return PLCSIM_ERR_RANGE;
  return plc->wiring.selector_of[addr]>=0;
}

extern "C" unsigned plcsim_step_count(const plcsim_t *plc)
{
  return plc ? plc->program.step_count : 0;
}

extern "C" int plcsim_get_step(const plcsim_t *plc, unsigned index,
                               plcsim_step *out)
{
  if (plc==NULL || out==NULL) return PLCSIM_ERR_ARG;
  if (index>=plc->program.step_count) return PLCSIM_ERR_RANGE;
  const step &st=plc->program.steps[index];
  out->is_function=(st.flags & F_FUNCTION) ? 1 : 0;
  out->opcode=st.opcode;
  out->operand=st.operand;
  out->timer=(st.flags & F_TIMER_OPERAND) ? 1 : 0;
  out->dot=0;
  if (out->is_function && st.opcode>=110 && st.opcode<=119)
  {
    out->opcode-=110;
    out->dot=1;
  }
  return PLCSIM_OK;
}

//----------------------------------------------------------------------------
// Execution

extern "C" void plcsim_reset(plcsim_t *plc)
{
  if (plc) reset(plc);
}

extern "C" int plcsim_scan(plcsim_t *plc)
{
  return plcsim_run(plc,PLCSIM_RUN_ALL);
}

extern "C" int plcsim_run(plcsim_t *plc, unsigned what)
{
  if (plc==NULL) return PLCSIM_ERR_ARG;
  return run(plc,what);
}

extern "C" uint64_t plcsim_scan_count(const plcsim_t *plc)
{
  return plc ? plc->scans : 0;
}

extern "C" void plcsim_set_clock(plcsim_t *plc, plcsim_clock_fn clock,
                                 void *user)
{
  if (plc==NULL) return;
  plc->clock=clock ? clock : system_clock;
  plc->clock_user=clock ? user : NULL;
}

//----------------------------------------------------------------------------
// I/O and memory

extern "C" int plcsim_get(const plcsim_t *plc, unsigned addr)
{
  if (plc==NULL) return PLCSIM_ERR_ARG;
  if (!valid_address(addr)) return PLCSIM_ERR_RANGE;
  return (plc->mem[addr] & ON) ? 1 : 0;
}

extern "C" int plcsim_set(plcsim_t *plc, unsigned addr, int on)
{
  if (plc==NULL) return PLCSIM_ERR_ARG;
  if (!valid_address(addr)) return PLCSIM_ERR_RANGE;
  int s=plc->wiring.selector_of[addr];
  if (on && s>=0)  // turn off the other positions of the selector
  {
    selector &sel=plc->wiring.selectors[s];
    for (int i=0; i<sel.count; i++) plc->mem[sel.positions[i]]=0;
    sel.current=addr;
  }
  plc->mem[addr]=on ? ON : 0;
  return PLCSIM_OK;
}

extern "C" int plcsim_get_byte(const plcsim_t *plc, unsigned addr)
{
  if (plc==NULL) return PLCSIM_ERR_ARG;
  if (!valid_address(addr)) return PLCSIM_ERR_RANGE;
  return plc->mem[addr];
}

extern "C" int plcsim_set_byte(plcsim_t *plc, unsigned addr, uint8_t value)
{
  if (plc==NULL) return PLCSIM_ERR_ARG;
  if (!valid_address(addr)) return PLCSIM_ERR_RANGE;
  plc->mem[addr]=value;
  return PLCSIM_OK;
}

extern "C" int plcsim_get_registers(const plcsim_t *plc, plcsim_registers *out)
{
  if (plc==NULL || out==NULL) return PLCSIM_ERR_ARG;
  out->ar=plc->AR;
  out->er=plc->ER;
  out->carry=(plc->CR & ON) ? 1 : 0;
  return PLCSIM_OK;
}

//----------------------------------------------------------------------------
// Timers and counters

extern "C" int plcsim_get_timer(const plcsim_t *plc, unsigned index,
                                plcsim_timer_info *out)
{
  if (plc==NULL || out==NULL) return PLCSIM_ERR_ARG;
  if (index>=PLCSIM_TIMER_COUNT) return PLCSIM_ERR_RANGE;
  const timer_def &td=plc->program.timers[index];
  const timer_state &ts=plc->timer_state[index];
  out->is_counter=td.is_counter;
  out->preset=td.preset;
  if (out->is_counter) out->value=ts.start;
    else out->value=ts.running ? now(plc)-ts.start : 0;
  out->done=(ts.value & ON) ? 1 : 0;
  return PLCSIM_OK;
}

//----------------------------------------------------------------------------
// Symbols

extern "C" const char *plcsim_io_name(const plcsim_t *plc, unsigned addr)
{
  if (plc==NULL || !valid_address(addr)) return NULL;
  return plc->program.io[addr].name;
}

extern "C" const char *plcsim_io_comment(const plcsim_t *plc, unsigned addr)
{
  if (plc==NULL || !valid_address(addr)) return NULL;
  return plc->program.io[addr].comment;
}

extern "C" const char *plcsim_timer_name(const plcsim_t *plc, unsigned index)
{
  if (plc==NULL || index>=PLCSIM_TIMER_COUNT) return NULL;
  return plc->program.timers[index].name;
}

static bool equals_ignore_case(const char *a, const char *b)
{
  for (; *a && *b; a++, b++)
    if (tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return false;
  return *a==*b;
}

extern "C" int plcsim_find_io(const plcsim_t *plc, const char *name)
{
  if (plc==NULL || name==NULL) return PLCSIM_ERR_ARG;
  if (name[0]=='\0') return PLCSIM_ERR_RANGE;
  for (unsigned addr=0; addr<PLCSIM_MEMORY_SIZE; addr++)
    if (equals_ignore_case(plc->program.io[addr].name,name)) return addr;
  return PLCSIM_ERR_RANGE;
}

// Each 16 point word takes 20 numbers in the external numbering.
extern "C" int plcsim_addr_to_external(unsigned addr)
{
  if (!valid_address(addr)) return PLCSIM_ERR_RANGE;
  return addr+(addr/16)*4;
}

extern "C" int plcsim_addr_from_external(unsigned external)
{
  if (external%20>=16) return PLCSIM_ERR_RANGE;
  unsigned addr=16*(external/20)+(external%20);
  if (!valid_address(addr)) return PLCSIM_ERR_RANGE;
  return addr;
}

//----------------------------------------------------------------------------
// Link between PLCs

extern "C" void plcsim_set_linked(plcsim_t *plc, int linked)
{
  if (plc) plc->linked=linked ? 1 : 0;
}

extern "C" int plcsim_is_linked(const plcsim_t *plc)
{
  return plc ? plc->linked : 0;
}

// Adapted from Link::actualizarLink: each input of the link area of a
// connected PLC is the OR of the outputs of the link areas of the connected
// PLCs.
extern "C" void plcsim_link_update(plcsim_t *const *plcs, size_t count)
{
  unsigned char area[PLCSIM_LINK_SIZE]={0};
  size_t i;
  unsigned b;

  if (plcs==NULL) return;
  for (i=0; i<count; i++)
    if (plcs[i] && plcs[i]->linked)
      for (b=0; b<PLCSIM_LINK_SIZE; b++)
        area[b]|=plcs[i]->mem[PLCSIM_LINK_WRITE_BASE+b];
  for (i=0; i<count; i++)
    if (plcs[i] && plcs[i]->linked)
      for (b=0; b<PLCSIM_LINK_SIZE; b++)
        plcs[i]->mem[PLCSIM_LINK_READ_BASE+b]=area[b];
}
