#include <stdio.h>
#include <stdlib.h>

#define MAX_STACK 256 // in bytes
#define MAX_WORD_SIZE 128 // in bytes

#define error printf
#define db printf
#define out printf

enum entry_type {
  PRIMITIVE,
  IMMEDIATE,
  CONSTANT,
  VARIABLE,
  COLON
};

struct dictionary;
union cell;
typedef union cell_t cell;

union cell_t {
  unsigned int i;
  cell * c;
  char * s;
  char ch;
  struct dictionary * d;
  void * v;
  int this_is_here_to_spoon_feed_the_compiler_yay;
};

struct dictionary {
  struct dictionary * prev;
  enum entry_type type;
  char * name;
  cell body;
};

typedef struct dictionary dict;

cell stack_start = { .i = 0 };
cell stack = { .i = 0 };

dict * cp = NULL; // last word to be defined
dict * compiling = NULL;

cell ip = { .i = 0 }; // instruction pointer (while executing word body)
cell dp = { .i = 0 }; // definition pointer (while adding to word definitions)

char * input = NULL;

unsigned short conditionals = 0xffff;
char conditional_depth = 0;

unsigned int loop_counters[16];
unsigned int loop_limits[16];
cell loop_starts[16];
char loop_depth = 0;


// helper functions

void string_eq(void);
cell drop(void);
void execute(void);
void then(void);
void elsee(void);

void push(cell c) {
  *stack.c = (cell)(c.i);
  stack.c++;
};

void define(char * name, enum entry_type type, void * body) {
  dict * cp_prev = cp;

  cp = (dict*)malloc(sizeof(dict));

  if(cp != NULL) {
    cp->prev = cp_prev;
    cp->name = name;
    cp->body.c = (cell*)body;
    cp->type = type;
  } else {
    error("oom\n");
  }
};

char maybe_unskip() {
  dict * entry = drop().d;

  if(entry && entry->type == PRIMITIVE) {
    // literals need an extra skip
    push((cell)entry->name);
    push((cell)"literal");
    string_eq();
    if(drop().i) {
      ip.c++;
    } else {
      push((cell)entry->name);
      push((cell)"then");
      string_eq();
      if(drop().i) {
        then();
      } else {
        push((cell)entry->name);
        push((cell)"else");
        string_eq();
        if(drop().i) {
          elsee();
        }
      }
    }
  }
};

void run_body(dict * entry) {
  for(ip.c = entry->body.c; ip.c->i; ip.c++) {
    // db("running ip %d, %d\n", ip.i, ip.c->c);
    push((cell)ip.c->c);
    if((1 << conditional_depth) & conditionals) {
      execute();
    } else {
      maybe_unskip();
    }
  }
};


// stack and memory primitives

cell drop(void) {
  stack.c--;
  if(stack.i < stack_start.i) {
    error("Stack underflow\n");
    stack = stack_start;
  }
  return *stack.c;
};

void dup(void) {
  cell c = drop();
  push(c);
  push(c);
};

void swap(void) {
  cell c1 = drop();
  cell c2 = drop();
  push(c1);
  push(c2);
};

void over(void) {
  cell c1 = drop();
  cell c2 = drop();
  push(c2);
  push(c1);
  push(c2);
};

void dot_s(void) {
  out("<%d> ", stack.c - stack_start.c);
  for(cell i = stack_start; i.i < stack.i; i.c++) {
    out("%d ", *(i.c));
  }
  out("\n");
};

void fetch(void) {
  cell target = drop();
  push(*target.c);
};

void store(void) {
  cell target = drop();
  *(target.c) = drop();
};

void fetch_byte(void) {
  char b = drop().s[0];
  push((cell)b);
};

void store_byte(void) {
  char * target = drop().s;
  *target = drop().ch;
};

void plus(void) {
  push((cell)(drop().i + drop().i));
};

void minus(void) {
  push((cell)(drop().i - drop().i));
};

