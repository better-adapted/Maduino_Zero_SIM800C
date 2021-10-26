# 1 "D:\\SourceTree\\Maduino_Zero_SIM800C\\Maduino_Zero_SIM800C_HTTP_POST\\sketches\\Maduino_Zero_SIM800C_HTTP_POST.ino"


# 4 "D:\\SourceTree\\Maduino_Zero_SIM800C\\Maduino_Zero_SIM800C_HTTP_POST\\sketches\\Maduino_Zero_SIM800C_HTTP_POST.ino" 2



//view page: https://thingspeak.com/channels/825092 
/**

 * Maduino Zero SIM800C AT Commands Demo

 * Author:Charlin

 * Date:2019/07/20

 * Version:v1.0

 * Website:www.makerfabs.com

 * Email:Charlin@makerfabs.com

 * Function: 

 * Post random data to website via GSM/GPRS network

 * So upload this firmware to board, you can view the data on the web page  https://thingspeak.com/channels/825092

 */
# 22 "D:\\SourceTree\\Maduino_Zero_SIM800C\\Maduino_Zero_SIM800C_HTTP_POST\\sketches\\Maduino_Zero_SIM800C_HTTP_POST.ino"
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

String DateString = String("Oct 26 2021") + " " + String("16:04:47");
String VersionString = "1V00a";
String ProductString = "Maduino_SIM800C_SVP";

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

   tank_volume_ml = (tank_level_mm * tank_cas_mm2);
   tank_volume_ml /= 1000;

   tank_volume_per = (1000*tank_volume_ml);
   tank_volume_per /= tank_max_volume_ml;
   tank_volume_per /= 10;
  }

 }

 void print_reading()
 {
  char string_msg[200];
  sprintf(string_msg, "Ultrasonic,reading_mm:%d,timeouts:%d,tank_level_mm:%d,tank_volume_ml:%d,tank_volume_percentage:%.01f%c,@:%ld", reading_mm, timeouts, tank_level_mm, tank_volume_ml, tank_volume_per, '%', millis());
  Serial.println(string_msg);
 }
};

jsn_srt04_tank ultrasonic(io_pin_ultrasonic_trigger, io_pin_ultrasonic_input);


void CPU_Reset()
{
 NVIC_SystemReset(); // processor software reset
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
 SerialUSB.println(temp_msg);

 delay(1000);
 CPU_Reset();
}

void Modem_Reset_Pulse()
{
 digitalWrite(_IO_POWER_KEY, HIGH);
 delay(1000);
 digitalWrite(_IO_POWER_KEY, LOW);
}

int Modem_Init()
{
 Modem_clear_buffer();

 // check if the modem is one - it should not be!
 SIM800C_ON = false;
 Modem_sendData("AT", 1000, true /*true: debug on; false:debug off*/);
 delay(1000);

 if (SIM800C_ON)
 {
  Modem_sendData("AT+CPOWD=1", 1000, true /*true: debug on; false:debug off*/);
  delay(2000);

  SIM800C_ON = false;
  Modem_sendData("AT", 1000, true /*true: debug on; false:debug off*/);
  delay(2000);

  if (SIM800C_ON)
  {
   SerialUSB.println("Modem_Init(),AT+CPOWD=1 did not work,trying pulse instead");

   Modem_Reset_Pulse();
  }

  SIM800C_ON = false;
  Modem_sendData("AT", 1000, true /*true: debug on; false:debug off*/);
  delay(2000);

  if (SIM800C_ON)
  {
   SerialUSB.println("Modem_Init(),failed to power down");
   return -1;
  }
 }

 Serial1.flush();
 Modem_Reset_Pulse();

 int pu_counter = 0;
 while (!SIM800C_ON && pu_counter < 30)
 {
  Modem_sendData("AT", 1000, true /*true: debug on; false:debug off*/);

  for(int x=0;x<pu_counter;x++)
   SerialUSB.print(".");
  SerialUSB.println();

  pu_counter++;
 }

 if (!SIM800C_ON)
 {
  return -2;
 }

 Modem_sendData("AT+CMEE=2", 1000, true /*true: debug on; false:debug off*/);

 // command line should be up - start setup!	
 Modem_sendData("AT+CREG=1", 1000, true /*true: debug on; false:debug off*/);

 gl_response_ena = true;

 int creg_wait = 0;
 while (gl_response.indexOf("+CREG: 1,1")<0)
 {
  Modem_sendData("AT+CREG?", 1000, true /*true: debug on; false:debug off*/);

  creg_wait++;

  if (creg_wait > 60)
  {
   return -1;
  }
 }


 //**************Open internet connection*************************  
 Modem_sendData("AT+CGATT=1", 1000, true /*true: debug on; false:debug off*/);//Attach to GPRS service
 delay(2000);
 Modem_sendData("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", 1000, true /*true: debug on; false:debug off*/);

 //  sendData("AT+SAPBR=3,1,\"APN\",\"CMNET\"", 1000, DEBUG);
 Modem_sendData("AT+SAPBR=1,1", 1000, true /*true: debug on; false:debug off*/);

 //***************************************************************
 Modem_sendData("AT+HTTPINIT", 1000, true /*true: debug on; false:debug off*/);//initalize HTTP Service

 return 0;
}

