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
snd_output_t *output = NULL;

SoundType *FahrSound::soundFiles=NULL;
bool FahrSound::soundFilesLoaded=false;
snd_pcm_format_t Sound::bits=SND_PCM_FORMAT_UNKNOWN;
int Sound::sample_rate=0;

/**
 * initialisiert alsa - kann beim raspi anscheinend auch mehrmals paralell passieren !!!
 * @param mode: 0 oder NONBLOCK --- NICHT ASYNC, das bringt nix !!!
 *
 */
void Sound::init(int mode)
{
	if(this->handle) {
		printf("sound already initialized\n");
		throw std::runtime_error("sound already initialized");
	}
	if(Sound::bits==SND_PCM_FORMAT_UNKNOWN) { // setParams crasht sonst
		printf("Sound::init() sound format not set\n");
		throw std::runtime_error("Sound::init() sound format not set\n");
	}
	int err;

	if ((err = snd_pcm_open(&this->handle, device, SND_PCM_STREAM_PLAYBACK, mode)) < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		throw std::runtime_error(std::string("Sound::init() Playback open error: )") + snd_strerror(err));
	}
	printf("Sound::init() handle=%p\nbits:%d, sample_rate:%d\n",this->handle,this->bits,this->sample_rate);
	if ((err = snd_pcm_set_params(this->handle,
					this->bits,							// format
					SND_PCM_ACCESS_RW_INTERLEAVED,		// access
					1,									// channels
					this->sample_rate,					// rate
					1,									// soft resample: allow
					500000)) < 0) {   /* 0.5sec */
		printf("Playback open error - error setting params %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

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

}

Sound::~Sound() {
	printf("Sound::[%p]~Sound()\n",this->handle);
	this->close();
	printf("Sound::[%p]~Sound() done\n",this->handle);
}

void Sound::close(bool waitDone) {
	printf("Sound::[%p]close()\n",this->handle);
	// FIXME: race condition: wenn close() in 2 threads in snd_pcm_drain hängtstirbt snd_pcm_close
	if(this->handle) {
		if(waitDone) {
			printf("Sound::[%p]close() -- wait till done\n",this->handle);
			snd_pcm_drain(this->handle); // darauf warten bis alles bis zum ende gespielt wurde
		}
		snd_pcm_close(this->handle);
		this->handle=NULL;
		printf("Sound::[%p]close() -- closed\n",this->handle);
	}
}

void Sound::loadSoundFiles(SoundType *soundFiles) {
	FahrSound::soundFiles=soundFiles;
	if(FahrSound::soundFilesLoaded) {
		return;
	}
	FahrSound::soundFiles->loadSoundFiles();
	FahrSound::soundFilesLoaded=true;

	for(unsigned int i=0; i < countof(cfg_funcSound); i++) {
		if(cfg_funcSound[i] != NOT_SET) {
			Sound::loadSoundFile(cfg_funcSound[i], cfg_funcSound[i] );
		} else {
			printf("no CFG_FUNC_SOUND_%d\n",i);
		}
	}
}

void Sound::loadSoundFile(const std::string &fileName, std::string &dst) {
	Sound::loadWavFile(fileName, dst);
	return; /*
			   WavHeader wavHeader;
			   FILE *f=fopen(fileName.c_str(),"r");
			   if(!f) {
			   fprintf(stderr, "error open '%s' %s\n", fileName.c_str(), strerror(errno));
			   abort();
			   }
			   if(fread(&wavHeader,1,sizeof(wavHeader),f) != sizeof(wavHeader)) {
			   fprintf(stderr, "error fread header '%s' %s\n", fileName.c_str(), strerror(errno));
			   abort();
			   }
			   struct stat buf;
			   fstat(fileno(f), &buf);
			   printf("filename: %s, sample_rate: %d num_channels: %d, bits_per_sample:%d len:%lu\n", fileName.c_str(),
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


void Sound::loadWavFile(const std::string &filename, std::string &out) {
	printf("Sound::loadWavFile() read file: %s\n", filename.c_str());
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
										// printf("formatsize=%d\n",formatsize);
										if ( formatsize == 18 ) {
											int32_t extradata = reader.ReadInt16( );
											printf("seek skipsize=%d\n",extradata);
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
						 printf("seek skipsize=%d\n",skipsize);
						 reader.Seek( skipsize, SEEK_CUR );
						 break; }
		}
	}
	printf("wav file: size=%d, bitdepth=%d, bytespersecond=%d, samplerate=%d channels=%d wavFormat=%d\n", datasize, bitdepth, bytespersecond, samplerate, channels, wavFormat);
	if(bitdepth == 8) {
		Sound::bits=SND_PCM_FORMAT_U8;
	}
	reader.ReadData(datasize, out);

	if(Sound::sample_rate == 0) {
		Sound::sample_rate = samplerate;
	} else if(Sound::sample_rate == samplerate) {
		printf("samplerate OK\n");
	} else if(Sound::sample_rate == samplerate*2) {
		std::string outX2;
		Sound::resampleX2(out, outX2);
		out=outX2;
	} else {
		printf("========== error: invalid samplerate %d\n",samplerate);
		abort();
	}
}

void Sound::resampleX2(const std::string &in, std::string &out) {
	printf("Sound::resampleX2()\n");
	assert(in.length() > 0);
	out.resize((in.length() * 2) -1 );
	for(size_t i=0; i < in.length()-1; i++) {
		out[i*2]=in[i];
		out[i*2+1]=(((unsigned int) (unsigned char) in[i]) + ((unsigned int) (unsigned char) in[i+1])) / 2 ;

		// printf("%d [%d] (%d) ",out[i*2] & 0xff, out[i*2+1] & 0xff, in[i+1] & 0xff);
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
		printf("FahrSound::outloop invalid sound config\n");
	}
}

void FahrSound::cancel() {
	this->currFahrstufe=-1;
	this->doRun=false;
	printf(ANSI_RED2 "FahrSound::cancel() %p\n" ANSI_DEFAULT, this);
}

void FahrSound::diOutloop() {
	printf("FahrSound::diOutloop()\n");
	Sound sound;
	sound.init();
	DiSoundType *diSoundFiles = dynamic_cast<DiSoundType*>(this->soundFiles);
	assert(diSoundFiles && "FahrSound::diOutloop() no di sound");
	int lastFahrstufe=this->currFahrstufe;
	while(this->doRun || lastFahrstufe >= 0) {
		printf("playing [%d]\n",lastFahrstufe); fflush(stdout);
		std::string wav;
		if(this->currFahrstufe == lastFahrstufe) {
			if(lastFahrstufe == -1) {
				sleep(1);
				continue;
			}
			wav=diSoundFiles->steps[lastFahrstufe].run;
		} else if(this->currFahrstufe < lastFahrstufe) {
			wav=diSoundFiles->steps[lastFahrstufe].down;
			printf("v");
			lastFahrstufe--;
		} else {
			lastFahrstufe++;
			wav=diSoundFiles->steps[lastFahrstufe].up;
			printf("^");
		}

		sound.writeSound(wav);

		// printf("Sound::outloop() - testcancel\n");
		this->testcancel();
	}
}

class BoilSteamOutLoop : public Thread {
public:
	BoilSteamOutLoop(const FahrSound *fahrsound) {
		this->fahrsound=fahrsound;
	};
	~BoilSteamOutLoop() {
		this->cancel();
	};
	void run() {
		if(cfg_funcSound[CFG_FUNC_SOUND_BOIL] == NOT_SET) {
			printf(ANSI_RED "no boiler sound");
			return;
		}
		Sound sound;
		sound.init();
		int lastSpeed=fahrsound->currSpeed;
		while(true) {
			if(lastSpeed > 0 && fahrsound->currSpeed==0) {
				PlayAsync quietschen(CFG_FUNC_SOUND_BRAKE);
			}
			lastSpeed=fahrsound->currSpeed;
			sound.writeSound(cfg_funcSound[CFG_FUNC_SOUND_BOIL]);
			this->testcancel();
		}
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

	printf(ANSI_RED2 "FahrSound::steamOutloop() %lu\n" ANSI_DEFAULT, tid);
	while(this->doRun || this->currFahrstufe >= 0) {
		printf(ANSI_RED "FahrSound::steamOutloop %p Fahrstufe:%d\n" ANSI_DEFAULT,this,this->currFahrstufe); fflush(stdout);
		std::string wav;
		if(this->currSpeed <= 0) {
			printf(" ---- out silence\n");
			sleep(1);
			continue;
		}

		wav=dSoundFiles->steps[this->currFahrstufe].ch[1][(slot++)%dSoundFiles->nslots];
		/*
		if(this->currFahrstufe == lastFahrstufe) {
			if(lastFahrstufe == -1) {
				sleep(1);
				continue;
			}
			wav=dSoundFiles->steps[lastFahrstufe].run;
		} else if(this->currFahrstufe < lastFahrstufe) {
			wav=dSoundFiles->steps[lastFahrstufe].down;
			printf("v");
			lastFahrstufe--;
		} else {
			lastFahrstufe++;
			wav=dSoundFiles->steps[lastFahrstufe].up;
			printf("^");
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

		printf("FahrSound::steamOutloop wait: %g\n", s);
		// usleep(s*1000000);
		for(int i = 0; i < ((s-0.1)*100); i++) {
			sound.writeSound(outSilence); // => 0,01s stille
		}
		printf("Sound::steamOutloop() - testcancel\n");
		this->testcancel();
	}
}

void Sound::playSingleSound(int index) {
	printf("Sound::playSingleSound(%d)\n", index);

	this->writeSound(cfg_funcSound[index]);
	printf("Sound::playSingleSound(%d) - done\n", index);
}

/**
 * spielt eine wav datei ab
 * hint: setBlocking(true/false); bestimmt ob bis zum ende gewartet wird
 * @param startpos=0
 * @return frames
 */
int Sound::writeSound(const std::string &data, int startpos) {
	printf("Sound::writeSound(len=%lu, start=%d) \n", data.length(), startpos);
	assert(startpos >= 0);
	assert(data.length() > (unsigned) startpos);
	const char *wavData = data.data() + startpos;
	size_t len = data.length() - startpos;


	snd_pcm_status_t *status;
	snd_pcm_status_alloca(&status);
	int err;
	if ((err = snd_pcm_status(this->handle, status)) < 0) {
		printf("Stream status error: %s\n", snd_strerror(err));
		// exit(0);
	}

	//printf("Sound::[%p]writeSound() ========= status dump\n",this->handle);
	//snd_output_t* out;
	//snd_output_stdio_attach(&out, stderr, 0);
	//snd_pcm_status_dump(status, out);
	if (snd_pcm_state(this->handle) == SND_PCM_STATE_XRUN || 
		snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED) {
		printf("Sound::writeSound need to recover ...\n");
		err = snd_pcm_prepare(handle);
		assert(err >= 0 && "Can't recovery from underrun, prepare failed"); // , snd_strerror(err));
	}

	//printf("Sound::writeSound dataLength=%zd startpos=%d\n", data.length(), startpos);
	snd_pcm_sframes_t frames = snd_pcm_writei(this->handle, wavData, len);
	//printf("Sound::writeSound frames=%ld\n", frames);
	if (frames < 0) { // 2* probieren:
		printf("Sound::[%p]writeSound recover error: %s\n", this->handle, snd_strerror(frames));
		frames = snd_pcm_recover(this->handle, frames, 0);
	}
	if (frames == -EPIPE) {
		/* EPIPE means underrun */
		fprintf(stderr, "underrun occurred\n");
		snd_pcm_prepare(this->handle);
	}
	/*
	if (frames < 0) { // noch immer putt
		printf("Sound::writeSound snd_pcm_writei failed: %s\n", snd_strerror(frames));
		return frames;
	} */
	if (frames > 0 && frames < (snd_pcm_sframes_t) len)
		printf("Sound::writeSound Short write (expected %zi, wrote %li) %s\n", len, frames, strerror(errno));
	printf("Sound::[%p]writeSound done\n",this->handle);
	return frames;
}

/**
 * raspi hat nix geändert
 */
void Sound::setBlocking(bool blocking) {
	printf("Sound::[%p]setBlocking %d\n", this->handle, blocking);	
	int rc=snd_pcm_nonblock	(this->handle, blocking ? 0 : 1);
	if(rc != 0) {
		printf("error setting blocking mode\n");
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
	printf("Sound::setMasterVolume(%d)\n",volume);
    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    // const char *selem_name = "Master";
    // const char *selem_name = "PCM";
	int rc;
	// snd_ctl_card_info_t *info;

    if((rc=snd_mixer_open(&handle, 0))) {
		printf("snd_mixer_open: %s", snd_strerror(rc));
		abort();
	}
	printf("Sound::[%p]setMasterVolume(%d)\n", handle, volume);
	/*
	snd_ctl_card_info_alloca(&info);
	if (rc = snd_ctl_card_info(handle, info)) {
		printf("Control device %s hw info error: %s", card, snd_strerror(rc));
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
		printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
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
	printf("min:%ld max:%ld\n", min, max);
	long range = max - min;
	long calcVolume = (float) range * volume / 255;
	printf("calcVolume %ld => %ld\n", calcVolume, calcVolume + min);
    snd_mixer_selem_set_playback_volume_all(elem, calcVolume + min);
    // snd_mixer_selem_set_playback_dB_all(elem, volume, 0);

    snd_mixer_close(handle);
}

// http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html#a41
static void async_callback(snd_async_handler_t *ahandler)
{
	printf("async_callback\n");
        // snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
        PlayAsyncData *data = (PlayAsyncData*) snd_async_handler_get_callback_private(ahandler);
	if (data->sound == NULL) {
		printf("async_callback --- sound closing/deleted\n");
		return;
	}
        // signed short *samples = data->samples;
        // snd_pcm_channel_area_t *areas = data->areas;
        int end=false;
        
        // snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);
		int writtenFrames=data->sound->writeSound(data->wav, data->position);
	printf("async_callback writtenFrames=%d\n",writtenFrames);
		if(writtenFrames > 0) {
			data->position+=writtenFrames;
			if(data->position >= (int) data->wav.length()) {
				end=true;
			}
		} else {
			end=true;
		}
	printf("async_callback new position=%d\n", data->position);

		if(end) {
			printf("playAsync end\n");
			// macht das ~Sound() schon. Damit destructur fix nur einmal aufgerufen wird:
			// data->sound->close();
			Sound *tmp_sound=data->sound;
			data->sound=NULL;
			delete tmp_sound;
			delete data;
			data=NULL;
		}
/*
        while (avail >= period_size) {
                generate_sine(areas, 0, period_size, &data->phase);
                err = snd_pcm_writei(handle, samples, period_size);
                if (err < 0) {
                        printf("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                }
                if (err != period_size) {
                        printf("Write error: written %i expected %li\n", err, period_size);
                        exit(EXIT_FAILURE);
                }
                avail = snd_pcm_avail_update(handle);
        }
		*/
}

PlayAsync::PlayAsync(const std::string &wav) {
	Sound *sound=new Sound();
	PlayAsyncData *data = new PlayAsyncData(wav, sound, 0);
	data->index=-1;
	data->start();
}

PlayAsync::PlayAsync(int index) {
	Sound *sound=new Sound();
	PlayAsyncData *data = new PlayAsyncData(cfg_funcSound[index], sound, 0);
	data->index=index;
	if(false) { // use alsa async: --- not supported by pulseaudio, crashes sometimes on raspi
		// this->sound->init(SND_PCM_NONBLOCK| SND_PCM_ASYNC);
		sound->init(SND_PCM_NONBLOCK);

		snd_pcm_uframes_t buffer_size = 1024*8;
		snd_pcm_uframes_t period_size = 64*8;
	/*

	snd_pcm_hw_params_t *hw_params;

	snd_pcm_hw_params_malloc (&hw_params);
	snd_pcm_hw_params_any (pcm_handle, hw_params);
		snd_pcm_hw_params_set_buffer_size_near (this->sound->handle, hw_params, &buffer_size);
		snd_pcm_hw_params_set_period_size_near (this->sound->handle, hw_params, &period_size, NULL);
		snd_pcm_hw_params (pcm_handle, hw_params);
		snd_pcm_hw_params_free (hw_params);
	*/

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
			printf("Unable to register async handler %s\n",snd_strerror(err));
			abort();
		}
		err=snd_pcm_prepare(data->sound->handle);
		if (err < 0) {
			printf("prepare error: %s\n", snd_strerror(err));
			abort();
		}

		data->position=data->sound->writeSound(data->wav);
		if (snd_pcm_state(data->sound->handle) == SND_PCM_STATE_PREPARED) {
			printf("PlayAsync in PREPARED state\n");
			err = snd_pcm_start(data->sound->handle);
			if (err < 0) {
				printf("Start error: %s\n", snd_strerror(err));
				abort();
			}
		}
	} else { // use thread
		data->start();
	}
};

void PlayAsyncData::run() {
	this->sound->init();
	this->sound->writeSound(this->wav);
	delete(this);
}

FahrSound::~FahrSound() {
	printf("FahrSound::~FahrSound()\n");
	this->cancel();
	printf("FahrSound::~FahrSound() done\n");
}

void FahrSound::start() {
	printf("FahrSound::start()\n");
	if(!FahrSound::soundFiles) {
		printf("===== no sound files loaded ====\n");
		return;
	}
	/*
// FIXME: wenn thread rennt und doRun false is dann warten bis thread tot und neu starten
	if(this->thread) {
		printf("FahrSound::run: already started\n");
		return;
	}
	printf("FahrSound::run() starting sound thread\n");
	this->doRun=true;

	// Start a thread and then send it a cancellation request

	int s = pthread_create(&this->thread, NULL, &sound_thread_func, (void *) this);
	if (s != 0)
		perror("pthread_create");
*/
	if(this->isRunning() ) {
		printf("FahrSound::start already started\n");
	} else {
		this->doRun=true;
		Thread::start();
	}
	usleep(10000);
	this->currFahrstufe=0;
}

/*
void FahrSound::kill() {
	printf("FahrSound::[%p]kill()\n",this->handle);
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

void FahrSound::setSpeed(int speed) {
	this->currSpeed=speed;
	if( DiSoundType *diSoundFiles = dynamic_cast<DiSoundType*>(this->soundFiles) ) {
		if(this->soundFiles) {
			for(int i=0; i < diSoundFiles->maxSteps; i++) {
				if(speed <= diSoundFiles->steps[i].limit) {
					this->currFahrstufe=i;
					printf("set fahrstufe: %d\n", i);
					return;
				}
			}
		}
	} else if(SteamSoundType *dSoundFiles = dynamic_cast<SteamSoundType*>(this->soundFiles) ) {
		this->currFahrstufe=speed/(256.0) * dSoundFiles->nsteps; // bei 3 fahrstufen:  0-90 => [0] ; -175 => [1] ; -255 => [2]
	}
}


/*
int main(int argc, char *argv[]) {
	int volume=0;
	if(argc == 2) {
		volume=atoi(argv[1]);
	}
	printf("setting volume to %d\n",volume);
	SetAlsaMasterVolume(volume);
}
*/

void DiSoundType::loadSoundFiles() {
	for(int step=0; step < this->nsteps; step++) {
		if(this->steps[step].down != NOT_SET) {
			Sound::loadSoundFile(this->steps[step].down, this->steps[step].down);
			Sound::loadSoundFile(this->steps[step].run,  this->steps[step].run);
		}
		if(this->steps[step].up != NOT_SET) { // bei der höchsten stufe gibts kein up
			Sound::loadSoundFile(this->steps[step].up,   this->steps[step].up);
		}
	}
}

void SteamSoundType::loadSoundFiles() {
	for(int step=0; step < this->nsteps; step++) {
		for(int hml=0; hml < 3; hml ++) {
			for(int slot=0; slot < this->nslots; slot++) {
				Sound::loadSoundFile(this->steps[step].ch[hml][slot], this->steps[step].ch[hml][slot]);
			}
		}
	}
}
