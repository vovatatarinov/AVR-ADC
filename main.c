#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>

//Use UTF-8.
//F_CPU должен быть задан в параметрах компилятора.

#define UART_puts_P(__strP) UART_puts_p(PSTR(__strP)) //Вывод строк напрямую из памяти прошивки, минуя стек.

typedef struct {
  uint32_t micros;
  uint8_t data;
  uint8_t notSent;
} ADC_result;

static volatile uint32_t micros_timer;
static ADC_result adc_result;
static char str_buf[32];

ISR (TIMER2_OVF_vect) {
   uint32_t m = micros_timer;
   ++m;
   micros_timer = m;
}

ISR(ADC_vect)
{
  //Здесь можем смело читать micros_timer, не запрещая прерывания
  uint8_t getTCNT2 = TCNT2;
  getTCNT2 >>= 1; // Делим на 2
  //micros_timer нужно сдвинуть на байт влево, а затем вернуть на бит вправо для деления на 2
  adc_result.micros = (micros_timer << 7) + getTCNT2;
  adc_result.data = ADCH;
  adc_result.notSent = 1;
}

void timer_init(void) {
   cli();
  //Timer2 Init
  //Считать время в микросекундах, вызывая прервания 
  //каждую микросекунду - плохая идея.
  //Микроконтроллер будет все время занят подсчетом количества микросекунд
  //Заметим, что регистр TCNT2 предоставляет прямой доступ, как для чтения, 
  //так и для записи, к 8-разрядному счетчику.
  //Его можно считать дополнительным байтом. 
  //Количество микросекунд будем находить сдвигом счетчика переполнения
  //таймера на 8 бит влево,
  //а затем прибавлять значение регистра TCNT2.
  //Необходимое значение делителя равно 8.
  //Получим количество микросекунд, умноженное на 2
   micros_timer = 0;
   TCCR2B = 0b00000010;  // x8
   TCCR2A = 0b00000000;  //Normal mode
   TIMSK2 |= (1<<TOIE2);
   sei();
}

void UART_Init() {
  /*Set baud rate */
  UBRR0H = 0;
  UBRR0L = 1; /* Здесь вынужден ставить скорость в 1 мегабод */
  /* Мой CH340G не захотел работать со скоростью 2 Mbps */
  /* Однако, в документации к этой микросхеме сказано, */
  /* что поддерживается скорость до 2 Mbps */
  /* Проверено, что при использовании ATmega16U2 в качестве USB - UART преобразователя */
  /* Можно использовать скорость 2 Mbps */
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
  cli();
  ADCSRA |= (1<<ADEN) // Разрешение использования АЦП. Пока не включаем.
  |(1 << ADATE) //ADC Auto Trigger Enable
  |(1 << ADIE) //Разрешаем прерывания для АЦП
  
  |(1<<ADPS2)|(0<<ADPS1)|(0<<ADPS0);//Делитель 16 = 1 МГц
  
  ADMUX = 0; //Выберем ADC0 канал
  ADMUX |= (0<<REFS1)|(1<<REFS0); //Используем напряжение питания, на AREF есть конденсатор.
  ADMUX |= (1 << ADLAR); //Меняем порядок байтов результата АЦП.
  
  ADCSRB = 0x00; //режим Free Running
  ADCSRA |= (1<<ADEN); //Включили АЦП.
  sei();
  ADCSRA |= (1<<ADSC); //Начинаем преобразование
}

int main() {
  DDRC = 0x00; //Весь PORTC в режиме входа.
  micros_timer = 0;
  adc_result.micros = 0;
  adc_result.data = 0;
  adc_result.notSent = 0;
  UART_Init();
  timer_init(); 
  ADC_Init();
  UART_puts_P("Hello!\n");
  while(1) {
    if (adc_result.notSent) {
      adc_result.notSent = 0;
      cli();
      ADC_result adc_result_c = adc_result;
      sei();
      ltoa(adc_result_c.micros, str_buf, 10);
      UART_puts(str_buf);
      UART_puts_P("    ");
      itoa(adc_result_c.data, str_buf, 10);
      UART_puts(str_buf);
      UART_putc('\n');
    }
  }
}
