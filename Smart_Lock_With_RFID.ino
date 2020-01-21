#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <ESP8266HTTPClient.h> 
#include <NTPClient.h>
#include <WiFiUdp.h>

// Defined pins for ESP8266 (DO NOT USE D4)
#define SS_PIN D8       // RFID pin SS
#define RST_PIN D3      // RFID pin RST
#define lockPin D2       // Transistor pin
#define redLED D0
#define greenLED D1

String stringTagUID = "";
String masterKeyUID = "59371213";

// WiFi connection
const char* ssid     = "Redmi";           // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "12345678";        // The password of the Wi-Fi network

// RFID Reader initialization
MFRC522 rfid(SS_PIN, RST_PIN);            // Instance of the class
//byte nuidPICC[4];                         // Init array that will store new NUID 

bool lockStatus = true;                   // 1 - locked. 0 - unlocked.
bool adminMode = false;


// Define NTP Client to get time
const long utcOffsetInSeconds = 29000; // UTC+1 Time Zone
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup() {
  Serial.begin(9600);         // Start the Serial communication to send messages to the computer
  SPI.begin();                // Init SPI bus
  rfid.PCD_Init();            // Init MFRC522 
  delay(10);

  // Lock mechanism initialization
  pinMode(lockPin,OUTPUT);
  pinMode(redLED,OUTPUT);
  pinMode(greenLED,OUTPUT);
  
  // WiFi connection initialization
  Serial.println('\n');
  WiFi.begin(ssid, password);                 // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  timeClient.begin();
}

void loop() {
  digitalWrite(lockPin,HIGH);
  lockStatus = true;
  lightLED(lockStatus, adminMode);
  stringTagUID = "";
  
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    //nuidPICC[i] = rfid.uid.uidByte[i];
    stringTagUID.concat(rfid.uid.uidByte[i]);
  }
  
  
  // Serial printing for debugging/additional info
  Serial.println(F("The NUID tag is:"));
  Serial.print(F("String: "));
  Serial.println(stringTagUID);
  Serial.print(F("In orig: "));
  for (byte i = 0; i < 4; i++) {
    Serial.print(rfid.uid.uidByte[i]);
    Serial.print(" ");
  }
  Serial.println("");
  Serial.print(F("In hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  Serial.println();
  
  if(stringTagUID == masterKeyUID){
    adminMode = true; 
  }
  
  String result = "";
  
  if(adminMode){
    
    // Halt PICC
    rfid.PICC_HaltA();
    
    Serial.println("Admin mode initialized!");
    if(stringTagUID != masterKeyUID){
      result = sendQuery("GET", stringTagUID);
      
      if(result == "0"){
        // SEND POST REQUEST TO REGISTER NEW TAG UID
        Serial.println("Registering new UID...");
        result = sendQuery("POSTreg", stringTagUID);
        
      }
      else if(result == "1"){
        // SEND POST REQUEST TO DEREGISTER NEW TAG UID
        Serial.println("Deregistering UID...");
        result = sendQuery("POSTdereg", stringTagUID);
      }else{
        Serial.println("ERROR: Unreadable result. Wait and try again.");
      }
      adminMode = false;
    }
    return;
  }
  else if(!adminMode){
    result = sendQuery("GET", stringTagUID);
  }
  
  action(result, &lockStatus);
  
  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  
  
  }


String Post(String query) {
  Serial.println("query is " + query);
  
  HTTPClient http;
  http.begin(query);
  int httpCode = http.POST(query);
  
  String result = "";
  if (httpCode == 200) {
      Serial.println("POST success");
      result = http.getString();
  } else {
      Serial.println("ERROR with httpcode: " + httpCode);
  }
  http.end();
  return result;
}

String Get(String query) {
  Serial.println("query is " + query);
  
  HTTPClient http;
  http.begin(query);
  int httpCode = http.GET();
  
  String result = "";
  if (httpCode == 200) {
      Serial.println("GET success");
      result = http.getString();
  } else {
      Serial.println("ERROR with httpcode: " + httpCode);
  }
  http.end();
  return result;
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void lightLED(bool lockStatus, bool adminMode){
  if(adminMode){
    digitalWrite(greenLED,HIGH);
    digitalWrite(redLED,HIGH);
  }
  else if(lockStatus){
    digitalWrite(greenLED,LOW);
    digitalWrite(redLED,HIGH);
  }else{
    digitalWrite(redLED,LOW);
    digitalWrite(greenLED,HIGH);
  }
}

void action(String result, bool* lockStatus){
  if(result == "0"){
    *lockStatus = true;
  }else if(result == "2"){
    digitalWrite(lockPin, LOW);
    *lockStatus = false;
    lightLED(*lockStatus, false);
    delay(2000);
  }else if(result == "1"){
    timeClient.update();
    
    // Check if it's not weekend (Sunday = 0, Saturday = 6) and if it's between 8:00 and 18:00.
    if(timeClient.getDay()>=1 && timeClient.getDay()<=5 && timeClient.getHours()>=8 && timeClient.getHours()<=17){
      digitalWrite(lockPin, LOW);
      *lockStatus = false;
      lightLED(*lockStatus, false);
      delay(2000);
      }
  }else{
    Serial.println("ERROR: Unreadable result. Wait and try again.");
  }
}

String sendQuery(String request, String stringTagUID){
  String result = "";
  String query = "http://rfidlock.azurewebsites.net/data.php?tagUID='";
  query.concat(stringTagUID);
  query.concat("'");
  if(request == "GET"){
    result = Get(query);
  }
  else if(request == "POSTreg"){
    query.concat("&register=1");
    result = Post(query);
  }else if(request == "POSTdereg"){
    result = Post(query);
  }
  Serial.print("Result: ");
  Serial.println(result);
  return result;
}
