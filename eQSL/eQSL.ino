#include "pin_config.h"
#include "eqsl.h"
#include <TFT_eSPI.h>
#include <sdusb.h>
#include <IniFile.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <Regexp.h>
#include <vector>
#include <algorithm>

#define CONFIG_FATFS_LONG_FILENAMES FATFS_LFN_STACK


TFT_eSPI tft = TFT_eSPI();
SDCard2USB dev;

const char *ntpServer1 = "0.pool.ntp.org";
const char *ntpServer2 = "1.pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

char eqsl_callsign[21] = "";
char eqsl_password[51] = "";
uint8_t eqsl_rcvd_period = 14;
uint8_t eqsl_update_interval = 60;

uint8_t updated_images = 0;
std::vector<String> valid_filenames;

void TFT_sleep() {
  tft.writecommand(0x10); // Sleep
  delay(200); // needed!
  ledcWrite(0, 0); // Backlight off
}

void TFT_wake() {
  tft.writecommand(0x11); // Wake up
  delay(200); // needed!
  ledcWrite(0, 255); // Backlight on
}

void TFT_reset() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(0, 0);
}

void printErrorMessage(uint8_t e, bool eol = true)
{
  switch (e) {
  case IniFile::errorNoError:
    tft.print("no error");
    break;
  case IniFile::errorFileNotFound:
    tft.print("file not found");
    break;
  case IniFile::errorFileNotOpen:
    tft.print("file not open");
    break;
  case IniFile::errorBufferTooSmall:
    tft.print("buffer too small");
    break;
  case IniFile::errorSeekError:
    tft.print("seek error");
    break;
  case IniFile::errorSectionNotFound:
    tft.print("section not found");
    break;
  case IniFile::errorKeyNotFound:
    tft.print("key not found");
    break;
  case IniFile::errorEndOfFile:
    tft.print("end of file");
    break;
  case IniFile::errorUnknownError:
    tft.print("unknown error");
    break;
  default:
    tft.print("unknown error value");
    break;
  }
  if (eol)
    tft.println();
}

void sd_card_setup()
{
  TFT_reset();
  tft.println("Initializes the SD Card");
  
  if(!dev.initSD(SD_PIN_NUM_CLK, SD_PIN_NUM_MISO, SD_PIN_NUM_MOSI, SD_PIN_NUM_CS))
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Failed to init SD");
    // Cannot do anything else
    while (1)
      ;
  }

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("No SD_MMC card attached");
    // Cannot do anything else
    while (1)
      ;
  }

  tft.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC)
    tft.println("MMC");
  else if (cardType == CARD_SD)
    tft.println("SDSC");
  else if (cardType == CARD_SDHC)
    tft.println("SDHC");
  else
    tft.println("UNKNOWN");

  if(!dev.begin()) 
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Failed to init USB");
    // Cannot do anything else
    while (1)
      ;
  }

  uint32_t cardSize = SD.cardSize() / (1024 * 1024);
  String str = "SDCard Size: " + String(cardSize) + "MB";
  tft.println(str);
}

void wifi_setup()
{
  TFT_reset();
  tft.println("Initialising the WIFI");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  const char *filename = "/config.ini";
  IniFile ini(filename);
  if (!ini.open()) {
    tft.print("Ini file ");
    tft.print(filename);
    tft.println(" does not exist");
    // Cannot do anything else
    while (1)
      ;
  }

  const size_t bufferLen = 80;
  char ssid[bufferLen];
  char password[bufferLen];

  // Fetch a value from a key which is present
  if (!ini.getValue("wifi", "ssid", ssid, bufferLen)) {
    tft.println("Could not read 'ssid' from section 'wifi', error was ");
    printErrorMessage(ini.getError());
    while (1)
      ;
  }
  // Fetch a value from a key which is present
  if (!ini.getValue("wifi", "password", password, bufferLen)) {
    tft.println("Could not read 'password' from section 'wifi', error was ");
    printErrorMessage(ini.getError());
    while (1)
      ;
  }
  ini.close();
  
  tft.print("Connecting to ");
  tft.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    tft.print(".");
  }
  tft.println("");
  tft.println("WiFi connected");
  tft.print("IP address: ");
  tft.println(WiFi.localIP());

}

void sync_time()
{
  struct tm timeinfo;
  // Synchronize RTC time
  tft.println("Synchronize RTC time");
  while (!getLocalTime(&timeinfo))
  {
    // init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
    tft.print(".");
  }
}

