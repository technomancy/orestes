/* #include <avr/io.h> */
/* #include <util/delay.h> */
#include <stdio.h>
#include <stdlib.h>

#define CELL_SIZE 2
#define MAX_STACK 1024

typedef short cell;

typedef cell xt;

struct dictionary {
  struct dictionary * prev;
  int name_size;
  char * name;
  xt * code;
  xt * body;
  char immediate;
};

typedef struct dictionary dict;

cell * s0 = 0;

void * ip = 0;
dict * cp = 0;
cell * sp = 0;
cell * rp = 0;

char * tib = "3211 .s";

char state = 0;


// helper functions

void push(cell c) {
  * sp = c;
  sp += CELL_SIZE;
};


// primitives

void dot_s(void) {
  printf("<%d> ", (sp - s0) / CELL_SIZE);
  for(cell * i = s0; i < sp; i += CELL_SIZE) {
    printf("%d ", *i);
  }
  printf("\n");
};

void number(void) {
  cell n = 0;
  int place = 1;
  char * eos = 0;
  char * i = 0;

  for(eos = tib; *eos != 32; eos++) {}

  for(i = eos - 1; i >= tib; i--) {
    n += ((*i - 48) * place);
    place = place * 10;
  }

  tib = eos;
  push(n);
};

void interpret(void) {
  number();
  printf(" ok\n");
};



int main (void) {
  sp = s0 = malloc(MAX_STACK);

  interpret();
  dot_s();
};
