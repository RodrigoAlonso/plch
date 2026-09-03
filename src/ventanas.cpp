/* =================================================
   |                                               |
   |                  Ventanas                     |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */

#include <graphics.h>
#include <string.h>
#include "dos_shim.h"
#include <stdlib.h>
#include <stdio.h>
#include "conio_shim.h"
#include <ctype.h>
#include "ventanas.h"

//----------------------------------------------------------------------------
// Generales
int MAXX,MAXY,modografico=-1;
unsigned alturaT=8, //  anchoT=textwidth("A");
         anchoT=8;  //  alturaT=textheight("A");

void mensaje(char *mensaj, int cl, int cf, int ct)
{
  int aux=strlen(mensaj);

  cajadialogo cd_mensaje((MAXX-(aux+4)*8)/2,120,60);
//  cd_error.setcolores(LIGHTRED,BLACK,RED);
  display *di=new display(mensaj,LongTexto,0,10);
  di->setcolores(cl,BLACK,cf,1,ct,1);
  cd_mensaje.insertar(di);
  boton *bo=new boton("  Ok  ",100,0,0,50,100);
  cd_mensaje.insertar(bo);
  eventoM ev;
  scanEvento(&ev);
  cd_mensaje.evento(&ev);
  delete di;
  delete bo;
}

void error(char *mensaj, int doom)  // si 'doom'==1 -> finaliza la ejecucion.
{
  if (modografico!=-1)
  {
    mensaje(mensaj,LIGHTRED,RED,YELLOW);
  } else
    {
      printf("\n");
      textcolor(LIGHTRED);
      printf(" ERROR: %s",mensaj);
      textcolor(LIGHTGRAY);
      printf("\n");
    }
  if (doom)
  {
    if (modografico!=-1) Finalizar();
    exit(EXIT_FAILURE);
  }
}

void minusculas(char *s)
{
  for (int aux=0; aux<strlen(s); aux++) s[aux]=tolower(s[aux]);
}

void mayusculas(char *s)
{
  for (int aux=0; aux<strlen(s); aux++) s[aux]=toupper(s[aux]);
}

//----------------------------------------------------------------------------
// Graficos
//----------------------------------------------------------------------------
void caja3d(int x, int y, unsigned ancho, unsigned alto, int cl, int cs,
            int cf)
{
  ancho-=1;
  alto-=1;
  setcolor(cl);
  setfillstyle(SOLID_FILL,cf);
  bar3d(x,y,x+ancho,y+alto,0,0);  // ancho-1 y alto-1
  setcolor(cs);
  moveto(x,y+alto);
  linerel(ancho,0);
  linerel(0,-alto);
}

void circulo3d(int x, int y, unsigned radio, int cl, int cs, int cf)
{
  radio--;
  setcolor(cs);
  setfillstyle(SOLID_FILL,cf);
  pieslice(x,y,0,360,radio);
  setcolor(cf);
  line(x,y,x+radio-1,y);
  setcolor(cl);
  arc(x,y,45,225,radio);
}

void pizarra3d(int x, int y, unsigned ancho, unsigned alto, int cl, int cs,
               int cf, int cpapel)
{
  ancho-=1;
  alto-=1;
  setcolor(cs);
  setfillstyle(SOLID_FILL,cpapel);
  bar3d(x,y,x+ancho,y+alto,0,0);
  rectangle(x+1,y+1,x+ancho-1,y+alto-1);
  setcolor(cl);
  moveto(x+1,y+alto);
  linerel(ancho-1,0);
  linerel(0,-alto);
  setcolor(cf);
  moveto(x+1,y+alto-1);
  linerel(ancho-2,0);
  linerel(0,-alto+2);
}

void hendiduraCaja(int x, int y, unsigned ancho, unsigned alto,
                   int cl, int cs)
{
  ancho-=1;
  alto-=1;
  setcolor(cl);
  rectangle(x+1,y+1,x+ancho,y+alto);
  setcolor(cs);
  rectangle(x,y,x+ancho-1,y+alto-1);
}

void hendiduraCirculo(int x, int y, unsigned radio, int cl, int cs)
{
  radio--;
  setcolor(cs);
  circle(x-1,y-1,radio);
  setcolor(cl);
  circle(x,y,radio);
}

extern SDL_Window* bgi_window; 
static void iniciarGraficos(void)
{
  int modog,driver=DETECT;
  
  // Initialize a 640x480 graphics window
  int ret = initwindow(640, 480, "PLC Hitachi Sim");
  SDL_SetWindowMinimumSize(bgi_window, 640, 480);

  if (graphresult()!=grOk)
  {
    error("No se pudo iniciar el modo grafico",1);
  }
  modografico=modog;
}

void setModo(int modo)
{
  setgraphmode(modo);
  modografico=modo;
}

void setFuente(int fuente, int tamanio)
{
  settextstyle(fuente,HORIZ_DIR,tamanio);
}

void hendidura(int x, int y, int x2, int y2, int cl, int cs)
{
  setcolor(cl);
  line(x,y,x2,y2);
  setcolor(cs);
  line(x+1,y+1,x2+1,y2+1);
}

//----------------------------------------------------------------------------
// Mouse
//----------------------------------------------------------------------------

