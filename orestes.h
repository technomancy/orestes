char * input;

enum entry_type {
  PRIMITIVE,
  IMMEDIATE,
  CONSTANT,
  VARIABLE,
  COLON
};

void primitives(void);
void interpret(void);
void define(char * name, enum entry_type type, void * body);
