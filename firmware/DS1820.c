#include "DS1820.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

EE_RAM_t __attribute__((section (".eeprom"))) eeprom =
{
		150,//	t_conv ms
		1100, //uint8_t  t_reset_tx;
		1100, //uint8_t  t_reset_rx;
		30,//uint8_t  t_write_low;
		100,//uint8_t  t_write_slot;
		30,//uint8_t  t_read_samp;
		100, //uint8_t  t_read_slot;
		0, 0, 0, 0, 0, 0, 0, 0,//0
		16, 234, 182, 210, 1, 8, 0, 21, //6  Device
//	16,  72, 183, 210,   1,   8,   0, 234,
//	16,  48, 184, 210,   1,   8,   0, 188,
//	16,  27, 191, 210,   1,   8,   0, 163,
//	16, 242,  55, 192,   1,   8,   0, 214,
//	16,   2, 189, 210,   1,   8,   0, 237,
//	16, 154,  63, 211,   1,   8,   0,  58,
//
//		16, 126, 137, 210, 1, 8, 0, 147,//1  Basement staircase
//		16, 185, 54, 192, 1, 8, 0, 150, //2  Basement corner window
//		16, 184, 142, 210, 1, 8, 0, 239,//3  Basement corner window outside
//		16, 151, 41, 192, 1, 8, 0, 32, //4  Basement corner by the electrical panel
//		16, 243, 241, 210, 1, 8, 0, 96, //5  Basement over the desk
//		16, 72, 183, 210, 1, 8, 0, 234, //7  Device
//		16, 2, 189, 210, 1, 8, 0, 237, //8  Device
//		16, 27, 191, 210, 1, 8, 0, 163, //9  Device
//		16, 73, 184, 210, 1, 8, 0, 178, //10 Device
//		16, 242, 55, 192, 1, 8, 0, 214, //11
		0, 0, 0, 0, 0, 0, 0, 255, //Device 12
		0, 0, 0, 0, 0, 0, 0, 255, //Device 12
		0, 0, 0, 0, 0, 0, 0, 255, //Device 12
		0, 0, 0, 0, 0, 0, 0, 255, //Device 12
		0, 0, 0, 0, 0, 0, 0, 255, //Device 13
		0, 0, 0, 0, 0, 0, 0, 255, //Device 14
		0, 0, 0, 0, 0, 0, 0, 255, //Device 15
		0, 0, 0, 0, 0, 0, 0, 255, //Device 16
		0, 0, 0, 0, 0, 0, 0, 255, //Device 17
		0, 0, 0, 0, 0, 0, 0, 255, //Device 18
		0, 0, 0, 0, 0, 0, 0, 255, //Device 19
		0, 0, 0, 0, 0, 0, 0, 255, //Device 12
		0, 0, 0, 0, 0, 0, 0, 255, //Device 13
		0, 0, 0, 0, 0, 0, 0, 255, //Device 14
		0, 0, 0, 0, 0, 0, 0, 255, //Device 15
		0, 0, 0, 0, 0, 0, 0, 255, //Device 16
		0, 0, 0, 0, 0, 0, 0, 255, //Device 17
		0, 0, 0, 0, 0, 0, 0, 255, //Device 18
		0, 0, 0, 0, 0, 0, 0, 255 //Device 19
		//    0,0,0,0,0,0,0,255   //Device 20
		// DS1820 TIMING 1us ~= 2.2858 cycles @ 16MHz
		};

DS1820_t DS;

void therm_init(void)
{
	uint8_t i;
	for (i = 0; i < 9; i++)
		DS.scratchpad[i] = 0;
	for (i = 0; i < 8; i++)
		DS.devID[i] = 0;
	DS.therm_pin = PINB0;
	THERM_OUTPUT_MODE(THERM_DDR, 1);
	PIN_HIGH(THERM_PORT,1);
	DS.t_conv       = eeprom_read_word(&eeprom.t_conv);
	DS.t_reset_tx   = eeprom_read_word(&eeprom.t_reset_tx);
	DS.t_reset_rx   = eeprom_read_word(&eeprom.t_reset_rx);
	DS.t_write_low  = eeprom_read_byte(&eeprom.t_write_low);
	DS.t_write_slot = eeprom_read_byte(&eeprom.t_write_slot);
	DS.t_read_samp  = eeprom_read_byte(&eeprom.t_read_samp);
	DS.t_read_slot  = eeprom_read_byte(&eeprom.t_read_slot);
}