struct iconoMouse
      iconoFlecha=
        {{{0x3FFF,0x1FFF,0xFFF,0x7FF,0x3FF,0x1FF,0xFF,0x7F,0x3F,0x3F,0x1FF,
           0x10FF,0x30FF,0xF87F,0xF87F,0xFC7F},
          {0x0,0x4000,0x6000,0x7000,0x7800,0x7C00,0x7E00,0x7F00,0x7F80,
           0x7C00,0x6C00,0x4600,0x600,0x300,0x300,0x0}},
           1,
           1},

      iconoMano=
        {{{0xFFFF,0xF3FF,0xE1FF,0xE1FF,0xE07F,0xE00F,0xE001,0xC000,0x8000,
           0x0,0x0,0x0,0x0,0x0,0x8001,0xC003},
          {0x0,0x0,0xC00,0xC00,0xC00,0xD80,0xDB0,0xDB6,0x2DB6,0x6DB6,
           0x6FFE,0x6FFE,0x7FFE,0x7FFE,0x3FFC,0x0}},
           4,
           2},

      iconoNulo=
        {{{0xFFFF,0xCFF3,0x87E1,0xC3C3,0xE187,0xF00F,0xF81F,0xFC3F,0xF81F,
           0xF00F,0xE187,0xC3C3,0x87E1,0xCFF3,0xFFFF,0xFFFF},
          {0x0,0x0,0x300C,0x1818,0xC30,0x660,0x3C0,0x180,0x3C0,0x660,0xC30,
           0x1818,0x300C,0x0,0x0,0x0}},
           8,
           7},

      iconoLapiz=
        {{{0xFFF1,0xFFE0,0xFFC0,0xFF80,0xFF01,0xFE03,0xFC07,0xF80F,0xF01F,
           0xE03F,0xC07F,0x80FF,0x81FF,0x83FF,0x7FF,0x3FFF},
          {0x0,0x6,0x16,0x30,0x6C,0xD8,0x1B0,0x360,0x6C0,0xD80,0xB00,0x2E00,
           0x2000,0x3800,0x0,0x0}},
           2,
           13},

      iconoProhibido=
        {{{0xFC3F,0xF00F,0xE007,0xC3C3,0x8FC1,0x8F81,0x1F18,0x1E38,0x1C78,
           0x18F8,0x81F1,0x83F1,0xC3C3,0xE007,0xF00F,0xFC3F},
          {0x0,0x3C0,0xC30,0x1008,0x2014,0x2024,0x4042,0x4082,0x4102,0x4202,
           0x2404,0x2804,0x1008,0xC30,0x3C0,0x0}},
           8,
           8},

/*      iconoSelec=            // una 'S' grande
        {{{0xF87F,0xF03F,0xE01F,0xC78F,0xC7DF,0xC7FF,0xE1FF,0xF07F,0xF83F,
           0xFE1F,0xFF8F,0xEF8F,0xC78F,0xE01F,0xF03F,0xF87F},
          {0x0,0x780,0x840,0x1020,0x1000,0x1000,0x800,0x600,0x180,0x40,0x20,
           0x20,0x1020,0x840,0x780,0x0}},
           8,
           8};*/

/*      iconoSelec=
        {{{0xFFF3,0xF3E1,0xE1C0,0xE1E1,0xE073,0xE00F,0xE001,0xC000,0x8000,0x0,
         0x0,0x0,0x0,0x0,0x8001,0xC003},
        {0x0,0xC,0xC12,0xC0C,0xC00,0xD80,0xDB0,0xDB6,0x2DB6,0x6DB6,0x6FFE,
         0x6FFE,0x7FFE,0x7FFE,0x3FFC,0x0}},
         4,
         2}; */

      iconoSelec=
        {{{0xFFFB,0xF3F1,0xE1E1,0xE1F1,0xE071,0xE00B,0xE001,0xC000,0x8000,0x0,
           0x0,0x0,0x0,0x0,0x8001,0xC003},
          {0x0,0x4,0xC0C,0xC04,0xC04,0xD80,0xDB0,0xDB6,0x2DB6,0x6DB6,0x6FFE,
           0x6FFE,0x7FFE,0x7FFE,0x3FFC,0x0}},
           4,
           2};

ptrIconoMouse picmo=NULL;


