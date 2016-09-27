/*
 * dlgCementosArtigas.h
 *
 *  Created on: 26 de set. de 2016
 *      Author: pablo
 */

#ifndef DLGCEMENTOSARTIGAS_H_
#define DLGCEMENTOSARTIGAS_H_

#include <argp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <stddef.h>
#include <syslog.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <wait.h>

#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>

//--------------------------------------------------------------------------
#define LOGBUFFERLENGHT	1024
#define MAXLENPARAMETERS 64
#define SERIALLINESLENGH 256
#define MAXLINE 256
#define MAXFD 64
#define MAXSQLCMDSTRING	 512

#define VERSION (const char *)("Version 1.0.0 @ Spymovil,2016-09-26")

//--------------------------------------------------------------------------
char logBuffer[LOGBUFFERLENGHT];
char serialRdLine[SERIALLINESLENGH];
char mysqlCmd[MAXSQLCMDSTRING];

int spFd;	// descriptor del puerto serial

struct {
	char configFile[MAXLENPARAMETERS];
	char serialPort[MAXLENPARAMETERS];
	int daemonMode;
	char dbaseName[MAXLENPARAMETERS];
	char dbaseUser[MAXLENPARAMETERS];
	char dbasePassword[MAXLENPARAMETERS];
} systemVars;

MYSQL *conn;
MYSQL_STMT *stmt_main, *stmt_values;

//--------------------------------------------------------------------------

void F_syslog(void);
void u_readConfigFile(void);
void u_initSerialPort(void);
void u_initBD(void);
void daemonize( void);

#endif /* DLGCEMENTOSARTIGAS_H_ */
