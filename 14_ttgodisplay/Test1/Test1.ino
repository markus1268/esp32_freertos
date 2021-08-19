#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(135, 240);

void setup()
{
    Serial.begin(115200);
    Serial.println("Start");

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("Hello", tft.width() / 2, tft.height() / 2 - 16);
    delay(1000);
    tft.setRotation(0);
    tft.fillScreen(TFT_RED);
    delay(1000);
    tft.fillScreen(TFT_BLUE);
    delay(1000);
    tft.fillScreen(TFT_GREEN);
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("Hello World", tft.width() / 2, tft.height() / 2 - 16);
}

void loop()
{
}
