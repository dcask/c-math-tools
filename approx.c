/*****************************************************/
/*     Построение кусочно-линейной функции           */
/*     Если разница между соседними значениями       */
/*     менее параметра "width", промежуточные        */ 
/*     значения линейно апроксимируются              */
/*     Параметры : [-f file] width                   */
/*     file - файл для записи в формате recview      */
/*     width - минимальная ширина интервалов         */
/*     Выход : каждая строка "значение1 значение2\n" */
/*     Вход : каждая строка "значение1 значение2\n"  */
/*****************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	FILE *fout=NULL;
	
	int n=0;
	int value;
	
	double val1=0.0;	// текущее значение1
	double val2=0.0;	// текущее значение2
	double pval1=0.0;	// предыдущее значение1
	double pval2=0.0;	// предыдущее значение2
	double width;		// минимальная ширина интервалов
		
	char st[50];		// буффер для чтения строки из потока
	char* filename=NULL;	// имя файла для записи  значений2 в формате recview
	char c;
	
	while ((c = getopt(argc, argv, "f:")) != -1)
		switch (c)
		{
			case 'f': filename=optarg;
				break;
			default:
				fprintf(stderr, "Usage: %s [-f file] width\n", argv[0]);
				return 1;
		}
	if(argc!=2&&argc!=4)
	{
		fprintf(stderr,"Usage: %s [-f file] width\n", argv[0]);
		return 1;
	}
	
	if(filename) fout=fopen(filename,"w");
	width=atof(argv[optind]);
	
	while(!feof(stdin))
		if(fgets(st,49,stdin))
		{
			//if(strlen(st)<2) break;		// fail
			pval1=val1;	pval2=val2;			// перенос предыдущих значений
			val1=atof(st);	val2=atof(strstr(st," "));	// чтение текущих значений
			n++;	
			if(n!=1)
				if(abs(val1-pval1)>width)
				{
					int i;
					int c=(int)(abs(val1-pval1)/width);	// количество разбиений
					c+= (abs(val1-pval1)/width-c) !=0;
					for(i=1; i<c; i++)	// построение линии между текущими и предыдущими значениями
					{
						if(fout) { value=(int) (pval2+(val2-pval2)/c*i); fwrite(&value,4,1,fout); }
						fprintf(stdout,"%.0f %.0f\n",pval1+(val1-pval1)/c*i,pval2+(val2-pval2)/c*i);
					}
				}
			fprintf(stdout,"%.0f %.0f\n",val1,val2);
			if(fout) { value=(int) val2; fwrite(&value,4,1,fout); }
		}
		
	if(ferror(stdin))
	{
		perror("stdin");
		return 1;
	}
	return 0;
}
