#include <Arduino.h>
#include <RTCZero.h>
#include <SAMD21_COM.h>
#include <Wire.h>

#define BSUM_FIRST_ADDRESS										0x00000
#define BSUM_LAST_ADDRESS											0x01FFF
#define PSUM_FIRST_ADDRESS										0x02000
#define PSUM_LAST_ADDRESS											0x3FFFF

uint32_t Program_Checksum = 0;
uint32_t Bootloader_Checksum = 0;

String CLI_String;
bool CLI_StringComplete;
static bool config_mode;
bool CLI_KeepAwake = false;

bool CLI_Modem = false;
bool rtc_interupt_flag = 0;

int _IO_POWER_KEY = 9; //D9
int IO_DTR_OUT = 5;

bool SIM800C_ON = false;
bool SIM800C_ON_Once = false;

//*******for senser **********
float humidity;//humidity
float temperature;//temperature


String gl_response = "";
bool gl_response_ena = false;


const int io_pin_ultrasonic_trigger = 32;
const int io_pin_ultrasonic_input = 33;

float batt_voltage_nominal = 0.0;

String DateString = String(__DATE__) + " " + String(__TIME__);
String VersionString = "1V00a";
String ProductString = "Maduino_SIM800C_SVP";

Stream* DebugPort=nullptr;

RTCZero rtc;

volatile uint32_t T3_Counter = 0;
volatile bool T3_WakeUp=false;

class jsn_srt04_tank
{
public:
	const uint32_t tank_height_mm = 820;
	const uint32_t tank_cas_mm2 = 7698;
	const uint32_t tank_max_volume_ml = (tank_height_mm*tank_cas_mm2) / 1000;

	uint32_t reading_mm = 0;
	uint32_t duration_us = 0;
	uint32_t timeouts = 0;
	uint32_t tank_level_mm = 0;
	uint32_t tank_volume_ml = 0;
	float tank_volume_per = 0;
	
	int _io_pin_ultrasonic_trigger = -1;
	int _io_pin_ultrasonic_input = -1;
	
	jsn_srt04_tank(int pTrigger, int pInput)
	{
		_io_pin_ultrasonic_trigger = pTrigger;
		_io_pin_ultrasonic_input = pInput;
	}
	
	void take_reading()
	{
		// Trigger the sensor by setting the io_pin_ultrasonic_trigger high for 10 microseconds:
		digitalWrite(_io_pin_ultrasonic_trigger, HIGH);
		delayMicroseconds(50);
		digitalWrite(_io_pin_ultrasonic_trigger, LOW);
  
		// Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:
		duration_us = pulseIn(_io_pin_ultrasonic_input, HIGH, 10000);

		if (duration_us == 0)
		{
			timeouts++;
			
			tank_level_mm = 0;
			tank_volume_ml = 0;
			tank_volume_per = 0;
		}		
		else
		{
			// Calculate the distance:
			reading_mm = duration_us * 0.343 / 2;

			if (reading_mm < tank_height_mm)
			{
				tank_level_mm = tank_height_mm - reading_mm;
			}
			else
			{
				// dont allow a negative level!
				tank_level_mm = 0;
			}
  
			tank_volume_ml =   (tank_level_mm * tank_cas_mm2);
			tank_volume_ml /= 1000;
  
			tank_volume_per = (1000*tank_volume_ml);
			tank_volume_per /= tank_max_volume_ml;
			tank_volume_per /= 10;			
		}
  
	}	
	
	String get_reading()
	{
		char string_msg[200];
		sprintf(string_msg, "Ultrasonic,reading_mm:%d,timeouts:%d,tank_level_mm:%d,tank_volume_ml:%d,tank_volume_percentage:%.01f%c,@:%ld", reading_mm, timeouts, tank_level_mm, tank_volume_ml, tank_volume_per, '%', millis());
		return string_msg;
	}
};