int bderant=0,bizqant=0;
int xmant = 0, ymant = 0;
void scanEvento(eventoM *ev)
{
     word a,b,boi=0,bod=0;
     int tecla;

	 int mouseX, mouseY;
	 Uint32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
	 
/*     asm {
           PUSHA
           MOV     AX,0x3
           INT     0x33
           MOV     a,CX
           MOV     b,DX
           MOV     BH,BL
           and     BL,1
           JZ      salto1
           MOV     boi,1
         }
salto1:
     asm {
            and     BH,2
            JZ      salto2
            MOV     bod,1
         }
salto2:
     asm {
            POPA
         }*/
		 
	 boi = mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT);
     bod = mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT);
	 
     ev->xm=mouseX;
     ev->ym=mouseY;
     ev->prei=!bizqant && boi;
     ev->pred=!bderant && bod;
     ev->soli=bizqant && !boi;
     ev->sold=bderant && !bod;
     ev->bizq=bizqant=boi;
     ev->bder=bderant=bod;
 	 
	 if (kbhit())
     {
       tecla=getch();
	   printf("Evento  keyboard: %d, %d\n", tecla);
       if (tecla==0)
       {
         ev->tecla=getch();
         ev->extendida=1;
       } else
         {
           ev->tecla=tecla;
           ev->extendida=0;
         }
     } else ev->tecla=0;

	 bool eventHappened = false;
	 //Debug
	 if (xmant != mouseX || ymant != mouseY || ev->prei || ev->pred || ev->soli || ev->sold) {
		//printf("Evento  mouse: %d, %d, %d, %d\n", mouseX, mouseY, boi, bod);
		eventHappened = true;
	 }

	 xmant = mouseX;
	 ymant = mouseY;

	 if (ev->tecla != 0) {
	 	//printf("Evento  keyboard: %d, %d\n", tecla, ev->extendida);
		eventHappened = true;
	 }
 	 
	 if (eventHappened) {
		refresh();
		delay(16);
	    fflush(stdout);
	 }
}

void flushEventos(void)
{
  while (kbhit()) getch();
}

void cambiarIconoMouse(struct iconoMouse *p)
{
   word s,o;
   int hs,vs;
   
   
   
   /*
   
   Ver:
   
   SDL_Cursor* cursorBw = SDL_CreateCursor(data, mask, 16, 16, 0, 0);
   
   */

   
   SDL_Cursor* cursorBw = SDL_CreateCursor((const unsigned char*)&(p->arreglo[0]), (const unsigned char*)&(p->arreglo[1]), 16, 16, p->hhs, p->vhs);
   
/*   if (p != picmo)
   {
     picmo=p;
     o=FP_OFF(p->arreglo);
     s=FP_SEG(p->arreglo);
     hs=p->hhs;
     vs=p->vhs;
     asm {
        MOV     AX,9
        MOV     BX,hs
        MOV     CX,vs
        MOV     DX,s
        MOV     ES,DX
        MOV     DX,o
        INT     0x33
     }
   }*/
}

void Iniciar(void)
{
	printf("VENTANAS::Iniciar()\n");
   iniciarGraficos();
   setFuente(DEFAULT_FONT,1);
   setModo(VGAHI);
   cambiarIconoMouse(&iconoFlecha);
   setwritemode(COPY_PUT);
   setfillstyle(SOLID_FILL,colorFondo);
   setpalette(6,6);
   MAXX=getmaxx();
   MAXY=getmaxy();
}

void Finalizar(void)
{
  closegraph();
  modografico=-1;
}

//----------------------------------------------------------------------------
// Implementacion de la Clase container
//----------------------------------------------------------------------------
container::container(void)
{
  nro=0;
  _actual=0;
}

container::container(int npunt)
{
  nro=0;
  _actual=0;
}

container::~container()
{
  nro=0;
  _actual=0;
}

int container::insertar(void *p)
{
  if (nro < maxPuntero)
  {
    vector[nro++]=p;
    return true;
  } else return false;
}

unsigned int container::buscar(void *p)
{
  for (int aux=0; aux<nro; aux++)
    if (vector[aux] == p) return aux;
  return -1;
}

int container::borrar(void *p)  // NO ANDA
{
  int aux=buscar(p),bux;

  if (aux != -1)
  {
    for (bux=aux; bux < nro-1; bux++) vector[bux]=vector[bux+1];
    if (_actual == nro--) _actual--;
    return true;
  } else return false;
}

int container::existe(void *p)
{
  int aux=buscar(p);

  if (aux!=-1)
  {
    _actual=aux;
    return true;
  } else return false;
}

void *container::siguiente()
{
  if (nro != 0)
  {
    if (_actual == nro-1) _actual=0;
      else _actual++;
    return vector[_actual];
  } else return NULL;
}

void *container::anterior()
{
  if (nro != 0)
  {
    if (_actual == 0) _actual=nro-1;
      else _actual--;
    return vector[_actual];
  } else return NULL;
}

void *container::primero()
{
  if (nro != 0)
  {
    _actual=0;
    return vector[_actual];
  } else return NULL;
}

void *container::ultimo()
{
  if (nro != 0)
  {
    _actual=nro-1;
    return vector[_actual];
  } else return NULL;
}

void *container::actual()
{
  if (nro != 0) return vector[_actual];
    else return NULL;
}

//----------------------------------------------------------------------------
// Implementacion de la Clase itemVisible
//----------------------------------------------------------------------------
int itemVisible::evento(eventoM *ev)
{
  if (interior(ev->xm,ev->ym))
  {
    if (picmo!=&iconoFlecha) cambiarIconoMouse(&iconoFlecha);
    return EV_NULO;
  } else return EV_AFUERA;
}

//----------------------------------------------------------------------------
// Implementacion de la Clase boton
//----------------------------------------------------------------------------
void boton::setear(const char *s, int _command, unsigned char t, unsigned char ap,
                   int _x, int _y)
{
  strncpy(texto,s,MaxLongBoton-1); texto[MaxLongBoton-1]='\0';
  anc=anchoT*(strlen(texto)+2);       // "+2" => 1 de cada lado (izq,der)
  alt=alturaT*2+alturaT/2;            // => 5/8 de cada lado (arr,aba)
  estado=NORMAL;
  x=_x;
  y=_y;
  command=_command;
  toggle=t;
  apret=ap;
}

