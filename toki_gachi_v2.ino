#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <math.h>

// ====================== PINS ======================
#define BTN_UP     4
#define BTN_DOWN   5
#define BTN_SELECT 10
#define BUZZER     3

// ====================== OLED ======================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ====================== SONGS ======================
const char* songs[] = {
  "Mario", "Tetris", "Zelda", "Imperial", "Pacman", "Jingle Bells", "FF Prelude","Megalovania", "Bonetrousle", "Death By Glamour"
};

uint8_t  animFrame = 0;
uint32_t animTimer = 0;

// Full-length RTTTL strings
const char* rtttlSongs[] = {

  // Super Mario Bros - full opening theme
  "Mario:d=4,o=5,b=200:"
  "e,e,p,e,p,c,e,g,p,p,g4,p,p,"
  "c,p,p,g4,p,p,e4,p,p,a4,p,b4,p,a#4,a4,p,"
  "g4,e,g,a,p,f,g,p,e,p,c,d,b4,p,"
  "c,p,p,g4,p,p,e4,p,p,a4,p,b4,p,a#4,a4,p,"
  "g4,e,g,a,p,f,g,p,e,p,c,d,b4",

  // Tetris - Korobeiniki complete A+B section
  "Tetris:d=4,o=5,b=144:"
  "e,b4,c,d,c,b4,a4,a4,c,e,d,c,b4,b4,c,d,e,c,a4,a4,p,"
  "d,f,a,g,f,e,c,e,d,c,b4,b4,c,d,e,c,a4,a4,p,"
  "e,b4,c,d,c,b4,a4,a4,c,e,d,c,b4,b4,c,d,e,c,a4,a4,p,"
  "d,f,a,g,f,e,c,e,d,c,b4,b4,c,d,e,c,a4",

  // Zelda - Song of Time
  "Zelda:d=4,o=5,b=120:"
  "a4,d,f,a4,d,f,a4,d,f,g,f,e,f,p,"
  "a4,d,f,a4,d,f,a4,d,f,e,c#,d,p,"
  "f,a,c6,b,a,f,a,p,"
  "f,g,a,a#,a,g,f,e,d,p,"
  "a4,d,f,a4,d,f,g,f,e,f",

  // Imperial March - complete opening
  "Imperial:d=4,o=5,b=110:"
  "a4,a4,a4,f4,8c,a4,f4,8c,a4,p,"
  "e,e,e,f,8c,g#4,f4,8c,a4,p,"
  "a,2a,a,2a#,8a,8a,a#,8a,8a#,g,8f,8g,a,p,"
  "a,2a,f4,8f4,f4,g,8e,8f,g,p,"
  "8a4,8a4,a4,8a#4,8a#4,a#4,a4,g#4,8g4,8f#4,g4",

  // Pac-Man theme - opening jingle
  "Pacman:d=4,o=5,b=140:"
  "b4,b5,f#5,d#5,b5,f#5,d#5,c5,c6,g5,e5,c6,g5,e5,"
  "b4,b5,f#5,d#5,b5,f#5,d#5,d#5,e5,f5,f5,f#5,g5,g5,"
  "g#5,a5,b5,p,a5,p,g#5,p,"
  "b4,b5,f#5,d#5,b5,f#5,d#5,c5,c6,g5,e5,c6,g5,e5",

  // Jingle Bells - two full verses
  "Jingle:d=4,o=5,b=180:"
  "e,e,e,p,e,e,e,p,e,g,c,8d,e,p,"
  "f,f,f,8f,f,e,e,8e,8e,e,d,d,e,d,p,g,p,"
  "e,e,e,p,e,e,e,p,e,g,c,8d,e,p,"
  "f,f,f,8f,f,e,e,8e,8e,g,g,f,d,c,p,"
  "g,e,d,c,g4,p,g4,g4,g4,p,"
  "g,e,d,c,a4,p,a4,a4,a4,p,"
  "a4,f,e,d,b4,p,b4,b4,b4",

  // Final Fantasy I - Main Theme (Prelude + opening)
  "FF1:d=8,o=5,b=140:"
  "c,e,g,c6,e6,g,c6,e6,"
  "c,e,g,c6,e6,g,c6,e6,"
  "c,f,a,c6,f6,a,c6,f6,"
  "c,f,a,c6,f6,a,c6,f6,"
  "b4,d,g,b,d6,g,b,d6,"
  "b4,d,g,b,d6,g,b,d6,"
  "c,e,g,c6,e6,g,c6,e6,"
  "c,e,g,c6,e6,g,c6,e6,"
  "4c6,4g,4e,4c",

  // ==================== UNDERTALE SONGS ====================

  // Megalovania (Sans Theme) - Full Length Complete Arrangement
  "Megalovania:d=16,o=5,b=180:"
  "d,d,d6,8p,a,8p,g#,g,f,8d,f,g,c,c,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "b4,b4,d6,8p,a,8p,g#,g,f,8d,f,g,a#4,a#4,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "d,d,d6,8p,a,8p,g#,g,f,8d,f,g,c,c,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "b4,b4,d6,8p,a,8p,g#,g,f,8d,f,g,a#4,a#4,d6,8p,a,8p,g#,g,f,8d,f,g," 
  "f6,f6,f6,8f6,e6,8e6,d6,8d6,c6,8p,d6,8d6,f6,8g6,g#6,g6,f6,d6,f6,g6,"
  "f6,f6,f6,8f6,g6,8g#6,a6,8a6,g6,8f6,d6,8d6,f6,8g6,a6,8a6,c7,8a6,d7,8p," 
  "d,d,d6,8p,a,8p,g#,g,f,8d,f,g,c,c,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "b4,b4,d6,8p,a,8p,g#,g,f,8d,f,g,a#4,a#4,d6,8p,a,8p,g#,g,f,8d,f,g,"
  "f5,8f5,e5,8e5,d5,8d5,c5,8p,d5,8d5,f5,8g5,g#5,g5,f5,d5,f5,g5,"
  "f5,8f5,g5,8g#5,a5,8a5,g5,8f5,d5,8d5,f5,8g5,a5,8a5,c6,8a5,d6,8p,"
  "8d6,8d6,8d6,8d6,8c6,8c6,8c6,8c6,8b5,8b5,8b5,8b5,8a#5,8a#5,8a#5,8a#5,"
  "d6,e6,f6,g6,a6,a#6,c7,d7,e7,f7,8p,d7,8p,c7,8p,a6,8p,g6,8p,f6,8p,d6",

  // Bonetrousle (Papyrus Theme) - Complete Verse and Escalating Pitch Run
  "Bonetrousle:d=8,o=5,b=150:"
  "f,f,f,f,f,p,g,g,g,g,g,p,a,a,a,a,a,p,a#,a#,a#,a#,a#,p,"
  "c6,c6,c6,c6,c6,p,d6,d6,d6,d6,d6,p,e6,f6,e6,d6,c6,p,"
  "c6,d6,e6,f6,g6,a6,a#6,p,a6,g6,f6,e6,d6,c6,b,a,"
  "f,f,f,f,f,p,g,g,g,g,g,p,a,a,a,a,a,p,a#,a#,a#,a#,a#,p,"
  "c6,p,d6,p,e6,p,f6,p,g6,4p,a6,g6,f6,e6,4d6",

  // Death By Glamour (Mettaton Theme) - Full Main Intro and Extended Chorus Hooks
  "DeathByGlam:d=16,o=5,b=148:"
  "8c#6,8b,8g#,8e,8g#,8b,8c#6,8b,8g#,8e,8g#,8b,c#6,b,c#6,8p,"
  "8d6,8c6,8a,8f,8a,8c6,8d6,8c6,8a,8f,8a,8c6,d6,c6,d6,8p,"
  "c#6,8c#6,b,8a,g#,8g#,f#,8e,f#,8f#,g#,8a,b,8c#6,d6,8p,"
  "e6,8e6,d6,8c#6,b,8b,a,8g#,a,8a,b,8c#6,d6,8e6,f#6,8p,"
  "c#6,8c#6,b,8a,g#,8g#,f#,8e,f#,8f#,g#,8a,b,8c#6,d6,8p,"
  "8e6,8d#6,8d6,8c#6,8c6,8b5,8a#5,8a5,8g#5,8g5,8f#5,8f5,4e5"

};

