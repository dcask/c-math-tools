/************************************************************/
/*    Поиск среднего значения для входного потока           */
/*    Параметры:  samples gain adc_coef                     */
/*    samples  - количество входных отсчётов                */
/*    gain     - усиление                                   */
/*    adc_coef - единица АЦП                                */
/*    Вход : поток значений int                             */
/*    Выход : среднее_значение                              */
/************************************************************/

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

/* чтение данных из входного потока*/
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
/* поиск среднего в буффере */
int average(int *b, int s)
{
	int* pBuf=b;
	double avg=0.0;
	for(;pBuf<b+s; avg+=*(pBuf++)); // average
	avg/=s;
	return (int)avg;
}

int main(int argc, char* argv[])
{
	int* buff;		// буффер
	int samples;		// количество отсчётов
	double gain;		// усиление
	double adc_coef;	// единица АЦП
	if(argc!=4)
	{
		fprintf(stderr, "Usage: %s samples gain adc_coef\n", argv[0]);
		return 1;
	}
	
	samples = atoi(argv[1]);
	gain = atof(argv[2]);
	adc_coef = atof(argv[3]);
	buff=(int*)malloc(4*samples);

	readf(buff,4,samples,stdin);
	fprintf(stdout,"%.2f\n",average(buff,samples)/gain*adc_coef/1000);
	return 0;
}
