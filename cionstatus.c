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
// gcc -g -I/usr/local/include -L/usr/local/lib -lmodbus -o cionstat 
// check : ldd cionstat -> should have no unresolved references
 
#include <stdio.h>
#include <unistd.h>
#include <modbus/modbus.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm/ioctls.h>

#define NB_REGS 18
#define UART_PATH "/dev/ttyUSB1"

typedef struct _modreglen_t {
	int adr;
	int anz;
} modreglen_t;

modreglen_t cionreads[] = {{100,1},{127,1},{507,1},{101,1},{303,1},{141,1},{139,1},{151,2},{301,1},{302,1},{148,1},{149,1},{150,1},{832,16}};
int main(int argc, char** argv)
{
    int ret;
    modbus_t *ctx;
    uint16_t dest [NB_REGS];
    int i;

    struct timeval response_timeout;
    response_timeout.tv_sec = 10;
    response_timeout.tv_usec = 10;

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

    ret = modbus_connect(ctx);


    if(ret < 0){
	fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        return -1;
    }
///    printf(" Actual RTS Delay %d us\n",modbus_rtu_get_rts_delay(ctx));
///    modbus_rtu_set_rts_delay(ctx,250);

//    ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
//    if(ret < 0){
//        perror("modbus_rtu_set_serial_mode error\n");
//       return 1;
//    }

    for(i=0;i<14;i++){
        // Read 2 registers from adress 0, copy data to dest.
        ret = modbus_read_registers(ctx, cionreads[i].adr, cionreads[i].anz, dest);
        if(ret < 0){
	    fprintf(stderr, "read_regs failed: %s\n", modbus_strerror(errno));
            perror("modbus_read_regs error\n");
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
			printf("Zeit seit Ladebeginn : %d ms, HW:%d, LW:%d \n",((unsigned int)dest[1])*65536+dest[0],dest[1],dest[0]);
			break;
			case 301:
			printf("Modul Versorgungsspannung : %d mV\n",dest[0]);
			break;
			case 302:
			printf("Netzspannung : %d V\n",dest[0]);
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
    sleep(1);
    modbus_close(ctx);
    modbus_free(ctx);
    return(0);
}
