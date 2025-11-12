[English](#english) | [Русский](#russian)


<a id="russian"></a>
# Документация ATL

## 1. Особенности

- **Асинхронность**: Все процессы библиотеки выполняются без единого блокирования системы
- **Управление командами**: АПИ для отправки групп AT-команд и построения сложных логических цепочек
- **Память**: Библиотека после инициализации использует около 2.5кБ оперативной памяти по дефолту, зависит от размера выделенного буфера для пользовательского динамического аллокатора TLSF. Выделять за раз в TLSF можно блок не более 16кБ. Для выделения памяти TLSF единоразово создает как раз оверхэд структуру в 500 байт. [GitHub](https://github.com/mattconte/tlsf)
- **Обработка ошибок**: Design by Contract (DBC) для надежной проверки ошибок.   [GitHub](https://github.com/QuantumLeaps/DBC-for-embedded-C)
- **Потокобезопасность**: Защита критических секций для многопоточных приложений.
- **Поддержка URC**: Обработка URC для асинхронных событий.
- **Тестирование**: Embedded тесты от QLP. [GitHub](https://github.com/QuantumLeaps/Embedded-Test)
- **Срезы**: Использование кольцевых срезов вместо прямого копирования буфера. [GitHub](https://github.com/ferrero222/ringslice/tree/dev)

---

## 2. Настройка и предварительные требования

### Базовая конфигурация

Определите свой кольцевой буфер и указатели:

```c
uint8_t rx_ring_buffer[1024];
uint16_t ring_head = 0;
uint16_t ring_tail = 0;
```

Инициализируйте ATL:

```c
atl_init(
    your_printf_function,     // Пользовательская printf для отладки
    your_uart_write_function, // Функция записи в UART  
    rx_ring_buffer,           // RX кольцевой буфер
    sizeof(rx_ring_buffer),   // Размер буфера
    &ring_tail,               // Указатель на хвост
    &ring_head                // Указатель на голову
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

void atl_crit_enter(void) 
{ 
  /* disable irq if needed */
}

void atl_crit_exit(void)  
{
   /* enable irq  if needed */ 
}
```

В h файле настройке параметры макросы:

```c
#define ATL_MAX_ITEMS_PER_ENTITY 50     //Max amount of AT cmds in one group
#define ATL_ENTITY_QUEUE_SIZE    10     //Max amount of groups 
#define ATL_URC_QUEUE_SIZE       10     //Amount of handled URC
#define ATL_MEMORY_POOL_SIZE     4096   //Memory pool for TLSF heap
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
- `atl_get_cur_time`
- `atl_get_init`

Подробнее о функциях и их параметрах читайте в самом файле. Разберем некоторые примеры использования.

### Пример 1, команды без форматирования ответа

```c
atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
{
  ATL_ITEM(ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF,     "+CIPMODE: 0", NULL, 1, 150, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMODE=0"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMUX?"ATL_CMD_CRLF,                   "+CIPMUX: 0", NULL, 1, 150, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMUX=0"ATL_CMD_CRLF,                          NULL, NULL, 2, 500, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,            "STATE: IP START", NULL, 1, 250, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CSTT=\"\",\"\",\"\""ATL_CMD_CRLF,               NULL, NULL, 2, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,          "STATE: IP GPRSACT", NULL, 1, 250, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIICR"ATL_CMD_CRLF,                             NULL, NULL, 4, 800, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,          "STATE: IP GPRSACT", NULL, 1, 250, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIFSR"ATL_CMD_CRLF,                             NULL, NULL, 1, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPHEAD?"ATL_CMD_CRLF,                 "+CIPHEAD: 1", NULL, 1, 150, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPHEAD=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSRIP?"ATL_CMD_CRLF,                 "+CIPSRIP: 1", NULL, 1, 150, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSRIP=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSHOWTP?"ATL_CMD_CRLF,             "+CIPSHOWTP: 1", NULL, 1, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSHOWTP=1"ATL_CMD_CRLF,                       NULL, NULL, 2, 150, 0, 0, NULL, ATL_NO_ARG),
};
atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx);
```

### Пример 2, команды с форматированием ответа

```c
atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
{   
  ATL_ITEM("AT+GSN"ATL_CMD_CRLF,       NULL,               "%15[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_imei)),
  ATL_ITEM("AT+GMM"ATL_CMD_CRLF,       NULL,               "%15[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_id)),  
  ATL_ITEM("AT+GMR"ATL_CMD_CRLF,       NULL,      "Revision:%29[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_rev)),                
  ATL_ITEM("AT+CCLK?"ATL_CMD_CRLF,     NULL,      "+CCLK: \"%21[^\"]\"", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_clock)),          
  ATL_ITEM("AT+CCID"ATL_CMD_CRLF,   "+CCLK",               "%21[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_iccid)),             
  ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,  "+COPS", "+COPS: 0, 0,\"%49[^\"]\"", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_operator)),            
  ATL_ITEM("AT+CSQ"ATL_CMD_CRLF,     "+CSQ",                 "+CSQ: %d", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_rssi)),             
  ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,    NULL,                       NULL, 2, 600, 0, 1, NULL, NULL),                
  ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,     NULL,                       NULL, 2, 600, 0, 0, atl_mdl_general_ceng_cb, NULL),                         
};
atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_rtd_t), ctx);
```

### Параметры АТ команды

Итак, давайте рассмотрим отдельно каждый указываемый параметр АТ команды и разберем что происходит:

- **ATL_ITEM** - макрос для заполнения структуры, используйте для добавления АТ команды в группу.
- **[REQ]** - сам запрос, состоит из конкатенации АТ команды и ATL_CMD_CRLF, в данном случае является строковым литералом, но может быть и массивом символов*.
- **[PREFIX]** - строковый литерал который будет искаться в ответе. Можно не указывать.
- **[FORMAT]** - формат парсинга ответа для SSCANF, используется вместе с VA_ARGS, полученные данные будут собраны в соответствии с этим форматом, положены в аргументы и переданы в коллбек группы. Можно не указывать.
- **[RPT]** - количество повторов в случае возникновения ошибки (0...15).
- **[WAIT]** - таймер ожидания ответа в 10мс (0...4095).
- **[STEPERROR]** - в случае возникновения ошибки у команды мы можем перескочить на несколько шагов вперед или назад внутри группы, либо ничего не делать (-31...31).
- **[STEPOK]** - как и в случае ошибки, но тут в случае успеха можем прошагать на какую-либо конкретную АТ команду (-31...31), по дефолту шагаем на +1.
- **[CB]** - коллбек на АТ команду, будет вызван по результату выполнения АТ команды. В него же передаются полученные форматом данные. Можно не указывать
- **[VA_ARG]** - аргументы для формата в виде ATL_ARG, где мы указываем какая структура и какое поле будет использоваться чтобы сохранить данные из формата. Можно не указывать. Максимум 6.

### Параметры функции atl_entity_enqueue

```c
bool atl_entity_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, void* const ctx);
```

- **item** - собственно наша группа АТ команд.
- **item_amount** - размер структуры команд.
- **cb** - коллбек на всю группу, который будет вызван по исполнению этой группы. Можно не указывать
- **data_size** - размер структуры данных которая будет создана динамически автоматически для сохранения данных указанных в полях [FORMAT] и [VA_ARGS], указатель на эту структуру вернется в коллбек на группу а после исполнения память для него будет очищена. Может не указываться.
- **ctx** - указатель на некий пользовательский контекст выполнения группы, который будет передан в коллбек выполнения. По сути некие мета-данные группы которые где-то объявлены. Можно не указывать

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

- Если хотите получить данные из ответа но не передали размер структуры для создания или формат, то в процессе выполнения сработает ассерт.
- Для получения полезных данных, библиотека сама создает динамически структуру данных если вы все правильно указали, передает ее в коллбек выполнения, а затем сама также ее удаляет после завершения. Поэтому если данные нужны какое то время то скопируйте их из коллбека в свою структуру.
- Иногда нужно создать команду в ран-тайме на стеке, а не использовать статический строковый литерал. Так как библиотека работает асинхронно, мы можем указать ей чтобы она сохранила команду себе в память используя макрос `ATL_CMD_SAVE`, например: `ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF`.
- В поле [PREFIX] можно указывать более сложные конструкции для проверки сразу нескольких строк:
  - Используйте `|` для операций ИЛИ: `"+CREG: 0,1|+CREG: 0,5"`
  - Используйте `&` для операций И: `"+IPD&SEND OK"`
- Можно не указывать запрос в команде но передать ожидание для создания искусственной задержки между командами, или просто распарсить и отфарматировать данные в буфере.
- В папке modules и вв папке tests содержатся файлы и реализации уже готовых групп ат команд. Для полного понимаю можно заглянуть туда.
- Парсер АТ команд работает таким образом что сначала ищет эхо команды, потом результат выполнения а потом данные, если вы указываете только запрос, то он вернет ошибку если в ответе его не будет. Однако если вы также укажете поиск префикса или данных, то даже если не будет результата или эхо, но он найдет ожидаемые данные или префикс, то вернется успешное выполнение. То есть необязательно наличие всей комбинации ЭХО, РЕЗУЛЬТАТ, ДАННЫЕ в ответе, если вы указали для команды достаточный набор для поиска и хотя бы часть есть в ответе. Изучите примеры.
- В папке tests есть make файл который запускает тесты на ваше машине для проверки логики вне зависимсоти от микроконтроллера. Вы можете запустить его через консоль или отладить через VS code, попробовать добавить свои или посмотреть как происходит процесс выполнения готовых тестов, чтобы понять как что работает.
- Коллбек на АТ команду получает срез на данные, это сделано для того чтобы вы могли написать собственный парсер данных, в случае если стандартного форматирования недостаточно, пример такого можно увидеть в готовой функции atl_mdl_rtd, там структура данных динамически создается библиотекой и в коллбеке на команду мы вручную парсим ее данные нужным способом и кладем их в эту структуру.

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
    ATL_CHAIN("MODEM RESET",  "NEXT", "MODEM RESET", atl_mdl_modem_reset, entity_cb, param, ctx, 3),
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
- **[Success target]** - Имя шага куда идти в случае успеха. Можем указать NULL или "NEXT" для шага на +1 или "PREV" для шага на -1, ну или конкретное имя шага.
- **[Error target]** - тоже самое что и при успехе только в случае ошибки. 
- **[Func]** - та самая функция которая будет вызвана для исполнения шага. 
- **[Cb]** - Коллбек на шаг 
- **[Param]** - те самые параметры которые будут переданы в указанную функцию когда она начнет исполнятся
- **[Ctx]** - тот самый коллбек который будет передан в функцию.
- **[Retries]** - количество повторов в случае ошибки 

**ATL_CHAIN_COND** - макрос для создания и проверки условия, содержит:
- **[Name]** - Имя шага.
- **[True target]** - Имя шага куда идти в случае успеха. Можем указать NULL или "NEXT" для шага на +1 или "PREV" для шага на -1, ну или конкретное имя шага.
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




<a id="english"></a>
# ATL Documentation

## 1. Features

- **Asynchronous**: All library processes execute without any system blocking
- **Command management**: API for sending AT command groups and building complex logical chains
- **Memory**: After initialization, the library uses about 2.5KB of RAM by default, depends on the size of the allocated buffer for user TLSF dynamic allocator. Maximum block size that can be allocated in TLSF at once is 16KB. For memory allocation, TLSF creates a 500-byte overhead structure once. [GitHub](https://github.com/mattconte/tlsf)
- **Error handling**: Design by Contract (DBC) for reliable error checking. [GitHub](https://github.com/QuantumLeaps/DBC-for-embedded-C)
- **Thread safety**: Critical section protection for multi-threaded applications.
- **URC support**: URC handling for asynchronous events.
- **Testing**: Embedded tests from QLP. [GitHub](https://github.com/QuantumLeaps/Embedded-Test)
- **Slices**: Using ring slices instead of direct buffer copying. [GitHub](https://github.com/ferrero222/ringslice/tree/dev)

---

## 2. Setup and Prerequisites

### Basic Configuration

Define your ring buffer and pointers:

```c
uint8_t rx_ring_buffer[1024];
uint16_t ring_head = 0;
uint16_t ring_tail = 0;
```

Initialize ATL:

```c
atl_init(
    your_printf_function,     // Custom printf for debugging
    your_uart_write_function, // UART write function  
    rx_ring_buffer,           // RX ring buffer
    sizeof(rx_ring_buffer),   // Buffer size
    &ring_tail,               // Tail pointer
    &ring_head                // Head pointer
);
```

### Configuration Parameters

In the port folder, implement critical section handlers and exception handler in the C file:

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

void atl_crit_enter(void) 
{ 
  /* disable irq if needed */
}

void atl_crit_exit(void)  
{
   /* enable irq  if needed */ 
}
```

