/* =================================================
   |                                               |
   |               Simulacion de PLC               |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//plcm_big.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <dos.h>
#include "plc.h"
#include "soporte.h"

//----------------------------------------------------------------------------
// main
link linkable;
plc plc1(20,3,&linkable,72,72);

int main(int argc, char *argv[])
{
   unsigned char salir=0;
   eventoM ev;
   int com1,com3=0;
   menu menuPrinc;
   display *di;

// Chequeo de integridad
   if (chk(codigo,copyright) != 113648) exit(EXIT_FAILURE);
   printf("\n");
   textcolor(LIGHTCYAN);
   cprintf("%s",copyright);
   delay(500);

// Inicializacion
   Iniciar();

   if (argc>1 && argc<4)
   {
     mayusculas(argv[1]);
     plc1.cargar_programa(argv[1]);
     if (argc>2)
     {
      mayusculas(argv[2]);
      plc1.cargar_cableado_externo(argv[2]);
     }
   }

   plc1.dibujar(1);

   char hora[15];
   struct time hor;
   menuPrinc.insertar(new boton("Acerca de PLC",200));
   menuPrinc.insertar(new boton("Salir        ",SALIR));
   menuPrinc.insertar(new display(hora,14));
   reticula ret(20,24,5,5);
   ret.setcolores(colorLuz,colorSombra,colorFondo,LIGHTRED,colorSombra,RED);
   menuPrinc.insertar(&ret);
   menuPrinc.insertar(new boton("Boton Toggle ",300,1,0));
   menuPrinc.insertar(new barraDespHorizontal(108,1,10,3));
   char entrada[15];
   menuPrinc.insertar(new lineaInput(entrada,14));

   di=new display(copyright,LongTexto,50,5);
   di->setcolores(colorSombra,colorLuz,colorFondo,1,colorTexto,1);
   cd_copyright.insertar(di);
   di=new display(msg1,LongTexto,50,35);
   di->setcolores(colorSombra,colorLuz,colorFondo,1,colorTexto,1);
   cd_copyright.insertar(di);
   di=new display(msg2,LongTexto,50,65);
   di->setcolores(colorSombra,colorLuz,colorFondo,1,colorTexto,1);
   cd_copyright.insertar(di);
   cd_copyright.insertar(new boton("  Ok  ",300,0,0,50,100));

   if (chk(codigo,copyright) != 113648) exit(EXIT_FAILURE);
   if (chk(codigo,msg1) != 89917) exit(EXIT_FAILURE);
   if (chk(codigo,msg2) != 136571) exit(EXIT_FAILURE);
   cd_copyright.evento(&ev);

   do
   {
     com1=plc1.evento(&ev);

     if (com1==-1)
     {
       if (picmo!=&iconoNulo) cambiarIconoMouse(&iconoNulo);
       if (ev.pred)
       {
         gettime(&hor);
         sprintf(hora," Hora %2d:%2d",hor.ti_hour,hor.ti_min);
         com3=menuPrinc.evento(&ev);
         if (com3==200) cd_copyright.evento(&ev);
       }
     }
     if (com1==SALIR || com3==SALIR) salir=1;

     scanEvento(&ev);
   } while (!salir);
   Finalizar();
   textcolor(LIGHTGRAY);
   clrscr();
   return 1;
}