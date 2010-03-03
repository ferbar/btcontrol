/***************************************************************************
 *   Copyright (C) 2004 by Nicolas Sutre                                   *
 *   nicolas.sutre@free.fr                                                 *
 *                                                                         *
 *   Copyright (C) 2005 by Bob Dempsey                                     *
 *   bdempsey_64@msn.com                                                   *
 *                                                                         *
 *   Copyright (C) 2005 by Julien Etelain and Edward Nys                   *
 *   Converted to C                                                        *
 *   Commented and improved by Julien Etelain <julien.etelain@utbm.fr>     *
 *                             Edward Nys <edward.ny@utbm.fr>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>

#include "k8055.h"

#define STR_BUFF 256
#define false 0
#define true 1

#define CMD_INIT		0x00
#define CMD_SETDBT1		0x01
#define CMD_SETDBT2		0x02
#define CMD_RESETCNT1   0x03
#define CMD_RESETCNT2   0x04
#define CMD_SETOUTPUT   0x05

int cfg_ia1 = 0;
int cfg_ia2 = 0;
int cfg_id8 = 0;
int cfg_ipid = 1;
int cfg_numread = 1;
bool cfg_debug = false;
int cfg_delay = 0;
int cfg_dbt1 = -1; // (-1 => not to set)
int cfg_dbt2 = -1; // (-1 => not to set)

int cfg_resetcnt1 = false;
int cfg_resetcnt2 = false;

/*
	Convert a string on n chars to an integer
	Return  1 on sucess
			0 on failure (non number)
*/
int Convert_StringToInt(char *text, int *i)
{
	return sscanf(text, "%d", i);
}

/*
	Write help to standard output
*/
void display_help ( char *params[] ) {

	printf("K8055 version 0.3 UTBM Build\n");
	printf("Copyright (C) 2004 by Nicolas Sutre\n");
	printf("Copyright (C) 2005 by Bob Dempsey\n");
	printf("Copyright (C) 2005 by Julien Etelain and Edward Nys\n");
	printf("\n");
	printf("Syntax : %s [-P:(number)] [-D:(value)] [-A1:(value)] [-A2:(value)] [-NUM:(number) [-DELAY:(number)]]\n",params[0]);
	printf(" [-DBT1:(value)] [-BDT2:(value)] [-reset1] [-reset2] [-debug]\n");
	printf("	-P:(number)		Set board number\n");
	printf("	-D:(value)		Set digital output value (8 bits in decimal)\n");
	printf("	-A1:(value)		Set analog output 1 value (0-255)\n");
	printf("	-A2:(value)		Set analog output 2 value (0-255)\n");
	printf("	-NUM:(number)   Set number of measures\n");	
	printf("	-DELAY:(number) Set delay between two measures (in msec)\n");		
	printf("	-DBT1:(value)   Set debounce time for counter 1 (in msec)\n");	
	printf("	-DBT2:(value)   Set debounce time for counter 2 (in msec)\n");	
	printf("	-reset1			Reset counter 1\n");
	printf("	-reset2			Reset counter 2\n");
	printf("	-debug			Activate debug mode\n");
	printf("Exemple : %s -P:1 -D:147 -A1:25 -A2:203\n",params[0]);
	printf("\n");
	printf("Output : (timestamp);(digital);(analog 1);(analog 2);(counter 1);(counter 2)\n");
	printf("Note : timestamp is the number of msec when data is read since program start\n");
	printf("Example : 499;16;128;230;9;8\n");
	printf("499 : Measure done 499 msec after program start\n");
	printf("16  : Digital input value is 10000 (I5=1, all other are 0)\n");
	printf("128 : Analog 1 input value is 128\n");
	printf("230 : Analog 2 input value is 230\n");
	printf("9   : Counter 1 value is 9\n");
	printf("8   : Counter 2 value is 8\n");

}