Configure macro parameters in the h file:

```c
#define ATL_MAX_ITEMS_PER_ENTITY 50     //Max amount of AT cmds in one group
#define ATL_ENTITY_QUEUE_SIZE    10     //Max amount of groups 
#define ATL_URC_QUEUE_SIZE       10     //Amount of handled URC
#define ATL_MEMORY_POOL_SIZE     4096   //Memory pool for TLSF heap
#ifndef ATL_TEST
  #define ATL_DEBUG_ENABLED      1      //Recommend to turn on DEBUG logs
#endif
```

Call the main processing function every 10ms in the system timer:

```c
void timer_10ms_handler(void) {
  atl_core_proc();
}
```

---

## 3. AT Commands

The `atl_core.h` file provides API for working with AT commands and the library core, containing:

- `atl_entity_enqueue`
- `atl_entity_dequeue` 
- `atl_urc_enqueue`
- `atl_urc_dequeue`
- `atl_core_proc`
- `atl_get_cur_time`
- `atl_get_init`

Read more about functions and their parameters in the file itself. Let's examine some usage examples.

### Example 1, commands without response formatting

```c
atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
{
  ATL_ITEM(ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF,     "+CIPMODE: 0", NULL, 1, 150, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMODE=0"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMUX?"ATL_CMD_CRLF,                   "+CIPMUX: 0", NULL, 1, 150, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPMUX=0"ATL_CMD_CRLF,                          NULL, NULL, 2, 500, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,            "STATE: IP START", NULL, 1, 250, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CSTT=\"\",\"\",\"\""ATL_CMD_CRLF,               NULL, NULL, 2, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,          "STATE: IP GPRSACT", NULL, 1, 250, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIICR"ATL_CMD_CRLF,                             NULL, NULL, 4, 800, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,          "STATE: IP GPRSACT", NULL, 1, 250, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIFSR"ATL_CMD_CRLF,                             NULL, NULL, 1, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPHEAD?"ATL_CMD_CRLF,                 "+CIPHEAD: 1", NULL, 1, 150, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPHEAD=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSRIP?"ATL_CMD_CRLF,                 "+CIPSRIP: 1", NULL, 1, 150, 0, 2, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSRIP=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSHOWTP?"ATL_CMD_CRLF,             "+CIPSHOWTP: 1", NULL, 1, 150, 0, 1, NULL, ATL_NO_ARG),
  ATL_ITEM("AT+CIPSHOWTP=1"ATL_CMD_CRLF,                       NULL, NULL, 2, 150, 0, 0, NULL, ATL_NO_ARG),
};
atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx);
```

