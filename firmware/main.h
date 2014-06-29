/*
 * main.h
 *
 *  Created on: Apr 13, 2014
 *      Author: andrzej
 */

#ifndef MAIN_H_
#define MAIN_H_

#define NUM_OF_ADCS  5
#define NUM_OF_DIO_D 4
#define NUM_OF_DIO_B 4
#define NUM_OF_IRQ   4
#define MAX_NUMBER_OF_1WIRE_DEVICES 20
uint32_t EEMEM count_cWh_eeprom = 0;

typedef struct {
	uint8_t label[20];
} Label_t;

typedef struct {
	uint8_t  print_cWh;
	uint8_t  print_irq0;
	uint16_t tcnt1;
	uint8_t  timer1_ovf_count;
} Flags_t;

uint8_t EEMEM eep_devid = 1;
Label_t eep_dev_sn[1] EEMEM;
Label_t eep_dev_location[1] EEMEM;
Label_t eep_adc_sn[NUM_OF_ADCS] EEMEM;
Label_t eep_portd_sn[NUM_OF_DIO_D] EEMEM;
Label_t eep_portb_sn[NUM_OF_DIO_B] EEMEM;
Label_t eep_irq_sn[NUM_OF_IRQ] EEMEM;

uint8_t  timer1_ovf_count;
uint16_t timer1_count;
uint8_t  count_Wh;
uint32_t ext_interupt_count_0;
uint32_t count_cWh;

uint8_t Run;
uint8_t stream_timer_0;
uint8_t port_d_last_val;

Flags_t Flags;

void SetDevSNs(void);
void GetDevSNs(void);
void GetFW(void);

void GetA2D(void);
void GetDIO(void);
void GetWh(void);
void ResetWh(void);

void GetPortD(void);

void CmdLineLoop(void);
void HelpFunction(void);
void GetIDN(void);
void StreamingControl(void);

void test(void);

void Poke(void);
void Peek(void);
void Dump(void);

void ResetCounters(void);

void StartTemperatureMeasurement(void);
void GetTemperature(void);
void GetOneWireMeasurements(void);
void OneWireLoadRom(void);
void SaveThermometerIdToRom(void);
void OneWireReadRom(void);
void OneWireReadPage(void);
void OneWirerintScratchPad(void);
void OneWireWritePage(void);
void ChangeTmermPin(void);

void PrintLabel(Label_t *eep_label);
void json_sep(uint8_t i, uint8_t num_of_elements);
void json_end_bracket(void);
void json_open_bracket(void);
void json_comma(void);
void PrintCount_cWh(void);

void Interrupt0(void);
void Interrupt1(void);
void Timer1OvfFunc(void);
void Timer0Func(void);

#endif /* MAIN_H_ */
