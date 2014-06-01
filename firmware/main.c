/*
 * main.c
 *
 *  Created on: Sep 11, 2009
 *      Author: Andrzej
 */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include "global.h"

#include "uart.h"
#include "rprintf.h"
#include "vt100.h"
#include "cmdline.h"
#include "a2d.h"
#include "timer.h"
#include "extint.h"

#include "DS1820.h"

#include "main.h"

void Interrupt0(void)
{
	rprintfProgStrM("irq_0\",\"data\":[");
	rprintfNum(10, 10, 0, ' ', ext_interupt_count_0);
	rprintfProgStrM(",");
	rprintfNum(10, 10, 0, ' ', count_cWh);
	rprintfProgStrM(",");
	rprintfNum(10, 3, 0, ' ', timer1_ovf_count);
	rprintfProgStrM(",");
	rprintfNum(10, 6, 0, ' ', TCNT1);

	rprintfProgStrM("]");
	cmdlinePrintPromptEnd();
	cmdlinePrintPrompt();

	TCNT1            = 0;
	timer1_ovf_count = 0;
}
void Interrupt1(void)
{
	rprintfProgStrM("irq_1\",\"data\":[");
	rprintfProgStrM("]");
	cmdlinePrintPromptEnd();
	cmdlinePrintPrompt();
}

void Timer1OvfFunc(void)
{
	timer1_ovf_count++;
	toggle(PORTB,PB1);
}

ISR(INT0_vect)
{
	ext_interupt_count_0++;
	count_Wh++;
	if (count_Wh == 100)
	{
		count_cWh++;
		count_Wh = 0;
		eeprom_write_dword(&count_cWh_eeprom, count_cWh);
		rprintfProgStrM("irq_cWH\",\"data\":");
		rprintfNum(10, 10, 0, ' ', count_cWh);
		cmdlinePrintPromptEnd();
		cmdlinePrintPrompt();

	}
	sbi(PORTB,PB0);
	Interrupt0();
	_delay_ms(1);
	toggle(PORTB,PB5);
	cbi(PORTB,PB0);
}
ISR(INT1_vect)
{
	toggle(PORTB,PB5);
	Interrupt1();
}
ISR(TIMER1_OVF_vect)
{
	Timer1OvfFunc();
}

