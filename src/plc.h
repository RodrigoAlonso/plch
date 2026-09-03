/* =================================================
   |                                               |
   |               Simulacion de PLC               |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */
//plc.h

#ifndef _PLC_H__
#define _PLC_H__

#include <stdio.h>
#include <ctype.h>
#include <ventanas.h>

typedef unsigned long ulong;

#define SALIR 27
//Opcodes
#define ORG       0x01
#define	ORG_NOT   0x41
#define STR       0x02
#define STR_NOT   0x42
#define AND       0x04
#define AND_NOT   0x44
#define OR        0x20
#define OR_NOT    0x60
#define AND_STR   0x06
#define OR_STR    0x22
#define OUT       0x08
#define OUT_NOT   0x48
//Agregados para otimizacion
#define STR_AND      0xFF
#define STR_OR       0xFE
#define STR_NOT_AND  0xFD
#define STR_NOT_OR   0xFC

extern char copyright[];
extern char msg1[];
extern char msg2[];
extern char codigo[];

extern cajadialogo cd_copyright;

class Link;  // declarado aqui porque se usa en la clase 'plc'
class visor;

//----------------------------------------------------------------------------
// Clase PLC
// NOTA: la cantidad(y tipo) de Instrucciones, de Inputs y Outputs y de Timers
//       esta fijada(hard coded) en el fuente del programa. Si se requiere
//       generalidad estos datos deberian leerse de un archivo.
// Se incluyen las interpretaciones que se le dan a las estructura de datos.
// (En el header PLC.H)

// Instrucciones
#define PASOS 1950               // hard coded para los PLC Hitachi serie EC.

// Inputs & Outputs
#define MEMORIA 864              // hard coded para los PLC Hitachi serie EC.

// Timers & Counters
#define TIMERS 96                // hard coded para los PLC Hitachi serie EC.

// Cableado externo: maximo 100 temporizadores y 10 selectoras.
#define Maxtemporizadores 50
#define Maxselectoras 10
#define Posselectoras 7 // Numero maximo+1 de posiciones de cada selectora

class plc
{
  private:

//--------------
// Instrucciones
// (Si llega a faltar espacio para codoficar estado en este byte, se pueden
//  agrupar los idicadores complementarios : INSTRUCCION/FUNCION,
//  RELE/TIMER
#define INSTRUCCION 0x0001
#define FUNCION     0x0002
#define CAIDA       0x0004
#define TRABA       0x0008
#define OP_RELE     0x0010  //operando tipo rele
#define OP_TIMER    0x0020  //operando tipo timer
struct inst_tag{
         unsigned char tipo;     /*     Instruccion --------------|
                                                                  |
                                        Funcion --------------|   |
                        /                                     |   |
                       |       Rele --------------|           |   |
         ------------ <                           |           |   |
         |             |  Timer --------------|   |           |   |
         |              \                     |   |           |   |
         |                          |---|---|---|---|---|---|---|---|
         |                     MSB  |   |   | X | X | X | X | X | X |  LSB
         |                          |---|---|---|---|---|---|---|---|
         |                                            |   |
         |                              anterior ------   |
         |                                                |
         |                              actual ------------
         |                          Se almacena el valor actual y el anterior
         |                          solo si se trata de una funcion de EDGE.
         |
El tipo de operando (RELE/TIMER) no se aplica para funciones. Estas lo usan
como un campo de almacenamiento.(ver el codigo de cada funcion en particular)

                                 */
         unsigned char opcode;   // en el caso de una funcion indica el nro.
         unsigned operando;
       };

//-----------------
// Inputs & Outputs
struct IO_tag{
         char nombre[11];
//         char descrip[31];
       };
struct IO_tag2 {
         unsigned char actual;
//         unsigned char anterior;
       };

//------------------
// Timers & Counters
struct TC_tag{
         char nombre[11];
//         char descrip[31];
         unsigned char tipo;     // 'T'=timer (se consideran con 2 decimales)
                                 // 'C'=counter
         ulong valor;            // si es un Timer indica tiempo,
                                 // sino una cantidad
       };

struct TC_tag2 {
         unsigned char actual;
//         unsigned char anterior;
         ulong inicio;           // Tiempo en que se disparo el timer,
                                 // o cuenta para counters.
         unsigned char counter;  // Auxiliar para los contadores(unicamente).
       };

//-----------------
// Cableado externo
struct cabext_tag {
         int output;
         ulong tiempo;
         int input;
         long inicio;       // -1 => listo para ser disparado;
                            // -2 => ya fue disparado.
                            // se resetea (-1) cuando deja de estar el output.
       };

//-----------
// Selectoras
struct selectoras_tag {
         signed char nro;                     // El numero de selectora.
         signed char actual;                  // El boton actualmente
                                              // presionado.
         unsigned char posic[Posselectoras];  // La primera es la posicion
                                              // por omision
       };