/*
	Read arguments, and store values
	Return true if arguments are valid
		else return false
*/
int read_param(int argc,char *params[])
{ 
	int erreurParam = false;
	int i;
  	
	cfg_ipid = 1;
  		
	for (i=1; i<argc;i++)
	{
		if ( !strncmp(params[i],"-P:",3) &&
		     !Convert_StringToInt(params[i]+3,&cfg_ipid) ) erreurParam = true;
		else
			if ( !strncmp(params[i],"-A1:",4)  &&
		    	!Convert_StringToInt(params[i]+4,&cfg_ia1) ) erreurParam = true;
		else
			if ( !strncmp(params[i],"-A2:",4) &&
		    	!Convert_StringToInt(params[i]+4,&cfg_ia2) ) erreurParam = true;
		else
			if ( !strncmp(params[i],"-D:",3) &&
		    	!Convert_StringToInt(params[i]+3,&cfg_id8) ) erreurParam = true;
		else
			if ( !strncmp(params[i],"-NUM:",5) &&
		    	!Convert_StringToInt(params[i]+5,&cfg_numread) ) erreurParam = true;	
		else
			if ( !strncmp(params[i],"-DELAY:",7) &&
		    	!Convert_StringToInt(params[i]+7,&cfg_delay) ) erreurParam = true;					
		else
			if ( !strncmp(params[i],"-DBT1:",6) &&
		    	!Convert_StringToInt(params[i]+6,&cfg_dbt1) ) erreurParam = true;						
			else
			if ( !strncmp(params[i],"-DBT2:",6) &&
		    	!Convert_StringToInt(params[i]+6,&cfg_dbt2) ) erreurParam = true;											
		else
			if ( !strcmp(params[i],"-debug") ) cfg_debug = true;
		else
			if ( !strcmp(params[i],"-reset1") ) cfg_resetcnt1 = true;
		else
			if ( !strcmp(params[i],"-reset2") ) cfg_resetcnt2 = true;					
		else
			if ( !strcmp(params[i],"--help") || !strcmp(params[i],"-h") ) {
				display_help(params);
				return false;
			}

	}
	
	
	/*
		Send parameters to standard error
	*/
	if ( cfg_debug )
		fprintf(stderr,"Parameters : Card=%d Analog1=%03d Analog2=%03d Digital=%03d\n",cfg_ipid,cfg_ia1,cfg_ia2,cfg_id8);
	
		
	if (erreurParam) 
	{

		printf("Invalid or incomplete options\n");
		
		display_help(params);
		return false;
	} 


	return true; 
}

/*
	Function to convert msec to the DebounceTime encoding
	ATTENTION : Empirical values
*/
int msec_to_dbt_code ( int msec ) {

	if (msec < 2)
		return 1;
	else if (msec < 10)
		return 3;
	else if (msec < 1000)
		return 8;
	else if (msec < 5000)
		return 88;
	else
		return 255;
}


/*
	Give current timestamp in miliseconds
*/
inline unsigned long int time_msec ( void ) {
	struct timeval t; struct timezone tz;
	gettimeofday (&t,&tz);
	return (1000*t.tv_sec)+(t.tv_usec/1000);
}


int main (int argc,char *params[]) 
{
	int i;
	char c;
	unsigned char a1, a2, d;
	unsigned short c1, c2;
	unsigned long int start,mstart=0,lastcall=0;


	start = time_msec();

	/*
		Load parameters
		If parameters are valid continue
	*/
	
	if (read_param(argc,params)) 
	{
		K8055 dev(cfg_ipid,cfg_debug);

				
		if ( cfg_resetcnt1 )
			dev.reset_counter1();
		if ( cfg_resetcnt2 )
			dev.reset_counter2();

		if ( cfg_dbt1 != -1 )
			dev.set_counter1_bouncetime(cfg_dbt1);
			
		if ( cfg_dbt2 != -1 )
			dev.set_counter2_bouncetime(cfg_dbt2);
			
		mstart = time_msec(); // Measure start
		for (i=0; i<cfg_numread; i++) {
			
			if ( cfg_delay ) {
				// Wait until next measure
				while ( time_msec()-mstart < i*cfg_delay );
			} 
			dev.read_input ( &a1, &a2, &d, &c1, &c2 );
			lastcall = time_msec();
			printf("%d;%d;%d;%d;%d;%d\n", (int)(lastcall-start),d, a1,a2,c1,c2 );
		}
		
		dev.write_output(cfg_ia1, cfg_ia2, cfg_id8);
	}
}
