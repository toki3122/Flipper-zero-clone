#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <math.h>
#include <WiFi.h>
#include <HijelHID_BLEMouse.h>
#include "esp_wifi.h"
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <IRremoteESP8266.h>
#include <SD.h>
#include <SPI.h>

#define IR_RX_PIN 25
#define IR_TX_PIN 26

// ====================== PINS ======================
#define BTN_UP     4
#define BTN_DOWN   16
#define BTN_LEFT   17
#define BTN_RIGHT  18
#define BTN_SELECT 19
#define BUZZER     23
#define USECPERTICK 50
// ====================== OLED ======================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ====================== DEBOUNCE ======================
unsigned long lastBtnTime[5] = { 0 };
const unsigned long DEBOUNCE = 150;
const uint8_t btnPins[5] = { BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_SELECT };
#define IDX_UP    0
#define IDX_DOWN  1
#define IDX_LEFT  2
#define IDX_RIGHT 3
#define IDX_SEL   4

bool btnPressed(uint8_t idx) {
  if (digitalRead(btnPins[idx]) == LOW && millis() - lastBtnTime[idx] > DEBOUNCE) {
    lastBtnTime[idx] = millis();
    return true;
  }
  return false;
}

// raw low = held, no debounce stamp update
bool btnHeld(uint8_t idx) {
  return digitalRead(btnPins[idx]) == LOW;
}

// ======================================================
// GLOBAL APP STATE
// ======================================================
enum AppState { STATE_BOOT, STATE_MENU, STATE_MUSIC, STATE_SNAKE, STATE_WIFI, STATE_MOUSE, STATE_DEAUTH, STATE_IR, STATE_SD};
AppState appState = STATE_BOOT;
enum DeauthMode { DEAUTH_SCANNER, DEAUTH_TRANSMITTER };
DeauthMode deauthMode = DEAUTH_SCANNER;
bool inDeauthSubmenu = false;
// ====================== SD CARD ======================
bool sdMounted = false;

// SD Pins (using your available GPIOs)
#define SD_CS     5
#define SD_SCK    2
#define SD_MOSI   15
#define SD_MISO   12

// SD Tools
String sdFiles[20];
uint8_t sdFileCount = 0;
int8_t sdFileSel = 0;
int8_t menuSel = 0;
const uint8_t MENU_COUNT = 7;
const char* const menuLabels[] PROGMEM = { "Music", "Snake", "WiFi Scan", "BT Mouse", "DeAuth", "IR Remote" ,"SD Tools"};

// ======================================================
// SONGS  (unchanged)
// ======================================================
const char* songs[] = {
  "Mario", "Tetris", "Zelda", "Imperial", "Pacman",
  "Jingle Bells", "FF Prelude", "Megalovania", "Bonetrousle", "Death By Glamour"
};

const char* rtttlSongs[] = {
  "Mario:d=4,o=5,b=200:"
  "e,e,p,e,p,c,e,g,p,p,g4,p,p,"
  "c,p,p,g4,p,p,e4,p,p,a4,p,b4,p,a#4,a4,p,"
  "g4,e,g,a,p,f,g,p,e,p,c,d,b4,p,"
  "c,p,p,g4,p,p,e4,p,p,a4,p,b4,p,a#4,a4,p,"
  "g4,e,g,a,p,f,g,p,e,p,c,d,b4",

  "Tetris:d=4,o=5,b=144:"
  "e,b4,c,d,c,b4,a4,a4,c,e,d,c,b4,b4,c,d,e,c,a4,a4,p,"
  "d,f,a,g,f,e,c,e,d,c,b4,b4,c,d,e,c,a4,a4,p,"
  "e,b4,c,d,c,b4,a4,a4,c,e,d,c,b4,b4,c,d,e,c,a4,a4,p,"
  "d,f,a,g,f,e,c,e,d,c,b4,b4,c,d,e,c,a4",

  "Zelda:d=4,o=5,b=120:"
  "a4,d,f,a4,d,f,a4,d,f,g,f,e,f,p,"
  "a4,d,f,a4,d,f,a4,d,f,e,c#,d,p,"
  "f,a,c6,b,a,f,a,p,"
  "f,g,a,a#,a,g,f,e,d,p,"
  "a4,d,f,a4,d,f,g,f,e,f",

  "Imperial:d=4,o=5,b=150:"
  "a4,a4,a4,f4,8c,a4,f4,8c,a4,p,"
  "e,e,e,f,8c,g#4,f4,8c,a4,p,"
  "a,2a,a,2a#,8a,8a,a#,8a,8a#,g,8f,8g,a,p,"
  "a,2a,f4,8f4,f4,g,8e,8f,g,p,"
  "8a4,8a4,a4,8a#4,8a#4,a#4,a4,g#4,8g4,8f#4,g4",

  "Pacman:d=4,o=5,b=140:"
  "b4,b5,f#5,d#5,b5,f#5,d#5,c5,c6,g5,e5,c6,g5,e5,"
  "b4,b5,f#5,d#5,b5,f#5,d#5,d#5,e5,f5,f5,f#5,g5,g5,"
  "g#5,a5,b5,p,a5,p,g#5,p,"
  "b4,b5,f#5,d#5,b5,f#5,d#5,c5,c6,g5,e5,c6,g5,e5",

  "Jingle:d=4,o=5,b=180:"
  "e,e,e,p,e,e,e,p,e,g,c,8d,e,p,"
  "f,f,f,8f,f,e,e,8e,8e,e,d,d,e,d,p,g,p,"
  "e,e,e,p,e,e,e,p,e,g,c,8d,e,p,"
  "f,f,f,8f,f,e,e,8e,8e,g,g,f,d,c,p,"
  "g,e,d,c,g4,p,g4,g4,g4,p,"
  "g,e,d,c,a4,p,a4,a4,a4,p,"
  "a4,f,e,d,b4,p,b4,b4,b4",

  "FF1:d=8,o=5,b=140:"
  "c,e,g,c6,e6,g,c6,e6,c,e,g,c6,e6,g,c6,e6,"
  "c,f,a,c6,f6,a,c6,f6,c,f,a,c6,f6,a,c6,f6,"
  "b4,d,g,b,d6,g,b,d6,b4,d,g,b,d6,g,b,d6,"
  "c,e,g,c6,e6,g,c6,e6,c,e,g,c6,e6,g,c6,e6,"
  "4c6,4g,4e,4c",

  "Megalovania:d=16,o=5,b=180:"
  "d,d,d6,8p,a,8p,g#,g,f,8d,f,g,c,c,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "b4,b4,d6,8p,a,8p,g#,g,f,8d,f,g,a#4,a#4,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "d,d,d6,8p,a,8p,g#,g,f,8d,f,g,c,c,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "b4,b4,d6,8p,a,8p,g#,g,f,8d,f,g,a#4,a#4,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "f6,f6,f6,8f6,e6,8e6,d6,8d6,c6,8p,d6,8d6,f6,8g6,g#6,g6,f6,d6,f6,g6,"
  "f6,f6,f6,8f6,g6,8g#6,a6,8a6,g6,8f6,d6,8d6,f6,8g6,a6,8a6,c7,8a6,d7,8p,"
  "d,d,d6,8p,a,8p,g#,g,f,8d,f,g,c,c,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "b4,b4,d6,8p,a,8p,g#,g,f,8d,f,g,a#4,a#4,d6,8p,a,8p,g#,g,f,8d,f,g",

  "Bonetrousle:d=8,o=5,b=150:"
  "f,f,f,f,f,p,g,g,g,g,g,p,a,a,a,a,a,p,a#,a#,a#,a#,a#,p,"
  "c6,c6,c6,c6,c6,p,d6,d6,d6,d6,d6,p,e6,f6,e6,d6,c6,p,"
  "c6,d6,e6,f6,g6,a6,a#6,p,a6,g6,f6,e6,d6,c6,b,a,"
  "f,f,f,f,f,p,g,g,g,g,g,p,a,a,a,a,a,p,a#,a#,a#,a#,a#,p,"
  "c6,p,d6,p,e6,p,f6,p,g6,4p,a6,g6,f6,e6,4d6",

  "DeathByGlam:d=16,o=5,b=148:"
  "8c#6,8b,8g#,8e,8g#,8b,8c#6,8b,8g#,8e,8g#,8b,c#6,b,c#6,8p,"
  "8d6,8c6,8a,8f,8a,8c6,8d6,8c6,8a,8f,8a,8c6,d6,c6,d6,8p,"
  "c#6,8c#6,b,8a,g#,8g#,f#,8e,f#,8f#,g#,8a,b,8c#6,d6,8p,"
  "e6,8e6,d6,8c#6,b,8b,a,8g#,a,8a,b,8c#6,d6,8e6,f#6,8p,"
  "c#6,8c#6,b,8a,g#,8g#,f#,8e,f#,8f#,g#,8a,b,8c#6,d6,8p,"
  "8e6,8d#6,8d6,8c#6,8c6,8b5,8a#5,8a5,8g#5,8g5,8f#5,8f5,4e5"
};

const int totalSongs = 10;

// ====================== MUSIC STATE ======================
int  currentSong  = 0;
int  playingIndex = -1;
bool isPlaying    = false;
const char* rtttlPtr   = nullptr;
int  defaultDur   = 4;
int  defaultOct   = 5;
int  bpm          = 63;
unsigned long wholenote    = 0;
unsigned long noteStart    = 0;
unsigned long noteDuration = 0;
bool notePlaying  = false;
uint8_t animFrame = 0;

