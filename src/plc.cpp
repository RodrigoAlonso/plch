/* ====================================================
   |                                                  |
   |                 Simulacion de PLC                |
   |                                                  |
   |     (c) Copyright 1996,97,98  Rodrigo Alonso     |
   |                                                  |
   ===================================================|  */
//plc.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "conio_shim.h"
#include <graphics.h>
#include <ventanas.h>
#include "plc.h"
#include "soporte.h"
#include <cerrno>

//----------------------------------------------------------------------------
char copyright[]="(c) Copyright 1996-1997, Rodrigo Alonso - Version 1.7";
char msg1[]="Funcionalidad de esta version(28/05/97): 75%";
char msg2[]="Propuestas, consultas, sugerencias: (01)572-5151, (01)756-7057.";
char codigo[]="n..{.t.P3F.&.h...q.7";

cajadialogo cd_copyright(48,134,110);

//----------------------------------------------------------------------------
// Implementacion de la clase Link
//----------------------------------------------------------------------------
void Link::actualizarLink(void)
{
  plcsim_t *sims[maxPuntero];
  unsigned aux,bux=c.getNumero();
  plc *p=(plc *)c.primero();

  for (aux=0; aux<bux; aux++)  // Por cada PLC...
  {
    sims[aux]=p->sim;
    if (aux+1<bux) p=(plc *)c.siguiente();
  }
  plcsim_link_update(sims,bux);
}

void Link::dibujar(void)  // NOTA: solo dibuja correctamente si hay 2 PLCs
{
  plc *p1=(plc *)c.primero();
  plc *p2=(plc *)c.siguiente();
  int c1=GREEN,c2=MAGENTA,c3=CYAN;

  if (p1!=NULL && p2!=NULL && p1!=p2)
  {
    if (!plcsim_is_linked(p1->sim) || !plcsim_is_linked(p2->sim))
    {
      c1=c2=c3=BLACK;
    }
    setlinestyle(SOLID_LINE,1,THICK_WIDTH);
    setcolor(c1);
    esconderMouse();
    line(p1->xplc+p1->anchoplc-1,p1->yplc+p1->altoplc/2-10,635,p1->yplc+p1->altoplc/2-10);
    line(p2->xplc+p2->anchoplc-1,p2->yplc+p2->altoplc/2+10,635,p2->yplc+p2->altoplc/2+10);
    line(635,p1->yplc+p1->altoplc/2-10,635,p2->yplc+p2->altoplc/2+10);
    setcolor(c2);
    line(p1->xplc+p1->anchoplc-1,p1->yplc+p1->altoplc/2,625,p1->yplc+p1->altoplc/2);
    line(p2->xplc+p2->anchoplc-1,p2->yplc+p2->altoplc/2,625,p2->yplc+p2->altoplc/2);
    line(625,p1->yplc+p1->altoplc/2,625,p2->yplc+p2->altoplc/2);
    setcolor(c3);
    line(p1->xplc+p1->anchoplc-1,p1->yplc+p1->altoplc/2+10,615,p1->yplc+p1->altoplc/2+10);
    line(p2->xplc+p2->anchoplc-1,p2->yplc+p2->altoplc/2-10,615,p2->yplc+p2->altoplc/2-10);
    line(615,p1->yplc+p1->altoplc/2+10,615,p2->yplc+p2->altoplc/2-10);
    setlinestyle(SOLID_LINE,1,NORM_WIDTH);
    mostrarMouse();
  }
}

