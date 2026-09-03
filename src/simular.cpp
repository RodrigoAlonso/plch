/* =================================================
   |                                               |
   |               Simulacion de PLC               |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//simular.cpp

#include <stdlib.h>
#include "plc.h"
#include "soporte.h"

//----------------------------------------------------------------------------
// Simulacion del Programa
void plc::simular(void)
{
  int pila[20];
  unsigned char opcode,operando;
  unsigned char parcial=0,final=0,auxfinal=0,bux;
  unsigned char mc=ON;  //master control.  NOTA: permite hasta 3 (tres)
                        //                       M.C. anidados.
  int pilamc[3];        //pila de Master Controls.
  int aux,cabeza=0,cabezamc=0;    //,debug;
  unsigned dir_operando;
  unsigned long t,buxil;
  unsigned char auxil;
/*
 NOTA:
 ====
   Recorre y ejecuta todas las instrucciones. Las evaluaciones se realizan
   usando el campo 'anterior' de IOsimul. Los resultados se almacenan en el
   campo 'actual', y al final (cuando se ejecuto todo el programa, lo que se
   conoce como un 'scan') se copia el campo 'actual' en el campo 'anterior'.

 NOTA (14/03/97):
 ===============
   Lo anterior dejo de ser valido:
    - El unico campo disponible es el 'actual'.
    - No se efectua ninguna copia al final de un ciclo de scan.
    - La actualizacion se realiza inmediatamente despues de la evaluacion.
   Esto esta de acuerdo a la forma de ejecucion de un PLC real.
   Lo que se buscaba con el modelo de ejecucion anterior era mantener el
   estado interno del PLC 'congelado' durante el ciclo de scan.
   Entre los beneficios de este enfoque se cuentan:
    - El resultado de la evaluacion de una cadena de contactos es
      independiente de su ubicacion en el programa.
    - La simulacion es mas cercana a la 'realidad'.
   Los PLCs  evaluan una cadena de contactos y al final (con la instruccion
   OUT, o con alguna funcion), guardan su resultado en algun rele interno.
   El nuevo valor del rele esta inmediatamente disponible, y la proxima vez
   que se use ese contacto en alguna cadena, se usara el nuevo valor.
*/
  especiales(); // reles especiales
  for (aux=0; aux<nro_instruc; aux++)
  {
/*    debugg=!debugg;*/
    opcode=instruc[aux].opcode;
    dir_operando=instruc[aux].operando;
    if (instruc[aux].tipo & OP_RELE || (instruc[aux].tipo & FUNCION))
      operando=IOsimul[dir_operando].actual;
     else operando=TCsimul[dir_operando].actual;
    if (instruc[aux].tipo & INSTRUCCION)
    {
     switch (opcode)
     {
       case ORG  : cabeza=0;      // flush de la pila
       case STR_AND:
       case STR_OR :
                   push(pila,&cabeza,20,operando);
                   break;
       case ORG_NOT: cabeza=0;      // flush de la pila
       case STR_NOT_AND:
       case STR_NOT_OR:
                   push(pila,&cabeza,20,((!operando)?ON:0));
                   break;
       case AND  : parcial=pop(pila,&cabeza) & operando;
                   push(pila,&cabeza,20,parcial);
                   break;
       case AND_NOT:
                   parcial=pop(pila,&cabeza) & ((!operando)?ON:0);
                   push(pila,&cabeza,20,parcial);
                   break;
       case OR   : parcial=pop(pila,&cabeza) | operando;
                   push(pila,&cabeza,20,parcial);
                   break;
       case OR_NOT:
                   parcial=pop(pila,&cabeza) | ((!operando)?ON:0);
                   push(pila,&cabeza,20,parcial);
                   break;
/*
 NOTA: Tuve que agregar un paso intermedio en el calculo del parcial que
      incluya la instruccion AND STR y OR STR porque el compilador
      'Borland C' compila distinto lo siguiente:

      " parcial=pop(pila,&cabeza) && pop(pila,&cabeza); "
      da un resultado incorrecto (llama una sola vez a la funcion 'pop')
      Supongo que el compilador debe interpretar que son la misma expresion
      y para optimizar(!!!!!!!!!) el codigo elimina una evaluacion, pero
      aunque la expresion 'luce' igual, entre llamadas sucesivas cambiaron
      los datos.

      " bux=pop(pila,&cabeza);
        parcial=pop(pila,&cabeza) && bux; "
      da el resultado correcto.
*/
       case AND_STR:
                   bux=pop(pila,&cabeza);
                   parcial=pop(pila,&cabeza) & bux;
                   push(pila,&cabeza,20,parcial);
                   break;
       case OR_STR:
                   bux=pop(pila,&cabeza);
                   parcial=pop(pila,&cabeza) | bux;
                   push(pila,&cabeza,20,parcial);
                   break;
       case OUT  : auxfinal=popush(pila,&cabeza);
                   break;
       case OUT_NOT:
                   auxfinal=(!popush(pila,&cabeza)?ON:0);
                   break;
     }
    }
    else {        // Funciones
           final=mc & popush(pila,&cabeza); //uso 'popush' por si sigue un 'out'
           if (opcode==4)     // master contol set
           {
             if (push(pilamc,&cabezamc,3,mc) == -1)
               error("ERROR: Anidamiento de Master Control superior a 3",0);
             mc=final;
           } else
           if (opcode==5)     // master control reset
             mc=pop(pilamc,&cabezamc);
           else
/*
 Las funciones de Edges (rising y trailing) usan el campo 'tipo' del
 arreglo de instrucciones para almacenar la el valor actual y el anterior
 y asi determinar cuando se produce un Edge.
 Ver en la funcion 'Cargar_programa' los comentarios al respecto.
*/
           if (opcode==0)   // Flanco Ascendente (Rising Edge)
           {
             if (instruc[aux].tipo & CAIDA)     // 1 scan despues se cae
             {
               IOsimul[dir_operando].actual=0;
               instruc[aux].tipo&=~CAIDA;
             } else
               if (!final) instruc[aux].tipo&=~TRABA;  // le saco la traba
               else
                 if (final && !(instruc[aux].tipo & TRABA))
                 {
                   instruc[aux].tipo|=TRABA|CAIDA;
                   IOsimul[dir_operando].actual=ON;
                 }
           } else
           if (opcode==1)   // Flanco Descendente (Trailing Edge)
           {
             if (instruc[aux].tipo & CAIDA)     // 1 scan despues se cae
             {
               IOsimul[dir_operando].actual=0;
               instruc[aux].tipo&=~CAIDA;
             } else
               if (final) instruc[aux].tipo&=~TRABA;  // le saco la traba
               else
                 if (!final && !(instruc[aux].tipo & TRABA))
                 {
                   instruc[aux].tipo|=TRABA|CAIDA;
                   IOsimul[dir_operando].actual=ON;
                 }
           } else
// Dentro del 'if (final)' que sigue estan las funciones cuya evaluacion esta
// sujeta a que la evaluacion de los contactos anteriores haya sido positiva.
           if (final)
           {
/*
 Nota acerca de las instrucciones de carga y almacenamiento:
 ==========================================================
  Los bytes se cargan y se recuperan de acuerdo al orden de la arquitectura
  80X86, esto es, el byte de menor orden en la posicion menor y el de mayor
  orden en la posision mayor (Backword). De esta manera pueden ser usados
  como argumentos de las funcones aritmeticas.

NOTA IMPORTANTE: AR NO puede usarse directamente en operaciones aritmeticas,
                 ya que el orden de los bytes de la palabra es incorrecto:
                 debe convertirse en una BACKWORD (el MSByte en la posicion
                 mas alta de memoria) antes de usarse.(Para convertirla hay
                 que intercambiar los bytes)
NOTA IMPORTANTE: (10/05/97) Ahora AR SI puede usarse en oper. aritmeticas,
                 porque sus bytes tienen el orden correcto.

NOTA: el operando (por ahora) NO puede ser un Timer. Cambiarlo an el futuro
*/
             if (opcode==10)   // load 2 bytes -> AR
             {
/* Este codigo es comun a varias funciones: todas las que cargan 2 bytes
   y luego realizan una operacion (suma, resta, OR, AND, etc). */
               //unsigned ARaux;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
               /*asm {
                     mov   ah,operando
                     mov   al,cuxil
                     mov   ARaux,ax
                   }
               AR=ARaux;*/
               AR = (static_cast<unsigned short>(operando) << 8) | cuxil;
             } else
             if (opcode==11)   // add: AR + bcd -> AR
             {
               unsigned ARaux;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
               /*asm {
                     mov   ah,operando
                     mov   al,cuxil
                     mov   ARaux,ax
                   }*/
               ARaux = (static_cast<unsigned short>(operando) << 8) | cuxil;
               if (sumarBCD(&AR,ARaux)==ERROR) CR=ON;
                 else CR=0;
             } else
             if (opcode==12)   // sub: AR - bcd -> AR
             {
               unsigned ARaux;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
               /*asm {
                     mov   ah,operando
                     mov   al,cuxil
                     mov   ARaux,ax
                   }*/
               ARaux = (static_cast<unsigned short>(operando) << 8) | cuxil;
               if (restarBCD(&AR,ARaux)==ERROR) CR=ON;
                 else CR=0;
             } else
             if (opcode==15)   // and: AR & 2 bytes -> AR
             {
               unsigned ARaux=AR;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
               /*asm {
                     mov   ah,operando
                     mov   al,cuxil
                     and   ARaux,ax   // AND
                   }
               AR=ARaux;*/
               AR = (static_cast<unsigned short>(operando) << 8) | cuxil;

             } else
             if (opcode==16)   // or: AR & 2 bytes -> AR
             {
               unsigned ARaux=AR;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
               /*asm {
                     mov   ah,operando
                     mov   al,cuxil
                     or    ARaux,ax   // OR
                   }
               AR=ARaux;*/
               AR = (static_cast<unsigned short>(operando) << 8) | cuxil;
             } else
             if (opcode==17)   // AR >= 2 bytes -> CR=1
             {
               unsigned ARaux=AR,CRaux=CR=0;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
               /*asm {
                     mov   ah,operando
                     mov   al,cuxil
                     cmp   ARaux,ax
                     jb   alli3
                     mov   CRaux,ON
                   }
alli3:*/
               unsigned short combined_ax = (static_cast<unsigned short>(operando) << 8) | cuxil;

               if (ARaux >= combined_ax) {
                   CRaux = ON;
               }

               CR = CRaux;
             } else
             if (opcode==18)   // AR = 2 bytes -> CR=1
             {
               unsigned ARaux=AR,CRaux=CR=0;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
               /*asm {
                     mov   ah,operando
                     mov   al,cuxil
                     cmp   ARaux,ax
                     jne    alli4
                     mov   CRaux,ON
                   }
alli4:*/
               unsigned short combined_ax = (static_cast<unsigned short>(operando) << 8) | cuxil;

               if (ARaux == combined_ax) {
                   CRaux = ON;
               }

               CR = CRaux;
             } else
             if (opcode==19)   // AR < 2 bytes -> CR=1
             {
               unsigned ARaux=AR,CRaux=CR=0;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
/*               asm {
                     mov   ah,operando
                     mov   al,cuxil
                     cmp   ARaux,ax
                     jae   alli5
                     mov   CRaux,ON
                   }
alli5:*/
               unsigned short combined_ax = (static_cast<unsigned short>(operando) << 8) | cuxil;

               if (ARaux < combined_ax) {
                   CRaux = ON;
               }

               CR = CRaux;
             } else
/* NOTA: se considera 'ON' aquel rele cuyo MSB bit
         (el de mas a la izquierda) es uno.
*/
             if (opcode==20)   // load 16 reles internos -> AR
             {
               for (AR=0, auxil=0, buxil=1; auxil<16; auxil++, buxil<<=1)
               {
                 if (IOsimul[dir_operando+auxil].actual) AR|=buxil;
               }
             } else
             if (opcode==21)   // out AR -> 2 bytes
             {
               IOsimul[dir_operando].actual=AR>>8;
               IOsimul[dir_operando+1].actual=AR & 0x00FF;
             } else
             if (opcode==22)   // out AR -> 16 reles internos
             {
               for (auxil=0, buxil=1; auxil<16; auxil++, buxil<<=1)
               {
                 if (AR & buxil)
                   IOsimul[dir_operando+auxil].actual=ON;
                 else
                   IOsimul[dir_operando+auxil].actual=0;
               }
             } else
             if (opcode==23)   // Output Carry:   CR -> I/O
             {
               IOsimul[dir_operando].actual=CR&0x00ff;
             } else
             if (opcode==24)   // AR(bin) ---> AR(bcd)
             {
               if (BIN_a_BCD(AR,&AR)==ERROR) CR=ON;
                 else CR=0;
             } else
             if (opcode==25)   // AR(bcd) ---> AR(bin)
             {
               if (BCD_a_BIN(&AR,AR)==ERROR) CR=ON;
                 else CR=0;
             } else
             if (opcode==26)   // Left Shift Register
             {
               if (AR & 0x8000) CR=ON;
                 else CR=0;
               AR<<=1;
             } else
             if (opcode==27)   // Right Shift Register
             {
               if (AR & 0x0001) CR=ON;
                 else CR=0;
               AR>>=1;
             } else
             if (opcode==50)   // load byte -> AR(low)
             {
               AR&=0xff00;
               AR|=operando;
             } else
             if (opcode==61)   // add AR + 2 bytes -> AR
             {
               unsigned ARaux=AR,CRaux=CR=0;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
/*               asm {
                     mov   ah,operando
                     mov   al,cuxil
                     add   ARaux,ax
                     jnc   alli
                     mov   CRaux,ON
                   }
alli:*/
               unsigned short combined_ax = (static_cast<unsigned short>(operando) << 8) | cuxil;

               unsigned int temp_sum = static_cast<unsigned int>(ARaux) + combined_ax;
               ARaux = static_cast<unsigned short>(temp_sum);

               if (temp_sum > 0xFFFF) {
                   CRaux = ON;
               }

               AR = ARaux;
               CR = CRaux;
             } else
             if (opcode==62)   // sub AR - 2 bytes -> AR
             {
               unsigned ARaux=AR,CRaux=CR=0;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
/*               asm {
                     mov   ah,operando
                     mov   al,cuxil
                     sub   ARaux,ax
                     jnc   alli2
                     mov   CRaux,ON
                   }
alli2:*/
               unsigned short combined_ax = (static_cast<unsigned short>(operando) << 8) | cuxil;

               if (ARaux < combined_ax) {
                   CRaux = ON;
               }

               ARaux -= combined_ax;

               AR = ARaux;
               CR = CRaux;
             } else
             if (opcode==63)   // mul AR * 2 bytes -> AR
             {
               unsigned ARaux=AR,CRaux=CR=0,ERaux;
               unsigned char cuxil=IOsimul[dir_operando+1].actual;
/*               asm {
                     mov   ah,operando
                     mov   al,cuxil
                     mul   ARaux
                     mov   ARaux,ax
                     jnc   alli6
                     mov   CRaux,ON
                     mov   ERaux,dx
                   }
               ER=ERaux;
alli6:*/
               unsigned short combined_ax = (static_cast<unsigned short>(operando) << 8) | cuxil;

               unsigned int result = static_cast<unsigned int>(combined_ax) * ARaux;

               ARaux = static_cast<unsigned short>(result & 0xFFFF);

               if (result > 0xFFFF) {
                   CRaux = ON;
                   ERaux = static_cast<unsigned short>((result >> 16) & 0xFFFF);
                   ER = ERaux;
               }

               AR=ARaux;
               CR=CRaux;
             } else
             if (opcode==80)   // Swap
             {
               unsigned auxiliar=AR>>8;
               AR=(AR<<8)|auxiliar;
             } else
             if (opcode==82)   // Exchange
             {
               unsigned auxiliar=AR;
               AR=ER;
               ER=auxiliar;
             } else
             if (opcode==85)   // Not  VER SI EN UN PLC FUNCIONA IGUAL
                               // HACIENDO UN DUMP DE AR.
             {
               AR=AR^ON;
             } else
// de 110 a 119 son las funciones con punto (.) de 0. a 9.
             if (opcode==110)   // 0. : Load constante BCD en AR
             {
               AR=dir_operando;
             } else
             if (opcode==111)   // 1. : Add constante BCD a AR
             {
               if (sumarBCD(&AR,dir_operando)==ERROR) CR=ON;
                  else CR=0;
             } else
             if (opcode==112)   // 2. : Sub constante BCD a AR
             {
               if (restarBCD(&AR,dir_operando)==ERROR) CR=ON;
                  else CR=0;
             } else
             if (opcode==115)   // 5. : AR & constante bcd -> AR
             {
               AR&=dir_operando;
             } else
             if (opcode==116)   // 6. : AR | constante bcd -> AR
             {
               AR|=dir_operando;
             } else
             if (opcode==117)   // 7. : AR >= constante bcd -> CR=1
             {
               CR=(AR>=dir_operando)? ON : 0;
             } else
             if (opcode==118)   // 8. : AR = constante bcd -> CR=1
             {
               CR=(AR==dir_operando)? ON : 0;
             } else
             if (opcode==119)   // 9. : AR < constante bcd -> CR=1
             {
               CR=(AR<dir_operando)? ON : 0;
             }
           }
         }
// Calculo de salidas. (OUT y OUT NOT del programa}
    if (opcode==0x08 || opcode==0x48)
    {
      final=mc & auxfinal;
      if (instruc[aux].tipo & OP_RELE) IOsimul[dir_operando].actual=final;
        else
        if (TC[dir_operando].tipo!='C')  // TIMERS
        {
         if (final)
         {
          t=tiempo();
          if (TCsimul[dir_operando].inicio == 0)
             TCsimul[dir_operando].inicio=t;
          else if ((t-TCsimul[dir_operando].inicio)>TC[dir_operando].valor)
               {
                 TCsimul[dir_operando].actual=final;
               }
         } else
           {
             TCsimul[dir_operando].actual=final;
             TCsimul[dir_operando].inicio=0;
           }
        } else   // COUNTERS
          {
            pop(pila,&cabeza);                 // reset
            auxfinal=mc && pop(pila,&cabeza);  // incremento
            if (final)     //linea de Reset del contador
            {
              TCsimul[dir_operando].actual=0;
              TCsimul[dir_operando].inicio=0;
              if (auxfinal) TCsimul[dir_operando].counter=1;
                 else TCsimul[dir_operando].counter=0;
            } else
              if (auxfinal)  //linea de Incremento del contador
              {
               if (TCsimul[dir_operando].counter==0)
               {
                 TCsimul[dir_operando].counter=1;
                 if (TCsimul[dir_operando].inicio<TC[dir_operando].valor)
                    TCsimul[dir_operando].inicio++;
                 if (TCsimul[dir_operando].inicio==TC[dir_operando].valor)
                    TCsimul[dir_operando].actual=ON;
               }
              } else TCsimul[dir_operando].counter=0;
            push(pila,&cabeza,20,final);
          }
    } // end_if
  } // end_for
  IOsimul[839].actual=0; // 967: prendido durante el primer scan,
                         //      luego apagado.
}

void plc::simularcabext(void) // simula el cableado externo
{
  int operando,signo,aux;
  ulong t=tiempo();

  for (aux=0; aux<numtemporizadores; aux++)
  {
    operando=abs(cabext[aux].output);
    signo=(cabext[aux].output>0) ? ON : 0;
    if (IOsimul[operando].actual==signo)
    {
     if (cabext[aux].inicio == -1)
     {
       cabext[aux].inicio=t;
     }
     else if (cabext[aux].inicio != -2)
            if (t-cabext[aux].inicio > cabext[aux].tiempo)
            {
              operando=abs(cabext[aux].input);
              signo=(cabext[aux].input>0) ? ON : 0;
              IOsimul[operando].actual=signo;
              cabext[aux].inicio=-2;
              mostrar_uno(operando,0);
            }
    } else cabext[aux].inicio=-1;
  }
}
