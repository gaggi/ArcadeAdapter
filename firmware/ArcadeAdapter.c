/* Arcade Adapter 
 * 
 * Target: Arduino Pro Mini 16MHz
 *
 * Parts (c) 2009 MoJo aka Paul Qureshi
 * Parts (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2
 */



#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <avr/pgmspace.h>   /* required by usbdrv.h */

#include "usbdrv.h"
#include "report.h"
#include "descriptors.h"
#include "hid_modes.h"

#include "direct.h"

static	void*			usbDeviceDescriptorAddress;
static	int				usbDeviceDescriptorLength;
static	report_t		reportBuffer;

static const report_t emptyReportBuffer = {
	0,	// reportid
	0,	// b1
	0,	// b2
	0,	// x
	0	// y
};
	
void*	reportBufferAddress;
uchar	reportBufferLength;
uchar	hidMode;
void*	hidReportDescriptorAddress;
int		hidReportDescriptorLength;
void*	usbDescriptorStringDeviceAddress;
int		usbDescriptorStringDeviceLength;
uchar	hidNumReports;
uchar	idleRate;

const uchar hat_lut[] PROGMEM  = { -1, 0, 4, -1, 6, 7, 5, -1, 2, 1, 3, -1, -1, -1, -1, -1 };

/* ------------------------------------------------------------------------- */

void HardwareInit()
{
	// See schmatic for connections

	DDRB	= 0b00000000;
	PORTB	= 0b00111111;	// All inputs with pull-ups except xtal

	DDRC	= 0b00000000;
	PORTC	= 0b00111111;	// All inputs except unused bits

	DDRD	= 0b00000000;
	PORTD	= 0b11111001;	// All inputs with pull-ups except USB D+/D-	
	
}

/* ------------------------------------------------------------------------- */

void ReadController(uchar id)
{
	cli(); // disable interrupts when reading controller
	
	uchar	pcinton	= 0;
	
	reportBuffer = emptyReportBuffer;
	reportBuffer.reportid = id;
	
	ReadAll(&reportBuffer);

	if (!pcinton) PCICR	&= ~(1<<PCIE0);
	
	sei(); // re-enable interrupts after reading controller
}

/* ------------------------------------------------------------------------- */

