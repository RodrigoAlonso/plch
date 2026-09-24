/* ====================================================
   |                                                  |
   |                 Simulacion de PLC                |
   |                                                  |
   |     (c) Copyright 1996,97,98  Rodrigo Alonso     |
   |                                                  |
   ===================================================|  */
//plc.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "conio_shim.h"
#include <graphics.h>
#include <ventanas.h>
#include "plc.h"
#include "soporte.h"
#include <cerrno>

//----------------------------------------------------------------------------
char copyright[]="(c) Copyright 1996-1997, Rodrigo Alonso - Version 1.7";
char msg1[]="Funcionalidad de esta version(28/05/97): 75%";
char msg2[]="Propuestas, consultas, sugerencias: (01)572-5151, (01)756-7057.";
char codigo[]="n..{.t.P3F.&.h...q.7";

cajadialogo cd_copyright(48,134,110);

//----------------------------------------------------------------------------
// Implementacion de la clase Link
//----------------------------------------------------------------------------
#define AREA_LECTURA     80
#define AREA_ESCRITURA  240
void Link::actualizarLink(void)
{
  int aux,bux=c.getNumero(),dux,eux;
  unsigned char cux;
  plc *p1=(plc *)c.primero();
  plc *p2;

  for (aux=0; aux<bux; aux++)  // Por cada PLC...
  {
    if (p1->conectado != 0)  // ...si esta conectado ...
    for (dux=0; dux<16; dux++) //por cada input del area de link de ese PLC...
    {                          //(16 es el tama¤o en bytes del Area de Link)
      cux=0;
      p2=(plc *)c.primero();//...en funcion de los outputs de los otros PLCs...
      if (p2->conectado != 0)  // ...si estan conectados ...
      for (eux=0; eux<bux; eux++)   // ...calculo el nuevo valor...
      {
        cux|=p2->IOsimul[AREA_ESCRITURA+dux].actual;
        p2=(plc *)c.siguiente();
      }
      p1->IOsimul[AREA_LECTURA+dux].actual=cux; //...y actualizo el input
    }
    c.setActual(p1);
    p1=(plc *)c.siguiente();
  }
}

void Link::dibujar(void)  // NOTA: solo dibuja correctamente si hay 2 PLCs
{
  plc *p1=(plc *)c.primero();
  plc *p2=(plc *)c.siguiente();
  int c1=GREEN,c2=MAGENTA,c3=CYAN;

  if (p1!=NULL && p2!=NULL && p1!=p2)
  {
    if (p1->conectado==0 || p2->conectado==0)
    {
      c1=c2=c3=BLACK;
    }
    setlinestyle(SOLID_LINE,1,THICK_WIDTH);
    setcolor(c1);
    esconderMouse();
    line(p1->xplc+p1->anchoplc-1,p1->yplc+p1->altoplc/2-10,635,p1->yplc+p1->altoplc/2-10);
    line(p2->xplc+p2->anchoplc-1,p2->yplc+p2->altoplc/2+10,635,p2->yplc+p2->altoplc/2+10);
    line(635,p1->yplc+p1->altoplc/2-10,635,p2->yplc+p2->altoplc/2+10);
    setcolor(c2);
    line(p1->xplc+p1->anchoplc-1,p1->yplc+p1->altoplc/2,625,p1->yplc+p1->altoplc/2);
    line(p2->xplc+p2->anchoplc-1,p2->yplc+p2->altoplc/2,625,p2->yplc+p2->altoplc/2);
    line(625,p1->yplc+p1->altoplc/2,625,p2->yplc+p2->altoplc/2);
    setcolor(c3);
    line(p1->xplc+p1->anchoplc-1,p1->yplc+p1->altoplc/2+10,615,p1->yplc+p1->altoplc/2+10);
    line(p2->xplc+p2->anchoplc-1,p2->yplc+p2->altoplc/2-10,615,p2->yplc+p2->altoplc/2-10);
    line(615,p1->yplc+p1->altoplc/2+10,615,p2->yplc+p2->altoplc/2-10);
    setlinestyle(SOLID_LINE,1,NORM_WIDTH);
    mostrarMouse();
  }
}

