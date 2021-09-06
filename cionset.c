/* i-charge cion registers */
/* Reg.Nr.:	Name		Function								Format			Multi 	*/
/*   500	Bus-Adr		Adress-Overide (default 1)					uint16_t[-]		1		*/
/*   509	Baudrate	Baudrate-Overide (default 57600)			uint16_t[-]		1		*/
/*   100	Ladefreig.	0:gesperrt 1:erlaubt (override E1)			uint16_t[-]		1		*/
/*   127	Max. I-Lade	6-63A from DIP								uint16_t[mA]	1000	*/
/*   507	min i-Lade	Min I für 0-10V Interface					uint16_t[mA]	1000	*/
/*   101	I-Lade CP	Aktuelle Ladestromvorgabe (CP)				uint16_t[mA]	1000	*/
/*   303	Temperatur	Aktuelle Temperatur							uint16_t[°C]	1		*/
/*   141	PP-Kabel	PP-Ladekabelerkennung (GND,0,16,20,32,63)	uint16_t[-]		1		*/
/*   139	CP-Status	A,B,C,D,E,F,U s.u.							char[-]			-		*/
/*   151-2	Ladedauer	Zeit seit begin der Ladung					uint32_t[ms]	1???	*/
/*   301	UB-Modul	Versorgungsspannung des Moduls				uint16_t[mV]	1000	*/
/*   148	Spannung E1	Spannung an E1								uint16_t[mV]	1000	*/
/*   149	Spannung E3	Spannung an E3 								uint16_t[mV]	1000	*/
/*   150	Fehlerwort	Bitvector									uint16_t[-]		1		*/
/*   832-47	Version		Firmware Version							char32[-]		1		*/
/* CP Stati:    	R(CP-PE)     */
/* A: standby   	offen  */
/* B: angeschlossen 2700Ohm */
/* C: charging	    880Ohm 	*/
/* D: ventilation	240Ohm  */
/* E: no power		cp:0V	*/
/* F: error			cp:-12V	*/

// compile (better use Makefile ...) 
// export LD_LIBRARAY_PATH "/usr/local/lib"
// gcc -g -I/usr/local/include -L/usr/local/lib -lmodbus -o cionset
// check : ldd cionstat -> should have no unresolved references


// mutex via file locking:
//int fd_lock = open(LOCK_FILE, O_CREAT);
//-> fd_lock is ctx.s !
//flock(fd_lock, LOCK_EX);
//
//// do stuff
//
//flock(fd_lock, LOCK_UN);
 
#include <stdio.h>
#include <unistd.h>
#include <modbus/modbus.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm/ioctls.h>
#include <sys/file.h>
#include <stdlib.h>

//#include <semaphore.h>
//#include <pthread.h>

#define NB_REGS 18
#define UART_PATH "/dev/ttyUSB1"

// static sem_t rs485sem;

typedef struct _modreglen_t {
	int adr;
	int anz;
} modreglen_t;

modreglen_t cionreads[] = {{100,1},{127,1},{507,1},{101,1},{303,1},{141,1},{139,1},{151,2},{301,1},{302,1},{148,1},{149,1},{150,1},{832,16}};
char* mycommands[] ={"lock","unlock","current","load_for_mins","load_until","wait_cpa"};

int cionlock(modbus_t *ctx) {
  
	static uint16_t lockvalue = 0;
	return(modbus_write_register(ctx,101,lockvalue));
}

int cioncurrent(modbus_t *ctx,uint16_t newcurrent) {
	return(modbus_write_register(ctx,101,newcurrent));
}

int cionunlock(modbus_t *ctx) {
	return(modbus_write_register(ctx,100,0xff00));
// maybe 0xff00 works better ! modbus-spec.	
}

int cionwait_cpx(modbus_t *ctx,char waittoc) { /* Waits for cable change or charge start/stop */
  uint16_t ret;
  int rc; 
  do {
    sleep(10);
    modbus_read_registers(ctx,139,1,&ret);
    if (rc == -1) {
      fprintf(stderr, "%s\n", modbus_strerror(errno));
      sleep(60);
      // break;
    }
  } while ((char) ret != waittoc);
  return(rc);
}    

int cionwait_ms(modbus_t *ctx,unsigned int endtimems) { /* Waits for  WB internal timer */
  uint16_t ret[2];
  int rc; 
  do {
    sleep(30);
    modbus_read_registers(ctx,151,2,ret);
    if (rc == -1) {
      fprintf(stderr, "%s\n", modbus_strerror(errno));
      sleep(60);
      break;
    }
  } while (((unsigned int) ret[1]*65535+ret[0]) < endtimems );
  return(rc);
}    

