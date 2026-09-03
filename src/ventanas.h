/* =================================================
   |                                               |
   |                  Ventanas                     |
   |                                               |
   |     (c) Copyright 1996,97  Rodrigo Alonso     |
   |                                               |
   ================================================|  */


#ifndef _VENTANAS_H__
#define _VENTANAS_H__

#include <graphics.h>
#include <string.h>

//----------------------------------------------------------------------------
// Generales
//----------------------------------------------------------------------------
typedef unsigned int word;
void mensaje(char *mensaj, int cl, int cf, int ct);
void error(char *mensaj, int doom);
inline int min(int a, int b) { return (a<b)?a:b; }
inline int max(int a, int b) { return (a>b)?a:b; }
void mayusculas(char *s);
void minusculas(char *s);

//----------------------------------------------------------------------------
// Graficos
//----------------------------------------------------------------------------
const int
      colorBorde=DARKGRAY,
      colorFondo=LIGHTGRAY,
      colorTexto=BLACK,
      colorResaltado=LIGHTBLUE,
      colorHotKey=RED,
      colorON=RED,
      colorOFF=WHITE,
      colorSombra=DARKGRAY,
      colorLuz=WHITE,
      colorFondoLineaInput=WHITE;

extern int MAXX,MAXY;
extern unsigned alturaT,anchoT;

void caja3d(int x, int y, unsigned ancho, unsigned alto, int cl, int cs,
            int cf);
void circulo3d(int x, int y, unsigned radio, int cl, int cs, int cf);
void pizarra3d(int x, int y, unsigned ancho, unsigned alto, int cl, int cs,
               int cf, int cpapel);
void hendiduraCaja(int x, int y, unsigned ancho, unsigned alto,
                   int cl, int cs);
void hendiduraCirculo(int x, int y, unsigned radio, int cl, int cs);
void hendidura(int x, int y, int x2, int y2, int cl, int cs);

void Iniciar(void);
void Finalizar(void);

//----------------------------------------------------------------------------
// Mouse
//----------------------------------------------------------------------------
#define esconderMouse()  { ; } // asm { MOV AX,2; INT 0x33; }
#define mostrarMouse()  { ; } //asm { MOV AX,1; INT 0x33; }

#define IZQUIERDA 75
#define DERECHA   77
#define ARRIBA    72
#define ABAJO     80
#define PGUP      73
#define PGDOWN    81
#define DELETE    83
#define END       79
#define HOME      71
#define BACKSPACE 8
#define TAB       9
#define ENTER     13
#define ESCAPE    27
#define SPACE     32

struct eventoM          // Evento (la M es de 'M'ouse, porque anteriormente
                        // esta estructura solo contenia campos para eventos
                        // del mouse. Eso cambio, pero el nombre quedo)
{
  int xm;
  int ym;
  int bizq;
  int bder;
  int prei;
  int pred;
  int soli;
  int sold;
  int tecla;      // nro. de tecla que fue presionada, (0 si ninguna lo fue).
  int extendida;  // 1 si es una tecla de las llamadas extendidas.
};

#define EV_NULO -1
#define EV_AFUERA -2
#define EV_TERMINAR -3

struct iconoMouse
{
  word arreglo [2][16];
  int hhs,vhs;
};

int iniciar_mouse(void);
void scanEvento(eventoM *ev);
void flushEventos(void);
void cambiarIconoMouse(struct iconoMouse *p);

typedef struct iconoMouse *ptrIconoMouse;

extern struct iconoMouse iconoFlecha,iconoMano,iconoNulo,iconoProhibido,
                         iconoSelec;

extern ptrIconoMouse picmo;

//----------------------------------------------------------------------------
// Clase container
//----------------------------------------------------------------------------
#define maxPuntero 16
class container
{
  private:
    unsigned int _actual,nro;
    void *vector[maxPuntero];
    unsigned int buscar(void *p);
  public:
    container(void);
    container(int npunt);
    ~container();
    int insertar(void *p);
    int borrar(void *p);
    int existe(void *p);
    unsigned int getNumero() { return nro; }
    void *siguiente();
    void *anterior();
    void *primero();
    void *ultimo();
    void *actual();
    int setActual(void *p) { return existe(p); }
};

//----------------------------------------------------------------------------
// Clase item --abstracta--
//----------------------------------------------------------------------------
class itemVisible // clase virtual
{
  public:
    int x,y,anc,alt;
    virtual int interior(int xm, int ym)
        { return (xm>=x && xm<x+anc && ym>=y && ym<y+alt); }
    void setxy(int _x, int _y) { x=_x; y=_y; }
    void getxy(int *_x, int *_y) { *_x=x; *_y=y; }
    void getanal(int *an, int *al) { *an=anc; *al=alt; }
    virtual void mover(int dx, int dy) { x+=dx; y+=dy; }
    virtual void dibujar(void)=0;        //pure virtual function
    virtual int evento(eventoM *ev);
};

