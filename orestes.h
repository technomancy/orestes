char * input;

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

void primitives(void);
void interpret(void);
void define(char * name, enum entry_type type, void * body);
void define_constant(char * name, int value);
cell drop(void);
