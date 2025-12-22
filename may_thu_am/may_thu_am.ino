#include <Arduino.h>
#include <driver/i2s.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ================= PIN ================= */
#define I2S_WS       6
#define I2S_SCK      5
#define I2S_SD_MIC   4
#define I2S_DIN_SPK  7
#define I2S_PORT     I2S_NUM_0

#define SD_CS        8
#define SD_SCK       1
#define SD_MISO      3
#define SD_MOSI      2

#define BTN_REC      9
#define BTN_MODE     10
#define BTN_NEXT     21   // TX
#define BTN_DEL      20   // RX  ⭐ NÚT XÓA

/* ================= AUDIO ================= */
#define SAMPLE_RATE  16000
#define SAMPLE_BITS  16
#define BUFFER_LEN   512

/* ================= OLED ================= */
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

/* ================= STATE ================= */
enum SYS_MODE { MODE_RECORD, MODE_PLAY };
SYS_MODE sysMode = MODE_RECORD;

bool isRecording = false;

bool lastBtnRec  = HIGH;
bool lastBtnMode = HIGH;

bool nextLocked = false;
bool delLocked  = false;

unsigned long debounceRec  = 0;
unsigned long debounceMode = 0;
unsigned long debounceNext = 0;
unsigned long debounceDel  = 0;
const unsigned long debounceDelay = 200;

int recordIndex = 1;
int playIndex   = 1;
int totalFiles  = 0;

File audioFile;
int16_t i2sBuffer[BUFFER_LEN];

/* ================= UTIL ================= */
void makeFileName(char *buf, int idx) {
  sprintf(buf, "/rec%03d.wav", idx);
}

void showOLED(const char *l1, const char *l2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(l1);
  display.setCursor(0, 16);
  display.println(l2);
  display.display();
}

/* ================= WAV ================= */
void writeWavHeader(File &file) {
  byte header[44] = {0};
  header[0]='R'; header[1]='I'; header[2]='F'; header[3]='F';
  header[8]='W'; header[9]='A'; header[10]='V'; header[11]='E';
  header[12]='f'; header[13]='m'; header[14]='t'; header[15]=' ';
  uint32_t subChunk1 = 16;
  uint16_t format = 1, channels = 1, bits = SAMPLE_BITS;
  uint32_t sampleRate = SAMPLE_RATE;
  uint32_t byteRate = sampleRate * channels * bits / 8;
  uint16_t blockAlign = channels * bits / 8;
  memcpy(&header[16], &subChunk1, 4);
  memcpy(&header[20], &format, 2);
  memcpy(&header[22], &channels, 2);
  memcpy(&header[24], &sampleRate, 4);
  memcpy(&header[28], &byteRate, 4);
  memcpy(&header[32], &blockAlign, 2);
  memcpy(&header[34], &bits, 2);
  header[36]='d'; header[37]='a'; header[38]='t'; header[39]='a';
  file.write(header, 44);
}

void updateWavHeader(File &file) {
  uint32_t fileSize = file.size() - 8;
  uint32_t dataSize = file.size() - 44;
  file.seek(4);  file.write((byte*)&fileSize, 4);
  file.seek(40); file.write((byte*)&dataSize, 4);
  file.close();
}

/* ================= I2S ================= */
void setupI2S(bool mic) {
  i2s_driver_uninstall(I2S_PORT);

  i2s_config_t cfg = {
    .mode = I2S_MODE_MASTER,
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_LEN,
    .use_apll = false
  };

  i2s_pin_config_t pin = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = -1
  };

  if (mic) {
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    pin.data_in_num = I2S_SD_MIC;
  } else {
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    pin.data_out_num = I2S_DIN_SPK;
    cfg.tx_desc_auto_clear = true;
  }

  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin);
  i2s_zero_dma_buffer(I2S_PORT);
}

/* ================= PLAY ================= */
void playFile(const char *name) {
  setupI2S(false);
  File f = SD.open(name);
  if (!f) return;

  showOLED("PLAYING", name);
  f.seek(44);

  uint8_t buf[BUFFER_LEN * 2];
  size_t bw;

  while (f.available()) {
    int r = f.read(buf, sizeof(buf));
    i2s_write(I2S_PORT, buf, r, &bw, portMAX_DELAY);
  }

  f.close();
  delay(300);
  setupI2S(true);
}