### Example 2, commands with response formatting

```c
atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
{   
  ATL_ITEM("AT+GSN"ATL_CMD_CRLF,       NULL,               "%15[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_imei)),
  ATL_ITEM("AT+GMM"ATL_CMD_CRLF,       NULL,               "%15[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_id)),  
  ATL_ITEM("AT+GMR"ATL_CMD_CRLF,       NULL,      "Revision:%29[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_rev)),                
  ATL_ITEM("AT+CCLK?"ATL_CMD_CRLF,     NULL,      "+CCLK: \"%21[^\"]\"", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_clock)),          
  ATL_ITEM("AT+CCID"ATL_CMD_CRLF,   "+CCLK",               "%21[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_iccid)),             
  ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,  "+COPS", "+COPS: 0, 0,\"%49[^\"]\"", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_operator)),            
  ATL_ITEM("AT+CSQ"ATL_CMD_CRLF,     "+CSQ",                 "+CSQ: %d", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_rssi)),             
  ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,    NULL,                       NULL, 2, 600, 0, 1, NULL, NULL),                
  ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,     NULL,                       NULL, 2, 600, 0, 0, atl_mdl_general_ceng_cb, NULL),                         
};
atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_rtd_t), ctx);
```

### AT Command Parameters

Let's examine each AT command parameter separately:

