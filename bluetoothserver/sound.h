#ifndef SOUND_H
#define SOUND_H
#include <alsa/asoundlib.h>
#include "zsp.h"
#include "Thread.h"

class PlayAsync;

class Sound {
public:
	Sound() : handle(NULL) {} ;
	virtual ~Sound();
	void init(int mode=0);
	void close(bool waitDone=true);

	void setBufferSize(int frames);
	void setBlocking(bool blocking);
	void playSingleSound(int index);

	int writeSound(const std::string &data, int startpos=0);

	static void loadSoundFiles(SoundType *soundFiles);
	static void loadSoundFile(const std::string &fileName, std::string &dst);
	static void loadWavFile(std::string filename, std::string &out);

	static void setMasterVolume(int volume);
	void dump_sw() {
		snd_output_t* out;
		snd_output_stdio_attach(&out, stderr, 0);
		snd_pcm_dump_sw_setup(this->handle, out);
	}
protected:
	// jedes Sound objekt hat eigenes Handle 20150831: am raspi kamma das default device ohne probleme öfters aufmachen
	snd_pcm_t *handle;

	// die wav files solten alle im selben format sein ...
	static snd_pcm_format_t bits;
	static int sample_rate;

	friend class PlayAsync;
};

class PlayAsyncData : public Thread {
public:
	PlayAsyncData(const std::string &data, Sound *sound, int position) : data(data), position(position), sound(sound) {};
	const std::string &data;
	int position;
	Sound *sound;
	void run();
	int index;
};

class PlayAsync {
public:
	PlayAsync(int soundIndex);
private:
};

class FahrSound : public Sound {
public:
	// Sound(SoundSet soundSet);
	FahrSound(SoundType *soundFiles) { };
	virtual ~FahrSound();
	void run();
	void kill();
	void setSpeed(int speed);
	void setFahrstufe(int fahrstufe) { this->currFahrstufe = fahrstufe; } ;
	void outloop();
	void diOutloop();
	void dOutloop();
	void steamOutloop();


	static void loadSoundFiles(SoundType *soundFiles);
	static void loadSoundFile(const std::string &fileName, std::string &dst);
	static void loadWavFile(std::string filename, std::string &out);

private:
	static pthread_t thread;
	static bool doRun;

	static int currFahrstufe; // -1 aus, 0 stop
	static int currSpeed;
public:
	static SoundType *soundFiles;
	static bool soundFilesLoaded;

};
#endif
