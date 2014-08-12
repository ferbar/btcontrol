/*
 *  This file is part of btcontrol
 *
 *  Copyright (C) Christian Ferbar
 *
 *  btcontrol is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontrol is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontrol.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <qrencode.h>
#include <unistd.h>
#include <errno.h>
#include "qrcode.h"


void printQRCode(const char *url) {

	QRcode *qrcode = QRcode_encodeString("http://localhost/btctl.apk", /*version*/0, QR_ECLEVEL_M, /*hint*/ QR_MODE_8, /*casesensitive*/ 1 );
	printf("qrcode: %p\n", qrcode);
	if(qrcode) {
		// const char *BYTES[] = {" ", /*50%oben*/ "\xE2\x96\x80", /*50%unten*/ "\xE2\x96\x84", /*100%*/ "\xE2\x96\x88"};
		const char *BYTES_INVERTED[] = {/*100%*/ "\xE2\x96\x88", /*50%unten*/ "\xE2\x96\x84", /*50%oben*/ "\xE2\x96\x80", " "};
		printf("version: %d, width: %d\n\n",qrcode->version, qrcode->width);
		for(int x=0; x < qrcode->width+2; x++) printf(BYTES_INVERTED[1]); printf("\n");
		for(int y=0; y < qrcode->width; y++) {
			printf(BYTES_INVERTED[0]);
			for(int x=0; x < qrcode->width; x++) {
				int val=qrcode->data[y*qrcode->width + x] & 1;
				if((y+1) < qrcode->width) {
					val|=(qrcode->data[(y+1)*qrcode->width + x] & 1) << 1; // nächste zeile 
				}
					// 1=black/0=white
				printf(BYTES_INVERTED[val]);
			}
			y++;
			printf(BYTES_INVERTED[0]);
			printf("\n");
		}
		for(int x=0; x < qrcode->width+2; x++) printf(BYTES_INVERTED[2]); printf("\n");
		QRcode_free(qrcode);
		printf("\n");
	} else {
		printf("error: %m\n"); // %m = strerror(errno) ohne argument
		exit(1);
	}

	return;
}

