#define F_CPU 1000000UL
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>


volatile uint8_t display_mode = 0; //0 for minutes, 1 for seconds
volatile uint8_t stopwatch_mode = 0; //0 for off, 1 for on
volatile uint8_t set_time = 0; //0 for normal operation, 1 for blinking mode
volatile uint8_t chosen_row = 0; //0 for hours, 1 for minutes, 2 for seconds

//random values
volatile uint8_t hours = 11;
volatile uint8_t minutes = 38;
volatile uint8_t seconds = 27; 

//settings for each button
const int buffer = 6;
volatile int prell1;
volatile int prell2;
volatile int prell3;

int main(void){
	
	prell1 = buffer; //buffer initialisieren
	prell2 = buffer;
	prell3= buffer;

	LEDSetup();
	pwmSetup();
	buttonSetup();
	sei();

	while (1) {
		
		asm("nop");
		prellCheck();
	}
	
}

void prellCheck() {

	if (prell1 != 0) {
		prell1--;
	}
	if (prell2 != 0) {
		prell2--;
	}
	if (prell3 != 0) {
		prell3--;
	}
	_delay_ms(20);

}

void pwmSetup() {

	DDRB|= (1<<PB1); //Ausgang für PWM

	//Registerbelegung für PWM
	TCCR1A|= (1<<WGM11); //Fast-PWM with Top at ICR1
	TCCR1B|= (1<<WGM13) | (1<<WGM12); //Fast-PWM with Top at ICR1
	TCCR1B|= (1<<CS11) | (1<<CS10); //PreScaler 64
	ICR1 = 35; //circa 450 Hz
	OCR1A = 17; //50% Duty-Cycle
	TCCR1A|= (1<<COM1A1); //Clear on Compare Match, Set at Top 
}

void LEDSetup() {
	DDRD|= (1<<PD5) | (1<<PD6) | (1<<PD7); //top three of hours
	DDRB|= (1<<PB0) | (1<<PB2); //bottom two
	DDRC|= (1<<PC5) | (1<<PC4) | (1<<PC3) | (1<<PC2) | (1<<PC1) | (1<<PC0); //minutes or seconds
}

void buttonSetup() {


	//set input Pins for Buttons
	DDRD&= ~(1<<PD2) & ~(1<<PD3) & ~(1<<PD4); //Set Ports as Input
	PORTD|= (1<<PD2) | (1<<PD3) | (1<<PD4); //turn on Pull-up

	//manage Interrupts
	EIMSK|= (1<<INT1) | (1<<INT0); //turned on interrupt INT1/0
	EICRA|= (1<<ISC11) | (1<<ISC01); //INT1/0 on falling edge
	PCICR|= (1<<PCIE2); //i
	PCMSK2|= (1<<PCINT20);//interrupt on PCINT20
}

void update_display() {

	
}

void normal_display_operation() {

	//hours
	PORTD = (PORTD & ~0xE0) | (0xE0 & (hours<<3)); //PD7, PD6, PD5
	PORTB = (PORTB & ~0x05) | (0x01 & hours) | (0x04 & (hours<<1));

	if (display_mode == 0) {
		PORTC = (PORTC & ~0x3F) | (0x3F & minutes);
	}
	else {
		PORTC = (PORTC &  ~0x3F) | (0x3f & seconds);
	}
}

void setting_time_LEDs() {

	//blinking hours

}

void calibrate_time() {
	
	if (seconds == 60) {
		minutes++;
		seconds = 0;

		if (minutes == 60) {
			hours++;
			minutes = 0;

			if (hours == 24) {
				hours = 0;
			}
		}
	}
}

//switch between stopwatch mode and normal mode
ISR(PCINT2_vector) {

}