// ======================================================
// SNAKE STATE  (unchanged)
// ======================================================
const uint8_t CELL    = 4;
const uint8_t GRID_W  = 128 / CELL;
const uint8_t GRID_H  = (64 - 12) / CELL;
const uint8_t HUD_H   = 12;
const uint8_t MAX_LEN = 120;
int8_t   snakeX[MAX_LEN], snakeY[MAX_LEN];
uint8_t  snakeLen;
int8_t   dirX, dirY, nextDirX, nextDirY;
uint8_t  foodX, foodY;
uint16_t snakeScore;
uint16_t highScore    = 0;
unsigned long lastMoveTime = 0;
uint16_t moveInterval = 130;
enum SnakeState { SNAKE_PLAYING, SNAKE_PAUSED, SNAKE_OVER };
SnakeState snakeState = SNAKE_PLAYING;

// ======================================================
// WIFI STATE  (unchanged)
// ======================================================
enum WifiState { WIFI_IDLE, WIFI_SCANNING, WIFI_RESULTS };
WifiState wifiState = WIFI_IDLE;
struct NetEntry { char ssid[33]; int8_t rssi; uint8_t enc; };
const uint8_t MAX_NETS = 20;
NetEntry nets[MAX_NETS];
uint8_t  netCount     = 0;
int8_t   netScroll    = 0;
uint8_t  scanAnim     = 0;
unsigned long scanAnimTimer = 0;
unsigned long scanStartMs   = 0;
uint8_t rssiBars(int8_t rssi) {
  if (rssi >= -55) return 4;
  if (rssi >= -65) return 3;
  if (rssi >= -75) return 2;
  return 1;
}

// ======================================================
// BLE MOUSE STATE
// ======================================================
HijelBLEMouse bleMouse("toki_gachi", "toki");

enum MouseMode { MOUSE_MOVE, MOUSE_SCROLL };
MouseMode mouseMode = MOUSE_MOVE;

// connection / advertising state shown on OLED
enum BleState { BLE_ADVERTISING, BLE_CONNECTED };
BleState bleState = BLE_ADVERTISING;

// movement
const int8_t MOVE_STEP   = 18;   // pixels per tick when held
const int8_t SCROLL_STEP = 3;    // scroll units per tick
const unsigned long HOLD_DELAY  = 350;  // ms before hold-repeat kicks in
const unsigned long HOLD_REPEAT = 40;   // ms between repeat ticks

// per-direction hold tracking (UP DOWN LEFT RIGHT)
unsigned long holdStart[4]  = {0};
unsigned long holdLastTick[4] = {0};
bool          holdActive[4] = {false};

// multi-tap SELECT for click type
uint8_t       selTapCount  = 0;
unsigned long selTapWindow = 0;
const unsigned long TAP_WINDOW = 400;   // ms window for multi-tap

// hold-SELECT = drag
bool          selHeld       = false;
unsigned long selHoldStart  = 0;
const unsigned long HOLD_CLICK = 450;   // ms to register as hold
bool          dragging       = false;

// last action string shown on OLED
char lastAction[20] = "---";

// cursor ghost position (0-127, 0-63) just for OLED visualisation
int16_t cursorVX = 64, cursorVY = 32;

void beepConnect() {
  tone(BUZZER, 880,  60); delay(70);
  tone(BUZZER, 1320, 60); delay(70);
  noTone(BUZZER);
}
void beepClick(uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    tone(BUZZER, 1100, 30);
    delay(50);
  }
  noTone(BUZZER);
}

// ======================================================
// SOUND EFFECTS  (unchanged)
// ======================================================
void beepEat()  { tone(BUZZER, 880, 40); }
void beepDie()  {
  for (uint8_t i = 0; i < 3; i++) { tone(BUZZER, 200-i*40, 120); delay(130); }
  noTone(BUZZER);
}
void beepScan() {
  tone(BUZZER, 1200, 30); delay(40);
  tone(BUZZER, 1600, 30); delay(40);
  noTone(BUZZER);
}

// ======================================================
// SNAKE HELPERS  (unchanged)
// ======================================================
void snakeSpawnFood() {
  bool onSnake;
  do {
    onSnake = false;
    foodX = random(0, GRID_W);
    foodY = random(0, GRID_H);
    for (uint8_t i = 0; i < snakeLen; i++)
      if (snakeX[i]==foodX && snakeY[i]==foodY) { onSnake=true; break; }
  } while (onSnake);
}
void snakeReset() {
  snakeLen=3; dirX=1; dirY=0; nextDirX=1; nextDirY=0;
  snakeScore=0; moveInterval=130;
  for (uint8_t i=0;i<snakeLen;i++){snakeX[i]=8-i;snakeY[i]=GRID_H/2;}
  snakeSpawnFood(); snakeState=SNAKE_PLAYING; lastMoveTime=millis();
}

// ======================================================
// MUSIC ENGINE  (unchanged)
// ======================================================
void stopMusic() {
  noTone(BUZZER); isPlaying=false; playingIndex=-1;
  rtttlPtr=nullptr; notePlaying=false;
}
void playSong(int index) {
  if (index<0||index>=totalSongs) return;
  stopMusic(); playingIndex=index; isPlaying=true; rtttlPtr=rtttlSongs[index];
  while(*rtttlPtr&&*rtttlPtr!=':') rtttlPtr++;
  if(*rtttlPtr==':') rtttlPtr++;
  defaultDur=4; defaultOct=5; bpm=120;
  while(*rtttlPtr&&*rtttlPtr!=':') {
    while(*rtttlPtr==' ') rtttlPtr++;
    char key=*rtttlPtr;
    if(*(rtttlPtr+1)=='='){
      rtttlPtr+=2; int val=0;
      while(isdigit(*rtttlPtr)){val=val*10+(*rtttlPtr-'0');rtttlPtr++;}
      if(key=='d') defaultDur=val;
      else if(key=='o') defaultOct=val;
      else if(key=='b') bpm=val;
    } else rtttlPtr++;
    while(*rtttlPtr==','||*rtttlPtr==' ') rtttlPtr++;
  }
  if(*rtttlPtr==':') rtttlPtr++;
  wholenote=(60000UL*4)/bpm; notePlaying=false; animFrame=0;
}
void updateRTTTL() {
  if(!isPlaying||!rtttlPtr) return;
  if(notePlaying){
    if(millis()-noteStart<noteDuration) return;
    notePlaying=false; noTone(BUZZER); delay(8);
  }
  while(*rtttlPtr==','||*rtttlPtr==' '||*rtttlPtr=='\n') rtttlPtr++;
  if(*rtttlPtr=='\0'){stopMusic();return;}
  int duration=defaultDur,octave=defaultOct,note=-1;
  if(isdigit(*rtttlPtr)){duration=0;while(isdigit(*rtttlPtr)){duration=duration*10+(*rtttlPtr-'0');rtttlPtr++;}}
  switch(tolower(*rtttlPtr)){
    case 'c':note=0;break; case 'd':note=2;break; case 'e':note=4;break;
    case 'f':note=5;break; case 'g':note=7;break; case 'a':note=9;break;
    case 'b':note=11;break; case 'p':note=-1;break;
    default:rtttlPtr++;return;
  }
  rtttlPtr++;
  if(*rtttlPtr=='#'){if(note>=0)note++;rtttlPtr++;}
  if(isdigit(*rtttlPtr)){octave=*rtttlPtr-'0';rtttlPtr++;}
  bool dotted=false;
  if(*rtttlPtr=='.'){dotted=true;rtttlPtr++;}
  noteDuration=wholenote/duration;
  if(dotted) noteDuration+=noteDuration/2;
  if(noteDuration<20) noteDuration=20;
  if(note<0) noTone(BUZZER);
  else{int midi=(octave+1)*12+note;float freq=440.0f*powf(2.0f,(midi-69)/12.0f);tone(BUZZER,(int)(freq+0.5f));}
  noteStart=millis(); notePlaying=true; animFrame++;
}

// ======================================================
// WIFI ENGINE  (unchanged)
// ======================================================
void startWifiScan() {
  wifiState=WIFI_SCANNING; netCount=0; netScroll=0; scanAnim=0;
  scanStartMs=millis(); scanAnimTimer=millis();
  WiFi.mode(WIFI_STA); WiFi.setSleep(false);
  WiFi.scanNetworks(true); beepScan();
}
void updateWifiScan() {
  if(millis()-scanAnimTimer>200){
    scanAnimTimer=millis(); scanAnim=(scanAnim+1)%4;
    // drawWifiScanning called below in loop
  }
  int n=WiFi.scanComplete();
  if(n==WIFI_SCAN_RUNNING) return;
  netCount=0;
  if(n>0){
    uint8_t limit=min((int)MAX_NETS,n);
    for(uint8_t i=0;i<limit;i++){
      char ssidBuf[33] = {0};
      WiFi.SSID(i).toCharArray(ssidBuf, 33);
      if(ssidBuf[0]=='\0') strncpy(ssidBuf,"(hidden)",33);
      strncpy(nets[netCount].ssid, ssidBuf, 33);
      nets[netCount].rssi=(int8_t)WiFi.RSSI(i);
      nets[netCount].enc=(WiFi.encryptionType(i)==WIFI_AUTH_OPEN)?0:1;
      netCount++;
    }
  }
  WiFi.scanDelete(); wifiState=WIFI_RESULTS; beepScan();
}

// ======================================================
// DRAW — BOOT  (unchanged)
// ======================================================
void drawBoot() {
  u8g2.clearBuffer();
  u8g2.drawRFrame(2,2,124,60,4);
  u8g2.setFont(u8g2_font_9x15B_tf);
  u8g2.drawStr(22,28,"toki_gachi");
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(22,44,"^-^");
  u8g2.drawStr(22,56,"v5  ESP32-C3");
  u8g2.sendBuffer();
  delay(1400);
}

