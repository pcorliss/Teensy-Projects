#include <inttypes.h> 
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_debug_only.h"
#include "print.h"


// Teensy 2.0: LED is active high
#if defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__)
#define LED_ON		(PORTD |= (1<<6))
#define LED_OFF		(PORTD &= ~(1<<6))

// Teensy 1.0: LED is active low
#else
#define LED_ON	(PORTD &= ~(1<<6))
#define LED_OFF	(PORTD |= (1<<6))
#endif

#define LED_CONFIG	(DDRD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))
#define DIT 80		/* unit time for morse code */

#define LED1_GREEN (PORTB |= (1<<0))
#define LED1_RED (PORTB |= (1<<1))
#define LED2_GREEN (PORTE |= (1<<7))
#define LED2_RED (PORTE |= (1<<6))
#define LED3_GREEN (PORTB |= (1<<2))
#define LED3_RED (PORTB |= (1<<3))

#define LED1_GREEN_OFF (PORTB &= ~(1<<0))
#define LED1_RED_OFF (PORTB &= ~(1<<1))
#define LED2_GREEN_OFF (PORTE &= ~(1<<7))
#define LED2_RED_OFF (PORTE &= ~(1<<6))
#define LED3_GREEN_OFF (PORTB &= ~(1<<2))
#define LED3_RED_OFF (PORTB &= ~(1<<3))

#define LED1_GREEN_T (PORTB ^= (1<<0))
#define LED1_RED_T (PORTB ^= (1<<1))
#define LED2_GREEN_T (PORTE ^= (1<<7))
#define LED2_RED_T (PORTE ^= (1<<6))
#define LED3_GREEN_T (PORTB ^= (1<<2))
#define LED3_RED_T (PORTB ^= (1<<3))

#define OFF 0
#define GREEN 1
#define RED 2
#define BLINK_RED 3
#define BLINK_GREEN 4
#define RED_GREEN 5
#define TEMP_BLINK_RED 6
#define TEMP_BLINK_GREEN 7
#define ONE_BLINK_GREEN 8
#define ONE_BLINK_RED 9


#define MAX_CODE 10

char getKeyPress(void);
char waitKeyPress(void);
void resetAll(void);

int main(void)
{

	// set for 16 MHz clock, and make sure the LED is off
	CPU_PRESCALE(0);

  DDRB |= (1<<0);
  DDRB |= (1<<1);
  DDRB |= (1<<2);
  DDRB |= (1<<3);
  
  DDRE |= (1<<6);
  DDRE |= (1<<7);
  
  //All C ports as input all as pullup resistors
  DDRC =  0b00000000;
  PORTC = 0b11111111;
  
  /*
  F
  0 - Col 1 - Output 
  1 - Row 0 - Input
  2 - Col 0 - Output
  3 - Row 3 - Input
  4 - Col 2 - Output
  5 - Row 2 - Input
  6 - Row 1 - Input
  */
  
  DDRF =  0b00000000;
  PORTF = 0b11101010;
  
	usb_init();
	
	char code[MAX_CODE] = {'1','2','3','4'};
	
	char pass[MAX_CODE];
	memset(pass, 0, MAX_CODE);
	
	uint16_t ticks = 0;
	
  uint8_t keypad = 0;
  uint8_t safety = 0;
  uint8_t bigred = 0;
  
	while (1) {
    int8_t j = 0;

    if(PINC & (1<<0) && keypad && !safety){
      safety = 1;
      ticks = 0;
      LED2_RED_OFF;
      LED2_GREEN_OFF;
      while(ticks <= 10){
        _delay_ms(50);
        if(ticks % 5 == 0){
          LED2_GREEN_T;
        }
        ticks++;
      }
    } else if(PINC & (1<<0) && keypad){
      //safety = 1;
    } else if(PINC & (1<<0)){
      safety = 0;
      print("Resetting All");
      resetAll();
    } else {
      //safety = 0;
    }
    
    if(PINC & (1<<1) && keypad && safety && !bigred){
      bigred = 1;
      ticks = 0;
      LED3_RED_OFF;
      LED3_GREEN_OFF;
      while(ticks <= 10){
        _delay_ms(50);
        if(ticks % 5 == 0){
          LED3_GREEN_T;
        }
        ticks++;
      }
    } else if(PINC & (1<<1) && safety && keypad){
      bigred = 1;
    } else if(PINC & (1<<1)){
      bigred = 0;
      safety = 0;
      keypad = 0;
      resetAll();
    } else {
      //bigred = 0;
    }
    

    
    if(safety && keypad && bigred){
      LED3_GREEN;
      LED3_RED_OFF;
    } else {
      LED3_GREEN_OFF;
      LED3_RED;
    }

    if(safety && keypad){
      LED2_GREEN;
      LED2_RED_OFF;
    } else {
      LED2_GREEN_OFF;
      LED2_RED;
    }
    
    if(keypad){
      LED1_GREEN;
      LED1_RED_OFF;
    } else {
      LED1_GREEN_OFF;
      LED1_RED;
    }

    char c;
    if(c = waitKeyPress()){
      usb_debug_putchar(c);
      if(c == '*'){
        print("\nReset\n");
        memset(pass, 0, MAX_CODE);
        keypad = 0;
        print("Resetting All");
        resetAll();
      } else if(c == '#'){
        //Check Pass
        uint8_t match = 1;
        for(j=0;j<MAX_CODE;j++){
          if(code[j] != pass[j]){
            match = 0;
            j = MAX_CODE;
          }
        }
        if(match){
          print("Match!\n");
          keypad = 1;
          memset(pass, 0, MAX_CODE);
          ticks = 0;
          LED1_RED_OFF;
          LED1_GREEN_OFF;
          while(ticks <= 10){
            _delay_ms(50);
            if(ticks % 5 == 0){
              LED1_GREEN_T;
            }
            ticks++;
          }
        } else {
          print("Fail:");
          for(j=0;j<MAX_CODE;j++){
            usb_debug_putchar(pass[j]);
          }
          print("\n");
          keypad = 0;
          print("\nReset\n");
          memset(pass, 0, MAX_CODE);
          keypad = 0;
          ticks = 0;
          print("Resetting All");
          resetAll();
        }
      } else {
        for(j=0;j<MAX_CODE;j++){
          if(!pass[j]){
            pass[j] = c;
            j = MAX_CODE;
          } else if(j == (MAX_CODE-1)){
            //Pass Full
            print("Pass Full");
            memset(pass, 0, MAX_CODE);
            keypad = 0;
            print("Resetting All");
            resetAll();
          }
        }
      }
    }

    ticks++;
	}
}

void resetAll(void){
  uint16_t ticks = 0;

  LED1_GREEN_OFF;
  LED2_GREEN_OFF;
  LED3_GREEN_OFF;
  LED1_RED_OFF;
  LED2_RED_OFF;
  LED3_RED_OFF;
  while(PINC & (1<<0) || PINC & (1<<1) || ticks <= 60){
    _delay_ms(50);
    if(ticks % 5 == 0){
      LED1_RED_T;
      LED2_RED_T;
      LED3_RED_T;
    }
    ticks++;
  }
}

#define KEYPAD_COL 3
#define KEYPAD_ROW 4

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
  _delay_ms(50);
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

