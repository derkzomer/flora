#define SSID        "your_wifi_ssid"
#define PASS        "your_wifi_password"
#define DEST_HOST   "www.derkzomer.com.au"
#define TIMEOUT     10000 // mS
#define CONTINUE    false
#define HALT        true
//#define ECHO_COMMANDS // Un-comment to echo AT+ commands to serial monitor

#include <RH_NRF24.h>
#include <SPI.h>
#include <SoftwareSerial.h>

SoftwareSerial Serial1(2, 3); // RX, TX

RH_NRF24 nrf24;

long prevTempMillis = 0;
long prevLightMillis = 0;
long prevMoistureMillis = 0;

long tempInterval = 900000;
long lightInterval = 900000;
long moistureInterval = 900000;

const int ledReceive = 6;
const int ledSend = 5;
char reqMsg[10];

int i = 0;
String data;

void setup()
{
  
  //Pin Modes
  pinMode(ledReceive,OUTPUT);
  pinMode(ledSend,OUTPUT);
  
  //Start general setup
  delay(1000);
  Serial.begin(9600);	// Debugging only
  Serial1.begin(9600);      // Communication with ESP8266 via 5V/3.3V level shifter
  Serial1.setTimeout(TIMEOUT);
  Serial.println("setup");

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
  
  //Wifi setup
    echoCommand("AT+RST", "ready", HALT);    // Reset & test if the module is ready  
    Serial.println("Module is ready.");
    echoCommand("AT+GMR", "OK", CONTINUE);   // Retrieves the firmware ID (version number) of the module. 
    echoCommand("AT+CWMODE?","OK", CONTINUE);// Get module access mode. 
    echoCommand("AT+CWMODE=1", "", HALT);    // Station mode

  //connect to the wifi
    boolean connection_established = false;
    for(int i=0;i<5;i++)
    {
      if(connectWiFi())
      {
        connection_established = true;
        break;
      }
    }
    if (!connection_established) errorHalt("Connection failed");
       
}

void transmission(char id[2])
{
  // Compile the message to be sent to request sensor data
  strcat(reqMsg, id);
      
  digitalWrite(ledSend, HIGH); // Flash a light to show transmitting
  Serial.println(reqMsg);
  Serial.println("Sending...");
//  delay(2000);
  nrf24.send((uint8_t *)reqMsg, strlen(reqMsg));
  nrf24.waitPacketSent();
//  Serial.println("Request sent.");
//  Serial.println("Turning back on receiver");
  digitalWrite(ledSend, LOW);
//  delay(1000);
  memset(reqMsg, 0, sizeof(reqMsg)); // Clear reqMsg
}

void loop()
{
      
  unsigned long curTempMillis = millis();
  unsigned long curLightMillis = millis() + 300000;
  unsigned long curMoistureMillis = millis() + 600000;

  if(curTempMillis - prevTempMillis > tempInterval) {
    Serial.println("Temperature function called");
    prevTempMillis = curTempMillis;
    transmission("t");
  }
  
  if(curLightMillis - prevLightMillis > lightInterval) {
    Serial.println("Light function called");
    prevLightMillis = curLightMillis;
    transmission("l");
  }  

  if(curMoistureMillis - prevMoistureMillis > moistureInterval) {
    Serial.println("Moisture function called");
    prevMoistureMillis = curMoistureMillis;
    transmission("m");
  }
  
  // Radio Reception  
  uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  if (nrf24.available())
    {
      digitalWrite(ledReceive, HIGH);
      if (nrf24.available())
      {
        // Should be a message for us now
        uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        if (nrf24.recv(buf, &len))
        {
          // NRF24::printBuffer("request: ", buf, len);
          Serial.print("Sensor responded: ");
          
          for (i = 0; i < len; i++)
          {
            char buffer =+ buf[i];
            String dataStr = String(buffer);
            data += dataStr;
          }
          
          Serial.println(data);
          digitalWrite(ledReceive, LOW);

          sendToServer(data);
        
        } else
        {
          Serial.println("recv failed");
        }
      }
      
      data = "";
    }
}

// WIFI FUNCTIONS START HERE  ---------------------------------------------------------------------------------------------------------------------

boolean sendToServer(String data){
  
  // Establish TCP connection
  String cmd = "AT+CIPSTART=\"TCP\",\""; cmd += DEST_HOST; cmd += "\",80";
  if (!echoCommand(cmd, "OK", CONTINUE)) return false;
  delay(2000);
    
  // Set CIPMODE=1
  String cip = "AT+CIPMODE=1";
  if (!echoCommand(cip, "OK", CONTINUE)) return false;
  delay(2000);
  
  // Get connection status
  if (!echoCommand("AT+CIPSTATUS", "OK", CONTINUE)) return false;
  // Build HTTP request.
  cmd = "POST /add.php HTTP/1.0\r\nHost: www.derkzomer.com.au:80\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: ";
  cmd += data.length();
  cmd += "\r\n";
  cmd += "Connection: Close";
  cmd += "\r\n\r\n";
  cmd += data;
  Serial.println(cmd);
  
  String cipstart = "AT+CIPSEND";
  delay(5000);

  if (!echoCommand(cipstart, ">", CONTINUE))
  {
    echoCommand("AT+CIPCLOSE", "", CONTINUE);
    Serial.println("Connection timeout.");
    return false;
  }
  delay(500);
  
  // Send the raw HTTP request
  echoCommand(cmd, "200", CONTINUE);  // GET
  delay(5000);
  Serial1.print("+++");
  delay(2000);
  echoCommand("AT+CIPCLOSE", "OK", CONTINUE);
  // Loop forever echoing data received from destination server.
//  while(true)
//    while (Serial1.available())
//      Serial.write(Serial1.read());
      
//  errorHalt("ONCE ONLY");
  // Send quit request
  delay(2000);
}

// Connect to the specified wireless network.
boolean connectWiFi()
{
  String cmd = "AT+CWJAP=\""; cmd += SSID; cmd += "\",\""; cmd += PASS; cmd += "\"";
  if (echoCommand(cmd, "OK", CONTINUE)) // Join Access Point
  {
    Serial.println("Connected to WiFi.");
    return true;
  }
  else
  {
    Serial.println("Connection to WiFi failed.");
    return false;
  }
}

boolean echoCommand(String cmd, String ack, boolean halt_on_fail)
{
  Serial1.println(cmd);
  #ifdef ECHO_COMMANDS
    Serial.print("--"); Serial.println(cmd);
  #endif
  
  // If no ack response specified, skip all available module output.
  if (ack == "")
    echoSkip();
  else
    // Otherwise wait for ack.
    if (!echoFind(ack))          // timed out waiting for ack string 
      if (halt_on_fail)
        errorHalt(cmd+" failed");// Critical failure halt.
      else
        return false;            // Let the caller handle it.
  return true;                   // ack blank or ack found
}

void echoSkip()
{
  echoFind("\n");        // Search for nl at end of command echo
  echoFind("\n");        // Search for 2nd nl at end of response.
  echoFind("\n");        // Search for 3rd nl at end of blank line.
}

void echoFlush()
  {while(Serial1.available()) Serial.write(Serial1.read());}
  
boolean echoFind(String keyword)
{
  byte current_char   = 0;
  byte keyword_length = keyword.length();
  
  // Fail if the target string has not been sent by deadline.
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline)
  {
    if (Serial1.available())
    {
      char ch = Serial1.read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println();
          return true;
        }
    }
  }
  return false;  // Timed out
}

void errorHalt(String msg)
{
  Serial.println(msg);
  Serial.println("HALT");
  while(true){};
}
