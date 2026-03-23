#define F_CPU 1000000UL
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

volatile uint8_t display_mode = 0; //0 for minutes, 1 for seconds
volatile uint8_t blink_state = 1; //allows blinking, 1 for on, 0 for off
volatile uint8_t set_time = 0; //0 for normal operation, 1 for blinking mode
volatile uint8_t chosen_row = 0; //0 for hours, 1 for minutes, 2 for seconds
volatile uint8_t pwm_values[] = {0, 100, 255}; //levels for brightness
volatile uint8_t pwm_level = 0; //will be devided by %3

//random values
volatile uint8_t hours = 11;
volatile uint8_t minutes = 38;
volatile uint8_t seconds = 27; 

//settings for each button
volatile const int buffer = 6;

//functions for programm
void LEDSetup(void);
void pwmSetup(void);
void buttonSetup(void);
void update_display(void);
void normal_display_operation(void);
void setting_time_LEDs(void);

int main(void){
	timer1Setup();
	timer2Setup();
	LEDSetup();
	buttonSetup();
	sei();

	update_display(); //for debug purposes

	while (1) {
		
		asm("nop");
		enter_sleep_mode(); //saves power when nothing is to do
		_delay_ms(5); //delay for prell-check
	}
	return 0;
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

	calibrate_time(); //enforces time-rules

	if (set_time == 0) {
		normal_display_operation();
	}
	else {
		setting_time_LEDs(); 
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

//selected row when set_time==1 is supposed to blink
void setting_time_LEDs() {

    //masks for easier toggle
    uint8_t bitMask_hoursD = (PORTD & ~0xE0) | (0xE0 & (hours<<3)); //two for the hours
    uint8_t bitMask_hoursB = (PORTB & ~0x05) | (0x01 & hours) | (0x04 & (hours<<1));
    uint8_t bitMask_minutes = (PORTC & ~0x3F) | (0x3F & minutes);
    uint8_t bitMask_seconds = (PORTC & ~0x3F) | (0x3F & seconds);

    //non-chosen rows will be normaly displayed
    if (chosen_row%3 != 0) {
        PORTD = bitMask_hoursD;
        PORTB = bitMask_hoursB;
    }
    if (chosen_row%3 != 1) {
        PORTC = bitMask_minutes;
    }
    if (chosen_row%3 != 2) {
        PORTC = bitMask_seconds;
    }

    //blink state is toggled
    blink_state ^= 1;

    if (chosen_row%3 == 0) {
        if (blink_state) {
            PORTD = bitMask_hoursD; //turned on if blink state is one
            PORTB = bitMask_hoursB;
        } else {
            PORTD &= ~0xE0; //will be turned of if blink state is turned of
            PORTB &= ~0x05;
        }
    }
    if (chosen_row%3 == 1) {
        if (blink_state) {
            PORTC = bitMask_minutes;
        } else {
            PORTC &= ~0x3F;
        }
    }
    if (chosen_row%3 == 2) {
        if (blink_state) {
            PORTC = bitMask_seconds;
        } else {
            PORTC &= ~0x3F;
        }
    }
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
	_delay_ms(5); //wait for the bounce
	if (set_time == 0) {
		display_mode++; //changes displaymode
		update_display(); //updates display
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
		update_display(); //sets LEDs
		return; //exits
	}
}

ISR(INT1_vect) {
	_delay_ms(5); //wait for the bounce
	if (set_time == 0) {
		set_time = 1; //changes mode
		chosen_row = 0; //resets chosen blinking row to hours
		update_display(); //updates LEDs
		return; //exits
	}
	if (set_time == 1) {
		chosen_row++; //changes selected row
		update_display(); //updates LEDs
		return; //exits
	}
}

//change pwm-level ofr exit time-set mode
ISR(PCINT2_vect) {
	if (PIND & (1<<PD4)) {
		return; //ignores Button releases
	}
	_delay_ms(5); //wait for the bounce
	if (set_time == 0) { //not in Time-Set-Mode
		pwm_level++;
		OCR1A = pwm_values[pwm_level%3]; //switches right-round 
		return; //exits after pwm is switched
	}
	if (set_time == 1) {
		set_time = 0; //exits Time-setting-Mode
		display_mode = 0; 
		update_display(); //updates Display accordingly 
		return; //exits after mode is switched
	}
}

//increases seconds every time timer2 overflows, calibrates time if seconds reach 60 and updates display
ISR(TIMER2_OVF_vect) {
	if (set_time == 0) {
		seconds++; //only updates time when clock is moving nromaly
	}
	update_display(); //just to be sure
}

void timer2Setup() {
    // Activate asynchronous mode with an external clock 
    ASSR |= (1 << AS2);

    // Set Timer2 to Normal mode
    TCCR2A = 0;                     	    // Normal mode
    TCCR2B = (1 << CS22) | (1 << CS20);     // Prescaler = 128

    // Enable Timer2 Overflow interrupt
    TIMSK2 = (1 << TOIE2);

    // Wait for asynchronous clock stabilization
    while (ASSR & ((1 << TCR2AUB) | (1 << TCR2BUB))) {
        // Wait until Timer2 registers are updated
    }
}

void timer1Setup() {

    DDRB|= (1<<PB1); //Ausgang für PWM

	//Registerbelegung für PWM
	TCCR1A|= (1<<COM1A1); //Clear-on Compare Match
	TCCR1A|= (1<<WGM10); //8-Bit fast PWM
	TCCR1B|= (1<<WGM12);
	TCCR1B|= (1<<CS11); //Prescaler 8

	OCR1A = pwm_values[pwm_level]; //sets brightness
}

void enter_sleep_mode() {
	if(OCR1A == 255) {
        set_sleep_mode(SLEEP_MODE_PWR_SAVE); // Set Power-Down mode --> avr/sleep.h
        sleep_enable();                      // Enable sleep mode
        sleep_cpu();                         // Put the CPU to sleep
    } else {
	set_sleep_mode(SLEEP_MODE_IDLE); // Set Idle mode --> avr/sleep.h
        sleep_enable();                      // Enable sleep mode
        sleep_cpu();                         // Put the CPU to sleep	
    }
    // CPU wakes up here after an interrupt
    sleep_disable(); // Disable sleep mode after wake-up
}