void therm_set_pin(uint8_t newPin)
{
	DS.therm_pin = newPin;
}
void therm_delay(uint16_t delay)
{
	while (delay--)
		asm volatile("nop");
}

uint8_t therm_reset()
{
	uint8_t i;
#ifdef THERM_DEBUG
	rprintf("therm_reset();\n");
#endif

	THERM_OUTPUT_MODE(THERM_DDR, 1);
	THERM_LOW(THERM_PORT, DS.therm_pin);
	THERM_OUTPUT_MODE(THERM_DDR, DS.therm_pin);
	therm_delay(DS.t_reset_tx); //480 us
	THERM_INPUT_MODE(THERM_DDR, DS.therm_pin);
	//while (bit_is_clear(THERM_PIN, DS.therm_pin));
	therm_delay(2);
	i = READ_PIN(THERM_PIN, DS.therm_pin);
	therm_delay(DS.t_reset_rx); //480 us
	//Return the value read from the presence pulse (0=OK, 1=WRONG)
#ifdef THERM_DEBUG
	if(i == 1)
	rprintf("Device Not Detected: i=%d\n",i);
	else
	rprintf("Device Detected: i=%d\n",i);;
#endif

	return i;
}

void therm_write_bit(uint8_t bit)
{
	//Pull line low for 1uS
	THERM_LOW(THERM_PORT, DS.therm_pin);
	THERM_OUTPUT_MODE(THERM_DDR, DS.therm_pin);
	therm_delay(DS.t_write_low);
	//If we want to write 1, release the line (if not will keep low)
	if (bit)
		THERM_INPUT_MODE(THERM_DDR, DS.therm_pin);
	//Wait for 60uS and release the line
	therm_delay(DS.t_write_slot);
	THERM_INPUT_MODE(THERM_DDR, DS.therm_pin);
}

uint8_t therm_read_bit(void)
{
	PIN_LOW(THERM_PORT,1);
	uint8_t bit = 0;
	//Pull line low for 1uS
	THERM_LOW(THERM_PORT, DS.therm_pin);
	THERM_OUTPUT_MODE(THERM_DDR, DS.therm_pin);
	//Release line and wait for 14uS
	THERM_INPUT_MODE(THERM_DDR, DS.therm_pin);
	therm_delay(DS.t_read_samp);
	PIN_HIGH(THERM_PORT,1);
	if (THERM_PIN & (1 << DS.therm_pin))
		bit = 1;
	//Wait for 45uS to end and return read value
	therm_delay(DS.t_read_slot);
	return bit;
}

uint8_t therm_read_byte(void)
{
	uint8_t i = 8, n = 0;
	cli();
	while (i--)
	{
		//Shift one position right and store read value
		n >>= 1;
		n |= (therm_read_bit() << 7);
	}
	sei();
	return n;
}

void therm_write_byte(uint8_t byte)
{
	uint8_t i = 8;
	cli();
	while (i--)
	{
		//Write actual bit and shift one position right to make the next bit ready
		therm_write_bit(byte & 1);
		byte >>= 1;
	}
	sei();
}

/////////////////////////////////////////////////////////////////////////
void therm_print_scratchpad()
{
	uint8_t i;
	rprintfStr("[");
	for (i = 0; i < 9; i++)
	{
		rprintfNum(10, 3, 0, ' ', DS.scratchpad[i]);
		//rprintfNum(16, 2, 0, ' ', DS.scratchpad[i]);
		if (i !=8)
			rprintfStr(",");
	}
	rprintfStr("]");
}

