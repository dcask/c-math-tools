/********************************************************/
/*        Вычисление уровня шума                        */
/*     Параметры : adc_coef gain samples                */
/*     adc_coef - единица АЦП                           */
/*     gain - усиление                                  */
/*     samples - количество отсчётов                    */
/*     Вход  :  поток отсчётов                          */
/*     Выход : уровень_шума                             */
/********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

int* buff;			// буффер
double adc=1.0;			// единица АЦП 
double gain=1.0;		// усиление 
int samples=0;			// количество отсчётов

/* чтение отсчётов из потока */
void readf(void* buff, size_t size, size_t count, FILE* f)
{
	int rb=1,len=count;
	while(rb!=0 && count)
	{
		rb=fread(buff+len-count,size,count,f);
		count-=rb;
	}
	if(!count||!ferror(f))
	{
		perror("reading");
		exit(1);
	}
}
/*среднеквадратичное отклонение*/
double noiseLevel(int *b, double a, double g, int s)
{
	int* pBuf=b;
	double avg=0.0;
	for(;pBuf<b+s; avg+=*(pBuf++)); // среднее
	avg/=s;
	double noise=0.0;
	pBuf=b;
	for(;pBuf<b+s;++pBuf)		// среднеквадратичное
		noise+=(double)pow(*pBuf-avg,2);
	return sqrt(noise/s)*a/g;
}

int main(int argc, char* argv[])
{
	
	if(argc!=4)
	{
		fprintf(stderr, "Usage: %s adc_coef gain samples\n", argv[0]);
		return 1;
	}
	adc=atof(argv[1]);
	gain=atof(argv[2]);
	samples=atoi(argv[3]);
	buff=(int*)malloc(4*samples);
	int* pBuf=buff;
	/*Reading from stdin*/
	readf(buff,4,samples,stdin);
	fprintf(stdout,"%.2f\n",noiseLevel(buff,adc,gain,samples));
   	return 0;

}
