#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define TdsSensorPin 27
#define VREF 3.3
#define SCOUNT  30
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;

int valueTDS = 177;
int valueTurbidity = 21;
int valueWaterFlow = 40;

//Turbidity Sensor
#define pinTurb 26
float ntu;

//Water Flow Sensor
byte statusLed    = 13;
byte sensorInterrupt = 0;
byte sensorPin       = 25;
float calibrationFactor = 4.5;
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;

//millis
unsigned long previousMillisTurb = 0;
unsigned long previousMillisTds = 0;
unsigned long previousMillisFlow = 0;


void setup() {
  Serial.begin(115200);
  pinMode(TdsSensorPin, INPUT);
  pinMode(pinTurb, INPUT);
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // We have an active-low LED attached

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }
  oled.clearDisplay();

  oled.setTextSize(1.9);
  oled.setTextColor(WHITE);
  oled.setCursor(2, 1);
  oled.println("TDS");
  oled.setCursor(45, 1);
  oled.println(": ");

  oled.setCursor(2, 25);
  oled.println("Turbi");
  oled.setCursor(45, 25);
  oled.println(": ");

  oled.setCursor(2, 50);
  oled.println("Flow");
  oled.setCursor(45, 50);
  oled.println(": ");
  oled.display();
}

void loop() {

  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U)
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U)
  {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVolatge = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5;
    Serial.print("voltage:");
    Serial.print(averageVoltage, 2);
    Serial.print("V   ");
    Serial.print("TDS Value:");
    Serial.print(tdsValue, 0);
    Serial.println("ppm");
    oled.setTextSize(1);
    oled.setTextColor(WHITE, BLACK);
    oled.setCursor(52, 1);
    oled.print(tdsValue);
    oled.print(" ppm");
  }

  unsigned long currentMillisTurb = millis();
  if (currentMillisTurb - previousMillisTurb >= 500) {
    previousMillisTurb = currentMillisTurb;
    int sensorValue = analogRead(pinTurb);
    float voltage = sensorValue * (3.3 / 4096.0);
    ntu = 100.00 - (voltage / 3.3) * 100.00;
    oled.setTextSize(1);
    oled.setTextColor(WHITE, BLACK);
    oled.setCursor(52, 25);
    oled.print(ntu);
    oled.print(" Ntu");

  }

  unsigned long currentMillisFlow = millis();
  if (currentMillisFlow - previousMillisFlow >= 10) {
    previousMillisFlow = currentMillisFlow;

    if ((millis() - oldTime) > 1000)
    {
      detachInterrupt(sensorInterrupt);
      flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
      oldTime = millis();
      flowMilliLitres = (flowRate / 60) * 1000;
      totalMilliLitres += flowMilliLitres;

      unsigned int frac;
      Serial.print("Flow rate: ");
      Serial.print(int(flowRate));
      Serial.print("L/min");
      Serial.print("\t");

      Print the cumulative total of litres flowed since starting
      Serial.print("Output Liquid Quantity: ");
      Serial.print(totalMilliLitres);
      Serial.println("mL");
      Serial.print("\t");       // Print tab space
      Serial.print(totalMilliLitres / 1000);
      Serial.print("L");
      oled.setTextSize(1);
      oled.setTextColor(WHITE, BLACK);
      oled.setCursor(52, 50);
      oled.print(int(flowRate));
      oled.print(" L/min");

      pulseCount = 0;

      attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
    }




  }
  oled.display();
}

int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}

void pulseCounter()
{
  pulseCount++;
}
