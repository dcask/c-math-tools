/******************************************************************/
/*    Обработка данных, полученных в результате тестов            */
/*    Создание отчётного файла на основе шаблона из файла         */
/*     "ptempl". В шаблоне происходит замена переменных вида      */
/*    @идентификаторНомер (@n1, @t3) Номер - номер канала[0..3]   */
/*    Каждое появление переменной заменяется следующим числовым   */
/*          значением из соответствующего файла                   */
/*    Файлы с данными по тестам:                                  */
/*         "/noise-Номер","/afcan-Номер","/impulse-Номер",        */
/*           "/g-Номер","/traces-Номер"                           */
/*    Переменные : @n - данные теста шумов                        */
/*    @t - данные по тесту смещения трасс                         */
/*    @a - данные по тесту АЧХ                                    */
/*    @i - данные по тесту импульсной характеристики              */
/*    @g - данные теста усилителя                                 */
/*    @id- идентификатор устройства                               */ 
/*    Параметры:  путь к директориям                              */
/******************************************************************/
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
//#include <sys/dir.h>
char *param=NULL;
const char filenames[][15]={"/noise-","/afcan-","/impulse-","/g-","/traces-"};	// обрабатываемые файлы
const char *n_stopped="/stopped" ;	// имя файла признак конца тестов
const char *n_checked="checked" ;	// строка признак конца обработки файлов
const char *n_templ="/ptempl" ;		// имя файла шаблона
const char *n_passport="/passport" ;	// имя выходого файла отчёта
const int QTEST=5;			// количество тестов
const int CHNL=4;			// количество каналов
volatile sig_atomic_t keep_going = 1;

/*Обработка ошибок*/
void die(char* msg)
{
	perror(msg);
	exit(1);
}
/*поиск и запись чисел в символьной строке*/
void parser(const char* source, double* values, int* count)
{
	*count=0;
	int begin=0, i;
	int len=strlen(source);
	char tmp[20];tmp[0]=0;
	for(i=0; i<len; ++i)
	{
		if(source[i]>47 && source[i]<58 || source[i]==46)
		{
			strncat(tmp,source+i,1);
			begin=1;
		}
		else /* конвертация */
			if(begin)
			{
				begin=0;
				values[(*count)++]=atof(tmp);
				tmp[0]=0;
			}
		if(source[i]==10) break;
	}
}
/* перебор существующих директорий и исполнение для каждой функции (*fcn)(char *)*/
void dirwalk(char *dir, void (*fcn)(char *))
{
	char name[PATH_MAX];
	struct dirent *dp;
	DIR *dfd;
	if ((dfd = opendir(dir)) == NULL)
	{
		fprintf(stderr, "dirwalk: не могу открыть %s\n", dir);
		exit(1);
    	}
    	while ((dp = readdir(dfd)) != NULL)
	{
		if (strcmp(dp->d_name, ".") == 0 
			|| strcmp(dp->d_name, "..") == 0)
			continue; /* пропустить себя и родителя */
		if (strlen(dir)+strlen(dp->d_name) + 2 > sizeof(name))
			fprintf(stderr, "dirwalk: слишком длинное имя %s/%s\n",
				dir, dp->d_name);
		else
		{
			sprintf(name, "%s/%s", dir, dp->d_name);
			if (dp->d_type==DT_DIR) (*fcn) (name);
			
		}
	}
	closedir(dfd);
}