void boton::dibujar(void)
{
  int c1=LIGHTCYAN,c2=colorSombra,c3=CYAN,bias=0,ctex=colorTexto,temp,c4;

  if (toggle)
  {
    if (apret) estado|=APRETADO;
    c1=LIGHTGREEN;
    c3=GREEN;
  }
  c4=c1;
  if ((estado&APRETADO)==APRETADO)         // enmascaro el bit APRETADO
  {
    temp=c2;
    c2=c1;
    c1=temp;
    bias=1;
    if (toggle) ctex=WHITE;
  }
  esconderMouse();
  caja3d(x,y,anc,alt,c1,c2,c3);
  setcolor(ctex);
  outtextxy(x+anchoT+bias,y+3*alturaT/4,texto);
  if ((estado&RESALTADO)==RESALTADO)       // enmascaro el bit RESALTADO
  {
    hendiduraCaja(x+anchoT/2+bias,y+alturaT/4,anc-anchoT-1,alt-(alturaT/2+1),c4,colorSombra);
  }
  mostrarMouse();
}

int boton::evento(eventoM *ev)
{
  int est,rt=EV_NULO;

  if (interior(ev->xm,ev->ym))
  {
    ev->sold=ev->bder=0;
    est=estado|RESALTADO;
    if (picmo!=&iconoMano) cambiarIconoMouse(&iconoMano);
    if (ev->bizq)
    {
      est|=APRETADO;
      rt=EV_NULO;
    } else
    if (ev->soli)
    {
      apret=!apret;
      if (!toggle)
      {
        rt=command;
      } else ev->soli=0;    // enmascaro el soltado del boton izquierdo
      if (!toggle || !apret) est&=~APRETADO;
    }
  } else
    {
      est=estado&~RESALTADO;
      if (!toggle || !apret) est&=~APRETADO;
      rt=EV_AFUERA;
    }
  if (est!=estado)
  {
    estado=est;
    dibujar();
  }
  return rt;
}

//----------------------------------------------------------------------------
// Implementacion de la Clase reticula
//----------------------------------------------------------------------------
reticula::reticula(unsigned f, unsigned c, unsigned anC, unsigned alC)
{
  filas=f;
  columnas=c;
  anchoC=anC;
  altoC=alC;
  setcolores(colorLuz,colorSombra,colorFondo,colorSombra,colorLuz,colorFondo);
  anc=anchoC*columnas;
  alt=altoC*filas;
  actual=-1;
  if ((contenido=(char *)malloc(filas*columnas))==NULL) error("No se pudo reservar memoria",1);
  limpiar();
}

reticula::~reticula()
{
  if (contenido!=NULL) free(contenido);
}

void reticula::setcolores(int clu, int cso,int cfo, int clu2, int cso2,int cfo2)
{
  colorL=clu;
  colorS=cso;
  colorF=cfo;
  colorL2=clu2;
  colorS2=cso2;
  colorF2=cfo2;
}

void reticula::dibujar(void)
{
  int fil,col,aux;

  setfillstyle(SOLID_FILL,colorF);
  esconderMouse();
  bar3d(x,y,x+anc-1,y+alt-1,0,0);
  for (fil=0; fil<filas; fil++)
  {
    aux=y+fil*altoC;
    setcolor(colorL);
    line(x,aux,x+anc-1,aux);
  }
  for (col=0; col<columnas; col++)
  {
    aux=x+col*anchoC;
    setcolor(colorL);
    line(aux,y,aux,y+alt-1);
    setcolor(colorS);
    line(aux+anchoC-1,y,aux+anchoC-1,y+alt-1);
  }
  for (fil=0; fil<filas; fil++)
  {
    aux=y+fil*altoC+altoC-1;
    setcolor(colorS);
    line(x,aux,x+anc-1,aux);
  }
  for (fil=0; fil<filas; fil++)
    for (col=0; col<columnas; col++)
    {
      if (*(contenido+col+fil*columnas)!=0)
        dibujar_uno(col,fil);
    }
  mostrarMouse();
}

void reticula::limpiar(void)
{
  memset(contenido,0,filas*columnas);
}

int reticula::getpos(int xm, int ym)
{
  int xau=(xm-x)/anchoC,yau=(ym-y)/altoC;

  return yau*columnas+xau;
}

void reticula::dibujar_uno(unsigned c, unsigned f)
{
  int c1,c2,c3;

  if (*(contenido+c+f*columnas)==0)
  {
    c1=colorL;
    c2=colorS;
    c3=colorF;
  } else
    {
      c1=colorL2;
      c2=colorS2;
      c3=colorF2;
    }
  esconderMouse();
  caja3d(x+c*anchoC,y+f*altoC,anchoC,altoC,c1,c2,c3);
  mostrarMouse();
}

