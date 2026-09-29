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
#include <plcsim.h>

typedef unsigned long ulong;

#define SALIR 27
//Opcodes
#define ORG       PLCSIM_OP_ORG
#define	ORG_NOT   PLCSIM_OP_ORG_NOT
#define STR       PLCSIM_OP_STR
#define STR_NOT   PLCSIM_OP_STR_NOT
#define AND       PLCSIM_OP_AND
#define AND_NOT   PLCSIM_OP_AND_NOT
#define OR        PLCSIM_OP_OR
#define OR_NOT    PLCSIM_OP_OR_NOT
#define AND_STR   PLCSIM_OP_AND_STR
#define OR_STR    PLCSIM_OP_OR_STR
#define OUT       PLCSIM_OP_OUT
#define OUT_NOT   PLCSIM_OP_OUT_NOT
//Agregados para otimizacion de la impresion en formato ladder
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
// Interfaz grafica de un PLC. La simulacion la realiza la biblioteca plcsim
// (ver libplcsim/include/plcsim.h); esta clase solo guarda lo necesario para
// mostrarla.

// Instrucciones
#define PASOS PLCSIM_MAX_STEPS   // hard coded para los PLC Hitachi serie EC.

class plc
{
  private:

//--------------
// Instrucciones (copia del programa cargado en la simulacion, para
// mostrarlo en formato ladder)
#define INSTRUCCION 0x0001
#define FUNCION     0x0002
#define OP_RELE     0x0010  //operando tipo rele
#define OP_TIMER    0x0020  //operando tipo timer
struct inst_tag{
         unsigned char tipo;     // INSTRUCCION o FUNCION, OP_RELE o OP_TIMER
         unsigned char opcode;   // en el caso de una funcion indica el nro.
                                 // (las funciones con punto, 0. a 9., son
                                 // los nros. 110 a 119)
         unsigned operando;
       };

    friend class visor;

    plcsim_t *sim;          // el PLC simulado

    inst_tag instruc[PASOS];
    unsigned nro_instruc;

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

    char ayuda[31];         // descripcion del input/output resaltado
    int ayudaActual;

    unsigned char input[MaxInput];   // estado mostrado de cada input
    unsigned char output[MaxOutput]; // estado mostrado de cada output
    int xplc,yplc;
    unsigned anchoplc,altoplc;
    unsigned nroInputs,nroOutputs;
    unsigned altoInputs,altoOutputs;

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

    char nprog[41];
    cajadialogo cd_carg_prog;

    friend Link;

    void iniciar(void);
    void reset(void);
    void resetColores(void);
    void resetTotal(void);
    void copiar_programa(void);
    int procesar_archivo2(FILE *archivo);
    unsigned char valor(unsigned nro) const;
    void alternar(int nro);
    void simular(void);
    void mostrar_uno(int nro, int recuadro);
    void mostrar_grafica(int x, int y, int forzar);
    void actualizar_grafica(void);
    int calcular_destino(int xm, int ym);
    void moverplc(void);
    int crear_rung(visor *rg, int *ninst, int *nfilas) const;
  public:
    plc(int x, int y, Link *l, unsigned nroI, unsigned nroO);
    ~plc();
    int cargar_programa(char *archivo);
    int cargar_cableado_externo(char *archivo);
    int evento(eventoM *ev);
    void dibujar(int forzar);
    void conectar(int sino) { plcsim_set_linked(sim,sino); }
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