//----------------------------------------------------------------------------
// Clase boton
//----------------------------------------------------------------------------
#define MaxLongBoton 30
class boton : public itemVisible
{
  protected:
    enum {NORMAL=0,RESALTADO=1,APRETADO=2};
    int estado;
    int command;              // *DEBE* ser >0 . (Precondicion de la clase)
    char texto[MaxLongBoton];
  public:
    unsigned char toggle;    // Boton Flip-Flop
    unsigned char apret;     // Apretado. (solo para flip-flop)
    boton(void) { ; }
    boton(const char *s, int _command, unsigned char t=0, unsigned char ap=0, int _x=0, int _y=0)
      { setear(s,_command,t,ap,_x,_y); }
    virtual void dibujar(void);
    void setear(const char *s, int _command, unsigned char t,
                unsigned char ap, int _x, int _y);
    int comando(void) { return command; }
    virtual int evento(eventoM *ev);
};

//----------------------------------------------------------------------------
// Clase reticula
//----------------------------------------------------------------------------
class reticula : public itemVisible
{
  protected:
    unsigned filas;        // cantidad de filas
    unsigned columnas;     // cantidad de columnas
    unsigned anchoC;    // ancho de un cuadradito de la reticula
    unsigned altoC;     // alto de un cuadradito de la reticula
    int actual;
    int colorL,colorS,colorF;   // color de Luz,Sombra y Fondo
    int colorL2,colorS2,colorF2; // color de Luz,Sombra y Fondo del prendido
    char *contenido;
  public:
    reticula(unsigned f, unsigned c, unsigned anC, unsigned alC);
    ~reticula();
    virtual void dibujar(void);
    virtual int evento(eventoM *ev);
    int getpos(int xm, int ym);
    void dibujar_uno(unsigned c, unsigned f);
    void limpiar(void);
    void setcolores(int clu, int cso,int cfo, int clu2, int cso2,int cfo2);
};

//----------------------------------------------------------------------------
// Clase barraDesplazamiento --abstracta--
//----------------------------------------------------------------------------
class barraDesplazamiento : public itemVisible
{
  protected:
    int largo,menor,mayor;
    int *resultado;
    int posselectora;            // posicion del selector
    int prepos;  // posicion anterior del cursor
  public:
    barraDesplazamiento(void) { ; }
    barraDesplazamiento(int _largo, int _menor, int _mayor, int _pos,
                        int _x=0, int _y=0)
      { setear(_largo,_menor,_mayor,_pos,_x,_y); }
    void setear(int _largo, int _menor, int _mayor, int _pos, int _x, int _y);
//    virtual int evento(eventoM *ev);
};

//----------------------------------------------------------------------------
// Clase barraDespHorizontal
//----------------------------------------------------------------------------
class barraDespHorizontal : public barraDesplazamiento
{
  public:
    barraDespHorizontal(void) { ; }
    barraDespHorizontal(int _largo, int _menor, int _mayor, int _pos,
                        int _x=0, int _y=0);
    void dibSelectora(int borrar);
    virtual void dibujar(void);
    virtual int evento(eventoM *ev);
};

//----------------------------------------------------------------------------
// Clase barraDespVertical
//----------------------------------------------------------------------------
class barraDespVertical: public barraDesplazamiento
{
  public:
    barraDespVertical(void) { ; }
    barraDespVertical(int _largo, int _menor, int _mayor, int _pos,
                      int _x=0, int _y=0);
    void dibSelectora(int borrar);
    virtual void dibujar(void);
//    virtual int evento(eventoM *ev);
};

//----------------------------------------------------------------------------
// Clase display
//----------------------------------------------------------------------------
#define MaxLongDisplay 100
#define LongTexto -1
class display : public itemVisible
{
  protected:
    int caracteres;   // nro. de caracteres que entran en el display
    char *texto;
    int cl,cs,cf,cp,ct;
    int dibcomoboton;
  public:
    display(void) { x=y=0; }
    display(const char *s, int caract, int _x=0, int _y=0)
      { setear(s,caract,_x,_y); }
    virtual void dibujar(void);
    void setear(const char *s, int caract, int _x, int _y);
    void settexto(char *s) { texto=s; }
    void setcolores(int clu, int cso,int cfo, int cpa, int cti, int dcb);
};

