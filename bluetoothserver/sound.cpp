/**
 *  aus alsa demo:
 *  This extra small demo sends a random samples to your speakers.
 *
 *  macht extra sound thread und füttert die soundkarte
 *
 *  http://users.suse.com/~mana/alsa090_howto.html
 *  http://www.linuxjournal.com/article/6735?page=0,1
 *
 *  async alsa:
 *  http://alsa.opensrc.org/HowTo_Asynchronous_Playback
 */
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sound.h"
#include <errno.h>
#include <string.h>
#include <stdexcept>
#include <math.h>
#include "utils.h"
#include "reader.h"
#include "lokdef.h"

#define TAG "sound"

FahrSound clientFahrSound;

// typedef
struct __attribute__((packed)) WavHeader 
{
	char chunk_id[4];
	int chunk_size;
	char format[4];
	char subchunk1_id[4];
	int subchunk1_size;
	short int audio_format;
	short int num_channels;
	int sample_rate;
	int byte_rate;
	short int block_align;
	short int bits_per_sample;
	short int extra_param_size;
	char subchunk2_id[4];
	int subchunk2_size;
};
// WavHeader __attribute__((packed));


const char *device = "default";                        /* playback device */
//const char *device = "softvol";
//const char *device = "plughw:0,0";
//snd_output_t *output = NULL;

SoundType *FahrSound::soundFiles=NULL;
bool FahrSound::soundFilesLoaded=false;
snd_pcm_format_t Sound::bits=SND_PCM_FORMAT_UNKNOWN;
int Sound::sample_rate=0;
int Sound::soundObjects=0;

snd_pcm_uframes_t periodsize = 4096;    /* Periodsize (bytes) */

const char *bits2String(snd_pcm_format_t bits) {
	switch(bits) {
		case SND_PCM_FORMAT_U8: return "SND_PCM_FORMAT_U8";
		case SND_PCM_FORMAT_UNKNOWN: return "SND_PCM_FORMAT_UNKNOWN";
		default: return "SND_PCM_FORMAT_???";
	}
}
					 

/**
 * https://stackoverflow.com/questions/40346132/how-to-properly-set-up-alsa-device
 * @param bits:
 * SND_PCM_FORMAT_U8
 * SND_PCM_FORMAT_S16_LE
 */
int setup_alsa(snd_pcm_t *handle, unsigned int rate, snd_pcm_format_t bits)
{
    int rc;
    snd_pcm_uframes_t periods;          /* Number of fragments/periods */
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *sw_params;
    unsigned int exact_rate;

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    if (snd_pcm_hw_params_any(handle, params) < 0)
    {
        fprintf(stderr, "Can not configure this PCM device.\n");
        snd_pcm_close(handle);
        return(-1);
    }

    /* Set the desired hardware parameters. */
    /* Non-Interleaved mode */
    //snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_NONINTERLEAVED);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, bits);

    /* 44100 bits/second sampling rate (CD quality) */
    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    exact_rate = rate;
    if (snd_pcm_hw_params_set_rate_near(handle, params, &exact_rate, 0) < 0)
    {
        fprintf(stderr, "Error setting rate.\n");
        snd_pcm_close(handle);
        return(-1);
    }

    if (rate != exact_rate)
    {
        fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n==> Using %d Hz instead.\n", rate, exact_rate);
    }

    /* Set number of channels to 1 */
    if( (rc = snd_pcm_hw_params_set_channels(handle, params, 1 )) < 0 )
    {
        fprintf(stderr, "Error setting channels. rc=%d\n",rc);
        snd_pcm_close(handle);
        return(-1);
    }

    /* Set number of periods. Periods used to be called fragments. */
    periods = 4;
    if ( snd_pcm_hw_params_set_periods(handle, params, periods, 0) < 0 )
    {
        fprintf(stderr, "Error setting periods.\n");
        snd_pcm_close(handle);
        return(-1);
    }

    snd_pcm_uframes_t size = (periodsize * periods) >> 2;
    if( (rc = snd_pcm_hw_params_set_buffer_size_near( handle, params, &size )) < 0)
    {
        fprintf(stderr, "Error setting buffersize: [%s]\n", snd_strerror(rc) );
        snd_pcm_close(handle);
        return(-1);
    }
    else
    {
        printf("Buffer size = %lu\n", (unsigned long)size);
    }

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0)
    {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
        snd_pcm_close(handle);
        return -1;
    }

    // snd_pcm_hw_params_free(params);

    /* Allocate a software parameters object. */
    snd_pcm_sw_params_alloca(&sw_params);

    rc = snd_pcm_sw_params_current(handle, sw_params);
    if( rc < 0 )
    {
        fprintf (stderr, "cannot initialize software parameters structure (%s)\n", snd_strerror(rc) );
        return(-1);
    }

    if((rc = snd_pcm_sw_params_set_avail_min(handle, sw_params, 1024)) < 0)
    {
        fprintf (stderr, "cannot set minimum available count (%s)\n", snd_strerror (rc));
        return(-1);
    }

    rc = snd_pcm_sw_params_set_start_threshold(handle, sw_params, 1);
    if( rc < 0 )
    {
        fprintf(stderr, "Error setting start threshold\n");
        snd_pcm_close(handle);
        return -1;
    }

    if((rc = snd_pcm_sw_params(handle, sw_params)) < 0)
    {
        fprintf (stderr, "cannot set software parameters (%s)\n", snd_strerror (rc));
        return(-1);
    }

    // snd_pcm_sw_params_free(sw_params);

    return 0;
}

/**
 * initialisiert alsa - kann beim raspi anscheinend auch mehrmals paralell passieren !!!
 * @param mode: 0 oder NONBLOCK --- NICHT ASYNC, das bringt nix !!!
 *
 */