//----------------------------------------------------------------------------
// Implementacion de la clase PLC
//----------------------------------------------------------------------------
char cadenaNula='\0';
plc::plc(int x, int y, Link *l, unsigned nroI, unsigned nroO)
{
  if (nroI>160 || nroO>160) error("ERROR: Demasiados Inputs/Outputs",1);
  nroInputs=nroI;
  nroOutputs=nroO;
  xplc=x;
  yplc=y;
  altoInputs=(nroInputs/colxfila+(nroInputs%colxfila?1:0));
  altoOutputs=(nroOutputs/colxfila+(nroOutputs%colxfila?1:0));
  altoplc=(altoInputs+altoOutputs+1)*alto*biasy+2*slackY;
  anchoplc=ancho*biasx*colxfila+slackX*2;
  botMenu.setear(" Menu ",40,0,0,xplc+10,yplc+altoInputs*biasy*alto+slackY*3);
  strcpy(archivoPRG,"SIN PROGRAMA");
  dis.setear(archivoPRG,29,xplc+80,yplc+altoInputs*biasy*alto+slackY*3);
  disAyuda.setear(&cadenaNula,30,xplc+326,yplc+altoInputs*biasy*alto+slackY*3);
  disAyuda.setcolores(colorSombra,colorLuz,colorFondo,colorFondo,BLUE,1);

  cd_carg_prog.setear(140,190,80);
  display *di=new display(" Nombre ",LongTexto,0,10);
  di->setcolores(colorSombra,colorLuz,colorFondo,1,colorTexto,1);
  cd_carg_prog.insertar(di);
  cd_carg_prog.insertar(new boton("   Ok   ",100,0,0,30,100));
  cd_carg_prog.insertar(new boton("Cancelar",200,0,0,75,100));
  nprog[0]='\0';
  cd_carg_prog.insertar(new lineaInput(nprog,40,50,50));

  if ((sim=plcsim_create())==NULL) error("ERROR: No se pudo reservar memoria",1);
  linkable=l;
  linkable->insertar(this);

  iniciar();
}

plc::~plc()
{
   plcsim_destroy(sim);
}

void plc::reset(void)
{
  int aux;

  plcsim_reset(sim);
  for (aux=0; aux<nroInputs; aux++) input[aux]=2;
  for (aux=0; aux<nroOutputs; aux++) output[aux]=2;
//
  botres=0;
  precond=0;
  precond2=0;
  ayudaActual=0;
  modo=ver_plc;
}

void plc::resetColores(void)  // parte del cableado externo
{
  int aux;

  for (aux=0; aux<nroOutputs; aux++)   // outputs, colores
  {
    colorOut[aux].cluz=LIGHTRED;
    colorOut[aux].cfon=RED;
    colorOut[aux].ctexpres=RED;
    colorOut[aux].ctexdepr=DARKGRAY;
  }
  for (aux=0; aux<nroInputs; aux++)   // inputs, colores
  {
    colorIn[aux].cluz=WHITE;
    colorIn[aux].cfon=LIGHTGRAY;
    colorIn[aux].ctexpres=WHITE;
    colorIn[aux].ctexdepr=BLACK;
    inputBloq[aux]=0;
  }
}

void plc::resetTotal(void)
{
  int aux;

  nro_instruc=0;
  for (aux=0; aux<PASOS; aux++)
  {
    instruc[aux].tipo=instruc[aux].opcode=0;
    instruc[aux].operando=0;
  }
  resetColores();
  plcsim_clear_wiring(sim);
  reset();
}

void plc::iniciar(void)
{
  resetTotal();
  Botoner.insertar(&botMenu);
  Botoner.insertar(&dis);
  Botoner.insertar(&disAyuda);

  botProg.setear("Programa  On/Off",800,1,1,0,0);
  botCabext.setear("Cableado  On/Off",800,1,1,0,0);
  botSonido.setear("Sonido          ",30,1,1,0,0);
  m.insertar(new boton("Cargar Programa ",100));
  m.insertar(new boton("Cargar Cableado ",200));
  m.insertar(&botProg);
  m.insertar(&botCabext);
  m.insertar(new boton("Reset Programa  ",10));
  m.insertar(&botSonido);
  m.insertar(new boton("Ver Programa    ",400));
  m.insertar(new boton("Acerca de PLC   ",300));
  m.insertar(new boton("Salir           ",27));
  archivoCEX[0]='\0';
}