jsn_srt04_tank ultrasonic(io_pin_ultrasonic_trigger, io_pin_ultrasonic_input);

void Dummy_Handler(void)
{
	CPU_Reset();
}

void HardFault_Handler(void)
{
	CPU_Reset();
}

void Sleep_Enter()
{
	SysTick->CTRL  &= ~ SysTick_CTRL_ENABLE_Msk;
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	
	__DSB();
	__WFI();
	
	SysTick->CTRL  |= SysTick_CTRL_ENABLE_Msk;
}

void PM_Control(bool pUSB_Enable)
{
	PM->AHBMASK.bit.HPB0_=1;
	PM->AHBMASK.bit.HPB1_=1;
	PM->AHBMASK.bit.HPB2_=1;
	PM->AHBMASK.bit.DSU_=1;
	PM->AHBMASK.bit.NVMCTRL_=1;
	PM->AHBMASK.bit.DMAC_=0;
	PM->AHBMASK.bit.USB_=pUSB_Enable;
	
	PM->APBAMASK.bit.EIC_=1;
	PM->APBAMASK.bit.GCLK_=1;
	PM->APBAMASK.bit.PAC0_=1;
	PM->APBAMASK.bit.PM_=1;
	PM->APBAMASK.bit.RTC_=1;
	PM->APBAMASK.bit.SYSCTRL_=1;
	PM->APBAMASK.bit.WDT_=1;
	
	PM->APBBMASK.bit.DMAC_=0;
	PM->APBBMASK.bit.PAC1_=0;          /*!< bit:      0  PAC1 APB Clock Enable              */
	PM->APBBMASK.bit.DSU_=0;           /*!< bit:      1  DSU APB Clock Enable               */
	PM->APBBMASK.bit.NVMCTRL_=1;       /*!< bit:      2  NVMCTRL APB Clock Enable           */
	PM->APBBMASK.bit.PORT_=1;          /*!< bit:      3  PORT APB Clock Enable              */
	PM->APBBMASK.bit.DMAC_=0;          /*!< bit:      4  DMAC APB Clock Enable              */
	PM->APBBMASK.bit.USB_=pUSB_Enable;           /*!< bit:      5  USB APB Clock Enable               */
	PM->APBBMASK.bit.HMATRIX_=1;       /*!< bit:      6  HMATRIX APB Clock Enable           */

	PM->APBCMASK.bit.AC_=0;
	PM->APBCMASK.bit.ADC_=1;
	PM->APBCMASK.bit.DAC_=0;
	PM->APBCMASK.bit.EVSYS_=0;
	PM->APBCMASK.bit.I2S_=0;
	PM->APBCMASK.bit.PAC2_=1;
	PM->APBCMASK.bit.PTC_=0;
	PM->APBCMASK.bit.TC3_=1;
	PM->APBCMASK.bit.TC4_=0;
	PM->APBCMASK.bit.TC5_=0;
	PM->APBCMASK.bit.TC6_=0;
	PM->APBCMASK.bit.TC7_=0;
	PM->APBCMASK.bit.TCC0_=0;
	PM->APBCMASK.bit.TCC1_=0;
	PM->APBCMASK.bit.TCC2_=0;
	
	PM->APBCMASK.bit.SERCOM0_=1;
	PM->APBCMASK.bit.SERCOM1_=0;
	PM->APBCMASK.bit.SERCOM2_=0;
	PM->APBCMASK.bit.SERCOM3_=1;
	PM->APBCMASK.bit.SERCOM4_=0;
	PM->APBCMASK.bit.SERCOM5_=1;
}


void Modem_clear_buffer()
{
	gl_response_ena = false;
	gl_response = "";	
}

void Modem_Power_Down_Fail(int pTrace)
{
	char temp_msg[50];	
	sprintf(temp_msg, "Modem_power_down_fail(%d)", pTrace);
	DebugPort->println(temp_msg);	
	
	delay(1000);
	CPU_Reset();
}

