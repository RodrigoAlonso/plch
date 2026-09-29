//dump.cpp
//
// Text listings of the loaded program: instruction list and ladder diagram.
//
// The ladder is built from the instruction list, which is stack based: each
// rung (from one ORG to the next) is turned into a tree of series and
// parallel connections of contacts, one per output, and then drawn on a
// character canvas.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"

namespace plcsim_detail {

struct output {
  plcsim_write_fn write;
  void *user;
};

// Prints one line (without trailing blanks) followed by a newline.
static void print_line(output &out, const char *format, ...)
{
  char line[512];
  va_list ap;
  va_start(ap,format);
  vsnprintf(line,sizeof(line)-1,format,ap);
  va_end(ap);
  size_t n=strlen(line);
  while (n>0 && line[n-1]==' ') n--;
  line[n++]='\n';
  line[n]='\0';
  out.write(out.user,line);
}

//----------------------------------------------------------------------------
// Formatting of a step

static bool is_org(const step &st)
{
  return (st.flags & F_INSTRUCTION) &&
         (st.opcode==PLCSIM_OP_ORG || st.opcode==PLCSIM_OP_ORG_NOT);
}

static bool is_negated(const step &st)
{
  switch (st.opcode)
  {
    case PLCSIM_OP_ORG_NOT: case PLCSIM_OP_STR_NOT: case PLCSIM_OP_AND_NOT:
    case PLCSIM_OP_OR_NOT:  case PLCSIM_OP_OUT_NOT:
      return (st.flags & F_INSTRUCTION)!=0;
  }
  return false;
}

static bool is_dot_function(const step &st)
{
  return (st.flags & F_FUNCTION) && st.opcode>=110 && st.opcode<=119;
}

static const char *instruction_name(unsigned opcode)
{
  switch (opcode)
  {
    case PLCSIM_OP_ORG:     return "ORG";
    case PLCSIM_OP_ORG_NOT: return "ORG NOT";
    case PLCSIM_OP_STR:     return "STR";
    case PLCSIM_OP_STR_NOT: return "STR NOT";
    case PLCSIM_OP_AND:     return "AND";
    case PLCSIM_OP_AND_NOT: return "AND NOT";
    case PLCSIM_OP_OR:      return "OR";
    case PLCSIM_OP_OR_NOT:  return "OR NOT";
    case PLCSIM_OP_AND_STR: return "AND STR";
    case PLCSIM_OP_OR_STR:  return "OR STR";
    case PLCSIM_OP_OUT:     return "OUT";
    case PLCSIM_OP_OUT_NOT: return "OUT NOT";
  }
  return NULL;
}

static const char *function_description(unsigned function)
{
  switch (function)
  {
    case 0:   return "rising edge";
    case 1:   return "trailing edge";
    case 4:   return "master control set";
    case 5:   return "master control reset";
    case 10:  return "AR = word";
    case 11:  return "AR = AR + word (BCD)";
    case 12:  return "AR = AR - word (BCD)";
    case 15:  return "AR = AR AND word";
    case 16:  return "AR = AR OR word";
    case 17:  return "CR = AR >= word";
    case 18:  return "CR = AR = word";
    case 19:  return "CR = AR < word";
    case 20:  return "AR = 16 relays";
    case 21:  return "word = AR";
    case 22:  return "16 relays = AR";
    case 23:  return "relay = CR";
    case 24:  return "AR = BCD(AR)";
    case 25:  return "AR = binary(AR)";
    case 26:  return "shift AR left";
    case 27:  return "shift AR right";
    case 50:  return "AR low byte = byte";
    case 61:  return "AR = AR + word";
    case 62:  return "AR = AR - word";
    case 63:  return "AR = AR * word";
    case 80:  return "swap AR bytes";
    case 82:  return "exchange AR and ER";
    case 85:  return "AR = AR XOR 0x80";
    case 110: return "AR = constant";
    case 111: return "AR = AR + constant (BCD)";
    case 112: return "AR = AR - constant (BCD)";
    case 115: return "AR = AR AND constant";
    case 116: return "AR = AR OR constant";
    case 117: return "CR = AR >= constant";
    case 118: return "CR = AR = constant";
    case 119: return "CR = AR < constant";
  }
  return "not simulated";
}

static void mnemonic(const step &st, char *buf, size_t size)
{
  if (st.flags & F_INSTRUCTION)
  {
    const char *name=instruction_name(st.opcode);
    if (name) snprintf(buf,size,"%s",name);
      else snprintf(buf,size,"OP %02X",st.opcode);
  } else
  if (is_dot_function(st)) snprintf(buf,size,"FUN %u.",st.opcode-110);
  else snprintf(buf,size,"FUN %02u",st.opcode);
}

// Memory address used by the step, or -1.
static int step_address(const step &st)
{
  if (st.flags & F_INSTRUCTION)
  {
    if ((st.flags & F_TIMER_OPERAND) || st.opcode==PLCSIM_OP_AND_STR ||
        st.opcode==PLCSIM_OP_OR_STR || st.operand>=PLCSIM_MEMORY_SIZE)
      return -1;
    return st.operand;
  }
  if (is_dot_function(st) || operand_width(st.opcode)==0) return -1;
  return st.operand;
}

// Timer or counter used by the step, or -1.
static int step_timer(const step &st)
{
  if ((st.flags & F_INSTRUCTION) && (st.flags & F_TIMER_OPERAND) &&
      st.operand<PLCSIM_TIMER_COUNT)
    return st.operand;
  return -1;
}

static void operand_text(const plcsim *p, const step &st, char *buf,
                         size_t size)
{
  int addr=step_address(st),timer=step_timer(st);

  buf[0]='\0';
  if (addr>=0) snprintf(buf,size,"%03d",plcsim_addr_to_external(addr));
  else if (timer>=0)
    snprintf(buf,size,"%c%03o",p->program.timers[timer].is_counter ? 'C' : 'T',
             timer);
  else if (is_dot_function(st)) snprintf(buf,size,"%04X",st.operand & 0xFFFF);
}

static void step_comment(const plcsim *p, const step &st, char *buf,
                         size_t size)
{
  int addr=step_address(st),timer=step_timer(st);
  const char *name="";

  if (addr>=0) name=p->program.io[addr].name;
  else if (timer>=0) name=p->program.timers[timer].name;

  if (st.flags & F_FUNCTION)
    snprintf(buf,size,"%s%s%s",function_description(st.opcode),
             name[0] ? " - " : "",name);
  else if (timer>=0)
  {
    const timer_def &td=p->program.timers[timer];
    if (td.is_counter)
      snprintf(buf,size,"%s%scount %u",name,name[0] ? ", " : "",td.preset);
    else
      snprintf(buf,size,"%s%s%u.%02u s",name,name[0] ? ", " : "",
               td.preset/100,td.preset%100);
  }
  else snprintf(buf,size,"%s",name);
}

//----------------------------------------------------------------------------
// Instruction list

static void dump_list(const plcsim *p, output &out)
{
  const program &prog=p->program;
  char name[16],operand[16],comment[96];

  print_line(out,"Instruction list (%u steps)",prog.step_count);
  print_line(out,"");
  print_line(out,"Step  Instruction  Operand  Comment");
  for (unsigned i=0; i<prog.step_count; i++)
  {
    const step &st=prog.steps[i];
    mnemonic(st,name,sizeof(name));
    operand_text(p,st,operand,sizeof(operand));
    step_comment(p,st,comment,sizeof(comment));
    print_line(out,"%04u  %-11s  %-7s  %s",i,name,operand,comment);
  }
}

//----------------------------------------------------------------------------
// Ladder: rung model

enum { N_CONTACT, N_SERIES, N_PARALLEL };

struct node {
  int kind;
  int step;               // contacts
  int a,b;                // series / parallel
};

// How an output is attached to the rung.
enum {
  E_FULL,   // its own line, with the whole condition
  E_EXT,    // branch off the previous output, adding contacts in series
  E_SAME,   // same condition as the previous output: stacked coil
};

struct entry {
  int kind;
  int cond;               // condition of the output (node, -1 if none)
  int net;                // what is drawn before the coil (node, -1 if none)
  char label[40];         // coil or function box
  // layout
  int anchor;             // E_EXT / E_SAME: entry it hangs from
  int row;
  int x0,x_end;           // columns taken by 'net'
  int height;             // rows taken by 'net'
  int coils;              // coils stacked on this entry (itself included)
};

#define LADDER_STACK 64

struct rung {
  const plcsim *p;
  node *nodes;
  int node_count,node_max;
  entry *entries;
  int entry_count,entry_max;
  int stack[LADDER_STACK];
  int top;
  int rows;               // after layout
  int min_coil_col;       // after layout
};

static int new_node(rung &r, int kind, int step, int a, int b)
{
  if (kind!=N_CONTACT)
  {
    if (a<0) return b;
    if (b<0) return a;
  }
  if (r.node_count>=r.node_max) return a>=0 ? a : b;
  node &n=r.nodes[r.node_count];
  n.kind=kind;
  n.step=step;
  n.a=a;
  n.b=b;
  return r.node_count++;
}

static void push_node(rung &r, int n)
{
  if (r.top<LADDER_STACK) r.stack[r.top++]=n;
}

static int pop_node(rung &r)
{
  return r.top>0 ? r.stack[--r.top] : -1;
}

static int peek_node(rung &r)
{
  return r.top>0 ? r.stack[r.top-1] : -1;
}

static void add_entry(rung &r, int cond, const char *label)
{
  if (r.entry_count>=r.entry_max) return;
  entry &e=r.entries[r.entry_count];
  memset(&e,0,sizeof(e));
  e.cond=cond;
  e.net=cond;
  e.kind=E_FULL;
  snprintf(e.label,sizeof(e.label),"%s",label);
  if (r.entry_count>0)
  {
    const entry &prev=r.entries[r.entry_count-1];
    if (cond==prev.cond) e.kind=E_SAME;
    else if (prev.cond>=0)
    {
// Is the condition the previous one followed by more contacts in series?
      int n=cond;
      while (n>=0 && n!=prev.cond && r.nodes[n].kind==N_SERIES)
        n=r.nodes[n].a;
      if (n==prev.cond)
      {
        int extra=-1;
        for (n=cond; n!=prev.cond; n=r.nodes[n].a)
          extra=new_node(r,N_SERIES,-1,r.nodes[n].b,extra);
        e.kind=E_EXT;
        e.net=extra;
      }
    }
  }
  r.entry_count++;
}

// Builds the model of the rung starting at 'first'. Returns the first step
// of the next rung.
static unsigned build_rung(rung &r, unsigned first)
{
  const program &prog=r.p->program;
  unsigned i;
  char operand[16],label[40];

  r.node_count=r.entry_count=r.top=0;
  for (i=first; i<prog.step_count && (i==first || !is_org(prog.steps[i])); i++)
  {
    const step &st=prog.steps[i];
    operand_text(r.p,st,operand,sizeof(operand));
    if (st.flags & F_FUNCTION)
    {
      char name[16];
      mnemonic(st,name,sizeof(name));
      snprintf(label,sizeof(label),"[%s%s%s]",name,operand[0] ? " " : "",
               operand);
      add_entry(r,peek_node(r),label);
      continue;
    }
    int contact=new_node(r,N_CONTACT,i,-1,-1),a,b;
    switch (st.opcode)
    {
      case PLCSIM_OP_ORG: case PLCSIM_OP_ORG_NOT:
        r.top=0;
        push_node(r,contact);
        break;
      case PLCSIM_OP_STR: case PLCSIM_OP_STR_NOT:
        push_node(r,contact);
        break;
      case PLCSIM_OP_AND: case PLCSIM_OP_AND_NOT:
        push_node(r,new_node(r,N_SERIES,-1,pop_node(r),contact));
        break;
      case PLCSIM_OP_OR: case PLCSIM_OP_OR_NOT:
        push_node(r,new_node(r,N_PARALLEL,-1,pop_node(r),contact));
        break;
      case PLCSIM_OP_AND_STR:
        b=pop_node(r);
        a=pop_node(r);
        push_node(r,new_node(r,N_SERIES,-1,a,b));
        break;
      case PLCSIM_OP_OR_STR:
        b=pop_node(r);
        a=pop_node(r);
        push_node(r,new_node(r,N_PARALLEL,-1,a,b));
        break;
      case PLCSIM_OP_OUT: case PLCSIM_OP_OUT_NOT:
        if (step_timer(st)>=0 && r.p->program.timers[st.operand].is_counter)
        {
// A counter takes two conditions: count (below) and reset (on top).
          int reset=pop_node(r),count=pop_node(r);
          snprintf(label,sizeof(label),"(CNT %s)",operand);
          add_entry(r,count,label);
          snprintf(label,sizeof(label),"(RST %s)",operand);
          add_entry(r,reset,label);
          push_node(r,reset);
        } else
          {
            snprintf(label,sizeof(label),"(%c%-4s)",is_negated(st) ? '/' : ' ',
                     operand);
            add_entry(r,peek_node(r),label);
          }
        break;
    }
  }
// Contacts without an output are drawn anyway.
  if (r.entry_count==0 && r.top>0) add_entry(r,peek_node(r),"");
  return i;
}

//----------------------------------------------------------------------------
// Ladder: drawing

#define CONTACT_WIDTH 9   // -[ 000 ]-

struct canvas {
  int width,height;
  char *cells;
};

static void put(canvas &c, int x, int y, char ch)
{
  if (x>=0 && x<c.width && y>=0 && y<c.height) c.cells[y*c.width+x]=ch;
}

static void put_text(canvas &c, int x, int y, const char *s)
{
  for (; *s; s++, x++) put(c,x,y,*s);
}

static void measure(const rung &r, int n, int *w, int *h);

// Width and height of the widest / all the branches of a parallel block.
static void measure_branches(const rung &r, int n, int *w, int *h)
{
  if (r.nodes[n].kind==N_PARALLEL)
  {
    int wa,ha,wb,hb;
    measure_branches(r,r.nodes[n].a,&wa,&ha);
    measure_branches(r,r.nodes[n].b,&wb,&hb);
    *w=wa>wb ? wa : wb;
    *h=ha+hb;
  } else measure(r,n,w,h);
}

static void measure(const rung &r, int n, int *w, int *h)
{
  *w=0;
  *h=1;
  if (n<0) return;
  const node &nd=r.nodes[n];
  if (nd.kind==N_CONTACT)
  {
    *w=CONTACT_WIDTH;
  } else
  if (nd.kind==N_SERIES)
  {
    int wa,ha,wb,hb;
    measure(r,nd.a,&wa,&ha);
    measure(r,nd.b,&wb,&hb);
    *w=wa+wb;
    *h=ha>hb ? ha : hb;
  } else
    {
      measure_branches(r,n,w,h);
      *w+=4;                // connectors and a '-' on both sides
    }
}

static void draw(const rung &r, canvas &c, int n, int x, int y);

// Draws the branches of a parallel block, each one below the previous.
static void draw_branches(const rung &r, canvas &c, int n, int x, int *y,
                          int width)
{
  if (r.nodes[n].kind==N_PARALLEL)
  {
    draw_branches(r,c,r.nodes[n].a,x,y,width);
    draw_branches(r,c,r.nodes[n].b,x,y,width);
    return;
  }
  int w,h;
  measure(r,n,&w,&h);
  draw(r,c,n,x+1,*y);
  for (int i=x+1+w; i<x+1+width; i++) put(c,i,*y,'-');
  put(c,x,*y,'+');
  put(c,x+1+width,*y,'+');
  *y+=h;
}

static void draw(const rung &r, canvas &c, int n, int x, int y)
{
  if (n<0) return;
  const node &nd=r.nodes[n];
  if (nd.kind==N_CONTACT)
  {
    const step &st=r.p->program.steps[nd.step];
    char operand[16],text[32];
    operand_text(r.p,st,operand,sizeof(operand));
    snprintf(text,sizeof(text),"-[%c%-4s]-",is_negated(st) ? '/' : ' ',
             operand);
    put_text(c,x,y,text);
  } else
  if (nd.kind==N_SERIES)
  {
    int w,h;
    measure(r,nd.a,&w,&h);
    draw(r,c,nd.a,x,y);
    draw(r,c,nd.b,x+w,y);
  } else
    {
// Vertical connectors from the first branch down to the last one, then
// the branches, which put a '+' where they join them.
      int width,height,last=n,wl,hl,row=y;
      measure_branches(r,n,&width,&height);
      while (r.nodes[last].kind==N_PARALLEL) last=r.nodes[last].b;
      measure(r,last,&wl,&hl);
      put(c,x,y,'-');
      put(c,x+3+width,y,'-');
      for (int i=y; i<=y+height-hl; i++)
      {
        put(c,x+1,i,'|');
        put(c,x+2+width,i,'|');
      }
      draw_branches(r,c,n,x+1,&row,width);
    }
}

// Assigns rows and columns to the entries of the rung.
static void layout(rung &r)
{
  int row=0,anchor=-1;

  r.rows=0;
  r.min_coil_col=0;
  for (int k=0; k<r.entry_count; k++)
  {
    entry &e=r.entries[k];
    if (e.kind==E_SAME && anchor>=0)
    {
      entry &a=r.entries[anchor];
      e.anchor=anchor;
      e.row=a.row+a.coils++;
    } else
      {
        if (anchor>=0)
        {
          const entry &a=r.entries[anchor];
          row=a.row+(a.height>a.coils ? a.height : a.coils);
        }
        int w,h;
        measure(r,e.net,&w,&h);
        e.row=row;
        e.height=h;
        e.coils=1;
        if (e.kind==E_EXT)
        {
          e.anchor=anchor;
          e.x0=r.entries[anchor].x_end+1;
        } else
          {
            e.kind=E_FULL;
            e.x0=1;
          }
        e.x_end=e.x0+w;
        if (e.x_end+3>r.min_coil_col) r.min_coil_col=e.x_end+3;
        anchor=k;
      }
  }
  if (anchor>=0)
  {
    const entry &a=r.entries[anchor];
    r.rows=a.row+(a.height>a.coils ? a.height : a.coils);
  }
}

static void draw_rung(const rung &r, canvas &c, int coil_col)
{
  for (int y=0; y<c.height; y++) put(c,0,y,'|');  // power rail
  for (int k=0; k<r.entry_count; k++)
  {
    const entry &e=r.entries[k];
    if (e.kind==E_SAME)  // stacked coils take consecutive rows
    {
      put(c,coil_col-2,e.row-1,'+');
      put(c,coil_col-2,e.row,'+');
      put(c,coil_col-1,e.row,'-');
    } else
      {
        if (e.kind==E_EXT)
        {
          const entry &a=r.entries[e.anchor];
          for (int y=a.row; y<=e.row; y++)
            put(c,a.x_end,y,y==a.row || y==e.row ? '+' : '|');
        }
        draw(r,c,e.net,e.x0,e.row);
        for (int x=e.x_end; x<coil_col; x++) put(c,x,e.row,'-');
      }
    put_text(c,coil_col,e.row,e.label);
  }
}

// Prints the rows of a rung, the first one preceded by its step number.
static void print_canvas(output &out, const canvas &c, unsigned first)
{
  char *line=(char *)malloc(c.width+8);

  if (line==NULL) return;
  for (int y=0; y<c.height; y++)
  {
    int n=c.width;
    while (n>0 && c.cells[y*c.width+n-1]==' ') n--;
    if (y==0) snprintf(line,8,"%04u  ",first);
      else strcpy(line,"      ");
    memcpy(line+6,c.cells+y*c.width,n);
    strcpy(line+6+n,"\n");
    out.write(out.user,line);
  }
  free(line);
}

static void dump_ladder(const plcsim *p, output &out)
{
  const program &prog=p->program;
  rung r;
  unsigned first,next;
  int coil_col=0;

  r.p=p;
  r.node_max=3*prog.step_count+8;
  r.entry_max=2*prog.step_count+2;
  r.nodes=(node *)malloc(r.node_max*sizeof(node));
  r.entries=(entry *)malloc(r.entry_max*sizeof(entry));
  if (r.nodes==NULL || r.entries==NULL)
  {
    free(r.nodes);
    free(r.entries);
    print_line(out,"Ladder diagram: out of memory");
    return;
  }

// First pass: the coils of all the rungs are aligned on the same column.
  for (first=0; first<prog.step_count; first=next)
  {
    next=build_rung(r,first);
    layout(r);
    if (r.min_coil_col>coil_col) coil_col=r.min_coil_col;
  }

  print_line(out,"Ladder diagram");
  print_line(out,"");
  for (first=0; first<prog.step_count; first=next)
  {
    next=build_rung(r,first);
    layout(r);
    if (r.rows==0) continue;
    canvas c;
    c.width=coil_col+sizeof(((entry *)0)->label);
    c.height=r.rows;
    c.cells=(char *)malloc(c.width*c.height);
    if (c.cells==NULL)
    {
      print_line(out,"%04u  (out of memory)",first);
      continue;
    }
    memset(c.cells,' ',c.width*c.height);
    draw_rung(r,c,coil_col);
    print_canvas(out,c,first);
    print_line(out,"      |");
    free(c.cells);
  }
  free(r.nodes);
  free(r.entries);
}

static void write_file(void *user, const char *text)
{
  fputs(text,(FILE *)user);
}

} // namespace plcsim_detail

using namespace plcsim_detail;

extern "C" int plcsim_dump_to(const plcsim_t *plc, plcsim_write_fn write,
                              void *user, unsigned what)
{
  if (plc==NULL || write==NULL) return PLCSIM_ERR_ARG;
  output out={write,user};
  if (plc->program.step_count==0)
  {
    print_line(out,"No program loaded");
    return PLCSIM_OK;
  }
  if (what & PLCSIM_DUMP_LIST)
  {
    dump_list(plc,out);
    if (what & PLCSIM_DUMP_LADDER) print_line(out,"");
  }
  if (what & PLCSIM_DUMP_LADDER) dump_ladder(plc,out);
  return PLCSIM_OK;
}

extern "C" int plcsim_dump(const plcsim_t *plc, FILE *out, unsigned what)
{
  if (out==NULL) return PLCSIM_ERR_ARG;
  return plcsim_dump_to(plc,write_file,out,what);
}
