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

int FahrSound::currFahrstufe=-1;
SoundType *FahrSound::soundFiles=NULL;
bool FahrSound::soundFilesLoaded=false;
bool FahrSound::doRun=false;
pthread_t FahrSound::thread=0;
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
	printf("Sound::init() handle=%p\n",this->handle);
	if ((err = snd_pcm_set_params(this->handle,
					this->bits,
					SND_PCM_ACCESS_RW_INTERLEAVED,
					1,
					this->sample_rate,
					1,
					500000)) < 0) {   /* 0.5sec */
		printf("Playback open error - error setting params %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
}

Sound::~Sound() {
	printf("Sound::~Sound()\n");
	this->close();
	printf("Sound::~Sound() done\n");
}

void Sound::close(bool waitDone) {
	printf("Sound::close()\n");
	if(this->handle) {
		if(waitDone) {
			printf("Sound::close -- wait till done\n");
			snd_pcm_drain(this->handle); // darauf warten bis alles bis zum ende gespielt wurde
		}
		snd_pcm_close(this->handle);
		this->handle=NULL;
	}
}

void Sound::loadSoundFiles(SoundType *soundFiles) {
	assert(soundFiles);
	FahrSound::soundFiles=soundFiles;
	if(FahrSound::soundFilesLoaded) {
		return;
	}
	// std::string fileName=std::string("sound/DampfDieselZimoSounds/") + this->soundFiles[0].run;
	for(int i=0; i < 10; i++) {
		if(FahrSound::soundFiles[i].down != "") {
			Sound::loadSoundFile(FahrSound::soundFiles[i].down, FahrSound::soundFiles[i].down);
			Sound::loadSoundFile(FahrSound::soundFiles[i].up,   FahrSound::soundFiles[i].up);
			Sound::loadSoundFile(FahrSound::soundFiles[i].run,  FahrSound::soundFiles[i].run);
		}
	}
	FahrSound::soundFilesLoaded=true;

	if(cfg_funcSound[CFG_FUNC_SOUND_ABFAHRT] != "") {
		Sound::loadSoundFile(cfg_funcSound[CFG_FUNC_SOUND_ABFAHRT], cfg_funcSound[CFG_FUNC_SOUND_ABFAHRT] );
	} else {
		printf("no CFG_FUNC_SOUND_ABFAHRT\n");
	}
	if(cfg_funcSound[CFG_FUNC_SOUND_HORN] != "") {
		Sound::loadSoundFile(cfg_funcSound[CFG_FUNC_SOUND_HORN], cfg_funcSound[CFG_FUNC_SOUND_HORN] );
	} else {
		printf("no CFG_FUNC_SOUND_HORN\n");
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


void Sound::loadWavFile(std::string filename, std::string &out) {
	printf("read file: %s\n", filename.c_str());
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
	Sound::sample_rate = samplerate;
	reader.ReadData(datasize, out);
}

static void *sound_thread_func(void *startupData)
{
	FahrSound *s=(FahrSound*) startupData;
	s->outloop();
	return NULL;
}

void FahrSound::outloop() {
	int lastFahrstufe=this->currFahrstufe;
	while(this->doRun || lastFahrstufe >= 0) {
		printf(".[%d]",lastFahrstufe); fflush(stdout);
		std::string wav;
		if(this->currFahrstufe == lastFahrstufe) {
			if(lastFahrstufe == -1) {
				sleep(1);
				continue;
			}
			wav=this->soundFiles[lastFahrstufe].run;
		} else if(this->currFahrstufe < lastFahrstufe) {
			wav=this->soundFiles[lastFahrstufe].down;
			printf("v");
			lastFahrstufe--;
		} else {
			lastFahrstufe++;
			wav=this->soundFiles[lastFahrstufe].up;
			printf("^");
		}

		this->writeSound(wav);

		// printf("Sound::outloop() - testcancel\n");
		pthread_testcancel();
	}
}

void Sound::playSingleSound(int index) {
	printf("Sound::playSingleSound(%d)\n",index);

	this->writeSound(cfg_funcSound[index]);
	printf("Sound::playSingleSound(%d) - done\n",index);
}

/**
 * spielt eine wav datei ab
 * hint: setBlocking(true/false); bestimmt ob bis zum ende gewartet wird
 * @param startpos=0
 * @return frames
 */
int Sound::writeSound(const std::string &data, int startpos) {
	assert(startpos >= 0);
	assert(data.length() > (unsigned) startpos);
	const char *wavData = data.data() + startpos;
	size_t len = data.length() - startpos;

	snd_pcm_sframes_t frames = snd_pcm_writei(this->handle, wavData, len);
	if (frames < 0) { // 2* probieren:
		printf("Sound::writeSound recover error: %s\n", snd_strerror(frames));
		frames = snd_pcm_recover(this->handle, frames, 0);
	}
	if (frames < 0) { // noch immer putt
		printf("Sound::writeSound snd_pcm_writei failed: %s\n", snd_strerror(frames));
		return frames;
	}
	if (frames > 0 && frames < (snd_pcm_sframes_t) len)
		printf("Sound::writeSound Short write (expected %zi, wrote %li) %s\n", len, frames, strerror(errno));
	printf("Sound::writeSound done\n");
	return frames;
}

/**
 * raspi hat nix geändert
 */
void Sound::setBlocking(bool blocking) {
	printf("Sound::setBlocking %d\n",blocking);	
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
        // signed short *samples = data->samples;
        // snd_pcm_channel_area_t *areas = data->areas;
        int end=false;
        
        // snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);
		int writtenFrames=data->sound->writeSound(data->data, data->position);
	printf("async_callback writtenFrames=%d\n",writtenFrames);
		if(writtenFrames > 0) {
			data->position+=writtenFrames;
			if(data->position >= (int) data->data.length()) {
				end=true;
			}
		} else {
			end=true;
		}
	printf("async_callback new position=%d\n", data->position);

		if(end) {
			printf("playAsync end");
			data->sound->close();
			delete data->sound;
			data->sound=NULL;
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

PlayAsync::PlayAsync(int index) {
	this->sound=new Sound();
	// this->sound->init(SND_PCM_NONBLOCK| SND_PCM_ASYNC);
	this->sound->init(SND_PCM_NONBLOCK);
	PlayAsyncData *data = new PlayAsyncData(cfg_funcSound[index], this->sound, 0);

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
	snd_pcm_sw_params_current (this->sound->handle, sw_params);
	snd_pcm_sw_params_set_start_threshold(this->sound->handle, sw_params, buffer_size - period_size);
	snd_pcm_sw_params_set_avail_min(this->sound->handle, sw_params, period_size);
	snd_pcm_sw_params(this->sound->handle, sw_params);
	snd_pcm_sw_params_free (sw_params);

	snd_async_handler_t *ahandler;
	int err=snd_async_add_pcm_handler(&ahandler, this->sound->handle, async_callback, data);
	if (err < 0) {
		printf("Unable to register async handler\n");
		abort();
	}
	err=snd_pcm_prepare(this->sound->handle);
	if (err < 0) {
		printf("prepare error: %s\n", snd_strerror(err));
		abort();
	}

	data->position=this->sound->writeSound(data->data);
	if (snd_pcm_state(this->sound->handle) == SND_PCM_STATE_PREPARED) {
		printf("PlayAsync in PREPARED state\n");
		err = snd_pcm_start(this->sound->handle);
		if (err < 0) {
			printf("Start error: %s\n", snd_strerror(err));
			abort();
		}
	}
};

FahrSound::~FahrSound() {
	printf("FahrSound::~FahrSound()\n");
	this->currFahrstufe=-1;
	this->doRun=false;
	void *ret;
	if(this->thread) {
		pthread_join(this->thread,&ret);
	}
	this->thread=0;
	printf("FahrSound::~FahrSound() done\n");
}

void FahrSound::run() {
	if(!FahrSound::soundFiles) {
		printf("===== no sound files loaded ====\n");
		return;
	}
// FIXME: wenn thread rennt und doRun false is dann warten bis thread tot und neu starten
	if(this->thread) {
		printf("FahrSound::run: already started\n");
		return;
	}
	printf("Sound::run() starting sound thread\n");
	this->doRun=true;

	/* Start a thread and then send it a cancellation request */

	int s = pthread_create(&this->thread, NULL, &sound_thread_func, (void *) this);
	if (s != 0)
		perror("pthread_create");

	usleep(10000);
	this->currFahrstufe=0;
}

void FahrSound::kill() {
	if(this->thread) {
		int s = pthread_cancel(this->thread);
		if (s != 0)
			perror("pthread_cancel");
		void *ret;
		pthread_join(this->thread, &ret);
		this->thread=0;
	}
}

void FahrSound::setSpeed(int speed) {
	if(this->soundFiles) {
		for(int i=0; i < 10; i++) {
			if(speed <= this->soundFiles[i].limit) {
				this->currFahrstufe=i;
				printf("set fahrstufe: %d\n", i);
				return;
			}
		}
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