#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>

//Use UTF-8.
//F_CPU должен быть задан в параметрах компилятора.

#define UART_puts_P(__strP) UART_puts_p(PSTR(__strP)) //Вывод строк напрямую из памяти прошивки, минуя стек.
#define DEF_OCR2A_MICROS (F_CPU/500000 - 1) / 2 //Значение OCR2A для micros.

typedef struct {
  uint32_t micros;
  uint8_t data;
  uint8_t notSent;
} ADC_result;

static volatile uint32_t micros_timer;
static volatile ADC_result adc_result;
static char str_buf[32];

ISR (TIMER2_COMPA_vect) {
   uint32_t m = micros_timer;
   ++m;
   micros_timer = m;
}

ISR(ADC_vect)
{
  //Здесь можем смело читать micros_timer, не запрещая прерывания
  adc_result.micros = micros_timer;
  UART_puts_P("Read!\n");
  adc_result.data = ADCH;
  adc_result.notSent = 1;
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

void ADC_Init(void) {
  
  ADCSRA |= (1<<ADEN) // Разрешение использования АЦП. Пока не включаем.
  |(1 << ADATE) //ADC Auto Trigger Enable
  |(1 << ADIE) //Разрешаем прерывания для АЦП
  
  |(1<<ADPS2)|(0<<ADPS1)|(0<<ADPS0);//Делитель 16 = 1 МГц
  
  ADMUX = 0; //Выберем ADC0 канал
  ADMUX |= (0<<REFS1)|(1<<REFS0); //Используем напряжение питания, на AREF есть конденсатор.
  ADMUX |= (1 << ADLAR); //Меняем порядок байтов результата АЦП.
  
  ADCSRB = 0x00; //режим Free Running
  //ADCSRA |= (1<<ADEN); //Включили АЦП.
  ADCSRA |= (1<<ADSC); //Начинаем преобразование
}

int main() {
  DDRC = 0x00; //Весь PORTC в режиме входа.
  micros_timer = 0;
  adc_result.micros = 0;
  adc_result.data = 0;
  adc_result.notSent = 0;
  UART_Init();
  ADC_Init();
  timer_init();
  
  
  UART_puts_P("Hello!\n");
  while(1) {
    ltoa(micros(), str_buf, 10);
    UART_puts(str_buf);
    UART_putc('\n');
    if (adc_result.notSent) {
      uint8_t oldSREG = SREG;
      cli();
      adc_result.notSent = 0;
      ADC_result adc_result_c = adc_result;
      SREG = oldSREG; 
      itoa(adc_result_c.micros, str_buf, 10);
      UART_puts(str_buf);
      UART_puts_P("    ");
      itoa(adc_result_c.data, str_buf, 10);
      UART_puts(str_buf);
      UART_putc('\n');
    }
  }
}
