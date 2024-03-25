/********************************************************/
/*        Поиск первого нелинейного значения            */
/*     Параметры : begin end                            */
/*     begin - от данного значения                      */
/*     end   - до данного                               */
/*     Вход  :  массив символов формата                 */
/*          "напряжение напряжение\n..."                */
/*     Выход : точка нелинейности                       */
/********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	char st[50];	// буффер для чтения строки
	double val1=0.0, val2=0.0;
	double begin,end;
	if(argc!=3)
	{
		fprintf(stderr,"Usage: %s begin end\n", argv[0]);
		return 1;
	}
	begin=atof(argv[1]);
	end=atof(argv[2]);
	while(!feof(stdin))
		if(fgets(st,49,stdin))
		{
			//if(strlen(st)<2) break;
			val1=atof(st);
			val2=atof(strstr(st," "));
			if(val1>=begin && val1<=end)
				/*признак - отличие второго значения от первого на 1% */
				if(val1<val2-val2/100.0 || val1>val2+val2/100.0 )
					{fprintf(stdout,"%d\n",val1);return 0;}

		}
		
	if(ferror(stdin))
	{
		perror("stdin");
		return 1;
	}
	puts("none\n");
	return 0;
}
