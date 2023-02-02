#include <GyverPortal.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <LittleFS.h>
#include "MAX7219.h"
#include "dig1.h"
#include "dig2.h"
#include "dig3.h" // шрифт для часов
#include "pics.h"
#include "GyverTimer.h"

struct LoginPass {
  char ssid[20];
  char pass[20];
  int gmt;
  int bright;
  int fonttype;
  int selmode;
};
LoginPass lp;


#define DW 6  // кол-во МИКРОСХЕМ по горизонтали
#define DH 3  // кол-во МИКРОСХЕМ по вертикали

#define DWW (DW*4)
#define DHH (DH*2)

// указать пин CS!
MaxDisp<D8, DW, DH> disp;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 60000);
GTimer_ms myTimer; 
GyverPortal ui(&LittleFS);



void build() {
  GP.BUILD_BEGIN();
  GP.THEME(GP_DARK);

  GP.FORM_BEGIN("/login");
  GP.LABEL("SSID");
  GP.TEXT("lg", "SSID", lp.ssid);
  GP.BREAK();
  GP.LABEL("PASSWORD");
  GP.TEXT("ps", "Password", lp.pass);
  GP.BREAK();
  GP.LABEL("GMT");
  GP.NUMBER("gmt", "gmt", lp.gmt);
  GP.BREAK();
  GP.LABEL("Brightness");
  GP.NUMBER("brght", "brght", lp.bright);
  GP.BREAK();
  GP.LABEL("Font");
  GP.NUMBER("font", "font", lp.fonttype);
  GP.BREAK();
  GP.LABEL("Mode");
  GP.NUMBER("mode", "mode", lp.selmode);
  GP.SUBMIT("Submit");
  GP.FORM_END();

  GP.BUILD_END();
}