//----------------------------------------------------------------------------
// Implementacion de la clase PLC
//----------------------------------------------------------------------------
char cadenaNula='\0';
plc::plc(int x, int y, Link *l, unsigned nroI, unsigned nroO)
{
  if (nroI>160 || nroO>160) error("ERROR: Demasiados Inputs/Outputs",1);
  nroInputs=nroI;
  nroOutputs=nroO;
  xplc=x;
  yplc=y;
  altoInputs=(nroInputs/colxfila+(nroInputs%colxfila?1:0));
  altoOutputs=(nroOutputs/colxfila+(nroOutputs%colxfila?1:0));
  altoplc=(altoInputs+altoOutputs+1)*alto*biasy+2*slackY;
  anchoplc=ancho*biasx*colxfila+slackX*2;
  botMenu.setear(" Menu ",40,0,0,xplc+10,yplc+altoInputs*biasy*alto+slackY*3);
  strcpy(archivoPRG,"SIN PROGRAMA");
  dis.setear(archivoPRG,29,xplc+80,yplc+altoInputs*biasy*alto+slackY*3);
  disAyuda.setear(&cadenaNula,30,xplc+326,yplc+altoInputs*biasy*alto+slackY*3);
  disAyuda.setcolores(colorSombra,colorLuz,colorFondo,colorFondo,BLUE,1);

  cd_carg_prog.setear(140,190,80);
  display *di=new display(" Nombre ",LongTexto,0,10);
  di->setcolores(colorSombra,colorLuz,colorFondo,1,colorTexto,1);
  cd_carg_prog.insertar(di);
  cd_carg_prog.insertar(new boton("   Ok   ",100,0,0,30,100));
  cd_carg_prog.insertar(new boton("Cancelar",200,0,0,75,100));
  nprog[0]='\0';
  cd_carg_prog.insertar(new lineaInput(nprog,40,50,50));

  linkable=l;
  linkable->insertar(this);
  conectado=0;

  iniciar();
}

plc::~plc()
{
   for (int aux=0; aux<MaxInput+MaxOutput; aux++)
     if (Ayuda[aux]!=NULL) free(Ayuda[aux]);
}

void plc::reset(void)
{
  int aux;

  for (aux=0; aux<MEMORIA; aux++)
  {
    IOsimul[aux].actual=0;
//    IOsimul[aux].anterior=0;
  }
  for (aux=0; aux<TIMERS; aux++)
  {
    TCsimul[aux].actual=0;
//    TCsimul[aux].anterior=0;
    TCsimul[aux].inicio=0;
    TCsimul[aux].counter=0;
  }
//  for (aux=0; aux<nroInputs; aux++) input[aux]=0;
  for (aux=0; aux<nroOutputs; aux++) output[aux]=2;
//
  botres=0;
  precond=0;
  precond2=0;
  AR=ER=CR=0;
  ayudaActual=0;
  prenderSelectoras();
  IOsimul[839].actual=ON; // 967: prendido durante el primer scan.
  modo=ver_plc;
}

void plc::resetSelectoras(void)  // parte del cableado externo
{
  int aux;

  for (aux=0; aux<nroInputs; aux++)
  {
    inputBloq[aux]=0;
    inputSelec[aux]=-1;
  }
  numselectoras=0;
}

void plc::prenderSelectoras(void)  // Pone las selectoras en su posicion
                                   // por default.
{
  for (int aux=0,bux; aux<numselectoras; aux++)
  {
    bux=selectoras[aux].actual=selectoras[aux].posic[0];
    IOsimul[bux].actual=ON; //IOsimul[bux].anterior=ON;
  }
}

void plc::resetTemporizadores(void)  // parte del cableado externo
{
//  for (int aux=0; aux<Maxtemporizadores; aux++) cabext[aux].inicio=-1;
  numtemporizadores=0;
}

void plc::resetTotal(void)
{
  int aux;

  nro_instruc=0;
  for (aux=0; aux<PASOS; aux++)
  {
    instruc[aux].tipo=instruc[aux].opcode=0;
    instruc[aux].operando=0;
  }
  for (aux=0; aux<MEMORIA; aux++)
  {
//    IO[aux].nombre[0]=IO[aux].nombre[5]='\0';
    strcpy(IO[aux].nombre,"          ");
//    strcpy(IO[aux].descrip,"                              ");
  }
  for (aux=0; aux<TIMERS; aux++)
  {
//    strcpy(TC[aux].descrip,"                              ");
    TC[aux].nombre[0]='\0';
    TC[aux].tipo='T';
    TC[aux].valor=0;
  }
  for (aux=0; aux<nroOutputs; aux++)   // outputs, colores
  {
    colorOut[aux].cluz=LIGHTRED;
    colorOut[aux].cfon=RED;
    colorOut[aux].ctexpres=RED;
    colorOut[aux].ctexdepr=DARKGRAY;
  }
  for (aux=0; aux<nroInputs; aux++)   // inputs, colores
  {
    colorIn[aux].cluz=WHITE;
    colorIn[aux].cfon=LIGHTGRAY;
    colorIn[aux].ctexpres=WHITE;
    colorIn[aux].ctexdepr=BLACK;
  }
  tick=0;
  reset();
  resetTemporizadores();
  resetSelectoras();
}

