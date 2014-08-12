/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2013 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows 
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "mk20dx128.h"
#include "core_pins.h"
#include "HardwareSerial.h"

// UART0 and UART1 are clocked by F_CPU, UART2 is clocked by F_BUS
// UART0 has 8 byte fifo, UART1 and UART2 have 1 byte buffer

#define TX_BUFFER_SIZE 64
//#define TX_BUFFER_SIZE 40
static volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint8_t tx_buffer_head = 0;
static volatile uint8_t tx_buffer_tail = 0;
static volatile uint8_t transmitting = 0;

#define RX_BUFFER_SIZE 64
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_buffer_head = 0;
static volatile uint8_t rx_buffer_tail = 0;

#define C2_ENABLE		UART_C2_TE | UART_C2_RE | UART_C2_RIE | UART_C2_ILIE
#define C2_TX_ACTIVE		C2_ENABLE | UART_C2_TIE
#define C2_TX_COMPLETING	C2_ENABLE | UART_C2_TCIE
#define C2_TX_INACTIVE		C2_ENABLE

void serial_begin(uint32_t divisor)
{
	SIM_SCGC4 |= SIM_SCGC4_UART0;	// turn on clock, TODO: use bitband
	rx_buffer_head = 0;
	rx_buffer_tail = 0;
	tx_buffer_head = 0;
	tx_buffer_tail = 0;
	transmitting = 0;
	CORE_PIN0_CONFIG = PORT_PCR_PE | PORT_PCR_PS | PORT_PCR_PFE | PORT_PCR_MUX(3);
	CORE_PIN1_CONFIG = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(3);
	UART0_BDH = (divisor >> 13) & 0x1F;
	UART0_BDL = (divisor >> 5) & 0xFF;
	UART0_C4 = divisor & 0x1F;
	//UART0_C1 = 0;
	UART0_C1 = UART_C1_ILT;
	UART0_TWFIFO = 2; // tx watermark, causes S1_TDRE to set
	UART0_RWFIFO = 4; // rx watermark, causes S1_RDRF to set
	UART0_PFIFO = UART_PFIFO_TXFE | UART_PFIFO_RXFE;
	UART0_C2 = C2_TX_INACTIVE;
	NVIC_ENABLE_IRQ(IRQ_UART0_STATUS);
}

void serial_end(void)
{
	if (!(SIM_SCGC4 & SIM_SCGC4_UART0)) return;
	while (transmitting) yield();  // wait for buffered data to send
	NVIC_DISABLE_IRQ(IRQ_UART0_STATUS);
	UART0_C2 = 0;
	CORE_PIN0_CONFIG = PORT_PCR_PE | PORT_PCR_PS | PORT_PCR_MUX(1);
	CORE_PIN1_CONFIG = PORT_PCR_PE | PORT_PCR_PS | PORT_PCR_MUX(1);
	rx_buffer_head = 0;
	rx_buffer_tail = 0;
}

void serial_putchar(uint8_t c)
{
	uint32_t head;

	if (!(SIM_SCGC4 & SIM_SCGC4_UART0)) return;
	head = tx_buffer_head;
	if (++head >= TX_BUFFER_SIZE) head = 0;
	while (tx_buffer_tail == head) {
		yield(); // wait
	}
	tx_buffer[head] = c;
	transmitting = 1;
	tx_buffer_head = head;
	UART0_C2 = C2_TX_ACTIVE;
}

void serial_write(const void *buf, unsigned int count)
{
	const uint8_t *p = (const uint8_t *)buf;
	const uint8_t *end = p + count;
        uint32_t head;

        if (!(SIM_SCGC4 & SIM_SCGC4_UART0)) return;
	while (p < end) {
        	head = tx_buffer_head;
        	if (++head >= TX_BUFFER_SIZE) head = 0;
		if (tx_buffer_tail == head) {
        		UART0_C2 = C2_TX_ACTIVE;
			do {
				yield(); // wait
			} while (tx_buffer_tail == head);
		}
        	tx_buffer[head] = *p++;
        	transmitting = 1;
        	tx_buffer_head = head;
	}
        UART0_C2 = C2_TX_ACTIVE;
}

void serial_flush(void)
{
	while (transmitting) yield(); // wait
}

int serial_available(void)
{
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head >= tail) return head - tail;
	return RX_BUFFER_SIZE + head - tail;
}