void Sound::init(int mode)
{
	ERRORF("Sound::init(mode=%d, rate=%d, bits=%s)", mode, this->sample_rate, bits2String(this->bits));
	if(this->handle) {
		DEBUGF("sound already initialized");
		throw std::runtime_error("sound already initialized");
	}
	if(Sound::bits==SND_PCM_FORMAT_UNKNOWN) { // setParams crasht sonst
		DEBUGF("Sound::init() sound format not set");
		throw std::runtime_error("Sound::init() sound format not set");
	}
	int err;

	if ((err = snd_pcm_open(&this->handle, device, SND_PCM_STREAM_PLAYBACK, mode)) < 0) {
		DEBUGF("Playback open error: %s", snd_strerror(err));
		throw std::runtime_error(std::string("Sound::init() Playback open error: ") + snd_strerror(err));
	}

	if(setup_alsa(this->handle, this->sample_rate, this->bits) < 0) {
		throw std::runtime_error("error in setup_alsa");
	}
return;

	// DietPi + I2S DAC 202204: snd_pcm_set_params doesn't work with I2S DAC => aplay init is like setup_alsa, so skipping snd_pcm_set_params
/*
	snd_pcm_hw_params_t *params;
	snd_pcm_hw_params_alloca(&params);
	if (( err = snd_pcm_hw_params_any(handle, params) ) < 0) {
		throw std::runtime_error("Sound::init() error getting default parameters");
	}
	unsigned int val;
	snd_pcm_uframes_t frames;
	err = snd_pcm_hw_params_get_period_size(params, &frames, NULL);
	ERRORF("snd_pcm_hw_param_get err=%d, val=%d", err, frames);
	if ((err = snd_pcm_hw_params_get_period_time_max(params, &val, NULL)) < 0) {
		throw std::runtime_error("Sound::init() error getting period time max");
	}
	unsigned int period_time=std::min(val, 500000u); // either 0.5s or less if hardware doesn't like it
	DEBUGF("Sound::[%p] init() bits:%s, sample_rate:%d, max period_time: %u", this->handle, bits2String(this->bits), this->sample_rate, period_time);
	if ((err = snd_pcm_set_params(this->handle,
					this->bits,				// format
					SND_PCM_ACCESS_RW_INTERLEAVED,		// access
					1,					// channels
					this->sample_rate,			// rate
					1,					// soft resample: allow
					period_time)) < 0) {			// 0.5sec
		throw std::runtime_error(utils::format("Sound::init() error error setting params %s", snd_strerror(err)));
	}
*/

// another try ....
/*
	snd_pcm_hw_params_t *params;
	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(this->handle, params);

	// Set parameters
	if ((err = snd_pcm_hw_params_set_access(this->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		throw std::runtime_error(utils::format("ERROR: Can't set interleaved mode. %s", snd_strerror(err)));
	}

	if ((err = snd_pcm_hw_params_set_format(this->handle, params, SND_PCM_FORMAT_S16_LE)) < 0) {
		throw std::runtime_error(utils::format("ERROR: Can't set format. %s", snd_strerror(err)));
	}

	if ((err = snd_pcm_hw_params_set_channels(this->handle, params, 1)) < 0) {
		throw std::runtime_error(utils::format("ERROR: Can't set channels number. %s", snd_strerror(err)));
	}

	if ((err = snd_pcm_hw_params_set_rate_near(this->handle, params, (unsigned int *) &this->sample_rate, 0)) < 0) {
		throw std::runtime_error(utils::format("ERROR: Can't set rate. %s", snd_strerror(err)));
	}

	// set the buffer time
  snd_pcm_uframes_t     period_size_min;
  snd_pcm_uframes_t     period_size_max;
  snd_pcm_uframes_t     buffer_size_min;
  snd_pcm_uframes_t     buffer_size_max;
  unsigned int       buffer_time = 0;	            // ring buffer length in us
  unsigned int       period_time = 0;	            // period time in us
  unsigned int       nperiods    = 4;                  // number of periods
  snd_pcm_uframes_t  buffer_size;
  snd_pcm_uframes_t  period_size;

  err = snd_pcm_hw_params_get_buffer_size_min(params, &buffer_size_min);
  err = snd_pcm_hw_params_get_buffer_size_max(params, &buffer_size_max);
  err = snd_pcm_hw_params_get_period_size_min(params, &period_size_min, NULL);
  err = snd_pcm_hw_params_get_period_size_max(params, &period_size_max, NULL);
  printf("Buffer size range from %lu to %lu\n",buffer_size_min, buffer_size_max);
  printf("Period size range from %lu to %lu\n",period_size_min, period_size_max);
  if (period_time > 0) {
    printf("Requested period time %u us\n", period_time);
    err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, NULL);
    if (err < 0) {
      throw std::runtime_error(utils::format("Unable to set period time %u us for playback: %s\n",
	     period_time, snd_strerror(err)));
    }
  }
  if (buffer_time > 0) {
    printf(("Requested buffer time %u us\n"), buffer_time);
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, NULL);
    if (err < 0) {
      throw std::runtime_error(utils::format("Unable to set buffer time %u us for playback: %s\n",
	     buffer_time, snd_strerror(err)));
    }
  }
  if (! buffer_time && ! period_time) {
    buffer_size = buffer_size_max;
    if (! period_time)
      buffer_size = (buffer_size / nperiods) * nperiods;
    printf(("Using max buffer size %lu\n"), buffer_size);
    err = snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer_size);
    if (err < 0) {
      throw std::runtime_error(utils::format("Unable to set buffer size %lu for playback: %s\n",
	     buffer_size, snd_strerror(err)));
    }
  }
  if (! buffer_time || ! period_time) {
    printf(("Periods = %u\n"), nperiods);
    err = snd_pcm_hw_params_set_periods_near(handle, params, &nperiods, NULL);
    if (err < 0) {
      throw std::runtime_error(utils::format("Unable to set nperiods %u for playback: %s\n",
	     nperiods, snd_strerror(err)));
    }
  }

  // write the parameters to device
  err = snd_pcm_hw_params(handle, params);
  if (err < 0) {
    throw std::runtime_error(utils::format("Unable to set hw params for playback: %s\n", snd_strerror(err)));
  }

  snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
  snd_pcm_hw_params_get_period_size(params, &period_size, NULL);
  printf(("was set period_size = %lu\n"),period_size);
  printf(("was set buffer_size = %lu\n"),buffer_size);
  if (2*period_size > buffer_size) {
    throw std::runtime_error(utils::format("buffer to small, could not use\n"));
  }

	// Write parameters
	if ((err = snd_pcm_hw_params(this->handle, params)) < 0) {
		throw std::runtime_error(utils::format("ERROR: Can't set harware parameters. %s", snd_strerror(err)));
	}

	// Resume information
	DEBUGF("PCM name: '%s'", snd_pcm_name(this->handle));

	DEBUGF("PCM state: %s", snd_pcm_state_name(snd_pcm_state(this->handle)));

	unsigned int tmp;
	snd_pcm_hw_params_get_channels(params, &tmp);
	DEBUGF("channels: %i", tmp);

	snd_pcm_hw_params_get_rate(params, &tmp, 0);
	DEBUGF("rate: %d bps", tmp);

*/


	if(false) {
		// snd_pcm_uframes_t buffer_size = 1024*8;
		// snd_pcm_uframes_t period_size = 64*8;

		snd_pcm_sw_params_t *sw_params;

		snd_pcm_sw_params_malloc (&sw_params);
		snd_pcm_sw_params_current (this->handle, sw_params);
		// snd_pcm_sw_params_set_start_threshold(this->handle, sw_params, buffer_size - period_size);
		snd_pcm_sw_params_set_start_threshold(this->handle, sw_params, 500);
		// snd_pcm_sw_params_set_avail_min(this->handle, sw_params, period_size);
		snd_pcm_sw_params_set_avail_min(this->handle, sw_params, 100);
		snd_pcm_sw_params(this->handle, sw_params);
		snd_pcm_sw_params_free (sw_params);
	}
	// snd_pcm_hw_params_free(params);
}

