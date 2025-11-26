# Документация ATL

Библиотека предлагет конструктор и методы для простого создания любых одиночных, групповых АТ команд, автоматический парсинг их ответов, а также создание логических цепочек выполнения групповых команд для реализации различных сценариев работы с любым вазимодействующим устройством.

## 1. Особенности и свойства

- **Асинхронность**: Отсутствие блокирования и ожидания системы.
- **Управление командами**: АПИ для создания и отправки групп AT-команд, построения сложных логических цепочек.
- **Гибкость**: Возможность создания любого типа команды, с различной комбинацией параметров.
- **Память**: Использование пользовательского динамического аллокатора памяти с поддержкой выравнивания и защитой от фрагментации O1heap. [GitHub](https://github.com/pavel-kirienko/o1heap)
- **Обработка ошибок**: Design by Contract (DBC) для надежной проверки ошибок.   [GitHub](https://github.com/QuantumLeaps/DBC-for-embedded-C)
- **Потокобезопасность**: Защита критических секций для многопоточных приложений.
- **Поддержка URC**: Обработка URC для асинхронных событий.
- **Тестирование**: Embedded тесты от QLP. [GitHub](https://github.com/QuantumLeaps/Embedded-Test)
- **Срезы**: Использование кольцевых срезов вместо прямого копирования буфера. [GitHub](https://github.com/ferrero222/ringslice/tree/dev)
- **Модульность**: Набор предлагаемых, расширяемых готовых модулей групповых ат команд под контретные внешние устройства.
- **Расширяемость**: Возможность адаптирования и внедрения новых парсеров для поддержки любого формата команд.
- **Контекстность**: Возможность создания одновременно нескольких контекстов библиотеки для параллельной работы сразу с несколькими устройствами.

Протестировано с: SIM868....

## 2. Настройка и предварительные требования

### Базовая конфигурация

Необходимо определить свой кольцевой буфер и указатели:

```c
typedef struct {
  uint8_t *buffer;      
  uint16_t size;   
  uint16_t head;       
  uint16_t tail;       
  uint16_t count;     
} atl_ring_buffer_t;
```
Далее определяем глобально контекст:

```c
atl_context_t ctx = {0};
```

Инициализируем данный контекст:

```c
atl_init(
    &ctx,                     // Контекст
    your_printf_function,     // Пользовательская printf для отладки
    your_write_function,      // Функция записи в интерфейс  
    your_ring_buffer_struct,  // Структура кольцевого буфера
);
```

### Параметры конфигурации

В файле atl_port.c необходимо описать обработчики критических секций для защиты от параллельного доступа, а также обработчик исключений:

```c
DBC_NORETURN void DBC_fault_handler(char const* module, int label) 
{
  (void)module;
  (void)label;
  while (1) 
  {
    /* Typically you would trigger a system reset or safe state here */
  }
}

static void atl_crit_enter(void) 
{ 
  __disable_irq();
}

static void atl_crit_exit(void)  
{
  __enable_irq();
}
```

В файле atl_core.h можно настроить параметры:

```c
#define ATL_MAX_ITEMS_PER_ENTITY  50     //Max amount of AT cmds in one group
#define ATL_ENTITY_QUEUE_SIZE     10     //Max amount of groups 
#define ATL_URC_QUEUE_SIZE        10     //Amount of handled URC
#define ATL_MEMORY_POOL_SIZE      4096   //Memory pool for custom heap
#define ATL_URC_FREQ_CHECK        10     //Check urc each ATL_URC_FREQ_CHECK*10ms
 
#ifndef ATL_TEST 
  #define ATL_DEBUG_ENABLED       1      //Recommend to turn on DEBUG logs
#endif
```

Основная функция обработки должна вызываться для каждого контекста, каждые 10ms в системном таймере:

```c
void timer_10ms_handler(void) {
  atl_core_proc(&ctx1);
  atl_core_proc(&ctx2);
  ...
}
```

## 3. АТ команды

В файле `atl_core.h` представлено АПИ для работы с АТ командами и самим ядром библиотеки, содержащее:

- `atl_entity_enqueue`
- `atl_entity_dequeue` 
- `atl_urc_enqueue`
- `atl_urc_dequeue`
- `atl_core_proc`
- `atl_get_init`
- `atl_get_cur_time`
- `atl_malloc`
- `atl_free`

Подробнее о функциях и их параметрах в самом файле. Разберем некоторые примеры создания и использования АТ команд.

### Пример 1, групповые команды без форматирования ответа

```c
atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
{
  ATL_ITEM("AT+CIPMODE?"ATL_CMD_CRLF,        "+CIPMODE: 0", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMODE=0"ATL_CMD_CRLF,                NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMUX?"ATL_CMD_CRLF,          "+CIPMUX: 0", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMUX=0"ATL_CMD_CRLF,                 NULL, ATL_PARCE_SIMCOM, 30, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,   "STATE: IP START", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CSTT=\"\",\"\",\"\""ATL_CMD_CRLF,      NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP GPRSACT", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIICR"ATL_CMD_CRLF,                    NULL, ATL_PARCE_SIMCOM, 30, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP GPRSACT", ATL_PARCE_SIMCOM,  3, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIFSR"ATL_CMD_CRLF,           ATL_CMD_FORCE, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPHEAD?"ATL_CMD_CRLF,        "+CIPHEAD: 1", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPHEAD=1"ATL_CMD_CRLF,                NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSRIP?"ATL_CMD_CRLF,        "+CIPSRIP: 1", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSRIP=1"ATL_CMD_CRLF,                NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSHOWTP?"ATL_CMD_CRLF,    "+CIPSHOWTP: 1", ATL_PARCE_SIMCOM,  1, 100, 1, 0, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSHOWTP=1"ATL_CMD_CRLF,              NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 0, NULL, NULL, ATL_NO_ARG),
};
if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
```

### Пример 2, групповые команды с форматированием ответа

```c
atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
{   
  ATL_ITEM("AT+GSN"ATL_CMD_CRLF,       NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL,               "%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
  ATL_ITEM("AT+GMM"ATL_CMD_CRLF,       NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL,               "%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_id)),  
  ATL_ITEM("AT+GMR"ATL_CMD_CRLF,       NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL,      "Revision:%29[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_rev)),                
  ATL_ITEM("AT+CCLK?"ATL_CMD_CRLF,  "+CCLK", ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL,      "+CCLK: \"%21[^\"]\"", ATL_ARG(atl_mdl_rtd_t, modem_clock)),          
  ATL_ITEM("AT+CCID"ATL_CMD_CRLF,      NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL,               "%21[^\x0d]", ATL_ARG(atl_mdl_rtd_t, sim_iccid)),             
  ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,  "+COPS", ATL_PARCE_SIMCOM, 20, 100, 0, 1, NULL, "+COPS: 0, 0,\"%49[^\"]\"", ATL_ARG(atl_mdl_rtd_t, sim_operator)),            
  ATL_ITEM("AT+CSQ"ATL_CMD_CRLF,     "+CSQ", ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL,                 "+CSQ: %d", ATL_ARG(atl_mdl_rtd_t, sim_rssi)),             
  ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,    NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL,                       NULL, ATL_NO_ARG),                
  ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,     NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 0, atl_mdl_general_ceng_cb,    NULL, ATL_NO_ARG),                         
};
if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_rtd_t), meta)) return false;
```

### Параметры АТ команды

Рассмотрим отдельно каждый указываемый параметр АТ команды и разберем что происходит:

- **ATL_ITEM** - макрос для заполнения структуры, используйте для добавления АТ команды в группу.
- **[REQ]** - сам запрос, состоит из конкатенации АТ команды и ATL_CMD_CRLF, в данном случае является строковым литералом, но может быть и массивом символов.
- **[PREFIX]** - строковый литерал который будет искаться в ответе. Можно не указывать.
- **[PARCE_TYPE]** - тип используемого парсера, читайте ниже.
- **[FORMAT]** - формат парсинга ответа для SSCANF, используется вместе с VA_ARGS, полученные данные будут собраны в соответствии с этим форматом, положены в аргументы и переданы в коллбек группы. Можно не указывать.
- **[RPT]** - количество повторов в случае возникновения ошибки.
- **[WAIT]** - таймер ожидания ответа в 10мс.
- **[STEPERROR]** - в случае возникновения ошибки у команды мы можем перескочить на несколько шагов вперед или назад внутри группы, либо ничего не делать. 0 завершает выполнение всей группы
- **[STEPOK]** - как и в случае ошибки, но тут в случае успеха можем прошагать на какую-либо конкретную АТ команду.  0 завершает выполнение всей группы
- **[CB]** - коллбек на АТ команду, будет вызван по результату выполнения АТ команды. В него же передаются полученные форматом данные. Можно не указывать
- **[VA_ARG]** - аргументы для формата в виде ATL_ARG, где мы указываем какая структура и какое поле будет использоваться чтобы сохранить данные из формата. Можно не указывать. Максимум 6.

### Параметры функции atl_entity_enqueue

```c
bool atl_entity_enqueue(atl_context_t* const ctx, const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, void* const meta)ж
```

- **ctx** - глобальный контекст для которого создается группа.
- **item** - группа АТ команд.
- **item_amount** - размер структуры команд.
- **cb** - коллбек на всю группу, который будет вызван по исполнению этой группы. Можно не указывать
- **data_size** - размер структуры данных которая будет создана динамически, автоматически, для сохранения данных указанных в полях [FORMAT] и [VA_ARGS], указатель на эту структуру вернется в коллбек на группу а после исполнения память для него будет очищена. Может не указываться (0).
- **meta** - указатель на некоторые дополнительные мета данные группы. Передаются напрямую в коллбек выполнения и никак не обрабатываются библиотекой. Можно не указывать.

### Параметры коллбека на всю группу команд:

```c
void (*atl_entity_cb_t)(bool result, void* meta, void* data);
```

- **result** - результат.
- **meta** - те самые мета данные которые мы передавали при создании.
- **data** - та самая структура которая была создана динамически так как мы указывали размер, либо NULL если мы ничего не указывали.

### Параметры коллбека на АТ команду:

```c
(*answ_parce_cb_t)(ringslice_t data_slice, bool result, void* data);
```

- **data_slice** - библиотека работает с кольцевым буфером напрямую используя срезы, здесь вернется срез данных для этой АТ команды, если он есть, используйте АПИ библиотеки ringslice чтобы работать с этим срезом. 
- **result** - результат выполнения.
- **data** - указатель на ту самую созданную структуру данных для форматирования, если мы указали это при создании.

### Особенности

- Если необходимо получить данные из ответа, но не был передан размер структуры для создания или формат получаемых, то в процессе выполнения сработает ассерт.
- Для получения полезных данных из ответа и их сбор, библиотека сама создает динамически структуру данных, если все правильно было указано при создании группы команд, передает ее в коллбек выполнения, а затем сама также ее удаляет после завершения. Поэтому если данные нужны какое то время то необходимо скопировать их из коллбека в свою статическую структуру.
- Иногда заранее неизвестно из чего будет состоять команда и нужно создать ее в реальном времени на стеке, или в каком то временном буфере, а не использовать статический строковый литерал. Так как библиотека работает асинхронно, то нужно следить за тем чтобы этот временный буфер существовал в момент исполнения команды. Чтобы каждый раз не заниматься этим, можно просто указать, чтобы команда сохранилась в память библиотеки используя макрос `ATL_CMD_SAVE`, например: `ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF`. Тоже самое можно применять и для префикса.
- В поле [PREFIX] можно указывать более сложные конструкции для проверки сразу нескольких строк и префиксов:
  - Используйте `|` для операций ИЛИ: `"+CREG: 0,1|+CREG: 0,5"`
  - Используйте `&` для операций И: `"+IPD&SEND OK"`
- Также в поле [PREFIX] можно указать макрос-литерал `ATL_CMD_FORCE` который будет указывать что данная команда или данные должна/ы быть просто отправлена/ы, без парсинга и ожидания ответа. Остальные поля кроме [STEPOK] не будут никак использованы и их содержимое может быть любым.
- В папке modules содержатся некоторые файлы и реализации уже готовых групп ат команд.
- В папке tests есть make файл который запускает тесты хосте для проверки логики вне зависимсоти от микроконтроллера. Можно запустить его через консоль или отладить через VS code, попробовать добавить свои или посмотреть как происходит процесс выполнения готовых тестов, чтобы понять как что работает.
- Коллбек на АТ команду получает срез на данные, это сделано для того чтобы можно было написать собственный парсер данных, в случае если стандартного форматирования недостаточно, пример такого можно увидеть в готовой функции atl_mdl_rtd, там структура данных динамически создается библиотекой и в коллбеке на команду мы вручную парсим ее данные нужным способом и кладем их в эту структуру.
- В случае обработанных данных библиотека сама передвигает tail и counter вашего кольцевого буфера.
- В папке examples есть примеры использования.


### Парсеры 
`ATL_PARCE_SIMCOM`<br> 
 Парсер команд формата simcom. Данный парсер работает с включенным эхо. Обычно тут ответ на команды приходит последовательно ввиде: ЭХО → РЕЗУЛЬТАТ → ДАННЫЕ. Эхо в любом случае должно идти первым. Парсер работает таким же образом, что сначала ищет эхо команды, потом результат выполнения, а потом полезные данные, т.е. разбивает ответ на три поля: [REQ] [RES] [DATA]. Их комбинация, наличие и порядок могут изменяться от самой команды, у каждой по разному. Внутренний парсер обрабатывает только несколько комбинаций и возможных случаев, которых должно быть достаточно. Рассмотрим комбинации параметров при создании команды и какие ответы они поддерживают:

| Указанные параметры команды | Необходимые поля ответа для таких параметров |
|-------------------|----------------------------|
| [REQ] | [REQ] [RES] с OK<br>[REQ] [RES] [DATA] с OK |
| [REQ] [PREFIX] <br> [REQ] [PREFIX] [FORMAT] [ARG]| [REQ] [RES] [DATA] + проверка префикса и формата(если указан)<br>[REQ] [DATA] + проверка префикса и формата(если указан)<br>[DATA] + проверка префикса и формата(если указан) |
| [PREFIX] | [DATA] + проверка префикса |
| [PREFIX] [FORMAT] [ARG] | [DATA] + проверка префикса и формата |

Поле [DATA] обычно обрамляется символами CRLF — парсер находит границы по ним, удаляет их и работает с "чистыми" данными. Если [DATA] содержит многострочные данные с CRLF, парсер вернёт только ПЕРВУЮ строку.
Для сложных случаев рекомендуется писать кастомный парсер через коллбек (пример: функция atl_mdl_rtd). Используйте таблицу выше чтобы правильно создать команду, заранее зная тип ответа и какие поля он содержит.


`ATL_PARCE_RAW`<br> 
Парсер команд формата пердачи данных. Парсит сырые данные просто проверяя их на совпадение с полем [PREFIX] если оно указано. Также если указано поле [FORMAT][ARG], то парсер попытается 
отформатировать и получить полезные данные с самого начала буфера этих данных. Данные никак не подготавливаются а обрабтываются как есть, это необходимо учитывать. Поле [REQ] можно не указывать.

## 4. Создание логических цепочек

На основе групп АТ команд (о которых было выше) можно создавать собственные алгоритмы и цепочки выполнения. Предоставляется АПИ в файле `atl_chain.h`:

- `atl_chain_destroy`
- `atl_chain_start` 
- `atl_chain_stop`
- `atl_chain_reset`
- `atl_chain_run`
- `atl_chain_is_running`
- `atl_chain_get_current_step`
- `atl_chain_get_current_step_name`

### Создание функций-шагов

Для того чтобы создать цепочку каждая группа команд должна быть обернута в функцию стандартного типа:

```c
bool (*function)(atl_entity_cb_t cb, void* param, void* ctx);
```
Функция должна по результату исполнять передаваемый в нее колбек. 

Например:

```c
bool atl_mdl_gprs_socket_connect(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(101, param);
  char cipstart[128] = {0}; 
  atl_mdl_tcp_server_t* tcp = (atl_mdl_tcp_server_t*)param;
  snprintf(cipstart, sizeof(cipstart), "%sAT+CIPSTART=\"%s\",\"%s\",\"%s\"%s", ATL_CMD_SAVE, tcp->mode, tcp->ip, tcp->port, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP STATUS|STATE: TCP CLOSED", ATL_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM(cipstart,                           "CONNECT OK|ALREADY CONNECT", ATL_PARCE_SIMCOM,  6, 500,  0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,                  "STATE: CONNECT OK", ATL_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSEND?"ATL_CMD_CRLF,                           "+CIPSEND:", ATL_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ATL_NO_ARG),         
    ATL_ITEM("AT+CIPQSEND?"ATL_CMD_CRLF,                       "+CIPQSEND: 0", ATL_PARCE_SIMCOM,  1, 100,  1, 0, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPQSEND=0"ATL_CMD_CRLF,                                NULL, ATL_PARCE_SIMCOM, 10, 100,  0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}
```

Где:
- **ctx** - глобальный контекст для которого создается группа.
- **cb** - коллбек на всю группу
- **param** - указатель на входящие параметры для создания АТ команды. Нужно следить чтобы предоставляемая структура существовала все время пока аснихронно выполняется эта группа команд
- **meta** - указатель на некоторые дополнительные мета данные группы. Передаются напрямую в коллбек выполнения.

Такие функции можно будет использовать в цепочках или отдельно. Наборы уже готовых функций представлены в папке `modules`, можно их использовать и добавлять новые свои.

### Создание цепочки

Давайте создадим пример цепочки на основании представленных в библиотеке модулей:

```c
chain_step_t tcp_steps[] = 
{   
  //Main
  ATL_CHAIN("INIT_MODEM", "NEXT", "MODEM_RESTART", atl_mdl_modem_init, NULL, NULL, NULL, 1),
  ATL_CHAIN("GPRS INIT", "NEXT", "GPRS DEINIT", atl_mdl_gprs_init, NULL, NULL, NULL, 1),
  ATL_CHAIN("SOCKET CONFIG", "NEXT", "GPRS INIT", atl_mdl_gprs_socket_config, NULL, NULL, NULL, 1),
  ATL_CHAIN("CONNECT TO SERVER", "NEXT", "SOCKET CONFIG", atl_mdl_gprs_socket_connect, NULL, &atl_server_connect, NULL, 1),
  ATL_CHAIN("GET RTD", "NEXT", "DISCONNECT FROM SERVER", atl_mdl_rtd, atl_rtd_cb, NULL, NULL, 1),
  ATL_CHAIN_EXEC("CHECK RTD", "NEXT", "GET RTD", atl_rtd_check),
  
  ATL_CHAIN_EXEC("CREATE WIALON LOGIN", "NEXT", "DISCONNECT FROM SERVER", atl_server_data_wialon_login),
  ATL_CHAIN("SEND WIALON LOGIN", "NEXT", "DISCONNECT FROM SERVER", atl_mdl_gprs_socket_send_recieve, NULL, &atl_server_data, NULL, 3),
  
  ATL_CHAIN_LOOP_START(10),
    ATL_CHAIN("GET RTD", "NEXT", "DISCONNECT FROM SERVER", atl_mdl_rtd, atl_rtd_cb, NULL, NULL, 1),
    ATL_CHAIN_EXEC("CREATE WIALON DATA", "NEXT", "DISCONNECT FROM SERVER", atl_server_data_wialon_packet),
    ATL_CHAIN("SEND WIALON DATA", "NEXT", "DISCONNECT FROM SERVER", atl_mdl_gprs_socket_send_recieve, NULL, &atl_server_data, NULL, 3),
    ATL_CHAIN_DELAY(1000),
  ATL_CHAIN_LOOP_END,
  
  ATL_CHAIN_EXEC("WIALON DATA CLEAN", "STOP", "HARD RESET", atl_server_data_clean),
  
  //Error
  ATL_CHAIN("GPRS DEINIT", "GPRS INIT", "MODEM RESTART", atl_mdl_gprs_deinit, NULL, NULL, NULL, 1),
  ATL_CHAIN("DISCONNECT FROM SERVER", "CONNECT TO SERVER", "GPRS DEINIT", atl_mdl_gprs_socket_disconnect, NULL, NULL, NULL, 1),
  ATL_CHAIN("MODEM RESTART", "GPRS INIT", "MODEM RESTART", atl_mdl_modem_reset, NULL, NULL, NULL, 1),
  
  //Critical
  ATL_CHAIN_EXEC("HARD RESET", "STOP", "STOP", atl_hard_reset),
};

atl_chain_t* chain = atl_chain_create("TCP", tcp_steps, sizeof(tcp_steps)/sizeof(chain_step_t), ctx);
atl_chain_start(chain);

while(atl_chain_is_running(chain))
{
  bool res = atl_chain_run(chain);
  if(atl_chain_is_running(chain)) atl_chain_destroy(chain);
}
```

Данная цепочка единоразово настраивает simcom модем, контекст и соединение, получает данные реального времени, проверяет их, а затем 10 раз начинает собирать и слать эти данные на сервер с периодом в 10 сек с предварительным прохождением авторизации, в случае возникновения ошибки происходит отключение или деинициализация с попыткой повторной реинициализации и подключения или полного рестарта.

### Параметры цепочки

Итак, давайте рассмотрим что можно использовать в цепочке и какие параметры туда передавать:

**ATL_CHAIN** - основной макрос для добавления шага выполнения, содержит:
- **[Name]** - Имя шага.
- **[Success target]** - Имя шага куда идти в случае успеха. Можем указать NULL или "NEXT" для шага на +1 или "PREV" для шага на -1, ну или конкретное имя шага. "STOP" закончит выполнение цепочки.
- **[Error target]** - тоже самое что и при успехе только в случае ошибки. 
- **[Func]** - та самая функция которая будет вызвана для исполнения шага. 
- **[Cb]** - Коллбек на шаг.
- **[Param]** - те самые параметры которые будут переданы в указанную функцию когда она начнет исполнятся
- **[meta]** - те самые мета данные которые мы передавали при создании.
- **[Retries]** - количество повторов в случае ошибки 

**ATL_CHAIN_EXEC** - макрос для создания и исполнения некоторого действия, можно что то исполнить или проверить. Содержит:
- **[Name]** - Имя шага.
- **[True target]** - Имя шага куда идти в случае успеха. Можем указать NULL или "NEXT" для шага на +1 или "PREV" для шага на -1, ну или конкретное имя шага. "STOP" закончит выполнение цепочки.
- **[False target]** - тоже самое что и при успехе только в случае ошибки.
- **[Exec func]** - функция для исполнения. Должна быть типа bool (*exec)(void); Ее результат выполнения влияет на следующий переход.

**ATL_CHAIN_LOOP_START** - макрос для указания начала цикла далее, содержит:
- **[Iterations]** - Количество итераций цикла. 0 - Бесконечный.

**ATL_CHAIN_LOOP_END** - макрос для указания конца цикла.

**ATL_CHAIN_DELAY** - макрос для создания искусственной задержки, содержит:
- **[ms]** - время задержки в мс.

### Параметры функции atl_entity_enqueue

```c
atl_chain_t* atl_chain_create(const char* const name, const chain_step_t* const steps, const uint32_t step_count);
```

- **name** - Имя цепочки.
- **steps** - Сама структура цепочки.
- **step_count** - Количество шагов в цепочке

### Особенности

- Как можно видеть из примера можно создавать вложенные циклы, библиотека реализует и использует собственный динамический стек для работы с ними. Есть поддержка и возможность переходов через циклы, из циклов, в циклы с вложенностью и без, переходить как вперед так и назад, пропуская их обозначающие шаги, ограничений нет, однако, стоит учитывать что одна итерация цикла происходит только в моменте выполнения ATL_CHAIN_LOOP_END для соответствующего цикла. Также нужно следить за отсутствием шагов с одинаковыми именами, в таком случае переход на такой шаг будет выполнен для первого найденного в массиве.