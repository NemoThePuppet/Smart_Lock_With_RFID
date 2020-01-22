#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h> 
#include <NTPClient.h>
#include <WiFiUdp.h>

// Defined pins for our ESP8266 (DO NOT USE D4)
#define SS_PIN D8       // RFID pin SS
#define RST_PIN D3      // RFID pin RST
#define lockPin D2      // Transistor pin
#define redLED D0
#define greenLED D1

// Global variables definitons
bool lockStatus = true;                   // true - lock is locked. false - lock is unlocked.
bool adminMode = false;                   // Flag for admin mode initialization. (whether admin Mode was set)
bool timeSet = false;                     // Flag for admin mode timer. (whether timer was set)
const int adminModeTime = 5000;           // Duration of admin mode set for 5 seconds.
unsigned long startTime = 0;              // Time of admin mode initialization.
unsigned long currentTime = 0;            // Current time during admin mode.
String stringTagUID = "";                 // String to hold scanned UID.
String masterKeyUID = "3B0347D5";         // Master key UID (used for admin mode initialization).

// WiFi connection
const char* ssid     = "Redmi";           // The SSID (name) of the Wi-Fi network we connect to.
const char* password = "12345678";        // The password of the Wi-Fi network.

// RFID Reader initialization
MFRC522 rfid(SS_PIN, RST_PIN);            // Creating of instance of the RFID reader.


// Define NTP Client to get time
const long utcOffsetInSeconds = 21600;     // UTC+1 Time Zone
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
  Serial.println("");
  WiFi.begin(ssid, password);                 // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {     // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());             // Send the IP address of the ESP8266 to the computer
  Serial.println("");
  
  timeClient.begin();
}

void loop() {
  digitalWrite(lockPin,HIGH);
  lockStatus = true;
  lightLED(lockStatus, adminMode);

  /* 
   *  Following statement checks whether timer for admin mode was set. If it was,
   *  it checks whether admin mode is running for 5 seconds or longer. 
   *  If it is, admin mode is turned off.
   */
   
  if(timeSet){
    currentTime = millis();
    if((currentTime - startTime)>=5000){
      adminMode = false;
      timeSet = false;
    }
  }
  
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;
    
  // Store NUID into String
  stringTagUID = nuidToString(rfid.uid.uidByte, 4);
  
  // Serial printing for debugging/additional info
  Serial.print(F("NUID tag: "));
  Serial.println(stringTagUID);
  
  if(stringTagUID == masterKeyUID){
    adminMode = true; 
  }
  
  String result = "";
  
  if(adminMode){
    
    // Halt PICC
    rfid.PICC_HaltA();

    // Set timer to measure duration of admin mode.
    if(!timeSet){
      Serial.println("Start timer!");
      startTime = millis();
      currentTime = startTime;
      timeSet = true;
    }
    
    Serial.println("-----Admin mode initialized!-----");
    Serial.println("");
    
    if(stringTagUID != masterKeyUID){
      result = sendQuery("GET", stringTagUID);
      
      if(result == "0"){
        // SEND POST REQUEST TO REGISTER NEW TAG UID
        Serial.println("Registering new UID...");
        result = sendQuery("POSTreg", stringTagUID);
      }
      else if(result == "1" || result == "2"){
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

/*****
Purpose: Send a POST request to the HTTP server

Parameters:
String query - String of a query to be sent

Return value:
String result - Answer from the server after the query was sent
****/
String Post(String query) {
  Serial.println("Query is " + query);
  
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

/*****
Purpose: Send a GET request to the HTTP server

Parameters:
String query - String of a query to be sent

Return value:
String result - Answer from the server after the query was sent
****/
String Get(String query) {
  Serial.println("Query: " + query);
  
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

/*****
Purpose: Light two LEDs (red and green) based on the current state of the lock

Parameters:
bool lockStatus - Describes whether lock is locked (true) or unlocked (false)
bool adminMode - Describes whether admim mode is set (true) or not (false)

Return value:
void
****/
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

/*****
Purpose: Locks/unlocks the lock and changes variable lockStatus accordingly.

Parameters:
String accessLevel - accessLevel of the tag
bool* lockStatus - Describes whether lock is locked (true) or unlocked (false)

Return value:
void
****/
void action(String accessLevel, bool* lockStatus){
  if(accessLevel == "0"){
    *lockStatus = true;
  }else if(accessLevel == "2"){
    digitalWrite(lockPin, LOW);
    *lockStatus = false;
    lightLED(*lockStatus, false);
    delay(2000);
  }else if(accessLevel == "1"){
    timeClient.update(); // Checks current time using Network Time Protocol (NTP)
    /*
     * Following statements checks whether it's weekday and between 8:00 - 18:00. If it is, lock is unlocked. 
     */
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


/*****
Purpose: Creates query, sends it to the server and returns answer from the server. 

Parameters:
String request - What type of request should be sent. (GET, POSTreg, POSTdereg).
String stringTagUID - UID of the tag.

Return value:
String result - Answer from the server after the query was sent
****/
String sendQuery(String request, String stringTagUID){
  String result = "";
  String query = "http://rfidlock.azurewebsites.net/data.php?tagUID='";
  query.concat(stringTagUID);
  query.concat("'");
  if(request == "GET"){
    result = Get(query);
  }
  else if(request == "POSTreg"){            // POST request to register new tag UID to the DB
    query.concat("&register=1");
    result = Post(query);
  }else if(request == "POSTdereg"){         // POST request to deregister a tag UID from the DB
    result = Post(query);
  }
  Serial.print("Result: ");
  Serial.println(result);
  Serial.println("");
  return result;
}

/*****
Purpose: Creates a String variable from the byte array holding a tag UID.

Parameters:
byte* nuidArray - Pointer to a byte array.
int sizeOfArray - Size of a byte array.

Return value:
String tagID - Tag UID.
****/
String nuidToString(byte* nuidArray, int sizeOfArray){
  String tagID = "";
    for (byte i = 0; i < sizeOfArray; i++) {
      String temp = "";
      if ((int)nuidArray[i] < 10) {
        temp += "0";
      }
      temp += String((int)nuidArray[i], HEX);
      temp.toUpperCase();
      tagID.concat(temp);
  }
  return tagID;
}

  
