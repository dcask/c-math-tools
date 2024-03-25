/***************************************************/
/*     Поиск разброса между максимальным и         */
/*     минимальным значениями во входящем потоке   */
/*     Вход : символьный массив                    */
/*         "число\nчисло\n..."                     */
/*     Выход : разброс                             */
/***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

int main(int argc, char* argv[])
{
	char st[50];	// буффер для чтения строк
	int flag=1;
	double max=0.0,min=0.0;
	double curr;
	
	while(!feof(stdin))
		if(fgets(st,49,stdin))
		{
			//if(strlen(st)<2) break;
			curr=atof(st);
			if(flag){min=max=curr;flag=0;}
			if(curr<min) min=curr;
			if(curr>max) max=curr;
		}
	if(ferror(stdin))
	{
		perror("stdin");
		return 1;
	}
	fprintf(stdout,"%.0f\n",max-min);
	return 0;
}
