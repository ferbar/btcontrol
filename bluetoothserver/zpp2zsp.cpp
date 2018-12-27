#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#include <sys/stat.h>
#include <sys/types.h>


#include <sndfile.h>

#include <iostream>
#include <fstream>
#include <errno.h>

#define	BUFFER_LEN			4096

std::string baseFolder;

static void write_wav_file(const std::string &wavData, const char *outfilename)
{
	// static float buffer [BUFFER_LEN] ;

	SNDFILE		*outfile ;
	SF_INFO		sfinfo ;
	// int			k, readcount ;

	printf ("    -> %s ", outfilename) ;
	fflush (stdout) ;

	// k = 16 - strlen (outfilename) ;

	memset (&sfinfo, 0, sizeof (sfinfo)) ;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_U8 ;
	sfinfo.channels = 1;
	sfinfo.samplerate=22050;

	if (! sf_format_check (&sfinfo)) {
		printf ("Invalid encoding\n") ;
		return ;
	}

	if (! (outfile = sf_open (outfilename, SFM_WRITE, &sfinfo))) {
		printf ("Error : could not open file : %s\n", outfilename) ;
		puts (sf_strerror (NULL)) ;
		exit (1) ;
	}

	// while ((readcount = sf_read_float (infile, buffer, BUFFER_LEN)) > 0)
	sf_write_raw(outfile, wavData.data(), wavData.length()) ;

	sf_close (outfile) ;

	printf ("ok\n") ;

} /* encode_file */

