#define LILYGO_T5_V213

#include <SPI.h>
#include <WiFi.h>


#include <Time.h>         //We will need these two just to do some rough time math on the timestamps we get
#include <TimeLib.h>

#include <WiFiClient.h>

#include <cstring>
#include <boards.h>
#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <OctoPrintAPI.h>

#include <Fonts/FreeSans9pt7b.h>        // Small font
#include <Fonts/FreeSans18pt7b.h>       // Medium Font
#include <Fonts/FreeSans24pt7b.h>       // Big Font

#include <Fonts/FreeSansBold18pt7b.h>  // Medium bolt Font

#include "config.h"

#include <GxGDEM0213B74/GxGDEM0213B74.h>  // 2.13" b/w  form GoodDisplay 4-color

#include <WiFi.h>
#include <WiFiClient.h>
#define uS_TO_S_FACTOR 1000000             /* Conversion factor for micro seconds to seconds */


IPAddress ip(192, 168, 68, 134);                         

String octoprint_apikey = "B14149D44DA9408D9E7241A78C8953F3"; //See top of file or GIT Readme about getting API key


WiFiClient client;


OctoprintApi api(client, ip, 80, octoprint_apikey);               //If using IP address

typedef struct {
    char state[32];
    char file[32];
    char print_estimate_time[64];
    char print_estimate_percentage[64];
    char printer_temp_hotend[64];
    char printer_temp_heatbed[64];
} Display_data_t;


enum {
    GxEPD_ALIGN_RIGHT,
    GxEPD_ALIGN_LEFT,
    GxEPD_ALIGN_LEFT_OFFSET,
    GxEPD_ALIGN_CENTER,
};

enum {
    FONTSIZE_SMALL,
    FONTSIZE_MEDIUM,
    FONTSIZE_MEDIUM_BOLD,
    FONTSIZE_BIG,
};

GxIO_Class           io(SPI,  EPD_CS, EPD_DC,  EPD_RSET);
GxEPD_Class          display(io, EPD_RSET, EPD_BUSY);

Display_data_t       displayData;


static void displayText(const char *str, int16_t y, uint8_t align, uint8_t fontsize)
{
    
    int16_t x = 0;
    int16_t x1 = 0, y1 = 0;
    uint16_t w = 0, h = 0;

    switch(fontsize){
        case FONTSIZE_SMALL:
            display.setFont(&FreeSans9pt7b);
            break;
        case FONTSIZE_MEDIUM:
            display.setFont(&FreeSans18pt7b);
            break;
        case FONTSIZE_MEDIUM_BOLD:
            display.setFont(&FreeSansBold18pt7b);
            break;
        case FONTSIZE_BIG:
            display.setFont(&FreeSans24pt7b);
            break;
    }

    display.setCursor(x, y);
    display.getTextBounds(str, x, y, &x1, &y1, &w, &h);
    switch (align) {
        case GxEPD_ALIGN_RIGHT:
            display.setCursor(display.width() - w - x1, y);
            break;
        case GxEPD_ALIGN_LEFT:
            display.setCursor(0, y);
            break;
        case GxEPD_ALIGN_LEFT_OFFSET:
            display.setCursor(3, y);
            break;
        case GxEPD_ALIGN_CENTER:
            display.setCursor(display.width() / 2 - ((w + x1) / 2), y);
            break;
        default:
            break;
    }
    display.println(str);
}

static void updateDisplay()
{
    display.fillScreen(GxEPD_WHITE);

    Serial.println("Print Time");
    displayText(displayData.file, 10, GxEPD_ALIGN_LEFT_OFFSET, FONTSIZE_SMALL);
    displayText(displayData.print_estimate_time, 90, GxEPD_ALIGN_LEFT_OFFSET, FONTSIZE_SMALL);
    displayText(displayData.print_estimate_percentage, 10, GxEPD_ALIGN_CENTER, FONTSIZE_SMALL);
    displayText(displayData.printer_temp_heatbed, 155, GxEPD_ALIGN_RIGHT, FONTSIZE_SMALL);
    displayText(displayData.printer_temp_hotend, 155, GxEPD_ALIGN_RIGHT, FONTSIZE_SMALL);
    display.update();
}

void setup()
{
    Serial.begin(115200);
    delay(10);

    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }


    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

  
    SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);
    display.init();
    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(DEFAULT_FONT);




    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status     
      if(api.getPrintJob()){  //Get the print job API endpoint

        char buf[265];

        const float temp_percent = floor(api.printJob.progressCompletion*100)/100;
        /* Print time left (if printing) in human readable time HH:MM:SS */
        int runHours= api.printJob.progressPrintTimeLeft/3600;
        int secsRemaining=api.printJob.progressPrintTimeLeft%3600;
        int runMinutes=secsRemaining/60;
        int runSeconds=secsRemaining%60;
       
        
         /* Copy Printer Status to Struct */
        strcpy(displayData.state, api.printJob.printerState.c_str());
      
         /* Copy File name to Struct */
        strcpy(displayData.file, api.printJob.jobFileName.c_str()); 

         /* Copy Estimate Time  to Buff */
        memset(buf, 0, sizeof(buf));
        sprintf(buf,"Time left:\t%02d:%02d:%02d",runHours,runMinutes,runSeconds);
        strcpy(displayData.print_estimate_time, buf);              

        /* Copy Percentage to Buff */
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "Percentage:\t %.0f %%", temp_percent);
        strcpy(displayData.print_estimate_percentage, buf);    
          
        updateDisplay();
        
      }

    }
          Serial.println("Going to sleep now");

    
     esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void loop(){

  }



