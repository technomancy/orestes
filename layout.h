#define SHIFT(key)  (MODIFIERKEY_SHIFT + (key))

/* qwerty */

/*
int layer0[44] = {
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_0, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_0, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON,
    KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, MODIFIERKEY_ALT, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH,
    KEY_ESC, KEY_TAB, MODIFIERKEY_GUI, MODIFIERKEY_SHIFT, KEY_BACKSPACE, MODIFIERKEY_CTRL, KEY_SPACE, 201, KEY_MINUS, KEY_QUOTE, KEY_ENTER };

int layer1[44] = {
    SHIFT(KEY_1), SHIFT(KEY_2), SHIFT(KEY_LEFT_BRACE), SHIFT(KEY_RIGHT_BRACE), SHIFT(KEY_BACKSLASH), KEY_0, KEY_PAGE_UP, KEY_7, KEY_8, KEY_9, SHIFT(KEY_8),
    SHIFT(KEY_3), SHIFT(KEY_4), SHIFT(KEY_9), SHIFT(KEY_0), KEY_TILDE, KEY_0, KEY_PAGE_DOWN, KEY_4, KEY_5, KEY_6, SHIFT(KEY_EQUAL),
    SHIFT(KEY_5), SHIFT(KEY_6), KEY_LEFT_BRACE, KEY_RIGHT_BRACE, SHIFT(KEY_TILDE), MODIFIERKEY_ALT, SHIFT(KEY_7), KEY_1, KEY_2, KEY_3, KEY_BACKSLASH,
    112, SHIFT(KEY_INSERT), MODIFIERKEY_GUI, MODIFIERKEY_SHIFT, KEY_BACKSPACE, MODIFIERKEY_CTRL, KEY_SPACE, 201, KEY_PERIOD, KEY_0, KEY_EQUAL };
*/

/* int layer2[44] = { */
/*     KEY_HOME, KEY_UP, KEY_END, KEY_INSERT, KEY_PAGE_UP, KEY_0, KEY_UP, KEY_F7, KEY_F8, KEY_F9, KEY_F10,  */
/*     KEY_LEFT, KEY_DOWN, KEY_RIGHT, KEY_DELETE, KEY_PAGE_DOWN, KEY_0, KEY_DOWN, KEY_F4, KEY_F5, KEY_F6, KEY_F11,  */
/*     0, 0, 0, 0, 0, KEYBOARD_LEFT_ALT, 0, KEY_F1, KEY_F2, KEY_F3, KEY_F12,  */
/*     LAYER(0), 0, KEYBOARD_LEFT_GUI, KEYBOARD_LEFT_SHIFT, KEY_BACKSPACE, KEYBOARD_LEFT_CTRL, KEY_SPACE, PRE_FUNCTION(1), 0, 0, FUNCTION(0) }; */

/* dvorak */

int layer0[44] = {
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_0, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_0, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON,
    KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, MODIFIERKEY_ALT, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH,
    KEY_ESC, KEY_TAB, MODIFIERKEY_GUI, MODIFIERKEY_SHIFT, KEY_BACKSPACE, MODIFIERKEY_CTRL, KEY_SPACE, 201, KEY_QUOTE, KEY_LEFT_BRACE, KEY_ENTER };

int layer1[44] = {
    MODIFIERKEY_SHIFT + KEY_1, MODIFIERKEY_SHIFT + KEY_2, MODIFIERKEY_SHIFT + KEY_MINUS, MODIFIERKEY_SHIFT + KEY_EQUAL, MODIFIERKEY_SHIFT + KEY_BACKSLASH, KEY_0, KEY_PAGE_UP, KEY_7, KEY_8, KEY_9, MODIFIERKEY_SHIFT + KEY_8,
    MODIFIERKEY_SHIFT + KEY_3, MODIFIERKEY_SHIFT + KEY_4, MODIFIERKEY_SHIFT + KEY_9, MODIFIERKEY_SHIFT + KEY_0, KEY_TILDE, KEY_0, KEY_PAGE_DOWN, KEY_4, KEY_5, KEY_6, MODIFIERKEY_SHIFT + KEY_RIGHT_BRACE,
    MODIFIERKEY_SHIFT + KEY_5, MODIFIERKEY_SHIFT + KEY_6, KEY_MINUS, KEY_EQUAL, MODIFIERKEY_SHIFT + KEY_TILDE, MODIFIERKEY_ALT, KEY_BACKSLASH, KEY_1, KEY_2, KEY_3, MODIFIERKEY_SHIFT + KEY_LEFT_BRACE,
    112, MODIFIERKEY_SHIFT + KEY_INSERT, MODIFIERKEY_GUI, MODIFIERKEY_SHIFT, KEY_BACKSPACE, MODIFIERKEY_CTRL, KEY_SPACE, 201, KEY_E, KEY_0, KEY_RIGHT_BRACE };

int layer2[44] = {
    KEY_HOME, KEY_UP, KEY_END, KEY_INSERT, KEY_PAGE_UP, KEY_0, KEY_UP, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
    KEY_LEFT, KEY_DOWN, KEY_RIGHT, KEY_DELETE, KEY_PAGE_DOWN, KEY_0, KEY_DOWN, KEY_F4, KEY_F5, KEY_F6, KEY_F11,
    0, 0, 0, 0, 0, MODIFIERKEY_ALT, 0, KEY_F1, KEY_F2, KEY_F3, KEY_F12,
    0, 0, MODIFIERKEY_GUI, MODIFIERKEY_SHIFT, KEY_BACKSPACE, MODIFIERKEY_CTRL, KEY_SPACE, 201, 0, 0, 110 };

int *layers[] = {layer0, layer1, layer2};

#include "layout_common.h"
