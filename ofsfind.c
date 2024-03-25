/********************************************************/
/*        Поиск импульса от начала трассы               */
/*     Параметры : [-i файл-снимок] [-t тэг-файл]       */
/*         длина-трассы количество-трасс                */	
/*         смещение ширина-импульса период-импульса     */
/*     Вход  :  поток отсчётов                          */
/*     Выход : для каждой трассы строка вида            */
/*               "смещение амплитуда\n"                 */
/********************************************************/
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

int buff[BUF_SIZE];	// буффер чтения файла
// обработка ошибок
void die(const char* msg)
{
	perror(msg);
	exit(1);
}
// чтение из потока с/без поиском максимума и записью данных в файл
void readIn(int n, int* max,int* pmax, FILE* tofile)
{
	int r, rb=0, i; 
	while(!feof(stdin) && rb!=n)
	{
		r=fread(buff,4,n-rb > BUF_SIZE ? BUF_SIZE : n-rb,stdin);
		/*поиск максимума*/
		if(pmax)
		{
			*max=0;
			for(i=0; i<r; ++i)
				if( *max<abs(buff[i]) )
				{
					*pmax=i+rb;
					*max=abs(buff[i]);
				}
			if(tofile)
				if(fwrite(buff,4,r,tofile)!=r) die("impulse writing");
		}
		rb+=r;
	}
	if(n!=rb) die("Bad size");
}
//Возвращает количество актуальных отсчетов (непрерывный блок).
//Массив тэгов отсчетов трассы = tags, номер отсчета, с которого начать
//поиск = first, количество отсчетов для поиска = count.
static int actual(const unsigned char *tags, int first, int count)
{
	if(!tags) return count;
	const unsigned char *t = tags + first / 8;
	int i = 1 << first % 8;
	int n = count;
	for (; i != 256 && n && !(*t & i); i <<= 1)  n--;
		if (i == 256)
		{
			t++;
			for (i = n / 8; i; i--) if (*t++)  break;
			t -= i != 0;
			n = i * 8 + n % 8;
			for (i = 1; n && !(*t & i); i <<= 1)  n--;
		}
	return count - n;
}

/*
int size -длина трассы, 
int ofs - смещение от начала трассы до начала импульса
int nw- ширина исследуемого участка
int slen - перион подаваемого импульса
const unsigned char *tags - тэги
int* max, int* pmax - запись найденного максимума
const char* image - имя файла для записи образа импульса
int curr - текущая трасса
*/

void findimp(int size, int ofs,int nw,int slen,  const unsigned char *tags,int* max, int* pmax, const char* image, int curr)
{
	int psamp=0;	// позиция во входном потоке и файле тэгов
	int i=0;
	FILE *f=NULL;	// дескриптор
	readIn(ofs,NULL,NULL,NULL);	//прочитать offs отсчётов
	psamp+=ofs;
	/*поиск актуального куска*/
	while(actual(tags,psamp+curr*size,nw)!=nw) 
	{
		readIn(slen,NULL,NULL,NULL);
		psamp+=slen;
		if(psamp>size) die("Can't find any impulse");;
	}
	/*поиск максимума и запись образа импульса*/
	if (image)
	{
		f = fopen(image, "w");
		if(!f) die(image);
	}
	readIn(nw,max,pmax, f); // чтение импульса с записью
	if(f) fclose(f);
	psamp+=nw;
	readIn(size-psamp,NULL,NULL,NULL); // чтение остатка без записи
}
int main(int argc,char* argv[])
{
	int trlen=0;		// длина одной трасы
	int trcount=0;		// количество трасс
	int offs=0;		// смещение от начала трассы
	int nwidth=0;		// непрерывный кусок с предполагаемым экстремумом
	int slength=0;		// период повторения импульса
	int pmax=0,max=0;	// смещение и амплитуда в данной точке
	int i=0;
	int fd;			// файловый дескриптор
	
	struct stat st;		// свойства файла
	
	char *addr=NULL;	// указатель на область (mmap) отображения тэгов
	char* tfile=NULL;	// имя файла тэгов
	char* image=NULL;	// имя файла записи импульса в формате recview
	char c;
	
	while ((c = getopt(argc, argv, "i:t:")) != -1)
		switch (c)
		{
			case 'i':  image=optarg;
				break;
			case 't':  tfile=optarg;
				break;

			default:
				fprintf(stderr, "Usage: %s [-i файл-снимок] [-t тэг-файл] длина-трассы\tколичество-трасс\n"				\
			"смещение\tширина-импульса\n\t\tпериод-импульса\n", argv[0]);
				return 1;
		}

	if(argc<6 || argc >10)
	{
		fprintf(stderr, "Usage : %s [-i файл-снимок] [-t тэг-файл] длина-трассы\tколичество-трасс\n"				\
			"смещение\tширина-импульса\n\t\tпериод-импульса\n", argv[0]);
		return 1;
	}
	trlen=atoi(argv[optind]);		
	trcount=atoi(argv[optind+1]);		
	offs=atoi(argv[optind+2]);		
	nwidth=atoi(argv[optind+3]);		
	slength=atoi(argv[optind+4]);		
	
	// Файл тэгов опционально
	if(tfile)
	{
		fd = open(tfile, O_RDONLY);
		if(fd==-1) die("Can't open tags file");
		if (fstat(fd, &st) == -1) die("fstat");
		addr = mmap(NULL, st.st_size, PROT_READ,MAP_SHARED, fd, 0);
		if (addr == MAP_FAILED) die("mmap");
	}
	// Цикл по трассам. Для каждой трасы найти импульс
	for(i=0; i<trcount; ++i)
	{
		findimp(trlen,offs,nwidth,slength,(unsigned char*)addr,&max,&pmax,image, i);
		fprintf(stdout,"%d %d\n",pmax+1+offs, max);
	}
	return 0;
}
