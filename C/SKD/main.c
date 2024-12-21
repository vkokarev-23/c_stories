// АДИС (AFIS) Папилон https://www.papillon.ru/products/programs/adis/
//
//   Проект "СКД" - симуляция кипучей деятельности
// Действия каждого оператора в АДИС "Папилон" регистрируются
// в отдельных "именных" журналах - бинарных файлах особого формата,
// одно действие - одна новая запись.

//   Когда тебя и твоих товарищей заставляют заниматься бессмысленной работой
// ради изображения неких "показателей", тратить на это свою жизнь,
// не стоит пенять на судьбу, стоит посмотреть на ситуацию сбоку.
//   Хотите показателей - мы вам их "накрутим".


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ftw.h>

#define INITLOGOFFSET   0x8     // длина заголовка файла журнала
#define ARRCELLDIM      0x8     // строк в заголовке записи журнала, их 1-4 в действительности
#define LOGFLDSIZE      0x100   // LOGFLDSIZE 0x100 достаточно для коротких полей: c7 2c d2 d3 d4 d5 d7
#define STR256          0x100   // но мне попадались поля длиной 0x4a8: f9 fb
#define ERROR_FILE_OPEN -3
                          // КОДЫ ПОЛЕЙ В ЗАПИСИ ЖУРНАЛА
#define F_DAT2 0xd2     // имя файла дактилокарты
#define F_DAT3 0xd3     // имя файла следа
#define F_DAT4 0xd4     //
#define F_DAT5 0xd5     // код возврата
#define F_DAT7 0xd7     // продолжительность работы в секундах
#define F_PROC 0x12c    // имя процесса, модуля
#define F_STAT 0xc7     // статистика
#define F_BIG1 0x2f9    // два больших, 1-2К, поля, вероятно, сложной структуры
#define F_BIG2 0x2fb    // встречались при импорте, содержали заголовки ДК

#define COP_VIEW_PAIR_KSL 0x12  // пальцы просмотр пары К-С
#define COP_DELE_PAIR_KSL 0x13  // пальцы удаление пары К-С
#define COP_STRT_SESSION  0x4d  // сеанс начало
#define COP_TERM_SESSION  0x4e  // сеанс конец
#define COP_VIEW_PAIR_KST 0x84  // ладони просмотр пары К-С
#define COP_DELE_PAIR_KST 0x85  // ладони удаление пары К-С
#define COP_DELE_LIST_KSL 0xbe  // пальцы удаление списка К-С
#define COP_DELE_LIST_KST 0xc2  // ладони удаление списка К-С
#define COP_STRT_MODULE   0xfe  // модуль запущен
#define COP_TERM_MODULE   0xff  // модуль остановлен

#pragma pack(push, 1)
typedef struct  // структура ячейки в заголовке записи, их бывает от 1 до 4
{               // первая всегда типа 0xc7 stat. Бывают: d2 d3 d4 d5 d7 2c
    unsigned short int      cod;  // тип данных в ячейке
    unsigned short int      num;  // количество (чего? обычно 1)
    unsigned int            len;  // длина области данных
    unsigned int            ofs;  // сдвиг до области данных от начала файла
} typeLOGcell;

typedef                              // заголовок записи журнала
typeLOGcell tArrCell[ARRCELLDIM];    // массив c заведомым запасом ячеек

typedef struct // структура поля stst не в заголовке, а в области данных
{
    char                    sign[4]; // сигнатура всегда содержит "stat"
    unsigned char           vers;    // версия Папилона, в нашем случае 80
    unsigned char           csum;    // не знаю! это не ноль, и это опасно!
    unsigned short int      oper;    // код операции по справочнику
    unsigned int            utim;    // юникс-время
    unsigned int            utm0;    // не знаю, заполнитель до 16? всегда нули
    char           name[240];        // имя оператора, строка переменной длины
} typeStatRec;

typedef
unsigned char typeFLDbuff[LOGFLDSIZE]; // строка 256, с запасом

#pragma pack(pop)


//  =======================================================================================
//  Общие (глобальные) переменные

