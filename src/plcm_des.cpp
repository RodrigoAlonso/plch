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

// Chequeo de integridad
   if (chk(codigo,copyright) != 113648) exit(EXIT_FAILURE);
   printf("\n");
   textcolor(LIGHTCYAN);
   cprintf("%s",copyright);
//   delay(500);

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

   if (chk(codigo,copyright) != 113648) exit(EXIT_FAILURE);
   if (chk(codigo,msg1) != 89917) exit(EXIT_FAILURE);
   if (chk(codigo,msg2) != 136571) exit(EXIT_FAILURE);

//   do
//   {
     eventoM ev;
     plc1.mostrar_programa(&ev);
//   } while (!salir);
   Finalizar();
   textcolor(LIGHTGRAY);
//   clrscr();
   return 1;
}