/*
 ============================================================================
 Name        : dlgCementosArtigas.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 *
 * Programa que implementa el datalogger de Cementos Artigas.
 * Este esta formado por una placa SP5KV5_8CH, con un firmware modificado para que
 * no use el gprs y que mande los frames por el serial.
 * Este se conecta con RS485 al PC donde corre este programa.
 * El programa inicializa el puerto serial, abre el log y se queda en un loop
 * esperando lineas del puerto serial.
 * Cuando recibe una, si es del tipo correcto la loguea y la guarda en la BD mysql.
 * - Si al recibir una linea, la fecha,hora difiere de la del PC, entonces le manda
 *   al dataloger un frame de escritura de fechaHora.
 * - Corre en modo demonio.
 * - La configuracion del puerto serial la toma del /etc/dlgCementosArtigas.conf
 *
 */
//--------------------------------------------------------------------------
#include "dlgCementosArtigas.h"
//--------------------------------------------------------------------------

void pv_processRXLines(void);
void pv_checkDlgClock(void);

//--------------------------------------------------------------------------

int main(void) {

int i, res;

	openlog( "dlgCA", LOG_PID, LOG_USER);
	bzero(logBuffer, sizeof(logBuffer));
	sprintf(logBuffer, "Inicio dlgCementosArtigas %s...\n", VERSION);
	F_syslog();

	u_readConfigFile();

	if (systemVars.daemonMode == 1)
		daemonize();

	u_initSerialPort();
	u_initBD();

	// Quedo en un loop infinito bloqueado, leyendo el puerto serial
	// y procesando frames.
	while(1){

		bzero ( serialRdLine, sizeof(serialRdLine));
		// Se bloquea con el read hasta que halla una linea completa
		res = read(spFd, serialRdLine, SERIALLINESLENGH);
		serialRdLine[res] = '\0';

		// Elimino los \n y \r
		for (i=0; i < SERIALLINESLENGH; i++)
			if ( (serialRdLine[i] == '\r') || (serialRdLine[i] == '\n'))
				serialRdLine[i] = '\0';

		// Descarto las lineas vacias
		if (serialRdLine[0] == '\0')
			continue;

		sprintf(logBuffer, "RXline=[%s]\n", serialRdLine);
		F_syslog();

		pv_processRXLines();
	}

}
//--------------------------------------------------------------------------
void pv_processRXLines(void)
{
	// La linea recibida esta en serialRdLine y es de la forma
	// FRAME::{20160927,085755,MAG0=-247.76,MAG1=-243.39,MAG2=-246.54,MAG3=-242.07,MAG4=-247.36,MAG5=-244.30,MAG6=-248.58,MAG7=-247.66,D0_P=0.00,D0_L=0,D1_P=0.00,D1_L=0}
	// La idea es filtrar y si no tiene el tag FRAME descartarla
	// Si lo tiene, la inserto en la BD.

	if ( strstr ( serialRdLine, "FRAME") == NULL ) {
		return;
	}

	// Aqui es porque la linea es la correcta: inserto.

	/* Guardo los datos en la BD */
	/* Inserto en la BD:tb_datosInit */
	sprintf(mysqlCmd,"INSERT INTO tbRxFrames ( rxFrame )VALUES (\"%s\")", serialRdLine );
	if (mysql_query (conn, mysqlCmd) != 0) {
		sprintf (logBuffer, "ERROR_A: No puedo ejecutar stmt: %s\n", mysql_stmt_error(stmt_main) );
		F_syslog();
		sprintf (logBuffer, "ERROR_B [STMT=%s]\n", mysqlCmd );
		F_syslog();
	} else {
		sprintf(logBuffer, "Bd insert OK\n");
		F_syslog();
	}

	// Vemos si la fecha y hora del datalogger es correcta.
	pv_checkDlgClock();

}
//--------------------------------------------------------------------------
void pv_checkDlgClock(void)
{
	// Extraigo la fecha y hora de la linea y comparo con la del sistema
	// Si la diferencia es de mas de 3 minutos, mando al datalogger un frame
	// de ajuste de la hora.
	// 20160927,085755

time_t sysClock;
struct tm *cur_time;
char tmp[32];
long sys_fecha, sys_hora;
long frm_fecha, frm_hora;
char *token, cp[MAXLINE];
char parseDelimiters[] = "{,";
int i;
int outB, msgLenght;

	// Paso 1: Obtengo la fecha y hora del sistema
	sysClock = time (NULL);
	cur_time = localtime(&sysClock);

	// SysFecha
	bzero(tmp, sizeof(tmp));
	strftime (tmp, sizeof(tmp), "%Y%m%d", cur_time);
	sys_fecha = strtol(tmp,NULL,10);

	// SysHora
	bzero(tmp, sizeof(tmp));
	strftime (tmp, sizeof(tmp), "%H%M%S", cur_time);
	sys_hora = strtol(tmp,NULL,10);

	//sprintf (logBuffer, "sys_fecha [%d][%d]\n", sys_fecha,sys_hora );
	//F_syslog();

	// Paso 2: Extraigo el timestamp del frame
	/* Copio el frame recibido a un temporal que va a usar el strtok */
	strcpy(cp,serialRdLine);

	//i=0;
	token = strtok(cp, parseDelimiters);	// [FRAME::

	token = strtok(NULL, parseDelimiters);	// Fecha: 20160927
	frm_fecha = strtol(token,NULL,10);

	token = strtok(NULL, parseDelimiters);	// Hora: 100558
	frm_hora = strtol(token,NULL,10);

	//sprintf (logBuffer, "frm_fecha [%d][%d]\n", frm_fecha,frm_hora );
	//F_syslog();

	//while ( (token = strtok(NULL, parseDelimiters)) != NULL ) {
	//	sprintf (logBuffer, "TOK%02d [%s]\n", i++, token );
	//	F_syslog();
	//}

	//token = strtok(NULL, parseDelimiters);

	//sprintf (logBuffer, "DEBUG dates: sys[%d %d] frm[%d %d]\n",sys_fecha, sys_hora, frm_fecha, frm_hora );
	//F_syslog();

	// Fechas diferentes y horas con diferencia de mas de 3 minutos ( 180 s )
	// Actualizo el datalogger
	if ( ( frm_fecha != sys_fecha) ||  ( abs( frm_hora - sys_hora) > 180 ) ) {
		sprintf (logBuffer, "timeStamp drift !! sys[%d %d] frm[%d %d]\n",sys_fecha, sys_hora, frm_fecha, frm_hora );
		F_syslog();

		// Actualizo el RTC del datalogger
		bzero(tmp, sizeof(tmp));
		strftime (tmp, sizeof(tmp), "write rtc %y%m%d%H%M\r\n", cur_time);

		//sprintf (logBuffer, "cmd [%s]\n",tmp );
		//F_syslog();

		msgLenght = strlen(tmp);
		if ( (outB = write( spFd,tmp, msgLenght)) < 0 ) {
			sprintf(logBuffer, "ERROR updating DLG rtc: [%s]\n", tmp );
			F_syslog();
		} else {
			sprintf(logBuffer, "DLG rtc updated !!!\n" );
			F_syslog();
		}

	}


}
//--------------------------------------------------------------------------