static char d_D2 [STR256]; // здесь будут данные для соответствующих полей
static char d_D3 [STR256];
static char d_D4 [STR256];
static char d_D5 [STR256];
static char d_D7 [STR256];
static char d_12C[STR256];
static char d_C7 [STR256];
static int  len_C7;


static const int      GoodCode = 0;
static const int      ErrCode = 1;
static const int      BadFileLen = -1;

static const long int initLOGoffset = INITLOGOFFSET;
unsigned short int    LOGnumCell;  // занято ячеек в массиве, обычно от 1 до 4
tArrCell              LOGarrCell;  // собственно массив ячеек
unsigned int          LOGnextOfs;  // позиция следующей записи от начала файла
unsigned int          RecOffset;   // позиция текущей записи от начала файла
unsigned int          FLDoffset;   // позиция поля данных от начала файла

typeStatRec           fldStat;            // поле со структурой stat

static time_t         myTime;           // текущее какбы-время какбы-работающего какбы-оператора
static time_t         TimeStartSession; // отсечка времени начала сеанса для вычисления его продолжительности
static time_t         TimeStartDBC;     // отсечка времени открытия базы для вычисления продолжительности
static char           str1[STR256];     // две строки для промежуточных вычислений
static char           str2[STR256];

static int            numKStodo = 0;    // порог, сколько списков К-С нужно обработать
static int            numKSdone = 0;    // счетчик, сколько списков К-С ужо обработано
static int            numPRdone = 0;    // счетчик, сколько пар в списках К-С обработано

static const char     modeBinRW[] = "rb+";  // режим работы с файлом "ЧТЕНИЕ-ЗАПИСЬ"
static const char     modeBinRO[] = "rb";   // режим работы с файлом "ТОЛЬКО-ЧТЕНИЕ"
FILE                 *LOGfile = NULL;  // файл Журнала
unsigned int          LOGsize;         // длина файла протокола
FILE                 *KSfile = NULL;   // файл рексписка К-С

enum LOGstatus { Active, inActive };

//  =======================================================================================
// возвращает или длину существующего файла, или BadFileLen = -1
int fileSize (const char fullPath[])
{
    struct stat fStatBuf;

    if (stat(fullPath, &fStatBuf) == BadFileLen)
        return BadFileLen;
    return fStatBuf.st_size;
};

//  =======================================================================================
// случайное число в заданном диапазоне мин-макс
int randInRange (const int min, const int max)
{
    return rand() % (max - min + 1) + min;
}

//  =======================================================================================
// преобразует юникс-время в строку ГГГГ-ММ-ДД ЧЧ:ММ:СС
int time2string (const time_t wTime, char *wString)
{
    struct tm   *timeRec;

    timeRec = localtime(&wTime);
    strftime(wString, STR256, "%Y-%m-%d %H:%M:%S", timeRec);
    return GoodCode;
}

//  =======================================================================================
// вытряхивает из строки заданный строковый образец
int strShakeOut (char *whence, const char *what)
{
    char buf[STR256] = "";
    int l=strlen(what);
    char *p;
    for ( p = strstr(whence, what); p != NULL; p = strstr(whence, what) )
    {
        *p='\0';
        p+=l;
        strcpy(buf,p);
        strcat(whence, buf);
    }
    return 0;
}

