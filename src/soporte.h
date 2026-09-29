/* =================================================
   |                                               |
   |               Simulacion de PLC               |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//soporte.h

#ifndef _SOPORTE_H__
#define _SOPORTE_H__

#define ERROR -1
#define OK 1

#define ON 0x0080

#define ancho 8      // de un caracter en pixels
#define alto 9       // de un caracter en pixels
#define biasy 4      // filas (de texto) x boton (o led)
#define biasx 6      // columnas de texto x boton (o led)
#define colxfila 12  // botones ( o leds) x fila
#define slackX 4     // pixels del borde horizontal
#define slackY 4     // pixels del borde vertical

#include <stdio.h>

// (La simulacion, con su aritmetica BCD y el manejo del tiempo, esta en la
//  biblioteca plcsim.)
int push(int *pila, int *cabeza, int tope, int valor);
int pop(int *pila, int *cabeza);
unsigned long chk(char *codigo, char *copyright);

#endif // _SOPORTE_H__