void plc::iniciar(void)
{
  resetTotal();
  Botoner.insertar(&botMenu);
  Botoner.insertar(&dis);
  Botoner.insertar(&disAyuda);

  botProg.setear("Programa  On/Off",800,1,1,0,0);
  botCabext.setear("Cableado  On/Off",800,1,1,0,0);
  botSonido.setear("Sonido          ",30,1,1,0,0);
  m.insertar(new boton("Cargar Programa ",100));
  m.insertar(new boton("Cargar Cableado ",200));
  m.insertar(&botProg);
  m.insertar(&botCabext);
  m.insertar(new boton("Reset Programa  ",10));
  m.insertar(&botSonido);
  m.insertar(new boton("Ver Programa    ",400));
  m.insertar(new boton("Acerca de PLC   ",300));
  m.insertar(new boton("Salir           ",27));
  archivoCEX[0]='\0';
  for (int aux=0; aux<MaxInput+MaxOutput; aux++) Ayuda[aux]=NULL;
}

//----------------------------------------------------------------------------
// Procesar Archivo.  Lee el archivo con el programa y carga todas las tablas
// que se usan en la simulacion.
// No hace ningun tipo de chequeo (de sintaxis o restricciones) del programa,
// porque supone que el programa fue creado usando el software ACTSIP-E =>
// se trabaja sobre el supuesto de programa correcto.
int plc::procesar_archivo(FILE *archivo)
{
  char aux[120],cux[120];
  int opcode,operando,bux;
  unsigned nro;
  ulong tiemp;
  int resultado=OK;
  int pila[20],cabeza=0;
  int direcc[20],cabezadir=0,dir;

//----------------------------------------------------------------------------
// CABECERA.
  for (bux=0; bux<11; bux++)
  {
    obtener_linea(aux,120,archivo); // se come las 11 primeras lineas
    if (bux==8)
     if (strcmp(aux,"PGM-START")==0) resetTotal();
        else {
               error("ERROR: Formato de archivo incorrecto",0);
               return ERROR;
             }
  }
//----------------------------------------------------------------------------
// PGM-START .. PGM-END. Carga el arreglo de instrucciones.
  nro=0;
  obtener_linea(aux,15,archivo);
  while (strcmp(aux,"PGM-END") != 0)
  {
    cortar(aux,0,4,cux);  // numero de instruccion
//    printf(" %s ",cux);
    if (aux[2] == '-')    // INSTRUCCION
    {
      instruc[nro].tipo=INSTRUCCION;
      cortar(aux,0,2,cux);  // tipo de instruccion (opcode)
      sscanf(cux,"%x",&opcode);
      if (opcode>128)  // tipo de operando
      {
        opcode-=128;
        instruc[nro].tipo|=OP_TIMER;
      } else instruc[nro].tipo|=OP_RELE;
/*
  Desde aqui modificado el 12/01/98.
  Se agregan 4 opcodes para optimizar la impresion de codigo en formato
  ladder.
  Mientras se lee el archivo de programa se realiza el reemplazo, que puede
  interpretarse como una traduccion de notacion postfija (notacion de pila),
  a notacion prefija.
  Ejemplos:

    STR                              STR AND
    .                                .
    .             ===========>       .
    .                                .
    AND STR                          AND STR

    ,

    STR NOT                          STR NOT OR
    .                                .
    .             ===========>       .
    .                                .
    OR STR                           OR STR
*/
      if (opcode==STR || opcode==STR_NOT)  // marcado de opcodes a reemplazar
      {
        push(pila,&cabeza,20,opcode);
        push(direcc,&cabezadir,20,nro);
      } else
      if (opcode==AND_STR || opcode==OR_STR) // reemplazo de opcodes
      {
        bux=pop(pila,&cabeza);
        dir=pop(direcc,&cabezadir);
        if (bux==STR)
        {
          if (opcode==AND_STR) instruc[dir].opcode=STR_AND;
            else instruc[dir].opcode=STR_OR;
        } else //STR_NOT
          {
            if (opcode==AND_STR) instruc[dir].opcode=STR_NOT_AND;
              else instruc[dir].opcode=STR_NOT_OR;
          }
      }
/*
  Hasta aqui
*/
      instruc[nro].opcode=opcode;
      cortar(aux,4,4,cux);  // operando (en hexadecimal)
      sscanf(cux,"%x",&operando);
      instruc[nro].operando=operando;
/*      switch (opcode)
      {
	case 0x01 : printf ("org      %03d\n",operando); break;
	case 0x41 : printf ("org not  %03d\n",operando); break;
	case 0x02 : printf ("str      %03d\n",operando); break;
	case 0x42 : printf ("str not  %03d\n",operando); break;
	case 0x04 : printf ("and      %03d\n",operando); break;
	case 0x44 : printf ("and not  %03d\n",operando); break;
	case 0x20 : printf ("or       %03d\n",operando); break;
	case 0x60 : printf ("or not   %03d\n",operando); break;
	case 0x06 : printf ("and str\n"); break;
	case 0x22 : printf ("or str\n"); break;
	case 0x08 : printf ("out      %03d\n\n",operando); break;
	case 0x48 : printf ("out not  %03d\n\n",operando); break;
      }*/
    } else           // FUNCION
      {
        instruc[nro].tipo=FUNCION;
	cortar(aux,3,2,cux);  // nro. funcion (en decimal)
	sscanf(cux,"%d",&opcode);
// las funciones "con punto" (0. a 9.) se mapean en los nros. del 110 al 119.
        cortar(aux,2,1,cux);  // es una funcion punto ? (tiene un 3 ?)
        if (cux[0]=='3') opcode+=110;
// para que cuando arranque sea necesario un reset antes de detecta un flanco
        if (opcode==0 || opcode==1) instruc[nro].tipo|=TRABA;
        instruc[nro].opcode=opcode;
	cortar(aux,3,4,cux);  // operando (en hexadecimal)
        sscanf(cux,"%x",&operando);
        unsigned temp_operando = (unsigned)operando;
        if (opcode>=110 && opcode<=119)
          BIN_a_BCD(operando,&temp_operando);
        instruc[nro].operando=operando;
//        printf("Funcion   %02d\n\n",operando);
      }
    obtener_linea(aux,15,archivo);
//    getch();
    nro++;
  }
  nro_instruc=nro;          // numero de instrucciones del programa
//  printf(" %s...  Ok\n\n",aux);
//----------------------------------------------------------------------------
// IO-POINTERS-START .. IO-POINTERS-END, IO-DEF-START .. IO-DEF-END
  for (bux=0; bux<17; bux++)
  {
    obtener_linea(aux,120,archivo); // se come 17 lineas
//    if (bux==15) ;//printf(" %s\n",aux);
  }
//----------------------------------------------------------------------------
// IO-START .. IO-END. Carga el arreglo de Input / Output.
  obtener_linea(aux,47,archivo);
  while (strcmp(aux,"IO-END") != 0)
  {
    cortar(aux,0,4,cux);  // numero de IO (en hexadecimal)
    sscanf(cux,"%x",&nro);

    cortar(aux,0,10,cux);  // nombre
    strcpy(IO[nro].nombre,cux);

    cortar(aux,0,30,cux);  // descripcion
    if (nro<nroInputs || (nro>=160 && nro<160+nroOutputs))
    {
      int nroAyuda=nro-(nro<nroInputs?0:(160-nroInputs));
      if (Ayuda[nroAyuda]==NULL)
        if ((Ayuda[nroAyuda]=(char *)malloc(31))==NULL)
          error("ERROR: No se pudo reservar memoria",1);
      strcpy((char *)&(Ayuda[nroAyuda][0]),cux);
    }
//    printf ("%03d %s %s\n",nro,IO[nro].nombre,IO[nro].descrip);
    obtener_linea(aux,47,archivo);
//    getch();
  }
//  printf(" %s...  Ok\n\n",aux);
//----------------------------------------------------------------------------
// TC-START .. TC-END. Carga el arreglo de Timers / Counters.
  obtener_linea(aux,51,archivo);
//  printf(" %s\n",aux);
  obtener_linea(aux,51,archivo);
  obtener_linea(aux,51,archivo);
  while (strcmp(aux,"TC-END") != 0)
  {
    cortar(aux,0,3,cux);  // numero de TC (en base 8, u OCTAL)
    sscanf(cux,"%o",&nro);

    cortar(aux,0,10,cux);  // nombre
//    strcpy(TC[nro].nombre,cux);

    cortar(aux,0,30,cux);  // descripcion
//    strcpy(TC[nro].descrip,cux);

    if (aux[6]=='5')
    {
      TC[nro].tipo='T';
      cortar(aux,0,5,cux);
    } else {
             TC[nro].tipo='C';
             cortar(aux,0,4,cux);
           }
    sscanf(cux,"%D",&tiemp);
    TC[nro].valor=tiemp;
//    printf ("%02d %s %s %c %D\n",nro,TC[nro].nombre,TC[nro].descrip,
//            TC[nro].tipo,TC[nro].valor);
    obtener_linea(aux,51,archivo);
//    getch();
  }
//  printf(" %s...  Ok\n\n",aux);
  return resultado;
}

