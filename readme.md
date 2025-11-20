# Документация ATL

## 1. Особенности

- **Асинхронность**: Все процессы библиотеки выполняются без единого блокирования системы
- **Управление командами**: АПИ для отправки групп AT-команд и построения сложных логических цепочек
- **Память**: Библиотека после инициализации использует около 2.5кБ оперативной памяти по дефолту, зависит от размера выделенного буфера для пользовательского динамического аллокатора O1heap. [GitHub](https://github.com/pavel-kirienko/o1heap)
- **Обработка ошибок**: Design by Contract (DBC) для надежной проверки ошибок.   [GitHub](https://github.com/QuantumLeaps/DBC-for-embedded-C)
- **Потокобезопасность**: Защита критических секций для многопоточных приложений.
- **Поддержка URC**: Обработка URC для асинхронных событий.
- **Тестирование**: Embedded тесты от QLP. [GitHub](https://github.com/QuantumLeaps/Embedded-Test)
- **Срезы**: Использование кольцевых срезов вместо прямого копирования буфера. [GitHub](https://github.com/ferrero222/ringslice/tree/dev)

Протестировано с: SIM868....

---

## 2. Настройка и предварительные требования

### Базовая конфигурация

Определите свой кольцевой буфер и указатели:

```c
typedef struct {
  uint8_t *buffer;      
  uint16_t size;   
  uint16_t head;       
  uint16_t tail;       
  uint16_t count;     
} atl_ring_buffer_t;
```

Инициализируйте ATL:

```c
atl_init(
    your_printf_function,     // Пользовательская printf для отладки
    your_uart_write_function, // Функция записи в UART  
    your_ring_buffer_struct,  // Структура кольцевого буфера
);
```

### Параметры конфигурации

В папке port, в C файле реализуйте обработчики критических секций для защиты от параллельного доступа а также обработчик исключений:

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

В h файле настройте параметры:

```c
#define ATL_MAX_ITEMS_PER_ENTITY 50     //Max amount of AT cmds in one group
#define ATL_ENTITY_QUEUE_SIZE    10     //Max amount of groups 
#define ATL_URC_QUEUE_SIZE       10     //Amount of handled URC
#define ATL_MEMORY_POOL_SIZE     4096   //Memory pool for custom heap
#ifndef ATL_TEST
  #define ATL_DEBUG_ENABLED      1      //Recommend to turn on DEBUG logs
#endif
```

Вызывайте основную функцию обработки каждые 10ms в системном таймере:

```c
void timer_10ms_handler(void) {
  atl_core_proc();
}
```

---

## 3. АТ команды

В файле `atl_core.h` представлено АПИ для работы с АТ командами и самим ядром библиотеки, содержащее:

- `atl_entity_enqueue`
- `atl_entity_dequeue` 
- `atl_urc_enqueue`
- `atl_urc_dequeue`
- `atl_core_proc`
- `atl_get_init`

Подробнее о функциях и их параметрах читайте в самом файле. Разберем некоторые примеры использования.

### Пример 1, команды без форматирования ответа

```c
atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
{
  ATL_ITEM("AT+CIPMODE?"ATL_CMD_CRLF,        "+CIPMODE: 0",  3, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMODE=0"ATL_CMD_CRLF,                NULL, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMUX?"ATL_CMD_CRLF,          "+CIPMUX: 0",  3, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMUX=0"ATL_CMD_CRLF,                 NULL, 30, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,   "STATE: IP START",  3, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CSTT=\"\",\"\",\"\""ATL_CMD_CRLF,      NULL, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP GPRSACT",  3, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIICR"ATL_CMD_CRLF,                    NULL, 30, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP GPRSACT",  3, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIFSR"ATL_CMD_CRLF,           ATL_CMD_FORCE, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPHEAD?"ATL_CMD_CRLF,        "+CIPHEAD: 1",  3, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPHEAD=1"ATL_CMD_CRLF,                NULL, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSRIP?"ATL_CMD_CRLF,        "+CIPSRIP: 1",  3, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSRIP=1"ATL_CMD_CRLF,                NULL, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSHOWTP?"ATL_CMD_CRLF,    "+CIPSHOWTP: 1",  3, 100, 1, 0, NULL, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSHOWTP=1"ATL_CMD_CRLF,              NULL, 10, 100, 0, 0, NULL, NULL, ATL_NO_ARG),
};
atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx);
```

### Пример 2, команды с форматированием ответа

```c
atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
{   
  ATL_ITEM("AT+GSN"ATL_CMD_CRLF,       NULL, 10, 100, 0, 1, NULL,               "%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
  ATL_ITEM("AT+GMM"ATL_CMD_CRLF,       NULL, 10, 100, 0, 1, NULL,               "%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_id)),  
  ATL_ITEM("AT+GMR"ATL_CMD_CRLF,       NULL, 10, 100, 0, 1, NULL,      "Revision:%29[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_rev)),                
  ATL_ITEM("AT+CCLK?"ATL_CMD_CRLF,  "+CCLK", 10, 100, 0, 1, NULL,      "+CCLK: \"%21[^\"]\"", ATL_ARG(atl_mdl_rtd_t, modem_clock)),          
  ATL_ITEM("AT+CCID"ATL_CMD_CRLF,      NULL, 10, 100, 0, 1, NULL,               "%21[^\x0d]", ATL_ARG(atl_mdl_rtd_t, sim_iccid)),             
  ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,  "+COPS", 20, 100, 0, 1, NULL, "+COPS: 0, 0,\"%49[^\"]\"", ATL_ARG(atl_mdl_rtd_t, sim_operator)),            
  ATL_ITEM("AT+CSQ"ATL_CMD_CRLF,     "+CSQ", 10, 100, 0, 1, NULL,                 "+CSQ: %d", ATL_ARG(atl_mdl_rtd_t, sim_rssi)),             
  ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,    NULL, 10, 100, 0, 1, NULL,                       NULL, ATL_NO_ARG),                
  ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,     NULL, 10, 100, 0, 0, atl_mdl_general_ceng_cb,    NULL, ATL_NO_ARG),                         
};
atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_rtd_t), ctx);
```

### Параметры АТ команды

Рассмотрим отдельно каждый указываемый параметр АТ команды и разберем что происходит:

- **ATL_ITEM** - макрос для заполнения структуры, используйте для добавления АТ команды в группу.
- **[REQ]** - сам запрос, состоит из конкатенации АТ команды и ATL_CMD_CRLF, в данном случае является строковым литералом, но может быть и массивом символов.
- **[PREFIX]** - строковый литерал который будет искаться в ответе. Можно не указывать.
- **[FORMAT]** - формат парсинга ответа для SSCANF, используется вместе с VA_ARGS, полученные данные будут собраны в соответствии с этим форматом, положены в аргументы и переданы в коллбек группы. Можно не указывать.
- **[RPT]** - количество повторов в случае возникновения ошибки.
- **[WAIT]** - таймер ожидания ответа в 10мс.
- **[STEPERROR]** - в случае возникновения ошибки у команды мы можем перескочить на несколько шагов вперед или назад внутри группы, либо ничего не делать. 0 завершает выполнение всей группы
- **[STEPOK]** - как и в случае ошибки, но тут в случае успеха можем прошагать на какую-либо конкретную АТ команду.  0 завершает выполнение всей группы
- **[CB]** - коллбек на АТ команду, будет вызван по результату выполнения АТ команды. В него же передаются полученные форматом данные. Можно не указывать
- **[VA_ARG]** - аргументы для формата в виде ATL_ARG, где мы указываем какая структура и какое поле будет использоваться чтобы сохранить данные из формата. Можно не указывать. Максимум 6.

### Параметры функции atl_entity_enqueue

```c
bool atl_entity_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, void* const ctx);
```

- **item** - собственно наша группа АТ команд.
- **item_amount** - размер структуры команд.
- **cb** - коллбек на всю группу, который будет вызван по исполнению этой группы. Можно не указывать
- **data_size** - размер структуры данных которая будет создана динамически, автоматически, для сохранения данных указанных в полях [FORMAT] и [VA_ARGS], указатель на эту структуру вернется в коллбек на группу а после исполнения память для него будет очищена. Может не указываться (0).
- **ctx** - указатель на некий пользовательский контекст выполнения группы, который будет передан в коллбек выполнения. По сути некие мета-данные группы которые где-то объявлены. Можно не указывать.

### Параметры коллбека на всю группу команд:

```c
void (*atl_entity_cb_t)(bool result, void* ctx, void* data);
```

- **result** - результат.
- **ctx** - тот самый контекст который мы передавали при создании.
- **data** - та самая структура которая была создана динамически так как мы указывали размер, либо NULL если мы ничего не указывали.

### Параметры коллбека на АТ команду:

```c
(*answ_parce_cb_t)(ringslice_t data_slice, bool result, void* data);
```

- **data_slice** - библиотека работает с кольцевым буфером напрямую используя срезы, здесь вернется срез данных для этой АТ команды, если он есть, используйте АПИ библиотеки ringslice чтобы работать с этим срезом. 
- **result** - результат выполнения.
- **data** - указатель на ту самую созданную структуру данных для форматирования, если мы указали это при создании.

### Особенности

- Если хотите получить данные из ответа но не передали размер структуры для создания или формат получаемых, то в процессе выполнения сработает ассерт.
- Для получения полезных данных из ответа и их сбор, библиотека сама создает динамически структуру данных если вы все правильно указали при создании группы команд, передает ее в коллбек выполнения, а затем сама также ее удаляет после завершения. Поэтому если данные нужны какое то время то скопируйте их из коллбека в свою статическую структуру.
- Иногда заранее неизвестно из чего будет состоять команда и нужно создать ее в реальном времени на стеке или в каком то временном буфере, а не использовать статический строковый литерал. Так как библиотека работает асинхронно, то нужно следить за тем чтобы этот временный буфер существовал в момент исполнения команды. Чтобы каждый раз не заниматься этим, мы можем просто указать чтобы команда сохранилась в память библиотеки используя макрос `ATL_CMD_SAVE`, например: `ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF`.
- В поле [PREFIX] можно указывать более сложные конструкции для проверки сразу нескольких строк и префиксов:
  - Используйте `|` для операций ИЛИ: `"+CREG: 0,1|+CREG: 0,5"`
  - Используйте `&` для операций И: `"+IPD&SEND OK"`
- Также в поле [PREFIX] можно указать макрос-литерал `ATL_CMD_FORCE` который будет указывать что данная команда или данные должна/ы быть просто отправлена/ы, без парсинга и ожидания ответа. Остальные поля кроме [STEPOK] не будут никак использованы и их содержимое может быть любым.
- Можно не указывать запрос в команде но передать ожидание для создания искусственной задержки между командами, или просто распарсить и отформатировать данные в буфере.
- В папке modules и в папке tests содержатся некоторые файлы и реализации уже готовых групп ат команд.
- В папке tests есть make файл который запускает тесты на ваше машине для проверки логики вне зависимсоти от микроконтроллера. Вы можете запустить его через консоль или отладить через VS code, попробовать добавить свои или посмотреть как происходит процесс выполнения готовых тестов, чтобы понять как что работает.
- Коллбек на АТ команду получает срез на данные, это сделано для того чтобы вы могли написать собственный парсер данных, в случае если стандартного форматирования недостаточно, пример такого можно увидеть в готовой функции atl_mdl_rtd, там структура данных динамически создается библиотекой и в коллбеке на команду мы вручную парсим ее данные нужным способом и кладем их в эту структуру.
- В случае обработанных данных библиотека сама передвигает tail и counter вашего кольцевого буфера.


### Формат, префикс, аргументы и парсер
Парсер АТ команд работает таким образом что сначала ищет эхо команды, потом результат выполнения а потом полезные данные, т.е. получаем три поля ответа: REQ, RES, DATA. Их комбинация, наличие и порядок могут изменяться от самой команды, у каждой оп разному. Внутренний парсер обрабатывает только несколько комбинаций и возможных случаев, которых должно быть достаточно:
 -  **REQ  RES  NULL**  - парсер нашел эхо, нашел результат, но нет данных(пока?). В таком случае мы можем получить успех обработки только если передали в команду  запрос, без поля префикса, без формата и без аргументов.
 -  **REQ  NULL DATA**  - парсер нашел эхо и данные, но нет результата. Чтобы получить успех выполнения для такого типа, нужно передать в команду запрос, префикс, формат и аргументы.
 -  **REQ  RES  DATA**  - парсер нашел всё. Достаточно передать в команду только сам запрос чтобы получить успех, остальные поля по желанию.
 -  **NULL NULL DATA**  - есть только какие то данные. Мы можем получить успех в таком случае если передали префикс, формат и аргументы и они были успешно обработаны.
Таким образом, чтобы указать правильный набор параметров для успешной обработки, нужно заранее понимать что вы получите в ответ на команду и в каком виде.
Также стоит отметить что парсер получает поле DATA до первых \r\n. Т.е. если полезные данные в ответе на команду каким то образом перечислены через \r\n то парсер передаст лишь первую часть от них. В таких случая рекомендуется распарсить данные вручную используя свой кастомный колбек на команду куда передается весь этот буфер данных.
---

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
- `atl_chain_print_stats`

### Создание функций-шагов

Для того чтобы создать цепочку каждая группа команд должна быть обернута в функцию стандартного типа:

```c
bool (*function)(atl_entity_cb_t cb, void* param, void* ctx);
```

Например:

```c
bool atl_mdl_gprs_socket_connect(const atl_entity_cb_t cb, const void* const param, void* const ctx);
{
  DBC_REQUIRE(101, param);
  char cipstart[128] = {0}; 
  atl_mdl_tcp_server_t* tcp = (atl_mdl_tcp_server_t*)param;
  snprintf(cipstart, sizeof(cipstart), "%sAT+CIPSTART=\"%s\",\"%s\",\"%s\"%s", 
           ATL_CMD_SAVE, tcp->mode, tcp->ip, tcp->port, ATL_CMD_CRLF); 
  
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP STATUS|STATE: TCP CLOSED",           NULL, 2, 250,  0, 1, NULL, NULL),
    ATL_ITEM(cipstart,                                           "CONNECT OK",           NULL, 5, 1600, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,                  "STATE: CONNECT OK",           NULL, 2, 250,  0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSEND?"ATL_CMD_CRLF,                           "+CIPSEND:", "+CIPSEND: %u", 2, 150,  0, 1, NULL, NULL, &atl_mdl_gprs_send_len),         
    ATL_ITEM("AT+CIPQSEND?"ATL_CMD_CRLF,                       "+CIPQSEND: 0",           NULL, 1, 150,  0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPQSEND=0"ATL_CMD_CRLF,                                NULL,           NULL, 2, 150,  0, 0, NULL, NULL),
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}
```

Где:
- **cb** - коллбек на всю группу
- **param** - указатель на входящие параметры для создания АТ команды. Нужно следить чтобы предоставляемая структура существовала все время пока аснихронно выполняется эта группа команд
- **ctx** - контекст который будет передан в коллбек

Такие функции можно будет использовать в цепочках или отдельно. Наборы уже готовых функций представлены в папке `modules`, можно их использовать и добавлять новые свои.

### Создание цепочки

Давайте создадим пример цепочки на основании представленных в библиотеке модулей:

```c
chain_step_t server_steps[] = 
{   
    ATL_CHAIN("MODEM INIT", "NEXT", "MODEM RESET", atl_mdl_modem_init, entity_cb, param, ctx, 3),
    ATL_CHAIN("GPRS INIT", "NEXT", "MODEM RESET", atl_mdl_gprs_init, entity_cb, param, ctx, 3),
    ATL_CHAIN("SOCKET CONFIG", "NEXT", "GPRS DEINIT", atl_mdl_gprs_socket_config,  entity_cb, param, ctx, 3),
    ATL_CHAIN("SOCKET CONNECT", "NEXT", "GPRS DEINIT", atl_mdl_gprs_socket_connect, entity_cb, param, ctx, 3),
  
    ATL_CHAIN_LOOP_START(0), 
//  {
      ATL_CHAIN("MODEM RTD", "NEXT", "SOCKET DISCONNECT", atl_mdl_rtd, entity_cb, param, ctx, 3),
      ATL_CHAIN_LOOP_START(3), 
//    {
        ATL_CHAIN("MODEM RTD", "NEXT", "SOCKET DISCONNECT", atl_mdl_rtd, entity_cb, param, ctx, 3),
        ATL_CHAIN("SOCKET SEND RECIVE", "NEXT", "SOCKET DISCONNECT", atl_mdl_gprs_socket_send, entity_cb, param, ctx, 3),
        ATL_CHAIN_LOOP_END,
//    {        
      ATL_CHAIN("SOCKET SEND RECIVE", "NEXT", "SOCKET DISCONNECT", atl_mdl_gprs_socket_send, entity_cb, param, ctx, 3),
      ATL_CHAIN_DELAY(60000),
      ATL_CHAIN_LOOP_END,
//  }

    ATL_CHAIN_COND("CHECK CONN", "NEXT", "MODEM RESET", atl_check_cond),
    ATL_CHAIN("SOCKET DISCONNECT", "NEXT", "PREV", atl_mdl_gprs_socket_disconnect, entity_cb, param, ctx, 3),
    ATL_CHAIN("GPRS DEINIT",  "NEXT", "MODEM RESET", atl_mdl_gprs_deinit, entity_cb, param, ctx, 3),
    ATL_CHAIN("MODEM RESET",  "NEXT", "STOP", atl_mdl_modem_reset, entity_cb, param, ctx, 3),
};

atl_chain_t* chain = atl_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t));
atl_chain_start(chain);
while(atl_chain_is_running(chain))
{
  bool res = atl_chain_run(chain);
  if(atl_chain_is_running(chain))atl_chain_destroy(chain);
}
```

Данная цепочка единоразово настраивает simcom модем, контекст и соединение а затем бесконечно начинает собирать и слать данные на сервер с периодом в 60с, в случае возникновения ошибки происходит отключение или деинициализация с попыткой повторной инициализации сервера.

### Параметры цепочки

Итак, давайте рассмотрим что можно использовать в цепочке и какие параметры туда передавать:

**ATL_CHAIN** - основной макрос для добавления шага выполнения, содержит:
- **[Name]** - Имя шага.
- **[Success target]** - Имя шага куда идти в случае успеха. Можем указать NULL или "NEXT" для шага на +1 или "PREV" для шага на -1, ну или конкретное имя шага. "STOP" закончит выполнение цепочки.
- **[Error target]** - тоже самое что и при успехе только в случае ошибки. 
- **[Func]** - та самая функция которая будет вызвана для исполнения шага. 
- **[Cb]** - Коллбек на шаг 
- **[Param]** - те самые параметры которые будут переданы в указанную функцию когда она начнет исполнятся
- **[Ctx]** - тот самый коллбек который будет передан в функцию.
- **[Retries]** - количество повторов в случае ошибки 

**ATL_CHAIN_COND** - макрос для создания и проверки условия, содержит:
- **[Name]** - Имя шага.
- **[True target]** - Имя шага куда идти в случае успеха. Можем указать NULL или "NEXT" для шага на +1 или "PREV" для шага на -1, ну или конкретное имя шага. "STOP" закончит выполнение цепочки.
- **[False target]** - тоже самое что и при успехе только в случае ошибки.
- **[Cond func]** - функция для проверки условия. Должна быть типа bool (*condition)(void);

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

- Как можно видеть из примера можно создавать вложенные циклы, библиотека использует динамический стек для хранения данных. Однако, необходимо следить чтобы не было переходов извне в цикл и из цикла наружу без выполнения LOOP_START или LOOP_END. Библиотека отслеживает такие ситуации и высчитывает баланс, в случае не нулевого цепочка будет просто завершена.