void SetHIDMode()
{
	usbDeviceDescriptorAddress = usbDescriptorDeviceJoystick;
	usbDeviceDescriptorLength = sizeof(usbDescriptorDeviceJoystick);
	hidReportDescriptorAddress = usbHidReportDescriptor1P;
	hidReportDescriptorLength = sizeof(usbHidReportDescriptor1P);
	hidNumReports = 1;
	reportBufferAddress = &reportBuffer;
	reportBufferLength = sizeof(reportBuffer);
	usbDescriptorStringDeviceAddress = usbDescriptorStringDeviceDefault;
	usbDescriptorStringDeviceLength = sizeof(usbDescriptorStringDeviceDefault);
	
	usbDescriptorConfiguration[25] = hidReportDescriptorLength;

	cli();						// disable interrupts
    usbDeviceDisconnect();
	DDRD |= (1<<1) | (1<<2);	// USB reset

	_delay_ms(255);				// disconnect for >250ms

    usbDeviceConnect();
	DDRD &= ~((1<<1) | (1<<2));	// clear reset
	sei();						// restart interrupts
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    /* The following requests are never used. But since they are required by
     * the specification, we implement them in this example.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){
		/* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){
			/* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
			//ReadJoystick(rq->wValue.bytes[0]);
            usbMsgPtr = reportBufferAddress; //(void *)&reportBuffer;
            return reportBufferLength; //sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];

        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

uchar	usbFunctionDescriptor(struct usbRequest *rq)
{
	if (rq->bRequest == USBRQ_GET_DESCRIPTOR)
	{
		// USB spec 9.4.3, high byte is descriptor type
		switch (rq->wValue.bytes[1])
		{
			case USBDESCR_HID_REPORT:
				usbMsgPtr = (void*)hidReportDescriptorAddress;
				return hidReportDescriptorLength;
			case USBDESCR_CONFIG:
				usbMsgPtr = (void*)usbDescriptorConfiguration;
				return sizeof(usbDescriptorConfiguration);
			case USBDESCR_DEVICE:
				usbMsgPtr = usbDeviceDescriptorAddress;
				return usbDeviceDescriptorLength;
			case USBDESCR_STRING:
				switch (rq->wValue.bytes[0])
				{
					case 2:
						usbMsgPtr = (void*)usbDescriptorStringDeviceAddress;
						return usbDescriptorStringDeviceLength;
				}
		}
	}

	return 0;
}

/* ------------------------------------------------------------------------- */
void RemapController(char *x, char *y, char *rx, char *ry, uchar *b1, uchar *b2)
{
/*
Updated mapping used in all current subroutines:
b1	bit 0	bottom button 
	bit 1	right button 
	bit 2	left button  
	bit 3	upper button 
	bit 4	L2
	bit 5	R2
	bit 6	L1
	bit 7	R1
b2	bit 0	Select
	bit 1	Start 
	bit 2	L3
	bit 3	R3
	
Mapping required by Android:
	buttons in parentheses are non-standard, but seem to be supported in android
	bit 0	button 1: 	Android A (bottom)
	bit 1	button 2: 	Android B (right)
	bit 2	button 3: 	(Android C)
	bit 3	button 4: 	Android X (left)
	bit 4	button 5: 	Android Y (top)
	bit 5	button 6: 	(Android Z)
	bit 6	button 7: 	Android L1 
	bit 7	button 8: 	Android R1 
	bit 0	button 9: 	Android L2 
	bit 1	button 10: 	Android R2
	bit 2	button 11: 	(Android Select) (Select)
	bit 3	button 12: 	(Android Start) (Start)
	bit 4	button 13:	??
	bit 5	button 14: 	Android Left Thumb Stick Press
	bit 6	button 15: 	Android Right Thumb Stick Press
	bit 7	button 16: 	??
*/
	*x+=128;
	*y+=128;
	*rx+=128;
	*ry+=128;

	// So we have to map b1 bit 2 to 3 etc to conform with android
	if ((*b1 | 0x00) | (*b2 | 0x00))
	{	
			uchar oldb1 = *b1;
			uchar oldb2 = *b2;
			*b1=0;
			*b2=0;
			
			if (oldb1 & (1<<0)) *b1 |= (1<<0); // bottom
			if (oldb1 & (1<<1)) *b1 |= (1<<1); // right
			if (oldb1 & (1<<2)) *b1 |= (1<<2); // left
			if (oldb1 & (1<<3)) *b1 |= (1<<3); // top
			if (oldb1 & (1<<4)) *b1 |= (1<<4); // L2
			if (oldb1 & (1<<5)) *b1 |= (1<<5); // R2
			if (oldb1 & (1<<6)) *b1 |= (1<<6); // L1
			if (oldb1 & (1<<7)) *b1 |= (1<<7); // R1
			
			if (oldb2 & (1<<0)) *b2 |= (1<<0); // Select 
			if (oldb2 & (1<<1)) *b2 |= (1<<1); // Start
			if (oldb2 & (1<<2)) *b2 |= (1<<2); // L3
			if (oldb2 & (1<<3)) *b2 |= (1<<3); // R3
			if (oldb2 & (1<<4)) *b2 |= (1<<4); // 
			if (oldb2 & (1<<5)) *b2 |= (1<<5); // 
			if (oldb2 & (1<<6)) *b2 |= (1<<6); // 
			if (oldb2 & (1<<7)) *b2 |= (1<<7); // 
	}
}	

/* ------------------------------------------------------------------------- */

int main(void)
{
	uchar   i = 1;
	char remainingData=0;
	uchar offset=0;

	HardwareInit();
	usbInit();

	// Set up descriptor
	hidMode = HIDM_1P;
	ReadController(1);
	SetHIDMode();

//	uchar j = 1; //for speed test only
	
    for(;;){                /* main event loop */
        usbPoll();
        if(usbInterruptIsReady()){
            /* called after every poll of the interrupt endpoint */
			ReadController(i);
			
			remainingData=reportBufferLength;
			offset=0;

// For speed test, uncommnent the next three lines and the line "uchar j=0" above
//			reportBuffer.x=(j%4)*10; //for speed test only
//			reportBufferNegCon.x=(j%4)*10; //for speed test only
//			j++; //for speed test only
		
			// handle report with more than 8 byte length (for NegCon and future expansion)
			do {
				if (remainingData<=8) {
					usbSetInterrupt(reportBufferAddress+offset, remainingData);
					remainingData=0;
				}
				else {	
					usbSetInterrupt(reportBufferAddress+offset, 8);				
					offset+=8;
					remainingData-=8;
					do {
						usbPoll();
					} while (!usbInterruptIsReady());	
				}
			} while (remainingData>0);				

			i++;
			if (i > hidNumReports) i = 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