//  =======================================================================================
// открывает и закрывает журнал
// т.е. правит последнюю ссылку: 0 - закрыт, ENDFILE - открыт
int MakeLOG (enum LOGstatus LOGmode)
{
// RecOffset меняется скачками, всегда указывает на начало записи
// в начале файла перед первой записью есть заголовок файла длиной 8 байт
// FLDoffset указывает на поле данных LOGnextOfs, подлежащее корректировке
// LOGnextOfs - позиция следующей записи или 0 для последней записи в журнале
// 0 в LOGnextOfs - признак последней записи и конца журнала

    RecOffset = initLOGoffset;
    fseek (LOGfile, RecOffset, SEEK_SET);  // пропускаем заголовок 8 байт
    do
    {
        fread(&LOGnumCell, sizeof(LOGnumCell), 1, LOGfile);
        fread(&LOGarrCell, sizeof(typeLOGcell), LOGnumCell, LOGfile);
        FLDoffset = ftell(LOGfile); // запоминаем позицию ссылки на следующую запись
        fread(&LOGnextOfs, sizeof(LOGnextOfs), 1, LOGfile); // читаем саму ссылку

        RecOffset = LOGnextOfs;               // задаем позицию следующующей записи
        fseek (LOGfile, RecOffset, SEEK_SET); // встаем на нее
    }
    while ((RecOffset < LOGsize) && (RecOffset != 0)); // достигли EOF или последней записи

    unsigned int LOGlastOffset;
    if (LOGmode == inActive )
        LOGlastOffset = 0;
    else
        LOGlastOffset = LOGsize;

    fseek (LOGfile, FLDoffset, SEEK_SET);                      // встаем на поле ссылки на след.запись
    fwrite(&LOGlastOffset, sizeof(LOGlastOffset), 1, LOGfile); // пишем туда, что следует

    // если мы дошли до последней записи, а RecOffset != 0, значит журнал уже активирован
    // в любом случае RecOffset в итоге должен указывать на конец файла
    RecOffset = LOGsize;
    return GoodCode;
}

//  =======================================================================================
// некоторые замечания по поводу WriteLOG
// это универсальная функция записи в файл журнала, ее поведение задается матрицей действий
// матрица действий actFLDlin [строк][столбцов]: КОП поле1 поле 2 поле3 поле4 концевик 0x00
// КОП всегда заполнен, поле статистики 0xc7 всегда заполнено. если поле=0 - ничего не делай
// добавляя новые строки с новыми codOP, развиваем функционал

#define actFLDlin 8  // строк в таблице действий, последняя строка обязательно с нулями
#define actFLDcol 5  // столбцов в таблице действий: КОП + 4 поля + поле ограничителя с нулем

typedef struct
{
    int codOP;
    int logFLD[actFLDcol];
} type_actionFLD_line;

// это управляющий массив - важнейшая вещь !
// задает перечень и последовательность полей в записях журнала
// последняя строка и последняя колонка должны содержать только нули !

static const type_actionFLD_line actFLD[actFLDlin] =
{
//  codOP   FLD1  FLD2  FLD3  FLD4  FLD5
    {0x12, {0xc7, 0xd3, 0xd2, 0x00, 0x00}},
    {0x13, {0xc7, 0xd3, 0xd2, 0x00, 0x00}},
    {0x4d, {0xc7, 0x00, 0x00, 0x00, 0x00}},
    {0x4e, {0xc7, 0xd7, 0x00, 0x00, 0x00}},
    {0xbe, {0xc7, 0x12c,0x00, 0x00, 0x00}},
    {0xfe, {0xc7, 0x12c,0x00, 0x00, 0x00}},
    {0xff, {0xc7, 0x12c,0xd5, 0xd7, 0x00}},
    {0x00, {0x00, 0x00, 0x00, 0x00, 0x00}}
};

