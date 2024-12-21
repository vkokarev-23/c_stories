//#define DEBUG

/*
 * p7kvet.c

 *   Эта программа предлагается как "приправа" к штатному редактору тегов p7et
 * АДИС (AFIS) Папилон https://www.papillon.ru/products/programs/adis/

 *   Так как p7et корректирует только файлы-исходники в S-частях сегментов,
 * то индексные данные при этом остаются незатронутыми, а потому видимых
 * результатов в БД не видно, пока не сделаешь восстановление индексных
 * данных.
 *
 *   Кроме того, p7et не умеет корректировать многострочные поля,
 * где в качестве разделителя строк используется ";",
 * не сохраняет историю корректировки следа - работает "втемную".
 *
 *   Неудобно. А хочется корректировать все и сразу.
 *
 *   Когда мы из просмотра базы корректируем след, то на самом деле мы
 * ничего не корректируем, а только создаем запрос на корректировку
 * в каталоге /papillon1/qedit/0025.8001.800c0278.el - небольшой файлик
 * на пару килобайт, который содержит два тега: 699 и 763 - текстовые
 * данные следа. Тег 763 чудесным образом похож на свой первоисточник -
 * такой же тег в корректируемом следе (tv2 вам в помощь).
 *
 *    Тег 763 запроса повторяет тег 763 следа, отличаются только те
 * поля (они же теги внутри тега 763), которые вы откорректировали.
 * Еще изменяются (добавляются) поля "Оператор редактирования" и
 * "Дата редактирования".
 *
 *   Запросы на корректировку отрабатывает, если не ошибаюсь, a8.dbr.
 * Вот она то корректирует сразу во всех нужных местах и как положено.
 *
 *   Что же делать?
 *
 *   Взять след, вытащить из него желанный тег, сформировать из него
 * запрос на редактирование, внести изменения в соответствующие поля.
 * Если надо, добавить чего-нибудь и положить в /papillon1/qedit/
 * на растерзание a8.dbr.
 *
 *   Легко сказать ...
 *
 *    Наша утилита p7kvet корректирует только следы, и только по одному
 * полю за раз,значение поля может быть многострочным, разделитель строк ";".
 *
 *   Для меня эта утилита стала остро необходима, когда потребовалось
 * скопировать в БД Папилона информацию о текущем состоянии УД из базы
 * уголовных дел (ИЦ "Статистика"). Например, в поле "Статья УД" хотелось
 * поместить такое:
 *
 *   158 ч.2 п.БВ
 *   РЕШЕНИЕ 1 31.03.2023
 *   СМИРНОВ НИКОЛАЙ НИКОЛАЕВИЧ 12.05.1986
 *   ФАСТОВ ВЛАДИМИР ВЛАДИМИРОВИЧ 05.07.1988
 *
 *   p7kvet работает в двух режимах: или "Подсказка", или "Корректировка".
 * Режимы работы задаются ключами только в длинной форме:
 *
 * --help подсказка, в этом случае другие ключи игнорируются
 *
 *   Для корректировки необходимы четыре ключа:
 * --ppln в какой каталог складывать запросы, не обязательно в /papillon1/qedit,
 *        Важно, чтобы внутри него находился ./qedit, как в Папилоне.
 * --file полный путь к следу "/papillon1.db/4/00258010.ss/00258017.s/000003d0.l"
 * --tag  номер корректируемого тега
 * --val  его новое значение, например, "777;888;999" для многострочного
 *
 *   p7kvet корректирует только текстовые поля, но не отметки времени в стиле
 * unix-time, например, t:203 66bf7910 время создания запроса.
 *
 *   Будьте осторожны.
 *
 *   Зато у нее нет ограничений типа "льзя-нельзя" как в папилоновской a8.dbc.
 * Поправить номер следа, номер родительской БД - пожалуйста, легко.
 *
 *   Чтобы из исходника p7kvet.c получить исполнимый файл, откомпилируйте его.
 * gcc -o p7kvet p7kvet.c
 *
 *   Иногда компиятор gcc просит добавить ключей, как всегда - по обстановке.
 *
 *   В первой строке (посмотри вверх) находится закомментированная директива
 * //#define DEBUG, которая говорит компилятору: "печатай отладочную информацию".
 * Если компилировать утилиту с раскомментированной #define DEBUG, то будет
 * напечатано много подробностей на этапе выполнения.
 *
 *  Виталий Кокарев, Сочи, 2023.
 *
 *
 *
 *   2024-09-15 пытаюсь корректировать ДК. Получается
 *
 *   Текстовые данные в дактилокартах и следах лежат в разных тегах
 * 763 - следы (СЛ)
 * 761 - дактилокарты (ДК)
 *
 *   в теге 699 т.н. offset определяет тип данных
 * 0 TenPrintCard in DB
 * 1 LatentMark in DB
 *
 *   Сигнатура в теге текстов: ltxt и ftxt
 *
 *   Имена файлов запросов *.ei  *.ef
 *
 *   Оператор и дата редактирования, номера тегов разные ДК:115,124  СЛ:218,229
 *
 *   А в остальном и t:763, и t:761 устроены одинаково, поэтому если в тексте программы
 * и написано 763, то на самом деле применимо к обоим случаям
 *
 *   Поля Checksumm и DBfilenum по-прежнему заполняются нулями, ничего, и так работает
 *
 *   2024-10-07 корректировка ДК заработала
 *
 *   Интересная подробность, когда сам Папилон создает запрос на редактирование ДК, он
 * почему то вслед за заголовком копирует из ДК ее тексты в текущем состоянии,
 * т.е. до корректировки (копия t:761 ), а ему в хвост пристраивает новый t:761, причем
 * заголовок ничего не знает о существовании копии t:761. На нее нет ссылки.
 *   Ссылка идет только на новый t:761, перепрыгивая копию.
 *   Из-за этого размер оригинального запроса почти вдвое больше нашего запроса.
 *   Что это? Косяк Папилона?
 *
 *
 *
*/

