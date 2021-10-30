<?php

/*
CREATE TABLE SonicData (
	id INT(6) UNSIGNED AUTO_INCREMENT PRIMARY KEY,
	MAC VARCHAR(30) NOT NULL,
	HASH VARCHAR(30) NOT NULL,
	batt_voltage_nominal VARCHAR(30),
	seconds_since_reboot VARCHAR(30),
	sensor_raw_reading_mm VARCHAR(30),
	error_message_string VARCHAR(30),
	software_version VARCHAR(30),
	local_epoch VARCHAR(30),
	reading_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
)  

	MAC,HASH,batt_voltage_nominal,seconds_since_reboot,sensor_raw_reading_mm,error_message_string,software_version,local_epoch,reading_time
*/


$servername = "localhost";

// REPLACE with your Database name
$dbname = "indispen_test1";
$username = "indispen_adam";
$password = "sim800_test_user1";

// Keep this API Key value to be compatible with the ESP32 code provided in the project page. 
$hash_value = "tPmAT5Ab3j7F9";
$hash= $MAC = $batt_voltage_nominal = $seconds_since_reboot = $sensor_raw_reading_mm = $error_message_string = $software_version;

$pwm_current = $millis = $epoch = $local_time = "";

if ($_SERVER["REQUEST_METHOD"] == "GET") {
    $hash = test_input($_POST["HASH"]);
    if($hash == $hash) {
        $MAC = test_input($_POST["MAC"]);
        $batt_voltage_nominal = test_input($_POST["batt_voltage_nominal"]);
        $seconds_since_reboot = test_input($_POST["seconds_since_reboot"]);
        $sensor_raw_reading_mm = test_input($_POST["sensor_raw_reading_mm"]);
        $error_message_string = test_input($_POST["error_message_string"]);
        $software_version = test_input($_POST["software_version"]);
		
        // Create connection
        $conn = new mysqli($servername, $username, $password, $dbname);
        // Check connection
        if ($conn->connect_error) {
            die("Connection failed: " . $conn->connect_error);
        } 
        
        $sql = "INSERT INTO SonicData (MAC,HASH,batt_voltage_nominal,seconds_since_reboot,sensor_raw_reading_mm,error_message_string,software_version,local_epoch,reading_time)
        VALUES ('" . $MAC . "', '" . $HASH . "', '" . $batt_voltage_nominal . "', '" . $seconds_since_reboot . "', '" . $sensor_raw_reading_mm . "', '" . $error_message_string . "', '" . $software_version . "', '" . $local_epoch . "', '" . $reading_time . "')";
        
        if ($conn->query($sql) === TRUE) {
            echo "New record created successfully";
        } 
        else {
            echo "Error: " . $sql . "<br>" . $conn->error;
        }
    
        $conn->close();
    }
    else {
        echo "Wrong API Key provided.";
    }
}
else {
    echo "No data posted with HTTP POST.";
}

function test_input($data) {
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}