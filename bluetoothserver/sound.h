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
	static void loadWavFile(const std::string &filename, std::string &out);
	static void resampleX2(const std::string &in, std::string &out);

	static void setMasterVolume(int volume);
	void dump_sw() {
		snd_output_t* out;
		snd_output_stdio_attach(&out, stderr, 0);
		snd_pcm_dump_sw_setup(this->handle, out);
	}
	static int sample_rate;
protected:
	// jedes Sound objekt hat eigenes Handle 20150831: am raspi kamma das default device ohne probleme öfters aufmachen
	snd_pcm_t *handle;

	// die wav files solten alle im selben format sein ...
	static snd_pcm_format_t bits;

	friend class PlayAsync;
};

class PlayAsyncData : public Thread {
public:
	PlayAsyncData(const std::string &wav, Sound *sound, int position) : wav(wav), position(position), sound(sound) {};
	const std::string &wav;
	int position;
	Sound *sound;
	void run();
	int index;
};

class PlayAsync {
public:
	PlayAsync(int soundIndex);
	PlayAsync(const std::string &wav);
private:
};

class FahrSound : public Thread {
public:
	// Sound(SoundSet soundSet);
	FahrSound() : doRun(false), currFahrstufe(-1), currSpeed(0) { };
	virtual ~FahrSound();
	void start();
	void cancel();
	void run();
	void setSpeed(int speed);
	void setFahrstufe(int fahrstufe) { this->currFahrstufe = fahrstufe; } ;
	void diOutloop();
	void steamOutloop();


	static void loadSoundFiles(SoundType *soundFiles);
	static void loadSoundFile(const std::string &fileName, std::string &dst);
	static void loadWavFile(std::string filename, std::string &out);

	bool doRun;
	int currFahrstufe; // -1 aus, 0 stop
	int currSpeed;
private:

public:
	static SoundType *soundFiles;
	static bool soundFilesLoaded; // in soundFiles stehen zuerst nur die dateinamen

};
#endif
