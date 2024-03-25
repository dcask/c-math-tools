/************************************************/
/*    Чтение секторов регистратора              */
/*   Параметры : start_sector sectors_to_read   */
/*               [-d device] [-p portId]        */
/*   device - "/dev/устройство"                 */
/*   portId - 1,2,4,8 (комммутация)             */
/************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

const sectorsize=169; // размер сектора регистратора
unsigned char iodata[516]; // буфер
struct termios term;
int fd;
char* application; // имя рабочего файла

/*ошибка*/
void die(const char* msg)
{
	char tmp[100];
	sprintf(tmp,"%s: %s",application,msg);
	if(!strcmp(msg,"")) perror(application);
	else puts(tmp);
	tcflush(fd, TCIOFLUSH);
	exit(1);
}
/*отправка данных из буфера*/
void senddata(int len)
{
	int i, rw;
	if (tcflush(fd, TCIOFLUSH))  die("");
	rw = write(fd, iodata, len);
	if (rw < 0)  die("");
	if (rw != len)
	{
		fprintf(stderr, "error writing: %u bytes instead of %u\n", rw, i);
		die("");
	}
}
/* чтение данных, "i" реальное количество считаных байт*/
void readdata(int* i)
{
	int rw;
	iodata[0]=1;
	term.c_cc[VTIME] = 16;
	if (tcsetattr(fd, TCSANOW, &term))  die("");
	for (*i = 0; *i < sizeof(iodata); *i += rw)
	{
		rw = read(fd, iodata + *i, sizeof(iodata) - *i);
		if (rw < 0)  die("");
		if (!rw)  break;
		if (term.c_cc[VTIME] != 1)
		{
			term.c_cc[VTIME] = 1;
			if (tcsetattr(fd, TCSANOW, &term))  die("");
		}
	}
	//проверка наличия ответа
	if (!*i) die("no answer");
	if (iodata[0]!=0) {fprintf(stderr,"code=%d ",iodata[0]);die("device error");}
}


int main(int argc, char *argv[])
{
	// параметры
	const char *dev="/dev/ttyS0" ;
	int startsec=0; // начальный сектор
	int qsec=1; // количество секторов
	int portId=-1;
	
	//переменные для работы с данными
	int er,rw;
	unsigned int i,j;
	char sztmp[4];
	unsigned short ustmp; 
		
	//разбор опций командной строки
	char c;
	while ((c = getopt(argc, argv, "d:p:")) != -1)
		switch (c)
		{
			case 'd':  dev = optarg;
				break;
			case 'p':  portId = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s [-d device] [-p portId] start_sector sectors\n", argv[0]);
				return 1;
		}
	if (argc <3 || argc >7)
	{
		fprintf(stderr, "Usage: %s [-d device] [-p portId] start_sector sectors\n", argv[0]);
		return 1;
	}
	startsec=atoi(argv[optind]);
	qsec=atoi(argv[optind+1]);
	application=argv[0];
	//открытие файла устройства
	if ((fd = open(dev, O_RDWR | O_NONBLOCK)) == -1 ||
		fcntl(fd, F_SETFL, 0)                       ||
		tcflush(fd, TCIOFLUSH)                      ||
		tcgetattr(fd, &term))
		die("open device");

	//настройка порта
	cfmakeraw(&term);
	term.c_cflag &= ~(CSTOPB | CLOCAL);
	term.c_cflag |= CRTSCTS | HUPCL;
	term.c_cc[VMIN] = 0;
	if (cfsetospeed(&term, B1200) ||
		cfsetispeed(&term, B1200) ||
		tcsetattr(fd, TCSAFLUSH, &term))
		die("cfsetospeed");
  
	//настройка коммутатора
	if(portId!=-1)
	{
		iodata[0]=33;iodata[1]=33;
		iodata[2]=portId;iodata[3]=0;
		iodata[4]=portId;
		senddata(5);
		readdata(&rw);
	}
	
	// чтение секторов
	for(i=startsec; i<qsec+startsec; i++)
	{
		//команда 14 регистратору
		iodata[0]=14;iodata[1]=14;
		memcpy(&iodata[4],&i,4);
		ustmp=0;
		for(j=4;j<8;j++) ustmp+=iodata[j];
		memcpy(&(iodata[2]),&ustmp,2);
		//чтение текущего сектора
		senddata(8);
		readdata(&rw);
		//команда 15 регистратору
		iodata[0]=15; iodata[1]=15;
		senddata(2);
		readdata(&rw);
		// побайтово на поток выхода
		er=fwrite(&iodata[4],1,rw-4,stdout);
		if(er!=rw-4) die("stdout error");
	}
	tcflush(fd, TCIOFLUSH);
	return 0;
}