- **ATL_ITEM** - macro for filling the structure, use to add AT command to a group.
- **[REQ]** - the request itself, consists of AT command concatenated with ATL_CMD_CRLF, in this case it's a string literal, but can also be a character array*.
- **[PREFIX]** - string literal to be searched in the response. Can be omitted.
- **[FORMAT]** - response parsing format for SSCANF, used with VA_ARGS, obtained data will be collected according to this format, placed in arguments and passed to the group callback. Can be omitted.
- **[RPT]** - number of retries in case of error (0...15).
- **[WAIT]** - response wait timer in 10ms units (0...4095).
- **[STEPERROR]** - in case of command error, we can jump several steps forward or backward within the group, or do nothing (-31...31).
- **[STEPOK]** - same as for error, but here in case of success we can jump to a specific AT command (-31...31), by default we step +1.
- **[CB]** - callback for AT command, called based on AT command execution result. Format data is also passed to it. Can be omitted.
- **[VA_ARG]** - format arguments in the form of ATL_ARG, where we specify which structure and which field will be used to save data from the format. Can be omitted. Maximum 6.

### atl_entity_enqueue Function Parameters

```c
bool atl_entity_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, void* const ctx);
```

- **item** - our AT command group.
- **item_amount** - command structure size.
- **cb** - callback for the entire group, called when this group completes execution. Can be omitted.
- **data_size** - size of the data structure that will be dynamically created automatically to save data specified in [FORMAT] and [VA_ARGS] fields, pointer to this structure will be returned in the group callback and memory for it will be cleared after execution. Can be omitted.
- **ctx** - pointer to some user execution context of the group, passed to the execution callback. Essentially some group meta-data declared somewhere. Can be omitted.

