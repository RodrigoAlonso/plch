/* =================================================
   |                                               |
   |               Simulacion de PLC               |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
/*
   " Un nivel mas de indireccion es un nivel menos de problemas "
*/
/*
   " Un chip puede verse y sentirse como hardware, si se lo considera tal
     como esta acu¤ado, en silicio, pero no es mas que software que fue
     comprimido y convertido en hardware.
     (...)
     Las simulaciones son una cosa pero todos sabemos que no existe manera
     real de ejecutar todas las combinaciones posibles de entradas a una
     CPU para detectar aquellas que funcionan bien y las que no funcionan
     de manera correcta."
                                        Raphael Needleman
                                        Redactor de BYTE USA.
*/
/*
   Existe un isomorfismo entre los operadores booleanos OR y AND, y los
   operadores de conjuntos Union e Interseccion, respectivamente.
   Un OR agranda o mantiene el conjunto de posibilidades,
   un AND lo restringe.
*/
//mostprog.cpp

#include <stdlib.h>
#include "plc.h"
#include "soporte.h"
#include "conio_shim.h"
#include <string.h>
#include <ventanas.h>

//----------------------------------------------------------------------------
// elemento
/*
    Palabra de Estado:


          Se usan durante la             Se establecen durante
          simulacion dinamica            la creacion del rung
                   |                               |
         /---------^--------\ /--------------------^---------------------\

    Bit    15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
         |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
         |   | X | X | X | X | X |   |   |   | X | X | X |   | X | X | X |
         |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

           ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^  \----------/
           |   |   |   |   |   |   |   |   |   |   |   |   |        |
         --|   |   |   |   |   |   |   |   |   |   |   |   |        |
               |   |   |   |   |   |   |   |   |   |   |   |       ver
    Anterior --|   |   |   |   |   |   |   |   |   |   |   |   codificacion
                   |   |   |   |   |   |   |   |   |   |   |     mas abajo
    Desde Arriba --|   |   |   |   |   |   |   |   |   |   |
                       |   |   |   |   |   |   |   |   |   |
         Desde Abajo --|   |   |   |   |   |   |   |   |   |--
                           |   |   |   |   |   |   |   |
         Desde Izquierda --|   |   |   |   |   |   |   |-- Hacia Abajo
                               |   |   |   |   |   |
               Operando Rele --|   |   |   |   |   |-- Hacia arriba
                                   |   |   |   |
                                 --|   |   |   |-- Hacia Derecha
                                       |   |
                                     --|   |--


                 Desde Arriba
                   |----|
                   |    |                              Hacia Arriba
                   |----|
                      .            |           |            .
           |----|     .            |       /   |            .
   Desde   |    | .....============|     /     |============....   Hacia
 izquierda |----|     .            |   /       |            .     Derecha
                      .            |           |            .
                 Hacia Abajo                              |----|
                                                          |    |
                                                          |----|
                                                        Desde Abajo
*/

#define VACIO      0x0000
#define NA         0x0001
#define NC         0x0002
#define Output     0x0003
#define OutputNot  0x0004
#define TMR        0x0005
#define CNTR       0x0006
#define FCTN       0x0007
#define Mask3b     0x0007 // mascara de los primeros 3 bits
//#define            0x0008
#define HAbajo     0x0010
#define HArriba    0x0020
#define HDer       0x0040
//#define            0x0080
//#define            0x0100
//#define            0x0200
#define Opre       0x0400
#define DIzq       0x0800
#define DAba       0x1000
#define DArr       0x2000
#define Ante       0x4000
//#define Marcado    0x8000

struct elemento
{
  unsigned short int memoria;
  unsigned short int estado;
};

struct rung_data
{
  int primfil;
  int priminst;
  unsigned char nfil;
//  unsigned char ninst;
};

