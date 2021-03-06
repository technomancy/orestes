#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef F_CPU
#include "teensy3/usb_keyboard.h"
// #include "teensy3/usb_dev.h"
#include "teensy3/core_pins.h"
#endif

#include "orestes.h"

#define MAX_STACK 256 // in bytes
#define MAX_WORD_SIZE 512 // in bytes

cell stack_start = { .i = 0 };
cell stack = { .i = 0 };

dict * cp = NULL; // last word to be defined
dict * compiling = NULL;

cell ip = { .i = 0 }; // instruction pointer (while executing word body)
cell dp = { .i = 0 }; // definition pointer (while adding to word definitions)

char * input = NULL;

char debug = 0;

unsigned short conditionals = 0xffff;
char conditional_depth = 0;

unsigned int loop_counters[16];
unsigned int loop_limits[16];
cell loop_starts[16];
unsigned char loop_depth = 0;


// helper functions

void string_eq(void);
cell drop(void);
void execute(void);
void then(void);
void elsee(void);

#ifdef F_CPU
void out(char * s);
#else
#define out printf
#endif

void db(char * s) {
  if(debug) {
    out(s);
  };
};

void push(cell c) {
  *stack.c = c;
  stack.c++;
};

void define(char * name, enum entry_type type, void * body) {
  dict * cp_prev = cp;

  cp = malloc(sizeof(dict));

  if(cp != NULL) {
    cp->prev = cp_prev;
    cp->name = name;
    cp->body.c = (cell*)body;
    cp->type = type;
  } else {
    cp = cp_prev;
    out("oom");
  }
};

void define_constant(char * name, int value) {
  dict * cp_prev = cp;

  cp = malloc(sizeof(dict));

  if(cp != NULL) {
    cp->prev = cp_prev;
    cp->name = name;
    cp->body.i = value;
    cp->type = CONSTANT;
  } else {
    cp = cp_prev;
    out("oom");
  }
};

void maybe_unskip() {
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
        } else {
          push((cell)entry->name);
          push((cell)"if");
          string_eq();
          if(drop().i) {
            conditional_depth++;
            conditionals &= ~(1 << conditional_depth);
          }
        }
      }
    }
  }
};

void run_body(dict * entry) {
  cell orig_ip = ip;
  db("run "); db(entry->name); db("\n");
  for(ip.c = entry->body.c; ip.c->i; ip.c++) {
    push((cell)ip.c->c);
    if((1 << conditional_depth) & conditionals) {
      execute();
    } else {
      maybe_unskip();
    }
  }
  ip = orig_ip;
  db("run done "); db(entry->name); db("\n");
};


// stack and memory primitives

