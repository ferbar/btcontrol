/**
 *
 *
 * 0x80 Bytes header
 * pointer tables auf die eigentlichen infos (2 byte)
 * pointer auf die sounds (3 bytes)
 *
 * zum binary files vergleichen: vbindiff
 *
 */
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

// strings sind immer in iso8859-1
#include <boost/locale.hpp>

#define	BUFFER_LEN			4096

std::string baseFolder;

const char *soundTypeToString(unsigned char soundType) {
	switch(soundType) {
		case 1: return "Pfeife kurz";
		case 2: return "Pfeife lang";
		case 3: return "Glocke";
		case 4: return "Kohlenschaufeln";
		case 5: return "Injektor";
		case 6: return "Luftpumpe";
		case 8: return "Schaffnerpfiff";
		case 0xa: return "Kupplung";
		case 0xb: return "Generator";
		case 0x80: return "Sieden";
		case 0x82: return "Bremsenquietschen";
		case 0xff: return "Dampfschlag";
		default: return "unknown";
	}
}

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
	std::string zspFilename=baseFolder + "/generated.zsp";
	zspFile.open(zspFilename, std::ios_base::out);

	std::string data=zpp.substr(headerSize);

	// SAMPLE einlesen =========================================================================
	for(int i = 0; i < 128; i++) {
		// printf("%02x%02x:%02x%02x ", (unsigned char) data[i*4], (unsigned char) data[i*4+1], (unsigned char) data[i*4+2], (unsigned char) data[i*4+3]);
		int address=(((unsigned char) data[i*4+1]) << 16) + 
			(((unsigned char) data[i*4+2]) << 8) +
			((unsigned char) data[i*4+3]);
		unsigned char soundType=(unsigned char) data[i*4];
		printf("[%02x] type: %02x (%s) address: %06x ", i+1, soundType, soundTypeToString(soundType),  address);
		if(address > 0 && address < (signed) data.length()) {
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
				} else {
					// infoName in iso
					infoName = boost::locale::conv::to_utf<char>(infoName,"Latin1");
				}
				printf("Info: '%s' ", infoName.c_str());
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
				<< "\"ART\"," << (int) soundType << "\n"
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
			// printf("[%02x] type: %02x (%s) address: %06x out of range!!!!", i, soundType, soundTypeToString(soundType), address);
			printf("out of range!!!!\n");
		} else {
			printf("skipped\n");
		}
	}

	// Ablauf einlesen ==================================================
	/* pro Eintrag ein Objekt im ZSP, für jede lok eine Ablaufliste
"Abl"
"LOK",0
"NUMMER",0
"SAMPLE",2
"LAUTST",7
"/Abl"
	*/

	const char* ablNames[] = {"Sieden", "Glocke", "Bremsquietschen", "Entwässern", "Anfahrpfiff", "??", "E-Motor", "??", "Schaltwerk", "??", "??", "??", "??", "??",
		"E Bremse", "Kurve", "??", "??", "??"};
	int nablItems=sizeof(ablNames) / sizeof(char*);
	for(int addrDampE=0; addrDampE <= 1; addrDampE++)   // dampf Abl fängt bei 920 an, E bei 960
	for(int i = 0; i < 32; i++) {
		printf("Abl Set %d\n",i);
		int offset=(data[0x920-0x80+i*2+addrDampE*0x40] << 8) + data[0x920 - 0x80+i*2+addrDampE*0x40 + 1];
//		int offset=(data[0x960-0x80+i*2] << 8) + data[0x960 - 0x80+i*2 + 1];
		if(offset == 0)
			break;
		offset+=0x7b;
		// for(int j=0; j < 42; j++) printf("[%d] %02x ", j, data[offset + j]); printf("\n");
		for(int j = 0; j < nablItems; j++) {
			int address=offset+j*2;
			// printf("Abl Set %d [%d] Address: %#0x\n", i, j, address + 0x80);
			// printf("Address: %#0x\n", address+0x80);
		// b5 => -3db
		// 80 => -6db
		// 5b => -9db
		// 40 => -12db
		//
		// 2E => -15db
		// 17 => -21db
			unsigned char id=(unsigned char) data[address];
			unsigned char soundLevel=(unsigned char) data[address+1];
			printf("Abl Set %d [%d] : id:%02x level:%02x (%s)\n", i, j, id, soundLevel, ablNames[j] );
			zspFile
				<< "\"Abl\"\n"
				<< "\"LOK\"," << i << "\n"
				<< "\"NUMMER\"," << j << "\n"
				<< "\"SAMPLE\"," << (int) id << "\n"
				<< "\"LAUTST\"," << (int) soundLevel << "\n"
				<< "\"/Abl\"\n";
		}
	}

	// Func: ==============================================================
	// vielleicht addr pointer @920-0x80

	// DiSets: =============================================================
	for(int i = 0; i < 32; i++) {
		printf("DiSet [%d]\n",i);
		int offset=0x840;
		int address=(data[offset+i*2] << 8) + data[offset+i*2+1];
		printf("Address: %#0x\n", address + 0x80);
		if(address == 0)
			break;

		for(int j=0; j < 42; j++) {
			printf("[%d] %02x ", j, data[address + j]);
		}
		printf("\n");
		int stufen=data[address+0]; // [0] oder [1]
		int typ=data[address+1]; // [0] oder [1]
		printf("stufen: %d, typ: %d, Mstart: %d, stand: %d, Mstop: %d\nstand->F1: %d, F1: %d, F1->stand: %d\n",
			stufen, typ,
			data[address+5], data[address+6], data[address+7],
			data[address+8], data[address+9], data[address+10]);
		zspFile
			<< "\"DiSet\"\n"
			<< "\"NUMMER\"," << i << "\n"
			<< "\"NAME\",\"unknown-" << i << "\"\n"
			<< "\"STUFEN\"," << stufen << "\n"   // stufe 1 wird im programm mit '2' angezeigt
			<< "\"TYP\"," << typ << "\n"
			<< "\"SAMPLE\",0," << (int) data[address+5] << "\n"
			<< "\"SAMPLE\",1," << (int) data[address+6] << "\n"
			<< "\"SAMPLE\",2," << (int) data[address+7] << "\n"
			<< "\"SAMPLE\",3," << (int) data[address+8] << "\n"
			<< "\"SAMPLE\",4," << (int) data[address+9] << "\n"
			<< "\"SAMPLE\",5," << (int) data[address+10] << "\n";



		if(stufen > 1) {
			unsigned char schwelle=data[address+39];
			printf("F2 Schwelle: %u, F1>F2: %d, F2: %d, F2->F1: %d\n",
				schwelle,
				data[address+11], data[address+12], data[address+13]);
			zspFile
				<< "\"SCHWELLE\",1," << (int) schwelle << ",64\n"
				<< "\"SAMPLE\",6," << (int) data[address+11] << "\n"
				<< "\"SAMPLE\",7," << (int) data[address+12] << "\n"
				<< "\"SAMPLE\",8," << (int) data[address+13] << "\n";
		}
		if(stufen > 2) {
			unsigned char schwelle=data[address+39];
			printf("F3 Schwelle: %u, F2>F3: %d, F3: %d, F3->F2: %d\n",
				schwelle,
				data[address+14], data[address+15], data[address+16]);
			zspFile
			<< "\"SCHWELLE\",2," << (int) schwelle << ",32\n"
				<< "\"SAMPLE\",9," << (int) data[address+14] << "\n"
				<< "\"SAMPLE\",10," << (int) data[address+15] << "\n"
				<< "\"SAMPLE\",11," << (int) data[address+16] << "\n";
		}
/*
			<< "\"SCHWELLE\",3,255,21
			<< "\"SCHWELLE\",4,255,16
			<< "\"SCHWELLE\",5,255,13
			<< "\"SCHWELLE\",6,255,11
			<< "\"SCHWELLE\",7,255,9
			<< "\"SCHWELLE\",8,255,8
			<< "\"SCHWELLE\",9,255,7
			<< "\"SCHWELLE\",10,255,6
			*/
		zspFile
			<< "\"PANTOAUS\",0\n"
			<< "\"HG_SCHALTWERK\",0\n"
			<< "\"/DiSet\"\n";
	}

	// DSets: =============================================================
	// laut programm 1 ... 32 Plätze, name dürfte nicht vorkommen
	for(int i = 0; i < 32; i++) {
		printf("DSet [%d]\n",i);
		int offset=0x800;
		int address=(data[offset] << 8) + data[offset+1];
		printf("Address: %#0x\n", address + 0x80);
		if(address == 0)
			break;
		int pos=0;
		unsigned char schlaege=data[address + pos++];
		printf("Schläge: %d\n", schlaege);
		unsigned delay[11]; 
		// 140=D7
		// 150=D4 120=DD
		int ndelay=0;
		while(true) {
			unsigned char d=data[address + pos++];
			if(d==0xff) {
				delay[ndelay]=0;
				break;
			}
			printf("delay=%d\n", d);
			delay[ndelay++]=d;
			if(ndelay >=10) {
				printf("too many anz stufen???\n"); // max im programm ist 10
				abort();
			}
		}
		printf("ndelay=%d\n", ndelay);
		zspFile
			<< "\"DSet\"\n"
			<< "\"NUMMER\"," << i << "\n"
			<< "\"NAME\",\"unknown-" << i << "\n"
			<< "\"STUFEN\"," << (ndelay) << "\n"   // stufe 1 wird im programm mit '2' angezeigt
			<< "\"SLOTS\"," << (int) (schlaege) << "\n";  // schläge 3 wird im programm mit '4' angezeigt
		for(int j=0; j <= ndelay; j++) {
			unsigned char first_id_h=data[address + pos++];
			unsigned char first_id_l=data[address + pos++];
			unsigned char first_id_m=data[address + pos++];
			
			printf("h: %d m: %d l: %d\n", first_id_h, first_id_l, first_id_m);
			for(int s=0; s <= schlaege; s++)
				zspFile << (first_id_h+s) << "\n";
			for(int s=0; s <= schlaege; s++)
				zspFile << (first_id_m+s) << "\n";
			for(int s=0; s <= schlaege; s++)
				zspFile << (first_id_l+s) << "\n";
			switch(delay[j]) {
				case 0xDD: zspFile << "120\n"; break;
				case 0xD7: zspFile << "140\n"; break;
				case 0xD4: zspFile << "150\n"; break;
				case 0: zspFile << "0\n"; break;
				default: printf("unknown delay! (%d)\n",delay[j]); abort(); 
			}
		}
		unsigned char n1=data[address + pos++];
		unsigned char n2=data[address + pos++];
		if((n1 != 0) || (n2 != 0)) { printf("error end not found: %#02x %#02x @%#02x\n",n1,n2,address + pos + 0x80 -2); abort(); }
			
		zspFile
			<< "\"PANTOAUS\",0\n"
			<< "\"/DSet\"\n";

	}

	zspFile.close();
	printf("Written file %s\n", zspFilename.c_str());
	} catch (const std::ifstream::failure& e) {
		printf("Error: ifstream failure %s", strerror(errno));
	}
	return 0;
}
