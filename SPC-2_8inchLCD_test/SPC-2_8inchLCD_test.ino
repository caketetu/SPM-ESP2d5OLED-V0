//2.8inchLCDテストプログラム
//ページ0 情報表示
//ページ1 グラフィックデモ
//ページ2 CAN情報表示
//ページ3 RS485情報表示
#include "FS.h"
#include "SD.h"
//#include "SPI.h"

#include <SPI.h>
#include <TFT_eSPI.h>  // Hardware-specific library
#include <ESP32-TWAI-CAN.hpp>

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

#define CALIBRATION_FILE "/TouchCalData1"

#define PAGE_MAX 4

#define NOT_TOUCH -1
#define TOUCH_R 1
#define TOUCH_L 2
#define TOUCH_C 3

int16_t page = 0;

int16_t timer_1000ms = 0;
bool can_recv = false;
uint8_t can_id;
int16_t can_packetSize;
uint8_t can_data[100];

bool SD_detection = false;
uint8_t cardType;


#define DEG2RAD 0.0174532925

#define LOOP_DELAY 10  // Loop delay to slow things down

#define CAN_TXD 22
#define CAN_RXD 21

byte inc = 0;
unsigned int col = 0;

byte red = 31;   // Red is the top 5 bits of a 16-bit colour value
byte green = 0;  // Green is the middle 6 bits
byte blue = 0;   // Blue is the bottom 5 bits
byte state = 0;

uint16_t TEXT_HEIGHT = 16;
uint16_t BOT_FIXED_AREA = 0;
uint16_t TOP_FIXED_AREA = 16;
uint16_t YMAX = 240;
uint16_t yStart = TOP_FIXED_AREA;
uint16_t yArea = YMAX - TOP_FIXED_AREA - BOT_FIXED_AREA;
uint16_t yDraw = YMAX - BOT_FIXED_AREA - TEXT_HEIGHT;
uint16_t xPos = 0;
byte data = 0;
bool change_colour = 1;
bool selected = 1;
int blank[19];  // We keep all the strings pixel lengths to optimise the speed of the top line blanking

void setup() {
  // put your setup code here, to run once:
  // Use serial port
  Serial.begin(115200);
  Serial2.begin(115200);

  ledcSetup(0, 1000, 8);  //LCDバックライトPWM設定
  ledcAttachPin(4, 0);    //LCDバックライトPWMピン割り付け
  ledcWrite(0, 100);      //LCDバックライトPWM出力

  // start the CAN bus at 500 kbps
  // CAN.onReceive(onReceive);
  // Set pins
  // ESP32Can.setPins(CAN_TXD, CAN_RXD);
  // // You can set custom size for the queues - those are default
  // ESP32Can.setRxQueueSize(5);
  // ESP32Can.setTxQueueSize(5);
  // // .setSpeed() and .begin() functions require to use TwaiSpeed enum,
  // // but you can easily convert it from numerical value using .convertSpeed()
  // ESP32Can.setSpeed(ESP32Can.convertSpeed(500));
  // // You can also just use .begin()..
  // if (ESP32Can.begin()) {
  //   Serial.println("CAN bus started!");
  // } else {
  //   Serial.println("CAN bus failed!");
  // }

  // Initialise the TFT screen
  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(1);

  // Calibrate the touch screen and retrieve the scaling factors
  //touch_calibrate();

  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  // Draw keypad background
  tft.fillRect(0, 0, 320, 240, TFT_DARKGREY);

  //TFT-LCD初期表示
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.println("SPC-2_8inchLCD Test");
  delay(1000);
}

void loop() {
  tft.fillRect(0, 0, 320, 240, TFT_DARKGREY);
  switch (page) {
    case 0: PAGE0(); break;
    case 1: PAGE1(); break;
    case 2: PAGE2(); break;
    case 3: PAGE3(); break;
    default: break;
  }
}

int16_t touch_check(void) {
  uint16_t t_x = 0, t_y = 0;
  bool pressed = tft.getTouch(&t_x, &t_y);
  int16_t tp = NOT_TOUCH;
  if (pressed) {
    delay(250);
    if (t_x >= 0, t_x < 106) tp = TOUCH_L;
    else if (t_x >= 106, t_x < 212) tp = TOUCH_C;
    else if (t_x >= 212, t_x < 320) tp = TOUCH_R;
  }
  return tp;
}

void refresh_page(int16_t pp) {
  page += pp;
  if (page < 0) page = PAGE_MAX - 1;
  if (page >= PAGE_MAX) page = 0;
}

