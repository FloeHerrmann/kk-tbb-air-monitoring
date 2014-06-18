/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Author:    m2m systems GmbH, Florian Herrmann                             */
/* Copyright: 2014, m2m systems GmbH, Florian Herrmann                       */
/* Purpose:                                                                  */
/*   Monitoring and measuring the water consuption and temperatures of a     */
/*   shower to calculate the costs for taking this shower.                   */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <DHT.h>

// Define own data types
#define ubyte uint8_t
#define uint uint16_t
#define ulong uint32_t

#define HUMIDITY_SENSOR_SIG A0
#define HUMIDITY_SENSOR_TYPE DHT22

#define SEVEN_SEGMENT_TX 10
#define SEVEN_SEGMENT_RX 11

#define BUTTON_OPEN 3
#define BUTTON_CLOSE 2
#define OUTPUT_CLOSE 4
#define OUTPUT_OPEN 5

#define UPDATE_INTERVAL 10000

char TemperatureString[10]; 

SoftwareSerial TemperatureDisplay( SEVEN_SEGMENT_RX , SEVEN_SEGMENT_TX );

DHT HumiditySensor( HUMIDITY_SENSOR_SIG , HUMIDITY_SENSOR_TYPE );

Adafruit_24bargraph CO2Bar = Adafruit_24bargraph();
Adafruit_24bargraph HumidityBar = Adafruit_24bargraph();

uint CO2Value = 0;
uint HumidityCounter = 0;

uint CloseButtonIsPressed = false;
uint OpenButtonIsPressed = false;
ulong IntervalHelper = 0;

boolean WindowIsOpen = false;

void setup(){

	Serial.begin( 57600 );

	TemperatureDisplay.begin( 9600 );
	SetTemperatureDisplayBrightness( 255 );

	HumiditySensor.begin();

	CO2Bar.begin( 0x70 );
	HumidityBar.begin( 0x71 );

	pinMode( BUTTON_CLOSE , INPUT_PULLUP );
	pinMode( BUTTON_OPEN , INPUT_PULLUP );
	pinMode( OUTPUT_CLOSE , OUTPUT );
	pinMode( OUTPUT_OPEN , OUTPUT );

	UpdateValues();
	IntervalHelper = millis();
}

void loop(){
	
	if( ( millis() - IntervalHelper ) >= UPDATE_INTERVAL ) {
		UpdateValues();
		IntervalHelper = millis();
	}

	CloseButtonIsPressed = digitalRead( BUTTON_CLOSE ); 
	OpenButtonIsPressed = digitalRead( BUTTON_OPEN ); 

	if( CloseButtonIsPressed == 0 ) {
		digitalWrite( OUTPUT_CLOSE , HIGH );
		WindowIsOpen = false;
	} else if( OpenButtonIsPressed == 0) {
		digitalWrite( OUTPUT_OPEN , HIGH );
		WindowIsOpen = true;
	} else if( CO2Value >= 1500 && WindowIsOpen == false ) {
		digitalWrite( OUTPUT_OPEN , HIGH );
		WindowIsOpen = false;
	} else if( CO2Value <= 1100 && WindowIsOpen == true ) {
		digitalWrite( OUTPUT_CLOSE , HIGH );
		WindowIsOpen = true;
	} else {
		digitalWrite( OUTPUT_OPEN , LOW );
		digitalWrite( OUTPUT_CLOSE , LOW );
	}

}

void UpdateValues(){
	Serial.println( "Update Values" );
	float HumidityF = HumiditySensor.readHumidity();
	int Humidity = (int)HumidityF;
	Serial.println( Humidity );

	float TemperatureF = HumiditySensor.readTemperature();
	int Temperature = (int)( TemperatureF * 100.0 );
	Serial.println( Temperature );

	SetCO2Bar( CO2Value);
    SetHumidityBar( Humidity );
    SetTemperatureDisplayNumber( Temperature );

    Serial.println( CO2Value );

    if( CO2Value < 3600 ) CO2Value += 150;
	else CO2Value = 0;
}

void SetCO2Bar( uint value ) {
	ubyte bars = value / 150;
	for( ubyte bar = 0 ; bar < bars ; bar++ ) {
		CO2Bar.setBar( bar , LED_OFF );
		if( bar < 10 ) CO2Bar.setBar( 24 - bar , LED_GREEN );
		if( bar >= 10 && bar < 17 ) {
			CO2Bar.setBar( 24 - bar , LED_RED );
			CO2Bar.setBar( 24 - bar , LED_YELLOW );
		}
		if( bar >= 17 && bar < 24 ) {
			CO2Bar.setBar( 24 - bar , LED_RED );
		}
	}
	for( ubyte bar = bars ; bar < 24 ; bar++ ) {
		CO2Bar.setBar( 24 - bar , LED_OFF );
	}
	CO2Bar.writeDisplay();
}

void SetHumidityBar( uint value ) {
	ubyte bars = (float)value / 4.1666;
	for( ubyte bar = 0 ; bar < bars ; bar++ ) {
		if( bar < 7 ) {
			HumidityBar.setBar( 24 - bar , LED_RED );
			HumidityBar.setBar( 24 - bar , LED_YELLOW );
		}
		if( bar >= 7 && bar < 14 ) {
			HumidityBar.setBar( 24 - bar , LED_GREEN );
		}
		if( bar >= 14 && bar < 24 ) {
			HumidityBar.setBar( 24 - bar , LED_RED );
		}
	}
	for( ubyte bar = bars ; bar < 24 ; bar++ ) {
		HumidityBar.setBar( 24 - bar , LED_OFF );
	}
	HumidityBar.writeDisplay();
}

void ClearTempeatureDisplay(){
	TemperatureDisplay.write( 0x76 );
}

void SetTemperatureDisplayBrightness( byte brightness ) {
	TemperatureDisplay.write( 0x7A );
	TemperatureDisplay.write( brightness );
}

void SetTemperatureDisplayDecimals( byte decimals ) {
	TemperatureDisplay.write( 0x77 );
	TemperatureDisplay.write( decimals );
}

void SetTemperatureDisplayNumber( int value ) {
	sprintf( TemperatureString , "%4d" , value );
	TemperatureDisplay.print( TemperatureString );
	SetTemperatureDisplayDecimals( 0b00000010 );
}