// ======================================================
// DRAW — MAIN MENU  (3 visible tiles, scrolls L/R)
// ======================================================
// draws icon for menu item index idx at pixel origin (tx, TY)
void drawMenuIcon(uint8_t idx, uint8_t tx, uint8_t TY, uint8_t TW){
  uint8_t cx=tx+TW/2, iy=TY+6;
  if(idx==0){
    u8g2.drawVLine(cx+3,iy,8);
    u8g2.drawHLine(cx+1,iy,3);
    u8g2.drawDisc(cx+1,iy+8,3);
  } else if(idx==1){
    u8g2.drawBox(cx-5,iy,  6,4);
    u8g2.drawBox(cx-5,iy+4,6,4);
    u8g2.drawBox(cx+1,iy+4,6,4);
    u8g2.drawBox(cx+1,iy+8,6,4);
    u8g2.drawDisc(cx+9,iy+1,2);
  } else if(idx==2){
    u8g2.drawDisc(cx,iy+13,2,U8G2_DRAW_ALL);
    u8g2.drawCircle(cx,iy+13,5, U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
    u8g2.drawCircle(cx,iy+13,9, U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
    u8g2.drawCircle(cx,iy+13,13,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
  } else if(idx==3){
    u8g2.drawVLine(cx,  iy,   16);
    u8g2.drawLine (cx,  iy,   cx+4,iy+4);
    u8g2.drawLine (cx+4,iy+4, cx,  iy+8);
    u8g2.drawLine (cx,  iy+8, cx+4,iy+12);
    u8g2.drawLine (cx+4,iy+12,cx,  iy+16);
  } else if(idx==4){
    // deauth: wifi arcs + X strike
    u8g2.drawDisc(cx,iy+13,2,U8G2_DRAW_ALL);
    u8g2.drawCircle(cx,iy+13,5, U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
    u8g2.drawCircle(cx,iy+13,10,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
    u8g2.drawLine(cx-8,iy+2, cx+8,iy+14);
    u8g2.drawLine(cx+8,iy+2, cx-8,iy+14);
  } else {
    // IR remote: simple remote icon (rectangle body + signal dot)
    u8g2.drawRFrame(cx-4,iy,  10,16,2);  // remote body
    u8g2.drawDisc  (cx+1, iy+4, 2);      // top button
    u8g2.drawBox   (cx-2, iy+8, 6, 2);   // middle bar
    u8g2.drawBox   (cx-2, iy+12,6, 2);   // bottom bar
    // signal waves right side
    u8g2.drawCircle(cx+10,iy+8,3,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawCircle(cx+10,iy+8,6,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_LOWER_RIGHT);
  }
}

void drawMenu() {
  u8g2.clearBuffer();

  // header
  u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(3,10,"_toki_gachi_");
  u8g2.setDrawColor(1);

  // show 3 tiles: prev | selected (big center) | next
  // layout: left tile 0-38, center tile 40-87, right tile 89-127
  const uint8_t TY=15, TH=40;
  const uint8_t SW=49;  // selected tile width
  const uint8_t NW=36;  // neighbour tile width

  // tile positions
  const uint8_t lx=0,  lw=NW;   // left neighbour
  const uint8_t cx=39, cw=SW;   // center selected
  const uint8_t rx=91, rw=NW;   // right neighbour

  int8_t prevIdx = (menuSel-1+MENU_COUNT)%MENU_COUNT;
  int8_t nextIdx = (menuSel+1)%MENU_COUNT;

  // left neighbour — outline only, dimmer (just frame)
  u8g2.drawRFrame(lx,TY,lw,TH,3);
  drawMenuIcon(prevIdx, lx, TY, lw);
  u8g2.setFont(u8g2_font_4x6_tr);
  { uint8_t lbw=u8g2.getStrWidth(menuLabels[prevIdx]);
    u8g2.drawStr(lx+(lw-lbw)/2, TY+TH-4, menuLabels[prevIdx]); }

  // right neighbour — outline only
  u8g2.drawRFrame(rx,TY,rw,TH,3);
  drawMenuIcon(nextIdx, rx, TY, rw);
  { uint8_t rbw=u8g2.getStrWidth(menuLabels[nextIdx]);
    u8g2.drawStr(rx+(rw-rbw)/2, TY+TH-4, menuLabels[nextIdx]); }

  // center selected — filled, inverted
  u8g2.drawRBox(cx,TY,cw,TH,3);
  u8g2.setDrawColor(0);
  drawMenuIcon(menuSel, cx, TY, cw);
  u8g2.setFont(u8g2_font_4x6_tr);
  { uint8_t cbw=u8g2.getStrWidth(menuLabels[menuSel]);
    u8g2.drawStr(cx+(cw-cbw)/2, TY+TH-4, menuLabels[menuSel]); }
  u8g2.setDrawColor(1);

  // scroll dots at bottom
  for(uint8_t i=0;i<MENU_COUNT;i++){
    uint8_t dx=50+i*8;
    if(i==(uint8_t)menuSel) u8g2.drawDisc  (dx,60,2);
    else                     u8g2.drawCircle(dx,60,2);
  }

  // left/right arrows
  u8g2.drawStr(2,  62,"<");
  u8g2.drawStr(122,62,">");

  u8g2.sendBuffer();
}

// ======================================================
// DRAW — MUSIC  (unchanged)
// ======================================================
void drawMusic() {
  u8g2.clearBuffer();
  u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(3,10,isPlaying?"NOW PLAYING":"MUSIC PLAYER");
  u8g2.setDrawColor(1);
  if(isPlaying){
    u8g2.setFont(u8g2_font_9x15B_tf);
    int nw=u8g2.getStrWidth(songs[playingIndex]);
    u8g2.drawStr((128-nw)/2,36,songs[playingIndex]);
    const uint8_t barX[5]={14,30,46,62,78};
    for(uint8_t b=0;b<5;b++){
      uint8_t h=3+(uint8_t)(3.5f*fabsf(sinf((animFrame*0.35f)+b*0.9f)));
      u8g2.drawBox(barX[b],50-h,8,h);
    }
    u8g2.drawHLine(0,55,128);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(22,63,"SEL:stop  LEFT:back");
  } else {
    for(int i=-1;i<=1;i++){
      int idx=(currentSong+i+totalSongs)%totalSongs;
      int y=28+(i+1)*13;
      if(i==0){
        u8g2.drawRBox(0,y-10,122,13,2); u8g2.setDrawColor(0);
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(18,y,songs[idx]);
        u8g2.drawTriangle(4,y-7,4,y+1,12,y-3);
        u8g2.setDrawColor(1);
      } else {
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(18,y,songs[idx]);
      }
    }
    for(int i=0;i<totalSongs;i++){
      if(i==currentSong) u8g2.drawDisc  (124,16+i*4,2);
      else                u8g2.drawCircle(124,16+i*4,2);
    }
    u8g2.drawHLine(0,55,128);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(2,63,"UP/DN:scroll SEL:play LEFT:back");
  }
  u8g2.sendBuffer();
}

// ======================================================
// DRAW — SNAKE  (unchanged)
// ======================================================
void drawSnake() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);
  char buf[24];
  sprintf(buf,"S:%u",snakeScore); u8g2.drawStr(1,9,buf);
  sprintf(buf,"B:%u",highScore);  u8g2.drawStr(50,9,buf);
  uint8_t speedLevel=(130-moveInterval)/10+1;
  if(speedLevel>8) speedLevel=8;
  for(uint8_t p=0;p<speedLevel;p++) u8g2.drawBox(100+p*3,3,2,6);
  u8g2.drawHLine(0,HUD_H,128);
  if(snakeState==SNAKE_OVER){
    u8g2.setFont(u8g2_font_6x10_tr); u8g2.drawStr(22,32,"GAME OVER!");
    u8g2.setFont(u8g2_font_5x7_tr);
    sprintf(buf,"Score:%u  Best:%u",snakeScore,highScore); u8g2.drawStr(4,46,buf);
    u8g2.setFont(u8g2_font_4x6_tr); u8g2.drawStr(10,58,"SEL:restart  LEFT:menu");
    u8g2.sendBuffer(); return;
  }
  if(snakeState==SNAKE_PAUSED){
    u8g2.setFont(u8g2_font_6x10_tr); u8g2.drawStr(34,32,"PAUSED");
    u8g2.setFont(u8g2_font_4x6_tr); u8g2.drawStr(14,48,"SEL:resume  LEFT:menu");
    u8g2.sendBuffer(); return;
  }
  u8g2.drawFrame(foodX*CELL,HUD_H+foodY*CELL,CELL,CELL);
  u8g2.drawPixel(foodX*CELL+1,HUD_H+foodY*CELL+1);
  u8g2.drawPixel(foodX*CELL+2,HUD_H+foodY*CELL+2);
  for(uint8_t i=1;i<snakeLen;i++) u8g2.drawBox(snakeX[i]*CELL,HUD_H+snakeY[i]*CELL,CELL,CELL);
  uint8_t hx=snakeX[0]*CELL, hy=HUD_H+snakeY[0]*CELL;
  u8g2.drawBox(hx,hy,CELL,CELL); u8g2.setDrawColor(0);
  if     (dirX== 1){u8g2.drawPixel(hx+3,hy+1);u8g2.drawPixel(hx+3,hy+3);}
  else if(dirX==-1){u8g2.drawPixel(hx,  hy+1);u8g2.drawPixel(hx,  hy+3);}
  else if(dirY==-1){u8g2.drawPixel(hx+1,hy  );u8g2.drawPixel(hx+3,hy  );}
  else             {u8g2.drawPixel(hx+1,hy+3);u8g2.drawPixel(hx+3,hy+3);}
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}

// ======================================================
// DRAW — WIFI SCANNING  (unchanged)
// ======================================================
void drawWifiScanning() {
  u8g2.clearBuffer();
  u8g2.drawBox(0,0,128,13); u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr); u8g2.drawStr(3,10,"WIFI  SCANNER");
  u8g2.setDrawColor(1);
  uint8_t cx=64,cy=48;
  u8g2.drawDisc(cx,cy,3);
  const uint8_t radii[3]={10,18,26};
  for(uint8_t r=0;r<=scanAnim&&r<3;r++)
    u8g2.drawCircle(cx,cy,radii[r],U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
  const char* spinFrames[4]={"Scanning .  ","Scanning .. ","Scanning ...","Scanning    "};
  u8g2.setFont(u8g2_font_5x7_tr); u8g2.drawStr(28,62,spinFrames[scanAnim]);
  char ebuf[12]; sprintf(ebuf,"%lus",(millis()-scanStartMs)/1000);
  u8g2.setFont(u8g2_font_4x6_tr); u8g2.drawStr(104,62,ebuf);
  u8g2.sendBuffer();
}

// ======================================================
// DRAW — WIFI RESULTS  (unchanged)
// ======================================================
void drawWifiResults() {
  u8g2.clearBuffer();
  u8g2.drawBox(0,0,128,12); u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  char hbuf[22]; sprintf(hbuf,"FOUND: %u",netCount);
  u8g2.drawStr(3,10,hbuf); u8g2.setDrawColor(1);
  if(netCount==0){
    u8g2.setFont(u8g2_font_6x10_tr); u8g2.drawStr(14,36,"No networks :(");
    u8g2.setFont(u8g2_font_4x6_tr);  u8g2.drawStr(16,52,"SEL:rescan  LEFT:menu");
    u8g2.sendBuffer(); return;
  }
  const uint8_t VISIBLE=3;
  for(uint8_t i=0;i<VISIBLE;i++){
    int8_t idx=netScroll+i; if(idx>=netCount) break;
    uint8_t y=14+i*13;
    if(i==0){u8g2.drawRBox(0,y-1,119,12,2);u8g2.setDrawColor(0);}
    char truncSSID[18]; strncpy(truncSSID,nets[idx].ssid,17); truncSSID[17]='\0';
    u8g2.setFont(u8g2_font_5x7_tr); u8g2.drawStr(2,y+8,truncSSID);
    uint8_t bars=rssiBars(nets[idx].rssi);
    for(uint8_t b=0;b<4;b++){
      uint8_t bh=2+b*2,bx=93+b*4,by=y+9-bh;
      if(b<bars) u8g2.drawBox(bx,by,3,bh);
      else        u8g2.drawFrame(bx,by,3,bh);
    }
    if(nets[idx].enc){u8g2.drawFrame(110,y+3,6,5);u8g2.drawPixel(113,y+2);u8g2.drawPixel(112,y+2);}
    u8g2.setDrawColor(1);
  }
  if(netCount>VISIBLE){
    for(int i=0;i<netCount;i++){
      if(i==netScroll) u8g2.drawDisc  (124,16+i*4,2);
      else              u8g2.drawCircle(124,16+i*4,2);
    }
  }
  u8g2.drawHLine(0,55,128); u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2,63,"UP/DN:scroll SEL:rescan L:menu");
  u8g2.sendBuffer();
}

// ======================================================
// DRAW — BLE MOUSE
// ======================================================
void drawMouse() {
  u8g2.clearBuffer();

  // ── header ──────────────────────────────────────────
  u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  if(bleState==BLE_ADVERTISING)
    u8g2.drawStr(3,10,"BT: pairing...");
  else
    u8g2.drawStr(3,10,"BT MOUSE  paired");
  u8g2.setDrawColor(1);

  // ── mode badge ──────────────────────────────────────
  const char* modeTxt = (mouseMode==MOUSE_MOVE) ? "MOVE" : "SCROLL";
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawRFrame(2,15,30,11,2);
  u8g2.drawStr(4,23,modeTxt);

  // ── mini mouse icon (right side) ────────────────────
  // body outline
  u8g2.drawRFrame(98,14,18,26,4);
  // scroll wheel
  u8g2.drawRBox(105,16,4,8,1);
  // left/right button divider
  u8g2.drawVLine(107,14,8);
  // left button highlight if dragging
  if(dragging){
    u8g2.drawBox(99,15,7,7);
  }

  // ── cursor ghost (tiny crosshair in a 60×30 viewport) ─
  // viewport box
  u8g2.drawFrame(35,15,60,30);
  // map cursorVX(0-127),cursorVY(0-63) → viewport(35-95,15-45)
  int16_t vx = 35 + (int16_t)(cursorVX * 59L / 127);
  int16_t vy = 15 + (int16_t)(cursorVY * 29L / 63);
  vx = constrain(vx,36,94);
  vy = constrain(vy,16,44);
  // crosshair
  u8g2.drawHLine(vx-3, vy,   7);
  u8g2.drawVLine(vx,   vy-3, 7);
  // centre dot
  u8g2.drawPixel(vx, vy);

  // ── last action ─────────────────────────────────────
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(2,54,lastAction);

  // ── footer hint ─────────────────────────────────────
  u8g2.drawHLine(0,56,128);
  u8g2.setFont(u8g2_font_4x6_tr);
  if(mouseMode==MOUSE_MOVE)
    u8g2.drawStr(2,63,"SEL+UP:scroll  LONG-L:menu");
  else
    u8g2.drawStr(2,63,"L:move mode    LONG-L:menu");

  u8g2.sendBuffer();
}

// ======================================================
// BLE MOUSE ENGINE
// ======================================================

// Helper — clamp virtual cursor
void clampCursor(){
  cursorVX = constrain(cursorVX, 0, 127);
  cursorVY = constrain(cursorVY, 0, 63);
}

// Process one directional button for hold-repeat movement/scroll
void processDirBtn(uint8_t dirIdx, uint8_t btnIdx, int8_t mx, int8_t my, int8_t scrollDir){
  bool held = btnHeld(btnIdx);
  unsigned long now = millis();

  if(!held){
    holdActive[dirIdx] = false;
    holdStart[dirIdx]  = 0;
    return;
  }

  // first press detection via btnPressed stamp
  bool firstPress = (now - lastBtnTime[btnIdx] <= DEBOUNCE+5) && (now - lastBtnTime[btnIdx] > 0);

  if(!holdActive[dirIdx]){
    // waiting for hold threshold
    if(holdStart[dirIdx]==0) holdStart[dirIdx]=now;
    if(now - holdStart[dirIdx] >= HOLD_DELAY){
      holdActive[dirIdx]  = true;
      holdLastTick[dirIdx]= now;
    }
    return;
  }

  // hold repeat tick
  if(now - holdLastTick[dirIdx] >= HOLD_REPEAT){
    holdLastTick[dirIdx] = now;
    if(mouseMode==MOUSE_MOVE && bleMouse.isPaired()){
      bleMouse.move(mx, my);
      cursorVX += mx; cursorVY += my; clampCursor();
    } else if(mouseMode==MOUSE_SCROLL && bleMouse.isPaired()){
      bleMouse.addScroll(scrollDir);
    }
  }
}

void handleMouseInput(){
  unsigned long now = millis();

  // ── connection status ────────────────────────────────
  bleState = bleMouse.isPaired() ? BLE_CONNECTED : BLE_ADVERTISING;

  // ── LONG press LEFT = back to menu (any mode) ────────
  static unsigned long leftHoldStart = 0;
  static bool          leftHoldFired = false;
  if(btnHeld(IDX_LEFT)){
    if(leftHoldStart==0) leftHoldStart = now;
    if(!leftHoldFired && now-leftHoldStart > 800){
      leftHoldFired = true;
      if(dragging){ bleMouse.releaseAll(); dragging=false; }
      appState = STATE_MENU;
      leftHoldStart=0; return;
    }
  } else {
    if(!leftHoldFired && leftHoldStart!=0){
      // short release:
      // SCROLL mode → switch back to MOVE mode
      // MOVE mode   → cursor left already sent on btnPressed below
      if(mouseMode==MOUSE_SCROLL){
        mouseMode = MOUSE_MOVE;
        strcpy(lastAction,"-> MOVE mode");
      }
    }
    leftHoldStart=0; leftHoldFired=false;
  }

  // SCROLL mode: RIGHT tap switches to SCROLL mode
  if(mouseMode==MOUSE_MOVE && btnPressed(IDX_RIGHT) && btnHeld(IDX_UP)){
    // UP+RIGHT combo = enter scroll mode (optional, ignore for now)
  }
  // entering scroll mode: SELECT+RIGHT held is an alternative; keep simple:
  // pressing RIGHT while in MOVE and UP held → scroll. For now just use
  // the footer hint "LONG-L=menu / SEL-L=scroll"
  // Actually: mode toggle via SELECT long-press instead to free LEFT fully
  static unsigned long selModeHold = 0;
  static bool          selModeFired = false;
  if(btnHeld(IDX_SEL) && btnHeld(IDX_UP)){
    // SEL+UP held = enter scroll mode
    if(selModeHold==0) selModeHold=now;
    if(!selModeFired && now-selModeHold>500){
      selModeFired=true;
      mouseMode=MOUSE_SCROLL;
      strcpy(lastAction,"-> SCROLL mode");
    }
  } else { selModeHold=0; selModeFired=false; }

  if(!bleMouse.isPaired()) return;

  // ── directional hold-repeat ──────────────────────────
  // UP
  if(btnHeld(IDX_UP)){
    if(holdStart[0]==0) holdStart[0]=now;
    if(now-holdStart[0]>=HOLD_DELAY){
      if(now-holdLastTick[0]>=HOLD_REPEAT){
        holdLastTick[0]=now;
        if(mouseMode==MOUSE_MOVE){ bleMouse.move(0,-MOVE_STEP); cursorVY-=4; clampCursor(); }
        else                     { bleMouse.addScroll(SCROLL_STEP); strcpy(lastAction,"scroll UP"); }
      }
    }
  } else holdStart[0]=0;

  // DOWN
  if(btnHeld(IDX_DOWN)){
    if(holdStart[1]==0) holdStart[1]=now;
    if(now-holdStart[1]>=HOLD_DELAY){
      if(now-holdLastTick[1]>=HOLD_REPEAT){
        holdLastTick[1]=now;
        if(mouseMode==MOUSE_MOVE){ bleMouse.move(0,MOVE_STEP); cursorVY+=4; clampCursor(); }
        else                     { bleMouse.addScroll(-SCROLL_STEP); strcpy(lastAction,"scroll DN"); }
      }
    }
  } else holdStart[1]=0;

  // LEFT hold-repeat (MOVE mode, only while not firing long-press exit)
  if(mouseMode==MOUSE_MOVE){
    if(btnHeld(IDX_LEFT) && !leftHoldFired && leftHoldStart!=0 && (now-leftHoldStart < 800)){
      if(holdStart[2]==0) holdStart[2]=now;
      if(now-holdStart[2]>=HOLD_DELAY){
        if(now-holdLastTick[2]>=HOLD_REPEAT){
          holdLastTick[2]=now;
          bleMouse.move(-MOVE_STEP,0); cursorVX-=4; clampCursor();
        }
      }
    } else if(!btnHeld(IDX_LEFT)) holdStart[2]=0;
  }

  // RIGHT hold-repeat (MOVE mode only)
  if(mouseMode==MOUSE_MOVE){
    if(btnHeld(IDX_RIGHT)){
      if(holdStart[3]==0) holdStart[3]=now;
      if(now-holdStart[3]>=HOLD_DELAY){
        if(now-holdLastTick[3]>=HOLD_REPEAT){
          holdLastTick[3]=now;
          bleMouse.move(MOVE_STEP,0); cursorVX+=4; clampCursor();
        }
      }
    } else holdStart[3]=0;
  }

  // ── single-tap immediate step ─────────────────────────
  if(btnPressed(IDX_UP)){
    if(mouseMode==MOUSE_MOVE){ bleMouse.move(0,-MOVE_STEP); cursorVY-=4; clampCursor(); strcpy(lastAction,"move UP"); }
    else                     { bleMouse.addScroll(SCROLL_STEP); strcpy(lastAction,"scroll UP"); }
  }
  if(btnPressed(IDX_DOWN)){
    if(mouseMode==MOUSE_MOVE){ bleMouse.move(0,MOVE_STEP); cursorVY+=4; clampCursor(); strcpy(lastAction,"move DN"); }
    else                     { bleMouse.addScroll(-SCROLL_STEP); strcpy(lastAction,"scroll DN"); }
  }
  if(mouseMode==MOUSE_MOVE){
    if(btnPressed(IDX_LEFT)){
      bleMouse.move(-MOVE_STEP,0); cursorVX-=4; clampCursor();
      strcpy(lastAction,"move LT");
    }
    if(btnPressed(IDX_RIGHT)){
      bleMouse.move(MOVE_STEP,0); cursorVX+=4; clampCursor();
      strcpy(lastAction,"move RT");
    }
  }

  // ── SELECT — multi-tap + hold drag ──────────────────
  bool selDown = btnHeld(IDX_SEL);
  static bool selWasDown = false;

  if(selDown && !selWasDown){
    selWasDown   = true;
    selHoldStart = now;
    if(now - selTapWindow > TAP_WINDOW){
      selTapCount  = 1;
      selTapWindow = now;
    } else {
      selTapCount++;
    }
  }

  // drag threshold
  if(selDown && !dragging && (now-selHoldStart >= HOLD_CLICK)){
    dragging = true;
    bleMouse.press(MouseButton::Left);
    strcpy(lastAction,"DRAG hold");
    selTapCount = 0;
  }

  // release
  if(!selDown && selWasDown){
    selWasDown = false;
    if(dragging){
      bleMouse.release(MouseButton::Left);
      dragging = false;
      strcpy(lastAction,"drag END");
    }
  }

  // tap window expired — commit click
  if(!selDown && selTapCount>0 && (now-selTapWindow >= TAP_WINDOW)){
    if(selTapCount==1){
      bleMouse.click(MouseButton::Left);
      strcpy(lastAction,"LEFT click");
      beepClick(1);
    } else if(selTapCount==2){
      bleMouse.click(MouseButton::Right);
      strcpy(lastAction,"RIGHT click");
      beepClick(2);
    } else {
      bleMouse.doubleClick(MouseButton::Left);
      strcpy(lastAction,"DBL click");
      beepClick(3);
    }
    selTapCount = 0;
  }
}

// ======================================================
// UPDATE — SNAKE LOGIC  (unchanged)
// ======================================================
void updateSnake(){
  if(snakeState!=SNAKE_PLAYING) return;
  if(millis()-lastMoveTime<moveInterval) return;
  lastMoveTime=millis();
  dirX=nextDirX; dirY=nextDirY;
  for(uint8_t i=snakeLen-1;i>0;i--){snakeX[i]=snakeX[i-1];snakeY[i]=snakeY[i-1];}
  snakeX[0]+=dirX; snakeY[0]+=dirY;
  if(snakeX[0]<0||snakeX[0]>=GRID_W||snakeY[0]<0||snakeY[0]>=GRID_H){
    if(snakeScore>highScore) highScore=snakeScore;
    snakeState=SNAKE_OVER; beepDie(); return;
  }
  for(uint8_t i=1;i<snakeLen;i++){
    if(snakeX[0]==snakeX[i]&&snakeY[0]==snakeY[i]){
      if(snakeScore>highScore) highScore=snakeScore;
      snakeState=SNAKE_OVER; beepDie(); return;
    }
  }
  if(snakeX[0]==foodX&&snakeY[0]==foodY){
    if(snakeLen<MAX_LEN) snakeLen++;
    snakeScore++; snakeSpawnFood(); beepEat();
    moveInterval=max(50,130-(int)(snakeScore/5)*10);
  }
}

// ======================================================
// INPUT HANDLERS  (unchanged except menu now has 4 items)
// ======================================================
void handleMenuInput(){
  if(btnPressed(IDX_LEFT))  menuSel=(menuSel-1+MENU_COUNT)%MENU_COUNT;
  if(btnPressed(IDX_RIGHT)) menuSel=(menuSel+1)%MENU_COUNT;
  if(btnPressed(IDX_UP))    menuSel=(menuSel-1+MENU_COUNT)%MENU_COUNT;
  if(btnPressed(IDX_DOWN))  menuSel=(menuSel+1)%MENU_COUNT;
  if(btnPressed(IDX_SEL)){
    if     (menuSel==0){ appState=STATE_MUSIC; }
    else if(menuSel==1){ snakeReset(); appState=STATE_SNAKE; }
    else if(menuSel==2){ appState=STATE_WIFI; startWifiScan(); }
    else if(menuSel==3){
      mouseMode=MOUSE_MOVE; cursorVX=64; cursorVY=32;
      selTapCount=0; dragging=false;
      strcpy(lastAction,"---");
      if(!bleMouse.isPaired()) bleMouse.begin();
      appState=STATE_MOUSE;
    }
    else if(menuSel==4){ 
      inDeauthSubmenu = true; 
      deauthMode = DEAUTH_SCANNER;
      appState = STATE_DEAUTH; 
    }
    else if(menuSel==5){ startIR(); appState=STATE_IR; }
        else if(menuSel==6){ 
      if (sdMounted) listSDFiles();
      appState = STATE_SD; 
    }
  }
}

void handleMusicInput(){
  if(isPlaying){
    if(btnPressed(IDX_SEL)) stopMusic();
    if(btnPressed(IDX_LEFT)){stopMusic();appState=STATE_MENU;}
  } else {
    if(btnPressed(IDX_UP))   currentSong=(currentSong-1+totalSongs)%totalSongs;
    if(btnPressed(IDX_DOWN)) currentSong=(currentSong+1)%totalSongs;
    if(btnPressed(IDX_SEL))  playSong(currentSong);
    if(btnPressed(IDX_LEFT)){stopMusic();appState=STATE_MENU;}
  }
}

void handleSnakeInput(){
  if(snakeState==SNAKE_OVER){
    if(btnPressed(IDX_SEL)) snakeReset();
    if(btnPressed(IDX_LEFT)){noTone(BUZZER);appState=STATE_MENU;}
    return;
  }
  if(snakeState==SNAKE_PAUSED){
    if(btnPressed(IDX_SEL)) snakeState=SNAKE_PLAYING;
    if(btnPressed(IDX_LEFT)){noTone(BUZZER);appState=STATE_MENU;}
    return;
  }
  if(btnPressed(IDX_UP)   &&dirY!= 1){nextDirX= 0;nextDirY=-1;}
  if(btnPressed(IDX_DOWN) &&dirY!=-1){nextDirX= 0;nextDirY= 1;}
  if(btnPressed(IDX_LEFT) &&dirX!= 1){nextDirX=-1;nextDirY= 0;}
  if(btnPressed(IDX_RIGHT)&&dirX!=-1){nextDirX= 1;nextDirY= 0;}
  if(btnPressed(IDX_SEL)) snakeState=SNAKE_PAUSED;
}

void handleWifiInput(){
  if(wifiState==WIFI_SCANNING) return;
  if(wifiState==WIFI_RESULTS){
    if(btnPressed(IDX_UP))   {if(netScroll>0) netScroll--;}
    if(btnPressed(IDX_DOWN)) {if(netScroll<netCount-1) netScroll++;}
    if(btnPressed(IDX_SEL))  startWifiScan();
    if(btnPressed(IDX_LEFT)) {WiFi.mode(WIFI_OFF);appState=STATE_MENU;}
  }
}

// ======================================================
// DEAUTH DETECTOR STATE
// ======================================================
#include "esp_wifi.h"

// Ring buffer for detected attacker MACs
#define MAX_ATTACKERS 16
struct Attacker {
  uint8_t mac[6];
  uint16_t count;
  unsigned long lastSeen;
};

volatile uint16_t deauthPerSec    = 0;
volatile uint16_t deauthLastSec   = 0;
volatile uint32_t deauthTotal     = 0;
volatile bool     deauthAlert     = 0;
uint8_t  deauthChannel   = 1;
unsigned long deauthSecTimer   = 0;
unsigned long deauthAlertUntil = 0;
bool deauthRunning = false;
uint8_t  attackerCount = 0;
Attacker attackers[MAX_ATTACKERS];
int8_t   attackerScroll = 0;
#define DEAUTH_ALERT_THRESHOLD 3

// ── attack duration timer ────────────────────────────
bool          attackOngoing      = false;   // currently under attack?
unsigned long attackStartMs      = 0;       // when current attack began
unsigned long attackDurationMs   = 0;       // duration of current/last attack
// log of last 4 completed attack sessions
#define MAX_ATTACK_LOG 4
struct AttackLog {
  unsigned long durationMs;   // how long it lasted
  uint16_t      peakRate;     // peak deauths/sec during attack
  uint32_t      totalFrames;  // total frames in that attack
};
// ====================== DEAUTH TRANSMITTER ======================
#define MAX_TX_ATTACKERS 10
#define CHANNEL_HOP_INTERVAL 1500

struct AttackerNode {
  uint8_t mac[6];
  uint32_t last_seen;
  uint16_t detection_count;
};

AttackerNode txTargets[MAX_TX_ATTACKERS];
int txAttackerCount = 0;
uint8_t txCurrentChannel = 1;
unsigned long lastChannelHop = 0;

uint8_t deauth_packet[26] = {
  0xC0, 0x00, 0x00, 0x00,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x00,0x11,0x22,0x33,0x44,0x55,
  0x00,0x11,0x22,0x33,0x44,0x55,
  0x00,0x00, 0x07, 0x00
};

int findTxAttacker(uint8_t* mac) {
  for (int i = 0; i < txAttackerCount; i++) {
    if (memcmp(txTargets[i].mac, mac, 6) == 0) return i;
  }
  return -1;
}

void IRAM_ATTR txSniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t *payload = pkt->payload;
  if (payload[0] == 0xC0) {
    uint8_t* culprit = &payload[10];
    int idx = findTxAttacker(culprit);
    if (idx == -1 && txAttackerCount < MAX_TX_ATTACKERS) {
      memcpy(txTargets[txAttackerCount].mac, culprit, 6);
      txTargets[txAttackerCount].last_seen = millis();
      txTargets[txAttackerCount].detection_count = 1;
      txAttackerCount++;
    } else if (idx >= 0) {
      txTargets[idx].last_seen = millis();
      txTargets[idx].detection_count++;
    }
  }
}
AttackLog attackLog[MAX_ATTACK_LOG];
uint8_t   attackLogCount = 0;
uint16_t  attackPeakRate = 0;   // peak /s in current attack
uint32_t  attackFrameCount = 0; // frames in current attack
bool      showLog = false;      // toggle between attacker list and attack log

// MAC helpers
bool macEqual(const uint8_t* a, const uint8_t* b){
  for(uint8_t i=0;i<6;i++) if(a[i]!=b[i]) return false;
  return true;
}
bool macIsZero(const uint8_t* m){
  for(uint8_t i=0;i<6;i++) if(m[i]) return false;
  return true;
}

// Promiscuous callback — fires in WiFi task context
void IRAM_ATTR deauthSniffer(void* buf, wifi_promiscuous_pkt_type_t type){
  if(type != WIFI_PKT_MGMT) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* payload = pkt->payload;
  // 802.11 frame control bytes 0-1
  uint16_t fc = payload[0] | (payload[1]<<8);
  uint8_t ftype  = (fc >> 2)  & 0x03;   // bits 3:2
  uint8_t fstype = (fc >> 4)  & 0x0F;   // bits 7:4
  // type=0 (management), subtype=0xC (deauth) or 0xA (disassoc)
  if(ftype == 0 && (fstype == 0xC || fstype == 0xA)){
    deauthPerSec++;
    deauthTotal++;
    // source MAC is at bytes 10-15
    uint8_t* srcMac = payload + 10;
    if(macIsZero(srcMac)) return;
    // find or add attacker
    for(uint8_t i=0;i<attackerCount;i++){
      if(macEqual(attackers[i].mac, srcMac)){
        attackers[i].count++;
        attackers[i].lastSeen = millis();
        return;
      }
    }
    if(attackerCount < MAX_ATTACKERS){
      memcpy(attackers[attackerCount].mac, srcMac, 6);
      attackers[attackerCount].count    = 1;
      attackers[attackerCount].lastSeen = millis();
      attackerCount++;
    }
  }
}

void startDeauthScanner() {
  WiFi.mode(WIFI_OFF); delay(100);
  WiFi.mode(WIFI_STA);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&deauthSniffer);
  esp_wifi_set_channel(deauthChannel, WIFI_SECOND_CHAN_NONE);
  deauthRunning = true;
  attackerCount = 0;
  attackLogCount = 0;
  beepScan();
}

void startDeauthTransmitter() {
  WiFi.mode(WIFI_OFF); delay(100);
  WiFi.mode(WIFI_STA);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&txSniffer);
  esp_wifi_set_channel(txCurrentChannel, WIFI_SECOND_CHAN_NONE);
  deauthRunning = true;
  txAttackerCount = 0;
  lastChannelHop = millis();
  beepScan();
}

