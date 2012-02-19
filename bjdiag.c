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

//serial port
void serline(int control){
    //control = 0 = close port
    //control = 1 = open port
    
    if (control) {
        fd = open (xSerDev, O_RDWR | O_NOCTTY | O_NONBLOCK ); 
        if(fd<0) {
            fprintf(stderr,"ERR: Unable to open serial port %s\n",xSerDev);
            exit(1);}
        else {
            if (debug) 
               printf("DBG: Serial port opened\n");
            //save old serial parameters
            tcgetattr(fd, &oldtio);
            //set new parameters
            fcntl( fd, F_SETFL, O_SYNC); 
            memset( &newtio, 0, sizeof( newtio ));
            //apple needs different style of configuration in this place
            #if defined(__APPLE__) && defined(__MACH__)
            newtio.c_cflag = ( CS8 | CLOCAL | CREAD);
            #else
            newtio.c_cflag = ( BAUDRATE | CS8 | CLOCAL | CREAD);
            #endif
            newtio.c_iflag = (IGNPAR);
            newtio.c_oflag = 0;
            newtio.c_lflag = 0;
            newtio.c_cc[VMIN] = 1;
            newtio.c_cc[VTIME] = 1;
            newtio.c_ispeed = BAUDRATE;
            newtio.c_ospeed = BAUDRATE;
            tcsetattr( fd, TCSANOW, &newtio );
            tcflush( fd, TCIOFLUSH );}} 
    else { 
        //set original values of serial line
        if (debug) 
            printf("DBG: closing serial port\n");
        tcsetattr(fd,TCSANOW,&oldtio);
        tcflush( fd, TCIOFLUSH );
        close(fd);} 
}

//pointers for definition files
void deffile(int control){
    //control = 0 = close deffile
    //control = 1 = open deffile for engine
    //control = 2 = open deffile for abs
    char* dfile = defFileEng;
    switch (control) { 
        case 0:
            if (debug) 
                printf("DBG: closing deffile\n");
            fclose(df);
            break;
        case 2:
            dfile = defFileAbs;
        default:
            df = fopen(dfile, "r");
            if(!df) { 
                fprintf(stderr,"ERR: Unable to open definition file %s\n",dfile); 
                exit(1);} 
            else {
                if (debug)
                        printf("DBG: Definition file  %s opened\n",dfile);}
            break;}   
}

char *sendcmd(char *in){
    int cnt=0;
    char c; 
    tosend = strdup(in);
    strcat(tosend, "\r\n");
    res = write (fd, tosend, strlen(tosend));
    if( res < 0 ){
        fprintf(stderr,"ERR: Unable to write to the serial port \n"); 
        exit(1);}
    bzero(msg,BUFSIZE);
    while (c !='\r'){
        if (read(fd,&c,1)>0)
           strncat(msg, &c, 1);
        cnt+=1;
        if (cnt > BUFSIZE ) {
           fprintf(stderr,"ERR: increase BUFSIZE and recompile\n"); 
           exit(2);}} //buffer owerflow  
    if (!strcmp(msg,"*B10\r")) { //if good answer
        bzero(msg,BUFSIZE); 
        strcat(msg, "OK");}
    sleep(1);
    return msg; 
}

// usage screen -h
int opts(int argc, char** argv){
    //return 0 - engine diagnostic
    //return 1 - abs diagnostic
    int sw,ret=0;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"abs",     no_argument, 	   0,  'a' },
            {"engine",  no_argument,       0,  'e' },
            {"help",    no_argument,	   0,  'h' },
            {"debug",   required_argument, 0,  'd' },
            {"port",    required_argument, 0,  'p' },
            {0,         0,                 0,  0 }};

        sw = getopt_long(argc, argv, "d:p:aeh",
                 long_options, &option_index);
        if (sw == -1)
            break;

        switch (sw) {
            case 'e':
                ret = 0;
                break;

            case 'd':
                debug = atoi(optarg);
                printf("DBG: starting with debug level : %i\n",debug);
                break;

            case 'p':
                xSerDev = optarg;
                break;

            case 'a':
                ret = 1;
                break;
            case 'h':
            default : 
                printf ("Usage:  bjdiag [ -p port_path ] MODE\n\n");
                printf ("MODES\n");
                printf ("  -a,--abs\t:\tabs diagnostic mode\n");
                printf ("  -e,--engine\t:\tengine diagnostic mode (default)\n\n");
                printf ("SETTINGS\n");
                printf ("  -p,--port\t:\tserial port path, default is: %s\n\n",xSerDev); 
                printf ("OUTPUT\n");
                printf ("  -d [level]\t:\tdebug mode\n");
                printf ("  -h,--help\t:\tprint this message\n");
                if (optind < argc) {
                    printf("Unknown options : ");
                    while (optind < argc)
                        printf("%s ", argv[optind++]);
                    printf("\n");}
                exit(0);
                break;}}
    if (debug)
        printf("DBG: serial port is : %s\n",xSerDev);
    return ret;
}