const int totalSongs = 10;

// ====================== STATE ======================
int currentSong  = 0;
int playingIndex = -1;
bool isPlaying   = false;

// RTTTL engine state
const char* rtttlPtr  = nullptr;
int  defaultDur  = 4;
int  defaultOct  = 5;
int  bpm         = 63;
unsigned long wholenote   = 0;
unsigned long noteStart   = 0;
unsigned long noteDuration= 0;
bool notePlaying = false;

// Button debounce
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 180;

// ====================== FORWARD DECLS ======================
void drawBoot();
void drawMenu();
void drawNowPlaying();
void playSong(int index);
void stopMusic();
void updateRTTTL();
void handleButtons();

// ====================== SETUP ======================
void setup() {
  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  // ESP32 dynamic frequency configuration (Core 3.x API)
  // Assigns pin 3 at 2000Hz base speed with 8-bit accuracy resolution.
  ledcAttach(BUZZER, 2000, 8);

  Wire.begin(8, 9);
  u8g2.begin();
  u8g2.setContrast(200);
  drawBoot();
}

// ====================== LOOP ======================
void loop() {
  handleButtons();
  updateRTTTL();

  if (isPlaying) drawNowPlaying();
  else           drawMenu();

  delay(5);
}

// ====================== BUTTONS ======================
void handleButtons() {
  if (millis() - lastButtonTime < debounceDelay) return;

  bool up     = !digitalRead(BTN_UP);
  bool down   = !digitalRead(BTN_DOWN);
  bool select = !digitalRead(BTN_SELECT);

  if (up && !isPlaying) {
    currentSong = (currentSong - 1 + totalSongs) % totalSongs;
    lastButtonTime = millis();
  } else if (down && !isPlaying) {
    currentSong = (currentSong + 1) % totalSongs;
    lastButtonTime = millis();
  } else if (select) {
    lastButtonTime = millis();
    if (isPlaying) stopMusic();
    else           playSong(currentSong);
  }
}