//----------------------------------------------------------------------------
// Copiar Programa.  Copia el programa cargado en la simulacion para mostrarlo
// en formato ladder (ver mostprog.cpp).
void plc::copiar_programa(void)
{
  plcsim_step paso;
  int pila[20],cabeza=0;
  int direcc[20],cabezadir=0,dir,bux;
  unsigned nro;

  nro_instruc=plcsim_step_count(sim);
  for (nro=0; nro<nro_instruc; nro++)
  {
    plcsim_get_step(sim,nro,&paso);
    if (!paso.is_function)    // INSTRUCCION
    {
      instruc[nro].tipo=INSTRUCCION|(paso.timer ? OP_TIMER : OP_RELE);
      /*
        Modificado el 12/01/98.
        Se agregan 4 opcodes para optimizar la impresion de codigo en formato
        ladder.
        Mientras se copia el programa se realiza el reemplazo, que puede
        interpretarse como una traduccion de notacion postfija (notacion de pila),
        a notacion prefija.
        Ejemplos:

          STR                              STR AND
          .                                .
          .             ===========>       .
          .                                .
          AND STR                          AND STR

          ,

          STR NOT                          STR NOT OR
          .                                .
          .             ===========>       .
          .                                .
          OR STR                           OR STR
      */
      if (paso.opcode==STR || paso.opcode==STR_NOT)  // marcado de opcodes a reemplazar
      {
        push(pila,&cabeza,20,paso.opcode);
        push(direcc,&cabezadir,20,nro);
      } else
      if (paso.opcode==AND_STR || paso.opcode==OR_STR) // reemplazo de opcodes
      {
        bux=pop(pila,&cabeza);
        dir=pop(direcc,&cabezadir);
        if (dir>=0)
        {
          if (bux==STR)
          {
            if (paso.opcode==AND_STR) instruc[dir].opcode=STR_AND;
              else instruc[dir].opcode=STR_OR;
          } else //STR_NOT
            {
              if (paso.opcode==AND_STR) instruc[dir].opcode=STR_NOT_AND;
                else instruc[dir].opcode=STR_NOT_OR;
            }
        }
      }
      instruc[nro].opcode=paso.opcode;
    } else           // FUNCION
      {
        instruc[nro].tipo=FUNCION;
// las funciones "con punto" (0. a 9.) se mapean en los nros. del 110 al 119.
        instruc[nro].opcode=paso.opcode+(paso.dot ? 110 : 0);
      }
    instruc[nro].operando=paso.operand;
  }
}

int plc::cargar_programa(char *archivo)
{
   char mensaj[73];

   if (plcsim_load_program_file(sim,archivo)!=PLCSIM_OK)
   {
     printf("ERROR al cargar [%s]: %s\n",archivo,plcsim_last_error(sim));
     snprintf(mensaj,sizeof(mensaj),"ERROR: %s",plcsim_last_error(sim));
     error(mensaj,0);
     return ERROR;
   }
   resetTotal();
   copiar_programa();
   snprintf(archivoPRG,sizeof(archivoPRG),"%s",archivo);
   snprintf(nprog,sizeof(nprog),"%s",archivo);
   return OK;
}

//----------------------------------------------------------------------------
// Procesar Archivo2.  Lee el archivo asociado que simula el cableado externo
// al PLC.
int obtener_token(char *s, int longitud, FILE *archivo)
{
     int aux=0,res=0;
     int car=fgetc(archivo);

// se come los comentarios, newlines, blancos, Tabs y comas
     while (car=='/' || car=='\n' || car=='\r' || car==' ' || car=='\t' || car==',')
       if (car=='/')
       {
	 car=fgetc(archivo);
	 if (car=='/')
	 {
	   do car=fgetc(archivo); while (car!='\n' && car!=EOF);
//	   if (aux==0) car=fgetc(archivo);
	 } else
           if (car=='*')
           {
             do
             {
               do car=fgetc(archivo);
               while (car!='*' && car!=EOF);
               car=fgetc(archivo);
             } while (car!='/' && car!=EOF);
             car=fgetc(archivo);
           }
       } else car=fgetc(archivo);
// toma el token siguiente
     while (aux<longitud && car!='/' && car!='\n' && car!='\r' && car!=' '
	    && car!=EOF && car!='\t' && car!=',')
     {
       s[aux++]=car;
       car=fgetc(archivo);
       if (isalpha(car)) res=2;
     }
     if (car>=longitud) ungetc(car,archivo);
     s[aux]='\0';
     if (car==EOF) res=1;
     minusculas(s);  // convierte a minusculas
     return res;
}

