/*
Dieser Code gehört zu dem Buch Licht und Spaß
Er gehört zum Projekt 16 - Dunkelheitssensor
Autor: René Bohne

Das Programm basiert auf dem HTTP Beispiel aus der Wifly Library und nutzt die Adafruit Bibliotheken für den MLX90614 und das OLED Display mit dem SSD1351.
Die FreqMeasure Bibliothek wird genutzt, um den TSL237 Lichtsensor auszulesen.

Der Temperatursensor und der Lichtsensor werden ausgelesen und die Werte werden auf dem OLED Display angezeigt.
Alle 640 (20*32) Messungen werden die Mittelwerte an den phant Server von data.sparkfun.com übermittelt. 
*/

#include <Arduino.h>
#include "HTTPClient.h"
#include <WiFly.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <FreqMeasure.h>


#define SSID      "NAME_DES_WLANS_HIER_REIN"
#define KEY       "PASSWORT_DES_WLANS_HIER_REIN"

#define AUTH      WIFLY_AUTH_WPA2_PSK

//Hier muss der Public Key eingetragen werden
#define HTTP_POST_URL "http://data.sparkfun.com/input/xR08zM7g3zI8lZDdDbMW.txt"

//Hier muss der Private Key eingetragen werden
#define HTTP_POST_HEADER "Phant-Private-Key: Zae17Ao9E7hNqbwKwRA7"


/************** OLED PINS ***********************************/
#define sclk     13
#define miso     12
#define mosi     11
#define dc        7
#define oledcs   10
#define rst       9

/************** FARBEN **************************************/
#define	BLACK    0b0000000000000000
#define WHITE    0b1111111111111111
#define GRAY1    0b11000110000110000
#define GRAY2    0b11100111000111000
#define GRAY3    0b11110111100111100
#define GRAY4    0b11011111001110111
#define	BLUE     0b0000000000011111
#define	RED      0b1111100000000000
#define YELLOW   0b1111111111100000
#define ORANGE   0b1111110001000000

/************** Objekte **************************************/
WiFly wifly(Serial);
HTTPClient http;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_SSD1351 tft = Adafruit_SSD1351(oledcs, dc, rst);

/************** VARIABLEN ************************************/
char get;
int waitForNextTransmission = 20;

int counter = 0;
float maxFreq = 40;
float lastFreq = 0;
float currentFreq = 0;
int colorFreq = BLACK;

float maxTemp = 40;
float lastTemp = 0;
float currentTemp = 0;
int colorTemp = BLACK;

float ambientMean = 0.0f;
float objectMean = 0.0f;
float frequencyMean = 0.0f;

/************** SETUP ****************************************/
void setup()
{
  Serial.begin(9600);

  mlx.begin();
  tft.begin();

  //Dreht das Bild um 180 Grad
  tft.setRotation(2);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);

  tft.setCursor(0, 0) ;
  tft.println("Spass und Licht");
  tft.println("Internet wird verbunden");

  // check if WiFly is associated with AP(SSID)
  if (!wifly.isAssociated(SSID))
  {
    while (!wifly.join(SSID, KEY, AUTH))
    {
      tft.fillScreen(BLACK);
      tft.setCursor(0, 0) ;
      tft.println("WLAN nicht verfügbar: ");
      tft.println(SSID);
      tft.println("verbinde erneut...");
      delay(200);
    }

    wifly.save();    // save configuration,
  }

  tft.fillScreen(BLACK);
  tft.setCursor(0, 0);
}