#include <fcntl.h>
#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <stdbool.h>
#include <libgen.h>     // basename dirname
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


// РЕЖИМЫ РАБОТЫ затычка вместо них на этапе

//char PplnDir[]  = "/papillon1";
//char QEditDir[]  = "";
//char SledName[] = "/papillon1.db/4/00258010.ss/00258017.s/000003d0.l";
//unsigned short TagCod = 50011;
//char NewValue[] = "777;888;999";
//char NewValue[] = "158 ч.2 п.БВ;РЕШЕНИЕ 1 31.03.2023;СМИРНОВ НИКОЛАЙ НИКОЛАЕВИЧ 12.05.1986;ФАСТОВ ВЛАДИМИР ВЛАДИМИРОВИЧ 05.07.1988";

// РЕАЛЬНЫЕ РЕЖИМЫ РАБОТЫ

char PplnDir[256] = "";     // в какой каталог складывать запросы
char SledName[256] = "";    // полный путь к следу
unsigned short TagCod = 0;  // номер корректируемого тега
char NewValue[1024] = "";   // его новое значение

/*   Данные в следе, запросе на корректировку организованы в теги
 * Наличие и расположение тегов описывается в структуре IFD, а
 * расположение IFD задается в заголовке, с которого начинается файл.
 *   Все поля плотно прижаты друг к другу, не выровнены по границе слова,
 * вероятно, для экономии места на диске. Зачем?
 *   Кроме того, в теге 763 (текстовые данные следа), если в нескольких полях
 * встречается повторяющееся значение, то значение хранится в одном экземпляре,
 * а все поля ссылаются на него одного. Суперэкономия.
 *    p7kvet так не поступает, поэтому запросы получаются чуть длиннее.
*/


#pragma pack(push,1)	    // данные в структурах уплотнены, не по границе слова

    struct t_Header {		// заголовок 8 байт, содержит
        char Sign[4];		// сигнатуру "IIP0"
        int Ofs;			// смещение структуры IFD от начала файла
        };

    struct t_Tag {	        // Описатель тега, составная часть IFD, строчка из его таблицы
        short Cod;          // номер тега (699, 763 и т.д.) в запросе два тега, в следе - больше
        short Typ;	        // тип, всегда единица 1, почему ?
        int Len;	        // длина тега, 699 - 4, 763 - 1..2К
        int Ofs;	        // смещение тега от начала файла
        };

    struct t_IFD {			// структура IFD Image File Directory
        short MaxTag;		// сколько здесь тегов, далее их описатели
        struct t_Tag Tag[64];	// табличка тегов
        };

    // тег 763 содержит другие теги, состоит из заголовка и таблицы подтегов
    struct t_Tag763 {	// с описанием тегов, текстовые строки с данными
        int Len;		// общая длина тега 763 со всеми компонентами
        char ChS;		// контрольная сумма, не используется, 0
        char Ver;		// версия: 0x80
        int File;		// файл: 0x880203df, не используется, можно заменить нулями
        int Laddr;		// ссылка на laddr.dat: 0x0000bb80, не используется, можно заменить нулями
        char Sign[4];	// сигнатура "ltxt"
        short MaxTag;	// сколько здесь тегов, далее их описатели
        struct {
            unsigned short Cod;	// номер подтега 201, 202 и т.д.
            short Len;			// длина подтега
            int Ofs;			// смещение подтега от начала тега 763, не от заголовка
            } Item[64];		    // табличка подтегов
        };