// Solo se leen los colores y los inputs bloqueados, que son propios de la
// interfaz grafica. El resto del archivo lo interpreta la simulacion.
int plc::procesar_archivo2(FILE *archivo)
{
   char s[21];
   int aux,tok,c1,c2,c3,bloq;
   int resultado=OK;

   do tok=obtener_token(s,20,archivo);
   while (strcmp(s,"[colores]")!=0 && tok!=1);

// Colores
   if (strcmp(s,"[colores]")==0)
   {
     while ((tok=obtener_token(s,20,archivo))!=1)
     {
       sscanf(s,"%d",&aux);
       aux=(16*(aux/20)+(aux%20)); // pasaje de representacion ext. a int.
       obtener_token(s,20,archivo);
       if (strcmp(s,"verde")==0) { c1=GREEN; c2=WHITE; c3=BLACK; }
       else if (strcmp(s,"rojo")==0) { c1=RED; c2=WHITE; c3=LIGHTGRAY; }
       else if (strcmp(s,"amarillo")==0) { c1=6; c2=WHITE; c3=BLACK; }
       else if (strcmp(s,"azul")==0) { c1=BLUE; c2=WHITE; c3=LIGHTGRAY; }
       else if (strcmp(s,"celeste")==0) { c1=CYAN; c2=WHITE; c3=BLACK; }
       else if (strcmp(s,"violeta")==0) { c1=MAGENTA; c2=WHITE; c3=LIGHTGRAY; }
       else if (strcmp(s,"gris")==0) { c1=LIGHTGRAY; c2=WHITE; c3=BLACK; }
       else resultado=ERROR;
       if (aux>=160 && aux<160+nroOutputs)
       {
         colorOut[aux-160].cfon=c1;
         colorOut[aux-160].cluz=c1+8;
         if (c1==RED || c1==MAGENTA || c1==BLUE) c2=c1;
           else c2=c1+8;
         colorOut[aux-160].ctexpres=c2;
         colorOut[aux-160].ctexdepr=c3;
       } else
         if (aux>=0 && aux<nroInputs)
         {
           colorIn[aux].cfon=c1;
           colorIn[aux].cluz=c1+8;
           colorIn[aux].ctexpres=c2;
           colorIn[aux].ctexdepr=c3;
           obtener_token(s,20,archivo); // solo para los inputs
           sscanf(s,"%d",&bloq);
           inputBloq[aux]=bloq;
         } else
           if (aux>=0 && aux<160)
           {
             obtener_token(s,20,archivo); // flush
           }
     }
   }
   return resultado;
}

int plc::cargar_cableado_externo(char *archivo)
{
   FILE *ar;
   char mensaj[73];
   int resultado=ERROR;
   int linkado=plcsim_is_linked(sim);

   snprintf(archivoCEX,sizeof(archivoCEX),"%s",archivo);
   if (plcsim_load_wiring_file(sim,archivo)!=PLCSIM_OK)
   {
     printf("ERROR al cargar [%s]: %s\n",archivo,plcsim_last_error(sim));
     snprintf(mensaj,sizeof(mensaj),"ERROR: %s",plcsim_last_error(sim));
     error(mensaj,0);
     return resultado;
   }
   if (plcsim_is_linked(sim)!=linkado) linkable->dibujar();
   if ((ar = fopen(archivo,"rt")) == NULL)
   {
     snprintf(mensaj,sizeof(mensaj),"ERROR: No se puede abrir %s",archivo);
     error(mensaj,0);
     return resultado;
   }
   resetColores();
   resultado=procesar_archivo2(ar);
   fclose(ar);
   return resultado;
}