int plc::cargar_programa(char *archivo)
{
   FILE *ar;
   char mensaj[200]="ERROR: No se puede abrir ";
   char mensaj2[200];
   int resultado=ERROR;

//   printf("\n Abriendo %s ...",archivo);
   if ((ar = fopen(archivo,"rt")) == NULL)
   {
     printf("ERROR del Sistema Operativo al abrir [%s]: %s\n", archivo, strerror(errno));
     strcpy(&(mensaj[strlen(mensaj)]),archivo);
     error(mensaj,0);
     return resultado;
   }
   if ((resultado=procesar_archivo(ar))!=ERROR)
   {
     strcpy(archivoPRG,archivo);
     strcpy(nprog,archivo);
     strcpy(mensaj2,archivo);
     strcpy(&(mensaj2[strlen(mensaj2)])," cargado Ok");
//     mensaje(mensaj2,LIGHTBLUE,BLUE,YELLOW);
   }
   fclose(ar);
   return resultado;
}

//----------------------------------------------------------------------------
// Procesar Archivo2.  Lee el archivo asociado que simula el cableado externo
// al PLC.
int obtener_token(char *s, int longitud, FILE *archivo)
{
     int aux=0,res=0;
     int car=fgetc(archivo);

// se come los comentarios, newlines, blancos, Tabs y comas
     while (car=='/' || car=='\n' || car==' ' || car=='\t' || car==',')
       if (car=='/')
       {
	 car=fgetc(archivo);
	 if (car=='/')
	 {
	   do car=fgetc(archivo); while (car!='\n');
//	   if (aux==0) car=fgetc(archivo);
	 } else
           if (car=='*')
           {
             do
             {
               do car=fgetc(archivo);
               while (car!='*');
               car=fgetc(archivo);
             } while (car!='/');
             car=fgetc(archivo);
           }
       } else car=fgetc(archivo);
// toma el token siguiente
     while (aux<longitud && car!='/' && car!='\n' && car!=' ' && car!=EOF
	    && car!='\t' && car!=',')
     {
       s[aux++]=car;
       car=fgetc(archivo);
       if (isalpha(car)) res=2;
     }
     if (car>=longitud) ungetc(car,archivo);
     s[aux]='\0';
     if (car==EOF) res=1;
     minusculas(s);  // convierte a minusculas
     return res;
}

