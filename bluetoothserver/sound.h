#ifndef SOUND_H
#define SOUND_H
#include <alsa/asoundlib.h>
#include "zsp.h"

class Sound {
public:
	// Sound(SoundSet soundSet);
	Sound(SoundType *soundFiles);
	~Sound();
	void init();
	void run();
	void kill();
	void setSpeed(int speed);
	void setFahrstufe(int fahrstufe) { this->currFahrstufe = fahrstufe; } ;
	void addSound(const char soundfile);
	void outloop();
	static void loadSoundFiles(SoundType *soundFiles);
	static void loadSoundFile(const std::string &fileName, std::string &dst);
	static void loadWavFile(std::string filename, std::string &out);

private:
	static pthread_t thread;
	static bool doRun;
	static SoundType *soundFiles;
	static bool soundFilesLoaded;
	static snd_pcm_t *handle;
	static int currFahrstufe; // -1 aus, 0 stop

	// die wav files solten alle im selben format sein ...
	static snd_pcm_format_t bits;
	static int sample_rate;
};

#endif
