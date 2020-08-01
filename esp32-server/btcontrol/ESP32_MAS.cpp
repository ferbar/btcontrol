/*Copyright (C) 2018  Johannes Schreiner Otterthal AUSTRIA
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.
  If not, see <http://www.gnu.org/licenses/>.*/

#include "config.h"

#ifdef HAVE_SOUND

#include <Arduino.h>
#include <FS.h>
#include "driver/i2s.h"
#include "esp_task.h"
#include "ESP32_MAS.h"
#include "SPIFFS.h"
#include "xtensa/core-macros.h"

#define CHANNELS 3

void Audio_Player(void *ptr) {
  Serial.println("Audio Player started");

  ESP32_MAS *mas=(ESP32_MAS *) ptr;
  mas->run();
  Serial.println("Audio_Player stopped");
  vTaskDelete(NULL);
}

void ESP32_MAS::run() {
  int file_buf_len;
//  int8_t out_buf_8[64];
  int8_t out_buf_8[1024];
  int buf_len_8 = sizeof(out_buf_8);
  int16_t out_buf_16[sizeof(out_buf_8) / 2];
  int buf_len_16 = sizeof(out_buf_8) / 2;
  int8_t file_buf[3][sizeof(out_buf_8) / 2];
  float pitch[CHANNELS] = {0, 0, 0};
  float pitch_loc;
  bool end_file = false;
  File aiff_file[3];
  //-------------------------------------------------------------------------I2S-configuration
  //--------------------------------------------------------------------------I2S-interlal DAC
  i2s_config_t i2s_config_noDAC = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = 22050,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
//    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, //
// [chris] !!! in der doc steht nur I2S_COMM_FORMAT_I2S_MSB
//    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
//    .communication_format = (i2s_comm_format_t) I2S_COMM_FORMAT_I2S_MSB,
                                              // I2S_COMM_FORMAT_I2S ??? 
    .communication_format = (i2s_comm_format_t) I2S_COMM_FORMAT_PCM,
//    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
//    .dma_buf_len = 64,              // => bei loop gibts "tssiiit" bei 64Byte buffer
    .dma_buf_len = sizeof(out_buf_8),
    .use_apll  =  false,        // 20200611 idl 3.3: apll enabled only works with samplerate < 22kHz
    .tx_desc_auto_clear = true, // send zeros if buffer underrun
    .fixed_mclk = 0 // calculate by the driver
  };
  //----------------------------------------------------------------------------I2S-extern DAC
  i2s_config_t i2s_config_DAC = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 22050,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll  =  true,
    .tx_desc_auto_clear = true, // [chris] keine ahnung
	.fixed_mclk = 0 // calculate by the driver
  };
  //-----------------------------------------------------------------------------I2S-pin-config
  i2s_pin_config_t pin_config = {
    .bck_io_num = this->I2S_BCK,
    .ws_io_num = this->I2S_WS,
    .data_out_num = this->I2S_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  //-----------------------------------------------------------------------------------open I2S
  if (this->I2S_noDAC) {
    Serial.println("RUN noDAC ");
    printf("i2s_driver_install: port_num=%d dma_buf_len=%d\n",this->I2S_PORT, i2s_config_noDAC.dma_buf_len);
    if(i2s_driver_install(this->I2S_PORT, &i2s_config_noDAC, 0, NULL) != ESP_OK) {
      Serial.println("i2s_driver_install error");
      abort(); 
    }
    Serial.println("init noDAC");
    // [chris] 
    i2s_set_pin(this->I2S_PORT, NULL);
    if(i2s_config_noDAC.channel_format == I2S_CHANNEL_FMT_ONLY_RIGHT) {
      i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    } else if(i2s_config_noDAC.channel_format == I2S_CHANNEL_FMT_RIGHT_LEFT) {
      i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    } else {
      Serial.println("invalid channel format");
      abort();
    }
     
    //i2s_set_clk(mas->I2S_PORT, i2s_config_noDAC.sample_rate, i2s_config_noDAC.bits_per_sample, (i2s_channel_t) 1);
    //i2s_set_sample_rates(mas->I2S_PORT, 22050);
    Serial.println("init noDAC done");
  }
  else {
    i2s_driver_install(this->I2S_PORT, &i2s_config_DAC, 0, NULL);
    i2s_set_pin(this->I2S_PORT, &pin_config);
  }
  i2s_zero_dma_buffer(this->I2S_PORT);
  Serial.print("RUN I2S ON PORT_NUM: "); Serial.println(this->I2S_PORT);

  while (this->Audio_Player_run) {
    //------------------------------------------------------------------------AUDIO PLAYER LOOP
    for (int h = 0; h < CHANNELS; h++) {
      //--------------------------------------------------------------------------read channels
      if (this->Channel[h] > 1) {
        pitch_loc = this->Pitch[h];
        //---------------------------------------------------------------------------------play
        if (!aiff_file[h].available()) {
          printf("[%d] open file\n", h);
          //--------------------------------------------------------------------------open file
          this->openFile(h, aiff_file[h]);
        }//                                                                           open file
        //------------------------------------------------------------------read file to buffer
        
        file_buf_len = aiff_file[h].available();
        file_buf_len =  file_buf_len - (file_buf_len * pitch_loc) / 2;
        if (file_buf_len > buf_len_16) {
          file_buf_len = buf_len_16;
          end_file = false;
        }
        else {
          printf("[%d] end file reached (%d)\n", h, file_buf_len);
          end_file = true;
        }
        for (int i = 0; i < file_buf_len; i++) {
          file_buf[h][i] = aiff_file[h].read();
          pitch[h] += pitch_loc;
          if (pitch[h] >= 1) {
            file_buf[h][i] = aiff_file[h].read();
            pitch[h] -= 1;
          }
        }//                                                                 read file to buffer
        if (end_file) {
          //--------------------------------------------------------------------------file emty
          if (this->Channel[h] < 5 && this->Channel[h] > 1) {
            //--------------------------------------------------------------------open new file
            uint32_t fopen1 = XTHAL_GET_CCOUNT();
            this->openFile(h, aiff_file[h]);
            uint32_t fopen2 = XTHAL_GET_CCOUNT();

            //-------------------------------------------------------read new file to to buffer
            int available=aiff_file[h].available();
            if(available < 0) {
              printf("----- available < 0");
              abort();
            }
            int read_end=min(available+file_buf_len, buf_len_16);
            printf("[%d] open file (part) filling %d bytes open took %uclk\n", h, buf_len_16-file_buf_len,fopen2-fopen1);
            for (int i = file_buf_len; i  < read_end; i++) {
              file_buf[h][i] = aiff_file[h].read();
              pitch[h] += pitch_loc;
              if (pitch[h] >= 1) {
                file_buf[h][i] = aiff_file[h].read();
                pitch[h] -= 1;
              }
            }
            // could not fill buffer, give up and fill it with 0
            for (int i = read_end; i < buf_len_16; i++) {
              file_buf[h][i] = 0;
            }
            /*
            for(int i=0; i < file_buf_len; i++) {
              printf("%d ", file_buf[h][i]);
            }
            printf("***");
            for(int i = file_buf_len; i < buf_len_16; i++) {
              printf("%d ", file_buf[h][i]);
            }
            */
          }//                                                        read new file to to buffer
          else {
            //-----------------------------------------------------------cleare rest of channel
            if(h == 0) printf("End of file, closing + clearing %d bytes\n", buf_len_16-file_buf_len);
            aiff_file[h].close();
            this->Channel[h] = 0;
            for (int i = file_buf_len; i < buf_len_16; i++) {
              file_buf[h][i] = 0;
            }//for
          }//                                                        cleare rest of the channel
        }//                                                                           file emty
        
      }//                                                                                  play
      else {
        //---------------------------------------------------------------------------------stop
        // if(h == 0) printf("stop clearing %d bytes\n", buf_len_16);
        for (int i = 0; i < buf_len_16; i++) {
          //----------------------------------------------------------------write clear channel
          file_buf[h][i] = 0;
        }//                                                                 write clear channel
      }//                                                                                  stop
    }//read channels
    //------------------------------------------------------------------------------------MIXER
    
    for (int i = 0; i < buf_len_16; i++) {
      out_buf_16[i] = (file_buf[0][i] * (this->Gain[0]) +
                       file_buf[1][i] * (this->Gain[1]) +
                       file_buf[2][i] * (this->Gain[2]))
                      * float(this->Volume / 255.0);
      // printf("fill %d in:%d out:%d\n", i, file_buf[0][i], out_buf_16[i]);
    }//                                                                                   MIXER
    // [chris]
    int min=255;
    int max=0;
    if (this->I2S_noDAC) {
      // Serial.print("noDac: " ); Serial.println(buf_len_8);
      if(buf_len_8 % 2 != 0) {
        printf("----------error buf_len=%d\n",buf_len_8);
      }

      /*
      static uint32_t last=0;
      uint32_t ccount = XTHAL_GET_CCOUNT();

      printf("--%u\n", ccount-last);
      last=ccount;
      */
//    Serial.println(zahl > buf_len_8 ? "--------------- richtig3 -----------" : "--------------falsch3 -----------------");
//      printf("===out_buf_8=%p min=%p max=%p\n", &out_buf_8, &min, &max);
      for (int i = 0; i < buf_len_16; i ++) {
//      for (volatile int i = 0; i < (buf_len_8-1); i ++) {
//    Serial.println(zahl > buf_len_8 ? "--------------- richtig4 -----------" : "--------------falsch4 -----------------");
        // only right channel => only one 16bit value
        int16_t x = (out_buf_16[i] + 0x8000); // convert 16bit signed to 8bit unsigned
        //out_buf_8[i] = lowByte(x);
        out_buf_8[i*2] = 0;
        uint8_t val=highByte(x);
        // -------- sinus
        // uint8_t val=sin((double)i /1024*3.1416*2*10)*10+0x80;
        
        out_buf_8[i*2 + 1] = val;
        if(val > max) max=val;
        if(val < min) min=val;
        //printf("read %d: in:%d x:%d out:%u %u \n",i, out_buf_16[i], x, (unsigned char) out_buf_8[i*2], (unsigned char) out_buf_8[i*2 + 1]);
        
        // Serial.print(i); Serial.print(" "); Serial.print(buf_len_8); Serial.print(" in:"); Serial.print(out_buf_16[i]); 
        // Serial.print(" x="); Serial.print(x); Serial.print(" val="); Serial.println(val);
        // Serial.println();
        // printf("val: %d\n", val);
// silence:
//        out_buf_8[i] = 0x80;
//        out_buf_8[i +1] = 0x80;
      }
      /*
      if(loop++ % 22 == 0 ) {
        printf("buf_len_8=%d loop=%d min=%d max=%d gain[0]=%d\n", buf_len_8, loop, min, max, this->Gain[0]);
      }
      */
      // break;
	  } else {
      for (int i = 0; i < buf_len_8; i = i + 2) {
        //---------------------------------------------------------------------write IS2 buffer
        out_buf_8[i] = lowByte(out_buf_16[i / 2]);
        out_buf_8[i + 1] = highByte(out_buf_16[i / 2]);
      }
    }//                                                                        write IS2 buffer
//   for(int i=0; i < buf_len_8; i++) 
//      Serial.printf("%d:",out_buf_8[i]);
    // size_t bytes_written=0;
    // esp_err_t ret=i2s_write(mas->I2S_PORT, (const char *)&out_buf_8, buf_len_8, &bytes_written, 500);
    // Serial.printf("bytes_written: %d, err=%d", bytes_written, ret); Serial.println(); // 500 delay
    int ret=i2s_write_bytes(this->I2S_PORT, (const char *)&out_buf_8, buf_len_8, portMAX_DELAY);
    if(ret != buf_len_8) {
      printf("Error: bytes_written: %d to %d, len=%d\n", ret, this->I2S_PORT, buf_len_8);
    }
    vTaskDelay(10);
  }//                                                                         AUDIO PLAYER LOOP
  i2s_driver_uninstall(this->I2S_PORT); //stop & destroy i2s driver
}//                                                                           VOID AUDIO PLAYER