//ページ0　情報表示
void PAGE0(void) {
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.println("Page 0 Infomation");
  int16_t r = 0;
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  r++;
  tft.setCursor(0, 16 * r, 2);
  tft.printf("ESP32 Chip ID = %04X\r\n", (uint16_t)(ESP.getEfuseMac() >> 32));  //print High 2 bytes
  r++;
  tft.setCursor(0, 16 * r, 2);
  tft.printf("Chip Revision %d\r\n", ESP.getChipRevision());
  //r++; tft.setCursor(0, 16*r, 2);
  //tft.printf("Number of Core: %d\r\n", chip_info.cores);
  r++;
  tft.setCursor(0, 16 * r, 2);
  tft.printf("CPU Frequency: %d MHz\r\n", ESP.getCpuFreqMHz());
  //r++; tft.setCursor(0, 16*r, 2);
  //tft.printf("Flash Chip Size = %d byte\r\n", ESP.getFlashChipSize());
  //r++; tft.setCursor(0, 16*r, 2);
  //tft.printf("Flash Frequency = %d Hz\r\n", ESP.getFlashChipSpeed());
  //r++; tft.setCursor(0, 16*r, 2);
  //tft.printf("ESP-IDF version = %s\r\n", esp_get_idf_version());
  r++;
  tft.setCursor(0, 16 * r, 2);
  uint8_t mac0[6];
  esp_efuse_mac_get_default(mac0);
  tft.printf("Default Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  r++;
  tft.setCursor(0, 16 * r, 2);
  uint8_t mac3[6];
  esp_read_mac(mac3, ESP_MAC_WIFI_STA);
  tft.printf("[Wi-Fi Station] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac3[0], mac3[1], mac3[2], mac3[3], mac3[4], mac3[5]);
  r++;
  tft.setCursor(0, 16 * r, 2);
  uint8_t mac4[7];
  esp_read_mac(mac4, ESP_MAC_WIFI_SOFTAP);
  tft.printf("[Wi-Fi SoftAP] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac4[0], mac4[1], mac4[2], mac4[3], mac4[4], mac4[5]);
  r++;
  tft.setCursor(0, 16 * r, 2);
  uint8_t mac5[6];
  esp_read_mac(mac5, ESP_MAC_BT);
  tft.printf("[Bluetooth] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac5[0], mac5[1], mac5[2], mac5[3], mac5[4], mac5[5]);
  r++;
  tft.setCursor(0, 16 * r, 2);
  uint8_t mac6[6];
  esp_read_mac(mac6, ESP_MAC_ETH);
  tft.printf("[Ethernet] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac6[0], mac6[1], mac6[2], mac6[3], mac6[4], mac6[5]);
  //SD infomation
  //  if(SD.begin()){
  //    cardType = SD.cardType();
  //    if(cardType != CARD_NONE){
  //      SD_detection = true;
  //    }
  //  }

  //  if(SD_detection){
  //    r++; tft.setCursor(0, 16*r, 2);
  //    if(cardType == CARD_MMC){
  //        tft.println("SD Card Type: MMC");
  //    } else if(cardType == CARD_SD){
  //        tft.println("SD Card Type: SDSC");
  //    } else if(cardType == CARD_SDHC){
  //        tft.println("SD Card Type: SDHC");
  //    } else {
  //        tft.println("SD Card Type: UNKNOWN");
  //    }
  //    r++; tft.setCursor(0, 16*r, 2);
  //    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  //    tft.printf("SD Card Size: %lluMB\n", cardSize);
  //    r++; tft.setCursor(0, 16*r, 2);
  //    tft.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  //    r++; tft.setCursor(0, 16*r, 2);
  //    tft.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
  //  }

  while (1) {
    int16_t tp = touch_check();
    switch (tp) {
      case TOUCH_L:
        refresh_page(-1);
        return;
        break;
      case TOUCH_C: break;
      case TOUCH_R:
        refresh_page(1);
        return;
        break;
      default: break;
    }
  }
}

//ページ1　グラフィックテスト
void PAGE1(void) {
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.println("Page 1 Cube Demo");
  while (1) {
    fillArc(160, 120, inc * 6, 1, 140, 100, 10, rainbow(col));
    fillArc(160, 120, ((inc * 2) % 60) * 6, 1, 120, 80, 30, rainbow(col));
    fillArc(160, 120, inc * 6, 1, 42, 42, 42, rainbow(col));
    inc++;
    col += 1;
    if (col > 191) col = 0;
    if (inc > 59) inc = 0;
    delay(LOOP_DELAY);

    int16_t tp = touch_check();
    switch (tp) {
      case TOUCH_L:
        refresh_page(-1);
        return;
        break;
      case TOUCH_C: break;
      case TOUCH_R:
        refresh_page(1);
        return;
        break;
      default: break;
    }
  }
}

//ページ2　CAN情報表示
void PAGE2(void) {
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.println("Page 2 CAN Receiver");

  int row = 0;
  int ROW_MAX = 14;
  int COL_MAX = 40;
  char tarminal[ROW_MAX][COL_MAX];
  for (int i = 0; i < ROW_MAX; i++) {
    for (int j = 0; j < COL_MAX; j++) {
      tarminal[i][j] = ' ';
    }
  }
  //ディスプレイ更新
  tft.fillRect(0, 16, 320, 240, TFT_DARKGREY);
  for (int i = 0; i < ROW_MAX; i++) {
    tft.setCursor(0, (i + 1) * 16, 2);
    tft.print(tarminal[i]);
  }
  while (1) {
    if (can_recv) {
      can_recv = false;
      //受信データをすべて取得
      int16_t l = sprintf(tarminal[row], "ID=%d, N=%d %s",
                          can_id,
                          can_packetSize,
                          can_data);

      //ディスプレイ更新
      tft.fillRect(0, 16, 320, 240, TFT_DARKGREY);
      for (int i = 0; i < ROW_MAX; i++) {
        tft.setCursor(0, (i + 1) * 16, 2);
        tft.print(tarminal[i]);
      }

      row++;
      if (row >= ROW_MAX) {
        row = ROW_MAX - 1;
        for (int i = 0; i < ROW_MAX - 1; i++) {
          for (int j = 0; j < COL_MAX; j++) {
            tarminal[i][j] = tarminal[i + 1][j];
          }
        }
      }
    }
    int16_t tp = touch_check();
    switch (tp) {
      case TOUCH_L:
        refresh_page(-1);
        return;
        break;
      case TOUCH_C: break;
      case TOUCH_R:
        refresh_page(1);
        return;
        break;
      default: break;
    }
  }
}

//ページ2　RS485ターミナル
void PAGE3(void) {
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.println("Page 3 RS485 Receiver");


  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  // Zero the array
  for (byte i = 0; i < 18; i++) blank[i] = 0;
  while (1) {
    data = Serial2.read();
    // If it is a CR or we are near end of line then scroll one line
    if (data == '\r' || xPos > 231) {
      xPos = 0;
      yDraw = scroll_line();  // It can take 13ms to scroll and blank 16 pixel lines
    }
    if (data > 31 && data < 128) {
      xPos += tft.drawChar(data, xPos, yDraw, 2);
      blank[(18 + (yStart - TOP_FIXED_AREA) / TEXT_HEIGHT) % 19] = xPos;  // Keep a record of line lengths
    }
    //change_colour = 1; // Line to indicate buffer is being emptied
    int16_t tp = touch_check();
    switch (tp) {
      case TOUCH_L:
        refresh_page(-1);
        return;
        break;
      case TOUCH_C: break;
      case TOUCH_R:
        refresh_page(1);
        return;
        break;
      default: break;
    }
  }
}



void rs485_test(void) {
  uint8_t sbuf[100];
  int rl = 0;
  if (Serial.available()) {
    while (Serial.available()) {  //Serial2に受信データがあるか
      sbuf[rl] = Serial.read();
      rl++;
    }
    Serial2.write(sbuf, rl);
  }
  if (Serial2.available()) {
    while (Serial2.available()) {  //Serial2に受信データがあるか
      sbuf[rl] = Serial2.read();
      rl++;
    }
    Serial.write(sbuf, rl);
  }
}

void can_check(void) {
  if (can_recv) {
    can_recv = false;
    char sbuf[100];
    int16_t l = sprintf(sbuf, "ID=%d, N=%d %s\r\n",
                        can_id,
                        can_packetSize,
                        can_data);
    Serial.write(sbuf, l);
  }

  timer_1000ms++;
  if (timer_1000ms > 1000) {

    timer_1000ms = 0;
    // put your main code here, to run repeatedly:
    Serial.print("Sending packet ... ");

    CAN.beginPacket(0x12);
    CAN.write('h');
    CAN.write('e');
    CAN.write('l');
    CAN.write('l');
    CAN.write('o');
    CAN.endPacket();

    Serial.println("done");
  }
  delay(1);
}

void onReceive(int packetSize) {
  // received a packet
  //Serial.print("Received ");

  //if (CAN.packetExtended()) {
  //Serial.print("extended ");
  //}

  //if (CAN.packetRtr()) {
  // Remote transmission request, packet contains no data
  //Serial.print("RTR ");
  //}

  //Serial.print("packet with id 0x");
  //Serial.print(CAN.packetId(), HEX);
  can_id = CAN.packetId();
  can_packetSize = packetSize;

  if (CAN.packetRtr()) {
    //Serial.print(" and requested length ");
    //Serial.println(CAN.packetDlc());
  } else {
    //Serial.print(" and length ");
    //Serial.println(packetSize);

    // only print packet data for non-RTR packets
    int t = 0;
    while (CAN.available()) {
      //Serial.print((char)CAN.read());
      can_data[t] = CAN.read();
      t++;
    }
    //Serial.println();
  }
  can_recv = true;
  //Serial.println();
}



// #########################################################################
// Draw a circular or elliptical arc with a defined thickness
// #########################################################################

// x,y == coords of centre of arc
// start_angle = 0 - 359
// seg_count = number of 6 degree segments to draw (60 => 360 degree arc)
// rx = x axis outer radius
// ry = y axis outer radius
// w  = width (thickness) of arc in pixels
// colour = 16-bit colour value
// Note if rx and ry are the same then an arc of a circle is drawn

void fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour) {

  byte seg = 6;  // Segments are 3 degrees wide = 120 segments for 360 degrees
  byte inc = 6;  // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Calculate first pair of coordinates for segment start
  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x0 = sx * (rx - w) + x;
  uint16_t y0 = sy * (ry - w) + y;
  uint16_t x1 = sx * rx + x;
  uint16_t y1 = sy * ry + y;

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

    // Copy segment end to segment start for next segment
    x0 = x2;
    y0 = y2;
    x1 = x3;
    y1 = y3;
  }
}

// #########################################################################
// Return the 16-bit colour with brightness 0-100%
// #########################################################################
unsigned int brightness(unsigned int colour, int brightness) {
  byte red = colour >> 11;
  byte green = (colour & 0x7E0) >> 5;
  byte blue = colour & 0x1F;

  blue = (blue * brightness) / 100;
  green = (green * brightness) / 100;
  red = (red * brightness) / 100;

  return (red << 11) + (green << 5) + blue;
}

// #########################################################################
// Return a 16-bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value) {
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  switch (state) {
    case 0:
      green++;
      if (green == 64) {
        green = 63;
        state = 1;
      }
      break;
    case 1:
      red--;
      if (red == 255) {
        red = 0;
        state = 2;
      }
      break;
    case 2:
      blue++;
      if (blue == 32) {
        blue = 31;
        state = 3;
      }
      break;
    case 3:
      green--;
      if (green == 255) {
        green = 0;
        state = 4;
      }
      break;
    case 4:
      red++;
      if (red == 32) {
        red = 31;
        state = 5;
      }
      break;
    case 5:
      blue--;
      if (blue == 255) {
        blue = 0;
        state = 0;
      }
      break;
  }
  return red << 11 | green << 5 | blue;
}

// ##############################################################################################
// Call this function to scroll the display one text line
// ##############################################################################################
int scroll_line() {
  int yTemp = yStart;  // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line
  tft.fillRect(0, yStart, blank[(yStart - TOP_FIXED_AREA) / TEXT_HEIGHT], TEXT_HEIGHT, TFT_BLACK);

  // Change the top of the scroll area
  yStart += TEXT_HEIGHT;
  // The value must wrap around as the screen memory is a circular buffer
  if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - YMAX + BOT_FIXED_AREA);
  // Now we can scroll the display
  scrollAddress(yStart);
  return yTemp;
}

// ##############################################################################################
// Setup a portion of the screen for vertical scrolling
// ##############################################################################################
// We are using a hardware feature of the display, so we can only scroll in portrait orientation
void setupScrollArea(uint16_t tfa, uint16_t bfa) {
  tft.writecommand(ILI9341_VSCRDEF);  // Vertical scroll definition
  tft.writedata(tfa >> 8);            // Top Fixed Area line count
  tft.writedata(tfa);
  tft.writedata((YMAX - tfa - bfa) >> 8);  // Vertical Scrolling Area line count
  tft.writedata(YMAX - tfa - bfa);
  tft.writedata(bfa >> 8);  // Bottom Fixed Area line count
  tft.writedata(bfa);
}

// ##############################################################################################
// Setup the vertical scrolling start address pointer
// ##############################################################################################
void scrollAddress(uint16_t vsp) {
  tft.writecommand(ILI9341_VSCRSADD);  // Vertical scrolling pointer
  tft.writedata(vsp >> 8);
  tft.writedata(vsp);
}