Sound::~Sound() {
	DEBUGF("Sound::[%p] ~Sound()",this->handle);
	this->close();
	DEBUGF("Sound::[%p] ~Sound() done",this->handle);
	if(Sound::soundObjects==0) {
		snd_config_update_free_global();
	}
}

void Sound::close(bool waitDone) {
	DEBUGF("Sound::[%p]close()",this->handle);
	// ohne lock: race condition: wenn close() in 2 threads in snd_pcm_drain hängtstirbt snd_pcm_close weil dann this->handle == null
	Lock closeLock(this->mutex);
	if(this->handle) {
		if(waitDone) {
			DEBUGF("Sound::[%p]close() -- wait till done",this->handle);
			snd_pcm_drain(this->handle); // darauf warten bis alles bis zum ende gespielt wurde
		}
		snd_pcm_close(this->handle);
		this->handle=NULL;
		DEBUGF("Sound::[%p]close() -- closed",this->handle);
	}
}

void Sound::loadSoundFiles(SoundType *soundFiles) {
	DEBUGF("Sound::loadSoundFiles");
	FahrSound::soundFiles=soundFiles;
	if(FahrSound::soundFilesLoaded) {
		return;
	}
	FahrSound::soundFiles->loadSoundFiles();
	FahrSound::soundFilesLoaded=true;

	for(unsigned int i=0; i < countof(soundFiles->funcSound); i++) {
		if(soundFiles->funcSound[i]) {
			DEBUGF("loading CFG_FUNC_SOUND_%d = %s", i, soundFiles->funcSound[i].fileName.c_str());
			soundFiles->funcSound[i].load(soundFiles->funcSoundVolume[i]);
			DEBUGF("wav file length: %zuBytes", soundFiles->funcSound[i].wav.length());
		} else {
			DEBUGF("no CFG_FUNC_SOUND_%d",i);
		}
	}
}

void Sound::loadSoundFile(const std::string &fileName, std::string &dst, int volumeLevel) {
	Sound::loadWavFile(fileName, dst, volumeLevel);
	return; /*
			   WavHeader wavHeader;
			   FILE *f=fopen(fileName.c_str(),"r");
			   if(!f) {
			   DEBUGF(stderr, "error open '%s' %s\n", fileName.c_str(), strerror(errno));
			   abort();
			   }
			   if(fread(&wavHeader,1,sizeof(wavHeader),f) != sizeof(wavHeader)) {
			   DEBUGF(stderr, "error fread header '%s' %s\n", fileName.c_str(), strerror(errno));
			   abort();
			   }
			   struct stat buf;
			   fstat(fileno(f), &buf);
			   DEBUGF("filename: %s, sample_rate: %d num_channels: %d, bits_per_sample:%d len:%lu\n", fileName.c_str(),
			   wavHeader.sample_rate, wavHeader.num_channels, wavHeader.bits_per_sample, buf.st_size-sizeof(wavHeader));
			   int bufferSize=buf.st_size-sizeof(wavHeader);
			   unsigned char *buffer;
			   buffer=(unsigned char *) malloc(bufferSize);
			   fread(buffer,1,buf.st_size,f);
			   dst.assign((char*) buffer,bufferSize);
			   free(buffer);
			   if(wavHeader.bits_per_sample == 8) {
			   Sound::bits=SND_PCM_FORMAT_U8;
			   }
			   Sound::sample_rate = wavHeader.sample_rate;
			   */
}
enum class WavChunks {
	RiffHeader = 0x46464952, // RIFF
	WavRiff = 0x54651475,    // WAVE
	Format = 0x020746d66,    // fmt%10
	LabeledText = 0x478747C6,
	Instrumentation = 0x478747C6,
	Sample = 0x6C706D73,
	Fact = 0x47361666,
	Data = 0x61746164,
	Junk = 0x4b4e554a,
};

enum class WavFormat {
	PulseCodeModulation = 0x01,
	IEEEFloatingPoint = 0x03,
	ALaw = 0x06,
	MuLaw = 0x07,
	IMAADPCM = 0x11,
	YamahaITUG723ADPCM = 0x16,
	GSM610 = 0x31,
	ITUG721ADPCM = 0x40,
	MPEG = 0x50,
	Extensible = 0xFFFE
};