ESP32_MAS::ESP32_MAS() {

};

void ESP32_MAS::setPort(i2s_port_t port) {
  I2S_PORT = port;
};

void ESP32_MAS::setOut(uint8_t bck, uint8_t ws, uint8_t data) {
  I2S_BCK = bck;
  I2S_WS = ws;
  I2S_DATA = data;
};

void ESP32_MAS::setDAC(bool dac) {
  I2S_noDAC = dac;
};

void ESP32_MAS::startDAC() {
  if(Audio_Player_run) {
    Serial.println("already running");
  } else {
    Audio_Player_run=true;
    // [chris] xTaskCreatePinnedToCore(Audio_Player, "Audio_Player", 10000, (void*)&ptr_array, 1, NULL, 0);
    // xTaskCreatePinnedToCore(Audio_Player, "Audio_Player", 10000, (void*)&ptr_array, tskIDLE_PRIORITY, NULL, 0);
    xTaskCreate(Audio_Player, "Audio_Player", 15000, (void*)this, 5, NULL);
    Serial.println("Pinned AUDIO PLAYER to core 0");
  }
};

void ESP32_MAS::stopDAC() {
  Audio_Player_run=false;
}

void ESP32_MAS::setVolume(uint8_t volume) {
  Volume = volume;
};

void ESP32_MAS::stopChan(uint8_t channel) {
  assert(channel < CHANNELS);
  Channel[channel] = 0;
};

