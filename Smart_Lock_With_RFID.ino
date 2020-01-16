#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <ESP8266HTTPClient.h> 

const char* ssid     = "Redmi";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "12345678";      // The password of the Wi-Fi network
const int records = 1;                  // 0 = Get all records from the DB. 1 = Get only TagUID, AccessLevel.


void setup() {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
  
  WiFi.begin(ssid, password);             // Connect to the network
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

  String getQuery = "http://rfidlock.azurewebsites.net/data.php?records=";
  getQuery = getQuery + records;
  String result = Get(getQuery); // 
  Serial.println("Result: ");
  Serial.println(result);

}

void loop() {}


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
