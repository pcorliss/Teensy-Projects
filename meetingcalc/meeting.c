#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include "usb_debug_only.h"
#include "print.h"
#include <avr/interrupt.h>

// Teensy 1.0: LED is active low
#define LED_ON	(PORTD &= ~(1<<6))
#define LED_OFF	(PORTD |= (1<<6))
#define LED_T (PORTD ^= (1<<6))

#define LED_CONFIG	(DDRD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))
#define DIT 80		/* unit time for morse code */


volatile uint32_t ticks; //(Roughly .25 seconds, but close enough)

char getKeyPress(void);
char waitKeyPress(void);

#define KEYPAD_COL 3
#define KEYPAD_ROW 4

const char charArr[KEYPAD_ROW][KEYPAD_COL];
const unsigned char segment_table[18];
void displayClear(void);
void displayNum(unsigned int num);
void printStr(char *buf);
void printInt(uint32_t num);
int nthdigit(uint32_t x, int n);
void displayMulti(uint32_t num, char special);
void displayMultiF(float numF);
uint8_t getPeople(void);
uint8_t getHourly(void);

int main(void){
	// set for 16 MHz clock
	CPU_PRESCALE(0);

	usb_init();
	
	//D0->4 Output Bits for Transistors to individual displays
	DDRD =  0b01011111;
	PORTD = 0b00000000;
	
	//C0->7 Initially setting bits to input. Will set to output to turn on.
	DDRC =  0b00000000;
	PORTC = 0b00000000;
	//DDRC =  0b11111111;
	
	DDRF =  0b00000000;
  PORTF = 0b11101010;
	
	TCCR1B = 0b00000011;
	TIFR1 =  0b00000001;
  TIMSK1 = 0b00000001;
  
  //sei();

  //Turn on one of the columns for interupt purposes.
  PORTF &= ~(1<<2); //Setting Low
  DDRF |= (1<<2); //Set to Output
  
  uint8_t peeps = 0;
  uint16_t hourly = 0;
  
  peeps = getPeople();
  hourly = getHourly();
      
	while (1) {
    //.26 ticks per second, we'll say .25 for simplicity.
    displayMultiF((float)peeps*(float)hourly*ticks/60.0f/60.0f/4.0f);
	}
}

#define MAX_PEEPS 4

uint8_t getPeople(void){
  char c;
  char buf[MAX_PEEPS];
  uint8_t buf_count = 0;
  while(1){
    displayMulti(atoi(buf),'P');
    if(c = waitKeyPress()){
      usb_debug_putchar(c);
      if(c == '*'){
        memset(buf, 0, MAX_PEEPS);
        buf_count = 0;
      } else if(c == '#'){
        return atoi(buf);
      } else {
        buf[buf_count] = c;
        buf_count++;
        if (buf_count == MAX_PEEPS){
          return atoi(buf);
        }
      }
    }
  }
}

#define MAX_HOURLY 5

uint8_t getHourly(void){
  char c;
  char buf[MAX_HOURLY];
  uint8_t buf_count = 0;
  while(1){
    displayMulti(atoi(buf),'H');
    if(c = waitKeyPress()){
      usb_debug_putchar(c);
      if(c == '*'){
        memset(buf, 0, MAX_HOURLY);
        buf_count = 0;
      } else if(c == '#'){
        return atoi(buf);
      } else {
        buf[buf_count] = c;
        buf_count++;
        if (buf_count == MAX_HOURLY){
          return atoi(buf);
        }
      }
    }
  }
}

ISR(TIMER1_OVF_vect)
{
   //This is the interrupt service routine for TIMER0 OVERFLOW Interrupt.
   //CPU automatically call this when TIMER0 overflows.

   //Increment our variable
   ticks++;
}


void displayMultiF(float numF){
  //Pin 0 is actually 10^5 place
  //Round to 2 decimals
  uint32_t num = (uint32_t)(numF*100+.5);
  unsigned int decimal = 1;
  if(num >= 100000){
    num = num/100;
    decimal = 0;
  }
  for(int i=0;i<5;i++){
    displayNum(nthdigit(num,4-i));
    if(i==2 && decimal == 1){
      DDRC |= (1<<7);
    }
    PORTD |= (1<<i);
    _delay_ms(1);
    PORTD &= ~(1<<i);
    displayClear();
  }

}

void displayMulti(uint32_t num,char special){
  //Pin 0 is actually 10^5 place
  for(int i=0;i<5;i++){
    if(i==0 && special){
      if(special == 'H'){
        displayNum(16);
      } else if(special == 'P'){
        displayNum(17);
      }
    } else {
      displayNum(nthdigit(num,4-i));
    }
    PORTD |= (1<<i);
    _delay_ms(1);
    PORTD &= ~(1<<i);
    displayClear();
  }
}

int nthdigit(uint32_t x, int n){
    while (n--) {
        x /= 10;
    }
    return x % 10;
}

void printInt(uint32_t num){
  char buf[10];
  itoa(num,buf,10);
  printStr(buf);
}

void printStr(char *buf){
  char c;
  for(int i=0;c = buf[i];i++){
    pchar(buf[i]);
  }
}

void displayNum(unsigned int num){
  if(num > 18){
    print("Illegal Access:");
    printInt(num);
    print("\n");
    displayClear();
  } else {
    DDRC = segment_table[num];
  }
}
void displayClear(void){
  DDRC = 0b00000000;
}


const unsigned char segment_table[18] = {
  0b01111110, //0
  0b01001000, //1
  0b00111101, //2
  0b01101101, //3
  0b01001011, //4
  0b01100111, //5
  0b01110111, //6
  0b01001100, //7
  0b01111111, //8
  0b01101111, //9
  0b01011111, //A
  0b01110011, //b
  0b00110001, //c
  0b01111001, //d
  0b00110111, //E
  0b00010111, //F
  0b11011011, //H.
  0b10011111  //P.
};



const uint8_t colArr[KEYPAD_COL] = {2,0,4};
const uint8_t rowArr[KEYPAD_ROW] = {1,6,5,3};
    
const char charArr[KEYPAD_ROW][KEYPAD_COL] = {
      {'1','2','3'},
      {'4','5','6'},
      {'7','8','9'},
      {'*','0','#'}
    };

char waitKeyPress(void){
  char c = getKeyPress();
  uint16_t counter = 50;
  //_delay_ms(50);
  while(c != 0 && c == getKeyPress() && counter < 1000){
    _delay_ms(10);
    counter += 10;
  }
  return c;
}

char getKeyPress(void){
    uint8_t j = 0;
    uint8_t k = 0;
    for(j=0;j<KEYPAD_COL;j++){
      uint8_t pin = colArr[j];
      PORTF &= ~(1<<pin); //Setting Low
      DDRF |= (1<<pin); //Set to Output
      for(k=0;k<KEYPAD_ROW;k++){
        if (! (PINF & (1<<rowArr[k]))) {
          DDRF &= ~(1<<pin); //Set to Input
          PORTF |= (1<<pin); //Setting High
          return charArr[k][j];
        }
      }
      DDRF &= ~(1<<pin); //Set to Input
      PORTF |= (1<<pin); //Setting High
    }
    return 0;
}