int reticula::evento(eventoM *ev)
{
  int rt;

  if (interior(ev->xm,ev->ym))
  {
    if (picmo!=&iconoMano) cambiarIconoMouse(&iconoMano);
    if (ev->bizq || ev->bder)
    {
      int col=(ev->xm-x)/anchoC,fil=(ev->ym-y)/altoC;
      int act=fil*columnas+col;
      if (ev->bizq)
      {
        if (actual!=act || *(contenido+col+fil*columnas)==0)
        {
          actual=act;
          *(contenido+col+fil*columnas)=1;
          dibujar_uno(col,fil);
        }
      }
      if (ev->bder)
      {
        if (actual!=act || *(contenido+col+fil*columnas)==1)
        {
          actual=act;
          *(contenido+col+fil*columnas)=0;
          dibujar_uno(col,fil);
        }
        ev->bder=0;  // Enmascaro el presionado del boton derecho
      }
    }
    rt=EV_NULO;
    ev->soli=0;    // Enmascaro el soltado del boton izquierdo, que hace
                   // las veces de 'return'.
    ev->sold=0;
  } else rt=EV_AFUERA;
  return rt;
}

//----------------------------------------------------------------------------
// Implementacion de la Clase barraDespHorizontal
//----------------------------------------------------------------------------
void barraDesplazamiento::setear(int _largo, int _menor, int _mayor,
                                 int _pos, int _x, int _y)
{
  largo=_largo;
  menor=_menor;
  mayor=_mayor;
  posselectora=(largo/(mayor-menor))*(_pos-menor);
  x=_x;
  y=_y;
  prepos=0;
}

/*int barraDesplazamiento::evento(eventoM *ev)
{
  int est,rt=EV_NULO;

  if (interior(ev->xm,ev->ym))
  {
    est=estado|RESALTADO;
    if (picmo!=&iconoMano) cambiarIconoMouse(&iconoMano);
    if (ev->bizq)
    {
      est|=APRETADO;
      rt=EV_NULO;
    } else
    if (ev->soli)
    {
      apret=!apret;
      if (!toggle)
      {
        rt=command;
      } else ev->soli=0;
      if (!toggle || !apret) est&=~APRETADO;
    }
  } else
    {
      est=estado&~RESALTADO;
      if (!toggle || !apret) est&=~APRETADO;
      rt=EV_AFUERA;
    }
  if (est!=estado)
  {
    estado=est;
    dibujar();
  }
  return rt;
}*/

//----------------------------------------------------------------------------
// Implementacion de la Clase barraDespHorizontal
//----------------------------------------------------------------------------
#define anchosel 11
#define altosel 16
barraDespHorizontal::barraDespHorizontal(int _largo, int _menor,
                                              int _mayor, int _pos,
                                              int _x, int _y) :
             barraDesplazamiento(_largo,_menor,_mayor, _pos,_x,_y)
{
  anc=largo+anchosel+2;
  alt=altosel+2+7;
}

void barraDespHorizontal::dibSelectora(int borrar)
{
  int col1,col2,col3;
  if (!borrar)
  {
    col1=LIGHTCYAN;
    col2=DARKGRAY;
    col3=CYAN;
  } else col1=col2=col3=colorFondo;
  esconderMouse();
  caja3d(x+1+posselectora,y+1+1,anchosel,altosel,col1,col2,col3);
  if (!borrar)
  {
    setcolor(colorSombra);
    moveto(x+1+posselectora+anchosel/2,y+1+1+altosel/3*2);
    linerel(0,altosel/3);
  }
  mostrarMouse();
}

void barraDespHorizontal::dibujar(void)
{
  int rango=mayor-menor;

  esconderMouse();
  caja3d(x,y+1,anc,alt-2,colorSombra,colorLuz,colorFondo);
//  line(x+1,y-10,x+largo+1,y-10);
//  hendidura(100,106,180,106,DARKGRAY,WHITE);
//  caja3d(x,y+1,anc,altosel+2,colorSombra,colorLuz,colorFondo);

  setcolor(colorSombra);
  for (int aux=0; aux<=rango; aux++)
  {
    int xsel=(largo/rango)*(aux);
    line(x+1+xsel+anchosel/2,y+altosel+2+1,x+1+xsel+anchosel/2,y+altosel+2+4);
  }
  dibSelectora(0);
  mostrarMouse();
}

int barraDespHorizontal::evento(eventoM *ev)
{
  int rt=EV_NULO;

  if (interior(ev->xm,ev->ym))
  {
    if (prepos && ev->bizq)
    {
      if (ev->xm > x+1 && ev->ym >y+2 &&
          ev->xm < x+anc-2 && ev->ym < y+2+altosel)
      {
        if (ev->xm!=prepos)
        {
          dibSelectora(1);
          posselectora+=ev->xm-prepos;
          if (posselectora<0) posselectora=0;
          if (posselectora>anc-2-anchosel) posselectora=anc-2-anchosel;
          dibSelectora(0);
          prepos=ev->xm;
        }
      } else prepos=0;
    } else
    if (ev->xm > x+1+posselectora && ev->ym >y+2 &&
        ev->xm < x+1+posselectora+anchosel && ev->ym < y+2+altosel)
    {
        if (picmo!=&iconoFlecha) cambiarIconoMouse(&iconoFlecha);
        if (ev->bizq) prepos=ev->xm;
          else prepos=0;
    } else
      {
        prepos=0;
        rt=EV_AFUERA;
      }
    ev->soli=0;    // enmascaro la soltada del boton izquierdo
  }
  return rt;
}

