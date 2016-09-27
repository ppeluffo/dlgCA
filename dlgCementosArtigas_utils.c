/*
 * dlgCementosArtigas_utils.c
 *
 *  Created on: 26 de set. de 2016
 *      Author: pablo
 */

//--------------------------------------------------------------------------
#include "dlgCementosArtigas.h"

#define MAXCONFIGLINE 255

void pv_parseConfigLine(char *linea);
void pv_printConfigParameters(void);

void pv_bdMySqlInit(void);
void pv_bdMySqlConnect(void);
void pv_bdMySqlStmtInit(void);

//--------------------------------------------------------------------------
// CONFIGURACION
//--------------------------------------------------------------------------
void u_readConfigFile(void)
{
/* Lee el archivo de configuracion (default o pasado como argumento del programa
   Extraemos todos los parametros operativos y los dejamos en una estructura de
   configuracion.
   Si DEBUG=1 hacemos un printf al archivo de log de todos los parametros
*/
	FILE *configFile_fd;
	char linea[MAXCONFIGLINE];


	bzero(&systemVars, sizeof(systemVars));
	/* Valores por defecto */
	systemVars.daemonMode = 0;
	strcpy(systemVars.serialPort, "ttyUSB0");
	strcpy(systemVars.dbaseName, "dlgDb");
	strcpy(systemVars.dbaseUser, "root");
	strcpy(systemVars.dbasePassword, "spymovil");

	if (  (configFile_fd = fopen("/etc/dlgCementosArtigas.conf", "r" ) ) == NULL ) {
		sprintf(logBuffer,"WARN: No es posible abrir el archivo de configuracion: Configuracion por defecto !!\n");
		F_syslog();
	} else {
		// Parseo el archivo para reconfigurarme
		while ( fgets( linea, MAXCONFIGLINE, configFile_fd)  != NULL ) {
			pv_parseConfigLine(linea);
		}
	}

	pv_printConfigParameters();
}
//--------------------------------------------------------------------------
void pv_parseConfigLine(char *linea)
{
/* Tenemos una linea del archivo de configuracion.
 * Si es comentario ( comienza con #) la eliminamos
 * Si el primer caracter es CR, LF, " " la eliminamos.
 * Las lineas deben terminar con ;
*/
	char *index;
	int largo;

	if (linea[0] == '#') return;

	/* Busco los patrones de configuracion y los extraigo */

	/* Serial Port */
	if ( strstr ( linea, "SERIALPORT=") != NULL ) {
		index = strchr(linea, '=') + 1;
		if (strchr(linea, ';') != NULL ) {
			largo = strchr(linea, ';') - index;
			bzero(systemVars.serialPort, strlen(systemVars.serialPort));
			strncpy(systemVars.serialPort, index, (largo));
		}
		return;
	}

	/* Nombre de la base de datos mysql */
	if ( strstr ( linea, "DBASENAME=") != NULL ) {
		index = strchr(linea, '=') + 1;
		if (strchr(linea, ';') != NULL ) {
			largo = strchr(linea, ';') - index;
			bzero(systemVars.dbaseName, strlen(systemVars.dbaseName));
			strncpy(systemVars.dbaseName, index, (largo));
		}
		return;
	}

	/* Usuario de la base de datos */
	if ( strstr ( linea, "DBASEUSER=") != NULL ) {
		index = strchr(linea, '=') + 1;
		if (strchr(linea, ';') != NULL ) {
			largo = strchr(linea, ';') - index;
			bzero(systemVars.dbaseUser, strlen(systemVars.dbaseUser));
			strncpy(systemVars.dbaseUser, index, (largo));
		}
		return;
	}

	/* contraseña de la base de datos */
	if ( strstr ( linea, "DBASEPASS=") != NULL ) {
		index = strchr(linea, '=') + 1;
		if (strchr(linea, ';') != NULL ) {
			largo = strchr(linea, ';') - index;
			bzero(systemVars.dbasePassword, strlen(systemVars.dbasePassword));
			strncpy(systemVars.dbasePassword, index, (largo));
		}
		return;
	}

	/* Si trabaja en modo demonio o consola */
	if ( strstr ( linea, "DAEMON=") != NULL ) {
		index = strchr(linea, '=') + 1;
		systemVars.daemonMode = atoi(index);
		return;
	}

}
//--------------------------------------------------------------------------
void pv_printConfigParameters(void)
{

	sprintf(logBuffer,"SERIALPORT=%s\n", systemVars.serialPort); F_syslog();
	sprintf(logBuffer,"DBASENAME=%s\n", systemVars.dbaseName); F_syslog();
	sprintf(logBuffer,"DBASEUSER=%s\n", systemVars.dbaseUser); F_syslog();
	sprintf(logBuffer,"DBASEPASSWD=%s\n", systemVars.dbasePassword); F_syslog();
	sprintf(logBuffer,"DAEMON=%d\n", systemVars.daemonMode); F_syslog();

}
//--------------------------------------------------------------------------
// SERIAL PORT
//--------------------------------------------------------------------------
void u_initSerialPort(void)
{
	// Abro e inicializo el puerto serial con los datos de la configuracion

struct termios serialPortOptions;

	//strcpy (systemVars.serialPort, "/dev/ttyUSB0");

	// Paso1: Abro el puerto serial para lectura canonica
	bzero(logBuffer, sizeof(logBuffer));
	sprintf(logBuffer, "Abriendo el puerto serial %s...\n", systemVars.serialPort);
	F_syslog();

	spFd = open(systemVars.serialPort, O_RDWR | O_NOCTTY | O_NDELAY, 0777);
	// O_RDWR: podemos leer y escribir
	// O_NOCTTY: no queremos ser una terminal tty
	// O_NODELAY: no le presto atencion al estado de la linea DCD

	if (spFd < 0) {
		bzero(logBuffer, sizeof(logBuffer));
		sprintf(logBuffer, "ERROR::No puedo abrir puerto serial %s !!\n", systemVars.serialPort);
		F_syslog();
		exit(1);
	} else {
		fcntl(spFd, F_SETFL, 0);
		// Seteo las flags del dispositivo
		// Al ponerlas en 0 lo fijo en modo de bloqueo.
		// Para que retorne enseguida debo poner FNDELAY
	}

	// Paso2: Configuro los parametros de acuerdo a lo requerido por el GPS
	bzero(logBuffer, sizeof(logBuffer));
	sprintf(logBuffer, "Configuro puerto serial...\n");
	F_syslog();

	tcgetattr(spFd, &serialPortOptions); /* leo la configuracion actual del puerto */
	/*
	 * Control Flags
	 * -------------
	 * BAUDRATE: Fija la tasa bps. Podria tambien usar cfsetispeed y cfsetospeed.
	 * CRTSCTS : control de flujo de salida por hardware (usado solo si el cable
	 *           tiene todas las lineas necesarias Vea sect. 7 de Serial-HOWTO)
	 * CS8     : 8n1 (8bit,no paridad,1 bit de parada)
	 * CLOCAL  : conexion local, sin control de modem
	 * CREAD   : activa recepcion de caracteres
	*/
	cfsetispeed(&serialPortOptions, B115200);
	cfsetospeed(&serialPortOptions, B115200);
	// gpsSerialPortOptions.c_cflag |= systemParameters.gpsBaudrate;
	serialPortOptions.c_cflag &= ~CRTSCTS;
	serialPortOptions.c_cflag |= CS8;
	serialPortOptions.c_cflag |= CLOCAL;
	serialPortOptions.c_cflag |= CREAD;

	/*
	 * Input flags
	 * -----------
	 * IGNPAR  : ignora los bytes con error de paridad
	 * ICRNL   : mapea CR a NL (en otro caso una entrada CR del otro ordenador
	 *  	      no terminaria la entrada) en otro caso hace un dispositivo en bruto
	 *  	      (sin otro proceso de entrada)
	 *  IXON | IXOFF | IXANY : Control de flujo. ( No lo necesito )
	*/
	serialPortOptions.c_iflag &= ~IGNPAR;
	//	serialPortOptions.c_iflag &= ~ICRNL;
	serialPortOptions.c_iflag |= ICRNL;
	serialPortOptions.c_iflag &= ~(IXON | IXOFF | IXANY);

	/*
	 * Local flags
	 * ------------
	 * ICANON  : activa entrada canonica
	 *  	      desactiva todas las funcionalidades del eco, y no envia segnales al
	 *  	      programa llamador
	*/
	serialPortOptions.c_lflag |= ICANON;
	serialPortOptions.c_lflag &= ~ECHO;
	serialPortOptions.c_lflag &= ~ECHOE;
	//	gpsSerialPortOptions.c_lflag &= ~ISIG;

	/*
	 * Output flags
	 * ------------
	*/
	serialPortOptions.c_oflag = 0;

	/*
	 * Caracteres de control
	 * ---------------------
	 * Inicializa todos los caracteres de control
	 * los valores por defecto se pueden encontrar en /usr/include/termios.h,
	 * y vienen dadas en los comentarios, pero no los necesitamos aqui
	*/

	serialPortOptions.c_cc[VINTR]    = 0;     /* Ctrl-c */
	serialPortOptions.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	serialPortOptions.c_cc[VERASE]   = 0;     /* del */
	serialPortOptions.c_cc[VKILL]    = 0;     /* @ */
	serialPortOptions.c_cc[VEOF]     = 4;     /* Ctrl-d */
	serialPortOptions.c_cc[VTIME]    = 5;     /* temporizador entre caracter, no usado */
	serialPortOptions.c_cc[VMIN]     = 1;     /* bloqu.lectura hasta llegada de caracter. 1 */
	serialPortOptions.c_cc[VSWTC]    = 0;     /* '\0' */
	serialPortOptions.c_cc[VSTART]   = 0;     /* Ctrl-q */
	serialPortOptions.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	serialPortOptions.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	serialPortOptions.c_cc[VEOL]     = 0;     /* '\0' */
	serialPortOptions.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	serialPortOptions.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	serialPortOptions.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	serialPortOptions.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	serialPortOptions.c_cc[VEOL2]    = 0;     /* '\0' */

	// 	ahora limpiamos la linea del modem
	tcflush(spFd, TCIFLUSH);

	//	y activamos la configuracion del puerto NOW
	tcsetattr(spFd,TCSANOW,&serialPortOptions);

}
//--------------------------------------------------------------------------
// BASE DE DATOS
//--------------------------------------------------------------------------
void u_initBD(void)
{
	pv_bdMySqlInit();
	pv_bdMySqlConnect();
	pv_bdMySqlStmtInit();
}
//--------------------------------------------------------------------------
void pv_bdMySqlInit(void)
{
/* Inicializo la mysql. En caso de no poder aborta el programa y loguea
 * La secuencia de funciones es la extraida de "Writing MySql programs using C".
*/

	mysql_library_init(0,NULL,NULL);

	/* Inicializo el sistema de BD */
	conn = mysql_init(NULL);
	bzero(logBuffer, sizeof(logBuffer));
	if (conn == NULL) {
		sprintf(logBuffer, "ERROR_002: No puedo inicializar MySql. No puedo continuar\n");
		F_syslog();
		exit (1);
	} else {
		sprintf(logBuffer, "MySql inicializada.. OK.\n");
		F_syslog();
	}
}
//--------------------------------------------------------------------------
void pv_bdMySqlConnect(void)
{
/* Intento conectarme a la BD con el usuario y el passwd pasado en los args */

	bzero(logBuffer, sizeof(logBuffer));
	if (mysql_real_connect(conn, NULL, systemVars.dbaseUser, systemVars.dbasePassword, systemVars.dbaseName, 0, NULL, 0) == NULL ) {
		sprintf(logBuffer, "ERROR_003: No puedo conectarme a la BD. No puedo continuar\n");
		F_syslog();
		exit (1);
	} else {
		sprintf(logBuffer, "Conectado a la BD.. OK.\n");
		F_syslog();
	}
}
//--------------------------------------------------------------------------
void pv_bdMySqlStmtInit(void)
{
	/* Inicializo las consultas */
	stmt_main = mysql_stmt_init(conn);
	bzero(logBuffer, sizeof(logBuffer));
	if (!stmt_main) {
		sprintf(logBuffer, "ERROR_004: No puedo inicializar la statement MAIN. No puedo continuar\n");
		F_syslog();
		exit (1);
	} else {
		sprintf(logBuffer, "Inicializo la statement MAIN.. OK.\n");
		F_syslog();
	}

}
//--------------------------------------------------------------------------
// GENERAL
//--------------------------------------------------------------------------
void F_syslog(void)
{
/* Loguea por medio del syslog la linea de texto con el formato pasado
 * Por ahora solo imprimimos en pantalla
 * Tener en cuenta que en caso de los forks esto puede complicarse por
 * lo cual hay que usar el syslog
 * La funcion la definimos del tipo VARADIC (leer Manual de glibC.) para
 * poder pasar un numero variable de argumentos tal como lo hace printf.
 *
 * Siempre loguea en syslog. Si NO es demonio tambien loguea en stderr
*/
	char printbuf[MAXLINE - 1];
	int i,j;

	/* Elimino los caracteres  \r y \n para no distorsionar el formato. */
	i=0;
	j=0;
	while ( logBuffer[i] != '\0') {
		if ( ( logBuffer[i] == '\n') || (logBuffer[i] == '\r')) {
			i++;
		} else {
			printbuf[j++] = logBuffer[i++];
		}
	}
	printbuf[j++] = '\n';
	printbuf[j++] = '\0';

	/* Si el programa se transformo en demonio uso syslog, sino el dato lo
      manda tambien como stream al stdout
	*/
	syslog((LOG_INFO | LOG_USER), printbuf);

	/* Si el programa NO es demonio, muestro el log por el stdout */
	if (systemVars.daemonMode == 0) {
		fflush(stdout);
		fputs(logBuffer, stdout);
		fflush(stdout);
	}

}
//--------------------------------------------------------------------------
void daemonize( void)
{
	int i;
	pid_t pid;

	if ( ( pid = fork()) != 0)
		exit (0);					/* El parent termina */

	/* Continua el primer child  que se debe volver lider de sesion y no tener terminal */
	setsid();

	signal(SIGHUP, SIG_IGN);

	if ( (pid = fork()) != 0)
		exit(0);					/* El primer child termina. Garantiza que el demonio no pueda
									 * adquirir una terminal
									*/
	/* Continua el segundo child */
	chdir("/");
	umask(0);

	for(i=0; i< MAXFD; i++)	/* Cerramos todos los descriptores de archivo que pueda haber heredado */
		close(i);

}
/*--------------------------------------------------------------------------*/


