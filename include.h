//defaultni knihovny
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>

#if defined(__APPLE__) && defined(__MACH__)
/* OS X doesn't support clock_gettime (in at least OSX <= 10.6) */
#  include "osx.h"
#endif


//definice
#define BUFSIZE 200
#define BUGSIZE 50
#define MODEMDEVICE "/dev/ttyS1"
#define BAUDRATE B115200
#define FALSE 0
#define TRUE 1

//promenne
struct timespec t1,t2; //citace
struct termios oldtio,newtio; //tohle pro sockety
double tmr;
int stop=0,debug=0,run=0,imp=1,idx=0; //debug defaultne vypnut,index pole na 0
int inputmotor=1;
int bugcode; //nacteny kod chyby
int fd, res; //seriak
int ret=0; //defaultne konci program bez chyby
char *result;
char *xSerDev = MODEMDEVICE; // do xSerDev se ulozi cesta k seriovemu portu z definici
char *comm;
char *tosend = NULL; //sem se uklada text,ktery se nasledne odesila k vypsani
char *defFile = "323fbj.def";
int bugcodes[BUGSIZE]; // pole chybovych kodu
char msg[BUFSIZE]; //vytvori pole o velikosti BUFSIZE, ukladaji se sem docasne odpovedi
char c; //cteni seriaku