//----------------------------------------------------------------------------
// Simulacion. Un ciclo del programa y/o del cableado externo, segun los
// botones del menu.
void plc::simular(void)
{
  unsigned partes=0;

  if (botProg.apret) partes|=PLCSIM_RUN_PROGRAM;
  if (botCabext.apret) partes|=PLCSIM_RUN_WIRING;  // cableado externo
  if (partes && (plcsim_run(sim,partes) & PLCSIM_WARN_MC_NESTING))
    error("ERROR: Anidamiento de Master Control superior a 3",0);
}

unsigned char plc::valor(unsigned nro) const
{
  int v=plcsim_get_byte(sim,nro);
  return (v<0) ? 0 : v;
}

void plc::alternar(int nro)
{
  plcsim_set(sim,nro,valor(nro)==0);
}

//----------------------------------------------------------------------------
// Mover el PLC
void plc::moverplc(void)
{
  eventoM ev;
  int dx,dy,ax=0,ay=0,xold,yold;

  setwritemode(XOR_PUT);
  setcolor(YELLOW);
  esconderMouse();
  rectangle(xplc,yplc,xplc+anchoplc,yplc+altoplc);
  mostrarMouse();
  scanEvento(&ev);
  xold=ev.xm;
  yold=ev.ym;
  do
  {
    if (xold!=ev.xm || yold!=ev.ym)
    {
      dx=ev.xm-xold;
      dy=ev.ym-yold;
      esconderMouse();
      rectangle(xplc+ax,yplc+ay,xplc+ax+anchoplc,yplc+ay+altoplc);
      ax+=dx;
      ay+=dy;
      rectangle(xplc+ax,yplc+ay,xplc+ax+anchoplc,yplc+ay+altoplc);
      mostrarMouse();
    }
    xold=ev.xm;
    yold=ev.ym;
    scanEvento(&ev);
  } while (ev.bizq==1);
  xplc+=ax;
  yplc+=ay;
  Botoner.mover(ax,ay);
  esconderMouse();
  rectangle(xplc,yplc,xplc+anchoplc,yplc+altoplc);
  mostrarMouse();
  setwritemode(COPY_PUT);
}

