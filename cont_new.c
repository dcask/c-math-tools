/******************************************************************/
/*    Поиск непрерывной последовательности валидных значений      */
/*                  in input stream                               */
/*    Параметры : [-t tags_file] samples uninterrupted            */
/*    tags_file - файл тэгов                                      */
/*    samples   - количество сэмплов                              */
/*    uninteruppted - количество,непрерывная последовательность   */
/*    Вход : поток отсчётов                                       */
/*    Выход : непрерывный кусок размера uninterrupted             */
/******************************************************************/

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 1024

int buff[BUF_SIZE];	// буффер чтения
int csamples=0;		// количество отсчётов в непрерывной области
int samples=0;		// всего отсчётов
char* file=NULL;	// файл тэгов
char* application;	// собственное имя файла

/* обработка ошибок*/
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
	while(!feof(stdin) && rb<n)
	{
		r=fread(buff,4,n-rb > BUF_SIZE ? BUF_SIZE : n-rb,stdin);
		if(flag) 
			if(fwrite(buff,4,r,stdout)!=r)	die("writing error");
		rb+=r;
	}
	if(ferror(stdin)) die ("stdin");
	if(n!=rb) die("Bad size");
}
// поиск непрерывности (входных отсчётов, непрерывный кусок, тэги, количество тэгов)
void cont(int n,int con, const unsigned char * tags, int tmgsize)
{
	int smp=0; 
	int na,ac;
	int max_ac=0; // максимальное количество отсчётов без разрыва
	int rb=0,rw=0;
	while(smp+con<tmgsize && smp+con <=n )
	{
		na=nonactual(tags,smp,con);
		smp+=na;
		ac=actual(tags,smp,con);
		if(max_ac<ac) {max_ac=ac;max_smp=smp;}
		if(ac==con) break;
		smp+=ac;
	}
	if(smp+con>n) //  die("no sequence");
	{
		readwrite(max_smp,0);
		readwrite(max_ac,1);
		readwrite(n-max_ac-max_smp,0);
	}
	else
	{
		readwrite(smp,0);
		readwrite(con,1);
		readwrite(n-con-smp,0);
	}
}
int main(int argc, char* argv[])
{
	const unsigned char *addr=NULL;
	struct stat st; st.st_size=0;
	int fd;
	char c;
	
	application=argv[0];	
	
	while ((c = getopt(argc, argv, "t:")) != -1)
		switch (c)
		{
			case 't':  file = optarg;
				break;
			default:
				fprintf(stderr, "Usage: %s [-t tags_file] samples uninterrupted\n\n", application);
				return 1;
		}
	
	if(argc!=5&&argc!=3)
	{
		fprintf(stderr, "Usage: %s [-t tags_file] samples uninterrupted\n", application);
		return 1;
	}
	
	samples=atoi(argv[optind]);
	csamples=atoi(argv[optind+1]);
		
	if (file)
	{
		fd = open(file, O_RDONLY);
		if(fd==-1) die("Can't open tags file");
		if (fstat(fd, &st) == -1) die("fstat");
		addr = mmap(NULL, st.st_size, PROT_READ,MAP_PRIVATE, fd, 0);
		if (addr == MAP_FAILED) die("mmap");
	}
	
	cont(samples,csamples,addr, st.st_size*8);
   	return 0;
}