////////////////////////////////////////////////////////////////
// INTERRUPT CONTROL
void Timer0Func(void)
{
	if (timer0GetOverflowCount() >= 200)
	{
		if (stream_timer_0)
		{

			rprintfProgStrM("irq_tovf_0\",\"data\":[");
			//GetA2D();
			//rprintfProgStrM(", ");
			GetDIO();
			rprintfProgStrM("]");
			cmdlinePrintPromptEnd();
			timer0ClearOverflowCount();
			cmdlinePrintPrompt();
		}
		//therm_reset();
		//therm_start_measurement();
	}


}
////////////////////////////////////////////////////////////////
// MAIN
//
int main(void)
{
	uartInit(); //Initialize UART
	uartSetBaudRate(115200);//Default Baudrate
	rprintfInit(uartSendByte);
	cmdlineInit();
	cmdlineSetOutputFunc(uartSendByte);
	vt100Init();
	vt100ClearScreen();

	therm_init();

	///////////////////////////////////////////////////////
	// VARIABLE INIT

	count_Wh  = 0;
	count_cWh = eeprom_read_dword(&count_cWh_eeprom);

	port_d_last_val = PIND >> 4;
	///////////////////////////////////////////////////////
	// TIMER0
	stream_timer_0 = 0;
	timer0Init();
	timerAttach(0,Timer0Func);
	timer0SetPrescaler(TIMER_CLK_DIV1024);

	///////////////////////////////////////////////////////
	// TIMER1
	TIMSK1 = _BV(TOIE1);            //timer1Init();
	TCCR1B = _BV(CS12) | _BV(CS10); //timer1SetPrescaler(TIMER_CLK_DIV1024);

	ext_interupt_count_0 = 0;
	timer1_ovf_count     = 0;

	EICRA = _BV(ISC11) | _BV(ISC01);
	EIMSK = _BV(INT1)  | _BV(INT0);

	DDRB = _BV(PB0) | _BV(PB1) | _BV(PB5);

	PORTD = _BV(PD2) | _BV(PD3) | _BV(PD4) | _BV(PD5) | _BV(PD6) | _BV(PD7);

	cbi(DDRD,PB2);

	sbi(PORTB,PB5);
	cbi(DDRD,PB2);
	cbi(DDRD,PB3);


	// GENERIC COMMANDS
	cmdlineAddCommand("help", HelpFunction);
	cmdlineAddCommand("idn",  GetVersion);
	cmdlineAddCommand("test", test);
	cmdlineAddCommand("poke", Poke);
	cmdlineAddCommand("peek", Peek);
	cmdlineAddCommand("dump", Dump);

	cmdlineAddCommand("adc",  GetA2D);
	cmdlineAddCommand("dio",  GetDIO);

	cmdlineAddCommand("stream", StreamingControl);

	cmdlineAddCommand("getwh", GetWh);
	cmdlineAddCommand("resetwh", ResetWh);
	cmdlineAddCommand("reset", ResetCounters);

	//////////////////////////////////////////////////////////////
	//
	cmdlineAddCommand("owrom",  OneWireReadRom);
	cmdlineAddCommand("owload", OneWireLoadRom);
	cmdlineAddCommand("owsp",   OneWirerintScratchPad);
	cmdlineAddCommand("owsave", SaveThermometerIdToRom);
	cmdlineAddCommand("owstart", StartTemperatureMeasurement);
	cmdlineAddCommand("owtemp", GetTemperature);
	cmdlineAddCommand("owdata", GetOneWireMeasurements);
	cmdlineAddCommand("owrp",   OneWireReadPage);
	cmdlineAddCommand("owwp",   OneWireWritePage);

	cmdlinePrintPrompt();
	CmdLineLoop();
	return 0;
}
void CmdLineLoop(void)
{
	u08 c;
	// set state to run
	Run = TRUE;
	// main loop
	while (Run)
	{
		// pass characters received on the uart (serial port)
		// into the cmdline processor
		GetPortD();
		while (uartReceiveByte(&c))
		{
			switch (c)
			{
			{
			case 'D':
				port_d_last_val = 0;
				GetPortD();
				break;

			case 'I':
				Interrupt0();
				break;

			case 'C':
				vt100ClearScreen();
				vt100SetCursorPos(1, 1);
				cmdlinePrintPrompt();
				break;

			case 'Z':
				cmdlineResetPrompt();
				cmdlinePrintPrompt();
				break;
			}
			default:
				cmdlineInputFunc(c);
			}
		}
		// run the cmdline execution functions
		cmdlineMainLoop();
	}
}

void HelpFunction(void)
{
	rprintfProgStrM("Instant commands:\n");
	rprintfProgStrM("D                : get port D value\n");
	rprintfProgStrM("C                : force interrupt 0\n");
	rprintfProgStrM("I                : clear screen \n");
	rprintfProgStrM("Z                : reset command number to zero\n");

	rprintfProgStrM("\nCommands:\n");
	rprintfProgStrM("idn              : prints device ID and version info\n");
	rprintfProgStrM("test             : test function\n");
	rprintfProgStrM("peek [reg]       : returns dec, hex and bin value of register\n");
	rprintfProgStrM("poke [reg] [val] : sets register value to [val] \n");
	rprintfProgStrM("test             : test function\n");

	rprintfProgStrM("adc              : get ADC reading\n");
	rprintfProgStrM("dio              : get digital port readings\n");

	rprintfProgStrM("stream           : start streaming\n");

	rprintfProgStrM("\n\nPower Monitoring Commands:\n");

	rprintfProgStrM("getwh            : get accumulated Wh reading\n");
	rprintfProgStrM("resetwh          : reset the eeprom accumulator\n");
	rprintfProgStrM("reset            : reset timer0\n");

	rprintfProgStrM("\n\nOnewire Commands:\n");
	rprintfProgStrM("owrom            : read rom of a single device\n");
	rprintfProgStrM("owload           : load eeprom content\n");
	rprintfProgStrM("owsp             : show scratch pad\n");
	rprintfProgStrM("owsave           : save scratchpad to eeprom\n");
	rprintfProgStrM("owstart          : start temperature measurement\n");
	rprintfProgStrM("owtemp           : read temperatures\n");
	rprintfProgStrM("owdata           : read data from 3824 device\n");
	rprintfProgStrM("owrp             : read specific page from 3824 device\n");
	rprintfProgStrM("owwp             : write data to specified page\n");
}
void GetVersion(void)
{
	rprintfProgStrM("\"Avr328pConsole 328P V14.04.11\"");
	cmdlinePrintPromptEnd();
}