//----------------------------------------------------------------------------
// Implementacion de la Clase barraDespVertical
//----------------------------------------------------------------------------
barraDespVertical::barraDespVertical(int _largo, int _menor, int _mayor,
                                          int _pos, int _x, int _y) :
             barraDesplazamiento(_largo,_menor,_mayor, _pos,_x,_y)
{
  anc=altosel+2+5;
  alt=largo+anchosel+2;
}

void barraDespVertical::dibSelectora(int borrar)
{
  int col1,col2,col3;
  if (!borrar)
  {
    col1=LIGHTCYAN;
    col2=DARKGRAY;
    col3=CYAN;
  } else col1=col2=col3=colorFondo;
  caja3d(x+1,y+1+posselectora,altosel,anchosel,col1,col2,col3);
}

void barraDespVertical::dibujar(void)
{
  int rango=mayor-menor;

  esconderMouse();
//  line(x+1,y-10,x+largo+1,y-10);
  caja3d(x,y,altosel+2,alt,colorSombra,colorLuz,colorFondo);
//  hendidura(100,106,180,106,DARKGRAY,WHITE);
//  setcolor(BLACK);
//  line(100,106,180,106);

  setcolor(colorSombra);
  for (int aux=0; aux<=rango; aux++)
  {
    int xsel=(largo/rango)*(aux);
    line(x+altosel+2+1,y+1+xsel+anchosel/2,x+altosel+2+4,y+1+xsel+anchosel/2);
  }

  dibSelectora(0);

  mostrarMouse();
}

/*int barraDespVertical::evento(eventoM *ev)
{
} */

//----------------------------------------------------------------------------
// Implementacion de la Clase display
//----------------------------------------------------------------------------
void display::setcolores(int clu, int cso,int cfo, int cpa, int cti, int dcb)
{
  cl=clu;
  cs=cso;
  cf=cfo;
  cp=cpa;
  ct=cti;
  dibcomoboton=dcb;
  if (dcb) cp=cf;
}

void display::setear(const char *s, int caract, int _x, int _y)
{
  x=_x;
  y=_y;
  if (caract==LongTexto) caracteres=strlen(s);
    else caracteres=caract;
  anc=anchoT*(caracteres+1);
  alt=alturaT*2+alturaT/2;
//  strncpy(texto,s,caracteres);
  texto=(char *)s;
//  texto[caracteres]='\0';
  setcolores(colorLuz,colorSombra,colorFondo,WHITE,colorTexto,0);
}

void display::dibujar(void)
{
  char aux=texto[caracteres];

  esconderMouse();
//  if (refrescar)
  {
    if (dibcomoboton) caja3d(x,y,anc,alt,cl,cs,cf);
      else pizarra3d(x,y,anc,alt,cl,cs,cf,cp);
  }
/*  else
  {
    setfillstyle(SOLID_FILL,cp);
    setcolor(cp);
    bar3d(x+anchoT/2,y+alturaT/2,x+caracteres*anchoT,y+alturaT+alturaT/2,0,0);
  }*/
  setcolor(ct);
  texto[caracteres]='\0';
  outtextxy(x+anchoT/2,y+3*alturaT/4,texto);
  texto[caracteres]=aux;
  mostrarMouse();
}

//----------------------------------------------------------------------------
// Implementacion de la clase lineaInput
//----------------------------------------------------------------------------
lineaInput::lineaInput(void) :display()
{
  longcad=0;
  posi=-1;
  cc=LIGHTRED;
}

lineaInput::lineaInput(char *s, int caract, int _x, int _y)
{
  longcad=0;
  posi=-1;
  cc=LIGHTRED;
  setear(s,caract,_x,_y);
  texto[0]='\0';
}

void lineaInput::dib_cursor(int colcur)
{
  setcolor(colcur);
  esconderMouse();
  outtextxy(x+anchoT/2+anchoT*posi,y+3*alturaT/4+1,"_");
  outtextxy(x+anchoT/2+anchoT*posi,y+3*alturaT/4+2,"_");
  mostrarMouse();
}

void lineaInput::dibujar(void)
{
  display::dibujar();
  longcad=min(strlen(texto),caracteres);
}

void insertar(char i, int pos, char *s)
{
  int aux;

  for (aux=strlen(s)+1; aux>pos; aux--)
    s[aux]=s[aux-1];
  s[aux]=i;
}

int borrar(int pos, char *s)
{
  int aux;

  for (aux=pos; s[aux]!='\0'; aux++)
    s[aux]=s[aux+1];
  return aux!=pos?1:0;
}