/************** postDataToSparkfun ***************************/
/* Sendet die Mittelwerte der Messreihen ins Internet        */
void postDataToSparkfun()
{
  char buf[70];
  char astr[10];
  char ostr[10];
  char lstr[10];

  //Flieskommazahlen in Text wandeln
  dtostrf(ambientMean, 5, 2, astr);
  dtostrf(objectMean, 5, 2, ostr);
  dtostrf(frequencyMean, 5, 2, lstr);
  snprintf(buf, sizeof(buf), "ambienttemp=%s&brightness=%s&clientid=%d&objecttemp=%s", astr, lstr, 1, ostr);

  tft.fillScreen(BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(WHITE);
  tft.println("Daten werden gesendet");
  while (http.post(HTTP_POST_URL, HTTP_POST_HEADER, buf, 10000) < 0)
  {
  }

  tft.fillScreen(BLACK);
  tft.setCursor(0, 0);
  while (wifly.receive((uint8_t *)&get, 1, 1000) == 1) {
    //    tft.print(get);//Antwort vom Server
  }
}

/************** loop *****************************************/
/* Sammelt Messwerte und zeigt sie auf dem Display an        */
void loop()
{
  currentTemp =   mlx.readObjectTempC();
  if (currentTemp > maxTemp)
  {
    maxTemp = currentTemp;
  }

  if (currentTemp > 30)
  {
    colorTemp = RED;
  }
  else if (currentTemp > 20)
  {
    colorTemp = ORANGE;
  }
  else if (currentTemp > 15)
  {
    colorTemp = YELLOW;
  }
  else
  {
    colorTemp = BLUE;
  }

  //Hysterese - vermeidet zu starkes Zappeln
  if (abs(currentTemp - lastTemp) > 0.15)
  {
    lastTemp = currentTemp;

    //alten Text löschen
    tft.fillRect(0, 0, 64, 8, BLACK);

    tft.setCursor(0, 0);
    tft.setTextColor(colorTemp);
    tft.print(currentTemp);
  }
  else
  {
    currentTemp = lastTemp;
  }

  float f = currentTemp / maxTemp;
  tft.fillRect(2 * counter, 128 - f * 118, 2, f * 118,  colorTemp);

  currentFreq = read_TSL237_Hz();

  if (currentFreq > maxFreq)
  {
    maxFreq = currentFreq;
  }

  if (currentFreq > 100000)
  {
    colorFreq = GRAY4;
  }
  else if (currentFreq > 50000)
  {
    colorFreq = GRAY3;
  }
  else if (currentFreq > 10000)
  {
    colorFreq = GRAY2;
  }
  else
  {
    colorFreq = GRAY1;
  }

  //Hysterese - vermeidet zu starkes Zappeln
  if (abs(currentFreq - lastFreq) > 1000)
  {
    lastFreq = currentFreq;

    //alten Text löschen
    tft.fillRect(64, 0, 64, 8, BLACK);

    tft.setCursor(64, 0);
    tft.setTextColor(colorFreq);
    tft.print(currentFreq);
  }
  else
  {
    currentFreq = lastFreq;
  }

  f = currentFreq / maxFreq;
  tft.fillRect(128 - 2 * counter - 1, 128 - f * 118, 2, f * 118,  colorFreq); //x,y,w,h,farbe

  //Mittelwerte berechnen
  ambientMean = (ambientMean + mlx.readAmbientTempC()) / 2;
  objectMean = (objectMean + currentTemp) / 2;
  frequencyMean = (frequencyMean + currentFreq) / 2;

  delay(50);

  counter++;
  if (counter > 31)
  {
    counter = 0;

    //Graph löschen
    tft.fillRect(0, 10, 128, 118,  BLACK);//x,y,w,farbe

    waitForNextTransmission--;
    if (waitForNextTransmission == 0)
    {
      postDataToSparkfun();
      waitForNextTransmission = 20;

      tft.fillScreen(BLACK);
      tft.setCursor(0, 0);
    }
  }
}

/************** read_TSL237_Hz *******************************/
/* Liest den TSL237 Sensor aus, gibt das Ergebnis in Hz aus  */
float read_TSL237_Hz()
{
  double sum = 0; //Summe von 30 Messwerten
  double frequenz = 0.0f;//gemessene Frequenz des Sensors
  int count = 0; //wird unten in der for Schleife verwendet

  FreqMeasure.begin();////Frequenz-Messung beenden
  for (count = 0; count < 30; count++) //30 Messwerte werden erfasst
  {
    while (!FreqMeasure.available())
    {
      //auf neuen Messwert warten
    }
    // Einzelmessungen summieren
    sum = sum + FreqMeasure.read();
  }
  FreqMeasure.end();//Frequenz-Messung beenden
  frequenz = F_CPU / (sum / count);//F_CPU ist die Taktfrequenz des Arduinos

  return frequenz;
}
