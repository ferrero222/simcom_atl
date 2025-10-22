# ATL Documentation / Документация ATL

## Содержание
1. [Возможности](#возможности)
2. [Настройка и предварительные требования](#настройка-и-предварительные-требования)
3. [АТ команды](#ат-команды)
4. [Создание логических цепочек](#создание-логических-цепочек)

## 1. Возможности

ATL (AT Command Library) предоставляет комплексное решение для работы с сотовыми модемами:

- **Асинхронность**: Все процессы библиотеки выполняются без единого блокирования системы
- **Управление командами**: АПИ для отправки групп AT-команд и построения сложных логических цепочек
- **Управление памятью**: Пользовательский динамический аллокатор TLSF с контролем фрагментации, выравниванием и статическим выделением буфера
- **Обработка ошибок**: Design by Contract (DBC) для надежной проверки ошибок
- **Потокобезопасность**: Защита критических секций для многопоточных приложений
- **Поддержка URC**: Обработка URC для асинхронных событий

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

### Конфигурация критических секций

Реализуйте обработчики критических секций для защиты от параллельного доступа:

```c
// Определите ваши обработчики критических секций
__attribute__((weak)) void atl_crit_enter(void) 
{
    /* Отключите прерывания если нужно */
    __disable_irq();
}

__attribute__((weak)) void atl_crit_exit(void)  
{
    /* Включите прерывания если нужно */
    __enable_irq();
}
```

### Обработка во время выполнения

Вызывайте функцию обработки каждые 10ms в системном таймере:

```c
void timer_10ms_handler(void) {
    atl_core_proc();
}
```

### Параметры конфигурации

Измените `atl_core.h` для настройки поведения ATL:

```c
/*******************************************************************************
 * Конфигурация
 ******************************************************************************/
#define ATL_MAX_ITEMS_PER_ENTITY 50    // Максимум команд на группу
#define ATL_ENTITY_QUEUE_SIZE    10    // Размер очереди команд
#define ATL_URC_QUEUE_SIZE       10    // Размер очереди обработчиков URC
#define ATL_MEMORY_POOL_SIZE     2048  // Размер пула памяти TLSF
#define ATL_DEBUG_ENABLED        1     // Включить вывод отладки
```

## 3. АТ команды

В файле `atl_core.h` представлено АПИ для работы с АТ командами и самим ядром библиотеки, содержащее:

- `atl_enqueue`
- `atl_dequeue` 
- `atl_urc_enqueue`
- `atl_urc_dequeue`
- `atl_core_proc`
- `atl_get_cur_time`
- `atl_get_init`

Подробнее о функциях и их параметрах читайте в самом файле. Разберем некоторые примеры использования.

### Пример 1

```c
atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
{
  ATL_ITEM(ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF,                 "+CIPMODE: 0", NULL, 1, 150, 0, 2, NULL, NULL),
  ATL_ITEM("AT+CIPMODE=0"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIPMUX?"ATL_CMD_CRLF,                   "+CIPMUX: 0", NULL, 1, 150, 0, 2, NULL, NULL),
  ATL_ITEM("AT+CIPMUX=0"ATL_CMD_CRLF,                          NULL, NULL, 2, 500, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,            "STATE: IP START", NULL, 1, 250, 0, 2, NULL, NULL),
  ATL_ITEM("AT+CSTT=\"\",\"\",\"\""ATL_CMD_CRLF,               NULL, NULL, 2, 150, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,          "STATE: IP GPRSACT", NULL, 1, 250, 0, 2, NULL, NULL),
  ATL_ITEM("AT+CIICR"ATL_CMD_CRLF,                             NULL, NULL, 4, 800, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,          "STATE: IP GPRSACT", NULL, 1, 250, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIFSR"ATL_CMD_CRLF,                             NULL, NULL, 1, 150, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIPHEAD?"ATL_CMD_CRLF,                 "+CIPHEAD: 1", NULL, 1, 150, 0, 2, NULL, NULL),
  ATL_ITEM("AT+CIPHEAD=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIPSRIP?"ATL_CMD_CRLF,                 "+CIPSRIP: 1", NULL, 1, 150, 0, 2, NULL, NULL),
  ATL_ITEM("AT+CIPSRIP=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIPSHOWTP?"ATL_CMD_CRLF,             "+CIPSHOWTP: 1", NULL, 1, 150, 0, 1, NULL, NULL),
  ATL_ITEM("AT+CIPSHOWTP=1"ATL_CMD_CRLF,                       NULL, NULL, 2, 150, 0, 0, NULL, NULL),
};
atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx);
```

### Пример 2

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
if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_rtd_t), ctx)) return false;
```

### Параметры АТ команды

Итак, давайте рассмотрим отдельно каждый указываемый параметр АТ команды:

- **ATL_ITEM** - макрос для заполнения структуры, используйте для добавления АТ команды в группу
- **[REQ]** - сам запрос, состоит из конкатенации АТ команды и ATL_CMD_CRLF, в данном случае является строковым литералом, но может быть и массивом символов
- **[PREFIX]** - строковый литерал который будет искаться в ответе. Можно не указывать
- **[FORMAT]** - формат парсинга ответа для SSCANF, используется вместе с VA_ARGS, полученные данные будут собраны в соответствии с этим форматом, положены в аргументы и переданы в коллбек группы. Можно не указывать
- **[RPT]** - количество повторов в случае возникновения ошибки (0...15)
- **[WAIT]** - таймер ожидания ответа в 10мс (0...4095)  
- **[STEPERROR]** - в случае возникновения ошибки у команды мы можем перескочить на несколько шагов вперед или назад внутри группы, либо ничего не делать (-31...31)
- **[STEPOK]** - как и в случае ошибки, но тут в случае успеха можем прошагать на какую-либо конкретную АТ команду (-31...31)
- **[CB]** - коллбек на АТ команду, будет вызван по результату выполнения АТ команды. Можно не указывать
- **[VA_ARG]** - аргументы для формата в виде ATL_ARG, где мы указываем какая структура и какое поле будет использоваться чтобы сохранить данные из формата. Можно не указывать

### Параметры функции atl_enqueue

```c
bool atl_enqueue(const atl_item_t* const item, const uint8_t item_amount, 
                const atl_entity_cb_t cb, uint16_t data_size, const void* const ctx)
```

- **item** - собственно наши АТ команды
- **item_amount** - размер структуры команд  
- **cb** - коллбек на всю группу, который будет вызван по исполнению этой группы. Можем не указывать
- **data_size** - размер структуры данных которая будет создана динамически автоматически для сохранения данных указанных в полях [FORMAT] и [VA_ARGS], указатель на эту структуру вернется в коллбек на группу а после исполнения память для него будет очищена. Может не указываться
- **ctx** - указатель на некий пользовательский контекст выполнения группы, который будет передан в коллбек выполнения. По сути некие мета-данные которые где-то объявлены. Можно не указывать

### Параметры коллбеков

**Коллбек на АТ команду:**
```c
(*answ_parce_cb_t)(ringslice_t data_slice, bool result, void* data)
```
- **data_slice** - библиотека работает с кольцевым буфером напрямую используя срезы, здесь вернется срез данных для этой АТ команды, если он есть, используйте АПИ библиотеки ringslice чтобы работать с этим срезом
- **result** - результат выполнения
- **data** - указатель на ту самую созданную структуру данных для форматирования, если мы указали это при создании

**Коллбек на всю группу команд:**
```c
void (*atl_entity_cb_t)(bool result, void* ctx, void* data)
```
- **result** - результат
- **ctx** - тот самый контекст который мы передавали при создании
- **data** - та самая структура которая была создана динамически так как мы указывали размер, либо NULL если мы ничего не указывали

### Особенности

- Если хотите получить данные из ответа но не передали размер структуры для создания или формат или т.п. то в процессе выполнения сработает ассерт
- Для получения полезных данных, библиотека сама создает динамически структуру данных если вы все правильно указали, передает ее в коллбек выполнения, а затем сама также ее удаляет после завершения
- Иногда нужно создать команду в ран-тайме а не использовать строковый литерал. В таком случае мы можем указать библиотеке чтобы она сохранила команду в память используя макрос `ATL_CMD_SAVE`, например: `ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF`
- В поле [PREFIX] можно указывать более сложные конструкции для проверки сразу нескольких строк:
  - Используйте `|` для операций ИЛИ: `"+CREG: 0,1|+CREG: 0,5"`
  - Используйте `&` для операций И: `"+IPD&SEND OK"`
- Можно не указывать запрос в команде но передать ожидание для создания искусственной задержки между командами

## 4. Создание логических цепочек

На основе групп АТ команд можно создавать собственные алгоритмы и цепочки выполнения. Предоставляется АПИ в файле `atl_chain.h`:

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
bool (*function)(atl_entity_cb_t cb, void* param, void* ctx)
```

Например:

```c
bool atl_mdl_gprs_socket_connect(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
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
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}
```

Где:
- **cb** - коллбек на всю группу
- **param** - указатель на входящие параметры для создания АТ команды. Нужно следить чтобы предоставляемая структура существовала все время пока выполняется эта группа команд
- **ctx** - контекст который будет передан в коллбек

Такие функции можно будет использовать в цепочках или отдельно. Наборы уже готовых функций представлены в папке `modules`, можно их использовать и добавлять новые свои.

### Создание цепочки

Давайте создадим пример цепочки на основании представленных в библиотеке модулей:

```c
chain_step_t tcp[] = {
    // Последовательность инициализации
    ATL_CHAIN("MODEM INIT",     "GPRS INIT",      "MODEM RESET", atl_mdl_modem_init, NULL, NULL, 3),
    ATL_CHAIN("GPRS INIT",      "SOCKET CONFIG",  "MODEM RESET", atl_mdl_gprs_init, NULL, NULL, 3),
    ATL_CHAIN("SOCKET CONFIG",  "SOCKET CONNECT", "GPRS DEINIT", atl_mdl_gprs_socket_config, NULL, NULL, 3),
    ATL_CHAIN("SOCKET CONNECT", "MODEM RTD",      "GPRS DEINIT", atl_mdl_gprs_socket_connect, NULL, NULL, 3),

    // Главный цикл данных
    ATL_CHAIN_LOOP_START(0), 
    ATL_CHAIN("MODEM RTD",          "SOCKET SEND RECEIVE", "SOCKET DISCONNECT", atl_mdl_rtd, NULL, NULL, 3),
    ATL_CHAIN("SOCKET SEND RECEIVE", "MODEM RTD",          "SOCKET DISCONNECT", atl_mdl_gprs_socket_send, NULL, NULL, 3),
    ATL_CHAIN_DELAY(6000),
    ATL_CHAIN_LOOP_END,

    // Восстановление после ошибок
    ATL_CHAIN("SOCKET DISCONNECT", "SOCKET CONFIG", "MODEM RESET", atl_mdl_gprs_socket_disconnect, NULL, NULL, 3),
    ATL_CHAIN("GPRS DEINIT",       "GPRS INIT",     "MODEM RESET", atl_mdl_gprs_deinit, NULL, NULL, 3),
    ATL_CHAIN("MODEM RESET",       "GPRS INIT",     "MODEM RESET", atl_mdl_modem_reset, NULL, NULL, 3),
};

atl_chain_t *chain = atl_chain_create("ServerTCP", server_steps, sizeof(server_steps)/sizeof(server_steps[0]), tcp);
```

Данная цепочка единоразово настраивает модем, контекст и соединение а затем бесконечно начинает собирать и слать данные на сервер с периодом в 60с, в случае возникновения ошибки происходит отключение или деинициализация с попыткой повторной инициализации сервера.

Здесь вместо NULL нужно указать коллбеки и указатели на входящие параметры для функций исполнения если они нужны. Нужно учитывать чтобы передаваемые параметры существовали во время выполнения этой функции.