int lineaInput::evento(eventoM *ev)
{
  int posiaux=posi,dib=0;

  if (interior(ev->xm,ev->ym))
  {
    if (picmo!=&iconoLapiz) cambiarIconoMouse(&iconoLapiz);
    if (posi==-1)
    {
      posiaux=posi=min(longcad,caracteres-1);
      dib_cursor(cc);
    }
    if (ev->tecla)
    {
      if (ev->extendida)
      {
        switch (ev->tecla)
        {
          case IZQUIERDA: if (posi>0) posiaux--;
                          break;
          case DERECHA  : if (posi<longcad && posi<caracteres) posiaux++;
                          break;
          case HOME     : posiaux=0;
                          break;
          case END      : posiaux=min(longcad,caracteres-1);
                          break;
          case DELETE   : if (borrar(posi,texto))
                          {
                            longcad--;
                            dib=1;
                          }
                          break;
        }
      } else
        switch (ev->tecla)
        {
          case BACKSPACE: if (posi>0)
                          {
                            posiaux--;
                            borrar(posiaux,texto);
                            longcad--;
                            dib=1;
                          }
                          break;
          default: if (longcad<=caracteres)
                   {
                     dib=1;
                     insertar(ev->tecla,posiaux,texto);
                     if (longcad<caracteres) longcad++;
                     if (posiaux<caracteres-1) posiaux++;
                     texto[longcad]='\0';
                   }
        }
      if (dib) dibujar();
      if (posi!=posiaux || dib)
      {
        dib_cursor(cp);
        posi=posiaux;
        dib_cursor(cc);
      }
    }
    return EV_NULO;
  } else
    {
      if (posi!=-1)
      {
        dib_cursor(cp);
        posi=-1;
      }
      return EV_AFUERA;
    }
}

//----------------------------------------------------------------------------
// Implementacion de la Clase bolsa_de_items
//----------------------------------------------------------------------------
bolsa_de_items::bolsa_de_items()
{
  actual=NULL;
  iterando=-1;
}

bolsa_de_items::~bolsa_de_items(void)
{
/*  boton *bux;

  resetIterar(ACTUAL);
  while ((bux=(boton *)iterar()) != NULL)
    delete bux;*/
}

int bolsa_de_items::insertar(itemVisible *iv)
{
  if (actual==NULL)
  {
    actual=iv;  // el 'actual' es el primero que se inserta
  }
  return c.insertar(iv);
}

void bolsa_de_items::mover(int dx, int dy)
{
  itemVisible *bux;

  while ((bux=(itemVisible *)iterar()) != NULL)
    bux->mover(dx,dy);
  x+=dx;
  y+=dy;
}

int bolsa_de_items::evento(eventoM *ev)
{
  itemVisible *bux;

  if (ev->tecla==ESCAPE)
  {
    ev->soli=1;
    return EV_TERMINAR;
  } else
  while ((bux=(itemVisible *)iterar()) != NULL) {
   if (bux->interior(ev->xm,ev->ym))   // interior a alguno de los items
   {
     resetIterar(ACTUAL);
     if (actual!=bux)          // uno_nuevo
     {
       actual->evento(ev);
       actual=bux;
     }
     return bux->evento(ev);
   }
   refresh();
   delay(16);
  }
  return actual->evento(ev);
}

void bolsa_de_items::dibujar(void)
{
  itemVisible *bux;

  resetIterar(PRIMERO);
  while ((bux=(itemVisible *)iterar()) != NULL)
    bux->dibujar();
}

void bolsa_de_items::dibujar(int _x, int _y)
{
  mover(_x-x,_y-y);
  dibujar();
}

void bolsa_de_items::resetIterar(int modo)
{
  iterando=c.getNumero();
  nro=0;
  if (modo==PRIMERO) c.ultimo();
    else c.anterior();
}

itemVisible *bolsa_de_items::iterar(void)
{
  if (iterando==-1) resetIterar(PRIMERO);
  if (nro<iterando)
  {
    nro++;
    return (itemVisible *)c.siguiente();
  }
  iterando=-1;
  return NULL;
}

//----------------------------------------------------------------------------
// Implementacion de la Clase item_con_fondo
//----------------------------------------------------------------------------
void item_con_fondo::reservar(int anc, int alt)
{
  pointer=malloc(imagesize(0,0,anc,alt));
  if (pointer==NULL) error("No se pudo reservar memoria",1);
}

void item_con_fondo::liberar(void)
{
  if (pointer!=NULL)
  {
    free(pointer);
    pointer=NULL;
  }
}

void item_con_fondo::dibujar_fondo(int *x, int *y, unsigned ancho, unsigned alto,
                                   int cl, int cs, int cf)
{
  if (pointer==NULL)
  {
    reservar(ancho,alto);
  }
  *x=min(*x,MAXX-ancho);
  *y=min(*y,MAXY-alto);
  esconderMouse();
  getimage(*x,*y,*x+ancho,*y+alto,pointer);
  caja3d(*x,*y,ancho,alto,cl,cs,cf);
  mostrarMouse();
}

//----------------------------------------------------------------------------
// Implementacion de la Clase menu
//----------------------------------------------------------------------------
menu::menu() : bolsa_de_items(), item_con_fondo()
{
  anc=0;
  alt=alturaT;
  x=y=0;
}

menu::~menu()
{
}

int menu::insertar(itemVisible *iv)
{
  iv->setxy(x+anchoT/2,y+alt+alturaT/2-alturaT);
  calc_anc_alt(iv);
  return bolsa_de_items::insertar(iv);
}

void menu::calc_anc_alt(itemVisible *iv)
{
  int aan=0,aal=0;

  iv->getanal(&aan,&aal);
  anc=max(anc,aan+anchoT);
  alt+=aal;
}