void stopDeauth() {
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(nullptr);
  WiFi.mode(WIFI_OFF);
  deauthRunning = false;
}

void updateDeauth() {
  if (!deauthRunning) return;
  unsigned long now = millis();

  if (deauthMode == DEAUTH_SCANNER) {
    // === ORIGINAL SCANNER LOGIC ===
    if (now - deauthSecTimer >= 1000) {
      deauthSecTimer = now;
      deauthLastSec  = deauthPerSec;
      deauthPerSec   = 0;

      bool overThreshold = (deauthLastSec >= DEAUTH_ALERT_THRESHOLD);

      // attack start
      if (overThreshold && !attackOngoing) {
        attackOngoing    = true;
        attackStartMs    = now;
        attackPeakRate   = deauthLastSec;
        attackFrameCount = deauthLastSec;
        tone(BUZZER,2000,40); delay(60); tone(BUZZER,2400,40); noTone(BUZZER);
      }

      // attack ongoing
      if (overThreshold && attackOngoing) {
        attackDurationMs = now - attackStartMs;
        attackFrameCount += deauthLastSec;
        if (deauthLastSec > attackPeakRate) attackPeakRate = deauthLastSec;
        deauthAlert      = true;
        deauthAlertUntil = now + 2000;
      }

      // attack ended
      if (!overThreshold && attackOngoing) {
        attackOngoing    = false;
        attackDurationMs = now - attackStartMs;
        if (attackLogCount < MAX_ATTACK_LOG) {
          attackLog[attackLogCount] = { attackDurationMs, attackPeakRate, attackFrameCount };
          attackLogCount++;
        } else {
          for (uint8_t i = 0; i < MAX_ATTACK_LOG - 1; i++) attackLog[i] = attackLog[i + 1];
          attackLog[MAX_ATTACK_LOG - 1] = { attackDurationMs, attackPeakRate, attackFrameCount };
        }
        attackPeakRate   = 0;
        attackFrameCount = 0;
        tone(BUZZER,800,80); noTone(BUZZER);
      }

      if (now > deauthAlertUntil) deauthAlert = false;
    }
  } else { 
    // TRANSMITTER MODE
    if (now - lastChannelHop >= CHANNEL_HOP_INTERVAL) {
      txCurrentChannel = (txCurrentChannel % 11) + 1;
      esp_wifi_set_channel(txCurrentChannel, WIFI_SECOND_CHAN_NONE);
      lastChannelHop = now;
    }

    for (int i = 0; i < txAttackerCount; i++) {
      if (now - txTargets[i].last_seen > 30000) {
        for (int j = i; j < txAttackerCount - 1; j++) txTargets[j] = txTargets[j + 1];
        txAttackerCount--; i--; continue;
      }
      memcpy(&deauth_packet[4], txTargets[i].mac, 6);
      for (int b = 0; b < 5; b++) {
        esp_wifi_80211_tx(WIFI_IF_STA, deauth_packet, sizeof(deauth_packet), true);
        delay(2);
      }
    }
  }
}
// Format MAC to string  xx:xx:xx:xx:xx:xx  (18 chars + null)
void macToStr(const uint8_t* mac, char* out){
  sprintf(out,"%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void drawDeauthSubmenu() {
  u8g2.clearBuffer();
  u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(3,10,"DEAUTH MODE");
  u8g2.setDrawColor(1);

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(15, 32, deauthMode == DEAUTH_SCANNER ? "→ SCANNER" : "  SCANNER");
  u8g2.drawStr(15, 48, deauthMode == DEAUTH_TRANSMITTER ? "→ TRANSMITTER" : "  TRANSMITTER");

  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2,63,"UP/DN: select  SEL: start  L: back");
  u8g2.sendBuffer();
}
// ======================================================
// SD TOOLS
// ======================================================
void drawSDTools() {
  u8g2.clearBuffer();
  u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(3,10,"SD TOOLS");
  u8g2.setDrawColor(1);

  if (sdFileCount == 0) {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(20,35,"No files found");
  } else {
    for (uint8_t i = 0; i < 4 && i < sdFileCount; i++) {
      uint8_t idx = sdFileSel + i;
      if (idx >= sdFileCount) break;
      uint8_t y = 22 + i*11;
      if (i == 0) {
        u8g2.drawRBox(0, y-2, 128, 11, 2);
        u8g2.setDrawColor(0);
      }
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.drawStr(4, y, sdFiles[idx].c_str());
      u8g2.setDrawColor(1);
    }
  }

  u8g2.drawHLine(0,57,128);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2,63,"U/D:select SEL:delete R:Format L:Back");
  u8g2.sendBuffer();
}

