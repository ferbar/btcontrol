/**
 *  aus alsa demo:
 *  This extra small demo sends a random samples to your speakers.
 *
 *  macht extra sound thread und füttert die soundkarte
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

void Sound::init()
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

	if ((err = snd_pcm_open(&this->handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
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

void Sound::close() {
	if(this->handle) {
		snd_pcm_drain(this->handle); // darauf warten bis alles bis zum ende gespielt wurde
		snd_pcm_close(this->handle);
		this->handle=NULL;
	}
}

FahrSound::~FahrSound() {
	printf("FahrSound::~FahrSound()\n");
	this->currFahrstufe=-1;
	this->doRun=false;
	void *ret;
	pthread_join(this->thread,&ret);
	this->thread=0;
	this->close();
	printf("FahrSound::~FahrSound() done\n");
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

class Reader {
	public:
		Reader(const std::string fileName) {
			this->f=fopen(fileName.c_str(),"r");
			if(!this->f) {
				throw std::runtime_error("error opening " + fileName);
			}

		};
		~Reader() {
			fclose(this->f);
		}
		int32_t ReadInt32() {
			int32_t ret;
			fread(&ret,1,sizeof(ret),this->f);
			return ret;
		}
		int16_t ReadInt16( ) {
			int16_t ret;
			fread(&ret,1,sizeof(ret),this->f);
			return ret;
		}

		int ReadData(int len, std::string &dst) {
			unsigned char *buffer;
			buffer=(unsigned char *) malloc(len);
			if(fread(buffer,1,len,f) != (size_t) len) {
				throw std::runtime_error("error reading file data");
			}
			dst.assign((char*) buffer, len);
			free(buffer);
			return len;
		}

		// void Seek(int skipsize, SeekOrigin::Current ) {
		//  SEEK_SET, SEEK_CUR, or SEEK_END
		void Seek(int skipsize, int a ) {
			abort(); // not yet implemented
		}
	private:
		FILE *f;
};

void Sound::loadWavFile(std::string filename, std::string &out) {
	printf("read file: %s\n", filename.c_str());
	Reader reader(filename);
	int channels=-1;
	int32_t samplerate=-1;
	int bytespersecond=-1;
	int32_t memsize = -1;
	int32_t riffstyle = -1;
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
										int16_t formatblockalign = reader.ReadInt16( );
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
											memsize = reader.ReadInt32( );
											riffstyle = reader.ReadInt32( );
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
	int err;
	printf("Sound::playSingleSound(%d) - done\n",index);

	this->writeSound(cfg_funcSound[index]);
	printf("Sound::playSingleSound() - done\n");
}

void Sound::writeSound(const std::string &data) {
	const char *wavData = data.data();
	int len = data.length();
	int err;

	snd_pcm_sframes_t frames = snd_pcm_writei(this->handle, wavData, len);
	if (frames < 0) { // 2* probieren:
		frames = snd_pcm_recover(this->handle, frames, 0);
	}
	if (frames < 0) { // noch immer putt
		printf("snd_pcm_writei failed: %s\n", snd_strerror(err)); // FIXME !!!
		return;
	}
	if (frames > 0 && frames < (snd_pcm_sframes_t) data.size())
		printf("Short write (expected %zi, wrote %li) %s\n", data.size(), frames, strerror(errno));
}

void FahrSound::run() {
	if(!FahrSound::soundFiles) {
		printf("===== no sound files loaded ====\n");
		return;
	}
// FIXME: wenn thread rennt und doRun false is dann warten bis thread tot und neu starten
	if(this->thread) {
		printf("already started\n");
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

void Sound::setBlocking(bool blocking) {
	
	int rc=snd_pcm_nonblock	(this->handle, blocking);
	if(rc != 0) {
		printf("error setting blocking mode\n");
		abort();
	}
}