void Modem_Reset_Pulse()
{
	DebugPort->println("Modem_Reset_Pulse()");
	digitalWrite(_IO_POWER_KEY, HIGH);
	delay(1000); 
	digitalWrite(_IO_POWER_KEY, LOW);
}

int Modem_Init()
{	
	Modem_clear_buffer();
	
	// check if the modem is one - it should not be!
	SIM800C_ON = false;  
	Modem_sendData("AT", 1000,true);
	delay(1000);	

	if (SIM800C_ON)
	{
		Modem_sendData("AT+CPOWD=1", 1000,true);
		delay(2000);
		
		SIM800C_ON = false;  
		Modem_sendData("AT", 1000,true);
		delay(2000);	
		
		if (SIM800C_ON)
		{
			DebugPort->println("Modem_Init(),AT+CPOWD=1 did not work,trying pulse instead");
			
			Modem_Reset_Pulse();
		}

		SIM800C_ON = false;  
		Modem_sendData("AT", 1000,true);
		delay(2000);	
		
		if (SIM800C_ON)
		{
			DebugPort->println("Modem_Init(),failed to power down");
			return -1;
		}
	}
	
	Serial0.flush();
	Modem_Reset_Pulse();	
	
	int pu_counter = 0;	
	while (!SIM800C_ON && pu_counter < 30)
	{
		Modem_sendData("AT", 1000,true);
		
		for(int x=0;x<pu_counter;x++)
			DebugPort->print(".");
		DebugPort->println();
		
		pu_counter++;
	}

	if (!SIM800C_ON)
	{
		return -2;
	}
	
	Modem_sendData("AT+CMEE=2", 1000,true);
	
	// command line should be up - start setup!	
	Modem_sendData("AT+CREG=1", 1000,true);
	
	gl_response_ena = true;

	int creg_wait = 0;
	while (gl_response.indexOf("+CREG: 1,1")<0)
	{
		Modem_sendData("AT+CREG?", 1000,true);
		
		creg_wait++;
		
		if (creg_wait > 60)
		{			
			return -1;
		}
	}
	
	
	//**************Open internet connection*************************  
	Modem_sendData("AT+CGATT=1", 1000,true);//Attach to GPRS service
	delay(2000); 
	Modem_sendData("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", 1000,true);
	
	//  sendData("AT+SAPBR=3,1,\"APN\",\"CMNET\"", 1000,true);
	Modem_sendData("AT+SAPBR=1,1", 1000,true);
  
	//***************************************************************
	Modem_sendData("AT+HTTPINIT", 1000,true);//initalize HTTP Service
	
	return 0;
}

String Modem_sendData(String command, const int timeout, boolean debug)
{
	String response = "";    
	Serial0.println(command);
	DebugPort->println(">> " + command);

	String commandpre = Modem_getcommand_pref(command);
	//Serial_Debug->println(commandpre);

	long int time = millis();   
	while ((time + timeout) > millis())
	{
		if (commandpre == "AT") {       
			if (Serial0.find("OK")) {
				SIM800C_ON = true;
				SIM800C_ON_Once = true;
				//Serial_Debug->println("SIM800C is ON");
			}
		}

		while (Serial0.available())
		{       
			String c = Serial0.readString(); 
			response += c;
			if(gl_response_ena)
				gl_response += c;
		} 
	}   

	if (debug) {
		DebugPort->println("<< " + response);
	}
	return response;
}

String Modem_getcommand_pref(String command) {

	String command_pref = "";
	char *cstr = new char[command.length() + 1];
	strcpy(cstr, command.c_str());
	char * token = strtok(cstr, "=");
	int i = 0;
	while (token != NULL) {
	  //Serial_Debug->print(token);
	  //Serial_Debug->println("  line" + (String)i);

		switch (i) {
		case 0:
			command_pref = token;
			break;
        
		default:
			break;
		}

		token = strtok(NULL, ",");
		i = i + 1;
	}

	if (command_pref == "")
		command_pref = command;
    
	return command_pref;
}

