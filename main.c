#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>

//Use UTF-8.
//F_CPU должен быть задан в параметрах компилятора.

#define UART_puts_P(__strP) UART_puts_p(PSTR(__strP)) //Вывод строк напрямую из памяти прошивки, минуя стек.

typedef struct {
  uint32_t micros;
  uint8_t data;
} ADC_result;


void UART_Init() {
  /*Set baud rate */
  UBRR0H = 0;
  UBRR0L = 0; /* Set maximum speed */
  /*Set UCSR0A*/
  UCSR0A=0b00000000;
  UCSR0A |= (1 << U2X0); //set x2 speed for decreasing clock errors
  //Maximum clocking error is -3.5% for 230.4k baud rate for Fosc =  16.0000MHz (Arduino default oscillator)
  //You can set baud rate up to 2 Mbps with Fosc =  16.0000MHz
  /*Enable receiver and transmitter */
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  /* Set frame format: 8N1 */
  UCSR0C = 0b00000110;
}

void UART_putc(unsigned char data) {
    // wait for transmit buffer to be empty
    if (data == '\n')
        UART_putc('\r'); //Windows (DOS)-like line ending
    while(!(UCSR0A & (1 << UDRE0)));
    // load data into transmit register
    UDR0 = data;
}

void UART_puts(const char* s) {
    // transmit character until NULL is reached
    while(*s != 0)
      UART_putc(*s++);
}

void UART_puts_p(const char* s) {
    // transmit character until NULL is reached
    while(pgm_read_byte(s) != 0)
      UART_putc(pgm_read_byte(s++));
}

unsigned char UART_getc(void) {
    // wait for data
    while(!(UCSR0A & (1 << RXC0)));
    // return data
    return UDR0;
}

void UART_Flush( void ) {
  unsigned volatile char dummy;
  _delay_ms(1);
  while ( UCSR0A & (1<<RXC0) ) { dummy = UDR0; _delay_ms(1); }
  dummy += 0; //Warning
}

void UART_getLine(char* buf, uint8_t n) {
    uint8_t bufIdx = 0;
    char c;
    // while received character is not carriage return or newline symbol
    // and end of buffer has not been reached
    do
    {
        // receive character
        c = UART_getc();
        // store character in buffer
        buf[bufIdx++] = c;
    }
    while((bufIdx < n) && (c != '\r') && (c != '\n'));
    // ensure buffer is null terminated
    buf[bufIdx] = 0;
}


int main() {
  DDRC = 0x00; //Весь PORTC в режиме входа.
  UART_Init();
  
  while(1) {
    
  }
}
