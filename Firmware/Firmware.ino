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

#define SEVEN_SEGMENT_TX 8
#define SEVEN_SEGMENT_RX 7

char TemperatureString[10]; 

SoftwareSerial TemperatureDisplay( SEVEN_SEGMENT_RX , SEVEN_SEGMENT_TX );

DHT HumiditySensor( HUMIDITY_SENSOR_SIG , HUMIDITY_SENSOR_TYPE );

Adafruit_24bargraph CO2Bar = Adafruit_24bargraph();
Adafruit_24bargraph HumidityBar = Adafruit_24bargraph();

uint CO2Counter = 0;
uint HumidityCounter = 0;

void setup(){

	TemperatureDisplay.begin( 9600 );
	SetTemperatureDisplayBrightness( 200 );

	HumiditySensor.begin();

	CO2Bar.begin( 0x70 );
	HumidityBar.begin( 0x71 );	
}

void loop(){
	
	float HumidityF = HumiditySensor.readHumidity();
	int Humidity = (int)HumidityF;

	float TemperatureF = HumiditySensor.readTemperature();
	int Temperature = (int)( TemperatureF * 100.0 );

    SetCO2Bar( CO2Counter );
    SetHumidityBar( Humidity );
    SetTemperatureDisplayNumber( Temperature );

	if( CO2Counter < 3600 ) CO2Counter += 150;
	else CO2Counter = 0;

	delay( 1000 );

}

void SetCO2Bar( uint value ) {
	ubyte bars = value / 150;
	for( ubyte bar = 0 ; bar < bars ; bar++ ) {
		if( bar < 10 ) CO2Bar.setBar( bar , LED_GREEN );
		if( bar >= 10 && bar < 17 ) {
			CO2Bar.setBar( bar , LED_RED );
			CO2Bar.setBar( bar , LED_YELLOW );
		}
		if( bar >= 17 && bar < 24 ) {
			CO2Bar.setBar( bar , LED_RED );
		}
	}
	for( ubyte bar = bars ; bar < 24 ; bar++ ) {
		CO2Bar.setBar( bar , LED_OFF );
	}
	CO2Bar.writeDisplay();
}

void SetHumidityBar( uint value ) {
	ubyte bars = (float)value / 4.1666;
	for( ubyte bar = 0 ; bar < bars ; bar++ ) {
		if( bar < 7 ) {
			HumidityBar.setBar( bar , LED_RED );
			HumidityBar.setBar( bar , LED_YELLOW );
		}
		if( bar >= 7 && bar < 14 ) {
			HumidityBar.setBar( bar , LED_GREEN );
		}
		if( bar >= 14 && bar < 24 ) {
			HumidityBar.setBar( bar , LED_RED );
		}
	}
	for( ubyte bar = bars ; bar < 24 ; bar++ ) {
		HumidityBar.setBar( bar , LED_OFF );
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