/*преобразования файлов директория в выходной отчёт по шаблону? хранящемуся в файле ptempl*/
void routine(char *name)
{
	FILE *f, *res ,*fw, *ftemplate;	// дескрипторы для рез

	int cn[CHNL], ca[CHNL],ci[CHNL],cg[CHNL],ct[CHNL]; //счётчики переменных для каждого теста
	int i,n,t,pos;

	double data[QTEST][CHNL][100];	// хранилище числовых данных из обрабатываемых файлов
	
	char var[20];			// строка для конвертации текущей переменной
	char *varpos=NULL;		// позиция обнаруженной переменной
	char *strstrt=NULL;		// позиция в массиве символов шаблона для дальнейшей обработки
	char tmp[256];			// буффер чтения из файла
	char filename[200];		// строка для имён файлов
	char s_id[50];			// строка для хранения идентификатора устройства
	
	/* ищем файл stopped как признак окончания теста*/
	strcpy(filename,name);
	strcat(filename,n_stopped);
	f=fopen(filename,"r+");
	if(!f) return; // не обнаружен, следующий директорий
	
	/*проверка признака обработки*/
	fread(tmp,99 ,1,f);
	if(!strcmp(tmp,n_checked)) return; // уже обработан, следующий директорий
	
	/*чтение файла идентификации*/
	strcpy(filename,name); strcat(filename,"/id");
	fw=fopen(filename,"r");
	if(!fw) die("id");
	i=fread(s_id,1,49,fw); s_id[i-1]=0;
	fclose(fw);
	
	/*начальные значения счётчиков переменных*/
	for(i=0;i<CHNL; ++i) {cn[i]=1; ca[i]=1; ci[i]=1;cg[i]=1;ct[i]=1;}
	/*
	обработать файлы по 5 тестам
	  noise-0,noise-1,noise-2,noise-3
	  impulse-0,impulse-1,impulse-2,impulse-3
	  afcan-0,afcan-1,afcan-2,afcan-3
	  g-0,g-1,g-2,g-3,
	  traces-0,traces-1,traces-2,traces-3
	*/
	
	/*заполнение хранилища числовых данных*/
	for(t=0; t<QTEST; ++t)	// цикл по файлам
	{
		for(i=0; i<CHNL; ++i) // цикл по каналам
		{
			sprintf(tmp,"%s%d",filenames[t],i);
			strcpy(filename,name);
			strcat(filename,tmp);
			fw=fopen(filename,"r");
			if(!fw) continue;
			data[t][i][0]=0.0;
			while(!feof(fw))
			{
				pos=fread(tmp,255,1,fw); tmp[pos]=0;
				parser(tmp, data[t][i]+(int)(data[t][i][0])+1, &n);
				data[t][i][0]+=n;
			}
			fclose(fw);
		}
	}
	
	// использование файла-шаблона
	strcpy(filename,n_templ);
	ftemplate=fopen(filename,"r");
	if(!ftemplate) die("Template");
	strcpy(filename,name);
	strcat(filename,n_passport);
	res=fopen(filename,"w");
	if(!res) die("result file");
	int stop=0;
	
	/* поиск и замена переменных*/
	while(!feof(ftemplate))
	{
		n=fread(tmp,1,255,ftemplate);
		if(!n) continue;
		tmp[n]=0;
		varpos=tmp;strstrt=varpos;
		while (1)
		{
			varpos=strchr(strstrt,'@');
			if(!varpos) break;
			strncpy(var,varpos+1,2); // поиск следующей переменной
			var[2]=0;
			fwrite(strstrt,(int)(varpos-strstrt),1, res);
			strstrt=varpos+3;
			switch(var[0])
			{
				case 'n' : pos=0;n=cn[var[1]-48]++;break;
				case 'a' : pos=1;n=ca[var[1]-48]++;break;
				case 'i' : if(var[1]=='d') pos=5;else {pos=2;n=ci[var[1]-48]++;} break;
				case 'g' : pos=3;n=cg[var[1]-48]++;break;
				case 't' : pos=4;n=ct[var[1]-48]++;break;
				default: continue;
			}
			if(pos<5)
			{
				if (var[1]-48>=0&&var[1]-48<CHNL) 
					sprintf(var,"%.2f",data[pos][var[1]-48][n]); 
			}
			else strcpy(var,s_id);// конвертация
			fwrite(var,1,strlen(var), res);
		}
		fwrite(strstrt,strlen(tmp)-(int)(strstrt-tmp),1, res); // запись остатка массива
	}
	fclose(ftemplate);
	/*************************************/
	strcpy(tmp,n_checked);
	fprintf(stdout,"%s завершено\n",name);
	fwrite(tmp,strlen(tmp),1,f);
	fclose(f);
	fclose(res);
}

/*обработка сигнала*/
void catch_signal (int sig)
{
	//keep_going = 0; 
	dirwalk(param,routine);
	signal (sig, catch_signal);
}

int main(int argc, char* argv[])
{
	param=argv[1];
	/*
	signal (SIGALRM, catch_signal);
	while (1)
		alarm(600);
		pause();
	*/
	/*бесконечный цикл*/
	while(1)
		dirwalk(param,routine);
	return EXIT_SUCCESS;
}


