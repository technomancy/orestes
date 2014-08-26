#include <stdlib.h>
#include <string.h>

#include "teensy3/usb_keyboard.h"
#include "teensy3/core_pins.h"

#include "board.h"

#define DEBOUNCE_PASSES 10


// Layout setup

void reset(void);

// set this for layer changes that need to persist beyond one cycle
int current_layer_number = 0;
// this gets reset every cycle
int *current_layer;

int pressed_count = 0;
int presses[KEY_COUNT];
int last_pressed_count = 0;
int last_presses[KEY_COUNT];

int rows_pinout[] = {14, 15, 20, 21};
int cols_pinout[] = {12, 11, 8, 7, 6, 5, 4, 3, 2, 1, 0};

// layout.h must define:
// * layers: array of int[KEY_COUNT]
// * layer_functions: array of void function pointers
// ... plus any functions included in layer_functions
// per_cycle void function callback
#include "layout.h"


// Matrix scanning logic

void record(int col, int row) {
  presses[pressed_count++] = (row * COL_COUNT) + col;
};

void activate_row(int row) {
  for(int i = 0; i < ROW_COUNT; i++) {
    digitalWrite(rows_pinout[i], !(i == row));
  };
  delay(1);
};

void scan_row(int row) {
  for(int col = 0; col < COL_COUNT; col++) {
    if(digitalRead(cols_pinout[col])) {
      record(col, row);
    }
  }
};

void scan_rows() {
  pressed_count = 0;
  for(int i = 0; i < ROW_COUNT; i++) {
    activate_row(i);
    scan_row(i);
  };
};


// Cycle functions

void debounce(int passes_remaining) {
  while(passes_remaining) {
    last_pressed_count = pressed_count;
    for(int i = 0; i < last_pressed_count; i++) {
      last_presses[i] = presses[i];
    }

    scan_rows();

    if((pressed_count != last_pressed_count) || \
       memcmp(presses, last_presses, pressed_count)) {
      passes_remaining = DEBOUNCE_PASSES;
    } else {
      passes_remaining--;
    }
  }
};

void pre_invoke_functions() {
  for(int i = 0; i < pressed_count; i++) {
    int keycode = current_layer[presses[i]];
    if(keycode >= 200 && keycode < 300) {
      (layer_functions[keycode - 200])();
    }
  }
  per_cycle();
};

void calculate_presses() {
  int usb_presses = 0;
  for(int i = 0; i < pressed_count; i++) {
    int keycode = current_layer[presses[i]];
    if(keycode >= 110 && keycode < 136) {
      // regular layout functions
      (layer_functions[keycode - 110])();
    } else if(keycode >= 200 && keycode < 255) {
      // pre-invoke functions have already been processed
    } else if(keycode >= 136 && keycode < 200) {
      // layer set
      current_layer_number = keycode - 136;
    } else if(keycode > 100 && keycode <= 108) {
      // modifier
      keyboard_modifier_keys |= (keycode - 100);
    } else if(keycode > 255 && usb_presses < 6) {
      // modifier plus keypress
      keyboard_modifier_keys |= (keycode >> 8);
      keyboard_keys[usb_presses++] = (keycode & 255);
    } else if(usb_presses < 6){
      // keypress
      keyboard_keys[usb_presses++] = keycode;
    };
  };
};


// Top level stuff

void clear_keys() {
  current_layer = layers[current_layer_number];
  keyboard_modifier_keys = 0;
  for(int i = 0; i < 6; i++) {
    keyboard_keys[i] = 0;
  };
};

void init() {
  for(int i = 0; i < ROW_COUNT; i++) {
    pinMode(rows_pinout[i], OUTPUT);
  }
  for(int i = 0; i < COL_COUNT; i++) {
    pinMode(cols_pinout[i], INPUT_PULLUP);
  }
};

/* int main() { */
/*   init(); */
/*   while(1) { */
/*     clear_keys(); */
/*     debounce(DEBOUNCE_PASSES); */
/*     pre_invoke_functions(); */
/*     calculate_presses(); */
/*     usb_keyboard_send(); */
/*   }; */
/* }; */

void reset(void) {
  // TODO
};