String Modem_sendData(String command, const int timeout, boolean debug)
{
 String response = "";
 Serial1.println(command);
 SerialUSB.println(">> " + command);

 String commandpre = Modem_getcommand_pref(command);
 //SerialUSB.println(commandpre);

 long int time = millis();
 while ((time + timeout) > millis())
 {
  if (commandpre == "AT") {
   if (Serial1.find("OK")) {
    SIM800C_ON = true;
    SIM800C_ON_Once = true;
    //SerialUSB.println("SIM800C is ON");
   }
  }

  while (Serial1.available())
  {
   String c = Serial1.readString();
   response += c;
   if(gl_response_ena)
    gl_response += c;
  }
 }

 if (debug) {
  SerialUSB.println("<< " + response);
 }
 return response;
}

String Modem_getcommand_pref(String command) {

 String command_pref = "";
 char *cstr = new char[command.length() + 1];
 strcpy(cstr, command.c_str());
 char * token = strtok(cstr, "=");
 int i = 0;
 while (token != 
# 284 "D:\\SourceTree\\Maduino_Zero_SIM800C\\Maduino_Zero_SIM800C_HTTP_POST\\sketches\\Maduino_Zero_SIM800C_HTTP_POST.ino" 3 4
                __null
# 284 "D:\\SourceTree\\Maduino_Zero_SIM800C\\Maduino_Zero_SIM800C_HTTP_POST\\sketches\\Maduino_Zero_SIM800C_HTTP_POST.ino"
                    ) {
   //SerialUSB.print(token);
   //SerialUSB.println("  line" + (String)i);

  switch (i) {
  case 0:
   command_pref = token;
   break;

  default:
   break;
  }

  token = strtok(
# 297 "D:\\SourceTree\\Maduino_Zero_SIM800C\\Maduino_Zero_SIM800C_HTTP_POST\\sketches\\Maduino_Zero_SIM800C_HTTP_POST.ino" 3 4
                __null
# 297 "D:\\SourceTree\\Maduino_Zero_SIM800C\\Maduino_Zero_SIM800C_HTTP_POST\\sketches\\Maduino_Zero_SIM800C_HTTP_POST.ino"
                    , ",");
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
 String str_sensor_raw_reading_mm = String(ultrasonic.reading_mm);
 String str_sensor_deadband_mm = String(batt_voltage_nominal);
 String str_error_message_string = String("none");

 String str_tank_height_mm = String(ultrasonic.tank_height_mm);
 String str_tank_cas_mm2 = String(ultrasonic.tank_cas_mm2);
 String str_tank_max_volume_ml = String(ultrasonic.tank_max_volume_ml);

 String VersionTemp = ProductString + " " + VersionString + " " + DateString;
 VersionTemp.replace(" ", "_");

 String str_software_version = VersionTemp;

 String values_string = "?";

 values_string += "MAC=" + str_mac;
 values_string += "&";
 values_string += "HASH=" + str_hash_temp;
 values_string += "&";
 values_string += "tank_volume_per=" + str_tank_volume_per;
 values_string += "&";
 values_string += "tank_volume_ml=" + str_tank_volume_ml;
 values_string += "&";
 values_string += "tank_level_mm=" + str_tank_level_mm;
 values_string += "&";
 values_string += "batt_voltage_nominal=" + str_batt_voltage_nominal;
 values_string += "&";
 values_string += "seconds_since_reboot=" + str_seconds_since_reboot;
 values_string += "&";
 values_string += "sensor_raw_reading_mm=" + str_sensor_raw_reading_mm;
 values_string += "&";
 values_string += "sensor_deadband_mm=" + str_sensor_deadband_mm;
 values_string += "&";
 values_string += "error_message_string=" + str_error_message_string;
 values_string += "&";
 values_string += "tank_height_mm=" + str_tank_height_mm;
 values_string += "&";
 values_string += "tank_cas_mm2=" + str_tank_cas_mm2;
 values_string += "&";
 values_string += "software_version=" + str_software_version;

 return values_string;
}

void setup()
{
 pinMode(_IO_POWER_KEY, OUTPUT);
 digitalWrite(_IO_POWER_KEY, LOW);

 //Waiting for Arduino Serial Monitor port to connect, if you use other serial tool, you can commenting this.
 while (!SerialUSB) {
  delay(100);
 }

 //output 1 second pulse to turn on the SIM800C  
 SerialUSB.begin(115200);
 delay(100);

 if (true /*true: debug on; false:debug off*/) {
  SerialUSB.println("program starts to run!");
 }
 Serial1.begin(19200);

 randomSeed(analogRead(0));
}

//send data to website every 20 seconds
void loop()
{
 char temp_msg[50];
 int modem_init_result = Modem_Init();
 gl_response_ena = false;

 sprintf(temp_msg, "modem_init_result=%d\r\n", modem_init_result);
 SerialUSB.println(temp_msg);

 if (modem_init_result == 0)
 {
  temperature = random(-50, 80);// print a random number from -50 to 79

  humidity = random(0, 100);// print a random number from 0 to 99

  SerialUSB.print("random temperature is :");
  SerialUSB.println(temperature);

  SerialUSB.print("random humidity is :");
  SerialUSB.println(humidity);

  //String command = "AT+HTTPPARA=\"URL\",\"http://api.thingspeak.com/update.json?api_key=" + (String)APIKEY + "&field1=" + (String)temperature +"&field2=" + (String)humidity + "\"";
  //String command = "AT+HTTPPARA=\"URL\",\"http://api.thingspeak.com/update.json?api_key=" + (String)APIKEY + "&field1=25.5&field2=67.8\"";

  String command = "AT+HTTPPARA=\"URL\",\"http://www.indispensable.systems/esp32_readings_process.php" + Get_HTTP_Readings_String() + "\"";

  //SerialUSB.println(command);

  //Set HTTP Parameters Value 
  Modem_sendData(command, 1000, true /*true: debug on; false:debug off*/);
  delay(1000);
  Modem_sendData("AT+HTTPACTION=0", 5000, true /*true: debug on; false:debug off*/);//0 get, 1 post, 2 head
  delay(1000);
  Modem_sendData("AT+HTTPREAD", 1000, true /*true: debug on; false:debug off*/);//0 get, 1 post, 2 head

  SerialUSB.println("Now turning off SIM800C");

  Modem_clear_buffer();
  gl_response_ena = true;
  Modem_sendData("AT+CPOWD=1", 1000, true /*true: debug on; false:debug off*/);

  //Modem_Reset_Pulse();

  int power_down_mesg_wait = 0;
  while (gl_response.indexOf("NORMAL POWER DOWN")<0)
  {
   power_down_mesg_wait++;
   if (power_down_mesg_wait > 60)
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

 int sleep_counter = 60 * 60;

 SerialUSB.println("Going to Sleep");
 Modem_clear_buffer();

 while (sleep_counter > 0)
 {
  delay(1000);
  sleep_counter--;

  while (Serial1.available())
  {
   String c = Serial1.readString();
   gl_response += c;
  }

  SerialUSB.print(gl_response);
 }
}