int plc::procesar_archivo2(FILE *archivo)
{
   char s[21];
   int aux,cux,tok,c1,c2,c3,bloq;
   ulong bux;
   int resultado=OK;
   int respuesta=0;

   obtener_token(s,20,archivo);
// Temporizadores
   if (strcmp(s,"[temporizadores]")==0)
   {
     respuesta|=1;
     while (((tok=obtener_token(s,20,archivo)) < 1) && (numtemporizadores < Maxtemporizadores))
     {
       sscanf(s,"%d",&aux);
       aux=16*(aux/20)+(aux%20); // pasaje de representacion ext. a int.
       obtener_token(s,20,archivo);
       sscanf(s,"%U",&bux);
       obtener_token(s,20,archivo);
       sscanf(s,"%d",&cux);
       cux=16*(cux/20)+(cux%20); // pasaje de representacion ext. a int.
       cabext[numtemporizadores].output=aux;
       cabext[numtemporizadores].tiempo=bux;
       cabext[numtemporizadores].input=cux;
       cabext[numtemporizadores].inicio=-1;
       numtemporizadores++;
     }
   }

// Selectoras
   if (strcmp(s,"[selectoras]")==0)
   {
     respuesta|=2;
     obtener_token(s,20,archivo);
     while (strcmp(s,"inicio")==0)
     {
       cux=0;
       while ((tok=obtener_token(s,20,archivo))!=2 && cux<Posselectoras)
       {
         sscanf(s,"%d",&aux);
         aux=(16*(aux/20)+(aux%20)); // pasaje de representacion ext. a int.
         selectoras[numselectoras].posic[cux++]=aux;
         inputSelec[aux]=numselectoras;
       }
       selectoras[numselectoras].nro=cux;
       if (strcmp(s,"fin")==0)
       {
         obtener_token(s,20,archivo);
         numselectoras++;
       } else resultado=ERROR;
     }
     prenderSelectoras();
   }

// Selectoras
   if (strcmp(s,"[link]")==0)
   {
     respuesta|=4;
     tok=obtener_token(s,20,archivo);
     if (strcmp(s,"presente")==0)
     {
       conectado=1;
       linkable->dibujar();
     } else
       if (strcmp(s,"ausente")==0)
       {
         conectado=0;
         linkable->dibujar();
       } else resultado=ERROR;
     tok=obtener_token(s,20,archivo);
   }

// Colores
   if (strcmp(s,"[colores]")==0 && resultado !=ERROR)
   {
     respuesta|=8;
     while ((tok=obtener_token(s,20,archivo))!=1)
     {
       sscanf(s,"%d",&aux);
       aux=(16*(aux/20)+(aux%20)); // pasaje de representacion ext. a int.
       obtener_token(s,20,archivo);
       if (strcmp(s,"verde")==0) { c1=GREEN; c2=WHITE; c3=BLACK; }
       else if (strcmp(s,"rojo")==0) { c1=RED; c2=WHITE; c3=LIGHTGRAY; }
       else if (strcmp(s,"amarillo")==0) { c1=6; c2=WHITE; c3=BLACK; }
       else if (strcmp(s,"azul")==0) { c1=BLUE; c2=WHITE; c3=LIGHTGRAY; }
       else if (strcmp(s,"celeste")==0) { c1=CYAN; c2=WHITE; c3=BLACK; }
       else if (strcmp(s,"violeta")==0) { c1=MAGENTA; c2=WHITE; c3=LIGHTGRAY; }
       else if (strcmp(s,"gris")==0) { c1=LIGHTGRAY; c2=WHITE; c3=BLACK; }
       else resultado=ERROR;
       if (aux>=160 && aux<160+nroOutputs)
       {
         colorOut[aux-160].cfon=c1;
         colorOut[aux-160].cluz=c1+8;
         if (c1==RED || c1==MAGENTA || c1==BLUE) c2=c1;
           else c2=c1+8;
         colorOut[aux-160].ctexpres=c2;
         colorOut[aux-160].ctexdepr=c3;
       } else
         if (aux>=0 && aux<nroInputs)
         {
           colorIn[aux].cfon=c1;
           colorIn[aux].cluz=c1+8;
           colorIn[aux].ctexpres=c2;
           colorIn[aux].ctexdepr=c3;
           obtener_token(s,20,archivo); // solo para los inputs
           sscanf(s,"%d",&bloq);
           inputBloq[aux]=bloq;
         } else
           if (aux>=0 && aux<160)
           {
             obtener_token(s,20,archivo); // flush
           }
     }
   }
   if (resultado==ERROR || respuesta==0) resultado=ERROR;
   return resultado;
}