void Sound::loadWavFile(const std::string &filename, std::string &out, int volumeLevel) {
#warning FIXME volumeLevel TODO
	DEBUGF("Sound::loadWavFile() read file: %s", filename.c_str());
	Reader reader(filename);
	int channels=-1;
	int32_t samplerate=-1;
	int bytespersecond=-1;
	// int32_t memsize = -1;
	// int32_t riffstyle = -1;
	int32_t datasize = -1;
	int16_t bitdepth = -1;
	WavFormat wavFormat = (WavFormat) -1;
	while ( datasize < 0 ) {
		int32_t chunkid = reader.ReadInt32( );
		switch ( (WavChunks)chunkid ) {
			case WavChunks::Format: {
										int32_t formatsize = reader.ReadInt32( );
										wavFormat = (WavFormat)reader.ReadInt16( );
										// int16_t channels = (Channels)reader.ReadInt16( );
										channels = reader.ReadInt16( );
										samplerate = reader.ReadInt32( );
										bytespersecond = reader.ReadInt32( );
										/*int16_t formatblockalign = */ reader.ReadInt16( );
										bitdepth = reader.ReadInt16( );
										// DEBUGF("formatsize=%d\n",formatsize);
										if ( formatsize == 18 ) {
											int32_t extradata = reader.ReadInt16( );
											DEBUGF("seek skipsize=%d",extradata);
											reader.Seek( extradata, SEEK_CUR );
										}
										break; }
			case WavChunks::RiffHeader: {
											// headerid = chunkid;
											/*memsize =*/ reader.ReadInt32( );
											/*riffstyle =*/ reader.ReadInt32( );
											break; }
			case WavChunks::Data: {
									  datasize = reader.ReadInt32( );
									  break; }
			default: {
						 int32_t skipsize = reader.ReadInt32( );
						 DEBUGF("seek skipsize=%d",skipsize);
						 reader.Seek( skipsize, SEEK_CUR );
						 break; }
		}
	}
	DEBUGF("wav file: size=%d, bitdepth=%d, bytespersecond=%d, samplerate=%d channels=%d wavFormat=%x", datasize, bitdepth, bytespersecond, samplerate, channels, (int) wavFormat);
	if(bitdepth == 8) {
		Sound::bits=SND_PCM_FORMAT_U8;
	}
	reader.ReadData(datasize, out);

	if(Sound::sample_rate == 0) {
		Sound::sample_rate = samplerate;
	} else if(Sound::sample_rate == samplerate) {
		DEBUGF("samplerate OK");
	} else if(Sound::sample_rate == samplerate*2) {
		std::string outX2;
		Sound::resampleX2(out, outX2);
		out=outX2;
	} else {
		DEBUGF("========== error: invalid samplerate %d",samplerate);
		abort();
	}
}

void Sound::resampleX2(const std::string &in, std::string &out) {
	DEBUGF("Sound::resampleX2()");
	assert(in.length() > 0);
	out.resize((in.length() * 2) -1 );
	for(size_t i=0; i < in.length()-1; i++) {
		out[i*2]=in[i];
		out[i*2+1]=(((unsigned int) (unsigned char) in[i]) + ((unsigned int) (unsigned char) in[i+1])) / 2 ;

		// DEBUGF("%d [%d] (%d) ",out[i*2] & 0xff, out[i*2+1] & 0xff, in[i+1] & 0xff);
	}
	out[(in.length()-1)*2]=in[in.length()-1];
}

/*
static void *sound_thread_func(void *startupData)
{
	FahrSound *s=(FahrSound*) startupData;
	s->outloop();
	return NULL;
}
*/

void FahrSound::run() {
	if( dynamic_cast<DiSoundType*>(this->soundFiles)) {
		this->diOutloop();
	} else if( dynamic_cast<SteamSoundType*>(this->soundFiles)) {
		this->steamOutloop();
	} else {
		DEBUGF("FahrSound::outloop invalid sound config");
	}
}

void FahrSound::cancel() {
	this->currFahrstufe=-1;
	this->doRun=false;
	ERRORF("FahrSound::cancel() %p" ANSI_DEFAULT, this);
}

class EMotorOutLoop : public Thread {
public:
	EMotorOutLoop(const FahrSound *fahrsound) {
		this->fahrsound=fahrsound;
	};
	~EMotorOutLoop() {
		if(this->isRunning())
			this->cancel(true);
	};
	void run() {
		if(! this->fahrsound->soundFiles->funcSound[CFG_FUNC_SOUND_EMOTOR]) {
			NOTICEF("no EMotor sound (funcSound %d)", CFG_FUNC_SOUND_EMOTOR);
			return;
		}
		std::string orgWav=this->fahrsound->soundFiles->funcSound[CFG_FUNC_SOUND_EMOTOR].loop();
		// float currWavSpeed=0;
		std::string wav;
		DEBUGF("############################### EMotorOutLoop org.data=%p wav.data=%p", orgWav.data(), wav.data());
		wav.append(orgWav);
		DEBUGF("############################### EMotorOutLoop org.data=%p wav.data=%p", orgWav.data(), wav.data());

		Sound sound;
		sound.init();
		// start+stop muss nicht abgespielt werden, Sound sollte bei speed=0 = volume=0 anfangen
		// sound.writeSound(this->fahrsound->soundFiles->funcSound[CFG_FUNC_SOUND_EMOTOR].loopStart());
		unsigned int loopPlayPos=0;
		while(true) {
		
			this->testcancel(); // exit loop via exception
			if(lokdef[0].currspeed < 5) {
				DEBUGF("EMotorOutLoop while stopped");
				sleep(1);
				continue;
			}
			
			float nextWavSpeed=lokdef[0].currspeed/255.0;
//			DEBUGF("############################### EMotorOutLoop %f org.data=%p wav.data=%p loopPlayPos=%d out level=%d\n",
//				nextWavSpeed, orgWav.data(), wav.data(),loopPlayPos, lokdef[0].currspeed/255);
//			if(nextWavSpeed != currWavSpeed) {
				int outPos=0;
				float skip=0;
				// immer nur 0.33s samples senden damit wir schneller auf geschwindigkeits änderung reagieren können
				int inSamples=MIN(Sound::sample_rate/3, (int) (orgWav.length()-loopPlayPos));
				wav.resize(inSamples); // maximal brauchen wir inSamples bytes
				for(int i=0; i<inSamples; i++) {
					if(skip >= 1) {
						// skip
						skip=skip-1;
					} else {
						unsigned char s=((int_fast16_t)((unsigned char) orgWav[loopPlayPos+i])-0x80) * lokdef[0].currspeed/255 + 0x80;
						wav[outPos++]=s;
						skip+=nextWavSpeed;
					}
				}
				wav.resize(outPos-1);
				loopPlayPos+=inSamples;
				if(loopPlayPos >= orgWav.length())
					loopPlayPos=0;
//			}
			sound.writeSound(wav);
		}
		// sound.writeSound(this->fahrsound->soundFiles->funcSound[CFG_FUNC_SOUND_EMOTOR].loopEnd());
	};
private:
	const FahrSound *fahrsound;
};

