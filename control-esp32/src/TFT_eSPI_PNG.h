// Created by Bodmer 5/11/19 https://github.com/Bodmer/PNG_TEST_ONLY/blob/master/png_to_sprite/support_functions.h
// https://github.com/kikuchan/pngle

#include <math.h>

#include "pngle.h"

#ifdef USE_LINE_BUFFER
#define LINE_BUF_SIZE 240  // pixel = 524, 16 = 406, 32 = 386, 64 = 375, 128 = 368, 240 = 367, no draw = 324 (51ms v 200ms)
uint16_t lbuf[LINE_BUF_SIZE];
#endif

class TFT_eSPI_PngSprite {
private:
// static wegen callback !!! wir laden aber hoffentlich immer nur ein einziges png, nicht multithreaded 2 gleichzeitig
  TFT_eSprite &spr;

  int16_t sx=0;
  int16_t sy=0;
  uint32_t pc=0;

  int16_t png_dx=0, png_dy=0;
  uint32_t transparentColor=TFT_TRANSPARENT;

public:
  TFT_eSPI_PngSprite(TFT_eSprite &spr) : spr(spr)  {
  };


// Define corner position
void setPngPosition(int16_t x, int16_t y)
{
  png_dx = x;
  png_dy = y;
};

void setTransparentColor(uint32_t color)
{
  this->transparentColor=color;
}

// Draw pixel - called by pngle
void static pngle_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
  TFT_eSPI_PngSprite *ptr =(TFT_eSPI_PngSprite *) pngle_get_user_data(pngle);
  assert(ptr);
  // DEBUGF("pngle_on_draw %d:%d %02x%02x%02x(%d)", x+png_dx, y+png_dy, rgba[0],rgba[1],rgba[2],rgba[3]);
  if (rgba[3] > 127) { // Transparency threshold (no blending yet...)
    uint32_t color = ((rgba[0] >> 3) << 11) | ((rgba[1] >> 2) << 5) | (rgba[2] >> 3);

  #ifdef USE_LINE_BUFFER
    color = color << 8 | color >> 8;
    if ( (x == sx + pc) && (y == sy) && (pc < LINE_BUF_SIZE) ) lbuf[pc++] = color;
    else {
      // Push pixels to sprite
      ptr->spr.pushImage(png_dx + sx, png_dy + sy, pc, 1, lbuf);
      sx = x;
      sy = y;
      pc = 0;
      lbuf[pc++] = color;
    }
  #else
    ptr->spr.drawPixel(ptr->png_dx + (int32_t)x, ptr->png_dy + (int32_t)y, color);
  #endif
  } else {
    if(ptr->transparentColor != TFT_TRANSPARENT)
      ptr->spr.drawPixel(ptr->png_dx + (int32_t)x, ptr->png_dy + (int32_t)y, ptr->transparentColor);
  }
};

// Render from FLASH array
void load_file(const uint8_t* arrayData, uint32_t arraySize)
{
  sx = png_dx;
  sy = png_dy;
  pc = 0;

  pngle_t *pngle = pngle_new();
  if(!pngle) {
    throw std::runtime_error("failed to start pngle");
  }
  pngle_set_user_data(pngle, (void *) this);
  pngle_set_draw_callback(pngle, pngle_on_draw);

  // Feed data to pngle
  uint8_t buf[1024];

  uint32_t remain = 0;
  uint32_t arrayIndex = 0;
  uint32_t avail  = arraySize;
  uint32_t take = 0;
  
  //tft.startWrite(); // Crashes Adafruit_GFX, no advantage for TFT_eSPI with line buffer
  while ( avail > 0 ) {
    take = sizeof(buf) - remain;
    if (take > avail) take = avail;
    memcpy_P(buf + remain, (const uint8_t *)(arrayData + arrayIndex), take);
    arrayIndex += take;
    remain += take;
    avail -= take;
    int fed = pngle_feed(pngle, buf, remain);
    if (fed < 0) {
      //Serial.printf("ERROR: %s\n", pngle_error(pngle));
      break;
    }
    remain = remain - fed;
    if (remain > 0) memmove(buf, buf + fed, remain);
  }
#ifdef USE_LINE_BUFFER

  // Push any remaining pixels - because we had no warning that image has ended...
  if (pc) {spr.pushImage(png_dx + sx, png_dy + sy, pc, 1, lbuf); pc = 0;}

#endif
  //tft.endWrite();

  pngle_destroy(pngle);
};

void load_data(const std::string data)
{
  sx = png_dx;
  sy = png_dy;
  pc = 0;

  pngle_t *pngle = pngle_new();
  if(!pngle) {
    throw std::runtime_error("failed to start pngle");
  }
  pngle_set_user_data(pngle, (void *) this);
  pngle_set_draw_callback(pngle, pngle_on_draw);


  uint32_t take = 0;
  
  int n=0;

  //tft.startWrite(); // Crashes Adafruit_GFX, no advantage for TFT_eSPI with line buffer
  while ( data.length() > take ) {
    DEBUGF("read png (%d,%d)",take, data.length());
    int fed = pngle_feed(pngle, data.data()+take, data.length()-take);
    if (fed < 0) {
      ERRORF("pngle error: %s\n", pngle_error(pngle));
      break;
    }
    take+=fed;
    n++;
    if(n > 50) {
      abort();
    }
  }
#ifdef USE_LINE_BUFFER

  // Push any remaining pixels - because we had no warning that image has ended...
  if (pc) {spr.pushImage(png_dx + sx, png_dy + sy, pc, 1, lbuf); pc = 0;}

#endif

  pngle_destroy(pngle);
};

};