void handleSDInput() {
  if (btnPressed(IDX_LEFT)) {
    appState = STATE_MENU;
    return;
  }
  if (btnPressed(IDX_UP) && sdFileSel > 0) sdFileSel--;
  if (btnPressed(IDX_DOWN) && sdFileSel < sdFileCount-1) sdFileSel++;

  if (btnPressed(IDX_SEL) && sdFileCount > 0) {
    deleteSelectedFile();
  }
  if (btnPressed(IDX_RIGHT)) {
    if (formatSDCard()) {
      listSDFiles();
    }
  }
}
// ======================================================
// DRAW — DEAUTH (Scanner)
// ======================================================
void drawDeauth(){
  u8g2.clearBuffer();

  u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(3,10,"DEAUTH DETECTOR");
  u8g2.setDrawColor(1);

  // Alert / stats (original logic)
  if(deauthAlert && (millis()/250)%2==0){
    u8g2.drawRBox(0,14,128,12,2);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_5x7_tr);
    char abuf[24];
    unsigned long liveSec = attackOngoing ? (millis()-attackStartMs)/1000 : attackDurationMs/1000;
    sprintf(abuf,"ATTACK! %lus pk:%u/s", liveSec, attackPeakRate);
    u8g2.drawStr(2,23,abuf);
    u8g2.setDrawColor(1);
  } else {
    u8g2.setFont(u8g2_font_5x7_tr);
    char buf[32];
    if(attackOngoing){
      unsigned long liveSec=(millis()-attackStartMs)/1000;
      sprintf(buf,"CH:%u /s:%u ATK:%lus",deauthChannel,deauthLastSec,liveSec);
    } else {
      sprintf(buf,"CH:%u /s:%u TOT:%lu",deauthChannel,deauthLastSec,deauthTotal);
    }
    u8g2.drawStr(2,23,buf);
  }

  u8g2.drawHLine(0,25,128);

  if(!showLog){
    const uint8_t ROWS=3;
    if(attackerCount==0){
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.drawStr(14,40,"No attackers yet");
      u8g2.drawStr(20,52,"Monitoring...");
    } else {
      for(uint8_t i=0;i<ROWS;i++){
        int8_t idx=attackerScroll+i;
        if(idx>=attackerCount) break;
        uint8_t y=34+i*11;
        if(i==0){u8g2.drawRBox(0,y-8,119,10,2);u8g2.setDrawColor(0);}
        char macStr[20]; macToStr(attackers[idx].mac,macStr);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(2,y,macStr);
        char cbuf[6]; sprintf(cbuf,"x%u",attackers[idx].count);
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(106,y,cbuf);
        u8g2.setDrawColor(1);
      }
      if(attackerCount>ROWS){
        for(int i=0;i<attackerCount;i++){
          if(i==attackerScroll) u8g2.drawDisc (125,27+i*4,1);
          else                   u8g2.drawPixel(125,27+i*4);
        }
      }
    }
    u8g2.drawHLine(0,57,128);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(2,63,"U/D:scroll R:log L/R:ch LL:menu");
  } else {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(2,32,"# Duration  Peak  Frames");
    if(attackLogCount==0){
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.drawStr(14,46,"No attacks logged");
    } else {
      for(uint8_t i=0;i<attackLogCount && i<4;i++){
        uint8_t y=40+i*8;
        char lbuf[28];
        sprintf(lbuf,"%u %4lus  %3u/s  %5lu", i+1, attackLog[i].durationMs/1000, attackLog[i].peakRate, attackLog[i].totalFrames);
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(2,y,lbuf);
      }
    }
    u8g2.drawHLine(0,57,128);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(2,63,"R:attackers  SEL:clear log LL:menu");
  }
  u8g2.sendBuffer();
}