int WriteLOG (const unsigned short int COP, const int dt)
{
// при заполнении журнала RecOffset установлен на конец файла
// все приготовлено для добавления новой записи

    int todo, i, j;

    myTime += dt;

    todo = 0xffff;
    for (i=0; i<actFLDlin; i++)  // ищем управляющую строку с нашим КОП
        if (actFLD[i].codOP == COP)
        {
            todo = i;
            break;
        }
    if (todo == 0xffff) return ErrCode;  // не нашли, такого не должно быть

    LOGnumCell = 0xffff;
    for (j=0; j<actFLDcol; j++)
        if (actFLD[todo].logFLD[j] == 0)
        {
            LOGnumCell = j;
            break;
        }
    if (LOGnumCell == 0xffff) return ErrCode;
    // 0xffff полей - такое невозможно, хотя бы одно поле в записи должно быть

    // теперь определены и управляющая строка todo, и количество полей в ней LOGnumCell

    for (j=0; j<LOGnumCell; j++)
    {
        LOGarrCell[j].cod = actFLD[todo].logFLD[j];
        LOGarrCell[j].num = 1;
        // сначала считаем длины полей, а затем на их основе смещения
        switch (LOGarrCell[j].cod)
        {
        case F_DAT2:
            LOGarrCell[j].len = strlen(d_D2) + 1;
            break;
        case F_DAT3:
            LOGarrCell[j].len = strlen(d_D3) + 1;
            break;
        case F_DAT4:
            LOGarrCell[j].len = strlen(d_D4) + 1;
            break;
        case F_DAT5:
            LOGarrCell[j].len = strlen(d_D5) + 1;
            break;
        case F_DAT7:
            LOGarrCell[j].len = strlen(d_D7) + 1;
            break;
        case F_PROC:
            LOGarrCell[j].len = strlen(d_12C) + 1;
            break;
        case F_STAT:
            LOGarrCell[j].len = strlen(d_C7) + 1 + 16; // 16 - длина заголовка структуры stat
            break;
        default:
            break;
        }
    }
    for (j=0; j<LOGnumCell; j++)
    {
        switch (LOGarrCell[j].cod)
        {
        case F_STAT:
            LOGarrCell[j].ofs = sizeof(LOGnumCell) + LOGnumCell*sizeof(typeLOGcell) +
                                sizeof(LOGnextOfs) + RecOffset;
            break;
        default:
            LOGarrCell[j].ofs = LOGarrCell[j-1].len + LOGarrCell[j-1].ofs;
            break;
        }
    }
    LOGnextOfs = LOGarrCell[LOGnumCell-1].len + LOGarrCell[LOGnumCell-1].ofs;
    // итак, заголовок сформирован, строки данных тоже, осталось поле stat

    strncpy(fldStat.sign, "stat", 4); // сигнатура всегда содержит "stat"
    fldStat.vers = 0x80;              // версия Папилона, в нашем случае 80
    fldStat.csum = 0;                 // НЕ ЗНАЮ !!! ЗАВИСИТ ОТ СЛЕДУЮЩЕГО ЗА НИМ ПОЛЯ ВРЕМЕНИ. ЭТО КОНТРОЛЬНАЯ СУММА ?
    fldStat.oper = COP;               // код операции по справочнику
    fldStat.utim = myTime;            // юникс-время
    fldStat.utm0 = 0;                 // не знаю, обычно нули
    strcpy(fldStat.name, d_C7);       // имя оператора, строка переменной длины


    unsigned int   ctrlSumm;
    unsigned char *pByte;

    pByte = &fldStat.oper;
    ctrlSumm = 60;  // такой вот алгоритм: КС = (побайтная сумма + 60) % 0x100
    j = len_C7 + 10 + 6; // 10 - длина полей oper, utim, utm0
    for (i=6; i<j; i++)  //  6 - длина полей sign, vers, csum, - в КС не входят
       {
       ctrlSumm += *pByte;
       pByte++;
       }
    fldStat.csum = ctrlSumm % 256;

    // записываем сначала заголовок
    fseek (LOGfile, RecOffset, SEEK_SET); // встаем на хвост файла
    fwrite(&LOGnumCell, sizeof(LOGnumCell), 1, LOGfile);
    fwrite(&LOGarrCell, sizeof(typeLOGcell), LOGnumCell, LOGfile);
    fwrite(&LOGnextOfs, sizeof(LOGnextOfs), 1, LOGfile);

    // затем поля
    for (j=0; j<LOGnumCell; j++)
    {
        switch (LOGarrCell[j].cod)
        {
        case F_DAT2:
            fwrite(&d_D2, LOGarrCell[j].len, 1, LOGfile);
            break;
        case F_DAT3:
            fwrite(&d_D3, LOGarrCell[j].len, 1, LOGfile);
            break;
        case F_DAT4:
            fwrite(&d_D4, LOGarrCell[j].len, 1, LOGfile);
            break;
        case F_DAT5:
            fwrite(&d_D5, LOGarrCell[j].len, 1, LOGfile);
            break;
        case F_DAT7:
            fwrite(&d_D7, LOGarrCell[j].len, 1, LOGfile);
            break;
        case F_PROC:
            fwrite(&d_12C, LOGarrCell[j].len, 1, LOGfile);
            break;
        case F_STAT:
            fwrite(&fldStat, LOGarrCell[j].len, 1, LOGfile);
            break;
        default:
            break;
        }
    }
    RecOffset = LOGnextOfs; // обновляем указатель на хвост подросшего журнала
    return GoodCode;
}