void FahrSound::diOutloop() {
	DEBUGF("FahrSound::diOutloop() %d", this->currFahrstufe);
	Sound sound;
	sound.init();
	DiSoundType *diSoundFiles = dynamic_cast<DiSoundType*>(this->soundFiles);
	assert(diSoundFiles && "FahrSound::diOutloop() no di sound");
	EMotorOutLoop emotor(this);
	emotor.start();
	int lastFahrstufe=this->currFahrstufe;
	this->currFahrstufe=0;
	while(this->doRun || lastFahrstufe >= 0) {
DEBUGF("FahrSound::diOutloop ####### %d %d", this->doRun, lastFahrstufe);
		this->currSpeed=lokdef[0].currspeed;
		if(this->currSpeed == 0 && lastFahrstufe == 0 && ! this->doRun) {
			this->currFahrstufe = -1;
		} else {
			for(int i=0; i < diSoundFiles->nsteps; i++) {
				if(this->currSpeed < diSoundFiles->steps[i].limit) {
					this->currFahrstufe=i;
					DEBUGF("set fahrstufe: %d (limit %d)", i, diSoundFiles->steps[i].limit);
					break;
				}
			}
		}
		DEBUGF("FahrSound::diOutloop() playing [%d/%d]",lastFahrstufe, this->currFahrstufe); fflush(stdout);
		Sample sample;
		if(this->currFahrstufe == lastFahrstufe) {
			if(lastFahrstufe == -1) {
				sleep(1);
				continue;
			}
			sample=diSoundFiles->steps[lastFahrstufe].run;
		} else if(this->currFahrstufe < lastFahrstufe) {
			sample=diSoundFiles->steps[lastFahrstufe].down;
			DEBUGF("v");
			lastFahrstufe--;
		} else {
			lastFahrstufe++;
			sample=diSoundFiles->steps[lastFahrstufe].up;
			DEBUGF("^");
		}

		DEBUGF("FahrSound::diOutloop() playing %zu bytes (last byte: %d)", sample.wav.length(), 
			sample.wav.length() > 0 ? (unsigned char) sample.wav[0] : 0);
		sound.writeSound(sample.wav);

		// DEBUGF("Sound::outloop() - testcancel\n");
		this->testcancel();
	}
	DEBUGF("Sound::outloop() done");
}

class BoilSteamOutLoop : public Thread {
public:
	BoilSteamOutLoop(const FahrSound *fahrsound) {
		this->fahrsound=fahrsound;
	};
	~BoilSteamOutLoop() {
		this->cancel(true);
	};
	void run() {
		if(! this->fahrsound->soundFiles->funcSound[CFG_FUNC_SOUND_BOIL]) {
			ERRORF("no boiler sound");
			return;
		}
		Sound sound;
		sound.init();
		sound.writeSound(this->fahrsound->soundFiles->funcSound[CFG_FUNC_SOUND_BOIL].loopStart());
		while(true) {
			sound.writeSound(this->fahrsound->soundFiles->funcSound[CFG_FUNC_SOUND_BOIL].loop());
			this->testcancel();
		}
		sound.writeSound(this->fahrsound->soundFiles->funcSound[CFG_FUNC_SOUND_BOIL].loopEnd());
	};
private:
	const FahrSound *fahrsound;
};

void FahrSound::steamOutloop() {
	Sound sound;
	sound.init();
	SteamSoundType *dSoundFiles = dynamic_cast<SteamSoundType*>(this->soundFiles);
	assert(dSoundFiles && "Sound::steamOutloop() no steam sound");
	// int lastFahrstufe=this->currFahrstufe;
	int slot=0;
	BoilSteamOutLoop boil(this);
	boil.start();

	pthread_t   tid = this->self();
	// this->setBlocking(false);
	std::string outSilence= std::string(22000 / 100, 0x80);

	int lastSpeed=0;
	int lastBrake=0;
	int lastAcc=0;
	NOTICEF("FahrSound::steamOutloop() %lu", tid);
// ?????????? currFahrstufe umbaun auf 0 == stop ??????????????
	while(this->doRun || this->currFahrstufe >= 0) {
		this->currSpeed=lokdef[0].currspeed;
		this->currFahrstufe=this->currSpeed/(256.0) * dSoundFiles->nsteps; // bei 3 fahrstufen:  0-90 => [0] ; -175 => [1] ; -255 => [2]

		DEBUGF("FahrSound::steamOutloop %p Fahrstufe:%d",this,this->currFahrstufe); fflush(stdout);
		if(lastSpeed > 0 && this->currSpeed==0) {
			PlayAsync quietschen(this->soundFiles->funcSound[CFG_FUNC_SOUND_BRAKE]);
		} else if(lastSpeed == 0 && this->currSpeed > 0) {
			PlayAsync entwaessern(this->soundFiles->funcSound[CFG_FUNC_SOUND_ENTWAESSERN]);
		}
		if(this->currSpeed > lastSpeed) {
			lastAcc=time(NULL);
		} else if(this->currSpeed < lastSpeed) {
			lastBrake=time(NULL);
		}
		lastSpeed=this->currSpeed;

		if(this->currSpeed <= 0) {
			DEBUGF(" ---- out silence");
			sleep(1);
			continue;
		}

		int lmh=STEAM_SLOT_NORMAL;
		if(lastAcc > time(NULL) - 2) {
			lmh=STEAM_SLOT_ACC;
		} else // Acc hat vorrang
		if(lastBrake > time(NULL) -2) {
			lmh=STEAM_SLOT_BRAKE;
		}
		// DEBUGF("BoilSteamOutLoop:run -> lmh: %i #################################\n", lmh);
			
		const std::string &wav=dSoundFiles->steps[this->currFahrstufe].ch[lmh][(slot++)%dSoundFiles->nslots].wav;
		/*
		if(this->currFahrstufe == lastFahrstufe) {
			if(lastFahrstufe == -1) {
				sleep(1);
				continue;
			}
			wav=dSoundFiles->steps[lastFahrstufe].run;
		} else if(this->currFahrstufe < lastFahrstufe) {
			wav=dSoundFiles->steps[lastFahrstufe].down;
			DEBUGF("v");
			lastFahrstufe--;
		} else {
			lastFahrstufe++;
			wav=dSoundFiles->steps[lastFahrstufe].up;
			DEBUGF("^");
		}
		*/


		// sound.dump_sw();
		double x=this->currSpeed/255.0;
		// double factor=x - pow(x-0.5,2) + 0.25;
		// https://graphsketch.com/?eqn1_color=1&eqn1_eqn=&eqn2_color=2&eqn2_eqn=sin%28x*pi%2F2%29%5E0.6&eqn3_color=3&eqn3_eqn=&eqn4_color=4&eqn4_eqn=&eqn5_color=5&eqn5_eqn=&eqn6_color=6&eqn6_eqn=&x_min=-2&x_max=2&y_min=-2&y_max=2&x_tick=1&y_tick=1&x_label_freq=5&y_label_freq=5&do_grid=0&do_grid=1&bold_labeled_lines=0&bold_labeled_lines=1&line_width=4&image_w=850&image_h=525
		// https://www.desmos.com/calculator (x-1)^3+1
		double factor=pow(sin(x*3.14/2),0.6);
		double s=1-0.95*(factor);
		if(s < 0.1) {
			sound.writeSound(wav.substr(0, wav.length()*(s/0.1)) );
		} else {
			sound.writeSound(wav);
		}

		DEBUGF("FahrSound::steamOutloop wait: %g", s);
		// usleep(s*1000000);
		for(int i = 0; i < ((s-0.1)*100); i++) {
			sound.writeSound(outSilence); // => 0,01s stille
		}
		DEBUGF("Sound::steamOutloop() - testcancel");
		this->testcancel();
	}
}