void ESP32_MAS::brakeChan(uint8_t channel) {
  assert(channel < CHANNELS);
  Channel[channel] = 1;
};

void ESP32_MAS::playFile(uint8_t channel, String audio_file) {
  assert(channel < CHANNELS);
  Audio_File[channel] = audio_file;
  Channel[channel] = 2;
};

void ESP32_MAS::loopFile(uint8_t channel, String audio_file) {
  assert(channel < CHANNELS);
  Audio_File[channel] = audio_file;
  Channel[channel] = 3;
};

void ESP32_MAS::runChan(uint8_t channel) {
  assert(channel < CHANNELS);
  Channel[channel] = 4;
};

void ESP32_MAS::outChan(uint8_t channel) {
  assert(channel < CHANNELS);
  Channel[channel] = 5;
};

void ESP32_MAS::setGain(uint8_t channel, uint8_t gain) {
  assert(channel < CHANNELS);
  Gain[channel] = gain;
};

void ESP32_MAS::setPitch(uint8_t channel, float pitch) {
  assert(channel < CHANNELS);
  if (pitch < 0) {
    pitch = 0;
  }
  if (pitch > 1) {
    pitch = 1;
  }
  Pitch[channel] = pitch;
};

String ESP32_MAS::getChan(uint8_t channel) {
  assert(channel < CHANNELS);
  String Return;
  switch (Channel[channel]) {
    case 0: //  0 = STOP, 1 = BRAKE, 2 = PLAY, 3 = LOOP, 4 = RUN, 5 = OUT
      Return = "STOP";
      break;
    case 1:
      Return = "BRAKE";
      break;
    case 2:
      Return = "PLAY";
      break;
    case 3:
      Return = "LOOP";
      break;
    case 4:
      Return = "RUN";
      break;
    case 5:
      Return = "OUT";
      break;
  }
  return Return;
};

uint8_t ESP32_MAS::getGain(uint8_t channel) {
  assert(channel < CHANNELS);
  return Gain[channel];
};

float ESP32_MAS::getPitch(uint8_t channel) {
  assert(channel < CHANNELS);
  return Pitch[channel];
};


void ESP32_MAS::openFile(uint8_t channel, File &f) {
  if(this->Audio_File[channel] != f.name()) {
    printf("open file [%s, %s]\n", this->Audio_File[channel].c_str(), f.name() ? f.name() : "null");
    f = SPIFFS.open(this->Audio_File[channel], "r");
  } else {
    printf("[%d] seek 0\n", channel);
    f.seek(0); // seek0 is nicht viel schneller als neues open !!!!
  }
  switch (this->Channel[channel]) {
    case 2:
      this->Channel[channel] = 5;
      break;
    case 3:
      this->Channel[channel] = 4;
      break;
    case 4:
      this->Channel[channel] = 4;
      break;
  }
}

bool ESP32_MAS::isRunning() {
  return this->Audio_Player_run;
}


#endif
