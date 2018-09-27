// Open Source - use as you would like. 
// This example creates a server and exposes different pins, functions, and variables for a garage door opener linked with voice biometric authentication
// In this example, you have:
// a server listen on port 4444 to provide API
// HTTP client to query an external source over wireless 
// JSON Serializer 
// this project is expecting a Feather Huzzah.

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <aREST.h>

// Create aREST instance
aREST rest = aREST();

// Website connection param
String host = "example.website.io";
const int httpsPort = 443;
const char* fingerprint = "9b 34 55 88 88 03 9c 31 22 03 fe fe 8b a1 56 17 3b d4 d3 0e";

// Min value of trust.
float confidence_level = 0.90;


// WiFi parameters
const char* ssid = "ssid";
const char* password = "password";
String bearerToken = "Bearer example";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           4444

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Variables to be exposed to the API
const char* buildVersion;
const char* knownApiEndpoints;
const char* exampleCall;
String redLed;
String blueLed;
String yellowLed;
String doorButton;


// Declare functions to be exposed to the API
int selfTest(String command);
int pinState(String pin);
int setPinOn(String pin);
int setPinOff(String pin);
int checkConfidence(String uuid);
int openGarageDoor(String uuid);

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  // Init variables and expose them to REST API
  buildVersion = "1.0.0-Beta";
  knownApiEndpoints = "test, state, on, off, validity, openGarageDoor";
  exampleCall = "http://<ip>/[endpoint]?pin=<id>";
  redLed = "14";
  blueLed = "12";
  yellowLed = "13";
  doorButton = "15";
  rest.variable("buildVersion",&buildVersion);
  rest.variable("redLed",&redLed);
  rest.variable("blueLed",&blueLed);
  rest.variable("yellowLed",&yellowLed);
  rest.variable("knownApiEndpoints",&knownApiEndpoints);
  rest.variable("exampleCall",&exampleCall);
  rest.variable("host",&host);

  // Function to be exposed
  rest.function("test",selfTest);
  rest.function("state",getPinState);
  rest.function("on",setPinOn);
  rest.function("off",setPinOff);
  rest.function("validity",checkConfidence);
  rest.function("openGarageDoor",openGarageDoor);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("proto 1");
  rest.set_name("garage_door");

  // Set all used pins to OUTPUT
  pinMode(redLed.toInt(), OUTPUT);
  pinMode(blueLed.toInt(), OUTPUT);
  pinMode(yellowLed.toInt(), OUTPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);

}

int selfTest (String command) {
  Serial.println("Starting self tests.");
  pinMode(redLed.toInt(), OUTPUT);  //Red
  pinMode(blueLed.toInt(), OUTPUT);  //Blue
  pinMode(yellowLed.toInt(), OUTPUT);  //Yellow

  digitalWrite(redLed.toInt(), HIGH);
  digitalWrite(blueLed.toInt(), HIGH);
  digitalWrite(yellowLed.toInt(), HIGH);
  delay(1000);

  digitalWrite(redLed.toInt(), LOW);
  digitalWrite(blueLed.toInt(), LOW);
  digitalWrite(yellowLed.toInt(), LOW);
  delay(1000);

  digitalWrite(redLed.toInt(), HIGH);  //Red
  delay(1000);
  digitalWrite(blueLed.toInt(), HIGH);  //Blue
  delay(1000);
  digitalWrite(yellowLed.toInt(), HIGH);  //Yellow
  delay(1000);
  digitalWrite(redLed.toInt(), LOW);
  delay(1000);
  digitalWrite(blueLed.toInt(), LOW);
  delay(1000);
  digitalWrite(yellowLed.toInt(), LOW);

  Serial.println("Self tests complete.");
  return 0;
}

// Pulls the in voltage state. Note, a pin in INPUT mode is always high!
int getPinState(String pin) {
  String StrMessage = "Retrieving state for pin: " + pin;
  Serial.println(StrMessage);
  
  int val = 0;
  val = digitalRead(pin.toInt());
  Serial.println(val);
  
  return val;
}

int setPinOn(String pin) {
  String StrMessage = "Setting pin " + pin + " to HIGH.";
  Serial.println(StrMessage);
  
  digitalWrite(pin.toInt(), 1);
  
  return 0;
}

int setPinOff(String pin) {
  String StrMessage = "Setting pin " + pin + " to LOW.";
  Serial.println(StrMessage);
  
  digitalWrite(pin.toInt(), 0);
  
  return 0;
}

int checkConfidence(String uuid) {
  String StrMessage = "Checking uuid: " + uuid;
  Serial.println(StrMessage);

  String completeURI = "https://" + host + "/v1/a[i/" + uuid ;
    
  HTTPClient http;
  
  int beginResult = http.begin(completeURI, fingerprint);
  http.addHeader("Authorization", bearerToken);
  
  int httpCode = http.GET();
  String response = http.getString();
  
  Serial.print("Initial HTTP connection?: ");
  Serial.print(beginResult);
  Serial.println();
  
  Serial.print("http: ");
  Serial.print(httpCode);
  Serial.println();
  
  Serial.print("response: ");
  Serial.println(response);
  Serial.println("------Processing JSON------");
  
  http.end();
  // attempt to extract value. 
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  
  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("Failed to parse JSON...");
    return 1;
  }

  // Fetch values.
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do root["time"].as<long>();
  float confidence = root["identity"]["confidence"];
  const char* userId = root["identity"]["user"];

  Serial.print("Confidence Score: ");
  Serial.println(confidence, 6);
  Serial.print("User UUID: ");
  Serial.println(userId);

  int returnValue = 0;
  if (confidence > confidence_level) {
    returnValue = 1;
  }

  return returnValue;
}

int openGarageDoor(String uuid) {
  String StrMessage = "--------------- Open Garage Door request for " + uuid + " ---------------";
  Serial.println(StrMessage);

  if (checkConfidence(uuid)) {
    Serial.println("Trusted - Open Door.");
    setPinOn(blueLed);
    delay(5000);
    setPinOff(blueLed);
  } else {
    Serial.println("Untrusted.");
    setPinOn(redLed);
    delay(5000);
    setPinOff(redLed);
  }
  StrMessage = "------------------------------------------- end request -----------------------------------------------";
  Serial.println(StrMessage);
  Serial.println("");
  Serial.println("");
  return 0;
}