int plc::cargar_cableado_externo(char *archivo)
{
   FILE *ar;
   char mensaj[200]="ERROR: No se puede abrir ";
   char mensaj2[200];
   int resultado=ERROR;

   strcpy(archivoCEX,archivo);
//   printf("\n Abriendo %s ...",archivo);
   if ((ar = fopen(archivo,"rt")) == NULL)
   {
     strcpy(&(mensaj[strlen(mensaj)]),archivo);
     error(mensaj,0);
     return resultado;
   }
   if ((resultado=procesar_archivo2(ar))!=ERROR)
   {
     strcpy(mensaj2,archivo);
     strcpy(&(mensaj2[strlen(mensaj2)])," cargado Ok");
//     mensaje(mensaj2,LIGHTBLUE,BLUE,YELLOW);
   } else
     {
       strcpy(mensaj,archivo);
       strcpy(&(mensaj[strlen(mensaj)])," Formato de archivo incorrecto");
       error(mensaj,0);
       return resultado;
     }
   fclose(ar);
   return resultado;
}

//----------------------------------------------------------------------------
// Reles especiales
void plc::especiales(void)
{
  ulong s=tiempo(),t=s-tick;

  IOsimul[834].actual=!IOsimul[834].actual?ON:0;   // 962: oscilante c/scan time
  if (t>50)
  {
    IOsimul[836].actual=!IOsimul[836].actual?ON:0; // 964: oscilante cada 50 cent.
    tick=s;
  }
  IOsimul[862].actual=ON;                      // 990: siempre prendido
}

