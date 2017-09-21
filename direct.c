#include <avr/io.h>
#include <util/delay.h>
#include "report.h"
#include "direct.h"

void ReadAll(report_t *reportBuffer)
{
	DDRB	&= 0b11100000;				// PB1-PB4 inputs
	PORTB	|= 0b00011111;				// Pull-ups

	DDRC	&= 0b11000000;				// PC0-PC5 inputs
	PORTC	|= 0b00111111;				// Pull-ups

	DDRD	&= 0b00000110;				// All inputs except Select, don't touch USB D+/-
	PORTD	|= 0b11111001;				// Pull-ups
	
	// PD0?
	// PB5 LED?

	if (!(PIND & (1<<3))) reportBuffer->y = -128;		// up
	if (!(PIND & (1<<4))) reportBuffer->y = 127;		// down
	if (!(PIND & (1<<5))) reportBuffer->x = -128;		// left
	if (!(PIND & (1<<6))) reportBuffer->x = 127;		// right
	if (!(PIND & (1<<0))) reportBuffer->b1 |= (1<<0);	// Button 1
	if (!(PIND & (1<<7))) reportBuffer->b1 |= (1<<1);	// Button 2

	if (!(PINB & (1<<0))) reportBuffer->b1 |= (1<<2);	// Button 3
	if (!(PINB & (1<<1))) reportBuffer->b1 |= (1<<3);	// Button 4
	if (!(PINB & (1<<2))) reportBuffer->b1 |= (1<<4);	// Button 5
	if (!(PINB & (1<<3))) reportBuffer->b1 |= (1<<5);	// Button 6
	if (!(PINB & (1<<4))) reportBuffer->b1 |= (1<<6);	// Button 7
	if (!(PINB & (1<<5))) reportBuffer->b1 |= (1<<7);	// Button 8
	
	if (!(PINC & (1<<0))) reportBuffer->b2 |= (1<<0);	// Button 9
	if (!(PINC & (1<<1))) reportBuffer->b2 |= (1<<1);	// Button 10
	if (!(PINC & (1<<2))) reportBuffer->b2 |= (1<<2);	// Button 11
	if (!(PINC & (1<<3))) reportBuffer->b2 |= (1<<3);	// Button 12	
	if (!(PINC & (1<<4))) reportBuffer->b2 |= (1<<4);	// Button 13
	if (!(PINC & (1<<5))) reportBuffer->b2 |= (1<<5);	// Button 14
}
