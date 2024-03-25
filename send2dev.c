/************************************************/
/*    Отправка и чтение данных                  */
/*   Параметры : -d device -p portId            */
/************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

const sectorsize=169; // размер сектора регистратора
unsigned char iodata[33 + 512]; // буфер
unsigned char udata[1024]; 
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
	//переменные для управления последовательным портом
	const char *dev = "/dev/ttyS0";
	int portId=-1;
	
	//переменные для работы с данными
	char *cur, *inv;
	int i, j, k, rw;
	int rows, cols;
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
				fprintf(stderr, "Usage: %s [-d device] [-p portId]\n", argv[0]);
				return 1;
		}
	application=argv[0];

	//открытие файла устройства
	if ((fd = open(dev, O_RDWR | O_NONBLOCK)) == -1 ||
		fcntl(fd, F_SETFL, 0)                       ||
		tcflush(fd, TCIOFLUSH)                      ||
		tcgetattr(fd, &term))
		die("");

	//настройка порта
	cfmakeraw(&term);
	term.c_cflag &= ~(CSTOPB | CLOCAL);
	term.c_cflag |= CRTSCTS | HUPCL;
	term.c_cc[VMIN] = 0;
	if (cfsetospeed(&term, B1200) ||
		cfsetispeed(&term, B1200) ||
		tcsetattr(fd, TCSAFLUSH, &term))
		die("");
  
	//настройка коммутатора
	if(portId!=-1)
	{
		iodata[0]=33;iodata[1]=33;
		iodata[2]=portId;iodata[3]=0;
		iodata[4]=portId;
		senddata(5);
		readdata(&rw);
	}
	//ввод строки
	if (!(cur = gets(udata)))
	{
		putchar('\n');
		tcflush(fd, TCIOFLUSH);
		return ferror(stdin) != 0;
	}
	
	//преобразование введенного текста в двоичный вид
	for (i = 0; *cur && i < 33; i++)
	{
		iodata[i] = (char)strtol(cur, &inv, 16);
		if (inv == cur)	die("stdin: invalid character");
		cur = inv;
	}
	
	//проверка ввода
	if (!i)  die("");
	if (i == 33) die("stdin: too many values");
	//передача запроса регистратору
	senddata(i);
	//получение ответа
	readdata(&i);
	//проверка наличия ответа
	if (!i) die("no answer");
	for(rw=0; rw<i;rw++) fprintf(stdout,"%d ",iodata[rw]);
	puts("\n");
	if(iodata[0]!=iodata[1] || iodata[0]) die("device error");
	tcflush(fd, TCIOFLUSH);
	return 0;
}

