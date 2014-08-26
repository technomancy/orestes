#define ROW_COUNT 4
#define COL_COUNT 11
#define KEY_COUNT ROW_COUNT*COL_COUNT

void init();
void clear_keys();
void debounce(int passes_remaining);
void pre_invoke_functions();
void calculate_presses();
