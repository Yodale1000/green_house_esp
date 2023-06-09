#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <secrets.h>
#include <ArduinoJson.h>
#include <Adafruit_DotStar.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_LTR329_LTR303.h>

#define NUMPIXELS 1
#define DATAPIN 33
#define CLOCKPIN 21

Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LTR329 ltr = Adafruit_LTR329();

void setup()
{
  Serial.begin(115200);

  strip.begin();
  strip.setBrightness(20);
  strip.setPixelColor(0, 0, 0, 255);
  strip.show();

  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  bool res;
  res = wm.autoConnect();

  if (!res)
  {
    Serial.println("Failed to connect");
    strip.setPixelColor(0, 0, 255, 0);
    strip.show();
    ESP.restart();
  }
  else
  {
    Serial.println("Connected");
    strip.setPixelColor(0, 255, 0, 0);
    strip.show();
  }

  Serial.println("SHT31 test");
  if (!sht31.begin(0x44))
  {
    Serial.println("Couldn't find SHT31 sensor!");
    while (1)
      delay(1);
  }

  Serial.println("LTR329 test");
  if (!ltr.begin())
  {
    Serial.println("Couldn't find LTR sensor!");
    while (1)
      delay(10);
  }

  ltr.setGain(LTR3XX_GAIN_2);
  Serial.print("Gain : ");
  switch (ltr.getGain())
  {
  case LTR3XX_GAIN_1:
    Serial.println(1);
    break;
  case LTR3XX_GAIN_2:
    Serial.println(2);
    break;
  case LTR3XX_GAIN_4:
    Serial.println(4);
    break;
  case LTR3XX_GAIN_8:
    Serial.println(8);
    break;
  case LTR3XX_GAIN_48:
    Serial.println(48);
    break;
  case LTR3XX_GAIN_96:
    Serial.println(96);
    break;
  }

  ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
  Serial.print("Integration Time (ms): ");
  switch (ltr.getIntegrationTime())
  {
  case LTR3XX_INTEGTIME_50:
    Serial.println(50);
    break;
  case LTR3XX_INTEGTIME_100:
    Serial.println(100);
    break;
  case LTR3XX_INTEGTIME_150:
    Serial.println(150);
    break;
  case LTR3XX_INTEGTIME_200:
    Serial.println(200);
    break;
  case LTR3XX_INTEGTIME_250:
    Serial.println(250);
    break;
  case LTR3XX_INTEGTIME_300:
    Serial.println(300);
    break;
  case LTR3XX_INTEGTIME_350:
    Serial.println(350);
    break;
  case LTR3XX_INTEGTIME_400:
    Serial.println(400);
    break;
  }

  ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
  Serial.print("Measurement Rate (ms): ");
  switch (ltr.getMeasurementRate())
  {
  case LTR3XX_MEASRATE_50:
    Serial.println(50);
    break;
  case LTR3XX_MEASRATE_100:
    Serial.println(100);
    break;
  case LTR3XX_MEASRATE_200:
    Serial.println(200);
    break;
  case LTR3XX_MEASRATE_500:
    Serial.println(500);
    break;
  case LTR3XX_MEASRATE_1000:
    Serial.println(1000);
    break;
  case LTR3XX_MEASRATE_2000:
    Serial.println(2000);
    break;
  }
}

void loop()
{
  strip.setPixelColor(0, 0, 0, 255);
  strip.show();

  bool valid;
  uint16_t visible_plus_ir, infrared;

  if (ltr.newDataAvailable())
  {
    valid = ltr.readBothChannels(visible_plus_ir, infrared);
    if (valid)
    {
      Serial.println(visible_plus_ir);
      Serial.println(infrared);
    }
  }

  float temperature = sht31.readTemperature();
  float humidity = sht31.readHumidity();

  Serial.println(temperature);
  Serial.println(humidity);

  if (WiFi.status() == WL_CONNECTED)
  {
    StaticJsonDocument<128> doc;

    doc["temperatureReading"]["value"] = temperature;
    doc["humidityReading"]["value"] = humidity;

    JsonObject LightReading = doc.createNestedObject("lightReading");
    LightReading["visible_plus_ir_value"] = visible_plus_ir;
    LightReading["infrared_value"] = infrared;

    String output;

    serializeJson(doc, output);

    HTTPClient http;

    http.begin(API_URI);
    http.addHeader("Authorization", "Bearer " + String(API_KEY));
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(output);
    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
      if (httpResponseCode == 201)
      {
        strip.setPixelColor(0, 255, 0, 0);
        strip.show();
      }
      else
      {
        strip.setPixelColor(0, 0, 255, 0);
        strip.show();
      }
    }
    else
    {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
      strip.setPixelColor(0, 0, 255, 0);
      strip.show();
    }
    http.end();
  }
  else
  {
    Serial.println("Error in WiFi connection");
    strip.setPixelColor(0, 0, 255, 0);
    strip.show();
  }

  delay(60000);
}