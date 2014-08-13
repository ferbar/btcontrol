#include "zsp.h"
#include <alsa/asoundlib.h>

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
	void loadSoundFiles();
	void loadSoundFile(const std::string &fileName, std::string &dst);
private:
	static pthread_t thread;
	SoundType *soundFiles;
	int currFahrstufe; // -1 aus, 0 stop
	static snd_pcm_t *handle;
	snd_pcm_format_t bits;
	int sample_rate;
	bool doRun;
};
