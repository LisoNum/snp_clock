#define F_CPU 1000000UL
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>




volatile uint8_t display_mode = 0; //0 for minutes, 1 for seconds
volatile uint8_t set_time = 0; //0 for normal operation, 1 for blinking mode
volatile uint8_t chosen_row = 0; //0 for hours, 1 for minutes, 2 for seconds
volatile uint8_t pwm_values[] = {254, 100, 0}; //levels for brightness
volatile uint8_t pwm_level = 0; //will be devided by %3

//random values
volatile uint8_t hours = 11;
volatile uint8_t minutes = 38;
volatile uint8_t seconds = 27; 

//settings for each button
volatile const int buffer = 6;
volatile int prell1;
volatile int prell2;
volatile int prell3;

//functions for programm
void LEDSetup(void);
void pwmSetup(void);
void buttonSetup(void);
void update_display(void);
void prellCheck(void);

int main(void){
	
	prell1 = buffer; //buffer initialisieren
	prell2 = buffer;
	prell3= buffer;

	LEDSetup();
	pwmSetup();
	buttonSetup();
	sei();

	update_display(); //for debug purposes

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
	TCCR1A|= (1<<COM1A1); //Clear-on Compare Match
	TCCR1A|= (1<<WGM10); //8-Bit fast PWM
	TCCR1B|= (1<<WGM12);
	TCCR1B|= (1<<CS11); //Prescaler 8

	OCR1A = pwm_values[pwm_level]; //sets brightness

}

void LEDSetup() {
	DDRD|= (1<<PD5) | (1<<PD6) | (1<<PD7); //top three of hours
	DDRB|= (1<<PB0) | (1<<PB2); //bottom two
	DDRC|= (1<<PC5) | (1<<PC4) | (1<<PC3) | (1<<PC2) | (1<<PC1) | (1<<PC0); //minutes or seconds
}

void buttonSetup() {


	//set input Pins for Buttons
	DDRD&= ~(1<<PD2) & ~(1<<PD3) & ~(1<<PD4); //Set PD2, PD3, PD4
	PORTD|= (1<<PD2) | (1<<PD3) | (1<<PD4); //turn on Pull-up

	//manage Interrupts
	EIMSK|= (1<<INT1) | (1<<INT0); //turned on interrupt INT1/0
	EICRA|= (1<<ISC11) | (1<<ISC01); //INT1/0 on falling edge
	PCICR|= (1<<PCIE2); //i
	PCMSK2|= (1<<PCINT20);//interrupt on PCINT20
}

//updates Display according to current mode
void update_display() {

	if (set_time == 0) {
		normal_display_operation();
	}
	else {
		normal_display_operation(); //for debug purposes
	}
}

void normal_display_operation() {

	//hours
	PORTD = (PORTD & ~0xE0) | (0xE0 & (hours<<3)); //PD7, PD6, PD5
	PORTB = (PORTB & ~0x05) | (0x01 & hours) | (0x04 & (hours<<1));

	if (display_mode%2 == 0) {
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

//change displayed row or increase time
ISR(INT0_vect) {

	if (prell1 != 0) {
		return; //exits function
	}

	if (set_time == 0) {
		display_mode++; //changes displaymode
		update_display(); //updates display
		prell1 = buffer; //resets buffer
		return; //exits function
	}
	if (set_time == 1) {
		//selects according to row
		if (chosen_row%3 == 0) {
			hours++;
		}
		if (chosen_row%3 == 1) {
			minutes++;
		}
		if (chosen_row%3 == 2) {
			seconds++;
		}
		calibrate_time; //calculates time
		update_display; //sets LEDs
		prell1 = buffer; //resets button
		return; //exits
	}
}

ISR(INT1_vect) {

	if (prell2 != 0) {
		return; //exits function
	}

	if (set_time == 0) {
		set_time = 1; //changes mode
		update_display(); //updates LEDs
		prell2 = buffer; //resets Button
		return; //exits
	}

	if (set_time == 1) {
		chosen_row++; //changes selected row
		update_display; //updates LEDs
		prell2 = buffer; //resets button
		return; //exits
	}
}

//change pwm-level ofr exit time-set mode
ISR(PCINT2_vect) {
	
	if (PIND & (1<<PD4)) {
		return; //ignores Button releases
	}

	if (prell3 != 0) {
		return; //exites function
	}
	
	if (set_time == 0) { //not in Time-Set-Mode
		pwm_level++;
		OCR1A = pwm_values[pwm_level%3]; //switches right-round 
		prell3 = buffer; //resets buffer
		return; //exits after pwm is switched
	}

	if (set_time == 1) {
		set_time = 0; //exits Time-setting-Mode
		update_display(); //updates Display accordingly 
		prell3 = buffer; //resets buffer
		return; //exits after mode is switched
	}
}