void therm_print_devID()
{
#ifdef THERM_DEBUG
	rprintf("therm_print_devID();");
#endif
	uint8_t i;
	rprintfStr("\"");
	for (i = 0; i < 8; i++)
	{
		rprintf("%d",DS.devID[i]);
		if (i==7)
			rprintf("\"");
		else
			rprintf(".");
	}
}

void therm_print_timing()
{
	rprintf("DD18x20 Timing\n");
	rprintf("01 t_conv       : %d\n",DS.t_conv);
	rprintf("02 t_reset_tx   : %d\n",DS.t_reset_tx);
	rprintf("02 t_reset_rx   : %d\n",DS.t_reset_rx);
	rprintf("02 t_write_low  : %d\n",DS.t_write_low);
	rprintf("02 t_write_slot : %d\n",DS.t_write_slot);
	rprintf("02 t_read_samp  : %d\n",DS.t_read_samp);
	rprintf("02 t_read_slot  : %d\n",DS.t_read_slot);
}

void therm_set_timing(uint8_t time, uint16_t interval)
{
#ifdef THERM_DEBUG
	rprintf("therm_set_timing(time = %d, interval = %d): ",time,interval);
#endif
	cli();
	switch (time)
	{
	case 1:
		eeprom_write_word(&eeprom.t_conv, interval);
		break;
	case 2:
		eeprom_write_word(&eeprom.t_reset_tx, interval);
		break;
	case 3:
		eeprom_write_word(&eeprom.t_reset_rx, interval);
		break;
	case 4:
		eeprom_write_byte(&eeprom.t_write_low, interval);
		break;
	case 5:
		eeprom_write_byte(&eeprom.t_write_slot, interval);
		break;
	case 6:
		eeprom_write_byte(&eeprom.t_read_samp, interval);
		break;
	case 7:
		eeprom_write_byte(&eeprom.t_read_slot, interval);
		break;
	default:
		break;
	}
	therm_init();
	sei();
}

/////////////////////////////////////////////////////////////////////////
uint8_t therm_load_devID(uint8_t devNum)
{
	uint8_t no_error = 0, crc[1], i = 0;
#ifdef THERM_DEBUG
	rprintf("therm_load_devID(): ");
#endif
	for (i = 0; i < 8; i++)
		DS.devID[i] = eeprom_read_byte(&eeprom.dev[devNum][i]);

	no_error = therm_crc_is_OK(DS.devID, crc, 7);
#ifdef THERM_DEBUG
	rprintf("no_error=%d\n",no_error);
#endif
	return no_error;
}

void therm_save_devID(uint8_t devNum)
{
	uint8_t i;
	cli();
	for (i = 0; i < 8; i++)
		eeprom_write_byte(&eeprom.dev[devNum][i], DS.devID[i]);
	sei();
}

void therm_set_devID(uint8_t *devID)
{
	uint8_t i;
	for (i = 0; i < 9; i++)
		DS.devID[i] = devID[i];
}

void therm_send_devID()
{
	uint8_t i = 0;
#ifdef THERM_DEBUG
	rprintf("therm_send_devID(): ");
#endif

	if (DS.devID[0] == 0)
	{
#ifdef THERM_DEBUG
		rprintf("SKIPROM\n");
#endif
		therm_write_byte(THERM_CMD_SKIPROM);
	}
	else
	{
#ifdef THERM_DEBUG
		rprintf("MATCHROM:\n");
#endif
		therm_write_byte(THERM_CMD_MATCHROM);
		for (i = 0; i < 8; i++)
			therm_write_byte(DS.devID[i]);
	}
}

uint8_t therm_read_devID()
{
	uint8_t no_error = 0, i = 0, crc[1];
	crc[0] = 0;

#ifdef THERM_DEBUG
	rprintfStr("therm_read_devID");
#endif

	therm_reset();
	//therm_send_devID();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_reset();
	therm_write_byte(THERM_CMD_READROM);
	for (i = 0; i < 8; i++)
		DS.devID[i] = therm_read_byte();

	no_error = therm_crc_is_OK(DS.devID, crc, 7);
#ifdef THERM_DEBUG
	rprintf("no_error=%d\n",no_error);
#endif
	return no_error;
}

