/******************************************************************/
/*    Построение спектра с помощью   Fourier Tranformation        */
/*    поиск максимума в спектре                                   */
/*    Параметры : [-f file] ADC_rate samples                      */
/*    rate      - частота АЦП                                     */
/*    samples   - количество сэмплов                              */
/*    file      - файл для записи в формате recview               */
/*    Вход : поток отсчётов                                       */
/*    Выход : непрерывный кусок размера uninterrupted             */
/******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

int samples=0;		// количество отсчётов
int rate=0;		// частота АЦП
char *application;
/*ошибка*/
void die(const char* msg)
{
	char tmp[100];
	sprintf(tmp,"%s: %s",application,msg);
	if(!strcmp(msg,"")) perror(application);
	else puts(tmp);
	exit(1);
}
/*чтение отсчётов из потока*/
void readf(void* buff, size_t size, size_t count, FILE* f)
{
	int rb=1,len=count;
	while(rb)
	{
		rb=fread(buff+len-count,size,count,f);
		count-=rb;
	}
	if(!ferror(f)&&!count)	return ;
	die("stdin");
}
/*Фурье-преобразование*/
void dft(const int *samp, int size, double *re, double *im)
{
	int j, k;
	for (j = 0; j < size/2; j++)
	{
		for (k = 0; k < size; k++)
		{
			double tmp = 2.0 * M_PI * j * k / size;
			re[j] += samp[k] * cos(tmp);
			im[j] += samp[k] * -sin(tmp);
		}
		im[j] = sqrt(re[j] * re[j] + im[j] * im[j]) /size;
		re[j]=0.0;
	}
}


int main(int argc, char *argv[])
{
	
	FILE* f=NULL;
	
	int n=0;
	int *src;
	int max=0,pmax;
	int i=0;
	
	double incr;
	double *re, *im;
	
	char c;
	char *filename=NULL;
	
	while ((c = getopt(argc, argv, "f:")) != -1)
		switch (c)
	{
		case 'f':  filename=optarg;
				break;
		default:
			fprintf(stderr, "Usage: %s [-f filename]  ADC_rate samples\n", argv[0]);
			return 1;
	}
	if(argc<3||argc>5)
	{
		fprintf(stderr, "Usage: %s [-f filename]  ADC_rate samples\n", argv[0]);
		return 1;
	}
	
	rate=atoi(argv[optind]);
	samples=atoi(argv[optind+1]);
	
	if (samples <= 1 || rate <= 1)
	{
		fprintf(stderr,"%s: Samples quantity or ADC rate can't be less or equal 1\n",argv[0]);
		return 1;
	}
	if (samples < rate)
	{
		fprintf(stderr,"%s: Samples quantity can't be less than ADC rate\n",argv[0]);
		return 1;
	}
	
	application=argv[0];
	src = (int*) calloc(samples,sizeof(int));		
	re =(double*) calloc(samples, sizeof(double));
	im =(double*) calloc(samples, sizeof(double));

	if (!src||!re||!im)
	{
		perror(argv[0]);
		return 1;
	}
	
	readf(src, 4, samples, stdin);
		
	dft(src, samples, re, im);
	incr = (double)samples / rate;
	for (i = 0; i < samples/2; i++)
	{
		double d = i / incr;
		int pos = d;
		pos += d - pos >= 0.5;
		re[pos] += im[i];
	}
	// поиск максимума
	for(i=0; i<rate/2; ++i)
	{
		src[i]=(int) re[i];
		if( max<abs(src[i]) )
		{
			pmax=i;
			max=abs(src[i]);
		}
	}
	/*запись спектра в файл в формате recview*/
	if(filename)
	{
		f = fopen(filename, "w");
		if(!f) die(filename);
		if(fwrite(src,rate/2*4,1,f)!=1)	die("error writing");
	}
	fprintf(stdout,"%d %d\n",pmax,max*4);
	return 0;
}