//----------------------------------------------------------------------------
// Mover el PLC
void plc::moverplc(void)
{
  eventoM ev;
  int dx,dy,ax=0,ay=0,xold,yold;

  setwritemode(XOR_PUT);
  setcolor(YELLOW);
  esconderMouse();
  rectangle(xplc,yplc,xplc+anchoplc,yplc+altoplc);
  mostrarMouse();
  scanEvento(&ev);
  xold=ev.xm;
  yold=ev.ym;
  do
  {
    if (xold!=ev.xm || yold!=ev.ym)
    {
      dx=ev.xm-xold;
      dy=ev.ym-yold;
      esconderMouse();
      rectangle(xplc+ax,yplc+ay,xplc+ax+anchoplc,yplc+ay+altoplc);
      ax+=dx;
      ay+=dy;
      rectangle(xplc+ax,yplc+ay,xplc+ax+anchoplc,yplc+ay+altoplc);
      mostrarMouse();
    }
    xold=ev.xm;
    yold=ev.ym;
    scanEvento(&ev);
  } while (ev.bizq==1);
  xplc+=ax;
  yplc+=ay;
  Botoner.mover(ax,ay);
  esconderMouse();
  rectangle(xplc,yplc,xplc+anchoplc,yplc+altoplc);
  mostrarMouse();
  setwritemode(COPY_PUT);
}