void therm_start_measurement()
{
#ifdef THERM_DEBUG
	rprintf("therm_start_measurement();\n");
#endif
	//therm_send_devID();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_CONVERTTEMP);
}

uint8_t therm_read_scratchpad(uint8_t numOfbytes)
{
	uint8_t i = 0, crc[1], no_error;
#ifdef THERM_DEBUG
	rprintf("therm_read_scratchpad(); ");
#endif

	therm_send_devID();
	therm_write_byte(THERM_CMD_RSCRATCHPAD);
	for (i = 0; i < numOfbytes; i++)
		DS.scratchpad[i] = therm_read_byte();
	crc[0] = 0;
	no_error = therm_crc_is_OK(DS.scratchpad, crc, numOfbytes - 1);
	return no_error;
}

uint8_t therm_read_temperature(uint8_t devNum, int16_t *temperature)
{
#ifdef THERM_DEBUG
	rprintf("therm_read_temperature()\n");
#endif

	uint8_t no_error = 0;
	temperature[0] = 999;
	temperature[1] = 9999;
	if (therm_load_devID(devNum))
	{
		//therm_print_devID(devID);rprintfCRLF();
		therm_reset();
		therm_start_measurement();
		_delay_ms(200);
		therm_reset();
		no_error = therm_read_scratchpad(9);
		//therm_print_scratchpad(s);rprintfCRLF();
		if (no_error)
		{
			temperature[0] = (int16_t) (((DS.scratchpad[1] << 8)
					| (DS.scratchpad[0])) >> 1);
			if (temperature[0] < 0)
				temperature[1] = (int16_t) (10000
						- (int16_t) (DS.scratchpad[6])
								* THERM_DECIMAL_STEPS_12BIT);
			else
				temperature[1] = (int16_t) (10000
						- (int16_t) (DS.scratchpad[6])
								* THERM_DECIMAL_STEPS_12BIT);

		}
	}
#ifdef THERM_DEBUG
	rprintf("therm_read_temperature() no_error=%d\n",no_error);
#endif
	return no_error;

}

uint8_t therm_read_result(int16_t *temperature)
{
	uint8_t no_error = 1;
#ifdef THERM_DEBUG
	rprintf("therm_read_result()\n");
#endif
	temperature[0] = 999;
	temperature[1] = 9999;

#ifdef THERM_DEBUG
	rprintf("DS18x20 = %d\n",DS.devID[0]);
#endif
	therm_reset();

	if (no_error)
	{
		if(DS.devID[0] == DS18S20)
		{
			no_error = therm_read_scratchpad(9);
			//temperature[0] = (int16_t) (((DS.scratchpad[1] << 8) | (DS.scratchpad[0])) >> 1);
				temperature[0] = ((int16_t)((DS.scratchpad[1]<<8) | DS.scratchpad[0]));
				if (DS.scratchpad[1] == 255)
				{
					temperature[0] = (temperature[0] >> 1);
					if (DS.scratchpad[6] >= 12)
					{
						temperature[1] = (int16_t) (DS.scratchpad[6] * 625) - (int16_t) (7500);
					}
					else
					{
						temperature[0] += 1;
						temperature[1] = (int16_t) (2500) + (int16_t) (DS.scratchpad[6] * 625);
						if(temperature[0] == 0)
							rprintf("-");
					}
				}
				else
				{
					temperature[0] = (temperature[0] >> 1);
					if (DS.scratchpad[6] > 12)
					{
						temperature[0] -= 1;
						if(DS.scratchpad[0] == 0)
						{
							temperature[1] = (DS.scratchpad[6]) * 625 - 7500;
							temperature[0] = 0;
							if (DS.scratchpad[6] > 12)
								rprintf("-");
						}
//						else if ((DS.scratchpad[0] == 2) & (DS.scratchpad[6] == 12))
//							temperature[0] += 1;
						else
							temperature[1] = (int16_t) (17500) - (int16_t) (DS.scratchpad[6] * 625);
					}
					else
						temperature[1] = (int16_t) (7500) - (int16_t) (DS.scratchpad[6] * 625);
				}
			}
			else if(DS.devID[0] == DS18B20)
				{
					no_error = therm_read_scratchpad(9);
					temperature[0] = (int16_t) (((DS.scratchpad[1] << 8) | (DS.scratchpad[0])) >> 4);
					temperature[1] = (int16_t) (((DS.scratchpad[1] << 8) | (DS.scratchpad[0])) & 15)*THERM_DECIMAL_STEPS_12BIT;
				}
			else if (DS.devID[0] == DS2438)
			{
				if (get_ds2438_temperature())
				{
					temperature[0] = (int16_t) (DS.scratchpad[2]);
					temperature[1] = (int16_t) (DS.scratchpad[1]);
				}
			}

			else
				{
					rprintf("DEV_NOT_FOUND");
				}
	}
	else
		//rprintf("CRC ERROR");
		;


#ifdef THERM_DEBUG
	therm_print_scratchpad();rprintfCRLF();
	rprintf("therm_read_result() no_error=%d\n",no_error);
#endif

	rprintf("%d.",temperature[0]);
	rprintfNum(10, 4, 0, '0', temperature[1]);
	return no_error;
}