// ======================================================
// DRAW — DEAUTH TRANSMITTER (NEW)
// ======================================================
void drawDeauthTransmitter() {
  u8g2.clearBuffer();

  u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(3,10,"DEAUTH TRANSMITTER");
  u8g2.setDrawColor(1);

  // Status
  char buf[32];
  sprintf(buf,"CH:%d  Targets:%d", txCurrentChannel, txAttackerCount);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(4,28,buf);

  if(txAttackerCount == 0){
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(12,45,"Waiting for attackers...");
    u8g2.drawStr(20,55,"Hopping channels");
  } else {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(8,45,"ATTACKING DETECTED MACs!");
    
    // Show first few targets
    for(int i=0; i<min(3, txAttackerCount); i++){
      char macStr[20];
      macToStr(txTargets[i].mac, macStr);
      u8g2.setFont(u8g2_font_4x6_tr);
      u8g2.drawStr(4, 56 + i*8, macStr);
    }
  }

  u8g2.drawHLine(0,57,128);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2,63,"LONG-L:menu  R:next ch");
  u8g2.sendBuffer();
}
void handleDeauthInput(){
  if (inDeauthSubmenu) {
    if (btnPressed(IDX_UP))    deauthMode = DEAUTH_SCANNER;
    if (btnPressed(IDX_DOWN))  deauthMode = DEAUTH_TRANSMITTER;
    if (btnPressed(IDX_RIGHT)) deauthMode = DEAUTH_TRANSMITTER;

    if (btnPressed(IDX_SEL)) {
      inDeauthSubmenu = false;
      if (deauthMode == DEAUTH_SCANNER) startDeauthScanner();
      else startDeauthTransmitter();
    }
    // LEFT = back to menu — no conflict now since LEFT not used above
    if (btnPressed(IDX_LEFT)) {
      appState = STATE_MENU;
      inDeauthSubmenu = false;
    }
  } else {
    // Original scanner controls
    static unsigned long dLeftStart=0; 
    static bool dLeftFired=false;

    if(btnHeld(IDX_LEFT)){
      if(dLeftStart==0) dLeftStart=millis();
      if(!dLeftFired && millis()-dLeftStart>800){
        dLeftFired=true; 
        stopDeauth(); 
        appState=STATE_MENU;
        dLeftStart=0; 
        return;
      }
    } else {
      if(!dLeftFired && dLeftStart!=0){
        if(showLog){ showLog=false; }
        else if(attackLogCount>0){ showLog=true; }
        else if(deauthChannel>1){ 
          deauthChannel--; 
          esp_wifi_set_channel(deauthChannel,WIFI_SECOND_CHAN_NONE); 
        }
      }
      dLeftStart=0; 
      dLeftFired=false;
    }

    if(btnPressed(IDX_RIGHT)){
      if(showLog){ showLog=false; }
      else if(deauthChannel<13){ 
        deauthChannel++; 
        esp_wifi_set_channel(deauthChannel,WIFI_SECOND_CHAN_NONE); 
      }
    }

    if(!showLog){
      if(btnPressed(IDX_UP))   { if(attackerScroll>0) attackerScroll--; }
      if(btnPressed(IDX_DOWN)) { if(attackerScroll<attackerCount-1) attackerScroll++; }
    }

    if(btnPressed(IDX_SEL)){
      if(showLog){ attackLogCount=0; }
      else       { 
        attackerCount=0; 
        attackerScroll=0; 
        deauthTotal=0; 
      }
    }
  }
}
// ======================================================
// SD CARD FUNCTIONS
// ======================================================
void initSD() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS)) {
    sdMounted = true;
    Serial.println("SD Card Mounted Successfully");
  } else {
    sdMounted = false;
    Serial.println("SD Card Mount Failed");
  }
}

