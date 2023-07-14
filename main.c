#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

//Use UTF-8.
//F_CPU должен быть задан в параметрах компилятора.

#define UART_puts_P(__strP) UART_puts_p(PSTR(__strP)) //Вывод строк напрямую из памяти прошивки, минуя стек.
#define DEF_OCR2A_MICROS (F_CPU/500000 - 1) / 2 //Значение OCR2A для micros.

typedef struct {
  uint32_t micros;
  uint8_t data;
} ADC_result;

static volatile uint32_t micros_timer;

ISR (TIMER2_COMPA_vect) {
   uint32_t m = micros_timer;
   ++m;
   micros_timer = m;
}

void timer_init(void) {
   cli();
   //Timer2 Init
   micros_timer = 0;
   TCCR2B = 0b00000001;  // x1
   TCCR2A = 0b00000010;  //CTC mode
   TIMSK2 |= (1<<OCIE2A);
   OCR2A = DEF_OCR2A_MICROS; // F_CPU / (Prescaler * (OCR2A + 1) ) = 1000000. Toogle interrupt at every microsecond.
   sei();
}


uint32_t micros() {
  uint32_t m;
  uint8_t oldSREG = SREG;

  //Отключим прерывания, иначе можем получить не то значение. (Прерывание во время чтения переменной)
  cli();
  m = micros_timer;
  SREG = oldSREG; //Верните на место старый SREG... Мы не знаем, были ли включены прерывания до вызова функции.

  return m;
}

void UART_Init() {
  /*Set baud rate */
  UBRR0H = 0;
  UBRR0L = 0; /* Set maximum speed */
  /*Set UCSR0A*/
  UCSR0A=0b00000000;
  UCSR0A |= (1 << U2X0); //set x2 speed for decreasing clock errors
  //Maximum clocking error is -3.5% for 230.4k baud rate for Fosc =  16.0000MHz (Arduino default oscillator)
  //You can set baud rate up to 2 Mbps with Fosc =  16.0000MHz
  /*Disable receiver. Enable transmitter */
  UCSR0B = (0<<RXEN0) | (1<<TXEN0);
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


int main() {
  DDRC = 0x00; //Весь PORTC в режиме входа.
  micros_timer = 0;
  UART_Init();
  timer_init();
  
  while(1) {
    
  }
}