/* ================= DELETE ================= */
void deleteCurrentFile() {
  if (totalFiles == 0) return;

  char fname[16];
  makeFileName(fname, playIndex);

  if (SD.exists(fname)) {
    SD.remove(fname);

    // dồn file phía sau lên
    for (int i = playIndex + 1; i <= totalFiles; i++) {
      char oldName[16], newName[16];
      makeFileName(oldName, i);
      makeFileName(newName, i - 1);
      SD.rename(oldName, newName);
    }

    totalFiles--;
    recordIndex = totalFiles + 1;

    if (playIndex > totalFiles) playIndex = 1;
    showOLED("DELETED", "OK");
  }
}

/* ================= SETUP ================= */
void setup() {
  pinMode(BTN_REC, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_DEL,  INPUT_PULLUP);

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  SD.begin(SD_CS);

  Wire.begin(19, 18);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  setupI2S(true);
  showOLED("MODE: RECORD", "BTN9: REC");
}

/* ================= LOOP ================= */
void loop() {
  bool btnRec  = digitalRead(BTN_REC);
  bool btnMode = digitalRead(BTN_MODE);
  bool btnNext = digitalRead(BTN_NEXT);
  bool btnDel  = digitalRead(BTN_DEL);

  /* ==== MODE ==== */
  if (lastBtnMode == HIGH && btnMode == LOW &&
      millis() - debounceMode > debounceDelay) {
    debounceMode = millis();
    sysMode = (sysMode == MODE_RECORD) ? MODE_PLAY : MODE_RECORD;

    if (sysMode == MODE_PLAY) {
      playIndex = 1;
      char f[16]; makeFileName(f, playIndex);
      showOLED("MODE: PLAY", f);
    } else {
      showOLED("MODE: RECORD", "BTN9: REC");
    }
  }
  lastBtnMode = btnMode;

  /* ==== NEXT ==== */
  if (sysMode == MODE_PLAY) {
    if (btnNext == LOW && !nextLocked &&
        millis() - debounceNext > debounceDelay) {
      debounceNext = millis();
      nextLocked = true;
      if (totalFiles > 0) {
        playIndex++;
        if (playIndex > totalFiles) playIndex = 1;
        char f[16]; makeFileName(f, playIndex);
        showOLED("SELECT", f);
      }
    }
    if (btnNext == HIGH) nextLocked = false;
  }

  /* ==== DELETE ==== */
  if (sysMode == MODE_PLAY) {
    if (btnDel == LOW && !delLocked &&
        millis() - debounceDel > debounceDelay) {
      debounceDel = millis();
      delLocked = true;
      deleteCurrentFile();
    }
    if (btnDel == HIGH) delLocked = false;
  }

  /* ==== REC / PLAY ==== */
  if (lastBtnRec == HIGH && btnRec == LOW &&
      millis() - debounceRec > debounceDelay) {
    debounceRec = millis();

    if (sysMode == MODE_RECORD) {
      if (!isRecording) {
        char f[16]; makeFileName(f, recordIndex);
        audioFile = SD.open(f, FILE_WRITE);
        writeWavHeader(audioFile);
        isRecording = true;
        showOLED("RECORDING", f);
      } else {
        isRecording = false;
        updateWavHeader(audioFile);
        recordIndex++;
        totalFiles++;
        showOLED("SAVED", "OK");
      }
    } else {
      if (totalFiles > 0) {
        char f[16]; makeFileName(f, playIndex);
        playFile(f);
      }
    }
  }
  lastBtnRec = btnRec;

  /* ==== RECORD LOOP ==== */
  if (isRecording) {
    size_t br;
    i2s_read(I2S_PORT, i2sBuffer, sizeof(i2sBuffer), &br, portMAX_DELAY);
    for (int i = 0; i < br / 2; i++) i2sBuffer[i] <<= 1;
    audioFile.write((uint8_t*)i2sBuffer, br);
  }
}