void eqsl_setup()
{
  TFT_reset();
  
  const char *filename = "/config.ini";
  IniFile ini(filename);
  if (!ini.open()) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Ini file ");
    tft.print(filename);
    tft.println(" does not exist");
    // Cannot do anything else
    while (1)
      ;
  }

  // Fetch a value from a key which is present
  if (!ini.getValue("eqsl", "callsign", eqsl_callsign, sizeof(eqsl_callsign))) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Could not read 'callsign' from section 'eqsl', error was ");
    printErrorMessage(ini.getError());
    while (1)
      ;
  }
  // Fetch a value from a key which is present
  if (!ini.getValue("eqsl", "password", eqsl_password, sizeof(eqsl_password))) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Could not read 'password' from section 'eqsl', error was ");
    printErrorMessage(ini.getError());
    while (1)
      ;
  }

  // Fetch a value from a key which is present
  const size_t bufferLen = 80;
  char tmp_buffer[bufferLen];
  if (!ini.getValue("eqsl", "rcvd_period", tmp_buffer, bufferLen, eqsl_rcvd_period)) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print("Could not read 'rcvd_period' from section 'eqsl', error was ");
    printErrorMessage(ini.getError());
    tft.print("Defaulting to "); tft.print(eqsl_rcvd_period); tft.println(" days.");
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    delay(1000);
  }

  if (!ini.getValue("eqsl", "update_interval", tmp_buffer, bufferLen, eqsl_update_interval)) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print("Could not read 'update_interval' from section 'eqsl', error was ");
    printErrorMessage(ini.getError());
    tft.print("Defaulting to "); tft.print(eqsl_update_interval); tft.println(" minutes.");
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    delay(1000);
  }
  
  ini.close();
}

    
void handle_qsl_match(const char * match,
                      const unsigned int length,
                      const MatchState & ms)
{
  char callsign[21];
  char qsoyear[5];
  char qsomonth[3];
  char qsoday[3];
  char qsohour[3];
  char qsominute[3];
  char qsoband[9];
  char qsomode[11];

  ms.GetCapture(callsign, 0);
  ms.GetCapture(qsoyear, 1);
  ms.GetCapture(qsomonth, 2);
  ms.GetCapture(qsoday, 3);
  ms.GetCapture(qsohour, 4);
  ms.GetCapture(qsominute, 5);
  ms.GetCapture(qsoband, 6);
  ms.GetCapture(qsomode, 7);

  // Skip if already exists
  String image_filename = qsoyear;
  image_filename += qsomonth;
  image_filename += qsoday;
  image_filename += "_";
  image_filename += qsohour;
  image_filename += qsominute;
  image_filename += "-";
  image_filename += callsign;
  image_filename += "-";
  image_filename += qsoband;
  image_filename += "-";
  image_filename += qsomode;
  image_filename += ".JPG";
  image_filename.replace("/", "_");

  String image_path = "/";
  image_path += image_filename;
  valid_filenames.push_back(image_filename);

  if (SD.exists(image_path.c_str()))
  {
    // Already downloaded this image.
    return;
  }

  TFT_reset();
  tft.println("Fetching eQSL card: ");
  tft.print(" Call: "); tft.println(callsign);
  tft.print(" Band: "); tft.println(qsoband);
  tft.print(" Mode: "); tft.println(qsomode);
  tft.print(" Date: "); tft.print(qsoyear); tft.print(qsomonth); tft.println(qsoday);
  tft.print(" Time: "); tft.print(qsohour); tft.println(qsominute);

  delay(10000); // Pause here, as must limit downloads once per 10 seconds

  String qsl_url = EQSL_BASE_URL;
  qsl_url += EQSL_GET_PATH;
  qsl_url += "?Username=";
  qsl_url += urlEncode(eqsl_callsign);
  qsl_url += "&Password=";
  qsl_url += urlEncode(eqsl_password);
  qsl_url += "&CallsignFrom=";
  qsl_url += urlEncode(callsign);
  qsl_url += "&QSOBand=";
  qsl_url += urlEncode(qsoband);
  qsl_url += "&QSOMode=";
  qsl_url += urlEncode(qsomode);
  qsl_url += "&QSOYear=";
  qsl_url += qsoyear;
  qsl_url += "&QSOMonth=";
  qsl_url += qsomonth;
  qsl_url += "&QSODay=";
  qsl_url += qsoday;
  qsl_url += "&QSOHour=";
  qsl_url += qsohour;
  qsl_url += "&QSOMinute=";
  qsl_url += qsominute;

  HTTPClient qsl_http;
  qsl_http.setReuse(false);
  qsl_http.begin(qsl_url, ROOT_CA);
  qsl_http.setTimeout(30000);
  
  int qsl_httpCode = qsl_http.GET();
  if (qsl_httpCode != HTTP_CODE_OK)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error fetching eQSL card");
    tft.print("HTTP Code: ");
    tft.println(qsl_httpCode);
    qsl_http.end();
    delay(5000);
    return;
  }
 
  String qsl_payload = qsl_http.getString();
  qsl_http.end();

  if(qsl_payload.indexOf("Error:") > 0)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println(qsl_payload);
    delay(5000);
    return;
  }

  MatchState img_ms;
  img_ms.Target(const_cast<char*>(qsl_payload.c_str()));
  char result = img_ms.Match("<img src=\"([^\"]+)\"");
  if (result != REGEXP_MATCHED)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error finding eQSL card link");
    delay(5000);
    return;
  }
  
  char match_buffer[100];
  String image_url = EQSL_BASE_URL;
  image_url += img_ms.GetCapture(match_buffer, 0);

  HTTPClient image_http;
  image_http.setReuse(false);
  image_http.begin(image_url, ROOT_CA);
  image_http.setTimeout(30000);
    
  int image_httpCode = image_http.GET();
  if (image_httpCode != HTTP_CODE_OK)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error fetching eQSL card");
    tft.print("HTTP Code: ");
    tft.println(image_httpCode);
    image_http.end();
    delay(5000);
    return;
  }
  WiFiClient * stream = image_http.getStreamPtr();
  
  int len = image_http.getSize();  // -1 no size delcared
  uint8_t buff[2048] = {0};
  File image_file = SD.open(image_path, FILE_WRITE);
  while (image_http.connected() && (len > 0 || len == -1))
  {
      size_t size = stream->available();
      if (size) 
      {
        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
        image_file.write(buff, c);
        if (len > 0) {
          len -= c;
        }
      }
      delay(1);
  }
  updated_images += 1;
  image_file.close();
  image_http.end();
}

