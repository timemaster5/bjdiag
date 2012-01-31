/*
 * Copyright (c) 2012 by Michal Altmann <timemaster@trillinis.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "include.h"
//serial link settings
void serline(){
  fd = open (xSerDev, O_RDWR | O_NOCTTY | O_NONBLOCK ); 
  if (fd <0) {perror(xSerDev); exit(-1); }
  tcgetattr(fd, &oldtio);
  fcntl( fd, F_SETFL, O_SYNC); 

  // serial link parameters
  memset( &newtio, 0, sizeof( newtio ));
  newtio.c_cflag = (  CS8 | CLOCAL | CREAD);
  newtio.c_iflag = (IGNPAR);
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 1;
  newtio.c_cc[VTIME] = 1;
  newtio.c_ispeed = BAUDRATE;
  newtio.c_ospeed = BAUDRATE;
  tcsetattr( fd, TCSANOW, &newtio );
  tcflush( fd, TCIOFLUSH );
}


char *sendcommand(char *ggt){
  int cnt=0;
  bzero(msg,BUFSIZE);
  tosend = strdup(ggt);
  strcat(tosend, "\r\n");
  res = write (fd, tosend, strlen(tosend));

  if( res < 0 ){
    perror( "Write" );
    exit(1);
  }
  bzero(msg,BUFSIZE);
  while (c!='\r'){ 
    if (read(fd,&c,1)>0)        
    strncat(msg, &c, 1);
     if (debug) printf("DBG: Characters readed: %d\n",cnt);  
    cnt+=1;
    if (cnt > BUFSIZE ) {
      if (debug) fprintf(stderr,"DBG: ERROR! increase BUFSIZE and recompile\n"); 
      ret=2;
      break;
    }  
  }
  if (!strcmp(msg,"*B10\r")) { //if good anser
    bzero(msg,BUFSIZE);  
    strcat(msg, "OK");
  }
  return msg; 
} 


// usage screen -h
void usage(){
  
  printf ("Usage:  bjdiag [ -s path ] COMMAND\n\n");

  printf ("  -s:  serial port path,   default is: %s\n",xSerDev);
    printf ("COMMANDS\n");
  printf ("  -c:  your own command\n");
  
    printf ("OUTPUT\n");
    printf ("  -d:   debug mode\n");
  printf ("  -?,-h,--help:    print this message\n");

  return;
}


void tm_start(){ 
  clock_gettime(CLOCK_MONOTONIC,&t1);
  run=1;
  return;
}

double tm_stop(){
    if(run) {
  clock_gettime(CLOCK_MONOTONIC, &t2);
  run=0;
  return ((double)(t2.tv_sec - t1.tv_sec) + 1.e-9*(t2.tv_nsec - t1.tv_nsec));
    }else return 0;
}

void clean(){
  if (debug) printf("DBG: answer: %s\n",msg);
  bzero(msg,BUFSIZE); //clear mesage buffer
}

int showbug(char num[]){
   char str[200];
   FILE *fp;
 
   fp = fopen(defFile, "r");
   if(!fp) {printf("je to v riti\n"); return 1;} // bail out if file not found
   while(fgets(str,sizeof(str),fp) != NULL)
   {
     // strip trailing '\n' if it exists
     int len = strlen(str)-1;
     if(str[len] == '\n') 
        str[len] = 0;
	char*    del = "\t";
	char* record = strtok (str, del);
	int index=0;
 	while (record) {
		if (!strcmp(record,num)) {printf("Cislo chyby: %s\n",record);index=1;} 
		if (index==2) printf("Chyba: %s \n",record);
		if (index==3) printf("Predpokladana pricina: %s\n",record);
		if (index) index++;
		record = strtok(NULL, del);
   		}
   }
   fclose(fp);
return 0;
}

void bugs(int code){
  bugcodes[idx]=code; //save bugcode
  char bugnum[3];
  if (debug) printf("DBG: Saving %i to the index %i\n",bugcodes[idx],idx);
  if (idx > 1) { //if we have more then two values, we test whether the end
    int cnt=0,cnt2=1,last=0;
    for (cnt=1; cnt<idx; cnt++) { 
      cnt2++; if (debug) printf("DBG: %i|%i comparation with %i|%i\n",
              bugcodes[cnt],bugcodes[cnt2],bugcodes[0],bugcodes[1]);
      if((bugcodes[0] == bugcodes[cnt]) && (bugcodes[1] == bugcodes[cnt2])) {
        last=cnt;
      }
                        
    }
    if (last) 
      for (cnt = 0; cnt < last; cnt++) { //print bugs
        printf("%i ",bugcodes[cnt]);
        if (cnt==last-1) printf("\n");
        stop=1;
      }
    //if the two values are equal, we have only one bug
  } else if ((idx==1)&&(bugcodes[0]==bugcodes[1])){  
//      printf("%i \n",bugcodes[0]); 
      sprintf(bugnum, "%i", bugcodes[0]);
      showbug(bugnum);
      printf("chyba cislo: %i\n",bugcodes[0]);
      stop=1; 
    } 
  idx++;  //increase index
}

//main function
int main(int argc, char *argv[]){
int j;
  //startup arguments testing
  if (argc == 1) {usage(); stop=1;}
  for (j=0;j<argc;j++){
    if(!strcmp(argv[j],"-s")) {if (argv[j+1]) xSerDev = argv[j+1]; 
                                else {stop=1; usage();}}
    if(!strcmp(argv[j],"-c")) {if (argv[j+1]) comm = argv[j+1]; 
                                else {stop=1; usage();}}
    if(!strcmp(argv[j],"-?")) {stop=1;usage();}
    if(!strcmp(argv[j],"-d")) {debug=1;}
    } 
  //stop if ve haven't got some argument
  if (stop) return 0;
  //serial line start
  //TODO set serial to original values on every end of program
  serline();
  
   if (comm)  { printf("%s\n",sendcommand(comm)); stop=1;}
  //set quido
  printf("Setting quido .. %s\n",sendcommand("*B1IS1"));
  while (!stop) {
      if (read(fd,&c,1)>0)
    strncat(msg, &c, 1);
      if (c == '\r') { //if there is an end of the line
        if (!strncmp(msg,"*B1D",4)) {
            if ((msg[inputmotor+4]=='H')&&(imp==1)) {
                imp=0;
                tmr=tm_stop(); //mezera mezi impulzy, kdyz nebezel timer tak 0
                if ((debug)&&(tmr)) printf("DBG: pause length: %f\n",tmr); 
                if (!run)  tm_start();
                if(tmr > 2.5) {
                    if (debug) printf("DBG: long space, new value\n");
                    bugs(bugcode);//save the value
                    bugcode=0;
                } else if (tmr > 0.5) {
                    if (debug) printf("DBG: short space, continue\n");
                }
                }
            else if((msg[inputmotor+4]=='L')&&(imp==0)){
                imp=1;
                tmr=tm_stop();
                if ((debug)&&(tmr)) printf("DBG: pulse length: %f\n",tmr);
                if (!run) tm_start(); //zde je celkovy cas impulzu,zde vlozime porovnani
                   if (tmr > 1) { 
                        if (debug) printf("DBG: long impulz\n"); //new value
                        bugcode=bugcode+10;
                        }else if (tmr > 0.3) {
                         if(debug) printf("DBG: short impulz\n"); 
                         bugcode=bugcode+1;
                        } else if(debug) printf("DBG: unknown impulz\n");
                  }else if (debug) printf("DBG: unexpeced impulz\n");
        }bzero(msg,BUGSIZE);    
        }
        }    
//set original values of serial line
tcsetattr(fd,TCSANOW,&oldtio);
tcflush( fd, TCIOFLUSH );
close(fd);
return ret;
}