#pragma pack(pop)

    struct t_Node {     // узел односвязного списка
        unsigned short Cod;	    // номер тега 201, 202 и т.д.
        short Len;	    // длина тега
        char *Str;      // указатель на строку
        struct t_Node *Next;
};


/*   План такой: берем след, проецируем (отображаем) его в область памяти,
 * туда же проецируем структуры Header, IFD, Tag763, и можно работать.
 *
 *   Для удобства копируем данные из Tag763 в односвязный список List1.
 * Корректуру помещаем в аналогичный List2. Удаляем из List1 корректируемый тег.
 * Добавляем в List1 содержимое List2. Повторяем эту процедуру трижды:
 * для корректируемого тега, для тега 218 (дата корректировки),
 * для тега 229 (оператор корректировки).
*/

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ:

    void *SledInMemory;         // файл следа, отображенный в память, на него ссылаются:
    struct t_Header *Header;    // Заголовок
    struct t_IFD    *IFD;       // IFD следа
    struct t_Tag763 *Tag763;    // тег-763 следа

    bool isFingerPrint;         // если работаем с ДК - истина, след - ложь
    unsigned short TagDate;     // ДК:115  СЛ:218
    unsigned short TagOper;     // ДК:124  СЛ:229

    struct t_Node *List1=NULL;  // тег 763 корректируемого следа в виде списка
    struct t_Node *List2=NULL;  // корректор - добавка в тег 763

    bool WorkModeADD;           // режим работы: добавление в тег, замещение тега



// предварительное объявление некоторых функций
void PrintHelp();
int file_exists (char *filename);


// Режим работы: добавление в тег или его замещение в переменной WorkModeADD
void SetWorkMode (int argc, char *argv[]) {

    char *FullName, *BaseName;

    FullName = strdup(argv[0]);
    BaseName = basename(FullName);

    char s_p7kvet[]     = "p7kvet";
    char s_p7kvet_add[] = "p7kvet.ADD";
    char s_p7kvet_mod[] = "p7kvet.MOD";


    if ( strcmp(BaseName, s_p7kvet) == 0)
      WorkModeADD = false;
    else if (strcmp(BaseName, s_p7kvet_mod) == 0)
      WorkModeADD = false;
    else if (strcmp(BaseName, s_p7kvet_add) == 0)
      WorkModeADD = true;
    else {
      printf("\nКто я? Зачем я?\nЯ %s\n\n", BaseName);
      exit(EXIT_SUCCESS);
      }

    if (WorkModeADD)
      printf("\nРежим ДОБАВЛЕНИЯ %s.\n\n", BaseName);
    else
      printf("\nРежим ЗАМЕЩЕНИЯ %s.\n\n", BaseName);
}


// разбираем аргументы командной строки, формируем режимы работы

/* Разбираем аргументы по полочкам, результаты - в глобальные переменные:
    PplnDir, SledName, TagCod, NewValue

 *  используются параметры только в длинной форме
 *  если задан --help - будет help и никакой корректировки
 *  для корректировки обязательны 4 параметра,
 *  например, для корректировки номера следа в следокарточке:
 *
 * --ppln	/home/st или /papillon1, или ...
 * --file	/papillon1.db/00258010.ss/00258017.s/00000261.l
 * --tag	201
 * --val	3
*/