/*
void Sound::playSingleSound(int index) {
	DEBUGF("Sound::playSingleSound(%d)\n", index);

	this->writeSound(cfg_funcSound[index]);
	DEBUGF("Sound::playSingleSound(%d) - done\n", index);
}
*/

/**
 * spielt eine wav datei ab
 * hint: setBlocking(true/false); bestimmt ob bis zum ende gewartet wird
 * @param startpos=0
 * @return frames
 */
int Sound::writeSound(const std::string &data, int startpos) {
	//DEBUGF("Sound::writeSound(len=%lu, start=%d) \n", data.length(), startpos);
	assert(startpos >= 0);
	assert(data.length() >= (unsigned) startpos);
	const char *wavData = data.data() + startpos;
	size_t len = data.length() - startpos;
/* ===================================== check clicks ==============================================================================
	DEBUGF("Sound::writeSound checking data len=%zu, startpos=%d", data.length(), startpos);
	if(len > 0) {
		if((unsigned char) data[0] > this->lastSample +20 || (unsigned char) data[0] < this->lastSample-20) {
			ERRORF("Sound::writeSound[0] differs %u -> %u", this->lastSample, (unsigned char) data[0]);
		}
		for(size_t i=1; i < len-1; i++) {
			if((unsigned char) data[i] > (unsigned char) data[i+1]+20 || (unsigned char) data[i] < (unsigned char) data[i+1]-20) {
				ERRORF("Sound::writeSound[%zu] differs %u -> %u", i, (unsigned char) data[i], (unsigned char) data[i+1]);
			}

		}
		this->lastSample=(unsigned char) data[len-1];
	}
*/

	snd_pcm_status_t *status;
	snd_pcm_status_alloca(&status);
	int err;
	if ((err = snd_pcm_status(this->handle, status)) < 0) {
		ERRORF("Stream status error: %s", snd_strerror(err));
		// exit(0);
	}

	//DEBUGF("Sound::[%p]writeSound() ========= status dump\n",this->handle);
	//snd_output_t* out;
	//snd_output_stdio_attach(&out, stderr, 0);
	//snd_pcm_status_dump(status, out);
	if (snd_pcm_state(this->handle) == SND_PCM_STATE_XRUN || 
		snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED) {
		DEBUGF("Sound::writeSound need to recover ...");
		err = snd_pcm_prepare(handle);
		assert(err >= 0 && "Can't recovery from underrun, prepare failed"); // , snd_strerror(err));
	}

	//DEBUGF("Sound::writeSound dataLength=%zd startpos=%d\n", data.length(), startpos);
	snd_pcm_sframes_t frames = snd_pcm_writei(this->handle, wavData, len);
	//DEBUGF("Sound::writeSound frames=%ld\n", frames);
	if (frames == -EPIPE) {
		/* EPIPE means underrun */
		ERRORF("underrun occurred");
		snd_pcm_prepare(this->handle);
	} else if (frames == -EBADFD) {
		ERRORF("pcm in bad state");
	} else if (frames == -ESTRPIPE) {
		ERRORF("a suspend event occurred");
	} else if (frames < 0) { // 2* probieren:
		DEBUGF("Sound::[%p] writeSound recover error: %lu = %s", this->handle, frames, snd_strerror(frames));
		frames = snd_pcm_recover(this->handle, frames, 0);
	}
	/*
	if (frames < 0) { // noch immer putt
		DEBUGF("Sound::writeSound snd_pcm_writei failed: %s\n", snd_strerror(frames));
		return frames;
	} */
	if (frames > 0 && frames < (snd_pcm_sframes_t) len)
		DEBUGF("Sound::writeSound Short write (expected %zi, wrote %li) %s", len, frames, strerror(errno));
	// DEBUGF("Sound::[%p]writeSound done\n",this->handle);
	return frames;
}

/**
 * raspi hat nix geändert
 */
void Sound::setBlocking(bool blocking) {
	DEBUGF("Sound::[%p]setBlocking %d", this->handle, blocking);	
	int rc=snd_pcm_nonblock	(this->handle, blocking ? 0 : 1);
	if(rc != 0) {
		ERRORF("error setting blocking mode\n");
		abort();
	}
}