uint8_t therm_computeCRC8(uint8_t inData, uint8_t seed)
{
	uint8_t bitsLeft;
	uint8_t temp;

	for (bitsLeft = 8; bitsLeft > 0; bitsLeft--)
	{
		temp = ((seed ^ inData) & 0x01);
		if (temp == 0)
		{
			seed >>= 1;
		}
		else
		{
			seed ^= 0x18;
			seed >>= 1;
			seed |= 0x80;
		}
		inData >>= 1;
	}
	return seed;
}

uint8_t therm_crc_is_OK(uint8_t *scratchpad, uint8_t *crc, uint8_t numOfBytes)
{
	uint8_t i = 0;
#ifdef THERM_DEBUG
	rprintfStr("therm_crc_is_OK(); ");
#endif
	crc[0] = 0;
	for (i = 0; i < numOfBytes; i++)
		crc[0] = therm_computeCRC8(scratchpad[i], crc[0]);
#ifdef THERM_DEBUG
	rprintf(" if(%d == %d)\n",scratchpad[numOfBytes],crc[0]);
#endif
	return (scratchpad[numOfBytes] == crc[0]);
}

//////////////////////////////////////////////////////////////
// DS2438
//
void recal_memory_page(uint8_t page)
{
	uint8_t i = 0, crc[1], no_error, numOfbytes = 9;
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	_delay_ms(1);
	therm_write_byte(0xb8);
	therm_write_byte(page);

	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	_delay_ms(1);
	therm_write_byte(0xbe);
	therm_write_byte(page);
	for (i = 0; i < 9; i++)
		DS.scratchpad[i] = therm_read_byte();
		crc[0] = 0;
	no_error = therm_crc_is_OK(DS.scratchpad, crc, numOfbytes - 1);
}
void test_ds2438()
{
	uint8_t no_error = 0;

	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_CONVERTTEMP);
	_delay_ms(25);

	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_CONVERT_VOLTAGE);

	_delay_ms(25);

	recal_memory_page(0);
	therm_print_scratchpad();
	therm_reset();
}

uint8_t get_ds2438_temperature(void)
{
	uint8_t i = 0, crc[1], no_error = -1, numOfbytes = 9;
	no_error =  1;//therm_load_devID(devNum);
	if (no_error)
	{
		therm_reset();
		therm_send_devID();

		_delay_ms(1);
		therm_write_byte(0xb8);
		therm_write_byte(0);

		therm_reset();
		therm_send_devID();
		_delay_ms(1);
		therm_write_byte(0xbe);
		therm_write_byte(0);
		for (i = 0; i < numOfbytes; i++)
			DS.scratchpad[i] = therm_read_byte();
		crc[0] = 0;
		no_error = therm_crc_is_OK(DS.scratchpad, crc, numOfbytes - 1);
	}
	return no_error;
}

