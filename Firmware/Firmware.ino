#include <Wire.h>
#include <SoftwareSerial.h>
#include <SevSeg.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <DHT.h>
#include <kSeries.h>
#include <EEPROM.h>

#define BUTTON_OPEN_PIN 3
#define OUTPUT_OPEN_PIN 5
#define BUTTON_CLOSE_PIN 2
#define OUTPUT_CLOSE_PIN 4

#define SEVEN_SEGMENT_TX_PIN 10
#define SEVEN_SEGMENT_RX_PIN 11

#define CO2_INTERVAL 150
#define SAMPLE_INTERVAL 1000

#define MODE_AUTOMATIC 0
#define MODE_MANUAL 1

#define CO2_LOWER 1200
#define CO2_UPPER 1500

#define HUMIDITY_UPPER 60
#define HUMIDITY_LOWER 30

#define WINDOW_OPEN 0
#define WINDOW_CLOSED 1

SevSeg SevenSegment = SevSeg();

Adafruit_24bargraph CO2Bar = Adafruit_24bargraph();

Adafruit_24bargraph HumidityBar = Adafruit_24bargraph();

DHT HumiditySensor( A0 , DHT22 );

kSeries CO2Sensor( 13 , 12 );

long SampleIntervalHelper = 0;

byte OperationMode = 0;
byte WindowMode = 0;

bool CloseButtonPressed = false;
bool OpenButtonPressed = false;

volatile int Humidity;
volatile int CO2Value;

void setup(){

	SevenSegment.Init( &Serial1 );
	SevenSegment.Clear();
	SevenSegment.Brightness( 255 );
	SevenSegment.Text( " HI " );

	pinMode( BUTTON_CLOSE_PIN , INPUT_PULLUP );
	pinMode( BUTTON_OPEN_PIN , INPUT_PULLUP );

	pinMode( OUTPUT_CLOSE_PIN , OUTPUT );
	pinMode( OUTPUT_OPEN_PIN , OUTPUT );

	HumiditySensor.begin();

	CO2Bar.begin( 0x70 );
	CO2Bar.clear();
	CO2Bar.setBrightness( 15 );

	delay( 500 );

	int CO2Value = 0;
	while( CO2Value <= 3600 ) {
		DisplayCO2Bar( CO2Value );
		CO2Value = CO2Value + 150;
		delay( 10 );
	}

	HumidityBar.begin( 0x71 );
	HumidityBar.clear();
	HumidityBar.setBrightness( 15 );

	delay( 500 );

	int HumidityValue = 0;
	while( HumidityValue <= 100 ) {
		DisplayHumidityBar( HumidityValue );
		HumidityValue = HumidityValue + 10;
		delay( 10 );
	}

	delay( 500 );

	for( byte i = 0 ; i < 24 ; i++ ) {
		CO2Bar.setBar( i , LED_OFF );
		CO2Bar.writeDisplay();

		HumidityBar.setBar( i , LED_OFF );
		HumidityBar.writeDisplay();

		delay( 10 );
	}

	SevenSegment.Clear();
	SevenSegment.Text( "CLSE" );
	digitalWrite( OUTPUT_CLOSE_PIN , HIGH );
	delay( 2000 );
	digitalWrite( OUTPUT_CLOSE_PIN , LOW );
	WindowMode = WINDOW_CLOSED;
	delay( 10000 );
	SevenSegment.Clear();
	SevenSegment.Text( "DONE" );

	delay( 1000 );

	byte mode = EEPROM.read( 100 );
	if( mode == MODE_MANUAL ) {
		SetMode( MODE_MANUAL );
	} else if( mode == MODE_AUTOMATIC ) {
		SetMode( MODE_AUTOMATIC );
	} else {
		SetMode( MODE_AUTOMATIC );
		EEPROM.write( 100 , OperationMode );
	}

	UpdateValues();

	SampleIntervalHelper = millis();

}

