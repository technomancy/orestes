#include <stdio.h>
#include <stdlib.h>

#define MAX_STACK 256 // in bytes
#define MAX_WORD_SIZE 64 // in bytes

#define error printf
#define db printf
#define out printf

typedef int cell;

enum entry_type {
  PRIMITIVE,
  IMMEDIATE,
  CONSTANT,
  VARIABLE,
  COLON
};

typedef struct {
  struct dict * prev;
  enum entry_type type;
  char * name;
  cell * body;
} dict;

cell * s0 = NULL;
cell * sp = NULL;

dict * cp = NULL;
dict * compiling = NULL;

cell * ip = NULL;
cell * dp = NULL;

char * tib = NULL;

short conditionals = 0xffff;
char conditional_depth = 0;


// helper functions

void push(cell c) {
  * sp = c;
  sp++;
};

void define(char * name, enum entry_type type, void * body) {
  dict * cp_prev = cp;

  cp = (dict*)malloc(sizeof(dict));

  if(cp != NULL) {
    cp->prev = cp_prev;
    cp->name = name;
    cp->body = (cell*) body;
    cp->type = type;
  } else {
    error("oom\n");
  }
};

char check_for_done_skipping() {
  dict * entry = drop();

  if(entry && entry->type == PRIMITIVE) {
    // literals need an extra skip
    push(entry->name);
    push("literal");
    string_eq();
    if(drop()) {
      ip++;
    }
    push(entry->name);
    push("then");
    string_eq();
    return !drop();
  } else {
    return 1;
  }
};

void run_body(dict * entry) {
  for(ip = entry->body; *ip; ip++) {
    push(*ip);
    if((1 << conditional_depth) & conditionals) {
      execute();
    } else {
      if(!check_for_done_skipping()) {
        conditional_depth--;
        conditionals = (1 << conditional_depth) | conditionals;
      };
    }
  }
};


// stack and memory primitives

cell drop(void) {
  sp--;
  if(sp < s0) {
    error("Stack underflow\n");
    sp = s0;
    return 0;
  }
  return *sp;
};

void dup(void) {
  cell c = drop();
  push(c);
  push(c);
};

void dot_s(void) {
  out("<%d> ", sp - s0);
  for(cell * i = s0; i < sp; i++) {
    out("%d ", *i);
  }
  out("\n");
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

void plus(void) {
  push(drop() + drop());
};


// parsing primitives

void to_number(void) {
  char * in = (char*)drop();
  cell n = 0;

  for(char i = 0; in[i]; i++) {
    if(in[i] >= 48 && in[i] < 58) {
      n *= 10;
      n += (in[i] - 48);
    } else {
      free(in);
      push(0);
      return;
    }
  }

  free(in);
  push(n);
  push(1);
};

void word(void) {
  char i = 0;
  char * new_str = 0;

  // skip till space or newline or null
  while(tib[i] != 32 && tib[i] != 10 && tib[i]) { i++; };
  i++;

  if(new_str = malloc(i)) {
    for(char j = 0; j < i - 1; j++) {
      new_str[j] = tib[j];
    }
    new_str[i] = 0; // slap a null terminator on

    push(new_str);
    tib += i;
  } else {
    error("oom\n");
  }
};

void string_eq(void) {
  char * c1 = (char*)drop();
  char * c2 = (char*)drop();

  for(int i = 0; c1[i] || c2[i]; i++) {
    if(c1[i] != c2[i]) {
      push(0);
      return;
    }
  }
  push(1);
};


// flow control primitives

void iff(void) {
  conditional_depth++;
  if(conditional_depth++ > 0xffff) {
    error("if too nested\n");
  } else {
    if(drop()) { // there's surely a cleverer way to do this
      conditionals = (1 << conditional_depth) | conditionals;
    } else {
      conditionals = (0 << conditional_depth) & conditionals;
    }
  }
};

void then(void) {}; // placeholder


// interpreter and compiler primitives

void find(void) {
  char * target = drop();

  for(dict * cur = cp; cur; cur = cur->prev) {

    push(target);
    push(cur->name);
    string_eq();
    if(drop()) {
      push(cur);
      return;
    }
  }
  push(0);
};

void literal(void) {
  ip++;
  push(*ip);
};

void constant(void) {
  cell value = drop();
  word();
  char * name = (char*)drop();
  define(name, CONSTANT, value);
};

void variable(void) {
  word();
  char * name = (char*)drop();
  define(name, VARIABLE, -1);
};

void colon(void) {
  word();

  compiling = (dict*)malloc(sizeof(dict));
  compiling->name = (char*)drop();
  compiling->body = malloc(MAX_WORD_SIZE);
  compiling->type = COLON;

  dp = compiling->body;
};

void semicolon(void) {
  compiling->prev = cp;
  dp++;
  *dp = NULL;
  cp = compiling;
  // TODO: this causes all kinds of segfaults
  // realloc(cp->body, (db - (int)cp->body));
  compiling = dp = NULL;
};

void add_to_definition(void) {
  *dp = drop();
  dp++;
  if(dp > compiling->body + MAX_WORD_SIZE) {
    error("definition too big");
  }
};

void execute(void) {
  dict * entry = drop();

  if(compiling && entry->type != IMMEDIATE) {
    push(entry);
    add_to_definition();
  } else if(entry->type == PRIMITIVE || entry->type == IMMEDIATE) {
    // db(" p> %s\n", entry->name);
    void (*primitive)(void) = entry->body;
    (*primitive)();
  } else if(entry->type == COLON) {
    // db(" :> %s %d\n", entry->name, compiling);
    run_body(entry);
  } else if(entry->type == CONSTANT) {
    push(entry->body);
  } else if(entry->type == VARIABLE) {
    push(&entry->body);
  } else {
    error("Unknown type %d\n", entry->type);
  }
};

void interpret(void) {
  // dot_s();
  word();

  dup(); // keep it around in case it's a number
  find();
  dict * entry = drop();

  if(entry) {
    free((char *)drop());
    push(entry);
    execute();
  } else {
    if(compiling) {
      to_number();
      if(!drop()) {
        error("unknown thingy\n");
      } else {
        push("literal");
        find();
        add_to_definition();
        add_to_definition();
      }
    } else {
      to_number();
      if(!drop()) {
        error("unknown thingy\n");
      }
    }
  }

  out(" ok\n");
};



int main (void) {
  sp = s0 = malloc(MAX_STACK);

  // define primitives
  define("drop", PRIMITIVE, &drop);
  define("dup", PRIMITIVE, &dup);
  define(".s", PRIMITIVE, &dot_s);
  define("@", PRIMITIVE, &fetch);
  define("!", PRIMITIVE, &store);
  define("+", PRIMITIVE, &plus);

  define(">number", PRIMITIVE, &to_number);
  define("word", PRIMITIVE, &word);
  define("string=", PRIMITIVE, &string_eq);

  define("if", PRIMITIVE, &iff);
  define("then", PRIMITIVE, &then);

  define("find", PRIMITIVE, &find);
  define("literal", PRIMITIVE, &literal);
  define("constant", PRIMITIVE, &constant);
  define("variable", PRIMITIVE, &variable);
  define(":", PRIMITIVE, &colon);
  define(";", PRIMITIVE, &semicolon); cp->type = IMMEDIATE;
  define(",", PRIMITIVE, &add_to_definition);
  define("execute", PRIMITIVE, &execute);
  define("interpret", PRIMITIVE, &interpret);

  tib = malloc(80);

  while(gets(tib)) {
    while(*tib) {
      interpret();
    }
  };
};