void GetPortD(void)
{
	uint8_t port_d_new;
	static uint8_t count = 0;

	port_d_new = PIND >> 4;
	if (port_d_last_val != port_d_new)
	{
		if(count++ > 50)
		{
			rprintfProgStrM("irq_port_d\",\"data\":[");
			rprintfNum(10,3,FALSE,' ',port_d_new);
			rprintfProgStrM(",");
			rprintfNum(10,4,FALSE,' ',(port_d_last_val & port_d_new));
			rprintfProgStrM("]");
			cmdlinePrintPromptEnd();
			cmdlinePrintPrompt();
			port_d_last_val = port_d_new;
			count = 0;
		}

	}
}

////////////////////////////////////////////////////////////////
//Testing and utility functions
void test(void)

{
	uint8_t arg1 = (uint8_t) cmdlineGetArgInt(1);
	uint8_t arg2 = (uint8_t) cmdlineGetArgInt(2);
	uint8_t value;

	get_ds2438_temperature();
	therm_print_scratchpad();
	cmdlinePrintPromptEnd();

//	value = arg1 & arg2;
//	rprintfNum(10,4,FALSE,' ',value);rprintf("\t");
//	rprintfNum(16,4,FALSE,' ',value);rprintf("\t");
//	rprintfNum(2,8,FALSE,'0',value);rprintf("\t");
}

void Poke(void) {

	uint16_t address = 0;
	uint8_t value = 0;

	address = cmdlineGetArgHex(1);
	value = cmdlineGetArgHex(2);
	_SFR_MEM8(address) = value;
}
void Peek(void) {

	uint16_t address = 0;
	uint8_t value = 0;

	address = cmdlineGetArgHex(1);
	value = _SFR_MEM8(address);
	rprintf("[%d,\"0x%x\"]", value, value);
	//rprintfNum(10,4,FALSE,' ',value);rprintf("\t");
	//rprintfNum(16,4,FALSE,' ',value);rprintf("\t");
	//rprintfNum(2,8,FALSE,'0',value);rprintf("\t");
	//rprintf("\n");
	cmdlinePrintPromptEnd();
}
void Dump(void) {

	uint16_t address, start, stop;
	uint8_t value, add_h, add_l;
	uint8_t col = 0;

	start = cmdlineGetArgHex(1);
	stop = cmdlineGetArgHex(2);

	for (address = start; address <= stop; address++) {
		value = _SFR_MEM8(address);
		if (col == 0) {
			add_h = (address >> 8);
			add_l = (address & 0xff);
			rprintf("0x%x%x 0x%x", add_h, add_l, value);
		} else {
			rprintf(" 0x%x", value);
		}
		col++;
		if (col == 8) {
			rprintf("\r\n");
			col = 0;
		}
	}
	rprintf("\r\n");

}
//////////////////////////////////////////
void ResetCounters(void)
{
	ext_interupt_count_0 = 0;
	TCNT1                = 0;
	timer1_ovf_count     = 0;
	rprintfProgStrM("1");
	cmdlinePrintPromptEnd();
}
/////////////////////////////////////////////////////////////////////////////////////
// EEPROM ACCESS
void GetWh(void)
{
	uint8_t skip_prompt = (uint8_t) cmdlineGetArgInt(1);
	rprintfNum(10,10,FALSE,' ',eeprom_read_dword(&count_cWh_eeprom));
	if (!skip_prompt)
		cmdlinePrintPromptEnd();

}
void ResetWh(void)
{
	count_cWh = 0;
	eeprom_write_dword(&count_cWh_eeprom, count_cWh);
	rprintfNum(10,10,FALSE,' ',eeprom_read_dword(&count_cWh_eeprom));
	cmdlinePrintPromptEnd();
}
/////////////////////////////////////////////////////////////////////////////////////
// ADC
void GetA2D(void)
{
	uint8_t skip_prompt = (uint8_t) cmdlineGetArgInt(1);
	uint8_t i;

	// configure a2d port (PORTA) as input
	// so we can receive analog signals
	DDRC = 0x00;
	// make sure pull-up resistors are turned off
	PORTC = 0x00;

	// turn on and initialize A/D converter
	a2dInit();
	// set the a2d prescaler (clock division ratio)
	// - a lower prescale setting will make the a2d converter go faster
	// - a higher setting will make it go slower but the measurements
	//   will be more accurate
	// - other allowed prescale values can be found in a2d.h
	a2dSetPrescaler(ADC_PRESCALE_DIV128);

	// set the a2d reference
	// - the reference is the voltage against which a2d measurements are made
	// - other allowed reference values can be found in a2d.h
	a2dSetReference(ADC_REFERENCE_AVCC);

	// use a2dConvert8bit(channel#) to get an 8bit a2d reading
	// use a2dConvert10bit(channel#) to get a 10bit a2d reading
	rprintfProgStrM("[");
	for (i = 0; i <NUM_OF_ADCS; i++)
	{
		rprintf("%d", a2dConvert10bit(i));
		if (i !=(NUM_OF_ADCS-1))
			rprintfProgStrM(",");

	}
	rprintfProgStrM("]");
	if (!skip_prompt)
		cmdlinePrintPromptEnd();
	a2dOff();
}
void GetDIO(void)
{
	uint8_t skip_prompt = (uint8_t) cmdlineGetArgInt(1);
	rprintf("[%d, %d]",PINB,PIND);
	if (!skip_prompt)
		cmdlinePrintPromptEnd();

}
/////////////////////////////////////////////////////////////////////////////////////
// STREAMING FUNCTION
void StreamingControl(void)
{
	stream_timer_0 = (uint8_t) cmdlineGetArgInt(1);
	rprintf("%d",stream_timer_0);
	cmdlinePrintPromptEnd();
}

