/********************************************************/
/*        Удаление невалидных отсчётов                  */
/*     Параметры : [-t file] samples                    */
/*     samples - количество отсчётов                    */
/*     file    - файл тэгов                             */
/*     Вход  :  поток отсчётов                          */
/*     Выход : поток отсчётов                           */
/********************************************************/

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FILE_BUF 1024
int	buff[FILE_BUF];		// буффер
char* application;		// имя собственного файла

/*Ошибка*/
void die(const char* msg)
{
	char tmp[100];
	sprintf(tmp,"%s: %s",application,msg);
	perror(tmp);
	exit(1);
}
//Возвращает количество неактуальных отсчетов (непрерывный блок).
//Массив тэгов отсчетов трассы = tags, номер отсчета, с которого начать
//поиск = first, количество отсчетов для поиска = count.
//Если возвращается не 0, конец блока неактуальности не гарантируется.
static int nonactual(const unsigned char *tags, int first, int count)
{
	//текущая позиция просмотра тэгов
	const unsigned char *t = tags + first / 8;
	int i = 1 << first % 8;
	//обратный счетчик отсчетов
	int n = count;
	//поиск первого байта, в котором есть хоть один актуальный отсчет
	if (i == 1)
	{
		for (n /= 8; n; n--) if (*t++ != 255)  break;
		t -= n != 0;
		n = n * 8 + count % 8;
	}
	//подсчет количества тэгов неактуальности в первом [найденном] байте
	for (; i != 256 && n && (*t & i); i <<= 1)  n--;
	//может быть продолжение блока в следующем байте, если поиск шел не с бита 0
	return count - n;
}
//Возвращает количество актуальных отсчетов (непрерывный блок).
//Массив тэгов отсчетов трассы = tags, номер отсчета, с которого начать
//поиск = first, количество отсчетов для поиска = count.
static int actual(const unsigned char *tags, int first, int count)
{
	//текущая позиция просмотра тэгов
	const unsigned char *t = tags + first / 8;
	int i = 1 << first % 8;
	//обратный счетчик отсчетов
	int n = count;
	//подсчет количества тэгов актуальности в первом байте
	for (; i != 256 && n && !(*t & i); i <<= 1)  n--;
	//подсчет количества актуальных отсчетов в следующих байтах
	if (i == 256)
	{
		t++;
		//поиск первого байта с тэгом неактуального отсчета
		for (i = n / 8; i; i--) if (*t++)  break;
		t -= i != 0;
		n = i * 8 + n % 8;
		//подсчет количества тэгов актуальности в первом найденном байте
		for (i = 1; n && !(*t & i); i <<= 1)  n--;
	}
	return count - n;
}
//чтение с записью(flag=1) или без(flag=0)
void readwrite(int n,int flag)
{
	int rb=0,r;	
	while(!feof(stdin) && rb!=n)
	{
		r=fread(buff,4,n-rb > FILE_BUF ? FILE_BUF : n-rb,stdin);
		if(flag) 
			if(fwrite(buff,4,r,stdout)!=r) die("writing error");
		rb+=r;
	}
	if(ferror(stdin)) die("stdin");
	//if(n!=rb) {fprintf(stderr,"%d %d ", n,rb);die("Bad size");}
}
void noHoles(int n,unsigned char* tags,int tmgsize)
{
	int smp=0;
	int ac,na;
	int rb=0;
	while(smp<n)
	{
		if(smp<tmgsize)
		{
			na=nonactual(tags,smp,tmgsize-smp);
			smp+=na;
			readwrite(na,0);
			ac=actual(tags,smp,tmgsize-smp);
			readwrite(ac,1);
			smp+=ac;
		}
		else
		{
			readwrite(n-tmgsize,1);
			break;
		}
	}
}
int main(int argc, char* argv[])
{
	char c;
	char *addr=NULL;
	int fd=-1;
	struct stat st;
	st.st_size=0;
	int samples=0;			// количество отсчётов
	char* file=NULL;		// файл тэгов
	
	while ((c = getopt(argc, argv, "t:")) != -1)
		switch (c)
		{
			case 't':  file = optarg;
				break;
			
			default:
				fprintf(stderr, "Usage: %s [-t tags_file] samples\n", argv[0]);
				return 1;
		}
	if(argc!=4&&argc!=2)
	{
		fprintf(stderr, "Usage: %s [-t tags_file] samples\n", argv[0]);
		return 1;
	}
	samples=atoi(argv[optind]);
	if(file)
	{
		fd = open(file, O_RDONLY);
		if(fd==-1) die("Can't open tags file");
		if (fstat(fd, &st) == -1) die("fstat");
		if (st.st_size!=0)
		{
			addr = mmap(NULL, st.st_size, PROT_READ,MAP_SHARED, fd, 0);
			if (addr == MAP_FAILED) die("mmap");
		}
	}
	noHoles(samples,(unsigned char*) addr, st.st_size*8);
   	return 0;
}