void GetParameters (int argc, char *argv[]) {

// используются здесь в качестве индексов массива
#define HELP 0
#define PPLN 1
#define FIL  2
#define TAG  3
#define VAL  4

    struct {			// табличка аргументов ЧТО ДЕЛАТЬ
        char Name[8];	// имя параметра для наглядности
        int  Flag;		// 1 - включен, 0 - выключен
        int  Indx;		// значение параметра - индекс в argv: argv[Indx]
        } RunOpt[] = {
            {"help\0", 0, 0},
            {"ppln\0", 0, 0},
            {"file\0", 0, 0},
            {"tag \0", 0, 0},
            {"val \0", 0, 0},
        };

    const char* short_options = "";

    //
    const struct option long_options[] = {
            { "help", no_argument,       &RunOpt[0].Flag, 1 },
            { "ppln", required_argument, &RunOpt[1].Flag, 1 },
            { "file", required_argument, &RunOpt[2].Flag, 1 },
            { "tag",  required_argument, &RunOpt[3].Flag, 1 },
            { "val",  required_argument, &RunOpt[4].Flag, 1 },
            { 0, 0, 0, 0 }
        };

//    int i_optind, i_opterr, i_optopt;
    int longindex = 0;
    int RC = 0;
    opterr = 0;		// не выводить сообщения о неизвестных аргументах

    while (1) {
        RC = getopt_long_only(argc, argv, short_options, long_options, &longindex);
//      i_optind = optind;	// отладка
//      i_opterr = opterr;	// отладка
//      i_optopt = optopt;	// отладка
        if (RC == -1)
            break;
        if (RC == 0)
            RunOpt[longindex].Indx = optind - 1;
        else {
            printf("\nНепонятные аргументы в командной строке\n");
            PrintHelp();
            }
        }

#ifdef DEBUG
    printf("\n");		// отладка списка аргументов
    for( int ia = 0 ; ia < 5; ia++) {
        if (RunOpt[ia].Flag == 1)
            printf("Parameter %d: %s %d %d %s \n", ia, RunOpt[ia].Name, RunOpt[ia].Flag, RunOpt[ia].Indx, argv[RunOpt[ia].Indx]);
        else
            printf("Parameter %d: %s %d %d \n", ia, RunOpt[ia].Name, RunOpt[ia].Flag, RunOpt[ia].Indx);
        }
#endif

    if (RunOpt[HELP].Flag == 1) {
        PrintHelp();
        }

    // формирование параметров работы, проверка их на корректность

    if (!(RunOpt[PPLN].Flag & RunOpt[FIL].Flag & RunOpt[TAG].Flag & RunOpt[VAL].Flag)) {
        printf("\nНеполные аргументы в командной строке\n");
        PrintHelp();
        }

    strncpy(PplnDir, argv[RunOpt[PPLN].Indx], sizeof(PplnDir));
    if (!(file_exists(PplnDir))) {     // Проверка наличия PplnDir
        printf("\nКаталог не найден: %s \n", SledName);
        PrintHelp();
        }

    strncpy(SledName, argv[RunOpt[FIL].Indx], sizeof(SledName));
    if (!(file_exists(SledName))) {     // Проверка наличия SledName
        printf("\nФайл не найден: %s \n", SledName);
        PrintHelp();
        }

    TagCod = atoi (argv[RunOpt[TAG].Indx]);

    strncpy(NewValue, argv[RunOpt[VAL].Indx], sizeof(NewValue));
}


// печать подсказки и выход

void PrintHelp() {
    printf("\np7kvet редактор тега 763 - текстовые данные следа\n");
    printf("он же генератор запросов на корректировку следа. \n\n");
    printf("--help вывод подсказки, запрос не формируется, \n");
    printf("       одно из двух: или help, или корректировка. \n\n");
    printf("Четыре ключа для корректировки: --ppln --file --tag --val \n");
    printf("--ppln /home/st/ запросы лягут в /home/st/qedit \n");
    printf("--file /papillon1.db/00258010.ss/00258017.s/00000261.l \n");
    printf("--tag 201 что корректируем, например, номер следа в следокарточке \n");
    printf("--val 3   новый номер у следа будет тройка \n\n");
    printf("/home/p8bin/tv2 расскажет, какие бывают теги \n\n");
    printf("Сочи, 2023-2024, Виталий Кокарев \n\n");

    exit(EXIT_SUCCESS);
    }


// печать содержимого Tag763 для отладки

void PrintTag763() {

    int i;
    char *Str;

    printf("\n\n    Печатаем из тега 763:\n\n");
    for (i = 0; i < Tag763->MaxTag; i++) {
        Str = (char *) Tag763;
        Str += Tag763->Item[i].Ofs;
    	printf("%02d t:%03d l:%02d o:%6x value:%s\n", i, Tag763->Item[i].Cod, Tag763->Item[i].Len, Tag763->Item[i].Ofs, Str);
        };
}


// данные из тега 763 загружаем в односвязный список