////////////////////////////////////////////////////////////////
//Thermometer functions
//
void ChangeTmermPin(void)
{
	therm_set_pin((uint8_t)cmdlineGetArgInt(1));
}
void StartTemperatureMeasurement(void)
{
	therm_reset();
	therm_start_measurement();
	rprintfProgStrM("1");
	cmdlinePrintPromptEnd();
}
void GetTemperature(void)
{
	int16_t t[2];
	uint8_t i, device_count = 0, loop_count=0;
	int8_t devNum = (int8_t) cmdlineGetArgInt(1);

	therm_reset();
	therm_start_measurement();
	_delay_ms(750);
	rprintfProgStrM("[");
	if (devNum == 0)
	{
		for (i = 1; i <= 20; i++)
		{
			if (therm_load_devID(i))
				device_count++;
		}
		for (i = 1; i <= 20; i++)
		{
			if (therm_load_devID(i))
			{
				loop_count++;
				rprintfProgStrM("[");
				therm_print_devID();
				rprintfProgStrM(", ");
				therm_read_result(t);
				rprintfProgStrM(", ");
				therm_print_scratchpad();
				rprintfProgStrM("],");
			}
		}
	}
	else if (devNum == -1)
	{
		therm_read_devID();
		therm_read_result(t);
	}
	else
	{
		if (therm_load_devID(devNum))
		{
			therm_read_result(t);
		}
	}
	rprintfProgStrM("[0,0,0]]");
	cmdlinePrintPromptEnd();
}
void GetOneWireMeasurements(void)
{
	test_ds2438();
	cmdlinePrintPromptEnd();
}
void OneWireReadRom(void)
{
	if(therm_read_devID())
		therm_print_devID();
	else
	{
		therm_print_devID();
		rprintf("CRC Error");
	}
	cmdlinePrintPromptEnd();
}
void OneWireLoadRom(void)
{
	uint8_t i;

	rprintfProgStrM("[");
	for (i = 1; i < 20; i++)
	{
		rprintf("[%d, ",i);
		if (therm_load_devID(i))
			therm_print_devID();
		else
			rprintfProgStrM("[]");

		if (i<19)
			rprintf("],");
		else
			rprintfProgStrM("]");
	}
	rprintfProgStrM("]");
	cmdlinePrintPromptEnd();

}
void SaveThermometerIdToRom(void)
{
	uint8_t  devNum = (uint8_t) cmdlineGetArgInt(1);
	uint8_t  devID[8], i;
	for(i=0;i<8;i++)
		devID[i] = (uint8_t) cmdlineGetArgInt(i+2);

	if (devID[0] != 0)
		therm_set_devID(devID);
	therm_save_devID(devNum);
	OneWireLoadRom();
}
void OneWireReadPage(void)
{
	uint8_t  page = (uint8_t) cmdlineGetArgInt(1);

	recal_memory_page(page);
	therm_print_scratchpad();
	cmdlinePrintPromptEnd();
}
void OneWireWritePage(void)
{
	//rprintfProgStrM("OneWireWritePage");
	uint8_t  page = (uint8_t) cmdlineGetArgInt(1);
	uint8_t  val  = (uint8_t) cmdlineGetArgInt(2);
	write_to_page(page, val);
	_delay_ms(1);
	recal_memory_page(page);
	therm_print_scratchpad();
	cmdlinePrintPromptEnd();
}
void OneWirerintScratchPad(void)
{
	therm_print_scratchpad();
	cmdlinePrintPromptEnd();
}
