#include <Arduino.h>
// #include <EEPROM.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Preferences.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

String dataPreferences = "";
String deviceId = "";

const char *MQTT_SERVER = "192.168.220.149";
const char *MQTT_USERNAME = "risma";
const char *MQTT_PASSWORD = "1234";

const char *ssid = "vivo V25e";
const char *password = "hotspotpassword";
unsigned long sendtimestamp = 0;
unsigned long displayTimeStamp = 0;
int interval = 5000;
int intervalDisplay = 2000;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
WiFiClient espClient;
PubSubClient client(espClient);
Preferences preferences;

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String listenTo = "reminder-" + deviceId;
  String mqttTopiq = String(topic);
  const char *topicChar = listenTo.c_str();
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp", MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String readFromPreferences(const char *key)
{
  return preferences.getString(key, "");
}

void writeToPreferences(const char *key, const String &value)
{
  preferences.putString(key, value);
  preferences.end();
}

void setup()
{
  Serial.begin(9600);
  delay(10);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  mlx.begin();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);

  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  // Display static text
  display.println("Temperature");
  // Serial.println("Device Ready");
  display.display();
  preferences.begin("my-preferences");
  dataPreferences = readFromPreferences("deviceId");

  if (!mlx.begin())
  {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1)
      ;
  };

  if (dataPreferences == "")
  {
    Serial.println("Data Kosong");
    HTTPClient http;
    http.begin("http://192.168.220.149:8000/api/v1/smartbracelet/generateid");
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST("");

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    String responsebody = http.getString();
    Serial.print("HTTP Body");
    Serial.println(responsebody);
    StaticJsonDocument<512> doc;

    DeserializationError error = deserializeJson(doc, responsebody);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    String uniqcode = doc["bracelet"]["uniqCode"];
    Serial.println(uniqcode);

    writeToPreferences("deviceId", uniqcode);
    deviceId = uniqcode;
  }

  if (dataPreferences != "")
  {
    Serial.println("data tidak kosong");
    Serial.print("ID adalah ");
    Serial.println(dataPreferences);
    deviceId = dataPreferences;
  }

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
}

void loop()
{
  int objectTempC = round(mlx.readObjectTempC());

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (millis() - sendtimestamp > interval)
  {
    sendtimestamp = millis();
    Serial.print("Mengirim pesan");
    String topic = "/bracelet/update/temp/" + deviceId;
    const char *topicChar = topic.c_str();
    int suhu = (objectTempC);
    String suhuStr = String(suhu);
    String payload = "{\"temp\":\"" + suhuStr + "\",\"id\":\"" + deviceId + "\"}";
    const char *payloadChar = payload.c_str();
    client.publish(topicChar, payloadChar);
    Serial.println(payload);
  }

  displayTimeStamp = millis();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("ID: ");
  display.print(deviceId);
  display.setCursor(0, 30);
  display.print("Suhu: ");
  display.print(objectTempC);
  display.println(" C");
  display.display();

  // Serial.print("Suhu: ");
  // Serial.print(objectTempC);
  // Serial.println(" C");

  // delay(1000);
  // delay(500);
}