void Tag763_to (struct t_Node **List) {

    struct t_Node *Tail, *NewNode;
    int i;
    char *Str, *NewStr;

    Tail = *List;        // сначала NULL, потом указывает на последний элемент

    for (i = 0; i < Tag763->MaxTag; i++) {
        Str = (char *) Tag763;
        Str += Tag763->Item[i].Ofs;

        NewNode = (struct t_Node *) malloc(sizeof(struct t_Node));
        if (i == 0) {
            *List = NewNode;         // указывает на первый элемент
            Tail = *List;            // он-же и последний
            }
        else {
            Tail->Next = NewNode;   // новый элемент привязали к хвосту
            Tail = NewNode;         // теперь он хвост
            }

        NewNode->Cod = Tag763->Item[i].Cod;
        NewNode->Len = Tag763->Item[i].Len;
        NewNode->Next = NULL;
        NewStr = (char *) malloc(Tag763->Item[i].Len);
        strncpy(NewStr, Str, Tag763->Item[i].Len);
        NewNode->Str = NewStr;
        };
};


// используется для очистки List2 перед повторным использованием

void ClearList(struct t_Node **List) {

    struct t_Node *Current, *Next;

    if (*List == NULL)
        return;
    else
        Current = *List;

    while (Current != NULL) {
        Next = Current->Next;
        free(Current->Str);
        free(Current);
        Current = Next;
        }
}


// загрузка нового значения в List2

void Str_to_List (unsigned short Cod, char *Str, struct t_Node **List) {

    char Semicolon[] = ";"; // разделитель в многострочных полях

    struct t_Node *Tail, *NewNode;
    char *NewStr;

    // изначально List должен быть NULL
    // Tail сначала NULL, потом указывает на последний элемент
    if (*List != NULL) {
        printf("Str_to_List: List != NULL");
        exit(EXIT_FAILURE);
        }

    Tail = *List;

    // разбираем строку на токены, разделитель ";"
    char *Token = strtok(Str, Semicolon);  // получили первый токен

    while (Token != NULL) { //пока есть токены

        NewNode = (struct t_Node *) malloc(sizeof(struct t_Node));
        if (*List == NULL) {
            *List = NewNode;         // указывает на первый элемент
            Tail = *List;            // он-же и последний
            }
        else {
            Tail->Next = NewNode;   // новый элемент привязали к хвосту
            Tail = NewNode;         // теперь он хвост
            }

        NewNode->Cod = Cod;
        NewNode->Len = strlen(Token)+1;
        NewNode->Next = NULL;
        NewStr = (char *) malloc(NewNode->Len);
        strncpy(NewStr, Token, NewNode->Len);
        NewNode->Str = NewStr;

        Token = strtok(NULL, Semicolon);  // следующий токен
        };
};


// ПЕЧАТЬ списка List1 или List2 для отладки

void Print_List(struct t_Node **List) {

    struct t_Node *Node;
    int i = 0;


    printf("\n\n    Печатаем из СПИСКА:\n\n");

    Node = *List;
    while (Node != NULL) {
    	printf("%02d t:%03d l:%02d o:%6x value:%s\n", i, Node->Cod, Node->Len, 0, Node->Str);
    	Node = Node->Next;
    	i++;
    }
}


// Удаление из списка старого значения тега

void DelTag_from_List (unsigned short TagCod, struct t_Node **List) {

    struct t_Node *Prev, *Node;

    Prev = *List;
    Node = Prev->Next;

    if (Prev->Cod == TagCod) {  // тег в начале списка, т.е. t:201 однострочный
        *List = Prev->Next;
        free(Prev->Str);
        free(Prev);
        }
    else while (Node != NULL) {

        if (Node->Cod == TagCod) {  // тег где-то дальше
            Prev->Next = Node->Next;
            free(Node->Str);
            free(Node);
            Node = Prev->Next;
            }
        else {
            Prev = Node;
            Node = Prev->Next;
            }
    }
}


// List2 добывляется в List1, из которого на предыдущем этапе
// вставляемый тег удален. Пересечения тегов не должно быть

void Add_List (struct t_Node **List1, struct t_Node **List2) {

    struct t_Node *Prev, *Node, *Last2;
    unsigned short TagCod;

    Last2 = *List2;         // Список2 для вклейки нужны два указателя:
    TagCod = Last2->Cod;    // List2 - первый (First2) и Last2 - последний элемент
    while (Last2->Next != NULL)
        Last2 = Last2->Next;

    Prev = *List1;          // Список1: Prev и Node идут друг за другом:
    Node = Prev->Next;      // 1-2, 2-3 и т.д. вставлять между ними

    if ((Prev == *List1) & (Prev->Cod > TagCod)) {
        Last2->Next = *List1;   // вставились в голову списка
        *List1 = *List2;
        *List2 = NULL;
        return;
        }
    else while (Node != NULL) { // ищем дырочку в Список1
        if ((Prev->Cod <= TagCod) & (Node->Cod > TagCod)) {
            Prev->Next = *List2;// вставились между Prev и Node
            Last2->Next = Node;
            *List2 = NULL;
            return;
            }
        Prev = Prev->Next;  // сдвигаемся на шаг по списку
        Node = Prev->Next;
        }
    // почему мы оказались здесь? закончился список?
    if ((Prev->Cod < TagCod) & (Node == NULL))
        Prev->Next = *List2;    // вставились в хвост

    // данные вклеены в List1, List2 указывает в то место, где вклейка
    // нужно разорвать эту связь и освободить List2
    *List2 = NULL;
}