// ====================== PLAY ======================
void playSong(int index) {
  if (index < 0 || index >= totalSongs) return;
  stopMusic();

  playingIndex = index;
  isPlaying    = true;
  rtttlPtr     = rtttlSongs[index];

  // Skip song name
  while (*rtttlPtr && *rtttlPtr != ':') rtttlPtr++;
  if (*rtttlPtr == ':') rtttlPtr++;

  // Parse defaults: d=N,o=N,b=N
  defaultDur = 4;
  defaultOct = 5;
  bpm        = 63;

  while (*rtttlPtr && *rtttlPtr != ':') {
    while (*rtttlPtr == ' ') rtttlPtr++;

    char key = *rtttlPtr;
    if (*(rtttlPtr + 1) == '=') {
      rtttlPtr += 2;               
      int val = 0;
      while (isdigit(*rtttlPtr)) { val = val * 10 + (*rtttlPtr - '0'); rtttlPtr++; }
      if      (key == 'd') defaultDur = val;
      else if (key == 'o') defaultOct = val;
      else if (key == 'b') bpm        = val;
    } else {
      rtttlPtr++; 
    }
    while (*rtttlPtr == ',' || *rtttlPtr == ' ') rtttlPtr++;
  }
  if (*rtttlPtr == ':') rtttlPtr++;

  wholenote    = (60000UL * 4) / bpm;
  notePlaying  = false;
}

// ====================== STOP ======================
void stopMusic() {
  ledcWriteTone(BUZZER, 0); // Stops the generation safely on ESP32 architecture
  isPlaying    = false;
  playingIndex = -1;
  rtttlPtr     = nullptr;
  notePlaying  = false;
}

// ====================== DISPLAY DRAW CHANNELS ======================
void drawBoot() {
  u8g2.clearBuffer();

  // outer border
  u8g2.drawRFrame(2, 2, 124, 60, 4);

  u8g2.setFont(u8g2_font_9x15B_tf);
  u8g2.drawStr(22, 28, "toki_gachi");

  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(22, 44, "^-^");
  u8g2.drawStr(22, 56, "v2  ESP32-C3");

  u8g2.sendBuffer();
  delay(1400);
}