int cionprintstatus(modbus_t *ctx) {
	int i,ret;
	uint16_t dest [NB_REGS];
	// try locking via fd_lock
//	if (!flock(ctx->s,LOCK_EX)) fprintf(stderr,"Serial Device Locking via flock failed with errno: %d\n",errno);
    for(i=0;i<14;i++){
        // Read 2 registers from adress 0, copy data to dest.
        ret = modbus_read_registers(ctx, cionreads[i].adr, cionreads[i].anz, dest);
        if(ret < 0){
	    fprintf(stderr, "Modbus_read_registers failed due to: %s\n", modbus_strerror(errno));
            break;
        }
		switch (cionreads[i].adr) {
			case 100:
			printf("Ladefreigabe : %d\n",dest[0]);
			break;
			case 127:
			printf("I-Max Ladekabel: %5d A\n",dest[0]);
			break;
			case 507:
			printf("i-min 0-10V IF : %5d A\n",dest[0]);
			break;
			case 101:
			printf("Aktuelle Ladestromvorgabe : %2d A\n",dest[0]);
			break;
			case 303:
			printf("Aktuelle Temperatur : %d °C\n",dest[0]);
			break;
			case 141:
			printf("PP Ladekabelerkennung : %d A\n",dest[0]);
			break;
			case 139:
			printf("CP-Status : %c\n",dest[0]);
			break;
			case 151:
			  printf("Zeit seit Ladebeginn : %d\n",(unsigned int)dest[1]*65536+dest[0]);
			break;
			case 301:
			printf("Modul Versorgungsspannung : %d mV\n",dest[0]);
			break;
		        case 302:
			  printf("Netzspannung : %d V\n",dest[0]/100);
			  break;
			case 148:
			printf("Spannung an E1 : %d mV\n",dest[0]);
			break;
			case 149:
			printf("Spannung an E3 : %d mV\n",dest[0]);
			break;
			case 150:
			printf("Fehlerwort : %x\n",dest[0]);
			break;
			case 832:
			dest[16]=0;
			printf("Software Version : %s\n", (char*) dest);
			break;
			default:
			printf("Unknown Modbus register value %d\n",cionreads[i].adr);
		}/*switch*/
   }/*for*/     
//   if (!flock(ctx->s,LOCK_UN)) fprintf(stderr,"Serial Device Unlocking via flock failed with errno: %d\n",errno);
}
int main(int argc, char** argv)
{
    int ret,val;
    modbus_t *ctx;
	static int actarg;    
    struct timeval response_timeout;
    response_timeout.tv_sec = 10;
    response_timeout.tv_usec = 0;

    ctx = modbus_new_rtu(UART_PATH, 57600, 'N', 8, 1);
    if (ctx == NULL) {
        perror("Unable to create the libmodbus context\n");
        return -1;
    }

    modbus_set_response_timeout(ctx, &response_timeout);

    ret = modbus_set_slave(ctx, 1);
    if(ret < 0){
        perror("modbus_set_slave error\n");
        return -1;
    }

//	ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
//    if(ret < 0){
//        perror("modbus_rtu_set_serial_mode error\n");
//        return;
//    }

    ret = modbus_connect(ctx);
    if(ret < 0){
		fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        perror("modbus_connect error\n");
        return -1;
    }
    cionprintstatus(ctx) ;
    for(actarg=1;actarg<argc;actarg++){
		if (!strcmp("lock",argv[actarg])) {
			fprintf(stderr,"try locking\n");
			if (!cionlock(ctx)) fprintf(stderr, "Lock failed: %s\n", modbus_strerror(errno));
			continue;
		}
		if (!strcmp("unlock",argv[actarg])) {
			if (!cionunlock(ctx)) fprintf(stderr, "Unlock failed: %s\n", modbus_strerror(errno));
			continue;
		}
		if (!strcmp("wait_cpa",argv[actarg])) {
		  cionwait_cpx(ctx,'A');
		  continue;
		}
		if (!strcmp("wait_cpb",argv[actarg])) {
		  cionwait_cpx(ctx,'B');
		  continue;
		}
		if (!strcmp("wait_cpc",argv[actarg])) {
		  cionwait_cpx(ctx,'C');
		  continue;
		}
		if (!strcmp("current",argv[actarg])) {
			if (actarg >=  argc) { 
				fprintf(stderr,"Missing current value \n");
				break;
			}
			actarg++;
			val  = atoi(argv[actarg]);
			if ((val != 0) && ((val < 6 ) || (val >160))) { fprintf(stderr," Not allowed current: %d \n",val);}
			else if (!cioncurrent(ctx,val)) fprintf(stderr, "Set current failed: %s\n", modbus_strerror(errno));
			sleep(5);
		}
		if (!strcmp("waitformin",argv[actarg])) {
			if (actarg >=  argc) { 
				fprintf(stderr,"Missing number of minutes to wait\n");
				break;
			}
			actarg++;
			val  = atoi(argv[actarg]);
			if ( val < 1 ) { fprintf(stderr," Value is to small: %s -> %d \n",argv[actarg],val);}
			else if (!cionwait_ms(ctx,((unsigned int)val)*60000)) fprintf(stderr, "cionwait_ms failed: %s\n", modbus_strerror(errno));
		}
		
	}/*for*/
	cionprintstatus(ctx) ;
    modbus_close(ctx);
    modbus_free(ctx);
    return(0);
}
