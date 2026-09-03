/* =================================================
   |                                               |
   |               Simulacion de PLC               |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//plcmain.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//#include <conio.h>
#include "conio_shim.h"
//#include <dos.h>
#include "dos_shim.h"
#include "plc.h"
#include "soporte.h"

//----------------------------------------------------------------------------
// main
Link linkable;
plc plc1(4,5,&linkable,36,24);
plc plc2(4,245,&linkable,36,24);

int main(int argc, char *argv[])
{
   unsigned char salir=0;
   eventoM ev;
   int com1,com2,com3=0;
   menu menuPrinc;
   display *di;

// Chequeo de integridad
   //if (chk(codigo,copyright) != 113648) exit(EXIT_FAILURE);
   textcolor(LIGHTCYAN);
   printf("%s\n",copyright);
   delay(500);
   textcolor(LIGHTGRAY);

// Inicializacion
   Iniciar();

/*   textcolor(LIGHTGRAY);
   cprintf("\n");
   if (argc<2 || argc>5)
   {
     textcolor(LIGHTGRAY);
     printf("\n Uso:     PLC programa1 [cableado_externo1]  [ programa2 [cableado_externo2] ]");
     printf("\n                  |             |");
     printf("\n                  |          ---|");
     printf("\n                  |          |   ");
     printf("\n Ejemplo: PLC ilumin.epg extilum.sda");
     printf("\n \'cableado_externo\' es opcional, simula el cableado externo al PLC");
     printf("\n Pueden simularse hasta 2 programas con sus cableados externos.");
     printf("\n DEBE incluirse la extension de los archivos.\n (\'epg\' para los programas y \'sda\' para los cableados externos)\n");
     return 0;
   }*/

   if (argc>1 && argc<6)
   {
     mayusculas(argv[1]);
     plc1.cargar_programa(argv[1]);
     if (argc>2)
     {
      mayusculas(argv[2]);
      if (strcmp(argv[2]+strlen(argv[2])-3,"SDA")==0)
      {
        plc1.cargar_cableado_externo(argv[2]);
        mayusculas(argv[3]);
        if (argc>3) plc2.cargar_programa(argv[3]);
        if (argc>4) plc2.cargar_cableado_externo(argv[4]);
      } else
        {
          plc2.cargar_programa(argv[2]);
          if (argc>3) plc2.cargar_cableado_externo(argv[3]);
        }
     }
   }

   plc1.dibujar(1);
   plc2.dibujar(1);

   menuPrinc.insertar(new boton("Linkar       ",100));
   menuPrinc.insertar(new boton("Deslinkar    ",150));
   menuPrinc.insertar(new boton("Acerca de PLC",200));
   menuPrinc.insertar(new boton("Salir        ",SALIR));
   menuPrinc.insertar(new display("HOLA MUNDO    ",LongTexto));
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

   printf("Show copyright notice\n");
   if (chk(codigo,copyright) != 113648) exit(EXIT_FAILURE);
   if (chk(codigo,msg1) != 89917) exit(EXIT_FAILURE);
   if (chk(codigo,msg2) != 136571) exit(EXIT_FAILURE);
   cd_copyright.evento(&ev);
   refresh();

//   di=new display(" No disponible en la version de prueba ",LongTexto,2*8,2*8);
//   di->setcolores(colorSombra,colorLuz,colorFondo,1,colorTexto,1);
//   mensaje.insertar(di);
//   mensaje.insertar(new boton("  Ok  ",300,0,0,19*8,6*8));

   /* Main event cycle */
   printf("Entering main event cycle\n");
   do
   {
     linkable.actualizarLink();
     com1=plc1.evento(&ev);
     com2=plc2.evento(&ev);

     if (com1==-1 && com2==-1)
     {
       if (picmo!=&iconoNulo) cambiarIconoMouse(&iconoNulo);
       if (ev.pred)
       {
         com3=menuPrinc.evento(&ev);
         if (com3==100)
         {
           plc1.conectar(1);
           plc2.conectar(1);
           linkable.dibujar();
         } else
         if (com3==150)
         {
           plc1.conectar(0);
           plc2.conectar(0);
           linkable.dibujar();
         } else
         if (com3==200) cd_copyright.evento(&ev);
       }
     }
     if (com1==SALIR || com2==SALIR || com3==SALIR) salir=1;

	 refresh();
     scanEvento(&ev);
   } while (!salir);
   Finalizar();
   textcolor(LIGHTGRAY);
   clrscr();
   return 1;
}