bool formatSDCard() {
  if (!sdMounted) return false;
  return SD.format();
}

void listSDFiles() {
  sdFileCount = 0;
  File root = SD.open("/");
  File file = root.openNextFile();
  while (file && sdFileCount < 20) {
    sdFiles[sdFileCount++] = String("/") + file.name();
    file = root.openNextFile();
  }
}

void deleteSelectedFile() {
  if (sdFileCount == 0) return;
  if (SD.remove(sdFiles[sdFileSel])) {
    Serial.println("File deleted");
    listSDFiles(); // refresh list
  }
}

// Auto-save IR Codes to SD
void saveIRToSD() {
  if (!sdMounted) return;
  File file = SD.open("/ircodes.txt", FILE_WRITE);
  if (file) {
    for (int i = 0; i < IR_SLOTS; i++) {
      if (irCodes[i].hasCode) {
        file.print(irCodes[i].label);
        file.print("=");
        if (irCodes[i].isRaw) {
          file.println("RAW");
        } else {
          file.println(irCodes[i].code, HEX);
        }
      }
    }
    file.close();
    Serial.println("IR Codes auto-saved to SD");
  }
}
// ======================================================
// IR REMOTE STATE
// ======================================================
IRrecv irRecv(IR_RX_PIN);
IRsend irSend(IR_TX_PIN);
decode_results irResults;

