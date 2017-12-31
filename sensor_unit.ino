#include <LowPower.h>

#include <RH_NRF24.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

RH_NRF24 nrf24;

long prevTransmissionMillis = 0;
long transmissionInterval = 900000;

// Radio defs
const int ledReceive = 6;
const int ledSend = 5;

// Temperature defs
#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress thermometer;

// Moisture defs
const int moisturePin = A1; // Analog pin A3
const int moistureChargePin = 2;

// Light defs
const int lightPin = A0;

void setup()
{
  Serial.begin(9600);
  sensors.begin();
  
  // Pin modes
  pinMode(moistureChargePin,OUTPUT); // Moisture
  pinMode(ledReceive,OUTPUT); // Radio
  pinMode(ledSend,OUTPUT);
  Serial.println("Pin Modes Set");
  
  // Radio setup
    if (!nrf24.init()){
      Serial.println("radio init failed");
    } else{
      Serial.println("radio init success");
    }
    
    // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
    if (!nrf24.setChannel(1)){
      Serial.println("setChannel failed");
    }    
    if (!nrf24.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm)){
      Serial.println("setRF failed");
    }
  
  // Temperature setup
  Serial.println("Locating temperature device...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(),DEC);
  Serial.println(" device.");
  if (!sensors.getAddress(thermometer, 0)) Serial.println("Unable to find address for temperature device");
  Serial.print("temperature sensor Address: ");
  printAddress(thermometer);

  Serial.println();
  Serial.println();
}

void sleep(int multiplier)
{
  for (int i = 0; i < multiplier; i++) { 
     LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
  }
}

int pairing(int timeout) {
  int interval;
  
  
  
  return interval;
}

// Temperature address function
void printAddress(DeviceAddress deviceAddress)
{
  for (int i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

float tempRequest(){
  sensors.requestTemperatures();
  float tempVal = sensors.getTempC(thermometer);
  Serial.print("Temp C: ");
  Serial.println(tempVal);
  delay(1000);
  return tempVal;
}

float moistureRequest(){
  digitalWrite(moistureChargePin,HIGH);
  delay(500);
  float moistureVal = analogRead(moisturePin);
  Serial.print("Moisture Value: ");
  Serial.println(moistureVal);
  pinMode(moistureChargePin,LOW);
  delay(1000);
  return moistureVal;
}

float lightRequest(){
  float lightVal = analogRead(lightPin);
  Serial.print("Light value: ");
  Serial.println(lightVal);
  delay(1000);
  return lightVal;
}

boolean transmission() {
  
  char msg[20] = "";
  
  char tempStr[6];
  Serial.println("temperature requested");
  float tempVal = tempRequest();
  dtostrf(tempVal, 5, 2, tempStr);
  strcat(msg,"t=");
  strcat(msg,tempStr);

  char lightStr[7];
  Serial.println("light requested");
  float lightVal = lightRequest();
  dtostrf(lightVal, 6, 2, lightStr);
  strcat(msg,"&l=");
  strcat(msg,lightStr);

  char moistureStr[7];
  float moistureVal = moistureRequest();
  dtostrf(moistureVal, 6, 2, moistureStr);
  Serial.println("moisture requested");
  strcat(msg,"&m=");
  strcat(msg,moistureStr);

  digitalWrite(ledSend, HIGH); // Flash a light to show transmitting
  nrf24.send((uint8_t *)msg, strlen(msg));
  delay(1000);
  Serial.println("Sending data...");
  Serial.print("Message: \"");
  Serial.print(msg);
  Serial.println("\"");
  if (nrf24.waitPacketSent()){
    Serial.println("waitPacketSent success");
  } else{
    Serial.println("waitPacketSent fail");
  }
  memset(msg, 0, sizeof(msg)); // Clear msg
  digitalWrite(ledSend, LOW);
  delay(1000);
  return true;
}

void ack() {
  
  // Radio Reception  
  uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  for (int i = 0; i < 50; i++) {
  
    if (nrf24.recv(buf, &len)) // Non-blocking
    {
      int i;
    
      String ack = "";
      char raw[50];
      char data[50];
      
      for (i = 0; i < len; i++)
      {
        char data =+ buf[i];
        String dataStr = String(data);
        ack += dataStr;
      }
      
      if (ack == "ok") {
        Serial.println("Acknowledgement received");
        i = 49;
      }
  
      Serial.println();
      
    } else {
      
      Serial.println("No acknowledgement");
      if (i = 49) {
        Serial.println("Data logged to memory");
      }
    }
  }
}

int pair() {
  
  // Radio Reception  
  uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  for (int i = 0; i < 50; i++) {
  
    if (nrf24.recv(buf, &len)) // Non-blocking
    {
      int i;
    
      String ack = "";
      char raw[50];
      char data[50];
      
      for (i = 0; i < len; i++)
      {
        char data =+ buf[i];
        String dataStr = String(data);
        ack += dataStr;
      }
      
      if (ack == "ok") {
        Serial.println("Acknowledgement received");
        i = 49;
      }
  
      Serial.println();
      
    } else {
      
      Serial.println("No acknowledgement");
      if (i = 49) {
        Serial.println("Data logged to memory");
      }
    }
  }
}

void loop()
{
  
  transmission();
  ack();
  int interval = pair();
  sleep(interval);

}