//======================================================================================================
// Рекомендательный список К-С - это файл типа flh или tph, он состоит из элементов,
// каждый из которых описывает одну пару совпадения Карта-След. Элемент начинается
// с размера (длины) элемента, далее из интересного: адреса в базе данных совпавших
// Карты (Q запрос) и Следа (A ответ), еще дальше пары совпавших точек, которые мы
// здесь не рассматриваем вообще.
// В KSobserv сосредоточена вся обработка рексписка К-С

int KSobserv (const char *KSfilename)
{
#pragma pack(push, 1)
    typedef struct
    {
        unsigned short dbNum;     /* номер базы данных                */
        unsigned short segNum;    /* номер сегмента                   */
        unsigned int   fName;     /* номер файла                      */
        unsigned int   fAddr;     /* некий адрес, может быть delux    */
    } typeDBobj;

    typedef struct                /* Элемент списка К-С               */
    {
        unsigned int   Len;       /* длина элемента                   */
        char           WhatIs;    /* не знаю, что это                 */
        char           Version;   /* версия Папилона (80)             */
        char           Abbr[4];   /* flrn и другие разновидности      */
        typeDBobj      Q;         /* адрес Карты в БД - запрос        */
        typeDBobj      A;         /* адрес Следа в БД - ответ         */
    } typeKS;
#pragma pack(pop)

    typeKS     KS;          /* элемент в списке К-С                       */
    int        KSsize;      /* размер всего списка - сумма длин элементов */
    long int   KSoffset;    /* сдвиг текущего элемента от начала файла    */

    KSsize = fileSize(KSfilename); // Если с файлом проблемы, fileSize вернет -1, иначе - реальный размер
    if (KSsize == BadFileLen)
        return GoodCode;

    KSfile = fopen(KSfilename, modeBinRO);
    KSoffset = 0;
    do
    {
        fread(&KS, sizeof(KS), 1, KSfile);

        // обработка пары
        sprintf(d_D3, "%#06x.%#06x.%#010x.l", KS.Q.dbNum, KS.Q.segNum, KS.Q.fName);
        strShakeOut(d_D3, "0x");
        sprintf(d_D2, "%#06x.%#06x.%#010x.f", KS.A.dbNum, KS.A.segNum, KS.A.fName);
        strShakeOut(d_D2, "0x");
        WriteLOG(COP_VIEW_PAIR_KSL, 0);
        WriteLOG(COP_DELE_PAIR_KSL, randInRange(2, 8)); // на просмотр пары 2-4 секунды

        numPRdone++;             /* счетчик обработанных пар */
        KSoffset += KS.Len;   /* встаем на следующую позицию от начала файла */
        fseek (KSfile, KSoffset, SEEK_SET);
    }
    while (KSoffset < KSsize);
    fclose(KSfile);

    return GoodCode;
}

//======================================================================================================
// RazborKS - это функция-обработчик одного файла рексписка К-С
// вызывающая функция ftw перебирает рексписки сегмента и для каждого рексписка
// вызывает обработчик - RazborKS в нашем случае.
// Если обработчик возвращает 0, то обход продолжается.
// Возвращая не ноль, обработчик говорит ftw: "Хватит, кончай работу!"

int RazborKS(const char *fullKSpath, const struct stat *fileStatBuf, int flag)
{
//  if(flag == FTW_F && ((strstr(fullKSpath, ".flh") != NULL) || (strstr(fullKSpath, ".flh") != NULL)))
    if( flag == FTW_F && (strstr(fullKSpath, ".flh") != NULL) ) // только пальцы
    {
        KSobserv(fullKSpath);
        strcpy(d_12C, fullKSpath);      // "удаление" файла списка К-С
        WriteLOG(COP_DELE_LIST_KSL, 0);
        numKSdone++;                    // счетчик обработанных списков К-С
    };
    if (numKSdone < numKStodo)
        return GoodCode;
    else
        return ErrCode;
}

