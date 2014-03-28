#include <stdio.h>
#include <stdlib.h>

#define MAX_STACK 256 // in bytes
#define MAX_WORD_SIZE 64 // in bytes

#define error printf
#define db printf

typedef int cell;

enum entry_type {
  PRIMITIVE,
  IMMEDIATE,
  CONSTANT,
  VARIABLE,
  COLON
};

struct dictionary {
  struct dictionary * prev;
  char name_size;
  char * name;
  enum entry_type type;
  cell * body;
};

typedef struct dictionary dict;

cell * s0 = NULL;
cell * sp = NULL;

dict * cp = NULL;
dict * compiling = NULL;

cell * ip = NULL;
cell * dp = NULL;

char skipping = 0;

char * tib = \
  "0 : hi if 12 32 + then 11 ; hi .s "; // should be 11
  // "9 dup constant x variable y : inc 1 + ; 3211 y ! x + y @ + inc .s "; // 3230


// helper functions

// TODO: define this as taking a union type
void push(cell c) {
  * sp = c;
  sp++;
};

void define(char * name, char name_size, enum entry_type type, void * body) {
  dict * cp_prev = cp;

  cp = (dict*)malloc(sizeof(dict));

  if(cp != NULL) {
    cp->prev = cp_prev;
    cp->name_size = name_size;
    cp->name = name;
    cp->body = (cell*) body;
    cp->type = type;
  } else {
    error("oom\n");
  }
};

void define_primitive(char * name, void (*body)(void)) {
  int length = 0;
  for(length = 0; name[length]; length++) {}
  define(name, length, PRIMITIVE, body);
};

char check_for_done_skipping() {
  dict * entry = drop();

  db("entry %d\n", entry);
  if(entry && entry->type == PRIMITIVE) {
    // literals need an extra skip
    push(entry->name);
    push(entry->name_size);
    push("literal");
    push(7);
    string_eq();
    if(drop()) {
      ip++;
    }
    push(entry->name);
    push(entry->name_size);
    push("then");
    push(4);
    string_eq();
    return !drop();
  } else {
    return 1;
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
  return * sp;
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

void dot_s(void) {
  printf("<%d> ", (sp - s0) / sizeof(cell));
  for(cell * i = s0; i < sp; i++) {
    printf("%d ", *i);
  }
  printf("\n");
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

void word(void) { // char -- c-addr u
  cell delimiter = drop();
  char * i = 0;

  // TODO: need to allocate space for this once tib gets replaced
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


// flow control primitives

void iff(void) {
  skipping = !drop();
};

void then(void) {
  skipping = 0;
};


// interpreter and compiler primitives

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
      push(cur);
      return;
    }
  }
  push(0);
};

void literal(void) {
  ip++;
  db("lit\n");
  push(*ip);
  db("plit\n");
};

void constant(void) {
  cell value = drop();
  push((cell)32);
  word();
  char name_size = (char)drop();
  char * name = (char*)drop();
  define(name, name_size, CONSTANT, value);
};

void variable(void) {
  push((cell)32);
  word();
  char name_size = (char)drop();
  char * name = (char*)drop();
  define(name, name_size, VARIABLE, -1);
};

void colon(void) {
  push((cell)32);
  word();

  compiling = (dict*)malloc(sizeof(dict));
  compiling->name_size = (char)drop();
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
  // TODO: realloc compiling->body to save unused space
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

  if(compiling && ! entry->type == IMMEDIATE) {
    push(entry);
    add_to_definition();
  } else if(entry->type == PRIMITIVE || entry->type == IMMEDIATE) {
    db(" > %s\n", entry->name);
    void (*primitive)(void) = entry->body;
    (*primitive)();
  } else if(entry->type == COLON) {
    for(ip = entry->body; *ip; ip++) {
      db("docolon %d %d\n", ip, skipping);
      push(*ip);
      if(skipping) {
        skipping = check_for_done_skipping();
      } else {
        execute();
      }
    }
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
  push((cell)32);
  word();

  dup2(); // keep it around in case it's a number
  find();

  dict * entry = drop();

  if(entry) {
    drop(); drop(); // drop dup'd word for to_number
    push(entry);
    execute();
  } else {
    if(compiling) {
      to_number();
      drop();
      push("literal");
      push(7);
      find();
      add_to_definition();
      add_to_definition();
    } else {
      to_number();
      if(!drop()) {
        error("unknown thingy\n");
      }
    }
  }

  printf(" ok\n");
};



int main (void) {
  sp = s0 = malloc(MAX_STACK);

  // define primitives
  define_primitive("drop", &drop);
  define_primitive("dup", &dup);
  define_primitive("dup2", &dup2);
  define_primitive(".s", &dot_s);
  define_primitive("@", &fetch);
  define_primitive("!", &store);
  define_primitive("+", &plus);

  define_primitive(">number", &to_number);
  define_primitive("word", &word);
  define_primitive("string=", &string_eq);

  define_primitive("if", &iff);
  define_primitive("then", &then);

  define_primitive("find", &find);
  define_primitive("literal", &literal);
  define_primitive("constant", &constant);
  define_primitive("variable", &variable);
  define_primitive(":", &colon);
  define_primitive(";", &semicolon); cp->type = IMMEDIATE;
  define_primitive(",", &add_to_definition);
  define_primitive("execute", &execute);
  define_primitive("interpret", &interpret);

  while(*tib) {
    interpret();
  }

  return 0;
};
