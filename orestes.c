/* #include <avr/io.h> */
/* #include <util/delay.h> */
#include <stdio.h>
#include <stdlib.h>

#define CELL_SIZE 4
#define MAX_STACK 1024

typedef long cell;

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

dict * cp = 0;
cell * sp = 0;

char * tib = "3211 dup .s";

char state = 0;


// helper functions

void push(cell c) {
  * sp = c;
  sp += CELL_SIZE;
};

cell pop(void) {
  sp -= CELL_SIZE;
  if(sp < s0) {
    printf("Stack underflow\n");
    sp = s0;
  }
  return * sp;
};


// primitives

void dot_s(void) {
  printf("<%d> ", (sp - s0) / CELL_SIZE);
  for(cell * i = s0; i < sp; i += CELL_SIZE) {
    printf("%d ", *i);
  }
  printf("\n");
};

void to_number(void) { // c-addr1 u1 -- ud2 f
  cell in_size = pop();
  char * in = (char*) pop();
  cell n = 0;

  for(char * i = in; i < in_size + in; i++) {
    if(*i >= 48 && *i < 58) {
      n *= 10;
      n += (*i - 48);
    } else {
      push(0);
      return;
    }
  }

  push(n);
  push(1);
};

void word(void) { // char -- c-addr u
  cell delimiter = pop();
  char * i = 0;

  for(i = tib; *i != (char)delimiter; i++) {}

  push(tib);
  push(i - tib);
  tib = i + 1;
};

void find(void) {
  push(0);
  return;

  int target_size = pop();
  char * target = pop();

  for(dict * cur = cp; cur->prev; cur = cur->prev) {
    if(target_size == cur->name_size) {
      int mismatch = 0;
      for(int i = 0; i < target_size; i++) {
        if(*(cur->body + i) != *(target + i)) {
          mismatch = 1;
          break;
        }
      }

      if(!mismatch) {
        push(cur);
        return;
      }
    }
  }
};

void dup(void) {
  cell c = pop();
  push(c);
  push(c);
};

void execute (void) {};

void interpret(void) {
  push((cell)32);
  word();

  find();

  if(pop()) {
    execute();
  } else {
    to_number();
    if(!pop()) {
      printf("unknown thingy\n");
    }
  }

  printf(" ok\n");
};



int main (void) {
  sp = s0 = malloc(MAX_STACK);

  interpret();
  dot_s();
};
