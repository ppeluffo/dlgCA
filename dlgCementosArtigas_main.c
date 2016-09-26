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

void processRXLines(void);

int main(void) {

int i, res;

	openlog( "dlgCA", LOG_PID, LOG_USER);
	bzero(logBuffer, sizeof(logBuffer));
	sprintf(logBuffer, "Inicio dlgCementosArtigas %s...\n", VERSION);
	F_syslog();

	u_readConfigFile();
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
		for (i=0; i<SERIALLINESLENGH; i++)
			if ( (serialRdLine[i] == '\r') || (serialRdLine[i] == '\n'))
				serialRdLine[i] = '\0';

		// Descarto las lineas vacias
		if (serialRdLine[0] == '\0')
			continue;

		sprintf(logBuffer, "RXline=[%s]\n",serialRdLine);
		F_syslog();

		processRXLines();
	}

}
//--------------------------------------------------------------------------
void processRXLines(void)
{

}
//--------------------------------------------------------------------------