### Entire Command Group Callback Parameters:

```c
void (*atl_entity_cb_t)(bool result, void* ctx, void* data);
```

- **result** - execution result.
- **ctx** - the same context we passed during creation.
- **data** - the same structure that was dynamically created since we specified the size, or NULL if we didn't specify anything.

### AT Command Callback Parameters:

```c
(*answ_parce_cb_t)(ringslice_t data_slice, bool result, void* data);
```

- **data_slice** - the library works directly with the ring buffer using slices, here the data slice for this AT command will be returned if it exists, use the ringslice library API to work with this slice.
- **result** - execution result.
- **data** - pointer to the same created data structure for formatting, if we specified this during creation.

### Features

- If you want to get data from the response but didn't pass the structure size for creation or format, an assert will trigger during execution.
- To get useful data, the library itself dynamically creates a data structure if you specified everything correctly, passes it to the execution callback, and then automatically deletes it after completion. So if you need the data for some time, copy them from the callback to your own structure.
- Sometimes you need to create a command at runtime on the stack, not using a static string literal. Since the library works asynchronously, we can tell it to save the command to its memory using the `ATL_CMD_SAVE` macro, for example: `ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF`.
- In the [PREFIX] field, you can specify more complex constructions to check multiple lines at once:
  - Use `|` for OR operations: `"+CREG: 0,1|+CREG: 0,5"`
  - Use `&` for AND operations: `"+IPD&SEND OK"`
- You can omit the request in the command but pass wait to create an artificial delay between commands, or simply parse and format data in the buffer.
- The modules folder and tests folder contain files and implementations of ready-made AT command groups. For complete understanding, you can look there.
- The AT command parser works in such a way that it first searches for command echo, then execution result, and then data. If you only specify a request, it will return an error if there's no response. However, if you also specify prefix search or data, then even if there's no result or echo, but it finds the expected data or prefix, successful execution will be returned. That is, the entire combination of ECHO, RESULT, DATA in the response is not mandatory if you specified a sufficient set for search for the command and at least part is present in the response. Study the examples.
- In the tests folder, there's a make file that runs tests on your machine to check logic independent of the microcontroller. You can run it through the console or debug through VS Code, try adding your own or see how the process of ready-made tests execution works to understand how everything works.
- The AT command callback receives a data slice, this is done so you can write your own data parser in case standard formatting is insufficient. An example of this can be seen in the ready-made atl_mdl_rtd function, where the data structure is dynamically created by the library and in the command callback we manually parse its data in the needed way and put them in this structure.