void loop() {

	long TimeDifference = millis() - SampleIntervalHelper;

	if( TimeDifference > SAMPLE_INTERVAL ) {
		UpdateValues();
		SampleIntervalHelper = millis();
	}

	int CloseButton = digitalRead( BUTTON_CLOSE_PIN );
	int OpenButton = digitalRead( BUTTON_OPEN_PIN );

	if ( CloseButton == LOW && OpenButton == LOW ) {
		ChangeMode();
		UpdateValues();
	} else if( OperationMode == MODE_AUTOMATIC ) {
		if( CloseButton == LOW ) {
			CloseButtonPressed = true;
			digitalWrite( OUTPUT_CLOSE_PIN , HIGH );
			WindowMode = WINDOW_CLOSED;
			delay( 500 );
			ChangeMode();
		} else if( OpenButton == LOW ) {
			OpenButtonPressed = true;
			digitalWrite( OUTPUT_OPEN_PIN , HIGH );
			WindowMode = WINDOW_OPEN;
			delay( 500 );
			ChangeMode();
		} else {
			if( CO2Value > CO2_UPPER && WindowMode == WINDOW_CLOSED ) {
				digitalWrite( OUTPUT_OPEN_PIN , HIGH );
				WindowMode = WINDOW_OPEN;
				delay( 1000 );
				digitalWrite( OUTPUT_OPEN_PIN , LOW );
			} else if( CO2Value < CO2_LOWER && WindowMode == WINDOW_OPEN ) {
				digitalWrite( OUTPUT_CLOSE_PIN , HIGH );
				WindowMode = WINDOW_CLOSED;
				delay( 1000 );
				digitalWrite( OUTPUT_CLOSE_PIN , LOW );
			} else if( Humidity > 60  && WindowMode == WINDOW_CLOSED ) {
				digitalWrite( OUTPUT_OPEN_PIN , HIGH );
				WindowMode = WINDOW_OPEN;
				delay( 1000 );
				digitalWrite( OUTPUT_OPEN_PIN , LOW );
			}
		}
	} else if( OperationMode == MODE_MANUAL ) {
		if( CloseButton == LOW && CloseButtonPressed == false ) {
			CloseButtonPressed = true;
			digitalWrite( OUTPUT_CLOSE_PIN , HIGH );
			WindowMode = WINDOW_OPEN;
		} else if( CloseButton == HIGH && CloseButtonPressed == true ) {
			CloseButtonPressed = false;
			digitalWrite( OUTPUT_CLOSE_PIN , LOW );
		} if( OpenButton == LOW && OpenButtonPressed == false && CloseButtonPressed == false ) {
			OpenButtonPressed = true;
			digitalWrite( OUTPUT_OPEN_PIN , HIGH );
			WindowMode = WINDOW_CLOSED;
		} else if( OpenButton == HIGH && OpenButtonPressed == true ) {
			OpenButtonPressed = false;
			digitalWrite( OUTPUT_OPEN_PIN , LOW );
		}
	}

	delay( 100 );

}

void UpdateValues(){
	float TemperatureF = HumiditySensor.readTemperature();
	SevenSegment.Decimals( 0b00000010 );
	SevenSegment.Float( TemperatureF );

	float HumidityF = HumiditySensor.readHumidity();
	Humidity = int( HumidityF );
	DisplayHumidityBar( Humidity );

	float CO2ValueF = CO2Sensor.getCO2( 'p' );
	if( CO2ValueF > 0.0 && CO2ValueF < 10000.0 ) CO2Value = int( CO2ValueF );
	DisplayCO2Bar( CO2Value );
}

void DisplayCO2Bar( int Value ) {
	byte Bars = Value / CO2_INTERVAL;
	for( byte Bar = 0 ; Bar < Bars ; Bar++ ) {
		if( Bar < 10 ) {
			CO2Bar.setBar( 23 - Bar , LED_GREEN );
		} else if( Bar >= 10 && Bar < 17 ) {
			CO2Bar.setBar( 23 - Bar , LED_RED );
			CO2Bar.setBar( 23 - Bar , LED_YELLOW );
		} else if( Bar >= 17 && Bar < 24 ) {
			CO2Bar.setBar( 23 - Bar , LED_RED );
		}
	}
	for( byte Bar = Bars ; Bar < 24 ; Bar++ ) {
		CO2Bar.setBar( 23 - Bar , LED_OFF );
	}
	CO2Bar.writeDisplay();
}

void DisplayHumidityBar( int Value ) {
	byte Bars = float( Value ) / 4.16;
	for( byte Bar = 0 ; Bar < Bars ; Bar++ ) {
		if( Bar < 7 ) {
			HumidityBar.setBar( 23 - Bar , LED_RED );
			HumidityBar.setBar( 23 - Bar , LED_YELLOW );
		} else if( Bar >= 7 && Bar < 14 ) {
			HumidityBar.setBar( 23 - Bar , LED_GREEN );
		} else if( Bar >= 14 && Bar < 24 ) {
			HumidityBar.setBar( 23 - Bar , LED_RED );
		}
	}
	for( byte Bar = Bars ; Bar < 24 ; Bar++ ) {
		HumidityBar.setBar( 23 - Bar , LED_OFF );
	}
	HumidityBar.writeDisplay();
}

void ChangeMode( ) {
	if( OperationMode == MODE_MANUAL ) {
		SetMode( MODE_AUTOMATIC );
	} else if( OperationMode == MODE_AUTOMATIC ) {
		SetMode( MODE_MANUAL );
	}
	EEPROM.write( 100 , OperationMode );
}

void SetMode( byte mode ) {
	if( mode == MODE_MANUAL ) {
		OperationMode = MODE_MANUAL;
		SevenSegment.Clear();
		SevenSegment.Text( "HAND" );
	} else if( mode == MODE_AUTOMATIC ) {
		OperationMode = MODE_AUTOMATIC;
		SevenSegment.Clear();
		SevenSegment.Text( "AUTO" );
	}
	delay( 1000 );
	SevenSegment.Clear();
}