    friend class visor;

    inst_tag instruc[PASOS];
    unsigned nro_instruc;
    IO_tag IO[MEMORIA];
    IO_tag2 IOsimul[MEMORIA]; // Para la simulacion
                              // 2 valores: 0 - ON
                              //            1 - OFF
    TC_tag TC[TIMERS];
    TC_tag2 TCsimul[TIMERS];  // Para la simulacion
                              // 2 valores: 0 - ON
                              //            1 - OFF
    cabext_tag cabext[Maxtemporizadores];
    int numtemporizadores;

    selectoras_tag selectoras[Maxselectoras];
    int numselectoras;

struct colores {
         unsigned char cluz;
         unsigned char cfon;
         unsigned char ctexpres;   // color de texto con boton presionado
         unsigned char ctexdepr;   // color de texto sin boton presionado
       };

#define MaxInput  72
#define MaxOutput 72
    colores colorOut[MaxOutput];
    colores colorIn[MaxInput];

    unsigned char inputBloq[MaxInput];    // inputBloq: botones bloqueados
                                          //            (no presionables)
    signed char inputSelec[MaxInput];   // inputselec: botones que forman parte
                                        //             de una seectora.

// Arreglo de strings que guardan las descripciones de los inputs y outputs.
    char *Ayuda[MaxInput+MaxOutput];

    int ayudaActual;

    unsigned char output[MaxOutput];
    unsigned long tick;
    int xplc,yplc;
    unsigned anchoplc,altoplc;
    unsigned nroInputs,nroOutputs;
    unsigned altoInputs,altoOutputs;

    unsigned AR,ER,CR;  // registros internos del PLC

    char archivoPRG[80],archivoCEX[80];

#define ver_plc      0
#define ver_programa 1
    int modo; // ver_plc o ver_programa

    int botres;             // el boton resaltado
    int precond;            // se usa para mover el plc
    int precond2;           // se usa para el apretado de los botones

    menu m;
    boton botMenu;
    boton botSonido;
    boton botProg;
    boton botCabext;
    display dis;
    display disAyuda;
    bolsa_de_items Botoner;
    Link *linkable;
    int conectado;

    char nprog[41];
    cajadialogo cd_carg_prog;

    friend Link;

    void iniciar(void);
    void reset(void);
    void resetSelectoras(void);
    void prenderSelectoras(void);
    void resetTemporizadores(void);
    void resetTotal(void);
    int procesar_archivo(FILE *archivo);
    int procesar_archivo2(FILE *archivo);
    void simularcabext(void);
    void especiales(void);
    void mostrar_uno(int nro, int recuadro);
    void mostrar_grafica(int x, int y, int forzar);
    void actualizar_grafica(void);
    int calcular_destino(int xm, int ym);
    void moverplc(void);
    void simular(void);
    int crear_rung(visor *rg, int *ninst, int *nfilas) const;
  public:
    plc(int x, int y, Link *l, unsigned nroI, unsigned nroO);
    ~plc();
    int cargar_programa(char *archivo);
    int cargar_cableado_externo(char *archivo);
    int evento(eventoM *ev);
    void dibujar(int forzar);
    void conectar(int sino) { conectado=sino; }
    void mostrar_programa(eventoM *ev);
};

//----------------------------------------------------------------------------
class Link
{
  private:
    container c;
  public:
    Link(void) { };
    void insertar(plc *p) { c.insertar(p); }
    void actualizarLink(void);
    void dibujar(void);
};

#endif // _PLC_H__