String Get_HTTP_Readings_String()
{
	//Check the current connection status		
	int httpCode;
	String str_mac = "";//String(WiFi.macAddress());
	String str_hash_temp = "HASH_TEMP";
		
	String str_tank_volume_per = String(ultrasonic.tank_volume_per);
	String str_tank_volume_ml = String(ultrasonic.tank_volume_ml);
	String str_tank_level_mm = String(ultrasonic.tank_level_mm);
		
	String str_batt_voltage_nominal = String(batt_voltage_nominal);

	String str_seconds_since_reboot = String(millis() / 1000);
	String str_sensor_raw_reading_mm =  String(ultrasonic.reading_mm);
	String str_sensor_deadband_mm = String(batt_voltage_nominal);
	String str_error_message_string = String("none");
		
	String str_tank_height_mm = String(ultrasonic.tank_height_mm);
	String str_tank_cas_mm2 = String(ultrasonic.tank_cas_mm2);
	String str_tank_max_volume_ml = String(ultrasonic.tank_max_volume_ml);
		
	String VersionTemp = ProductString + " " + VersionString + " " + DateString;		
	VersionTemp.replace(" ", "_");
		
	String str_software_version = VersionTemp;		

	String values_string = "?";

	values_string += "MAC="  + str_mac;
	values_string += "&";
	values_string += "HASH=" + str_hash_temp;
	values_string += "&";
	values_string += "tank_volume_per="  + str_tank_volume_per;
	values_string += "&";
	values_string += "tank_volume_ml="  + str_tank_volume_ml;
	values_string += "&";
	values_string += "tank_level_mm="  + str_tank_level_mm;		
	values_string += "&";
	values_string += "batt_voltage_nominal="  + str_batt_voltage_nominal;
	values_string += "&";
	values_string += "seconds_since_reboot="  + str_seconds_since_reboot;
	values_string += "&";
	values_string += "sensor_raw_reading_mm="  + str_sensor_raw_reading_mm;
	values_string += "&";
	values_string += "sensor_deadband_mm="  + str_sensor_deadband_mm;
	values_string += "&";
	values_string += "error_message_string="  + str_error_message_string;
	values_string += "&";
	values_string += "tank_height_mm="  + str_tank_height_mm;		
	values_string += "&";
	values_string += "tank_cas_mm2="  + str_tank_cas_mm2;		
	values_string += "&";
	values_string += "software_version="  + str_software_version;
			
	return values_string;			
}

void Version_Echo()
{
	char temp[200];
	
	sprintf(temp,"%s %s GNU %d.%d.%d, FreeRam:%d\r\n", __DATE__, __TIME__, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__,FreeRam());
	DebugPort->println(temp);
	DebugPort->flush();	
	
	sprintf(temp,"Bootloader_Checksum:%08X,Program_Checksum:%08X\r\n",Bootloader_Checksum,Program_Checksum);	
	DebugPort->println(temp);
	DebugPort->flush();	
}

void RTC_alarm()
{
	rtc_interupt_flag = true;
}

void Sleep_Mode(void)
{
	DebugPort->println("Sleep_Test(),DebugPort->flush()");
	DebugPort->flush();
	delay(10);
	
	rtc_interupt_flag = false;
	rtc.SetAlarmSecsFromNow(10*60);
	rtc.attachInterrupt(RTC_alarm);
	
	//TC3->COUNT16.CTRLA.bit.RUNSTDBY=0;
	uint32_t T3_prescaler = 64;
	uint32_t delay_sec = 10;
	TC3->COUNT16.CC[0].reg = (32768*delay_sec)/T3_prescaler;
	
	while (!rtc_interupt_flag)
	{
		Sleep_Enter();
	
		if (T3_WakeUp)
		{
			T3_WakeUp = false;
		
			char temp[50];
			sprintf(temp,"Woke,T3:%ld",T3_Counter);
			DebugPort->println(temp);		
			DebugPort->flush();				
		}		
	}

	if (rtc_interupt_flag)
	{
		rtc_interupt_flag=false;
		char temp[50];
		sprintf(temp,"Woke,rtc_interupt_flag");
		DebugPort->println(temp);		
		DebugPort->flush();
	}
		
	//TC3->COUNT16.CTRLA.bit.RUNSTDBY=1;

	DebugPort->println("Sleep_Test(),Woke");
	DebugPort->flush();
}

