#ifndef SOUND_H
#define SOUND_H
#include <alsa/asoundlib.h>
#include "zsp.h"

class Sound {
public:
	Sound() : handle(NULL) {} ;
	virtual ~Sound();
	void init();
	void close();

	void setBlocking(bool blocking);
	void playSingleSound(int index);

	void writeSound(const std::string &data);

	static void loadSoundFiles(SoundType *soundFiles);
	static void loadSoundFile(const std::string &fileName, std::string &dst);
	static void loadWavFile(std::string filename, std::string &out);

private:
	// jedes Sound objekt hat eigenes Handle 20150831: am raspi kamma das default device ohne probleme öfters aufmachen
	snd_pcm_t *handle;

	// die wav files solten alle im selben format sein ...
	static snd_pcm_format_t bits;
	static int sample_rate;
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


	static void loadSoundFiles(SoundType *soundFiles);
	static void loadSoundFile(const std::string &fileName, std::string &dst);
	static void loadWavFile(std::string filename, std::string &out);

private:
	static pthread_t thread;
	static bool doRun;

	static int currFahrstufe; // -1 aus, 0 stop

public:
	static SoundType *soundFiles;
	static bool soundFilesLoaded;

};
#endif