//----------------------------------------------------------------------------
// visor
#define ANCHO_RUNG 10
#define ALTO_RUNG 250
#define ALTO_Pantalla 15
#define ALTO_Final (ALTO_RUNG*2+ALTO_Pantalla)
class visor // una clase ad hoc para mostrar los rungs que componen un programa
{
  private:
    elemento tabla[ALTO_RUNG][ANCHO_RUNG]; // tabla temporaria, para crear rungs
    elemento tabla_final[ALTO_Final][ANCHO_RUNG];
    int dims[10];
//    int nroinst[ALTO_RUNG*2+ALTO_Pantalla+1];

    rung_data rungs[ALTO_Final];
    int nrungs;
    int primfil,ultfil;
    int fila_tope;
    plc *p;
//    int nro_filas,nro_columnas;
  public:
    visor(plc *_p);
    int ini_rung(int tipo, int x, int y, int memoria, int tipo_op);
    int agregar(int tipo, int x, int y);
    void ini_visor(void);
    void scroll(int direccion);
    void dibujar(void);
//    void dibujar(plc *p, int x1, int y1, int x2, int y2);
    void actualizar(void);
    void limpiar(void) { memset(tabla,VACIO,(ANCHO_RUNG*ALTO_RUNG)*(sizeof(elemento))); }
};

/*
  Interpretacion de los valores del arreglo dims.

                  [0]  [1]         [2]         [3]         [4]  [5]
                   .    .           .           .           .    .
                   .    .           .           .           .    .
 [6] . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
                   .    .           |           |           .    .
                   .    .           |       /   |           .    .
 [7] . . . . . . . -----============|     /     |============----- . . . . .
                   .    .           |   /       |           .    .
                   .    .           |           |           .    .
 [8] . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
                   .    .           .           .           .    .
                   .    .           .           .           .    .
*/

visor::visor(plc *_p)
{
  dims[0]=0;
  dims[1]=7;
  dims[2]=23;
  dims[3]=39;
  dims[4]=55;
  dims[5]=63;
  dims[6]=0;
  dims[7]=15;
  dims[8]=31;
//  instruccion_base=instruccion_tope=0;
  primfil=ultfil=fila_tope=0;
  nrungs=0;
  p=_p;
//  limpiar();
  memset(tabla_final,VACIO,(ANCHO_RUNG*ALTO_Final)*(sizeof(elemento)));
  ini_visor();
  dibujar();
}

int visor::agregar(int tipo, int x, int y)
{
  if (x<ANCHO_RUNG && y<ALTO_RUNG)
   return tabla[y][x].estado|=tipo;
  else return 0;
}

int visor::ini_rung(int tipo, int x, int y, int memoria, int tipo_op)
{
  if (x<ANCHO_RUNG && y<ALTO_RUNG)
  {
    tabla[y][x].memoria=memoria;
    tabla[y][x].estado|=tipo_op;
  }
  return agregar(tipo,x,y);
}

void visor::ini_visor(void)
{
  int nfil,ninst=0;

//  nroinst[fila_base]=ninst=0;
  while (fila_tope<ALTO_Final && p->crear_rung(this,&ninst,&nfil)==1)
  {
    rungs[nrungs].primfil=fila_tope;
    rungs[nrungs].priminst=ninst;
    rungs[nrungs].nfil=nfil;
//    rungs[nrungs].ninst=ninst;
    memcpy(&(tabla_final[fila_tope][0]),&(tabla[0][0]),
           ANCHO_RUNG*nfil*sizeof(elemento));
    fila_tope+=nfil;
//    nroinst[fila_tope]=ninst;
    nrungs++;
  }
  if (fila_tope>=ALTO_Pantalla) ultfil=ALTO_Pantalla;
    else ultfil=fila_tope;
  dibujar();
}

