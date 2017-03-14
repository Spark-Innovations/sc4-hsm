
#include "Adafruit_SSD1306.h"
#include "hardware.h"
#include "utils.h"
#undef print

Adafruit_SSD1306 *display = 0;

extern "C" void init_display(void) {
  display = new Adafruit_SSD1306(0, 0, 0);
  display->begin(SSD1306_SWITCHCAPVCC);
  display->setTextColor(WHITE);
}

string banner = (char*)
  "\\2 SC4-HSM\\1 v0.4\n\n  Copyright (c) 2016\nSpark Innovations Inc";

extern "C" void show_banner(void) {
  lcd_print(banner);
}

extern "C" void invertDisplay(int i) { display->invertDisplay(i); }

extern "C" void rotateDisplay(int r) { display->setRotation(r); }

extern "C" void lcd_print(string s) {
  if (!display) init_display();
  display->clearDisplay();
  display->setTextSize(1);
  display->setCursor(0, 0);
  int esc = 0;
  if (strlen(s)==0) s=banner;
  for (int i=0; i<(int)strlen(s); i++) {
    char c = s[i];
    if (esc) {
      esc = 0;
      switch(c) {
      case '\\': display->print("\\"); break;
      case 'n': display->println(); break;
      case '1': display->setTextSize(1); break;
      case '2': display->setTextSize(2); break;
      case '3': display->setTextSize(3); break;
      case '4': display->setTextSize(4); break;
      default: i--;
      }
    } else {
      if (c=='\\') esc = 1;
      else display->print(c);
    }
  }
  display->display();
}

extern "C" void moire() {
  do {
    int x = rand() & 0x7f;
    int y = rand() & 0x1f;
    for (int i=0; i<128; i+=2) {
      display->drawLine(x,y,i,0, WHITE);
      display->drawLine(x,y,i,31, WHITE);
      display->drawLine(x,y,i+1,0, BLACK);
      display->drawLine(x,y,i+1,31, BLACK);
      display->display();
      delay(5);
    }
    for (int i=0; i<32; i+=2) {
      display->drawLine(x,y,0,i, WHITE);
      display->drawLine(x,y,127,i, WHITE);
      display->drawLine(x,y,0,i+1, BLACK);
      display->drawLine(x,y,127,i+1, BLACK);
      display->display();
      delay(5);
    }
    delay(100);
  } while (!(user_buttons() | serial_available()));
}

extern "C" void lcd_noise() {
  do {
    display->clearDisplay();
    for (int i=0; i<128; i++) {
      int r = read_rng();
      for (int j=0; j<32; j++) {
	display->drawPixel(i, j, r&1);
	r = r>>1;
      }
    }
    display->display();
    delay(100);
  } while (!(user_buttons() | serial_available()));
}
