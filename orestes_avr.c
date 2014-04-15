#include <avr/io.h>
#include <util/delay.h>

#include "usb_keyboard.h"
#include "orestes.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

void reset(void) {
  UDCON = 1;
  USBCON = (1<<FRZCLK);
  UCSR1B = 0;
  _delay_ms(5);
  EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
  TIMSK0 = 0; TIMSK1 = 0; TIMSK3 = 0; TIMSK4 = 0; UCSR1B = 0; TWCR = 0;
  DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0; TWCR = 0;
  PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
  asm volatile("jmp 0x7E00");
};

// delay only works with compile-time args; what the heck
void delay(void) {
  _delay_ms(100);
};

void delaysec(void) {
  _delay_ms(1000);
};

void delayten(void) {
  _delay_ms(10);
};

void blink(void) {
  (PORTD |= (1<<6));
  _delay_ms(100);
  (PORTD &= ~(1<<6));
  _delay_ms(100);
};

void blinq(void) {
  (PORTD |= (1<<6));
  _delay_ms(50);
  (PORTD &= ~(1<<6));
  _delay_ms(50);
};

void blinks(void) {
  (PORTD |= (1<<6));
  delaysec();
  (PORTD &= ~(1<<6));
  delaysec();
};

void out(char * s) {
  while(*s) { // gotta be a better way for this
    if(*s < 123 && *s > 96) {
      usb_keyboard_press((*s - 97) + 4, 0);
    } else if(*s == 32) {
      usb_keyboard_press(KEY_SPACE, 0);
    } else if(*s == 58) {
      usb_keyboard_press(KEY_SEMICOLON, KEY_SHIFT);
    } else if(*s > 48 && *s < 58) {
      usb_keyboard_press((*s - 19), 0);
    }
    s++;
  };
  usb_keyboard_press(KEY_SPACE, 0);
  usb_keyboard_press(KEY_SLASH, 0);
  usb_keyboard_press(KEY_SPACE, 0);
};

void send(void) {
  usb_keyboard_send();
};

void run(char * s) {
  input = s;
  while(*input) {
    interpret();
  }
};

int main (void) {
  primitives();

  CPU_PRESCALE(0);
  usb_init();
  while (!usb_configured()) /* wait */ ;
  (DDRD |= (1<<6));

  /* define("portb", CONSTANT, (int)PORTB); */
  /* define("portc", CONSTANT, (int)PORTC); */
  /* define("portd", CONSTANT, (int)PORTD); */

  define("keys", VARIABLE, &keyboard_keys);
  define("modifiers", VARIABLE, &keyboard_modifier_keys);
  define("send", PRIMITIVE, &send);

  define("reset", PRIMITIVE, &reset);
  define("blink", PRIMITIVE, &blink);
  define("blinks", PRIMITIVE, &blinks);
  define("blinq", PRIMITIVE, &blinq);

  define("delaysec", PRIMITIVE, &delaysec);
  define("delay", PRIMITIVE, &delay);
  define("delayten", PRIMITIVE, &delayten);

  delaysec();

  char * code = ": b do blinks loop ; 4 0 b reset ";
  run(code);

  delaysec();
  blinq(); blinq(); blinq();
  reset();
};