double timer(){
    if(run) {
        clock_gettime(CLOCK_MONOTONIC, &t2);
        run=0;
        return ((double)(t2.tv_sec - t1.tv_sec) + 
                 + 1.e-9*(t2.tv_nsec - t1.tv_nsec));}
    else {
        clock_gettime(CLOCK_MONOTONIC,&t1);
        run=1;
        return 0;}
}

void configure(int control){
    //control = 0 - for engine diagnostic
    //control = 1 - for abs diagnostic
    //control = 2 - deconfigure
    //serial line start
    switch  (control){
       case 0:
       case 1:
           serline(1); //open serial port
           printf("Configuring quido module ");
           fflush(stdout);
           //set quido auto send, R1 ON
           if (!strcmp(sendcmd("*B1IS1"),"OK")){
               if (debug>1) printf("\nDBG: Set autosend feature of quido\n");
               else printf(".");
               fflush(stdout);
               if (!strcmp(sendcmd("*B1OS1L"),"OK")){
                   if (debug>1) printf("DBG: Set R1 to open state\n");
                   else printf(".");
                   fflush(stdout);
                   if (!strcmp(sendcmd("*B1OS2L"),"OK")) {
                       if (debug>1) printf("DBG: Set R2 to open state\n");
                       else printf(".");
                       fflush(stdout);
                       if (control) { 
                           if (!strcmp(sendcmd("*B1OS2H"),"OK")){
                                   if (debug>1) printf("DBG: Set R2 to closed state\n");
                                   else printf(". DONE\n");}
                          else fprintf(stderr,"ERR: Unable to set output 1\n");}
                      else {
                          if (!strcmp(sendcmd("*B1OS1H"),"OK")){
                                  if (debug>1) printf("DBG: Set R1 to closed state\n");
                                  else printf(". DONE\n");}
                          else fprintf(stderr,"ERR: Unable to set output 1\n");}}
                  else fprintf(stderr,"ERR: Unable to reset output 2\n");} 
               else fprintf(stderr,"ERR: Unable to set output 1\n");}
           else fprintf(stderr,"ERR: Unable to set quido for auto send\n");         
           break;
       case 2:
           if (debug) printf("DBG: turning off relays\n");
           sendcmd("*B1OS1L");
           sendcmd("*B1OS2L");
           serline(0);
           break;
   }
   
}

//mozna smazat, jeste nevim
void clean(){
    if (debug) printf("DBG: answer: %s\n",msg);
    bzero(msg,BUFSIZE); //clear mesage buffer
}

void close_all(int sig){
    
    stop=1;
    cl=1;
    deffile(0);
    configure(2);
    (void) signal(SIGINT, SIG_DFL);
}

void showbug(int in){
    char str[200],num[10];
    int errsw=1;
    rewind(df);
    snprintf(num, sizeof(num), "%d", in);
    while(fgets(str,sizeof(str),df) != NULL){
        // strip trailing '\n' if it exists
        int len = strlen(str)-1;
        if(str[len] == '\n') 
            str[len] = 0;
        char*    del = "\t";
        char* record = strtok (str, del);
        int index=0;
        while (record) {
            if (!strcmp(record,num)) {
                printf("Bug number\t: %s\n",record);index=1,errsw=0;}
            if (index==2) printf("Bug definition\t: %s \n",record);
            if (index==3) printf("Possible cause\t: %s\n",record);
            if (index) index++;
            record = strtok(NULL, del);}
    }if (errsw) printf("Unknown bug number\n");
}