#define Arriba 1
#define Abajo 2
void visor::scroll(int direccion)
{
  int nfil,ninst=0;
  if (direccion==Abajo)
  {
    if (ultfil<fila_tope)
    {
      primfil++;
      ultfil++;
      dibujar();
    } else
    if (ultfil==fila_tope)
    {
    }
  } else // Arriba
    {
      if (primfil>0)
      {
        primfil--;
        ultfil--;
        dibujar();
      } else
      if (primfil==0)
      {
      }
    }
}

int plc::crear_rung(visor *rg, int *ninst, int *nfilas) const
{
#define maxpila 120
  int pila[maxpila],nroInst,temp,oldbase,ultimabasey=0;
  unsigned char opcode;
  int cabeza=0;
  int x=0,y=0,basex=0,basey=0;
  unsigned int primera_vez=1,primer_out=0;
  unsigned int dir_operando,tipo_oper,salida,continuar=1;

  for (nroInst=*ninst; nroInst<nro_instruc && continuar; )
  {
    opcode=instruc[nroInst].opcode;
    dir_operando=instruc[nroInst].operando;
    if (instruc[nroInst].tipo & OP_RELE || (instruc[nroInst].tipo & FUNCION))
      tipo_oper=Opre;
     else tipo_oper=0;
    if (instruc[nroInst].tipo & INSTRUCCION)
    {
     if (opcode==ORG || opcode==STR_AND || opcode==STR_OR||
         opcode==AND || opcode==OR)
       salida=NA;
     else if (opcode==OUT)
          {
            if (instruc[nroInst].tipo & OP_RELE) salida=Output;
              else salida=TMR;
          } else if (opcode==OUT_NOT) salida=OutputNot;
                 else salida=NC;
    }
    switch (opcode)
    {
      default:
      case ORG_NOT:
      case ORG  : if (primera_vez)
                  {
                    primera_vez=0;
                    rg->limpiar();
                    rg->ini_rung(salida|DIzq,basex,basey,dir_operando,tipo_oper);
                  } else continuar=0;
                  break;
      case STR_NOT_AND:
      case STR_AND:
                  push(pila,&cabeza,maxpila,basex);
                  push(pila,&cabeza,maxpila,y);
                  rg->agregar(HDer,x,basey);
                  ultimabasey=y=basey;
                  x++;
                  basex=x;
                  rg->ini_rung(salida,basex,basey,dir_operando,tipo_oper);
                  break;
      case STR_NOT_OR:
      case STR_OR :
                  push(pila,&cabeza,maxpila,basey);
                  push(pila,&cabeza,maxpila,x);
                  x=basex;
                  rg->agregar(HAbajo,basex,ultimabasey);
                  y++;
                  ultimabasey=basey=y;
                  rg->ini_rung(salida,basex,basey,dir_operando,tipo_oper);
                  break;
      case OR_NOT:
      case OR:    rg->agregar(HAbajo,basex,ultimabasey);
                  y++;
                  ultimabasey=y;
                  rg->ini_rung(salida,basex,y,dir_operando,tipo_oper);
                  if (basex < x)
                    rg->agregar(HDer,basex,y);
                  rg->agregar(HArriba,x,y);
                  break;
      case AND_NOT:
      case AND  : if (primer_out==1 || primer_out==2)
                  {
                    if (primer_out==1)
                    {
                      rg->agregar(HAbajo,x+1,basey-1);
                      primer_out=2;
                    }
                    rg->agregar(HDer,x+1,basey);
                  } else rg->agregar(HDer,x,basey);
                  x++;
                  rg->ini_rung(salida,x,basey,dir_operando,tipo_oper);
                  break;
      case AND_STR:
                  ultimabasey=pop(pila,&cabeza);
                  if (y < ultimabasey) y=ultimabasey;
                  basex=pop(pila,&cabeza);
                  break;
      case OR_STR:
                  oldbase=basey;
                  temp=pop(pila,&cabeza);
                  basey=pop(pila,&cabeza);
                  if (x > temp)
                  {
                    rg->agregar(HDer,temp,basey);
                  } else
                    if (x < temp)
                    {
                      rg->agregar(HDer,x,oldbase);
                      x=temp;
                    }
                  rg->agregar(HArriba,x,oldbase);
                  break;
      case OUT_NOT:
      case OUT  : if (primer_out==0)
                  {
                    rg->agregar(HDer,x,basey);
                    primer_out=1;
                  } else
                    if (primer_out==1)
                    {
                      rg->agregar(HAbajo|HDer,x+1,basey-1);
                      rg->agregar(HDer,x+1,basey);
                    }
                  rg->ini_rung(salida,ANCHO_RUNG-1,basey,dir_operando,tipo_oper);
                  basey++;
                  break;
    } // end_switch
    if (continuar) nroInst++;
  } // end_for
  if (nroInst>=nro_instruc && primera_vez==0 || continuar==0)
  {
    *ninst=nroInst;
    *nfilas=((y>=basey)?y+1:basey);
//    *nfilas=y+1;
    return 1;
  } else return -1;
} // end_crear

