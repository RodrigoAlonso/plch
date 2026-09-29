/* =================================================
   |                                               |
   |               Simulacion de PLC               |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//soporte.cpp

#include "soporte.h"
#include <string.h>

// Primitivas de pila. Logica:  Push: 1ro. guarda, 2do. avanza.
//                              Pop : 1ro. retrocede, 2do. saca.
int push(int *pila, int *cabeza, int tope, int valor)
{
  if (*cabeza < tope) return pila[(*cabeza)++]=valor;
     else return -1;
}

int pop(int *pila, int *cabeza)
{
  if (*cabeza > 0) return pila[--(*cabeza)];
     else return -1;
}

//----------------------------------------------------------------------------
// checksum
unsigned long chk(char *codigo, char *copyright)
{
  unsigned long int checksum=0,code=0,aux;

// Compilar con 'Unsigned characters' OFF
  for (aux=0; aux<strlen(codigo); aux++)
    code=code+((unsigned long)codigo[aux])*103;
  for (aux=0; aux<strlen(copyright); aux++)    // Checksum de integridad
    checksum+=code/((unsigned long)copyright[aux]);
  return checksum;
}