uint8_t d_width = 20;
void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  myTimer.setInterval(500);
  // читаем логин пароль из памяти
  EEPROM.begin(100);
  EEPROM.get(0, lp);
  randomSeed(analogRead(0));
  display_setup();
  timeClient.setTimeOffset(lp.gmt*3600);//Сдвиг часового пояса
  // если кнопка нажата - открываем портал
  pinMode(D2, INPUT_PULLUP);
  
  // запускаем портал
  ui.attachBuild(build);
  ui.start();
  ui.enableOTA();
  ui.attach(action);
  
  if (!LittleFS.begin()) 
    Serial.println("FS Error");
  ui.downloadAuto(true);
  
  if (!digitalRead(D2)) 
    loginPortal();

  if (lp.fonttype == 0)
    d_width = 23;
  else if (lp.fonttype == 1)
    d_width = 17;
  else if (lp.fonttype == 2)
    d_width = 20;
  // пытаемся подключиться
  Serial.print("Connect to: ");
  Serial.println(lp.ssid);
  running("Clock V2");
  running(lp.ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(lp.ssid, lp.pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! Local IP: ");
  running("Подключено");  
  running(WiFi.localIP().toString());  

  Serial.println(WiFi.localIP());
}

void display_setup(){
  disp.begin();
  disp.setBright(lp.bright);
  disp.textDisplayMode(GFX_ADD);
}

void running(String txt) {
  char Buf[50];
  txt.toCharArray(Buf, 50);
  running(Buf);
}
void running(char* txt) {
  disp.setScale(3);
  int w = strlen(txt) * 5 * 3 + disp.W();
  for (int x = disp.W(); x > -w; x--) {
    disp.clear();
    disp.setCursor(x, 6);
    disp.print(txt);
    disp.update();
    delay(10);
  }
}

void loginPortal() {
  Serial.println("Portal start");
  running("Режим настройки");  
  // запускаем точку доступа
  WiFi.mode(WIFI_AP);
  WiFi.softAP("WiFi Clocks");


  // работа портала
  while (ui.tick());
}

void action(GyverPortal& p) {
  if (p.form("/login")) {      // кнопка нажата
    p.copyStr("lg", lp.ssid);  // копируем себе
    p.copyStr("ps", lp.pass);
    p.copyInt("gmt", lp.gmt);
    p.copyInt("brght", lp.bright);
    p.copyInt("font", lp.fonttype);
    p.copyInt("mode", lp.selmode);
    EEPROM.put(0, lp);              // сохраняем
    EEPROM.commit();                // записываем
    WiFi.softAPdisconnect();        // отключаем AP
    running("Сохранение...");  
    ESP.restart();
  }
}

void loop() {
 switch(lp.selmode){
  case 1:
    clocks();
    break;
  case 2:
    party();
    break;
  case 3:
    net();
    break;
  case 4:
    bigBall();
    break;
 }
  ui.tick();
}

void clocks(){
   if (myTimer.isReady()){
    timeClient.update();
    int h = timeClient.getHours(); 
    int m = timeClient.getMinutes();
    Serial.println(timeClient.getFormattedTime());

   
    static volatile bool d;
    if(m==25)
      display_setup();//Чиниим сегменты
    drawClock(h, m, d);
    d = !d;
  }
}

void party() {
  // вечеринка gyver
  disp.setScale(3);
  delay(700);
  disp.clear();
  disp.setCursor(14, 6);
  disp.print("Alex");
  disp.update();

  delay(700);
  disp.clear();
  disp.setCursor(4, 6);
  disp.print("Gyver");
  disp.update();

  delay(700);
  disp.clear();
  disp.drawBitmap(26, 0, wrench_43x36, 43, 36, 0, 0);
  disp.update();
}

void net() {
  const byte radius = 6;
  const byte amount = 3;
  static bool start = false;
  static int x[amount], y[amount];
  static int velX[amount], velY[amount];
  if (!start) {
    start = 1;
    for (byte i = 0; i < amount; i++) {
      x[i] = random(radius, (disp.W() - radius) * 10);
      y[i] = random(radius, (disp.H() - radius) * 10);
      velX[i] = random(10, 20);
      velY[i] = random(10, 20);
    }
  }
  disp.clear();
  for (byte i = 0; i < amount; i++) {
    x[i] += velX[i];
    y[i] += velY[i];
    if (x[i] >= (disp.W() - 1 - radius) * 10 || x[i] < radius * 10) velX[i] = -velX[i];
    if (y[i] >= (disp.H() - 1 - radius) * 10 || y[i] < radius * 10) velY[i] = -velY[i];
    disp.circle(x[i] / 10, y[i] / 10, radius);
  }

  for (int i = 0; i < amount; i++) {
    for (int j = 0; j < amount; j++) {
      disp.line(x[i] / 10, y[i] / 10, x[j] / 10, y[j] / 10);
    }
  }
  disp.update();
}

void bigBall() {
  disp.clear();
  byte radius = 6;
  static int x = (disp.W() / 2) * 10, y = (disp.H() / 2) * 10;
  static int velX = random(3, 6), velY = random(3, 6);
  x += velX;
  y += velY;
  if (x >= (disp.W() - radius) * 10 || x < radius * 10) velX = -velX;
  if (y >= (disp.H() - radius) * 10 || y < radius * 10) velY = -velY;

  disp.circle(x / 10, y / 10, radius, GFX_FILL);
  disp.update();
}

void drawDigit(byte dig, int x) {
  switch(lp.fonttype)
  {
    case 0:
      disp.drawBitmap(x, 0, (const uint8_t*)pgm_read_dword(&(digs1[dig])), d_width, 36, 0);
      break;
    case 1:
      disp.drawBitmap(x, 0, (const uint8_t*)pgm_read_dword(&(digs2[dig])), d_width, 36, 0);
      break;
    case 2:
      disp.drawBitmap(x, 0, (const uint8_t*)pgm_read_dword(&(digs3[dig])), d_width, 36, 0);
      break;
  }
}

void drawClock(byte h, byte m, bool dots) {
  disp.clear();
  //if (h < 9) 
    drawDigit(h / 10, 0);
  drawDigit(h % 10, d_width + 2);
  if (dots) {
    disp.setByte(11, 2, 0b0011101);
    disp.setByte(11, 4, 0b1100011);
  }
  drawDigit(m / 10, 95 - d_width * 2 - 3);
  drawDigit(m % 10, 95 - d_width);
  disp.update();
}