void plc::mostrar_programa(eventoM *ev)
{
//  eventoM ev;
  unsigned char salir=0;
  visor rg(this);
/*
    visor(void);
    ~visor();
    int ini_rung(int tipo, int x, int y, int memoria, int tipo_op);
    int agregar(int tipo, int x, int y);
    void ini_visor(const plc *p);
    void scroll(int direccion);
    void dibujar(const plc *p);
    void actualizar(const plc *p);
    void limpiar(void) { memset(tabla,VACIO,(ANCHO_RUNG*ALTO_RUNG)*(sizeof(elemento))); }
*/

//  rg.ini_visor(this);
  do
  {
    simular();

    scanEvento(ev);
    if (ev->tecla)
    {
      if (ev->extendida)
      {
        switch (ev->tecla)
        {
          case ARRIBA:
            rg.scroll(Arriba);
            break;
          case ABAJO:
            rg.scroll(Abajo);
            break;
/*          case HOME:
           posiaux=0;
           break;
          case END:
           posiaux=min(longcad,caracteres-1);
           break;*/
        }
      } else
        if (ev->tecla==ESCAPE) salir=1;
    }
  } while (!salir);
}

#define DEBUG 1
void linea(int x1, int y1, int x2, int y2)
{
  int minx=0,miny=0;
  int maxx=639,maxy=479;

  if ((y1<maxy && y1>miny) || (y2<maxy && y2>miny))
  {
    if (y1>maxy) y1=maxy;
    if (y1<miny) y1=miny;
    if (y2>maxy) y2=maxy;
    if (y2<miny) y2=miny;
  }
  if (x1>maxx) x1=maxx;
  if (x1<minx) x1=minx;
  if (y1<miny) y1=miny;
  if (x2>maxx) x2=maxx;
  if (x2<minx) x2=minx;
  line(x1,y1,x2,y2);
}

