#define F_CPU 1000000UL
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>

const int buffer = 6;
volatile uint16_t s = 0;
volatile int prell1;

int main(void){
	
	prell1 = buffer; //buffer initialisieren

	//Pinbelegung
	DDRB|= (1<<PB1); //Ausgang für PWM
	DDRC|= (1<<PC5); //Aufgang für LED
	DDRD|= ~(1<<PD3); //PD3 als Eingang
	PORTD|= (1<<PD3); //Pull-Up an
	
	//Registerbelegung für PWM
	TCCR1A|= (1<<WGM11); //Fast-PWM with Top at ICR1
	TCCR1B|= (1<<WGM13) | (1<<WGM12); //Fast-PWM with Top at ICR1
	TCCR1B|= (1<<CS11) | (1<<CS10); //PreScaler 64
	ICR1 = 35; //circa 450 Hz
	OCR1A = 17; //50% Duty-Cycle
	TCCR1A|= (1<<COM1A1); //Clear on Compare Match, Set at Top 
	
	//Interrupt-Beschreibung
	EIMSK|= (1<<INT1); //aktiviert Interrupt INT1
	EICRA|= (ISC11); //Interrupt auf fallender Flanke getriggert;

	sei();
	
	while (1) {
		prellCheck(prell1);
	}
	
}

void prellCheck(int prell1) {

	if (prell1 != 0) {
			prell1--;
		}
		_delay_ms(20);
}