/**
 * @see https://fossies.org/dox/alsa-utils-1.1.2/amixer_8c_source.html
 *      http://www.alsa-project.org/alsa-doc/alsa-lib/group___simple_mixer.html
 * @volume: 0...255
 */
void Sound::setMasterVolume(int volume)
{
	DEBUGF("Sound::setMasterVolume(%d)",volume);
    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    // const char *selem_name = "Master";
    // const char *selem_name = "PCM";
	int rc;
	// snd_ctl_card_info_t *info;

    if((rc=snd_mixer_open(&handle, 0))) {
		ERRORF("snd_mixer_open: %s", snd_strerror(rc));
		abort();
	}
	DEBUGF("Sound::[%p]setMasterVolume(%d)", handle, volume);
	/*
	snd_ctl_card_info_alloca(&info);
	if (rc = snd_ctl_card_info(handle, info)) {
		DEBUGF("Control device %s hw info error: %s", card, snd_strerror(rc));
		return err;
	}
	*/
    if((rc=snd_mixer_attach(handle, device))) {
		perror("snd_mixer_attach");
		abort();
	}
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);

	snd_mixer_elem_t* elem;
	for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem)) {
		snd_mixer_selem_get_id(elem, sid);
		if (!snd_mixer_selem_is_active(elem)) {
			continue;
		}
		DEBUGF("Simple mixer control '%s',%i", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		break;
	}
	/*
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
	*/
	if(!elem) {
		perror("snd_mixer_find_selem");
		abort();
	}

    // snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	DEBUGF("min:%ld max:%ld", min, max);
	long range = max - min;
	long calcVolume = (float) range * volume / 255;
	DEBUGF("calcVolume %ld => %ld", calcVolume, calcVolume + min);
    snd_mixer_selem_set_playback_volume_all(elem, calcVolume + min);
    // snd_mixer_selem_set_playback_dB_all(elem, volume, 0);

    snd_mixer_close(handle);
}

/*
// http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html#a41
static void async_callback(snd_async_handler_t *ahandler)
{
	DEBUGF("async_callback");
        // snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
	PlayAsyncData *data = (PlayAsyncData*) snd_async_handler_get_callback_private(ahandler);
	if (data->sound == NULL) {
		DEBUGF("async_callback --- sound closing/deleted");
		return;
	}
        // signed short *samples = data->samples;
        // snd_pcm_channel_area_t *areas = data->areas;
        int end=false;
        
        // snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);
		int writtenFrames=data->sound->writeSound(data->wav, data->position);
	DEBUGF("async_callback writtenFrames=%d",writtenFrames);
		if(writtenFrames > 0) {
			data->position+=writtenFrames;
			if(data->position >= (int) data->wav.length()) {
				end=true;
			}
		} else {
			end=true;
		}
	DEBUGF("async_callback new position=%d", data->position);

		if(end) {
			DEBUGF("playAsync end");
			// macht das ~Sound() schon. Damit destructur fix nur einmal aufgerufen wird:
			// data->sound->close();
			Sound *tmp_sound=data->sound;
			data->sound=NULL;
			delete tmp_sound;
			delete data;
			data=NULL;
		}
/ *
        while (avail >= period_size) {
                generate_sine(areas, 0, period_size, &data->phase);
                err = snd_pcm_writei(handle, samples, period_size);
                if (err < 0) {
                        DEBUGF("Write error: %s", snd_strerror(err));
                        exit(EXIT_FAILURE);
                }
                if (err != period_size) {
                        DEBUGF("Write error: written %i expected %li", err, period_size);
                        exit(EXIT_FAILURE);
                }
                avail = snd_pcm_avail_update(handle);
        }
		* /
}
*/

PlayAsync::PlayAsync(const Sample &sample) : data(NULL) {
	PlayAsyncData *data = new PlayAsyncData(sample, 0);
	data->start();
	data->setAutodelete();
}

PlayAsync::PlayAsync(PlayAsyncData *data) : data(NULL) {
	data->start();
	data->setAutodelete();
}

void PlayAsync::play(PlayAsyncData *data) {
	if(this->data != NULL) {
		if(this->data->isRunning()) {
			this->data->setAutodelete();
		} else {
			delete this->data;
		}
	}
	this->data=data;
	data->start();
	// no autodelete here !! data->setAutodelete();
}

/*
PlayAsync::PlayAsync(int index) {
	Sound *sound=new Sound();
	PlayAsyncData *data = new PlayAsyncData(cfg_funcSound[index], sound, 0);
	data->index=index;
	if(false) { // use alsa async: --- not supported by pulseaudio, crashes sometimes on raspi
		// this->sound->init(SND_PCM_NONBLOCK| SND_PCM_ASYNC);
		sound->init(SND_PCM_NONBLOCK);

		snd_pcm_uframes_t buffer_size = 1024*8;
		snd_pcm_uframes_t period_size = 64*8;
	/ *

	snd_pcm_hw_params_t *hw_params;

	snd_pcm_hw_params_malloc (&hw_params);
	snd_pcm_hw_params_any (pcm_handle, hw_params);
		snd_pcm_hw_params_set_buffer_size_near (this->sound->handle, hw_params, &buffer_size);
		snd_pcm_hw_params_set_period_size_near (this->sound->handle, hw_params, &period_size, NULL);
		snd_pcm_hw_params (pcm_handle, hw_params);
		snd_pcm_hw_params_free (hw_params);
	* /

		snd_pcm_sw_params_t *sw_params;

		snd_pcm_sw_params_malloc (&sw_params);
		snd_pcm_sw_params_current (data->sound->handle, sw_params);
		snd_pcm_sw_params_set_start_threshold(data->sound->handle, sw_params, buffer_size - period_size);
		snd_pcm_sw_params_set_avail_min(data->sound->handle, sw_params, period_size);
		snd_pcm_sw_params(data->sound->handle, sw_params);
		snd_pcm_sw_params_free (sw_params);

		snd_async_handler_t *ahandler;
		int err=snd_async_add_pcm_handler(&ahandler, data->sound->handle, async_callback, data);
		if (err < 0) {
			DEBUGF("Unable to register async handler %s",snd_strerror(err));
			abort();
		}
		err=snd_pcm_prepare(data->sound->handle);
		if (err < 0) {
			DEBUGF("prepare error: %s", snd_strerror(err));
			abort();
		}

		data->position=data->sound->writeSound(data->wav);
		if (snd_pcm_state(data->sound->handle) == SND_PCM_STATE_PREPARED) {
			DEBUGF("PlayAsync in PREPARED state");
			err = snd_pcm_start(data->sound->handle);
			if (err < 0) {
				DEBUGF("Start error: %s", snd_strerror(err));
				abort();
			}
		}
	} else { // use thread
		data->start();
	}
};
*/