void write_to_page(uint8_t page, uint8_t val)
{
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(0x4e);
	therm_write_byte(page);
	therm_write_byte(val);
	therm_write_byte(val+1);
	therm_write_byte(val+3);
	therm_write_byte(val+4);
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(0x48);
	therm_write_byte(page);
}

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Search algorithm

//the 1-wire search algo taken from:
//http://www.maxim-ic.com/appnotes.cfm/appnote_number/187


//--------------------------------------------------------------------------
// Find the 'first' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : no device present
//

unsigned char OWFirst()
{
   // reset the search state
   LastDiscrepancy = 0;
   LastDeviceFlag = FALSE;
   LastFamilyDiscrepancy = 0;

   return OWSearch();
}

//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
unsigned char OWNext()
{
   // leave the search state alone
   return OWSearch();
}

//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
unsigned char OWSearch()
{
   unsigned char id_bit_number;
   unsigned char last_zero, rom_byte_number, search_result;
   unsigned char id_bit, cmp_id_bit;
   unsigned char rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;
   crc8 = 0;

   // if the last call was not the last one
   if (!LastDeviceFlag)
   {
      // 1-Wire reset
      if (OWReset())
      {
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = FALSE;
         LastFamilyDiscrepancy = 0;
         return FALSE;
      }

      // issue the search command
      OWWriteByte(SearchChar);  //!!!!!!!!!!!!!!!

      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = OWReadBit();
         cmp_id_bit = OWReadBit();

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit;  // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy)
                  search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == LastDiscrepancy);

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                     LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            OWWriteBit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                docrc8(ROM_NO[rom_byte_number]);  // accumulate the CRC
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!((id_bit_number < 65) || (crc8 != 0)))
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
         LastDiscrepancy = last_zero;

         // check for last device
         if (LastDiscrepancy == 0)
            LastDeviceFlag = TRUE;

         search_result = TRUE;
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !ROM_NO[0])
   {
      LastDiscrepancy = 0;
      LastDeviceFlag = FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = FALSE;
   }

   return search_result;
}

//--------------------------------------------------------------------------
// Verify the device with the ROM number in ROM_NO buffer is present.
// Return TRUE  : device verified present
//        FALSE : device not present
//
unsigned char OWVerify()
{
   unsigned char rom_backup[8];
   unsigned char i,rslt,ld_backup,ldf_backup,lfd_backup;

   // keep a backup copy of the current state
   for (i = 0; i < 8; i++)
      rom_backup[i] = ROM_NO[i];
   ld_backup = LastDiscrepancy;
   ldf_backup = LastDeviceFlag;
   lfd_backup = LastFamilyDiscrepancy;

   // set search to find the same device
   LastDiscrepancy = 64;
   LastDeviceFlag = FALSE;

   if (OWSearch())
   {
      // check if same device found
      rslt = TRUE;
      for (i = 0; i < 8; i++)
      {
         if (rom_backup[i] != ROM_NO[i])
         {
            rslt = FALSE;
            break;
         }
      }
   }
   else
     rslt = FALSE;

   // restore the search state
   for (i = 0; i < 8; i++)
      ROM_NO[i] = rom_backup[i];
   LastDiscrepancy = ld_backup;
   LastDeviceFlag = ldf_backup;
   LastFamilyDiscrepancy = lfd_backup;

   // return the result of the verify
   return rslt;
}

// TEST BUILD
static unsigned char dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current
// global 'crc8' value.
// Returns current global crc8 value
//
unsigned char docrc8(unsigned char value)
{
   // See Application Note 27
   // TEST BUILD
   crc8 = dscrc_table[crc8 ^ value];
   return crc8;
}

*/
