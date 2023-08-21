#define BLYNK_TEMPLATE_ID           "TMPL6MkeXmUJN"
#define BLYNK_TEMPLATE_NAME         "Quickstart Template"
#define BLYNK_AUTH_TOKEN            "W5K3X-dNxlAg4LcJN0K1IAwYxjb5Np3L"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "addons/TokenHelper.h" // Provide the token generation process info.
#include "addons/RTDBHelper.h" // Provide the RTDB payload printing info and other helper functions.

//Blynk
#define BLYNK_PRINT Serial
// Insert your network credentials
// #define WIFI_SSID "108 Hoang Du Khuong"
// #define WIFI_PASSWORD "108hoangdukhuong"
#define WIFI_SSID "Greenwich-Student"
#define WIFI_PASSWORD "12345678"
//RTDB
#define API_KEY "AIzaSyApX3hmogUaf6ClctJfCLLMi6q3FJ8o3js" // Insert Firebase project API Key
#define DATABASE_URL "https://iottest-90560-default-rtdb.asia-southeast1.firebasedatabase.app" // Insert RTDB URLefine the RTDB URL
// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "longna2910@gmail.com"
#define USER_PASSWORD "nguyenhamy0831"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;
// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String statusPath = "/status";
String timePath = "/timestamp";
// Parent Node (to be updated in every loop)
String parentPath;
FirebaseJson json;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
// Variable to save current epoch time
String timestamp;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;

BLYNK_CONNECTED()
{
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl",  "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
String getDateTime() {
  timeClient.update();
 	time_t epochTime = timeClient.getEpochTime();
 	struct tm *ptm = gmtime ((time_t *)&epochTime);
 	int monthDay = ptm->tm_mday;
 	int currentMonth = ptm->tm_mon+1;
 	int currentYear = ptm->tm_year+1900;
 	String formattedTime = timeClient.getFormattedTime();
 	return String(monthDay) + "-" + String(currentMonth) + "-" +
  String(currentYear) + " " + formattedTime;
}

//define led, sensor and other,...
int led = 4;
int brightness = 0;  // how bright the LED is
int fadeAmount = 5;  // how many points to fade the LED by
int sensorPin = 14;
int val = 0;
int status_1, status_2, count;
unsigned long timer;

void setup(){
  Serial.begin(115200);
  //Setup Sensor
  pinMode(sensorPin,INPUT);
  count = 0;
  //Setup LED
  pinMode(led,OUTPUT);
  //Blynk setup
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
  timeClient.begin();
  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
  //Firebase connect
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;
  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);
  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}
//Blynk config
BlynkTimer timer_1;
BLYNK_WRITE(V0) {
  int pinValue = param.asInt();
  if(pinValue == 1)
  {
    Blynk.virtualWrite(V5, "LED ON");
    analogWrite(led, 255);
    delay(30);
    Serial.println("Button switched to 1");
  }
  else
  { 
    ledOFF();
    Serial.println("Button switched to 0");
  }
}

void ledON() {
  Blynk.virtualWrite(V4, "LED ON");
  analogWrite(led, brightness);
  // change the brightness for next time through the loop:
  brightness = brightness + fadeAmount;
  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= 0 || brightness >= 255) 
  {fadeAmount = -fadeAmount;}
  // wait for 30 milliseconds to see the dimming effect
  delay(30);
  } 

void ledOFF() {
  Blynk.virtualWrite(V4, "LED OFF");
  Blynk.virtualWrite(V5, "LED OFF");
  analogWrite(led,LOW );
  delay(30);
}

void soundSensor() {
  val = digitalRead(sensorPin);
  Serial.print("value: ");
  Serial.println(val);
  delay(200);
  if(val == 1)
    {
     count++; delay(2000);
    }
  Serial.println(count);
  if(count == 1)
    {
      ledON();
    }
  else if (count >= 2)
    {
      ledOFF();
      count = 0;
    }
}

void sendFirebase()
{
  String mess;
  soundSensor();
  if (Firebase.ready() && val == 1)
  {
    timestamp = getDateTime();
    if (count == 1)
    {mess = "Led status: ON";}
    else
    {mess = "Led status: OFF";}
    Serial.print ("time: ");
    Serial.println (timestamp);
    parentPath= databasePath + "/" + String(timestamp);
    json.set(statusPath.c_str(), mess);
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}

void loop(){
  Blynk.run();
  timer_1.run();
  sendFirebase();
}