void orr(void) {
  push((cell)(drop().i | drop().i));
}

void andd(void) {
  push((cell)(drop().i & drop().i));
}

void nott(void) {
  push((cell)(~drop().i));
}

void equals(void) {
  push((cell)(drop().i == drop().i));
};


// parsing primitives

void to_number(void) {
  char * in = drop().s;
  cell n = {.i = 0};

  for(char i = 0; in[i]; i++) {
    if(in[i] >= 48 && in[i] < 58) {
      n.i *= 10;
      n.i += (in[i] - 48);
    } else {
      free(in);
      push((cell)0);
      return;
    }
  }

  free(in);
  push(n);
  push((cell)1);
};

void word(void) {
  char i = 0;
  char * new_str = 0;

  // skip till space or newline or null
  while(input[i] != 32 && input[i] != 10 && input[i]) { i++; };
  i++;

  if(new_str = malloc(i)) {
    for(char j = 0; j < i - 1; j++) {
      new_str[j] = input[j];
    }
    new_str[i] = 0; // slap a null terminator on

    push((cell)new_str);
    input += i;
  } else {
    error("oom\n");
  }
};

void string_eq(void) {
  char * c1 = drop().s;
  char * c2 = drop().s;

  for(int i = 0; c1[i] || c2[i]; i++) {
    if(c1[i] != c2[i]) {
      push((cell)0);
      return;
    }
  }
  push((cell)1);
};


// flow control primitives

void iff(void) {
  if(conditional_depth++ > 15) {
    error("if too nested\n");
  } else {
    if(drop().i) { // there's surely a cleverer way to do this
      conditionals |= (1 << conditional_depth);
    } else {
      conditionals &= ~(1 << conditional_depth);
    }
  }
};

void elsee(void) {
  conditionals ^= (1 << conditional_depth);
};

void then(void) {
  conditional_depth--;
  conditionals |= (1 << conditional_depth);
};

void doo(void) {
  if(loop_depth++ > 15) {
    error("do too nested\n");
  } else {
    loop_counters[loop_depth] = drop().i;
    loop_limits[loop_depth] = drop().i;
    loop_starts[loop_depth] = ip;
  }
};

void loop(void) {
  if(loop_counters[loop_depth] != loop_limits[loop_depth]) {
    loop_counters[loop_depth]++;
    ip = loop_starts[loop_depth];
  } else {
    if(--loop_depth < 0) {
      error("do/loop mismatch\n");
    }
  }
};

void i(void) {
  push((cell)loop_counters[loop_depth]);
};

void j(void) {
  push((cell)loop_counters[loop_depth - 1]);
};
void k(void) {
  push((cell)loop_counters[loop_depth - 2]);
};

void begin (void) {
  if(loop_depth++ > 15) {
    error("do too nested\n");
  } else {
    loop_starts[loop_depth] = ip;
  }
};

void again(void) {
  ip = loop_starts[loop_depth];
};


// interpreter and compiler primitives

void find(void) {
  char * target = drop().s;

  for(dict * cur = cp; cur; cur = cur->prev) {
    push((cell)target);
    push((cell)cur->name);
    string_eq();
    if(drop().i) {
      push((cell)cur);
      return;
    }
  }
  push((cell)0);
};

void literal(void) {
  ip.c++;
  push((cell)ip.c->i);
};

void constant(void) {
  cell value = drop();
  word();
  char * name = drop().s;
  define(name, CONSTANT, value.c);
};

void variable(void) {
  word();
  char * name = drop().s;
  define(name, VARIABLE, (cell*)-1);
};

void colon(void) {
  word();

  compiling = (dict*)malloc(sizeof(dict));
  compiling->name = drop().s;
  compiling->body.c = malloc(MAX_WORD_SIZE);
  compiling->type = COLON;

  dp.c = compiling->body.c;
};