int serial_getchar(void)
{
	uint8_t head, tail;
	int c;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head == tail) return -1;
	if (++tail >= RX_BUFFER_SIZE) tail = 0;
	c = rx_buffer[tail];
	rx_buffer_tail = tail;
	return c;
}

int serial_peek(void)
{
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head == tail) return -1;
	return rx_buffer[tail];
}

void serial_clear(void)
{
	if (!(SIM_SCGC4 & SIM_SCGC4_UART0)) return;
	UART0_C2 &= ~(UART_C2_RE | UART_C2_RIE | UART_C2_ILIE);
	UART0_CFIFO = UART_CFIFO_RXFLUSH;
	UART0_C2 |= (UART_C2_RE | UART_C2_RIE | UART_C2_ILIE);
	rx_buffer_head = rx_buffer_tail;
}

// status interrupt combines 
//   Transmit data below watermark  UART_S1_TDRE
//   Transmit complete              UART_S1_TC
//   Idle line                      UART_S1_IDLE
//   Receive data above watermark   UART_S1_RDRF
//   LIN break detect               UART_S2_LBKDIF
//   RxD pin active edge            UART_S2_RXEDGIF

void uart0_status_isr(void)
{
	uint8_t avail, head, newhead, tail, c;

	if (UART0_S1 & (UART_S1_RDRF | UART_S1_IDLE)) {
		__disable_irq();
		avail = UART0_RCFIFO;
		if (avail == 0) {
			// The only way to clear the IDLE interrupt flag is
			// to read the data register.  But reading with no
			// data causes a FIFO underrun, which causes the
			// FIFO to return corrupted data.  If anyone from
			// Freescale reads this, what a poor design!  There
			// write should be a write-1-to-clear for IDLE.
			c = UART0_D;
			// flushing the fifo recovers from the underrun,
			// but there's a possible race condition where a
			// new character could be received between reading
			// RCFIFO == 0 and flushing the FIFO.  To minimize
			// the chance, interrupts are disabled so a higher
			// priority interrupt (hopefully) doesn't delay.
			// TODO: change this to disabling the IDLE interrupt
			// which won't be simple, since we already manage
			// which transmit interrupts are enabled.
			UART0_CFIFO = UART_CFIFO_RXFLUSH;
			__enable_irq();
		} else {
			__enable_irq();
			head = rx_buffer_head;
			tail = rx_buffer_tail;
			do {
				c = UART0_D;
				newhead = head + 1;
				if (newhead >= RX_BUFFER_SIZE) newhead = 0;
				if (newhead != tail) {
					head = newhead;
					rx_buffer[head] = c;
				}
			} while (--avail > 0);
			rx_buffer_head = head;
		}
	}
	c = UART0_C2;
	if ((c & UART_C2_TIE) && (UART0_S1 & UART_S1_TDRE)) {
		head = tx_buffer_head;
		tail = tx_buffer_tail;
		do {
			if (tail == head) break;
			if (++tail >= TX_BUFFER_SIZE) tail = 0;
			avail = UART0_S1;
			UART0_D = tx_buffer[tail];
		} while (UART0_TCFIFO < 8);
		tx_buffer_tail = tail;
		if (UART0_S1 & UART_S1_TDRE) UART0_C2 = C2_TX_COMPLETING;
	}
	if ((c & UART_C2_TCIE) && (UART0_S1 & UART_S1_TC)) {
		transmitting = 0;
		UART0_C2 = C2_TX_INACTIVE;
	}
}



void serial_print(const char *p)
{
	while (*p) {
		char c = *p++;
		if (c == '\n') serial_putchar('\r');
		serial_putchar(c);
	}
}

static void serial_phex1(uint32_t n)
{
	n &= 15;
	if (n < 10) {
		serial_putchar('0' + n);
	} else {
		serial_putchar('A' - 10 + n);
	}
}

void serial_phex(uint32_t n)
{
	serial_phex1(n >> 4);
	serial_phex1(n);
}

void serial_phex16(uint32_t n)
{
	serial_phex(n >> 8);
	serial_phex(n);
}

void serial_phex32(uint32_t n)
{
	serial_phex(n >> 24);
	serial_phex(n >> 16);
	serial_phex(n >> 8);
	serial_phex(n);
}