void drawMenu() {
  u8g2.clearBuffer();

  // ── header ────────────────────────────────────────────
  u8g2.drawBox(0, 0, 128, 13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(3, 10, "_toki_gachi_");
  u8g2.setDrawColor(1);

  // ── song list: show prev / selected / next ─────────────
  for (int i = -1; i <= 1; i++) {
    int idx = (currentSong + i + totalSongs) % totalSongs;
    int y   = 28 + (i + 1) * 13;

    if (i == 0) {
      // selected row highlight
      u8g2.drawRBox(0, y - 10, 128, 13, 2);
      u8g2.setDrawColor(0);
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(18, y, songs[idx]);
      // play arrow
      u8g2.drawTriangle(4, y - 7,  4, y + 1,  12, y - 3);
      u8g2.setDrawColor(1);
    } else {
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.drawStr(18, y, songs[idx]);
    }
  }

  // ── scroll dots ───────────────────────────────────────
  for (int i = 0; i < totalSongs; i++) {
    uint8_t dx = 118;
    uint8_t dy = 15 + i * 5;
    if (i == currentSong) u8g2.drawDisc(dx, dy, 2);
    else                   u8g2.drawCircle(dx, dy, 2);
  }

  // ── footer hint ───────────────────────────────────────
  u8g2.drawHLine(0, 55, 128);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2, 63, "UP/DOWN: Nav    SEL: Play");

  u8g2.sendBuffer();
}

void drawNowPlaying() {
  u8g2.clearBuffer();

  // ── animated music note bar ────────────────────────────
  u8g2.drawBox(0, 0, 128, 13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(3, 10, "NOW PLAYING");
  u8g2.setDrawColor(1);

  // ── song name ─────────────────────────────────────────
  u8g2.setFont(u8g2_font_9x15B_tf);
  int nameW = u8g2.getStrWidth(songs[playingIndex]);
  int nameX = (128 - nameW) / 2;
  u8g2.drawStr(nameX, 36, songs[playingIndex]);

  // ── bouncing equaliser bars ────────────────────────────
  // 5 bars, heights driven by animFrame to create a waving effect
  const uint8_t barX[5] = {14, 30, 46, 62, 78};
  for (uint8_t b = 0; b < 5; b++) {
    // each bar gets a phase offset so they ripple
    uint8_t h = 3 + (uint8_t)(3.5f * fabsf(sinf((animFrame * 0.35f) + b * 0.9f)));
    u8g2.drawBox(barX[b], 50 - h, 8, h);
  }

  // ── stop hint ─────────────────────────────────────────
  u8g2.drawHLine(0, 56, 128);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(28, 63, "SEL = STOP");

  u8g2.sendBuffer();
}

// ====================== RTTTL ENGINE (Non-Blocking) ======================
void updateRTTTL() {
  if (!isPlaying || !rtttlPtr) return;

  if (notePlaying) {
    if (millis() - noteStart < noteDuration) return;
    notePlaying = false;
    ledcWriteTone(BUZZER, 0); // Clear between notes
    delay(8);
  }

  while (*rtttlPtr == ',' || *rtttlPtr == ' ' || *rtttlPtr == '\n') rtttlPtr++;

  if (*rtttlPtr == '\0') { stopMusic(); return; }

  int duration = defaultDur;
  int octave   = defaultOct;
  int note     = -1;   

  if (isdigit(*rtttlPtr)) {
    duration = 0;
    while (isdigit(*rtttlPtr)) { duration = duration * 10 + (*rtttlPtr - '0'); rtttlPtr++; }
  }

  switch (tolower(*rtttlPtr)) {
    case 'c': note =  0; break;
    case 'd': note =  2; break;
    case 'e': note =  4; break;
    case 'f': note =  5; break;
    case 'g': note =  7; break;
    case 'a': note =  9; break;
    case 'b': note = 11; break;
    case 'p': note = -1; break;
    default:
      rtttlPtr++;
      return;
  }
  rtttlPtr++;

  if (*rtttlPtr == '#') {
    note++;
    rtttlPtr++;
  }

  bool dotted = false;
  if (*rtttlPtr == '.') {
    dotted = true;
    rtttlPtr++;
  }

  if (isdigit(*rtttlPtr)) {
    octave = *rtttlPtr - '0';
    rtttlPtr++;
  }
  
  if (*rtttlPtr == '.') {
    dotted = true;
    rtttlPtr++;
  }

  noteDuration = wholenote / duration;
  if (dotted) noteDuration += noteDuration / 2;

  if (note == -1) {
    ledcWriteTone(BUZZER, 0);
  } else {
    int midiNote = 12 * (octave + 1) + note;
    float frequency = 440.0 * pow(2.0, (midiNote - 69) / 12.0);
    ledcWriteTone(BUZZER, (int)frequency); // ESP32 core specific square wave generation
  }

  noteStart = millis();
  notePlaying = true;
}