void semicolon(void) {
  compiling->prev = cp;
  dp.d++;
  cp = compiling;
  // TODO: this causes all kinds of segfaults
  // realloc(cp->body, (db - (int)cp->body));
  compiling = NULL;
};

void add_to_definition(void) {
  *(dp.c) = (cell)drop();
  dp.c++;
  if(dp.i > compiling->body.i + MAX_WORD_SIZE) {
    error("definition too big\n");
  }
};

void execute(void) {
  dict * entry = drop().d;

  if(compiling && entry->type != IMMEDIATE) {
    push((cell)entry);
    add_to_definition();
  } else if(entry->type == PRIMITIVE || entry->type == IMMEDIATE) {
    // db(" p> %s\n", entry->name);
    void (*primitive)(void) = entry->body.v;
    (*primitive)();
  } else if(entry->type == COLON) {
    // db(" :> %s %d\n", entry->name, compiling);
    run_body(entry);
  } else if(entry->type == CONSTANT) {
    push(entry->body);
  } else if(entry->type == VARIABLE) {
    push((cell)&entry->body);
  } else {
    error("Unknown type %d\n", entry->type);
  }
};

void interpret(void) {
  // dot_s();
  word();

  dup(); // keep it around in case it's a number
  find();
  dict * entry = drop().d;

  if(entry) {
    free(drop().s);
    push((cell)entry);
    execute();
  } else {
    if(compiling) {
      to_number();
      if(!drop().i) {
        error("unknown thingy\n");
      } else {
        push((cell)"literal");
        find();
        add_to_definition();
        add_to_definition();
      }
    } else {
      to_number();
      if(!drop().i) {
        error("unknown thingy\n");
      }
    }
  }
};

void exitt(void) {
  exit(0);
};



int main (void) {
  stack.v = stack_start.v = malloc(MAX_STACK);

  // define primitives
  define("drop", PRIMITIVE, &drop);
  define("dup", PRIMITIVE, &dup);
  define("swap", PRIMITIVE, &swap);
  define("over", PRIMITIVE, &over);
  define(".s", PRIMITIVE, &dot_s);
  define("@", PRIMITIVE, &fetch);
  define("!", PRIMITIVE, &store);
  define("@c", PRIMITIVE, &fetch_byte);
  define("!c", PRIMITIVE, &store_byte);

  define("+", PRIMITIVE, &plus);
  define("-", PRIMITIVE, &minus);
  define("or", PRIMITIVE, &orr);
  define("and", PRIMITIVE, &andd);
  define("not", PRIMITIVE, &nott);
  define("=", PRIMITIVE, &equals);

  define(">number", PRIMITIVE, &to_number);
  define("word", PRIMITIVE, &word);
  define("string=", PRIMITIVE, &string_eq);

  define("if", PRIMITIVE, &iff);
  define("else", PRIMITIVE, &elsee);
  define("then", PRIMITIVE, &then);

  define("do", PRIMITIVE, &doo);
  define("loop", PRIMITIVE, &loop);
  define("i", PRIMITIVE, &i);
  define("j", PRIMITIVE, &j);
  define("k", PRIMITIVE, &k);

  define("begin", PRIMITIVE, &begin);
  define("again", PRIMITIVE, &again);

  define("find", PRIMITIVE, &find);
  define("literal", PRIMITIVE, &literal);
  define("constant", PRIMITIVE, &constant);
  define("variable", PRIMITIVE, &variable);
  define(":", PRIMITIVE, &colon);
  define(";", PRIMITIVE, &semicolon); cp->type = IMMEDIATE;
  define(",", PRIMITIVE, &add_to_definition);
  define("execute", PRIMITIVE, &execute);
  define("interpret", PRIMITIVE, &interpret);
  define("exit", PRIMITIVE, &exitt);

  input = malloc(80);

  while(gets(input)) {
    while(*input) {
      interpret();
    }
    out(" ok\n");
  };
};