void setup(void)
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  tft.setTextFont(2);
  TFT_reset();

  ledcSetup(0, 4000, 8);
  ledcAttachPin(TFT_BL, 0);
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle = dutyCycle + 5)
  {
    ledcWrite(0, dutyCycle);
    delay(5);
  }

  sd_card_setup();
  delay(3000);
  wifi_setup();
  delay(1000);
  sync_time();
  
  eqsl_setup();
  delay(1000);
}

void loop()
{
  TFT_reset();
  tft.print("Fetching eQSL cards for ");
  tft.println(eqsl_callsign);
  tft.print(".");
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error getting time");

    delay(5000);
    return;
  }
  timeinfo.tm_mday -= eqsl_rcvd_period;
  mktime(&timeinfo);

  char rcvd_since[13];
  if (!strftime(rcvd_since, 13, "%Y%m%d%H%M", &timeinfo))
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error formatting rcvd since time");

    delay(5000);
    return;
  }
  
  HTTPClient http;
  http.setReuse(false);
  String url = EQSL_BASE_URL;
  url += EQSL_INBOX_PATH;
  url += "?Username=";
  url += urlEncode(eqsl_callsign);
  url += "&Password=";
  url += urlEncode(eqsl_password);
  url += "&RcvdSince=";
  url += rcvd_since;
  url += "&ConfirmedOnly=1";

  http.begin(url, ROOT_CA);
    
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error fetching Inbox");
    tft.print("HTTP Code: ");
    tft.println(httpCode);
    
    http.end();
    delay(5000);
    return;
  }
  
  String payload = http.getString();
  http.end();

  if(payload.indexOf("Your ADIF log file has been built") < 0)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error getting ADIF link from eQSL");
    delay(5000);
    return;
  }
  
  MatchState ms;
  ms.Target(const_cast<char*>(payload.c_str()));
  char result = ms.Match("<A HREF=\"%.%.(/[^\"]+)\">%.ADI file</A>");
  if (result != REGEXP_MATCHED)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error finding ADIF link from eQSL");

    delay(5000);
    return;
  }
  char match_buffer[100];
  String adif_url = EQSL_BASE_URL;
  adif_url += ms.GetCapture(match_buffer, 0);

  tft.print(".");

  HTTPClient adif_http;
  adif_http.setReuse(false);
  adif_http.begin(adif_url, ROOT_CA);

  int adif_httpCode = adif_http.GET();
  if (adif_httpCode != HTTP_CODE_OK)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error fetching ADIF file");
    tft.print("HTTP Code: ");
    tft.println(adif_httpCode);
    
    adif_http.end();
    delay(5000);
    return;
  }
  String adif = adif_http.getString();
  adif_http.end();
  
  updated_images = 0;
  uint8_t eqsl_count;
  MatchState qsl_ms;
  qsl_ms.Target(const_cast<char*>(adif.c_str()));
  eqsl_count = qsl_ms.GlobalMatch(QSO_RE, handle_qsl_match);

  uint8_t removed_images = 0;
  File root = SD.open("/");
  while (true) {
    File entry =  root.openNextFile();
    if (!entry)
    {
      // no more files
      break;
    }
    if ( (strcmp(entry.name(), "config.ini") == 0) || (entry.isDirectory()) )
    {
      entry.close();
      continue;
    }
    if (std::find(valid_filenames.begin(), valid_filenames.end(), entry.name()) == valid_filenames.end())
    {
      String entry_path = "/";
      entry_path += entry.name();
      entry.close();
      if (SD.remove(entry_path))
      {
        removed_images++;
      }
    }
  }

  TFT_reset();
  tft.print("Processed ");
  tft.print(eqsl_count);
  tft.println(" eQSL cards.");
  tft.print("Downloaded ");
  tft.print(updated_images);
  tft.println(" new eQSL cards.");
  tft.print("Removed ");
  tft.print(removed_images);
  tft.println(" old eQSL cards.");
  delay(10000);
  
  if ( (updated_images > 0) || (removed_images > 0) ) {
    // Need to restart for USB interface
    esp_restart();
  }

  TFT_reset();
  TFT_sleep();
  digitalWrite(LED_PIN, LOW);
  delay(eqsl_update_interval*60000);
  TFT_wake();
}