//====================================================================
// Закончили с тегами, займемся файлами


// проверка наличия файла

int file_exists (char *filename) {
    return (access(filename, F_OK) == 0);
}


// исследуемая строка содержит подстроку

int StringContain(char *String, const char *Pattern) {

    // Переменная, в которую будет занесен адрес первой найденной подстроки
    char *Index;
    Index = strstr (String, Pattern);

    if ( Index == NULL)
        return (0);
    else
        return (1);     // Строка найдена
}


// формируем имя файла запроса
// в зависимости от того, где лежит след - в простом сегменте или
// в макросегменте, имя запроса меняется, например, 7fff.8001.80010001.el

void MakeQEditName (char *FullName, char *QEditName)
{
    char PplnMacro[16] = "";
    char PplnSegment[16] = "";
    char PplnFile[16] = "";
    char Suff_ss[8]=".ss";
    char Suff_s[8]=".s";
    char Suff_l[8]=".l";
    char Suff_t[8]=".t";
    char Suff_f[8]=".f";
    char Slash[8] = "/";

    // разбираем строку на токены, разделитель слэш
    char *Token = strtok(FullName, Slash);  // получили первый токен

//    PplnMacro[0] = 0;
//    PplnSegment[0] = 0;
//    PplnFile[0] = 0;

    while (Token != NULL) //пока есть токены
    {
        if (StringContain(Token, Suff_ss))
            strcpy(PplnMacro, Token);
        else if (StringContain(Token, Suff_s))
            strcpy(PplnSegment, Token);
        else if (StringContain(Token, Suff_l) | StringContain(Token, Suff_t) | StringContain(Token, Suff_f)  )
            strcpy(PplnFile, Token);

        Token = strtok(NULL, Slash);  // следующий токен
    }

    QEditName[0] = 0;
    if (strlen(PplnMacro) > 0) {                // след из макросегмента 12345678.ss
        strncat(QEditName, PplnMacro,       4); // 0025
        strncat(QEditName, ".",             2); // .
        strncat(QEditName, &PplnMacro[4],   4); // 8010
        strncat(QEditName, ".",             2); // .
        strncat(QEditName, &PplnSegment[4], 4); // 8017
        strncat(QEditName, &PplnFile[4],    4); // 0123
//      strncat(QEditName, ".el",           4); // .el
    }
    else {
//        printf("MakeQEditName: PplnMacro: .%s. PplnSegment: .%s. PplnFile: .%s. \n" , PplnMacro, PplnSegment, PplnFile );

        strncat(QEditName, PplnSegment,     4); // 0025
        strncat(QEditName, ".",             2); // .
        strncat(QEditName, &PplnSegment[4], 4); // 8010
        strncat(QEditName, ".",             2); // .
        strncat(QEditName, &PplnFile[0],    8); // 00000123
//      strncat(QEditName, ".el",           4); // .el

//        printf("MakeQEditName: QEditName: .%s. \n" , QEditName );
    }

    if ( isFingerPrint ) {
      strncat(QEditName, ".ef",           4); }
    else {
      strncat(QEditName, ".el",           4); }

}


// проецируем след в память, размечаем его, расставляем Header, IFD, Tag763

void MarkupSledInMemory() {

    char *HeaderPtr, *IFDPtr, *Tag763Ptr;	// эти указатели можно перемещать по 1 байту
    int fdin, i, i763;
    struct stat statbuf;

//  отображаем файл следа в память по адресу SledInMemory
    fdin = open(SledName, O_RDONLY);
    i = fstat(fdin, &statbuf);
    SledInMemory = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0);

