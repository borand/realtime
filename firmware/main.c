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


ISR(INT0_vect)
{
	ext_interupt_count_0++;
	count_Wh++;
	if (count_Wh == 100)
	{
		count_cWh++;
		count_Wh = 0;
		PrintCount_cWh();
	}	
    TCNT1            = 0;
	timer1_ovf_count = 0;
    //sbi(PORTB,PB0);
	//Interrupt0();
	//_delay_ms(1);
	//toggle(PORTB,PB5);
	//cbi(PORTB,PB0);
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
			json_end_bracket();
			cmdlinePrintPromptEnd();
			timer0ClearOverflowCount();
			cmdlinePrintPrompt();
		}
		//therm_reset();
		//therm_start_measurement();
	}
}

void Interrupt0(void)
{
	rprintfProgStrM("irq_0\",\"data\":[");
	json_open_bracket();
	PrintLabel(&eep_irq_sn[0]);
	json_comma();
	rprintfNum(10, 10, 0, ' ', ext_interupt_count_0);
	json_comma();
	rprintfNum(10, 10, 0, ' ', count_cWh);
	json_end_bracket();
	json_comma();
	json_open_bracket();
	PrintLabel(&eep_irq_sn[1]);
	json_comma();
	rprintfNum(10, 3, 0, ' ', timer1_ovf_count);
	json_comma();
	rprintfNum(10, 6, 0, ' ', TCNT1);
	json_end_bracket();
	json_end_bracket();
	cmdlinePrintPromptEnd();
	cmdlinePrintPrompt();
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
void PrintCount_cWh(void)
{
    json_open_bracket();
    PrintLabel(&count_cWh_eeprom);
    json_comma();
    rprintfNum(10, 10, 0, ' ', count_cWh);
    json_end_bracket();
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
	cmdlineAddCommand("setsn",SetDevSNs);
	cmdlineAddCommand("getsn",GetDevSNs);

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
	uint8_t  c;	
	uint32_t last_ext_interupt_count_0 = ext_interupt_count_0;	

	// set state to run
	Run = TRUE;
	// main loop
	while (Run)
	{
		// pass characters received on the uart (serial port)
		// into the cmdline processor
		GetPortD();
		if (last_ext_interupt_count_0 != ext_interupt_count_0)
			Interrupt0();

		while (uartReceiveByte(&c))
		{
			switch (c)
			{
			{
			case 'D':
				port_d_last_val = 0;
				GetPortD();
				break;
			case 'C':
				vt100ClearScreen();
				vt100SetCursorPos(1, 1);
				cmdlinePrintPrompt();
				break;
			case 'I':
				Interrupt0();
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
	rprintfProgStrM("C                : clear screen\n");
	rprintfProgStrM("I                : force interrupt 0 \n");
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
	rprintfProgStrM("\"Avr328pConsole 328P V14.06.18\"");
	cmdlinePrintPromptEnd();
}

void GetPortD(void)
{
	uint8_t i;
    uint8_t port_d_new;
	static uint8_t count = 0;

	port_d_new = PIND >> 4;
	if (port_d_last_val != port_d_new)
	{
		if(count++ > 50)
		{
			rprintfProgStrM("irq_port_d\",\"data\":[");
			json_open_bracket();
			rprintfProgStrM("\"port_d_new\"");
			json_comma();
			rprintfNum(10,3,FALSE,' ',port_d_new);
			json_end_bracket();
			json_comma();

			json_open_bracket();
			rprintfProgStrM("\"port_d_new\"");
			json_comma();
			rprintfNum(10,4,FALSE,' ',(port_d_last_val & port_d_new));
			json_end_bracket();
            json_comma();
            
            for (i = 0; i <NUM_OF_DIO_D; i++)
            {
                json_open_bracket();
                PrintLabel(&eep_portd_sn[i]);
                json_comma();
                rprintf("%d]", port_d_new >> i & 1);
                json_sep(i,NUM_OF_DIO_D);
            }
            json_end_bracket();
            cmdlinePrintPromptEnd();
			cmdlinePrintPrompt();
			port_d_last_val = port_d_new;
			count = 0;
        }

	}
}
////////////////////////////////////////////////////////////////
//Saving serial numbers
void SetDevSNs(void)
{
	uint8_t *port  = cmdlineGetArgStr(1);
	uint8_t devNum = (uint8_t) cmdlineGetArgInt(2);
	uint8_t *label = cmdlineGetArgStr(3);
	Label_t Label;
	if (port[0] == 'a')
	{
		if((devNum >=0) && (devNum < NUM_OF_ADCS))
		{
			strcpy(Label.label,label);
			eeprom_write_block(&Label,&eep_adc_sn[devNum],sizeof(Label_t));
			rprintfProgStrM("\"OK\"");
		}
		else
			rprintfProgStrM("\"ERROR - Invalid device number\"");

	}
	else if (port[0] == 'b')
	{
		if((devNum >=0) && (devNum < NUM_OF_DIO_B))
		{
			strcpy(Label.label,label);
			eeprom_write_block(&Label,&eep_portb_sn[devNum],sizeof(Label_t));
			rprintfProgStrM("\"OK\"");
		}
		else
			rprintfProgStrM("\"ERROR - Invalid device number\"");
	}
	else if (port[0] == 'd')
		{
			if((devNum >=0) && (devNum < NUM_OF_DIO_D))
			{
				strcpy(Label.label,label);
				eeprom_write_block(&Label,&eep_portd_sn[devNum],sizeof(Label_t));
				rprintfProgStrM("\"OK\"");
			}
			else
				rprintfProgStrM("\"ERROR - Invalid device number\"");
		}
	else if (port[0] == 'i')
		{
			if((devNum >=0) && (devNum < NUM_OF_IRQ))
			{
				strcpy(Label.label,label);
				eeprom_write_block(&Label,&eep_irq_sn[devNum],sizeof(Label_t));
				rprintfProgStrM("\"OK\"");
			}
			else
				rprintfProgStrM("\"ERROR - Invalid device number\"");
		}
	else if (port[0] == 'l') // assign location
		{
		
			strcpy(Label.label,label);
			eeprom_write_block(&Label,&eep_dev_location[0],sizeof(Label_t));
			rprintfProgStrM("\"OK\"");
		}
	else if (port[0] == 's') // asign serial number
		{
			strcpy(Label.label,label);
			eeprom_write_block(&Label,&eep_dev_sn[0],sizeof(Label_t));
			rprintfProgStrM("\"OK\"");
		}
	else
	{
		rprintfProgStrM("\"ERROR - Invalid syntax label [a|b|d|i|l|s] devNum label_text\"");
	}	
	cmdlinePrintPromptEnd();

}
void GetDevSNs(void)
{
	uint8_t  i;

	rprintfProgStrM("{\"SN\":");
    PrintLabel(&eep_dev_sn[0]);
	rprintfProgStrM(",\"location\":");
    PrintLabel(&eep_dev_location[0]);
	
    rprintfProgStrM(",\"ADC\":[");
	for(i=0;i<NUM_OF_ADCS;i++)
	{
		rprintf("[%d,",i); PrintLabel(&eep_adc_sn[i]); json_end_bracket();
        json_sep(i, NUM_OF_ADCS);
	}
	json_end_bracket();
    
	rprintfProgStrM(",\"PORTB\":[");
	for(i=0;i<NUM_OF_DIO_B;i++)	{        		
		rprintf("[%d,",i); PrintLabel(&eep_portb_sn[i]); json_end_bracket();
        json_sep(i, NUM_OF_DIO_B);
	}
    
    json_end_bracket();
	rprintfProgStrM(",\"PORTD\":[");
	for(i=0;i<NUM_OF_DIO_D;i++)
	{        		
		rprintf("[%d,",i); PrintLabel(&eep_portd_sn[i]); json_end_bracket();
		json_sep(i, NUM_OF_DIO_D);        
	}
    
    json_end_bracket();
	rprintfProgStrM(",\"IRQ\":[");
	for(i=0;i<NUM_OF_IRQ;i++)
	{
		rprintf("[%d,",i); PrintLabel(&eep_irq_sn[i]); json_end_bracket();
        json_sep(i, NUM_OF_IRQ);
	}
	json_end_bracket();
	rprintfProgStrM("}");
	cmdlinePrintPromptEnd();
}
////////////////////////////////////////////////////////////////
//JSON utility functions
void PrintLabel(Label_t *eep_label)
{
	Label_t Label;
	rprintfProgStrM("\"");
	eeprom_read_block(&Label,eep_label,sizeof(Label_t));
	rprintfStr(Label.label);
	rprintfProgStrM("\"");
	
}
void json_comma(void)
{
    rprintfProgStrM(",");
}
void json_sep(uint8_t i, uint8_t num_of_elements)
{
    if (i != num_of_elements-1) json_comma();
}
void json_end_bracket(void)
{
    rprintfProgStrM("]");
}
void json_open_bracket(void)
{
    rprintfProgStrM("[");
}

////////////////////////////////////////////////////////////////
//Testing and utility functions
void test(void)

{
	uint8_t arg1 = (uint8_t) cmdlineGetArgInt(1);
	uint8_t arg2 = (uint8_t) cmdlineGetArgInt(2);
	uint8_t value;

	// get_ds2438_temperature();
	// therm_print_scratchpad();
	
	rprintfProgStrM("{\"test\":");
	PrintLabel(&eep_dev_sn[0]);	
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

	//rprintfProgStrM("irq_0\",\"data\":[");
	json_open_bracket();
	json_open_bracket();
	PrintLabel(&eep_irq_sn[0]);
	json_comma();
	rprintfNum(10, 10, 0, ' ', ext_interupt_count_0);
	json_end_bracket();
	json_comma();
	json_open_bracket();
	PrintLabel(&eep_irq_sn[1]);
	json_comma();
	rprintfNum(10, 10, 0, ' ', eeprom_read_dword(&count_cWh_eeprom));
	json_end_bracket();
	json_end_bracket();
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
	json_open_bracket();
	for (i = 0; i <NUM_OF_ADCS; i++)
	{
		json_open_bracket();
        PrintLabel(&eep_adc_sn[i]);
        json_comma();
		rprintf("%d]", a2dConvert10bit(i));
        json_sep(i,NUM_OF_ADCS);
	}
	json_end_bracket();
	
    if (!skip_prompt)
		cmdlinePrintPromptEnd();
	a2dOff();
}
void GetDIO(void)
{
	uint8_t skip_prompt = (uint8_t) cmdlineGetArgInt(1);
	rprintf("[[\"PORTB\",%d],[\"PORTB\",%d]]",PINB,PIND);
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
// ONE WIRE DEVICES
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
// 		for (i = 1; i <= MAX_NUMBER_OF_1WIRE_DEVICES; i++)
// 		{
// 			if (therm_load_devID(i))
// 				device_count++;
// 		}
		for (i = 1; i <= MAX_NUMBER_OF_1WIRE_DEVICES; i++)
		{
			if (therm_load_devID(i))
			{
				loop_count++;
				json_open_bracket();
				therm_print_devID();json_comma();
				therm_read_result(t);json_comma();
				therm_print_scratchpad();
                json_end_bracket();
                json_comma();
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
	rprintfProgStrM("[\"0\",0,0]]");
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
		rprintf("[%d,",i);
		if (therm_load_devID(i))
			therm_print_devID();
		else
			rprintfProgStrM("[]");

		if (i<MAX_NUMBER_OF_1WIRE_DEVICES-1)
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