//----------------------------------------------------------------------------
// Clase lineaInput
//----------------------------------------------------------------------------
class lineaInput : public display
{
  protected:
    int posi;  // posicion del cursor
    int longcad; // longitud de la cadena siendo editada
    int cc; // color cursor;
    void dib_cursor(int colcur);
  public:
    lineaInput(void);
    lineaInput(char *s, int caract, int _x=0, int _y=0);
    virtual void dibujar(void);
    virtual int evento(eventoM *ev);
};

//----------------------------------------------------------------------------
// Clase bolsa_de_items
//----------------------------------------------------------------------------
enum {PRIMERO=0,ACTUAL=1};
class bolsa_de_items : public itemVisible
{
  protected:
    itemVisible *actual;
    container c;
    int iterando,nro;
    void resetIterar(int modo);
    itemVisible *iterar(void);
  public:
    bolsa_de_items(void);
    ~bolsa_de_items(void);
    virtual int insertar(itemVisible *iv); // *DEBE* haber por lo menos un item.
                                           // Es una precondicion de la clase.
    virtual void mover(int dx, int dy);
    virtual void dibujar(void);
    void dibujar(int _x, int _y);
    virtual int evento(eventoM *ev);
};

//----------------------------------------------------------------------------
// Clase item_con_fondo --abstracta--
//----------------------------------------------------------------------------
// Los items que se insertan en esta clase (y sus derivadas) no tienen una
// posicion absoluta, sino relativa a la esqina sup. izq. del recuadro que
// los engloba.
class item_con_fondo
{
  protected:
    void *pointer;
    void reservar(int anc, int alt);
    void liberar(void);
    virtual void calc_anc_alt(itemVisible *b)=0;
    void dibujar_fondo(int *x, int *y, unsigned ancho, unsigned alto, int cl,
                       int cs, int cf);
  public:
    item_con_fondo() { pointer=NULL; }
    ~item_con_fondo() { liberar(); }
};

//----------------------------------------------------------------------------
// Clase menu
//----------------------------------------------------------------------------
// La clase 'menu' es igual a bolsa_de_items excepto que a los items los
// encolumna (la posicion x,y original del item es desechada), creando un
// layout tipico de menu, y guarda el 'fondo' (lo que hay detras) del
// recuadro donde dibuja el menu. (Notar que pueden insertarse items de
// cualquier tipo).
class menu : public bolsa_de_items, public item_con_fondo
{
  private:
    virtual void calc_anc_alt(itemVisible *iv);
  public:
    menu();
    ~menu();
    virtual int insertar(itemVisible *iv);
    virtual void dibujar(void);
    virtual int evento(eventoM *ev);
};

//----------------------------------------------------------------------------
// Clase cajadialogo
//----------------------------------------------------------------------------
// Idem 'menu' solo que aqui la posicion(de los items) no es fija(no estan
// encolumnados) sino que la define el usuario. La posicion es relativa y
// esta graduada de 1 a 100.
// Por ejemplo: si nos referimos a la posicion horizontal(x), un valor
// de 1 indica lo mas a la izquierda posible, 50 indica centrado y 100 indica
// lo mas a la derecha posible.
// Si nos referimos a la posicion vertical(y), un valor de 1,50, y 100 indica
// arriba,centrado, y abajo respectivamente.
// Si no se especifica altura cuando se setean los valores de la cajadialogo
// se asume por omision que la altura es la suma de las alturas de todos los
// items(mas los bordes de la cajadialogo). Tambien puede ser especificada
// por el usuario, en cuyo caso queda fija.
// El ancho de la cajadialogo es, por omision, el ancho del item mas ancho
// (mas los bordes de la cajadialogo), aunque tambien puede ser especificado
// por el usuario, en cuyo caso queda fijo.
class cajadialogo : public bolsa_de_items, public item_con_fondo
{
  private:
    int cl,cs,cf;
    char anflot; // flag que indica si el ancho es de tama¤o fijo o flotante.
    char alflot; // flag que indica si la altura es de tama¤o fijo o flotante.
    void calc_anc_alt(itemVisible *iv);
    void calc_posiciones(void);
  public:
    cajadialogo(int _x=0, int _y=0, int al=0, int an=0);
    ~cajadialogo();
    virtual int insertar(itemVisible *iv);
    void setear(int _x, int _y, int al=0, int an=0);
    void setcolores(int clu, int cso,int cfo);
    virtual void dibujar(void);
    virtual int evento(eventoM *ev);
};

#endif // _VENTANAS_H__