void bugs(int in){
    bugcodes[idx]=in; //save bugcode
    if (debug) 
        printf("DBG: Saving %i to the index %i\n",bugcodes[idx],idx);
    if (idx > 1) { //if we have more then two values, we test whether the end
        int cnt=0,cnt2=1,last=0;
        for (cnt=1; cnt<idx; cnt++) { 
            cnt2++;
            if (debug) 
                printf("DBG: %i|%i comparation with %i|%i\n",
                bugcodes[cnt],bugcodes[cnt2],bugcodes[0],bugcodes[1]);
            if((bugcodes[0] == bugcodes[cnt]) && 
               (bugcodes[1] == bugcodes[cnt2])) {
                last=cnt;}}
        if (last) 
        for (cnt = 0; cnt < last; cnt++) { //print bugs
            showbug(bugcodes[cnt]);
            //printf("%i ",bugcodes[cnt]);
           /* if (cnt==last-1) 
                printf("\n");*/
            stop=1;}}
        //if the two values are equal, we have only one bug
    else if ((idx==1)&&(bugcodes[0]==bugcodes[1])){  
        showbug(bugcodes[0]);
        printf("Bug number : %i\n",bugcodes[0]);
        stop=1;}
    idx++;  //increase index
}

//main function
int main(int argc, char **argv){
    (void) signal(SIGINT, close_all); //SIGINT handling for proper port close
    //parse command line options
    switch (opts(argc, argv)){
        case 0 : //engine (default) mode
            configure(0); //open serial, configure module
            deffile(1); //open definition file
            if (debug) printf("DBG: waiting for data\n");
             printf("Please turn ignition key to the position ON without "
            "starting the engine\n");
            char c;
            while (!stop){
                if (read(fd,&c,1)>0)
                strncat(msg, &c, 1);
                if (c == '\r') { //if there is an end of the line
                    if (!strncmp(msg,"*B1D",4)) {
                        if ((msg[inputmotor+4]=='H')&&(imp==1)) {
                            imp=0;
                            tmr=timer(); //mezera mezi impulzy, kdyz nebezel timer tak 0
                            if ((debug>1)&&(tmr)) printf("DBG: pause length: %f\n",tmr); 
                            if (!run) timer();//tm_start()
                            if(tmr > 2.5) {
                                if (debug>1) printf("DBG: long space, new value\n");
                                bugs(bugcode);//save the value
                                bugcode=0;
                            } else if (tmr > 0.5) {
                                if (debug>1) printf("DBG: short space, continue\n");
                            }
                            }
                        else if((msg[inputmotor+4]=='L')&&(imp==0)){
                            imp=1;
                            tmr=timer();
                            if ((debug>1)&&(tmr)) printf("DBG: pulse length: %f\n",tmr);
                            if (!run) timer(); //tm_start
                            if (tmr > 1) { 
                                    if (debug>1) printf("DBG: long impulz\n"); //new value
                                    bugcode=bugcode+10;
                                    }else if (tmr > 0.3) {
                                    if(debug>1) printf("DBG: short impulz\n"); 
                                    bugcode=bugcode+1;
                                    } else if(debug>1) printf("DBG: unknown impulz\n");
                            }else if (debug>1) printf("DBG: unexpeced impulz\n");
                    }bzero(msg,BUGSIZE);    
                    }
                    }  
            if (!cl){
            deffile(0); //close deffile
            configure(2); //deconfigure
            }
            break;
        case 1: //abs mode
            configure(1); //open serial, configure module
            deffile(2); //open definition file
            
            char line[5];
            printf("Please type bug number and press ENTER\n"
                   "you can write more numbers separated by comma and space\n"
                   "or send empty line to exit\n: ");
            while (fgets(line,sizeof(line),stdin)){  
                sscanf(line,"%d",&bugcode);
                if (line[1]==0){
                    break;}
                else {
                    showbug(bugcode);}
                printf(": "); 
                bzero(line,strlen(line));
            }
            
            deffile(0); //close deffile
            configure(2); //deconfigure module, close serial
            break;}
    

return 0;
}