void visor::dibujar(void)
{
  int a,b,u,v,aux,bux;
  unsigned char operando,transmitir;

  cleardevice();
  for (a=ultfil; a>primfil; )
  {
    a--;
    for (b=0; b<ANCHO_RUNG; b++)
    {
      if (tabla_final[a][b].estado!=0)
      {
        u=b*64;
        v=(a-primfil)*32;
//        caja3d(u,v,64,32,colorFondo,colorFondo,colorFondo);
        operando=p->valor(tabla_final[a][b].memoria);
#ifdef DEBUG
//        setcolor(LIGHTGRAY);
//        rectangle(u,v,u+64,v+32);
//        caja3d(u,v,64,32,colorLuz,colorSombra,colorFondo);
#endif //DEBUG
        setcolor(LIGHTGREEN);
        if ((tabla_final[a][b].estado & Mask3b)==NA || (tabla_final[a][b].estado & Mask3b)==NC)
        {
          linea(u+8,v+16,u+24,v+16);
          linea(u+40,v+16,u+56,v+16);
          linea(u+24,v+4,u+24,v+28);
          linea(u+40,v+4,u+40,v+28);
          if ((tabla_final[a][b].estado & Mask3b)==NC)
            linea(u+28,v+24,u+36,v+8);
        } else
          if ((tabla_final[a][b].estado & Mask3b)==Output)  //outputs
          {
            linea(u+8,v+16,u+16,v+16);
            linea(u+48,v+16,u+56,v+16);
            linea(u+16,v+16,u+24,v+8);
            linea(u+16,v+16,u+24,v+24);
            linea(u+48,v+16,u+40,v+8);
            linea(u+48,v+16,u+40,v+24);
            if ((tabla_final[a][b].estado & Mask3b)==OutputNot) linea(u+28,v+24,u+36,v+8);
          } else
            if ((tabla_final[a][b].estado & Mask3b)==TMR)  //timers
            {
              rectangle(u+8,v+2,u+62,v+30);
            }
  //    }
        setcolor(LIGHTMAGENTA);
  //      setcolor(LIGHTGREEN);
        if (tabla_final[a][b].estado & HAbajo) // HACIA ABAJO
        {
          aux=v;
          bux=a;
          do
          {
            linea(u+8,aux+16,u+8,aux+48);
            aux+=32;
            bux+=1;
          } while(tabla_final[bux][b].estado==0);
  // transmitir hacia abajo---------------------
  //        if (tabla_final[a][b].estado & (DArr|DIzq)) tabla_final[bux][b].estado|=DArr;
  // -------------------------------------------
#ifdef DEBUG
          setcolor(YELLOW);
          linea(u+8,v+16,u+8,v+32);
          setcolor(LIGHTMAGENTA);
#endif //DEBUG
        }
        if (tabla_final[a][b].estado & HDer) // HACIA LA DERECHA
        {
          aux=u;
          bux=b;
  // transmitir hacia la derecha----------------
          if ((operando & ON) || (tabla_final[a][b].estado & DAba))
            transmitir=1;
            else transmitir=0;
  // -------------------------------------------
          if (transmitir) setcolor(LIGHTRED);
          do
          {
            if ((tabla_final[a][bux+1].estado==0) || tabla_final[a][bux+1].estado==HArriba)
              linea(aux+56,v+16,aux+120,v+16);
             else linea(aux+56,v+16,aux+72,v+16);
            aux+=64;
            bux+=1;
          } while(tabla_final[a][bux].estado==0);
          if (tabla_final[a][b].estado==(HAbajo|HDer) || tabla_final[a][b].estado==(HDer))
             linea(u+8,v+16,u+64,v+16);
          if (transmitir)
          {
            setcolor(LIGHTGREEN);
//            tabla_final[a][bux].estado|=DAba;
          }
#ifdef DEBUG
          setcolor(YELLOW);
          linea(u+56,v+16,u+64,v+16);
          setcolor(LIGHTMAGENTA);
#endif //DEBUG
        }

        if (tabla_final[a][b].estado & HArriba) // HACIA ARRIBA
        {
          aux=v;
          bux=a;
  // transmitir hacia arriba--------------------
          if ((operando & ON) || (tabla_final[a][b].estado & DAba))
            transmitir=1;
            else transmitir=0;
  // -------------------------------------------
          if (transmitir) setcolor(LIGHTRED);
          do
          {
            aux-=32;
            bux-=1;
            linea(u+56,aux+16,u+56,aux+48);
          } while(tabla_final[bux][b].estado==0);
          if (transmitir)
          {
            setcolor(LIGHTGREEN);
            tabla_final[bux][b].estado|=DAba;
          }
#ifdef DEBUG
          setcolor(YELLOW);
          linea(u+56,v+16,u+56,v);
          setcolor(LIGHTMAGENTA);
#endif //DEBUG
        }
      }
    } // end_for
  } // end_for
} // end_mostrar
