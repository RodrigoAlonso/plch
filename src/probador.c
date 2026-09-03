#include <stdio.h>
#include <string.h>
#include <conio.h>

int numstrings=3;
char *copyright[]={"(c) Copyright 1996-1997, Rodrigo Alonso - Version 1.7",
                  "Funcionalidad de esta version(28/05/97): 75%",
                  "Propuestas, consultas, sugerencias: (01)572-5151, (01)756-7057."};

char codigo[]="n..{.t.P3F.&.h...q.7";

void main()
{
 int aux,luxor;
 unsigned long int bux,code=0;

 clrscr();
 for (aux=0; aux<strlen(codigo); aux++)
// si lo multiplico por 1000 da resultados incorrectos.
     code=code+((unsigned long)codigo[aux])*103;
 printf("\n\nCODIGO: %lu\n=======\n\n",code);

 for (luxor=0; luxor<numstrings; luxor++)
 {
  // NOTA: dependiendo si se compila con la opcion
  //        'Unsigned characters'  ON u OFF produce distintos resultados
  //        (66656 y 66289 respectivamente).
   bux=0;
   for (aux=0; aux<strlen(copyright[luxor]); aux++)
       bux+=code/((unsigned long)copyright[luxor][aux]);
   printf("-----------------\n%s\n",copyright[luxor]);
   printf("%lu\n",bux);  //este es el codigo encriptado.
 }
 getch();
}