---

## 4. Creating Logical Chains

Based on AT command groups (described above), you can create custom algorithms and execution chains. API is provided in the `atl_chain.h` file:

- `atl_chain_destroy`
- `atl_chain_start` 
- `atl_chain_stop`
- `atl_chain_reset`
- `atl_chain_run`
- `atl_chain_is_running`
- `atl_chain_get_current_step`
- `atl_chain_get_current_step_name`
- `atl_chain_print_stats`

### Creating Step Functions

To create a chain, each command group must be wrapped in a standard type function:

```c
bool (*function)(atl_entity_cb_t cb, void* param, void* ctx);
```

For example:

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

Where:
- **cb** - callback for the entire group
- **param** - pointer to input parameters for creating AT command. Need to ensure that the provided structure exists all the time while this command group is executing asynchronously
- **ctx** - context that will be passed to the callback

Such functions can be used in chains or separately. Sets of ready-made functions are presented in the `modules` folder, you can use them and add your own new ones.

### Creating a Chain

Let's create a chain example based on the modules presented in the library:

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
    ATL_CHAIN("MODEM RESET",  "NEXT", "MODEM RESET", atl_mdl_modem_reset, entity_cb, param, ctx, 3),
};

atl_chain_t* chain = atl_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t));
atl_chain_start(chain);
while(atl_chain_is_running(chain))
{
  bool res = atl_chain_run(chain);
  if(atl_chain_is_running(chain))atl_chain_destroy(chain);
}
```

This chain one-time configures the SIMCom modem, context and connection, and then infinitely starts collecting and sending data to the server with a period of 60s. In case of an error, disconnection or deinitialization occurs with an attempt to reinitialize the server.

### Chain Parameters

Let's examine what can be used in the chain and what parameters to pass there:

**ATL_CHAIN** - main macro for adding an execution step, contains:
- **[Name]** - Step name.
- **[Success target]** - Step name to go to in case of success. Can specify NULL or "NEXT" for +1 step or "PREV" for -1 step, or specific step name.
- **[Error target]** - same as for success but in case of error. 
- **[Func]** - the function that will be called to execute the step. 
- **[Cb]** - Step callback 
- **[Param]** - the parameters that will be passed to the specified function when it starts executing
- **[Ctx]** - the callback that will be passed to the function.
- **[Retries]** - number of retries in case of error 

**ATL_CHAIN_COND** - macro for creating and checking a condition, contains:
- **[Name]** - Step name.
- **[True target]** - Step name to go to in case of success. Can specify NULL or "NEXT" for +1 step or "PREV" for -1 step, or specific step name.
- **[False target]** - same as for success but in case of error.
- **[Cond func]** - function for checking the condition. Must be of type bool (*condition)(void);

**ATL_CHAIN_LOOP_START** - macro for specifying the start of a loop, contains:
- **[Iterations]** - Number of loop iterations. 0 - Infinite.

**ATL_CHAIN_LOOP_END** - macro for specifying the end of a loop.

**ATL_CHAIN_DELAY** - macro for creating an artificial delay, contains:
- **[ms]** - delay time in ms.

### atl_entity_enqueue Function Parameters

```c
atl_chain_t* atl_chain_create(const char* const name, const chain_step_t* const steps, const uint32_t step_count);
```

- **name** - Chain name.
- **steps** - The chain structure itself.
- **step_count** - Number of steps in the chain

### Features

- As can be seen from the example, nested loops can be created. The library uses a dynamic stack to store data. However, it's necessary to ensure that there are no transitions from outside into the loop and from the loop to outside without executing LOOP_START or LOOP_END. The library tracks such situations and calculates the balance. In case of non-zero balance, the chain will simply be terminated.