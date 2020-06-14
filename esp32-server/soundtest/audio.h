#include "SPIFFS.h"
#include "ESP32_MAS.h"

#ifdef MAS
#else
  void example_i2s_init();
  void example_i2s_start();
  void example_i2s_stop();
#endif

//extern bool Audio_Player_run;