int plc::evento(eventoM *ev)
{
   int comando,comando2,destino,comando3;

   if (botProg.apret) simular();
   if (botCabext.apret) simularcabext();  // cableado externo

   if (modo==ver_plc)
   {
     actualizar_grafica();

     destino=calcular_destino(ev->xm,ev->ym);
     if (destino!=-1)
     {
       if (destino!=ayudaActual && Ayuda[destino]!=NULL)
       {
         disAyuda.settexto((char *)&(Ayuda[destino][0]));
         disAyuda.dibujar();
         ayudaActual=destino;
       }
       if (destino>=nroInputs)
       {
         destino=-1;    // Con esto limito el procesamiento de eventos a
                        // los botones(inputs) unicamente.
       }
     }
     if (destino!=-1 && inputBloq[destino]==0)
     {
       if (inputSelec[destino]==-1)
       {
         if (picmo!=&iconoMano) cambiarIconoMouse(&iconoMano);
         if (((ev->bizq || ev->bder) && (destino!=botres || precond2==0)) || ev->prei || ev->pred)
         {
           if (ev->bder)
           {
             if (precond2==2) IOsimul[botres].actual=!IOsimul[botres].actual?ON:0;
             precond2=2;
           } else precond2=1;
           mostrar_uno(botres,0);
           IOsimul[destino].actual=!IOsimul[destino].actual?ON:0;
           mostrar_uno(destino,1);
           botres=destino;
         } else
         if (ev->sold && precond2!=0)
         {
           IOsimul[botres].actual=!IOsimul[botres].actual?ON:0;
           mostrar_uno(botres,1);
           precond2=0;
         } else
         if (ev->soli && precond2!=0) precond2=0;
         else
         if (destino!=botres)
         {
           mostrar_uno(botres,0);
           mostrar_uno(destino,1);
           botres=destino;
         }
  /*       for (int aux=0; aux<nroInputs; aux++)
         if (IOsimul[aux].actual!=0)
         {
           IOsimul[aux].actual=ON;
         }*/
       } else
         {
           if (picmo!=&iconoSelec) cambiarIconoMouse(&iconoSelec);
           int sel=inputSelec[destino];    // numero de selectora a la que
                                           // pertenece  el boton 'destino'.
           int sel2=selectoras[sel].actual;  // numero de boton de la selectora
                                             // que actualmente esta apretado.
           if (botres!=destino)
           {
             if (ev->bizq)
             {
               if (inputSelec[botres]==sel) IOsimul[botres].actual=0;
               IOsimul[destino].actual=ON; //IOsimul[destino].anterior=ON;
               if (destino!=sel2)
               {
                 IOsimul[sel2].actual=0; //IOsimul[sel2].anterior=0;
                 mostrar_uno(sel2,0);
                 selectoras[sel].actual=destino;
               }
             }
             mostrar_uno(botres,0);
             mostrar_uno(destino,1);
           } else
             if (ev->prei) //&& IOsimul[destino].actual!=ON)
             {
               if (sel2!=destino)
               {
                 IOsimul[sel2].actual=0;
                 mostrar_uno(sel2,0);
                 selectoras[sel].actual=destino;
               }
  //             IOsimul[destino].actual=ON;
               IOsimul[destino].actual=(IOsimul[destino].actual==0) ? ON : 0;
               mostrar_uno(destino,1);
             }
           botres=destino;
         }
     } else
       {
         if (inputBloq[destino]!=0 && picmo!=&iconoProhibido) cambiarIconoMouse(&iconoProhibido);
         if (precond2 == 2)
         {
           IOsimul[botres].actual=!IOsimul[botres].actual?ON:0;
           mostrar_uno(botres,1);
         }
         comando=Botoner.evento(ev);
         if (comando==40)
         {
           comando2=m.evento(ev);
           if (comando2==10)
           {
             reset();
             dibujar(0);
           } else
           if (comando2==27)
           {
             return SALIR;
           } else
           if (comando2==100)
           {
             comando3=cd_carg_prog.evento(ev);
//             mayusculas(nprog);
             if (comando3==100)
             if (cargar_programa(nprog)==OK)
             {
               mensaje(" Programa cargado OK ",LIGHTBLUE,BLUE,YELLOW);
               reset();
               dibujar(1);
             }
           } else
           if (comando2==200)
           {
             comando3=cd_carg_prog.evento(ev);
             mayusculas(nprog);
             if (comando3==100)
             if (cargar_cableado_externo(nprog)==OK)
             {
               mensaje(" Cableado Externo cargado OK ",LIGHTBLUE,BLUE,YELLOW);
               reset();
               dibujar(0);
             }
           } else
           if (comando2==300)
           {
             if (chk(codigo,copyright) != 113648) exit(EXIT_FAILURE);
             if (chk(codigo,msg1) != 89917) exit(EXIT_FAILURE);
             if (chk(codigo,msg2) != 136571) exit(EXIT_FAILURE);
             cd_copyright.evento(ev);
           } else
           if (comando2==400)
           {
             modo=ver_programa;
           } else
           if (comando2==500 || comando2==600)
           {
  //                    mensaje.evento(ev);
           }
         }
         precond2=0;
       }
  /*     bar3d(0,0,80,10,0,0);
     sprintf(st,"%lu",cont);
     setcolor(WHITE);
     outtextxy(1,1,st);*/     // muestra el numero de 'scan' del programa
     if (destino==-1 && comando==EV_AFUERA)
     {
      if (ev->xm>xplc && ev->xm <xplc+anchoplc && ev->ym>yplc && ev->ym<yplc+altoplc)
      {
        if (picmo!=&iconoFlecha) cambiarIconoMouse(&iconoFlecha);
  /*      if (ev->prei==1 && precond)
        {
          moverplc();
          link *lin=linkable;
          setlink(NULL);
          linkable->dibujar();
          esconderMouse();
          setfillstyle(SOLID_FILL,BLACK);
          setcolor(BLACK);
          bar3d(xplc,yplc,xplc+anchoplc,yplc+altoplc,0,0);
          mostrarMouse();
          setlink(lin);
          dibujar();
          linkable->dibujar();
          precond=0;
        } else if (ev->bizq==0) precond=1;
        precond=1; */
      } else
        {
  //        precond=0;
          return -1;
        }
     } //else precond=0;
   } else if (modo==ver_programa)
     {
       mostrar_programa(ev);
       modo=ver_plc;
       dibujar(1);
     }
   return 1;
}