void PlayAsyncData::run() {
	Sound sound;
	sound.init();
	sound.writeSound(this->sample.wav);
	// warten bis sound fertig gespielt wurde:
	sound.close(true);
	this->done();
}

FahrSound::~FahrSound() {
	DEBUGF("FahrSound::~FahrSound()");
	this->cancel();
	DEBUGF("FahrSound::~FahrSound() done");
}

void FahrSound::start() {
	DEBUGF("FahrSound::start()");

	if(!FahrSound::soundFiles) {
		NOTICEF("===== no sound files loaded ====");
		return;
	}
	if(lokdef[0].nFunc <= SOUND_FUNC_NUM_HORN ||
	  lokdef[0].nFunc <= SOUND_FUNC_NUM_WHISTLE ||
	  lokdef[0].nFunc <= SOUND_FUNC_NUM_ENABLE) {
		ERRORF("SOUND_FUNC_NUM_HORN / SOUND_FUNC_NUM_WHISTLE / SOUND_FUNC_NUM_ENABLE invalid!");
		abort();
	}
	/*
// FIXME: wenn thread rennt und doRun false is dann warten bis thread tot und neu starten
	if(this->thread) {
		DEBUGF("FahrSound::run: already started");
		return;
	}
	DEBUGF("FahrSound::run() starting sound thread");
	this->doRun=true;

	// Start a thread and then send it a cancellation request

	int s = pthread_create(&this->thread, NULL, &sound_thread_func, (void *) this);
	if (s != 0)
		perror("pthread_create");
*/
	if(this->isRunning() ) {
		DEBUGF("FahrSound::start already started");
	} else {
		this->doRun=true;
		Thread::start();
	}
	// usleep(10000);
	// this->currFahrstufe=0;
}

/*
void FahrSound::kill() {
	DEBUGF("FahrSound::[%p]kill()\n",this->handle);
	if(this->thread) {
		int s = pthread_cancel(this->thread);
		if (s != 0)
			perror("pthread_cancel");
		void *ret;
		pthread_join(this->thread, &ret);
		this->thread=0;
	}
}
*/

void FahrSoundPlayFuncAsyncData::done() {
	// DEBUGF("FahrSoundPlayFuncAsyncData::done ************************************* %d\n", this->func);
	lokdef[0].func[this->func].ison=false;
}

void FahrSound::startPlayFuncSound() {
	DEBUGF("FahrSound::startPlayFuncSound #####################################");
	static PlayAsync horn;
	static PlayAsync whistle;
	if(lokdef[0].func[SOUND_FUNC_NUM_HORN].ison && ! horn.isPlaying() && (this->soundFiles->funcSound[CFG_FUNC_SOUND_HORN] )) {
		FahrSoundPlayFuncAsyncData *data=new FahrSoundPlayFuncAsyncData(this->soundFiles->funcSound[CFG_FUNC_SOUND_HORN], SOUND_FUNC_NUM_HORN);
		horn.play(data);
	}
	if(lokdef[0].func[SOUND_FUNC_NUM_WHISTLE].ison && ! whistle.isPlaying() && this->soundFiles->funcSound[CFG_FUNC_SOUND_ABFAHRT] ) {
		FahrSoundPlayFuncAsyncData *data=new FahrSoundPlayFuncAsyncData(this->soundFiles->funcSound[CFG_FUNC_SOUND_ABFAHRT], SOUND_FUNC_NUM_WHISTLE);
		whistle.play(data);
	}

	// funcname sSound on off suchen
	if(SOUND_FUNC_NUM_ENABLE < 0) {
		for(int i=0; i < lokdef[0].nFunc; i++) {
			if(STREQ(lokdef[0].func[i].name, "sSound ein/aus")) {
				SOUND_FUNC_NUM_ENABLE=i;
				break;
			}
		}
		if(SOUND_FUNC_NUM_ENABLE < 0) {
			ERRORF("func sSound ein/aus not found, pease disable sound or add [sSound ein/aus] function");
			abort();
		}
		DEBUGF("FahrSound::start() SOUND_FUNC_NUM_ENABLE=%d", SOUND_FUNC_NUM_ENABLE);
	}
	
	if(lokdef[0].func[SOUND_FUNC_NUM_ENABLE].ison != this->isRunning()) {
		DEBUGF("FahrSound::startPlayFuncSound sound enabled: %d running: %d",
			lokdef[0].func[SOUND_FUNC_NUM_ENABLE].ison, this->isRunning());
		if(lokdef[0].func[SOUND_FUNC_NUM_ENABLE].ison) {
			this->start();
		} else {
			this->cancel();
		}
	}
}

/*
int main(int argc, char *argv[]) {
	int volume=0;
	if(argc == 2) {
		volume=atoi(argv[1]);
	}
	DEBUGF("setting volume to %d",volume);
	SetAlsaMasterVolume(volume);
}
*/

void DiSoundType::loadSoundFiles() {
	for(int step=0; step < this->nsteps; step++) {
		if(this->steps[step].down) {
			this->steps[step].down.load(0);
			this->steps[step].run.load(0);
		}
		if(this->steps[step].up) { // bei der höchsten stufe gibts kein up
			this->steps[step].up.load(0);
		}
	}
}

void SteamSoundType::loadSoundFiles() {
	for(int step=0; step < this->nsteps; step++) {
		for(int hml=0; hml < 3; hml ++) {
			for(int slot=0; slot < this->nslots; slot++) {
				this->steps[step].ch[hml][slot].load(0);
			}
		}
	}
}
