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

#include <sys/time.h>
#include <time.h>
#include <cstdint>


unsigned long tiempo(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    // Create a native 64-bit variable copy to satisfy the compiler
    time_t copy_sec = tv.tv_sec;
    struct tm *t = localtime(&copy_sec);
    
    return (t->tm_hour * 360000L) + (t->tm_min * 6000L) + (t->tm_sec * 100L) + (tv.tv_usec / 10000);
}


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

// Devuelve un elemento de la pila sin removerlo.
int popush(int *pila, int *cabeza)
{
  if (*cabeza > 0) return pila[(*cabeza)-1];
     else return -1;
}



// Helper to convert 4-digit packed BCD (e.g., 0x1234) to an integer (1234)
inline uint32_t bcd_to_int(uint16_t bcd) {
    return ((bcd >> 12) & 0xF) * 1000 +
           ((bcd >> 8)  & 0xF) * 100 +
           ((bcd >> 4)  & 0xF) * 10 +
           (bcd & 0xF);
}

// Helper to convert an integer (1234) back to 4-digit packed BCD (0x1234)
inline uint16_t int_to_bcd(uint32_t num) {
    return ((num / 1000) << 12) |
           (((num / 100) % 10) << 8) |
           (((num / 10) % 10) << 4) |
           (num % 10);
}

int sumarBCD(unsigned int *bcd1, unsigned int bcd2) {
    // 4-digit BCD maximum value is 9999
    uint32_t val1 = bcd_to_int(*bcd1 & 0xFFFF);
    uint32_t val2 = bcd_to_int(bcd2 & 0xFFFF);
    uint32_t suma = val1 + val2;

    // DOS 'jc error' triggered if the sum exceeds 4 BCD digits (9999)
    if (suma > 9999) {
        return ERROR;
    }

    *bcd1 = int_to_bcd(suma);
    return OK;
}




int restarBCD(unsigned int *bcd1, unsigned int bcd2) {
    uint32_t val1 = bcd_to_int(*bcd1 & 0xFFFF);
    uint32_t val2 = bcd_to_int(bcd2 & 0xFFFF);

    // DOS 'jc error' from 'sbb' triggers if bcd2 is greater than bcd1 (underflow)
    if (val2 > val1) {
        return ERROR;
    }

    uint32_t resta = val1 - val2;
    *bcd1 = int_to_bcd(resta);
    return OK;
}

//----------------------------------------------------------------------------
// Funciones generales
int obtener_linea(char *s, int longitud, FILE *archivo)
{
  int aux=0,car;

  car=fgetc(archivo);
  while (aux<longitud-1 && car!='\n' && car!=EOF)
  {
    s[aux++]=car;
    car=fgetc(archivo);
  }
  s[aux]='\0';
  return 1;
}

int cortar(char *strorigen, int pos, int num, char *strdest)
{
  int aux;

  for (aux=0; aux<num; aux++) strdest[aux]=strorigen[pos+aux];
  strdest[aux]='\0';
  aux=0;
  do
  {
    strorigen[pos+aux]=strorigen[pos+num+aux];
  } while (strorigen[pos+num+aux++] != '\0');
  return 1;
}

// Convierte un binario SIN signo en un BCD de 4 digitos.
int BIN_a_BCD(unsigned int numBIN, unsigned int *numBCD)
{
  unsigned n=0,resto;

  if (numBIN<10000)
  {
    *numBCD=0;
    while (numBIN>=10)
    {
      resto=numBIN%10;
      resto<<=(n*4);
      numBIN=numBIN/10;
      *numBCD|=resto;
      n++;
    }
    *numBCD|=(numBIN<<(n*4));
  } else return ERROR;
  return OK;
}

// Convierte un BCD de 4 digitos en un binario SIN signo.
int BCD_a_BIN(unsigned int *numBIN, unsigned int numBCD)
{
  unsigned int n,aux,mascara=0x000f,resul=0,potencia=1;

  for (n=0; n<4; n++)
  {
    aux=(numBCD&mascara)>>(n*4);
    if (aux>9) return ERROR;
    resul+=aux*potencia;
    mascara<<=4;
    potencia*=10;
  }
  *numBIN=resul;
  return OK;
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