void TC3_Handler()
{
	// Overflow interrupt triggered
	if(TC3->COUNT16.INTFLAG.bit.OVF == 1)
	{
		TC3->COUNT16.INTFLAG.bit.OVF = 1;
		TC3->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;		
		
		T3_WakeUp = true;
		T3_Counter++;
	}
}

void TC3_Setup()
{
	PM->APBCMASK.reg |= PM_APBCMASK_TC3;

	//GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(6) | GCLK_CLKCTRL_ID_TCC2_TC3;  // 6 = lp os
	
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(1) | GCLK_CLKCTRL_ID_TCC2_TC3;  // GENERIC_CLOCK_GENERATOR_OSC32K =1 from startup.c
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;
	
	//TC3->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_PRESCSYNC_RESYNC;
	TC3->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV64 | TC_CTRLA_PRESCSYNC_RESYNC;
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;

	TC3->COUNT16.CTRLA.bit.RUNSTDBY = 1;
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;	

	TC3->COUNT16.COUNT.reg = 0;
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;	
		
	//TC3->COUNT16.CC[0].reg = 32768/4;  // 32768=1s, 32768/4 = 250ms
	TC3->COUNT16.CC[0].reg = 512;  // 32768=1s, 32768/4 = 250ms
	while(TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;
	
	
	TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;

	TC3->COUNT16.DBGCTRL.reg = 0; //TC_DBGCTRL_DBGRUN;
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;

	TC3->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;

	NVIC_EnableIRQ(TC3_IRQn);
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;
	
	// setup timer 3...
	TC3->COUNT16.CTRLA.bit.ENABLE = 1;
	while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}
	;
}