int plc::evento(eventoM *ev)
{
   int comando,comando2,destino,comando3;

   simular();

   if (modo==ver_plc)
   {
     actualizar_grafica();

     destino=calcular_destino(ev->xm,ev->ym);
     if (destino!=-1)
     {
       if (destino!=ayudaActual)
       {
         unsigned nro=(destino<nroInputs) ? destino : destino-nroInputs+160;
         snprintf(ayuda,sizeof(ayuda),"%s",plcsim_io_comment(sim,nro));
         disAyuda.settexto(ayuda);
         disAyuda.dibujar();
         ayudaActual=destino;
       }
       if (destino>=nroInputs)
       {
         destino=-1;    // Con esto limito el procesamiento de eventos a
                        // los botones(inputs) unicamente.
       }
     }
     if (destino!=-1 && inputBloq[destino]==0)
     {
       if (!plcsim_is_selector(sim,destino))
       {
         if (picmo!=&iconoMano) cambiarIconoMouse(&iconoMano);
         if (((ev->bizq || ev->bder) && (destino!=botres || precond2==0)) || ev->prei || ev->pred)
         {
           if (ev->bder)
           {
             if (precond2==2) alternar(botres);
             precond2=2;
           } else precond2=1;
           mostrar_uno(botres,0);
           alternar(destino);
           mostrar_uno(destino,1);
           botres=destino;
         } else
         if (ev->sold && precond2!=0)
         {
           alternar(botres);
           mostrar_uno(botres,1);
           precond2=0;
         } else
         if (ev->soli && precond2!=0) precond2=0;
         else
         if (destino!=botres)
         {
           mostrar_uno(botres,0);
           mostrar_uno(destino,1);
           botres=destino;
         }
  /*       for (int aux=0; aux<nroInputs; aux++)
         if (IOsimul[aux].actual!=0)
         {
           IOsimul[aux].actual=ON;
         }*/
       } else
         {
           if (picmo!=&iconoSelec) cambiarIconoMouse(&iconoSelec);
// La simulacion apaga las otras posiciones de la selectora al prender una,
// y actualizar_grafica las redibuja.
           if (botres!=destino)
           {
             if (ev->bizq) plcsim_set(sim,destino,1);
             mostrar_uno(botres,0);
             mostrar_uno(destino,1);
           } else
             if (ev->prei)
             {
               alternar(destino);
               mostrar_uno(destino,1);
             }
           botres=destino;
         }
     } else
       {
         if (destino!=-1 && inputBloq[destino]!=0 && picmo!=&iconoProhibido) cambiarIconoMouse(&iconoProhibido);
         if (precond2 == 2)
         {
           alternar(botres);
           mostrar_uno(botres,1);
         }
         comando=Botoner.evento(ev);
         if (comando==40)
         {
           comando2=m.evento(ev);
           if (comando2==10)
           {
             reset();
             dibujar(0);
           } else
           if (comando2==27)
           {
             return SALIR;
           } else
           if (comando2==100)
           {
             comando3=cd_carg_prog.evento(ev);
//             mayusculas(nprog);
             if (comando3==100)
             if (cargar_programa(nprog)==OK)
             {
               mensaje(" Programa cargado OK ",LIGHTBLUE,BLUE,YELLOW);
               reset();
               dibujar(1);
             }
           } else
           if (comando2==200)
           {
             comando3=cd_carg_prog.evento(ev);
             mayusculas(nprog);
             if (comando3==100)
             if (cargar_cableado_externo(nprog)==OK)
             {
               mensaje(" Cableado Externo cargado OK ",LIGHTBLUE,BLUE,YELLOW);
               reset();
               dibujar(0);
             }
           } else
           if (comando2==300)
           {
             if (chk(codigo,copyright) != 113648) exit(EXIT_FAILURE);
             if (chk(codigo,msg1) != 89917) exit(EXIT_FAILURE);
             if (chk(codigo,msg2) != 136571) exit(EXIT_FAILURE);
             cd_copyright.evento(ev);
           } else
           if (comando2==400)
           {
             modo=ver_programa;
           } else
           if (comando2==500 || comando2==600)
           {
  //                    mensaje.evento(ev);
           }
         }
         precond2=0;
       }
  /*     bar3d(0,0,80,10,0,0);
     sprintf(st,"%lu",cont);
     setcolor(WHITE);
     outtextxy(1,1,st);*/     // muestra el numero de 'scan' del programa
     if (destino==-1 && comando==EV_AFUERA)
     {
      if (ev->xm>xplc && ev->xm <xplc+anchoplc && ev->ym>yplc && ev->ym<yplc+altoplc)
      {
        if (picmo!=&iconoFlecha) cambiarIconoMouse(&iconoFlecha);
  /*      if (ev->prei==1 && precond)
        {
          moverplc();
          link *lin=linkable;
          setlink(NULL);
          linkable->dibujar();
          esconderMouse();
          setfillstyle(SOLID_FILL,BLACK);
          setcolor(BLACK);
          bar3d(xplc,yplc,xplc+anchoplc,yplc+altoplc,0,0);
          mostrarMouse();
          setlink(lin);
          dibujar();
          linkable->dibujar();
          precond=0;
        } else if (ev->bizq==0) precond=1;
        precond=1; */
      } else
        {
  //        precond=0;
          return -1;
        }
     } //else precond=0;
   } else if (modo==ver_programa)
     {
       mostrar_programa(ev);
       modo=ver_plc;
       dibujar(1);
     }
   return 1;
}
