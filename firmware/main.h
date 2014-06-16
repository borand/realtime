/*
 * main.h
 *
 *  Created on: Apr 13, 2014
 *      Author: andrzej
 */

#ifndef MAIN_H_
#define MAIN_H_

uint32_t EEMEM count_cWh_eeprom = 0;
typedef struct {
	uint8_t label[20];
} Label_t;

uint8_t EEMEM eep_devid = 1;
Label_t eep_dev_location[1] EEMEM;
Label_t eep_adc_sn[8] EEMEM;
Label_t eep_portd_sn[8] EEMEM;
Label_t eep_portb_sn[8] EEMEM;
Label_t eep_irq_sn[8] EEMEM;


uint32_t ext_interupt_count_0;
uint32_t timer1_ovf_count;
uint32_t count_Wh;
uint32_t count_cWh;

uint8_t Run;
uint8_t stream_timer_0;
uint8_t port_d_last_val;

void SetDevSNs(void);
void GetDevSNs(void);

void GetA2D(void);
void GetDIO(void);
void GetWh(void);
void ResetWh(void);

void GetPortD(void);

void CmdLineLoop(void);
void HelpFunction(void);
void GetVersion(void);
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

#endif /* MAIN_H_ */