void setup_io()
{
	pinDigInit(14, OUTPUT, LOW);// PA02	
	//pinDigInit(42, OUTPUT, LOW);// PA03
	pinDigInit(17, OUTPUT, LOW);// PA04
	pinDigInit(18, OUTPUT, LOW);// PA05

	pinDigInit(8, OUTPUT, LOW); // PA06
	pinDigInit(9, OUTPUT, LOW); // PA07	- modem power key
	pinDigInit(4, OUTPUT, HIGH); // PA08 - sd cs
	pinDigInit(3, OUTPUT, LOW); // PA09

	//pinDigInit(1, OUTPUT, LOW); // PA10 - modem
	//pinDigInit(0, OUTPUT, LOW); // PA11 - modem
	
	pinDigInit(22, OUTPUT, LOW);// PA12 - miso
	pinDigInit(38, OUTPUT, LOW);// PA13
	pinDigInit(2, OUTPUT, LOW); // PA14
	pinDigInit(5, OUTPUT, LOW); // PA15	- modem dtr
	pinDigInit(11, OUTPUT, LOW);// PA16
	pinDigInit(13, OUTPUT, LOW);// PA17
	pinDigInit(10, OUTPUT, LOW);// PA18
	pinDigInit(12, OUTPUT, LOW);// PA19
	pinDigInit(6, OUTPUT, LOW); // PA20
	pinDigInit(7, OUTPUT, LOW); // PA21
	pinDigInit(20, INPUT, LOW);// PA22 - sda
	pinDigInit(21, INPUT, LOW);// PA23 - scl
	
	pinDigInit(28, OUTPUT, LOW);// PA24
	pinDigInit(29, OUTPUT, LOW);// PA25
	pinDigInit(26, OUTPUT, LOW);// PA27
	pinDigInit(27, OUTPUT, LOW);// PA28
	
	pinDigInit(44, OUTPUT, LOW);// PA30
	pinDigInit(45, OUTPUT, LOW);// PA31

	pinDigInit(19, OUTPUT, LOW);// PB02
	pinDigInit(25, OUTPUT, LOW);// PB03
	pinDigInit(15, OUTPUT, LOW);// PB08
	pinDigInit(23, OUTPUT, LOW);// PB10 - mosi
	pinDigInit(24, OUTPUT, LOW);// PB11 - sck
	pinDigInit(16, OUTPUT, LOW);// PB19
	pinDigInit(30, OUTPUT, LOW);// PB22
	pinDigInit(31, OUTPUT, LOW);// PB23
	
	if (0)
	{
		pinDigInit(43, INPUT, HIGH);// PA02	repeated	
		pinDigInit(32, INPUT, HIGH);// PA22 repeated
		pinDigInit(33, INPUT, HIGH);// PA23 repeated
	
		pinDigInit(40, INPUT, HIGH);// PA06 repeated
		pinDigInit(41, INPUT, HIGH);// PA07 repeated
		pinDigInit(39, INPUT, HIGH);// PA21 repeated

		pinDigInit(35, INPUT, HIGH);// PA16 repeated
		pinDigInit(37, INPUT, HIGH);// PA17 repeated
		pinDigInit(36, INPUT, HIGH);// PA18 repeated
		pinDigInit(34, INPUT, HIGH);// PA19 repeated
		
		pinDigInit(46, INPUT, HIGH);// PA20 repeated
		pinDigInit(47, INPUT, HIGH);// PA21 repeated		
	}	
}