// расставляем по местам указатели Header, IFD, Tag763
    Header = SledInMemory;
    HeaderPtr = (char *) Header;
    IFDPtr = HeaderPtr + Header->Ofs;
    IFD = (struct t_IFD*)IFDPtr;

    i763 = 0xffffffff;
    for (i = 0; i < IFD->MaxTag; i++) {

        if (IFD->Tag[i].Cod == 761) {   // это дактилокарта
            i763 = i;
            isFingerPrint = true;
            TagDate = 115;     // ДК:115  СЛ:218
            TagOper = 124;     // ДК:124  СЛ:229
            break;
            }

        if (IFD->Tag[i].Cod == 763) {   // это след
            i763 = i;
            isFingerPrint = false;
            TagDate = 218;     // ДК:115  СЛ:218
            TagOper = 229;     // ДК:124  СЛ:229
            break;
            }
        }

    if (i763 == 0xffffffff) {
        printf("Не найден tag:763");
        exit(EXIT_FAILURE);
        }

    Tag763Ptr = HeaderPtr + IFD->Tag[i763].Ofs;
    Tag763 = (struct t_Tag763*)Tag763Ptr;
}


// формируем текущую дату GGGG-MM-DD, она же дата корректировки

void GMD_Date(char* G_M_D) {

    struct tm *fld_time;
    time_t l_time;

    l_time = time(NULL);                    // unix время системы - секунд от 01.01.1970
    fld_time = localtime(&l_time);          // местное время, разделенное на компоненты, поля
    strftime(G_M_D, 100, "%F", fld_time);   // формируем дату GGGG-MM-DD
}


// кто откорректировал след? Ага ... "Кокарев Виталий Германович"
// Нет? Тогда кто, как узнать его имя, если программа запускается
// из командной строки?

void Editing_Oper(char *Ed_Oper) {

    char* kvg = "Кокарев Виталий Германович";

    strncpy(Ed_Oper, kvg, strlen(kvg));
}


// берем 4К памяти, это будет наш буфер, помещаем туда Header, IFD
// благо дело тегов у нас всегда два: 699 и 763,
// берем List1 и старательно переписываем из него данные в 763
// длину данных посчитали и побайтно записали в файл. Все.

