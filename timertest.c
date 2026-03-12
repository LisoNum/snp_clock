#define F_CPU 1000000UL
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>



const int buffer = 6;
volatile uint16_t s = 0;
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
		prellCheck(prell1, prell2, prell3);
	}
	
}

void prellCheck(int prell1, int prell2, int prell3) {

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
	DDRD|= (1<<PD5) | (1<<PD6) | (1<<PD7);
	DDRB|= (1<<PB0) | (1<<PB2);
	DDRC|= (1<<PC5) | (1<<PC4) | (1<<PC3) | (1<<PC2) | (1<<PC1) | (1<<PC0);
}

void buttonSetup() {

	//set input Pins for Buttons
	DDRD&= ~(1<<PD2) & ~(1<<P3) & ~(1<<P4); //Set Ports as Input
	PORTD|= (1<<PD2) | (1<<PD3) | (1<<PD4); //turn on Pull-up

	//manage Interrupts
	EIMSK|= (1<<INT1) | (1<<INT0); //turned on interrupt INT1/0
	EICRA|= (1<<ISC11) | (1<<ISC01); //INT1/0 on falling edge
	PCICR|= (1<<PCIE2); //i
	PCMSK2|= (1<<PCINT20);//interrupt on PCINT20
}



