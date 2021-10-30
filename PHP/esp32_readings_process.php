<?php
$headers =  getallheaders();
foreach($headers as $key=>$val){
  echo $key . ': ' . $val . '<br>';
  #file_put_contents("test.txt",$key . ': ' . $val . "\r\n" );
}

$MAC  = filter_input(INPUT_GET, 'MAC');
$HASH  = filter_input(INPUT_GET, 'HASH');
$tank_volume_per  = filter_input(INPUT_GET, 'tank_volume_per');
$tank_volume_ml  = filter_input(INPUT_GET, 'tank_volume_ml');
$tank_level_mm  = filter_input(INPUT_GET, 'tank_level_mm');
$vend_on_time_ms  = filter_input(INPUT_GET, 'vend_on_time_ms');
$vend_total_count  = filter_input(INPUT_GET, 'vend_total_count');
$batt_voltage_nominal  = filter_input(INPUT_GET, 'batt_voltage_nominal');
$batt_voltage_vend  = filter_input(INPUT_GET, 'batt_voltage_vend');
$seconds_since_reboot  = filter_input(INPUT_GET, 'seconds_since_reboot');
$sensor_raw_reading_mm  = filter_input(INPUT_GET, 'sensor_raw_reading_mm');
$sensor_deadband_mm  = filter_input(INPUT_GET, 'sensor_deadband_mm');
$error_message_string  = filter_input(INPUT_GET, 'error_message_string');
$tank_height_mm  = filter_input(INPUT_GET, 'tank_height_mm');
$tank_cas_mm2  = filter_input(INPUT_GET, 'tank_cas_mm2');
$software_version  = filter_input(INPUT_GET, 'software_version');

echo "payload:1"."<br>";
echo "reboot_request:" . "123" ."<br>";
echo "vend_on_time_ms:" . "300" ."<br>";

if(0)
{
  # echo backfor debug
  echo "MAC:" . $MAC ."<br>";
  echo "HASH:" . $HASH ."<br>";
  echo "tank_volume_per:" . $tank_volume_per ."<br>";
  echo "tank_volume_ml:" . $tank_volume_ml ."<br>";
  echo "tank_level_mm:" . $tank_level_mm ."<br>";
  echo "vend_on_time_ms:" . $vend_on_time_ms ."<br>";
  echo "vend_total_count:" . $vend_total_count ."<br>";
  echo "batt_voltage_nominal:" . $batt_voltage_nominal ."<br>";
  echo "batt_voltage_vend:" . $batt_voltage_vend ."<br>";
  echo "seconds_since_reboot:" . $seconds_since_reboot ."<br>";
  echo "sensor_raw_reading_mm:" . $sensor_raw_reading_mm ."<br>";
  echo "sensor_deadband_mm:" . $sensor_deadband_mm ."<br>";
  echo "error_message_string:" . $error_message_string ."<br>";
  echo "tank_height_mm:" . $tank_height_mm ."<br>";
  echo "tank_cas_mm2:" . $tank_cas_mm2 ."<br>";
  echo "software_version:" . $software_version ."<br>";  
}


if(0)
{
  # dump to file for debug
  $fileString="";
  $fileString .="MAC:" . $MAC ."\n";
  $fileString .="HASH:" . $HASH ."\n";
  $fileString .="tank_volume_per:" . $tank_volume_per ."\n";
  $fileString .="tank_volume_ml:" . $tank_volume_ml ."\n";
  $fileString .="tank_level_mm:" . $tank_level_mm ."\n";
  $fileString .="vend_on_time_ms:" . $vend_on_time_ms ."\n";
  $fileString .="vend_total_count:" . $vend_total_count ."\n";
  $fileString .="batt_voltage_nominal:" . $batt_voltage_nominal ."\n";
  $fileString .="batt_voltage_vend:" . $batt_voltage_vend ."\n";
  $fileString .="seconds_since_reboot:" . $seconds_since_reboot ."\n";
  $fileString .="sensor_raw_reading_mm:" . $sensor_raw_reading_mm ."\n";
  $fileString .="sensor_deadband_mm:" . $sensor_deadband_mm ."\n";
  $fileString .="error_message_string:" . $error_message_string ."\n";
  $fileString .="tank_height_mm:" . $tank_height_mm ."\n";
  $fileString .="tank_cas_mm2:" . $tank_cas_mm2 ."\n";
  $fileString .="software_version:" . $software_version ."\n";
  
  file_put_contents( $MAC."_trace.txt", $fileString);  
}

header($_SERVER["SERVER_PROTOCOL"].' 200 OK', true, 200);
?>