void WriteRequest () {

    char GMD[16];
    GMD_Date(GMD);

    char QEditName[256] = "";
    char QEditShort[32] = "";

    strncat(QEditName, PplnDir, strlen(PplnDir));
    strncat(QEditName, "/qedit/", 8);

    if (!(file_exists(QEditName))) {
        printf("\nКаталог: %s  не создан\n", QEditName);
        exit(EXIT_FAILURE);
        }

    MakeQEditName(SledName, QEditShort);
    strncat(QEditName, QEditShort, strlen(QEditShort));
    if (file_exists(QEditName)) {
        printf("\nФайл запроса: %s уже существует\n", QEditName);
        exit(EXIT_FAILURE);
        }


#define BufSize 4096
    FILE *FQE;       // файл запроса на редактирование
    char Buf[BufSize];
   	int j;

    for (j = 0; j <BufSize; j++)
        Buf[j] = 0;

    if ((FQE = fopen(QEditName, "wb")) == NULL) {
        printf("не могу открыть %s", QEditName);
        exit(EXIT_FAILURE);
        }

    struct t_Header *Q_HDR;    // Заголовок запроса
    struct t_IFD    *Q_IFD;    // IFD запроса
    struct t_Tag763 *Q_763;    // тег-763 запроса

#define Q_763_OFS 0x26
    // втыкаем элементы структуры запроса на свои места в буфер
    // элементы структуры, их количество постоянны, кроме тега 763
    Q_HDR = (struct t_Header *) &Buf[0];
    Q_IFD = (struct t_IFD *)    &Buf[8];
    Q_763 = (struct t_Tag763 *) &Buf[Q_763_OFS];

    Q_HDR->Sign[0] = 0x49;    // строка 'IIP'
    Q_HDR->Sign[1] = 0x49;
    Q_HDR->Sign[2] = 0x50;
    Q_HDR->Ofs = 8;         // сдвиг до IFD

    Q_IFD->MaxTag = 2;  // два тега: 699 и 763
    Q_IFD->Tag[0].Cod = 699;
    Q_IFD->Tag[0].Typ = 1;
    Q_IFD->Tag[0].Len = 4;


    if ( isFingerPrint ) {        // в теге 699 Ofs - это не смещение, а тип файла Папилона:
      Q_IFD->Tag[0].Ofs = 0;      // 0 - TenPrintCard in DB
      Q_IFD->Tag[1].Cod = 761; }  // соответственно тег 761 для ДК
    else {
      Q_IFD->Tag[0].Ofs = 1;      // 1 - LatentMark in DB и т.д. 0A 0B 11 13 14 15 ...
      Q_IFD->Tag[1].Cod = 763; }  // соответственно тег 763 для СЛ

    Q_IFD->Tag[1].Typ = 1;
    Q_IFD->Tag[1].Len = 0;        // посчитаем позже
    Q_IFD->Tag[1].Ofs = Q_763_OFS;

    Q_763->Len = 0;         // посчитаем позже
    Q_763->ChS = 0;         // контрольная сумма, не используется
    Q_763->Ver = 0x80;      // версия, papillon  8.0 имеется в виду?
    Q_763->File = 0;        // нечто 0x880203df, но работает и без него
    Q_763->Laddr = 0;       // нечто 0x0000bb80, но работает и без него
    Q_763->Sign[0] = 0x6c;  // строка 'ltxt'
    Q_763->Sign[1] = 0x74;
    Q_763->Sign[2] = 0x78;
    Q_763->Sign[3] = 0x74;
    if ( isFingerPrint ) {
      Q_763->Sign[0] = 0x66;  // строка 'ftxt'
      }

    Q_763->MaxTag = 0;      // посчитаем позже

    struct t_Node *Node;
    Node = List1;    // первый проход, берем List1 и заполняем таблицу тегов
    short i = 0;
    while (Node != NULL) {
        Q_763->Item[i].Cod = Node->Cod;
        Q_763->Item[i].Len = Node->Len;
        Q_763->Item[i].Ofs = 0;     // сдвиг до строки пока неизвестен
        Node = Node->Next;
    	i++;
        }
    Q_763->MaxTag = i;      // вот и посчитали

    // Q_763_Str - это сдвиг строки от начала тега 763
    int Q_763_Str = 20 + Q_763->MaxTag * 8;    // 20 - заголовок, 8 - элемент таблички

    Node = List1;    // второй проход, берем List1 и дописываем строки
    i = 0;
    while (Node != NULL) {
        Q_763->Item[i].Ofs = Q_763_Str;
        strncpy( &Buf[Q_763_OFS + Q_763_Str], Node->Str, Node->Len);
        Q_763_Str += Node->Len;
        Node = Node->Next;
    	i++;
        }

        Q_IFD->Tag[1].Len = Q_763_Str;  // вот и посчитали
        Q_763->Len = Q_763_Str;

    for (j = 0; j < (Q_763_OFS + Q_763_Str); j++)
        fputc (Buf[j], FQE);

    fclose(FQE);
};


int main(int argc, char *argv[]) {

    setbuf(stdout, NULL);

// Режим работы (WorkModeADD): добавление в тег или его замещение
// определяется по имени запущенной программы:
// p7kvet или p7kvet.MOD -  замещение
// p7kvet.ADD - добавление
    SetWorkMode(argc, argv);

    GetParameters(argc, argv);      // Разбираем параметры: --help или --ppln --file --tag --val

    MarkupSledInMemory();           // Отображаем файл в память

#ifdef DEBUG
    PrintTag763();
#endif
    Tag763_to(&List1);
#ifdef DEBUG
    Print_List(&List1);
#endif

    // правим корректируемый (TagCod) тег
    // в режиме добавления (WorkModeADD) дописываем в хвост существующего тега
    // в режиме замены (! WorkModeADD) удаляем старое значение, вставляем новое
    if (! WorkModeADD)
      DelTag_from_List(TagCod, &List1);

    ClearList(&List2);
    Str_to_List(TagCod, NewValue,  &List2);
#ifdef DEBUG
    Print_List(&List2);
#endif
    Add_List(&List1, &List2);
#ifdef DEBUG
    Print_List(&List1);
#endif

    // правим дату корректировки
    char Today[16] = "";
    GMD_Date(Today);
    DelTag_from_List(TagDate, &List1);
    ClearList(&List2);
    Str_to_List(TagDate, Today,  &List2);
#ifdef DEBUG
    Print_List(&List2);
#endif
    Add_List(&List1, &List2);
#ifdef DEBUG
    Print_List(&List1);
#endif

    // правим оператора корректировки
    char EdOper[128] = "";
    Editing_Oper(EdOper);
    DelTag_from_List(TagOper, &List1);
    ClearList(&List2);
    Str_to_List(TagOper, EdOper,  &List2);
    Add_List(&List1, &List2);
#ifdef DEBUG
    Print_List(&List1);
#endif

    WriteRequest();

    exit(EXIT_SUCCESS);
    }

