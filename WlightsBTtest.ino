// value storage in the Preferences-library

#include <Preferences.h>

Preferences preferences;

// Bluetooth serial library

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run 'make menu config' to and enable it
#endif

BluetoothSerial SerialBT;

// DHT library and pin

#include "DHT.h"
#define DHTPIN 22
#define DHTTYPE 11

// initialize DHT

DHT dht(DHTPIN, DHTTYPE);

// RGB pin numbers

const int ledPinR = 17;
const int ledPinG = 18;
const int ledPinB = 19;

// PWM properties and led channel ids

const int freq = 5000;
const int resolution = 8;

const int ledChannelR = 0;
const int ledChannelG = 1;
const int ledChannelB = 2;

// RGB variables

  int ledRedValue = 0;
  int ledGreenValue = 0;
  int ledBlueValue = 0;

// command string and color value string
String inCmd = "";
char incomingChar;
String colorValue;

// test variables
float ambTemp = 0;
float ambHumidity = 0;
float ambHeatIndex = 0;

void setup() {
  // put your setup code here, to run once:

  // start DHT

  dht.begin();

  // setup and attach led channels with PWM properties 

  ledcSetup(ledChannelR, freq, resolution);
  ledcSetup(ledChannelG, freq, resolution);
  ledcSetup(ledChannelB, freq, resolution);

  ledcAttachPin(ledPinR, ledChannelR);
  ledcAttachPin(ledPinG, ledChannelG);
  ledcAttachPin(ledPinB, ledChannelB);

  // shut leds initially

  ledcWrite(ledChannelR, 256);
  ledcWrite(ledChannelG, 256);
  ledcWrite(ledChannelB, 256);

  // create a serial connection and BT serial connection
  Serial.begin(115200);
  SerialBT.begin("Weatherlights"); // naming the BT device
  Serial.println("Device started, pair it with BT!");

  // open wlights namespace in RW and get RGB-values, if available

  preferences.begin("wlights", false);

  ledRedValue = preferences.getUInt("ledRedValue", 0);
  ledGreenValue = preferences.getUInt("ledGreenValue", 0);
  ledBlueValue = preferences.getUInt("ledBlueValue", 0);

  // close preferences

  preferences.end();
}

void loop() {
  // put your main code here, to run repeatedly:

  // observe events via serial connection, read inputs and return data or an error message

  if(SerialBT.available()) {
    incomingChar = SerialBT.read();
    if (incomingChar != '\n'){
      inCmd += String(incomingChar);
    }
    else{
      inCmd = "";
    }
    Serial.write(incomingChar);
    userInterface(inCmd);
  }
}

// UI commands for the serial console
void userInterface(String inCmd) {
  if (inCmd == "Lights on") {
    rgbLedControlOn();
  }
  else if(inCmd == "Lights off") {
    rgbLedControlOff();
  }
  else if(inCmd.indexOf("Colors") >= 0) {
    // if inCmd contains the word Colors, perform the color change method
    rgbLedColorChange();        
  }
  else if (inCmd == "Temperature") {
    heatAndHumidity();
    SerialBT.println("Temperature in °C is:");
    SerialBT.println(ambTemp);
  }
  else if (inCmd == "Humidity") {
    heatAndHumidity();
    SerialBT.println("Humidity % is:");
    SerialBT.println(ambHumidity);
  }
  else if (inCmd == "Heat index") {
    heatAndHumidity();
    SerialBT.println("Temperature in °C feels like:");
    SerialBT.println(ambHeatIndex);  
  } 
}

// turning leds on
void rgbLedControlOn() {
  // ledpins output N PWM duty cycle

  ledcWrite(ledChannelR, ledRedValue);
  ledcWrite(ledChannelG, ledGreenValue);
  ledcWrite(ledChannelB, ledBlueValue);

}

// turning leds off
void rgbLedControlOff() {
  //ledpins output 0 PWM duty cycle

  ledcWrite(ledChannelR, 256);
  ledcWrite(ledChannelG, 256);
  ledcWrite(ledChannelB, 256);
  
}

// changing colors, command format "Colors R:NNN G:NNN B:NNN"
void rgbLedColorChange() {

  // get the index numbers of R G B chars
  int iR = inCmd.indexOf('R');
  int iG = inCmd.indexOf('G');
  int iB = inCmd.indexOf('B');

  // derive 3-character substrings from index positions and convert them to int values
  // the used values are derived from the difference of 255 and input value
  // due to a common anode
  colorValue = inCmd.substring(iR+2, iR+5);
  int foundValue = colorValue.toInt();
  ledRedValue = 255-foundValue;
  
  colorValue = inCmd.substring(iG+2, iG+5);
  foundValue = colorValue.toInt();
  ledGreenValue = 255-foundValue;

  colorValue = inCmd.substring(iB+2, iB+5);
  foundValue = colorValue.toInt();
  ledBlueValue = 255-foundValue;

  // color change with the rgbLedControlOn-method
  rgbLedControlOn();

  // open preferences, store RGB-values and close preferences

  preferences.begin("wlights", false);

  preferences.putUInt("ledRedValue", ledRedValue);
  preferences.putUInt("ledGreenValue", ledGreenValue);
  preferences.putUInt("ledBlueValue", ledBlueValue);

  preferences.end();
  
}

// Temperature, humidity and heat index measuring

void heatAndHumidity() {

  float h = dht.readHumidity();
  
  float t = dht.readTemperature();

  // check if failed, error msg

  if (isnan(h) || isnan(t)) {
    SerialBT.println("Failed to read from DHT sensor!");
    return;
  }

  // heat index in celsius

  float hic = dht.computeHeatIndex(t, h, false);

  // set returnable variables

  ambTemp = t;
  ambHumidity = h;
  ambHeatIndex = hic;
  
}