void menu::dibujar(void)
{
  int vx=x,vy=y;

  dibujar_fondo(&vx,&vy,anc,alt,colorLuz,colorSombra,colorFondo);
  mover(vx-x,vy-y);
  bolsa_de_items::dibujar();
}

int menu::evento(eventoM *ev)
{
  int aux,ax=ev->xm,ay=ev->ym;

  bolsa_de_items::dibujar(ev->xm,ev->ym);
  do
  {
    scanEvento(ev);
    aux=bolsa_de_items::evento(ev);
    if (aux==EV_AFUERA && picmo!=&iconoNulo) cambiarIconoMouse(&iconoNulo);
    if (ev->bder && (ax!=ev->xm || ay!=ev->ym))
    {
      esconderMouse();
      putimage(x,y,pointer,COPY_PUT);
      mostrarMouse();
      bolsa_de_items::dibujar(ev->xm,ev->ym);
      ax=ev->xm;
      ay=ev->ym;
    }
  } while (aux<1 && !ev->soli && aux!=EV_TERMINAR);
  esconderMouse();
  putimage(x,y,pointer,COPY_PUT);
  mostrarMouse();
  liberar();
  return aux;
}

//----------------------------------------------------------------------------
// Implementacion de la Clase cajadialogo
//----------------------------------------------------------------------------
cajadialogo::cajadialogo(int _x, int _y, int al, int an)
                   : bolsa_de_items(), item_con_fondo()
{
  setear(_x,_y,al,an);
}

cajadialogo::~cajadialogo()
{
}

void cajadialogo::setear(int _x, int _y, int al, int an)
{
  x=_x;
  y=_y;
  alt=((alflot=(al ? 0 : 1)) ? alturaT*2: alturaT*2+al);
  anc=((anflot=(an ? 0 : 1)) ? anchoT*2 : anchoT*2+an);
  setcolores(LIGHTGREEN,BLACK,GREEN);  // colores por omision
}

void cajadialogo::setcolores(int clu, int cso,int cfo)
{
  cl=clu;
  cs=cso;
  cf=cfo;
}

//La posicion x,y de los items debe especificarse como porcentaje(de 1 a 100).
int cajadialogo::insertar(itemVisible *iv)
{
  int aux=bolsa_de_items::insertar(iv);
  calc_anc_alt(iv);
//  iv->mover(x+anchoT,y+alturaT);
  return  aux;
}

void cajadialogo::calc_anc_alt(itemVisible *iv)
{
  int aan=0,aal=0;
//  int _x,_y;

  iv->getanal(&aan,&aal);
//  iv->getxy(&_x,&_y);
  if (anflot) anc=max(anc,aan+anchoT*2);
//    else _x=x+(anc-anchoT*2)*_x/100+anchoT-(aan/2);
  if (alflot) alt+=aal;
//    else _y=y+(alt-alturaT*2)*_y/100+alturaT-(aal/2);
//  iv->setxy(_x,_y);
//  anc=max(anc,aan+_x+anchoT*2);
//  alt=max(alt,aal+_y+alturaT*2);
}

void cajadialogo::calc_posiciones(void)
{
  itemVisible *bux;
  int vx,vy,aan,aal;

  anflot=alflot=0;
  resetIterar(PRIMERO);
  while ((bux=(itemVisible *)iterar()) != NULL)
  {
    bux->getxy(&vx,&vy);
    bux->getanal(&aan,&aal);
    vx=x+(((anc-anchoT*2)-aan)*vx/100)+anchoT;
    vy=y+(((alt-alturaT*2)-aal)*vy/100)+alturaT;
    bux->setxy(vx,vy);
  }
}

void cajadialogo::dibujar(void)
{
  int vx=x,vy=y;

  if (alflot||anflot) calc_posiciones();
  dibujar_fondo(&vx,&vy,anc,alt,cl,cs,cf);  // dibuja el fondo DENTRO de los
                                            // limites de la pantalla
  mover(vx-x,vy-y); // si no entraba el fondo en la pantalla y hubo que
                    // moverlo => muevo todos los items.
  esconderMouse();
  hendiduraCaja(x+anchoT/2,y+alturaT/2,anc-anchoT,alt-alturaT,cl,cs);
  mostrarMouse();
  bolsa_de_items::dibujar();
}

int cajadialogo::evento(eventoM *ev)
{
  int aux,ax=ev->xm,ay=ev->ym;

  dibujar();
  do
  {
    scanEvento(ev);
    aux=bolsa_de_items::evento(ev);
    if (aux==EV_AFUERA && picmo!=&iconoNulo) cambiarIconoMouse(&iconoNulo);
    if (ev->bder && (ax!=ev->xm || ay!=ev->ym))
    {
      esconderMouse();
      putimage(x,y,pointer,COPY_PUT);
      mostrarMouse();
      bolsa_de_items::dibujar(ev->xm,ev->ym);
      ax=ev->xm;
      ay=ev->ym;
    }
  } while (aux<1 && aux!=EV_TERMINAR);
  esconderMouse();
  putimage(x,y,pointer,COPY_PUT);
  mostrarMouse();
  liberar();
  return aux;
}
