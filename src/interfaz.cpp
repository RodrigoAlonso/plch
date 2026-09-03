/* =================================================
   |                                               |
   |               Simulacion de PLC               |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//interfaz.cpp

#include <cstdio>
#include <stdlib.h>
#include "dos_shim.h"
#include <graphics.h>
#include <ventanas.h>
#include "plc.h"
#include "soporte.h"

//----------------------------------------------------------------------------
// Mostrar Interface Grafica
void plc::dibujar(int forzar)
{
	printf("plc::dibujar(), %d, %d, %d, %d\n", xplc, yplc, anchoplc,altoplc);
	
  mostrar_grafica(xplc,yplc,forzar);
  fflush(stdout);
  Botoner.dibujar();
  mostrar_uno(botres,1);

}

void plc::mostrar_uno(int nro, int recuadro)
{
   char s[40],bux;
   int aux,cux,dux,color1,color2,color3,color4,color5,hundido,bias=0,bias2=0;

   if (nro>=160)             // Outputs
   {
    aux=nro-160;
    bias=(altoInputs+1)*biasy*alto;
   } else aux=nro;
   cux=(aux%colxfila)*ancho*biasx+xplc;           // X de esq. sup. izq.
   dux=(aux/colxfila)*alto*biasy+yplc+bias;       // Y de esq. sup. izq.

   if (nro>=160)                // Outputs
   {
    if (IOsimul[nro].actual==0)
    {
      color1=colorSombra;
      color2=colorLuz;
      color3=colorFondo;
      color4=colorTexto;
      color5=color4;
    } else
      {
        color1=colorOut[aux].cluz;
        color2=colorSombra;
        color3=colorOut[aux].cfon;
        color4=colorOut[aux].ctexpres;
        color5=WHITE;
      }
    esconderMouse();
//    setcolor(colorSombra);
//    rectangle(cux+slackX,dux+slackY,
//              cux+slackX+ancho*biasx,dux+slackY+alto*biasy);
    caja3d(cux+slackX*3,
           dux+slackY*6+2,
           ancho*biasx-slackX*4,
           alto*biasy-slackY*6,
           color1,color2,color3);
    hundido=0;
    bias=alto*3+slackY-2;
    bias2=ancho;
    dux-=alto;
   } else                       // Inputs
     {
       if ((IOsimul[nro].actual&ON)!=ON)
       {
         color1=colorIn[nro].cluz;
         color2=colorSombra;
         color3=colorIn[nro].cfon;
         color4=colorIn[nro].ctexdepr;
         color5=color4;
         hundido=0;
       } else
         {
           color1=colorSombra;
           color2=colorIn[nro].cluz;
           color3=colorIn[nro].cfon;
           color4=colorIn[nro].ctexpres;
           color5=color4;
           hundido=1;
         }
       esconderMouse();
       caja3d(cux+slackX,dux+slackY,ancho*biasx,alto*biasy,
              color1,color2,color3);
       if (recuadro)
       {
         setcolor(colorResaltado);
         rectangle(cux+hundido+slackX+slackX/2,
                   dux+hundido+slackY+slackY/2,
                   cux+ancho*biasx,
                   dux+alto*biasy-hundido);
       }
       bias=0;
     }

   setcolor(color4);
   bux=IO[nro].nombre[5];
   IO[nro].nombre[5]='\0';
   outtextxy(cux+hundido+slackX+slackX,     // 1 linea de texto
             dux+alto*1+hundido+slackY*2,
             IO[nro].nombre);
   IO[nro].nombre[5]=bux;
   outtextxy(cux+hundido+slackX+slackX,     // 2 linea de texto
             dux+alto*2+hundido+slackY*2,
             &(IO[nro].nombre[5]));
   sprintf(s,"%02d",nro+(nro/16)*4);  // pasaje de representacion int. a ext.
   setcolor(color5);
   outtextxy(cux+hundido+slackX+slackX+bias2,    // numero del input/output
             dux+bias+hundido+slackY*2,s);
   mostrarMouse();
}

void plc::mostrar_grafica(int x, int y, int forzar)
{
  int aux;

  printf("this: %x, mostrar_grafica()\n", this);
  if (x!=xplc || y!=yplc || forzar)
  {
    xplc=x;
    yplc=y;
    esconderMouse();
    caja3d(xplc,yplc,anchoplc,altoplc,colorLuz,colorSombra,colorFondo);
    hendiduraCaja(xplc+slackX,yplc+altoInputs*biasy*alto+slackY+3,
                  anchoplc-slackX*2,biasy*alto-(slackY+2),
                  colorLuz,colorSombra);
    mostrarMouse();
  }
  for (aux=0; aux<nroInputs; aux++)
  {
    mostrar_uno(aux,0);
  }
  for (aux=0; aux<nroOutputs; aux++)
  {
    output[aux]=IOsimul[aux+160].actual;
    mostrar_uno(aux+160,0);
  }
}

void plc::actualizar_grafica(void)
{
  int aux;

  for (aux=0; aux<nroOutputs; aux++)
  {
    if (IOsimul[aux+160].actual != output[aux])
    {
      output[aux]=IOsimul[aux+160].actual;
      if (botSonido.apret) sound(200);
      mostrar_uno(aux+160,0);
    }
  }
  nosound();
}

// Devuelve un entero entre 0 y (nroInputs+nroOutputs-1)
int plc::calcular_destino(int xm, int ym)
{
   int aux,bux,cux,dux=8;

   aux=(xm-xplc-slackX);
   if (aux<=0) return -1;
   cux=aux/(ancho*biasx);
   if (cux>=colxfila) return -1; // columna, entre 0 y colxfila-1
   bux=(ym-yplc-slackY);
   if (bux<=0) return -1;
   dux=bux/(alto*biasy);
   if (dux==altoInputs || dux>altoInputs+altoOutputs) return -1;
   if (dux>altoInputs) dux--;    // fila, entre 0 y altoInputs+altoOutputs-1
   return dux*colxfila+cux;
}
