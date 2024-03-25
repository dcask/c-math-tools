/********************************************************/
/*        Анализ АЧХ                                    */
/*     Параметры : [-f file] passband adc_freq 3dB_freq */
/*                             ADC_coef                 */
/*     passband - начало завала                         */
/*     adc_freq - частота АЦП                           */
/*     3dB_freq - точка для определения                 */
/*     ADC_coef - единица АЦП в милливольтах            */
/*     file     - файл для записи АЧХ в формате recview */
/*     Вход  :  массив символов формата                 */
/*          "частота амплитуда\nчастота амплитуда\n..." */
/*     Выход : "разброс амплитуд" от начала до          */
/*        частоты passband                              */
/*        -дБ в точке 3dB_freq of the spectrum          */
/*        -дБ в точке определения                       */
/********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

int main(int argc, char* argv[])
{
	FILE *fout=NULL;	// файл для записи АЧХ в формате recview
	
	//int step;		// шаг частот для заданной частоты АЦП
	//int freq;		// предполагаемая частота для чтения
	int fr;			// прочитанное из файла очередное значение частоты
	int f3dB;		// точка определения
	int adc_f;		// чатота АЦП
	int pbandp;		// частота начала завала
	int tmp;	
	
	double amp;		// прочитанное из файла очередное значение амплитуды
	double max=0.0;		// максимальная амплитуда
	double min=0.0;		// минимальная амплитуда
	double sband=0.0;	// амплитуда на границе спектра
	double a3dB=0.0;	// амплитуда в точке определения
	double ADCcoef=0.0;// единица АЦП в миливольтах
	
	char *filename=NULL;	// имя файла для записи АЧХ
	char st[50];		// буффер для чтения строки из потока
	char c;	
	
	while ((c = getopt(argc, argv, "f:")) != -1)
		switch (c)
		{
			case 'f':  filename=optarg;
				break;
			default:
				fprintf(stderr, "Usage: %s [-f file] passband adc_freq 3dB_freq ADC_coef\n", argv[0]);
				return 1;
		}
		
	if(argc!=5&&argc!=7)
	{
		fprintf(stderr,"Usage: %s [-f file] passband adc_freq 3dB_freq ADC_coef\n", argv[0]);
		return 1;
	}
	
	if(filename) fout=fopen(filename,"w");
	pbandp=atoi(argv[optind]);
	adc_f=atoi(argv[optind+1]);
	f3dB=atoi(argv[optind+2]);
	ADCcoef=atof(argv[optind+3]);
	//step=adc_f/250;
	//freq=step;
	
	while(!feof(stdin))
		if(fgets(st,49,stdin))	// считать строку из файла
		{
			//if(strlen(st)<4) continue;	// fail
			fr=atoi(st);			// первое числовое значение в строке
			amp=atof(strstr(st," "));	// второе числовое значение в строке
			/*if(fr!=freq)
				fprintf(stderr,"Frequency %d expected\n",freq);*/
			if((fr>=f3dB)&&(a3dB==0.0)) a3dB=amp;
			if(fr==adc_f/2-1) sband=amp;	
			if(fr<=pbandp)
			{
				if(max==0.0)
				{
					min=amp;max=amp;
				}
				else
				{
					if(amp>max) max=amp;
					if(amp<min) min=amp;
				}
			}
			//freq+=step;
			tmp=(int) amp;
			if(!fout) fwrite(&tmp,4,1,fout);
		}
	if(ferror(stdin))
	{
		perror("stdin");
		return 1;
	}
	a3dB=10*log( ( (max-min)/2+min )/a3dB );
	sband=10*log( ( (max-min)/2+min )/sband );
	ADCcoef/=1000.0;
	fprintf(stdout,"%.2f\t|\t%dHz=%.2fDb\t|\t%dHz=%.2f\n",(max-min)*ADCcoef,f3dB,a3dB*ADCcoef,adc_f/2,sband*ADCcoef);
	return 0;
}