//  =======================================================================================
//    СОГЛАШЕНИЕ О ПАРАМЕТРАХ
// argv[0] имя программы
// argv[1] штук списков К-С, объем работы для оператора: 1000
// argv[2] каталог с рексписками: /papillon1.w/00250100.w/recom/
// argv[3] файл журнала, полный путь: /papillon1/report/stat8/stat8prot.aa
// argv[4] полное имя оператора: Касторкин Елисей Калистратович

int main (int argc, char *argv[])
{
    static char         LOGfileName[STR256];  // имя файла протокола
    int i;

//===============================================
// 0. Пролог
    if (argc != 5)
    {
        printf ("\n\n%s", "Программа требует 4 параметра:");
        printf (  "\n%s", "[1] штук списков К-С, объем работы для оператора: до 999");
        printf (  "\n%s", "[2] каталог с рексписками: /papillon1.w/00250100.w/recom/");
        printf (  "\n%s", "[3] файл журнала, полный путь: /papillon1/report/stat8/stat8prot.aa");
        printf (  "\n%s", "[4] полное имя оператора: Касторкин Елисей Калистратович");
        printf (  "\n\nПо факту получено %d параметров: \n\n", argc - 1);
        for (i=1; i<argc; i++)
            printf ("<%s> \n", argv[i]);
        return ErrCode;
    }

    numKStodo = atoi(argv[1]);
    numKStodo = numKStodo % 1000;

    strcpy(LOGfileName, argv[3]);
    LOGsize = fileSize(LOGfileName);
    if (LOGsize == BadFileLen)
        return ErrCode;
    LOGfile = fopen(LOGfileName, modeBinRW);
    MakeLOG (Active);  // убираем нулевую ссылку в последней записи журнала

    myTime = time(NULL); // начальное время сеанса = текущее время
    srand(time(NULL));   // запуск генератора случайных чисел, зерно = текущее время

    strcpy(d_C7, argv[4]);  // argv[4] - имя оператора, во время сеанса не изменяется
    len_C7 = strlen(d_C7);

//===============================================
// 1. Начало сеанса
    strcpy(d_12C, "q8.ppln");
    WriteLOG(COP_STRT_MODULE, 0);
    WriteLOG(COP_STRT_SESSION, 0);
    TimeStartSession = myTime; // понадобится в конце сеанса для вычесления длительности

    strcpy(d_12C, "x8.dbc");
    WriteLOG(COP_STRT_MODULE, randInRange(6, 20));
    TimeStartDBC = myTime;         // понадобится в конце сеанса для вычесления длительности
    myTime += randInRange(20, 40); // задержка перед просмотром первой пары

//===============================================
// 2. Основной этап - обработка списков
    char      segPath[STR256];
    const int maxOpenDirs = 1;
    strcpy(segPath, argv[2]);
    ftw(segPath, RazborKS, maxOpenDirs);

//===============================================
// 3. Завершение сеанса
    myTime += randInRange(8, 20);
    sprintf (str1, "%ld", myTime - TimeStartDBC);
    strcpy(d_12C, "x8.dbc");
    strcpy(d_D5,  "0");
    strcpy(d_D7,  str1);
    WriteLOG(COP_TERM_MODULE, 0);

    myTime += randInRange(8, 20);
    sprintf (str1, "%ld", myTime - TimeStartSession);
    strcpy(d_12C, "q8.ppln");
    strcpy(d_D5,  "0");
    strcpy(d_D7,  str1);
    WriteLOG(COP_TERM_MODULE, 0);
    WriteLOG(COP_TERM_SESSION, 0);
    fclose(LOGfile);

//===============================================
// 4. Эпилог
    // если журнал не закрыть, то неправильно отрабатывает fileSize
    LOGsize = fileSize(LOGfileName);
    LOGfile = fopen(LOGfileName, modeBinRW);
    MakeLOG (inActive);  // ставим нулевую ссылку в последней записи журнала
    fclose(LOGfile);

    time2string (TimeStartSession, str1);
    time2string (myTime, str2);

    printf ("\nОбработано списков К-С: %d ", numKSdone);
    printf ("\nВ списках содержится %d пар", numPRdone);
    printf ("\nРабота продолжалась \n  с %s \n по %s \n", str1, str2);

    return GoodCode;
}