int CLI_Process_Common(Stream *pStream,const String pString)
{	
	if (pString.equals(""))
	{
		pStream->println(">");
		return 0;
	}

	if (pString.startsWith("RESET"))
	{
		pStream->println("RESET");
		delay(1000);
		CPU_Reset();
		return 0;
	}
	  	  
	if (pString.equals("#"))
	{
		pStream->println("CLI_KeepAwake=1");
		CLI_KeepAwake = true;
		return 0;
	}

	if (pString.equals("~"))
	{
		pStream->println("CLI_KeepAwake=0");
		CLI_KeepAwake = false;
		return 0;
	}

	if (pString.startsWith(">>"))
	{
		if (!CLI_Modem)
		{
			pStream->println("CLI_DirectLora");
		}
		CLI_Modem = true;
		String Temp = pString.substring(2);
		Serial0.println(Temp);
		DebugPort->println(Temp);
		return 0;
	}
	
	/*if (pString.startsWith("NV."))
	{
		return NV.CLI_Handle(DebugPort, pString);
	}*/
	
	if (pString.startsWith("EPOCH="))
	{
		uint32_t temp_value = strtol(pString.substring(6).c_str(), 0, 10);          // decimal conversion
		
		rtc.setEpoch(temp_value);
		
		char temp_str[100];
		String rtc_temp;
		rtc.Get_TimeDate_String(rtc_temp);
		sprintf(temp_str, "EPOCH=%ld,%s", rtc.getEpoch(), rtc_temp.c_str());
		pStream->println(temp_str);
		return 0;		
	}	
		
	if ((pString.startsWith("RTC=")) && (pString.charAt(6) == ':') && (pString.charAt(9) == ':') && (pString.charAt(12) == ',') && (pString.charAt(15) == '/') && (pString.charAt(18) == '/') && (pString.charAt(23) == '\r'))
	{
		// RTC=15:31:00,14/02/2019r
		// 012345678901234567890123
		//           11111111112222
	
		String HoursValueString = pString.substring(4, 4 + 2);
		String MinsValueString = pString.substring(7, 7 + 2);
		String SecsValueString = pString.substring(10, 10 + 2);
		
		String DayValueString = pString.substring(13, 13 + 2);
		String MonthValueString = pString.substring(16, 16 + 2);
		String YearValueString = pString.substring(19, 19 + 4);

		long HoursValue = strtol(HoursValueString.c_str(), 0, 10);
		long MinssValue = strtol(MinsValueString.c_str(), 0, 10);
		long SecsValue = strtol(SecsValueString.c_str(), 0, 10);
		long DayValue = strtol(DayValueString.c_str(), 0, 10);
		long MonthValue = strtol(MonthValueString.c_str(), 0, 10);
		long YearValue = strtol(YearValueString.c_str(), 0, 10);
		
		rtc.setTime(HoursValue, MinssValue, SecsValue);
		rtc.setDate(DayValue, MonthValue, YearValue - 2000);
		
		String temp_rtc_str;
		rtc.Get_TimeDate_String(temp_rtc_str);
		
		char temp_str[100];
		sprintf(temp_str, "RTC=%s", temp_rtc_str.c_str());
		pStream->println(temp_str);
		return 0;
	}
	
	if (pString.startsWith("RTC?"))
	{
		String temp_rtc_str;
		rtc.Get_TimeDate_String(temp_rtc_str);
		char temp_str[100];
		sprintf(temp_str, "RTC=%s", temp_rtc_str.c_str());
		pStream->println(temp_str);
		return 0;
	}	
	
	if (pString.equals("VERSION"))
	{
		char temp[200];
		sprintf(temp, "\r\rLCM Proto %s %s GNU %d.%d.%d\r\n", __DATE__, __TIME__, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
		DebugPort->println(temp);
		DebugPort->flush();
	}
	
}

void DebugPortEvent()
{
	while (DebugPort->available())
	{
		// get the new byte:
		char inChar = (char)DebugPort->read();
		// add it to the inputString:
		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if(inChar == '\r')
		{
			CLI_StringComplete = true;
		}
		else if(inChar == '\n')
		{
		}		
		else if(inChar == 0x1b)
		{
			config_mode = true;
			DebugPort->println("ESC pressed - config mode");
		}		
		else
		{
			CLI_String += inChar;			
		}
	}
	
	if (CLI_StringComplete)
	{
		CLI_Process_Common(DebugPort, CLI_String);
		
		CLI_StringComplete = false;
		CLI_String = "";		
	}
}


void setup_fuses()
{
	const uint32_t USER_WORD_0 = 0xD8E087FF;//0x00200177;
	const uint32_t USER_WORD_1 = 0xFFFFFC5D;
	uint32_t UserFuseWordsRead[2] = { 0, 0 };
	uint32_t UserFuseWords[2] = { USER_WORD_0, USER_WORD_1 };
	
	samd21_read_fuse_bits(&UserFuseWordsRead[0]);
	char temp[100];
	sprintf(temp,"UW0:%08lX,UW1:%08lX,",UserFuseWordsRead[0], UserFuseWordsRead[1]);
	DebugPort->print(temp);
	
	if ((UserFuseWordsRead[0] == USER_WORD_0)&&(UserFuseWordsRead[1] == USER_WORD_1))
	{
		DebugPort->println("DEFAULT FUSES");
		DebugPort->flush();
	}
	else
	{
		DebugPort->println("!!!! CUSTOM FUSES !!!!");
		DebugPort->flush();		
		
		samd21_fix_fuses(UserFuseWords);
		
		delay(1000);
		
		DebugPort->println("Rebooting");
		DebugPort->flush();
		
		CPU_Reset();
	}
}

void setup()
{	
	setup_io();
		
	SerialUSB.begin(115200);
	Serial0.begin(19200);
	Serial5.begin(115200);
	Wire.begin();
	
	TC3_Setup();
	rtc.begin();	
	
	Bootloader_Checksum  = Get_Flash_Checksum(BSUM_FIRST_ADDRESS, BSUM_LAST_ADDRESS+1);
	Program_Checksum = Get_Flash_Checksum(PSUM_FIRST_ADDRESS, PSUM_LAST_ADDRESS+1);	
	
	SERCOM0->USART.CTRLA.bit.RUNSTDBY = 1;
	SERCOM5->USART.CTRLA.bit.RUNSTDBY = 1;	
							
	delay(100);
	
	//Waiting for Arduino Serial Monitor port to connect, if you use other serial tool, you can commenting this.
	int timeout = 0;
	bool usb_connected = 0;
	
	while (!SerialUSB && timeout > 0 && usb_connected)
	{		
		delay(1000);
		timeout--;
	}	
	
	if (timeout == 0)
	{
		PM_Control(0);
		DebugPort = (Stream*)&Serial5;		
	}
	else
	{
		PM_Control(1);
		DebugPort = (Stream*)&SerialUSB;		
				
		USB->DEVICE.CTRLA.bit.RUNSTDBY = 1;
	}
	
	Version_Echo();	
	
	setup_fuses();	
}

//send data to website every 20 seconds
void loop()
{
	char temp_msg[50];
	int modem_init_result = Modem_Init();
	gl_response_ena = false;	
	
	sprintf(temp_msg, "modem_init_result=%d\r\n", modem_init_result);
	DebugPort->println(temp_msg);
	
	if (modem_init_result == 0)
	{		
		temperature = random(-50, 80);// print a random number from -50 to 79
  
		humidity = random(0, 100);// print a random number from 0 to 99
                       
		DebugPort->print("random temperature is :");
		DebugPort->println(temperature);

		DebugPort->print("random humidity is :");
		DebugPort->println(humidity);
  
		//String command = "AT+HTTPPARA=\"URL\",\"http://api.thingspeak.com/update.json?api_key=" + (String)APIKEY + "&field1=" + (String)temperature +"&field2=" + (String)humidity + "\"";
		//String command = "AT+HTTPPARA=\"URL\",\"http://api.thingspeak.com/update.json?api_key=" + (String)APIKEY + "&field1=25.5&field2=67.8\"";

		String command = "AT+HTTPPARA=\"URL\",\"http://www.indispensable.systems/esp32_readings_process.php" + Get_HTTP_Readings_String() + "\"";
	
		//Serial_Debug->println(command);
  
		//Set HTTP Parameters Value 
		Modem_sendData(command, 1000,true);
		delay(1000); 
		Modem_sendData("AT+HTTPACTION=0", 5000,true);//0 get, 1 post, 2 head
		delay(1000); 
		Modem_sendData("AT+HTTPREAD", 1000,true);//0 get, 1 post, 2 head
	
		DebugPort->println("Now turning off SIM800C");	
	
		Modem_clear_buffer();
		gl_response_ena = true;
		Modem_sendData("AT+CPOWD=1", 1000,true);
		
		int power_down_msg_wait = 0;
		while (gl_response.indexOf("NORMAL POWER DOWN")<0)
		{
			power_down_msg_wait++;		
			if (power_down_msg_wait > 60)
			{
				Modem_Power_Down_Fail(1);
			}
			delay(1000);
		}				
	}
	else if (modem_init_result == -1)
	{
		Modem_Power_Down_Fail(2);
	}
	
	DebugPortEvent();
	
	Sleep_Mode();

	
/*	int sleep_counter = 60 * 60;	
	SerialUSB.println("Going to Sleep");	
	Modem_clear_buffer();	
	
	while (sleep_counter > 0)
	{		
		delay(1000);			
		sleep_counter--;
		
		while (Serial0.available())
		{       
			String c = Serial0.readString(); 
			gl_response += c;
		}
		
		SerialUSB.print(gl_response);
	}*/
}