enum IrScreen { IR_HOME, IR_LEARN };
IrScreen irScreen = IR_HOME;

#define IR_SLOTS 8
#define IR_RAW_BUF 300

struct IrCode {
  char          label[12];
  uint64_t      code;
  decode_type_t protocol;
  uint16_t      bits;
  bool          hasCode;
  bool          isRaw;
  uint16_t      rawBuf[IR_RAW_BUF];
  uint16_t      rawLen;
};

IrCode irCodes[IR_SLOTS] = {
  {"TV Power", 0, UNKNOWN, 32, false, false, {}, 0},
  {"Vol +",    0, UNKNOWN, 32, false, false, {}, 0},
  {"Vol -",    0, UNKNOWN, 32, false, false, {}, 0},
  {"Mute",     0, UNKNOWN, 32, false, false, {}, 0},
  {"Ch +",     0, UNKNOWN, 32, false, false, {}, 0},
  {"Ch -",     0, UNKNOWN, 32, false, false, {}, 0},
  {"Input",    0, UNKNOWN, 32, false, false, {}, 0},
  {"Custom",   0, UNKNOWN, 32, false, false, {}, 0}
};

int8_t  irSel = 0;
bool    irLearning = false;
unsigned long irLearnStart = 0;
#define IR_LEARN_TIMEOUT 10000

char irStatus[24] = "Ready";

void beepIR() { 
  tone(BUZZER, 1800, 40); delay(50); 
  tone(BUZZER, 2200, 40); delay(50); 
  noTone(BUZZER); 
}

void startIR(){
  irRecv.enableIRIn();
  irSend.begin();
  irScreen = IR_HOME;
  irSel = 0;
  irLearning = false;
  strcpy(irStatus, "Pick slot & press SEL");
}

void updateIR(){
  if (!irLearning) return;

  if (millis() - irLearnStart > IR_LEARN_TIMEOUT) {
    irLearning = false;
    strcpy(irStatus, "Timeout - try again");
    irScreen = IR_HOME;
    return;
  }

  if (irRecv.decode(&irResults)) {
    if (irResults.value == 0xFFFFFFFF) {
      irRecv.resume(); return;
    }

    irCodes[irSel].protocol = irResults.decode_type;
    irCodes[irSel].bits     = irResults.bits;

    if (irResults.decode_type != UNKNOWN && irResults.decode_type != RAW) {
      irCodes[irSel].code    = irResults.value;
      irCodes[irSel].isRaw   = false;
      irCodes[irSel].rawLen  = 0;
      irCodes[irSel].hasCode = true;
      snprintf(irStatus, 24, "Saved: %s", irCodes[irSel].label);
    } else {
      // Raw capture - better timing
      uint16_t len = irResults.rawlen - 1;
      if (len > IR_RAW_BUF) len = IR_RAW_BUF;
      for (uint16_t i = 0; i < len; i++) {
        irCodes[irSel].rawBuf[i] = irResults.rawbuf[i+1] * USECPERTICK;   // Proper constant
      }
      irCodes[irSel].rawLen  = len;
      irCodes[irSel].isRaw   = true;
      irCodes[irSel].hasCode = true;
      snprintf(irStatus, 24, "Saved RAW: %s", irCodes[irSel].label);
    }

    irLearning = false;
    irScreen = IR_HOME;
    beepIR(); beepIR();
    irRecv.resume();
  }
}
void drawIR(){
  u8g2.clearBuffer();

  // header
  u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(3,10,"IR REMOTE");
  u8g2.setDrawColor(1);

  if(irScreen==IR_LEARN){
    // learning screen
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(18,28,"Learning...");
    u8g2.setFont(u8g2_font_5x7_tr);
    char lbuf[18]; snprintf(lbuf,18,"Slot: %s",irCodes[irSel].label);
    u8g2.drawStr(14,40,lbuf);
    // progress bar (timeout based)
    unsigned long elapsed=millis()-irLearnStart;
    uint8_t prog=(uint8_t)(elapsed*120/IR_LEARN_TIMEOUT);
    u8g2.drawFrame(4,46,120,6);
    u8g2.drawBox(4,46,prog,6);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(10,62,"Point remote & press button");
    u8g2.sendBuffer(); return;
  }

  // home — show 4 slots, scroll
  const uint8_t ROWS=4;
  int8_t start=(irSel/ROWS)*ROWS;
  for(uint8_t i=0;i<ROWS;i++){
    uint8_t idx=start+i;
    if(idx>=IR_SLOTS) break;
    uint8_t y=15+i*12;
    if(idx==irSel){
      u8g2.drawRBox(0,y-8,120,11,2);
      u8g2.setDrawColor(0);
    }
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(3,y,irCodes[idx].label);
    // code present indicator
    if(irCodes[idx].hasCode){
      u8g2.setFont(u8g2_font_4x6_tr);
      char pbuf[10];
      if(irCodes[idx].isRaw){
        strcpy(pbuf,"RAW");
      } else {
        snprintf(pbuf,10,"%s",typeToString(irCodes[idx].protocol,false).c_str());
        pbuf[7]='\0';
      }
      u8g2.drawStr(62,y,pbuf);
      u8g2.drawStr(106,y,"[OK]");
    } else {
      u8g2.setFont(u8g2_font_4x6_tr);
      u8g2.drawStr(62,y,"--empty--");
    }
    u8g2.setDrawColor(1);
  }

  // status line
  u8g2.drawHLine(0,57,128);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2,63,irStatus);

  // scroll dots right
  for(uint8_t i=0;i<IR_SLOTS;i++){
    if(i==irSel) u8g2.drawDisc  (125,15+i*5,1);
    else          u8g2.drawPixel (125,15+i*5);
  }

  u8g2.sendBuffer();
}
void handleIRInput(){
  static unsigned long irLeftStart = 0;
  static bool irLeftFired = false;

  if (btnHeld(IDX_LEFT)) {
    if (irLeftStart == 0) irLeftStart = millis();
    if (!irLeftFired && millis() - irLeftStart > 800) {
      irLeftFired = true;
      stopIR();
      appState = STATE_MENU;
      return;
    }
  } else {
    irLeftStart = 0;
    irLeftFired = false;
  }

  if (irScreen == IR_LEARN) return;

  if (btnPressed(IDX_UP))    { if (irSel > 0) irSel--; }
  if (btnPressed(IDX_DOWN))  { if (irSel < IR_SLOTS-1) irSel++; }

  if (btnPressed(IDX_SEL)) {
    if (irCodes[irSel].hasCode) {
      if (irCodes[irSel].isRaw) {
        irSend.sendRaw(irCodes[irSel].rawBuf, irCodes[irSel].rawLen, 38);
      } else {
        irSend.send(irCodes[irSel].protocol, irCodes[irSel].code, irCodes[irSel].bits);
      }
      snprintf(irStatus, 24, "Sent: %s", irCodes[irSel].label);
      beepIR();
    } else {
      irLearning = true;
      irLearnStart = millis();
      irScreen = IR_LEARN;
      irRecv.resume();
      strcpy(irStatus, "Learning...");
    }
  }

  if (btnPressed(IDX_RIGHT)) {
    irLearning = true;
    irLearnStart = millis();
    irScreen = IR_LEARN;
    irRecv.resume();
    snprintf(irStatus, 24, "Re-learning %s", irCodes[irSel].label);
  }
}

void stopIR(){
  irRecv.disableIRIn();
}
// ======================================================
// SETUP & LOOP
// ======================================================
void setup(){
  for(uint8_t i=0;i<5;i++) pinMode(btnPins[i],INPUT_PULLUP);
  pinMode(BUZZER,OUTPUT);
  Wire.begin();
  u8g2.begin();
  irSend.begin();
  initSD();
  if (sdMounted) listSDFiles();
  u8g2.setContrast(200);
  randomSeed(esp_random());
  drawBoot();
  appState=STATE_MENU;
}

void loop(){
  switch(appState){
    case STATE_MENU:
      handleMenuInput();
      drawMenu();
      break;
    case STATE_MUSIC:
      handleMusicInput();
      updateRTTTL();
      drawMusic();
      break;
    case STATE_SNAKE:
      handleSnakeInput();
      updateSnake();
      drawSnake();
      break;
    case STATE_WIFI:
      handleWifiInput();
      if     (wifiState==WIFI_SCANNING) { updateWifiScan(); drawWifiScanning(); }
      else if(wifiState==WIFI_RESULTS)  { drawWifiResults(); }
      break;
    case STATE_MOUSE:
      handleMouseInput();
      drawMouse();
      break;
        case STATE_DEAUTH:
      handleDeauthInput();
      if (inDeauthSubmenu) {
        drawDeauthSubmenu();
      } else {
        updateDeauth();
        if (deauthMode == DEAUTH_SCANNER) {
          drawDeauth();
        } else {
          drawDeauthTransmitter();
        }
      }
      break;
    case STATE_IR:
      handleIRInput();
      updateIR();
      drawIR();
      break;
    case STATE_SD:
      handleSDInput();
      drawSDTools();
      break;
  }
  delay(5);
}
