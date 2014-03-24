#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STACK 256 // in bytes

typedef int cell;

typedef cell xt;

struct dictionary {
  struct dictionary * prev;
  int name_size;
  char * name;
  xt * code;
  xt * body;
};

typedef struct dictionary dict;

cell * s0 = NULL;
cell * sp = NULL;

dict * cp = NULL;

char * tib = "3211 dup .s ";

char state = 0;


// helper functions

void push(cell c) {
  * sp = c;
  sp += sizeof(cell);
};

void define(char * name, void (*body)(void)) {
  dict * cp_prev = cp;

  cp = (dict*)malloc(sizeof(dict));

  if(cp != NULL) {
    cp->prev = cp_prev;
    cp->name_size = strlen(name);
    cp->name = name;
    cp->code = (xt*) body;
  } else {
    printf("oom");
  }
};


// primitives

cell drop(void) {
  sp -= sizeof(cell);
  if(sp < s0) {
    printf("Stack underflow\n");
    sp = s0;
    return 0;
  }
  return * sp;
};

void dot_s(void) {
  printf("<%d> ", (sp - s0) / sizeof(cell));
  for(cell * i = s0; i < sp; i += sizeof(cell)) {
    printf("%d ", *i);
  }
  printf("\n");
};

void to_number(void) { // c-addr1 u1 -- ud2 f
  cell in_size = drop();
  char * in = (char*) drop();
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

void fetch(void) {
  cell * target = (cell*)drop();
  push(*target);
};

void store(void) {
  cell * target = (cell*)drop();
  cell value = drop();

  *target = value;
};

void word(void) { // char -- c-addr u
  cell delimiter = drop();
  char * i = 0;

  for(i = tib; *i != (char)delimiter; i++) {}

  push(tib);
  push(i - tib);
  tib = i + 1;
};

void string_eq(void) { // c1-addr u1 c2-addr u2 -- f
  int c1_size = (int)drop();
  char * c1 = (char*)drop();
  int c2_size = (int)drop();
  char * c2   = (char*)drop();

  if(c1_size != c2_size) {
    push(0);
  } else {
    for(int i = 0; i < c1_size; i++) {
      if(*(c1 + i) != *(c2 + i)) {
        push(0);
        return;
      }
    }
    push(1);
  }
};

void find(void) {
  cell target_size = drop();
  cell target = drop();

  for(dict * cur = cp; cur; cur = cur->prev) {
    push(target);
    push(target_size);
    push(cur->name);
    push(cur->name_size);
    string_eq();

    if(drop()) {
      push(cur->code);
      return;
    }
  }
  push(0);
};

void dup(void) {
  cell c = drop();
  push(c);
  push(c);
};

void dup2(void) {
  cell c1 = drop();
  cell c2 = drop();
  push(c2);
  push(c1);
  push(c2);
  push(c1);
};

void execute (void) {
  void (*primitive)(void) = drop();
  (*primitive)();
};

void interpret(void) {
  push((cell)32);
  word();

  dup2();
  find();

  cell xt = drop();
  if(xt) {
    drop(); drop(); // drop dup'd word for to_number
    push(xt);
    execute();
  } else {
    to_number();
    if(!drop()) {
      printf("unknown thingy\n");
    }
  }

  printf(" ok\n");
};



int main (void) {
  sp = s0 = malloc(MAX_STACK);

  // define primitives
  define(".s", &dot_s);
  define("drop", &drop);
  define(">number", &to_number);
  define("word", &word);
  define("find", &find);
  define("dup", &dup);
  define("dup2", &dup2);

  define("fetch", &fetch);
  define("store", &store);

  define("execute", &execute);
  define("interpret", &interpret);

  while(*tib) {
    interpret();
  }

  return 0;
};