cell drop(void) {
  stack.c--;
  if(stack.i < stack_start.i) {
    out("stack underflow");
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

void rot(void) {
  cell c1 = drop();
  cell c2 = drop();
  cell c3 = drop();
  push(c2);
  push(c1);
  push(c3);
};

void over(void) {
  cell c1 = drop();
  cell c2 = drop();
  push(c2);
  push(c1);
  push(c2);
};

cell rollstar(int i) {
  if(i) {
    cell x = drop();
    cell v = rollstar(i - 1);
    push(x);
    return v;
  } else {
    return drop();
  }
};

void roll(void) {
  push(rollstar(drop().i - 1));
};

void pick(void) {
  cell n = drop();
  push(*(stack.c - n.i));
};

void dot_s(void) {
  char * s = malloc(8);
  sprintf(s, "<%ld> ", (long int)(stack.c - stack_start.c));
  out(s);
  for(cell i = stack_start; i.i < stack.i; i.c++) {
    sprintf(s, "%u ", i.c->i);
    out(s);
  }
  free(s);
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
  unsigned int x = drop().i;
  unsigned int y = drop().i;
  push((cell)(y - x));
};

void times(void) {
  push((cell)(drop().i * drop().i));
};

void divided(void) {
  push((cell)(drop().i / drop().i));
};

void orr(void) {
  push((cell)(drop().i | drop().i));
}

void andd(void) {
  push((cell)(drop().i & drop().i));
}

void nott(void) {
  cell c = { .i = 1 };
  if(drop().i) {
    c.i = 0;
  }
  push(c);
}

void equals(void) {
  push((cell)(drop().i == drop().i));
};

void greaterthan(void) {
  push((cell)(drop().i < drop().i));
};

void lessthan(void) {
  push((cell)(drop().i > drop().i));
};

void right_shift(void) {
  unsigned int places = drop().i;
  push((cell)(drop().i >> places));
};

void left_shift(void) {
  unsigned int places = drop().i;
  push((cell)(drop().i << places));
};

void numout(void) {
  char * s = malloc(8);
  sprintf(s, "%u ", drop().i);
  out(s);
  free(s);
};

void cr(void) {
  out("\n");
};

void rrand(void) {
  unsigned int mod = drop().i;
  cell x = { .i = (rand() % mod) };
  push(x);
};


// parsing primitives

void to_number(void) {
  char * in = drop().s;
  cell n = {.i = 0};

  if(*in) {
    for(unsigned char i = 0; in[i]; i++) {
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
  } else {
    push((cell)0);
  }
};

void word(void) {
  unsigned char i = 0;
  char * new_str = 0;

  // if we're on space, advance
  while(*input == 32 || *input == 10) { input++; };

  // skip till space or newline or null
  while(input[i] != 32 && input[i] != 10 && input[i]) { i++; };

  if((new_str = malloc(i + 1))) {
    for(unsigned char j = 0; j < i; j++) {
      new_str[j] = input[j];
    }
    new_str[i] = 0; // slap a null terminator on
    push((cell)new_str);
    db("parsed: "); db(new_str); db("\n");
    input += i;
  } else {
    out("oomw");
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
    out("if too nested");
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
  conditionals |= (1 << conditional_depth);
  if(--conditional_depth < 0) {
    out("if/then mismatch");
  }
};

void doo(void) {
  if(loop_depth++ > 15) {
    out("do too nested");
  } else {
    loop_counters[loop_depth] = drop().i;
    loop_limits[loop_depth] = drop().i;
    loop_starts[loop_depth] = ip;
  }
};

void loop(void) {
  if(++loop_counters[loop_depth] < loop_limits[loop_depth]) {
    ip = loop_starts[loop_depth];
  } else {
    if(--loop_depth < 0) {
      out("do/loop mismatch");
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
    out("do too nested");
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
  define(name, VARIABLE, 0);
};

void allot(void) {
  int size = drop().i;
  cell array = {.c = malloc(size * sizeof(cell))};

  for(int i = 0; i < size; i++) {
    *(array.c + i) = drop();
  }
  word();
  char * name = drop().s;
  define_constant(name, array.i);
};

void cells(void) {
  unsigned int n = drop().i;
  cell c = {.i = (n * sizeof(cell))};
  push(c);
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
  if(compiling) {
    compiling->prev = cp;
    dp.d++;
    cp = compiling;
    // TODO: this causes all kinds of segfaults
    // realloc(cp->body, (db - (int)cp->body));
    compiling = NULL;
  } else {
    out("not compiling");
  }
};

void add_to_definition(void) {
  *(dp.c) = (cell)drop();
  dp.c++;
  if(dp.i > compiling->body.i + MAX_WORD_SIZE) {
    out("definition too big");
  }
};

void execute(void) {
  dict * entry = drop().d;

  if(compiling && entry->type != IMMEDIATE) {
    db("add "); db(entry->name); db("\n");
    push((cell)entry);
    add_to_definition();
  } else if(entry->type == PRIMITIVE || entry->type == IMMEDIATE) {
    db("prim "); db(entry->name); db("\n");
    void (*primitive)(void) = entry->body.v;
    (*primitive)();
  } else if(entry->type == COLON) {
    db("colon "); db(entry->name); db("\n");
    run_body(entry);
  } else if(entry->type == CONSTANT) {
    db("const "); db(entry->name); db("\n");
    push(entry->body);
  } else if(entry->type == VARIABLE) {
    db("var "); db(entry->name); db("\n");
    push((cell)&entry->body);
  } else {
    out(entry->name);
    out(" unknown type\n");
  }
};

void interpret(void) {
  // dot_s();
  word();

  dup(); // keep it around in case it's a number
  find();
  dict * entry = drop().d;

  if(entry) {
    db("entry ");
    free(drop().s);
    push((cell)entry);
    execute();
  } else {
    if(compiling) {
      to_number();
      if(!drop().i) {
        out(" compiling unknown thingy\n");
      } else {
        push((cell)"literal");
        find();
        add_to_definition();
        add_to_definition();
      }
    } else {
      to_number();
      if(!drop().i) {
        out("unknown thingy\n");
      } else {
        db("num\n");
      }
    }
  }
};

void exitt(void) {
  exit(0);
};

void noop (void) {};

void comment(void) {
  while(*input != 41) { input++; }
  input++;
};



void primitives (void) {
  stack.v = stack_start.v = malloc(MAX_STACK);

  // define primitives
  define("drop", PRIMITIVE, &drop);
  define("dup", PRIMITIVE, &dup);
  define("swap", PRIMITIVE, &swap);
  define("rot", PRIMITIVE, &rot);
  define("over", PRIMITIVE, &over);
  define("roll", PRIMITIVE, &roll);
  define("pick", PRIMITIVE, &pick);
  define(".s", PRIMITIVE, &dot_s);
  define("@", PRIMITIVE, &fetch);
  define("!", PRIMITIVE, &store);
  define("@c", PRIMITIVE, &fetch_byte);
  define("!c", PRIMITIVE, &store_byte);

  define("+", PRIMITIVE, &plus);
  define("-", PRIMITIVE, &minus);
  define("*", PRIMITIVE, &times);
  define("/", PRIMITIVE, &divided);

  define("or", PRIMITIVE, &orr);
  define("and", PRIMITIVE, &andd);
  define("not", PRIMITIVE, &nott);
  define("=", PRIMITIVE, &equals);
  define(">", PRIMITIVE, &greaterthan);
  define("<", PRIMITIVE, &lessthan);
  define(">>", PRIMITIVE, &right_shift);
  define("<<", PRIMITIVE, &left_shift);
  define("rand", PRIMITIVE, &rrand);

  define("numout", PRIMITIVE, &numout);
  define("cr", PRIMITIVE, &cr);

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
  define("allot", PRIMITIVE, &allot);
  define("cells", PRIMITIVE, &cells);

  define(":", PRIMITIVE, &colon);
  define(";", PRIMITIVE, &semicolon); cp->type = IMMEDIATE;
  define(",", PRIMITIVE, &add_to_definition);

  define("execute", PRIMITIVE, &execute);
  define("interpret", PRIMITIVE, &interpret);
  define("exit", PRIMITIVE, &exitt);

  define("noop", PRIMITIVE, &noop);
  define("(", PRIMITIVE, &comment); cp->type = IMMEDIATE;
  define_constant("debug", (int)&debug);
};

void run(char * s) {
  input = s;
  while(*input) {
    interpret();
  }
};



#ifdef F_CPU
#include "board.h"

void out(char * s) {
  while(*s) { // gotta be a better way for this
    if(*s < 123 && *s > 96) {
      usb_keyboard_press(((*s - 97) + 4) | 0x4000, 0);
    } else if(*s == 32) {
      usb_keyboard_press((uint8_t)KEY_SPACE, 0);
    } else if(*s == 58) {
      usb_keyboard_press((uint8_t)KEY_SEMICOLON, (uint8_t)KEY_LEFT_SHIFT);
    } else if(*s == 48) {
      usb_keyboard_press((uint8_t)KEY_0, 0);
    } else if(*s > 48 && *s < 58) {
      usb_keyboard_press((*s - 19) | 4000, 0);
    }
    s++;
  };
  usb_keyboard_press((uint8_t)KEY_SPACE, 0);
  usb_keyboard_press((uint8_t)KEY_SLASH, 0);
  usb_keyboard_press((uint8_t)KEY_SPACE, 0);
};

void read(void) {
  cell v = { .i = digitalRead(drop().i) };
  push(v);
};

void write(void) {
  digitalWrite(drop().i, drop().i);
};

void delayy(void) {
  delay(drop().i);
};

void debounce_f(void) {
  debounce(drop().i);
};

int main (void) {
  pinMode(13, OUTPUT);
  usb_keyboard_press((uint8_t)KEY_A, 0);
  board_main();
  primitives();

  pinMode(13, OUTPUT);
  pinMode(12, INPUT_PULLUP);
  // define("blink", PRIMITIVE, &blink);
  define("read", PRIMITIVE, &read);
  define("write", PRIMITIVE, &write);
  define("delay", PRIMITIVE, &delayy);

  define("clear", PRIMITIVE, &clear_keys);
  define("debounce", PRIMITIVE, &debounce_f);
  define("preinvoke", PRIMITIVE, &pre_invoke_functions);
  define("presses", PRIMITIVE, &calculate_presses);

  init();

  input = ": m begin clear 20 debounce preinvoke presses usbsend again ; m";
  while(*input) {
    interpret();
  }

  return 0;
};
#else

// for running on a PC
unsigned int keyboard_modifier_keys = 0;
cell * keyboard_keys[6] = {0, 0, 0, 0, 0, 0};

void usbsend(void) {
  out("mods %u pressed: ", keyboard_modifier_keys);
  for(int i = 0; i < 6; i++) {
    out("%u ", keyboard_keys[i]);
  }; out("\n");
};

int main(void) {
  char * init_input = malloc(80);
  primitives();

  // board simulator
  define_constant("pressedkeys", keyboard_keys);
  define_constant("pressedmodifiers", &keyboard_modifier_keys);
  define_constant("onboard", 0);
  define("portb", VARIABLE, 0);
  define("portd", VARIABLE, 0);
  define("portf", VARIABLE, 0);
  define("ddrb", VARIABLE, 0);
  define("ddrd", VARIABLE, 0);
  define("ddrf", VARIABLE, 0);
  define("pind", VARIABLE, 247);
  define("pinf", VARIABLE, 253);
  define("usbsend", PRIMITIVE, &usbsend);

  while(gets(init_input)) {
    input = init_input;
    while(*input) {
      interpret();
    }
    out(" ok\n");
  };
};
#endif
