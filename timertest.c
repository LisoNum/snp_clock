#define F_CPU 1000000UL
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>

volatile uint16_t s = 0;

int main(void){
	
	TCCR1B|= (1<<CS11) | (1<<CS10); //PreScaler 64
	TCCR1B|= (1<<WGM12); //aktiviert CTC
	//TCCR1A|= (1<<COM1A0); //Toggle OC1A on compare match
	OCR1A= 15625 - 1; //setzt Timer-Breite
	TIMSK1|= (1<<OCIE1A);
	sei();

	DDRB|= (1<<PB1);
	
	while (1) {

		asm("nop");

	}
	
}
/*
macht gleiche wie Togle on compare match
war test für die timer interrupts

*/
/*
TOD0
establish pwm -> watch icr1
aim for frequenzy of 90 hz
built timebase via external crystal
*/

ISR(TIMER1_COMPA_vect) {
	
	if (s == 0) {
		PORTB|=(1<<PB1);
		s = 1;
		return;
	}
	if (s == 1) {
		PORTB&= ~(1<<PB1);
		s = 0;
		return;
	}
}