int main(int argc, char *argv[]) {
	if(argc != 2) {
		printf("Error: %s <zpp-file>\n", argv[0]);
		exit(1);
	}

	if(! utils::endsWith(argv[1],".zpp")) {
		printf("Error: expected zpp file\n");
		exit(1);
	}

	baseFolder=argv[1];
	baseFolder=baseFolder.substr(0, baseFolder.length()-4);
	if(utils::isDir(baseFolder.c_str())) {
		printf("Warning: overwriting existing dir\n");
	} else {
		if(mkdir(baseFolder.c_str(), 0777) != 0) {
			printf("Error: creating basedir\n");
			exit(1);
		}
		printf("created %s\n", baseFolder.c_str());
	}

	//std::string zpp=readFile("tests/BR52.zpp");
	std::string zpp=readFile(argv[1]);
	// std::string zpp=readFile("/home/chris/downloads/lgb/zimo/share/test_neu/test_2.zpp");
	// parse header:
	if(zpp.length() < 0x80) {
		printf("Error: invalid header\n");
		return 1;
	}
	// check magick:
	if(zpp[0] != 'S' || zpp[1] != 'P') {
		printf("Error: invalid magic\n");
		return 1;
	}
	printf("Version: %d\n", zpp[3]);
	int headerSize=(unsigned char) zpp[8];
	if(headerSize != 0x80) {
		printf("Error: header size != 0x80\n");
		return 1;
	}

	std::fstream zspFile;
	zspFile.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
	try {
	zspFile.open(baseFolder + "/generated.zsp", std::ios_base::out);

	std::string data=zpp.substr(headerSize);
	// Ablauf einlesen
	const char* ablNames[] = {"Sieden", "??", "Bremsquietschen", "??", "Entwässern", "Anfahrpfiff","??", "??", "??", "??"};
	for(int i = 0; i < 10; i++) {
		int offset=0xc40 + 3;
		int address=offset+i*2;
		// b5 => -3db
		// 80 => -6db
		// 5b => -9db
		// 40 => -12db
		printf("Abl: %02x %02x (%s)\n", (unsigned char) data[address], (unsigned char) data[address+1], ablNames[i] );
	}
	
	// SAMPLE einlesen
	for(int i = 0; i < 128; i++) {
		// printf("%02x%02x:%02x%02x ", (unsigned char) data[i*4], (unsigned char) data[i*4+1], (unsigned char) data[i*4+2], (unsigned char) data[i*4+3]);
		int address=(((unsigned char) data[i*4+1]) << 16) + 
			(((unsigned char) data[i*4+2]) << 8) +
			((unsigned char) data[i*4+3]);
		if(address > 0 && address < (signed) data.length()) {
			printf("%02x address: %06x ", (unsigned char) data[i*4], address);
			printf("data: %02x%02x:%02x%02x ", (unsigned char) data[address], (unsigned char) data[address+1], (unsigned char) data[address+2], (unsigned char) data[address+3]);
			printf(" %02x%02x:%02x%02x ", (unsigned char) data[address+4], (unsigned char) data[address+5], (unsigned char) data[address+6], (unsigned char) data[address+7]);
			printf(" %02x%02x:%02x%02x ", (unsigned char) data[address+8], (unsigned char) data[address+9], (unsigned char) data[address+10], (unsigned char) data[address+11]);
			printf(" %02x%02x:%02x%02x ", (unsigned char) data[address+12], (unsigned char) data[address+13], (unsigned char) data[address+14], (unsigned char) data[address+15]);

			printf("%02x ", (unsigned char) data[address]);
#define ZPP_FLAG_LOOP  0x08
#define ZPP_FLAG_SHORT 0x40
			int flags=(unsigned char) data[address];
			if(flags & ZPP_FLAG_LOOP) {
				printf("L"); // loop flag
			} else {
				printf(" ");
			}
			if(flags & ZPP_FLAG_SHORT) {
				printf("S"); // short flag
			} else {
				printf(" ");
			}
			printf(" ");
			int wavEnd=(((unsigned char) data[address+3]) << 16) +
				(((unsigned char) data[address+2]) << 8) +
				((unsigned char) data[address+1]);
			int wavLoopStartAddr=(((unsigned char) data[address+6]) << 16) +
				(((unsigned char) data[address+5]) << 8) +
				((unsigned char) data[address+4]);
			int wavLoopEndAddr=(((unsigned char) data[address+9]) << 16) +
				(((unsigned char) data[address+8]) << 8) +
				((unsigned char) data[address+7]);
			if(wavLoopStartAddr != address +0x0a || wavLoopEndAddr != wavEnd) {
				printf("loop %06x-%06x ", wavLoopStartAddr, wavLoopEndAddr);
			} else {
				printf("                   ");
			}
			printf("wav end: %06x ", wavEnd);
			std::string infoName;
			if(wavEnd < (signed) data.length()) {
				printf("%02x%02x ", (unsigned char) data[wavEnd+1], (unsigned char) data[wavEnd+2]);
				int infoLen=(unsigned char) data[wavEnd+3];
				infoName=data.substr(wavEnd+4, infoLen);
				/*
				printf("%d ", infoLen);
				for(int j=0; j<infoLen; j++) {
					printf("%c", data[wavEnd+4+j]);
				}
				*/
				if(infoLen == 0) {
					infoName=utils::format("%d",i);
				}
				printf("%s ", infoName.c_str());
			} else {
				infoName=utils::format("no-name%d",i);
			}
			printf("\n");
			int wavLen=wavEnd-(address+0x0a)+1;
			int wavLoopStart=wavLoopStartAddr-(address+0x0a);
			int wavLoopEnd=wavLoopEndAddr-(address+0x0a);

			write_wav_file(data.substr(address+0x0a, wavLen), (baseFolder + "/" + infoName+".wav").c_str());

			zspFile 
				<< "\"Sample\"\n"
				<< "\"NUMMER\"," << i << "\n"
				<< "\"PFAD\",\"\"\n"
				<< "\"NAME\",\"" << infoName << ".wav\"\n"
				<< "\"ART\",255\n"
				<< "\"SIZE\"," << wavLen << "\n"
				<< "\"L1\"," << wavLoopStart << "\n"
				<< "\"L2\"," << wavLoopEnd << "\n"
				<< "\"Event1Pos\",0\n"
				<< "\"Event1Typ\",0\n"
				<< "\"Event1Akt\",0\n"
				<< "\"Event2Pos\"," << wavLoopEnd << "\n"
				<< "\"Event2Typ\",0\n"
				<< "\"Event2Akt\",0\n"
				<< "\"SR\",2\n"
				<< "\"INFO\",\"" << infoName << "\"\n"
				<< "\"LOOP\","  << (flags & ZPP_FLAG_LOOP  ? 1 : 0) << "\n"
				<< "\"SHORT\"," << (flags & ZPP_FLAG_SHORT ? 1 : 0) << "\n"
				<< "\"FNR\",70\n"
				<< "\"/Sample\"\n" ;

		} else if(address != 0) {
			printf("address %06x out of range!\n", address);
		}
	}
	zspFile.close();
	} catch (const std::ifstream::failure& e) {
		printf("Error: ifstream failure %s", strerror(errno));
	}
	return 0;
}
