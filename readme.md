# ATL Documentation

The library offers a constructor and methods for easily creating any single or grouped AT commands, automatically parsing their responses, and creating logical execution chains of grouped commands to implement various scenarios for working with any interacting device.

## 1. Features and Properties

- **Asynchrony**: Non-blocking and non-waiting system.
- **Command Management**: API for creating and sending groups of AT commands, building complex logical chains.
- **Flexibility**: Ability to create any type of command, with various parameter combinations.
- **Memory**: Use of a custom dynamic memory allocator with alignment support and O1heap fragmentation protection. [GitHub](https://github.com/pavel-kirienko/o1heap)
- **Error Handling**: Design by Contract (DBC) for reliable error checking. [GitHub](https://github.com/QuantumLeaps/DBC-for-embedded-C)
- **Thread Safety**: Critical section protection for multithreaded applications.
- **URC Support**: URC handling for asynchronous events.
- **Testing**: Embedded tests from QLP. [GitHub](https://github.com/QuantumLeaps/Embedded-Test)
- **Slices**: Use of ring slices instead of direct buffer copying. [GitHub](https://github.com/ferrero222/ringslice/tree/dev)
- **Modularity**: A set of provided, extensible ready-made modules of grouped AT commands for specific external devices.
- **Extensibility**: Ability to adapt and integrate new parsers to support any command format.
- **Contextuality**: Ability to create multiple library contexts simultaneously for parallel work with several devices.

Tested with: SIM868....

## 2. Setup and Prerequisites

### Basic Configuration

You need to define your ring buffer and pointers:

```c
typedef struct {
  uint8_t *buffer;      
  uint16_t size;   
  uint16_t head;       
  uint16_t tail;       
  uint16_t count;     
} atl_ring_buffer_t;
```
Next, define the context globally:

```c
atl_context_t ctx = {0};
```

Initialize this context:

```c
atl_init(
    &ctx,                     // Context
    your_printf_function,     // Custom printf for debugging
    your_write_function,      // Write function to the interface
    your_ring_buffer_struct,  // Ring buffer structure
);
```

### Configuration Parameters

In the atl_port.c file, you need to describe the critical section handlers for protection against parallel access, as well as an exception handler:

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

In the atl_core.h file, you can configure the parameters:

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

The main processing function must be called for each context, every 10ms in the system timer:

```c
void timer_10ms_handler(void) {
  atl_core_proc(&ctx1);
  atl_core_proc(&ctx2);
  ...
}
```

## 3. AT Commands

The `atl_core.h` file presents the API for working with AT commands and the library core itself, containing:

- `atl_entity_enqueue`
- `atl_entity_dequeue` 
- `atl_urc_enqueue`
- `atl_urc_dequeue`
- `atl_core_proc`
- `atl_get_init`
- `atl_get_cur_time`
- `atl_malloc`
- `atl_free`

For more details about the functions and their parameters, see the file itself. Let's look at some examples of creating and using AT commands.

### Example 1, Grouped Commands Without Response Formatting

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

### Example 2, Grouped Commands With Response Formatting

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

### AT Command Parameters

Let's look at each specified parameter of the AT command separately and analyze what happens:

- **ATL_ITEM** - macro for filling the structure, use it to add an AT command to the group.
- **[REQ]** - the request itself, consists of the concatenation of the AT command and ATL_CMD_CRLF, in this case it is a string literal, but it can also be a character array.
- **[PREFIX]** - a string literal that will be searched for in the response. Can be omitted.
- **[PARCE_TYPE]** - the type of parser used, read below.
- **[FORMAT]** - response parsing format for SSCANF, used together with VA_ARGS, the obtained data will be collected according to this format, placed in the arguments and passed to the group callback. Can be omitted.
- **[RPT]** - number of repetitions in case of an error.
- **[WAIT]** - response wait timer in 10ms.
- **[STEPERROR]** - in case of a command error, we can skip several steps forward or backward within the group, or do nothing. 0 terminates the entire group.
- **[STEPOK]** - as in the case of an error, but here in case of success we can step to some specific AT command. 0 terminates the entire group.
- **[CB]** - callback for the AT command, will be called based on the result of the AT command execution. The data obtained by the format is also passed to it. Can be omitted.
- **[VA_ARG]** - arguments for the format in the form of ATL_ARG, where we specify which structure and which field will be used to save the data from the format. Can be omitted. Maximum 6.

### Parameters of the atl_entity_enqueue Function

```c
bool atl_entity_enqueue(atl_context_t* const ctx, const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, void* const meta);
```

- **ctx** - global context for which the group is created.
- **item** - group of AT commands.
- **item_amount** - size of the command structure.
- **cb** - callback for the entire group, which will be called upon execution of this group. Can be omitted.
- **data_size** - size of the data structure that will be created dynamically, automatically, to save the data specified in the [FORMAT] and [VA_ARGS] fields, a pointer to this structure will be returned in the group callback and after execution the memory for it will be cleared. Can be omitted (0).
- **meta** - pointer to some additional metadata of the group. Passed directly to the execution callback and not processed by the library in any way. Can be omitted.

### Parameters of the Callback for the Entire Command Group:

```c
void (*atl_entity_cb_t)(bool result, void* meta, void* data);
```

- **result** - result.
- **meta** - the same metadata that we passed during creation.
- **data** - the same structure that was created dynamically because we specified the size, or NULL if we didn't specify anything.

### Parameters of the Callback for the AT Command:

```c
(*answ_parce_cb_t)(ringslice_t data_slice, bool result, void* data);
```

- **data_slice** - the library works directly with the ring buffer using slices, here a data slice for this AT command will be returned, if it exists, use the ringslice library API to work with this slice.
- **result** - execution result.
- **data** - pointer to the same created data structure for formatting, if we specified this during creation.

### Features

- If it is necessary to get data from the response, but the structure size for creation or the format of the received data was not passed, then an assert will be triggered during execution.
- To obtain useful data from the response and collect them, the library itself dynamically creates a data structure if everything was specified correctly when creating the command group, passes it to the execution callback, and then also deletes it after completion. Therefore, if the data is needed for some time, it is necessary to copy them from the callback to your static structure.
- Sometimes it is not known in advance what the command will consist of and it needs to be created in real time on the stack, or in some temporary buffer, rather than using a static string literal. Since the library works asynchronously, it is necessary to ensure that this temporary buffer exists at the moment the command is executed. To avoid doing this every time, you can simply specify that the command should be saved to the library memory using the `ATL_CMD_SAVE` macro, for example: `ATL_CMD_SAVE"AT+CIPMODE?"ATL_CMD_CRLF`. The same can be applied to the prefix.
- In the [PREFIX] field, you can specify more complex constructions to check multiple lines and prefixes at once:
  - Use `|` for OR operations: `"+CREG: 0,1|+CREG: 0,5"`
  - Use `&` for AND operations: `"+IPD&SEND OK"`
- Also, in the [PREFIX] field, you can specify the macro-literal `ATL_CMD_FORCE` which will indicate that this command or data should simply be sent, without parsing and waiting for a response. The other fields except [STEPOK] will not be used in any way and their content can be anything.
- The modules folder contains some files and implementations of already ready groups of AT commands.
- The tests folder has a make file that runs host tests to check the logic independently of the microcontroller. You can run it through the console or debug it through VS code, try to add your own or see how the process of executing ready-made tests works to understand how everything works.
- The callback for the AT command receives a slice of the data, this is done so that you can write your own data parser in case the standard formatting is not enough, an example of this can be seen in the ready-made function atl_mdl_rtd, where the data structure is dynamically created by the library and in the command callback we manually parse its data in the necessary way and put them into this structure.
- In the case of processed data, the library itself moves the tail and counter of your ring buffer.
- The examples folder contains usage examples.

### Parsers
`ATL_PARCE_SIMCOM`<br>
 Parser for commands of the simcom format. This parser works with echo enabled. Usually, the response to commands comes sequentially in the form: ECHO → RESULT → DATA. The echo must always come first. The parser works in the same way, first looking for the command echo, then the execution result, and then the useful data, i.e., it splits the response into three fields: [REQ] [RES] [DATA]. Their combination, presence, and order can vary from command to command, each differently. The internal parser handles only a few combinations and possible cases, which should be sufficient. Let's consider the combinations of parameters when creating a command and what responses they support:

| Specified Command Parameters | Required Response Fields for Such Parameters |
|-------------------|----------------------------|
| [REQ] | [REQ] [RES] with OK<br>[REQ] [RES] [DATA] with OK |
| [REQ] [PREFIX] <br> [REQ] [PREFIX] [FORMAT] [ARG]| [REQ] [RES] [DATA] + prefix and format check (if specified)<br>[REQ] [DATA] + prefix and format check (if specified)<br>[DATA] + prefix and format check (if specified) |
| [PREFIX] | [DATA] + prefix check |
| [PREFIX] [FORMAT] [ARG] | [DATA] + prefix and format check |

The [DATA] field is usually framed by CRLF characters — the parser finds the boundaries by them, removes them, and works with "clean" data. If [DATA] contains multiline data with CRLF, the parser will return only the FIRST line.
For complex cases, it is recommended to write a custom parser via callback (example: atl_mdl_rtd function). Use the table above to correctly create a command, knowing in advance the type of response and what fields it contains.


`ATL_PARCE_RAW`<br>
 Parser for data transmission format commands. Parses raw data simply checking for a match with the [PREFIX] field if specified. Also, if the [FORMAT][ARG] field is specified, the parser will try to format and obtain useful data from the very beginning of this data buffer. The data is not prepared in any way and is processed as is, which must be taken into account. The [REQ] field can be omitted.

## 4. Creating Logical Chains

Based on groups of AT commands (as described above), you can create your own algorithms and execution chains. The API is provided in the `atl_chain.h` file:

- `atl_chain_destroy`
- `atl_chain_start` 
- `atl_chain_stop`
- `atl_chain_reset`
- `atl_chain_run`
- `atl_chain_is_running`
- `atl_chain_get_current_step`
- `atl_chain_get_current_step_name`

### Creating Step Functions

To create a chain, each command group must be wrapped in a function of the standard type:

```c
bool (*function)(atl_entity_cb_t cb, void* param, void* ctx);
```
The function must, based on the result, execute the callback passed into it.

For example:

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

Where:
- **ctx** - global context for which the group is created.
- **cb** - callback for the entire group.
- **param** - pointer to input parameters for creating the AT command. It is necessary to ensure that the provided structure exists all the time while this command group is being executed asynchronously.
- **meta** - pointer to some additional metadata of the group. Passed directly to the execution callback.

Such functions can be used in chains or separately. Sets of already ready functions are presented in the `modules` folder, you can use them and add your own new ones.

### Creating a Chain

Let's create an example chain based on the modules presented in the library:

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

This chain one-time configures the simcom modem, context and connection, gets real-time data, checks it, and then 10 times starts collecting and sending this data to the server with a period of 10 sec with preliminary authorization, in case of an error, disconnection or deinitialization occurs with an attempt to re-initialize and reconnect or complete restart.

### Chain Parameters

So, let's look at what can be used in the chain and what parameters to pass there:

**ATL_CHAIN** - the main macro for adding an execution step, contains:
- **[Name]** - Step name.
- **[Success target]** - The name of the step to go to in case of success. We can specify NULL or "NEXT" for step +1 or "PREV" for step -1, or a specific step name. "STOP" will end the chain execution.
- **[Error target]** - the same as for success, but in case of error.
- **[Func]** - the very function that will be called to execute the step.
- **[Cb]** - Callback for the step.
- **[Param]** - the very parameters that will be passed to the specified function when it starts executing.
- **[meta]** - the very metadata that we passed during creation.
- **[Retries]** - number of retries in case of error.

**ATL_CHAIN_EXEC** - macro for creating and executing some action, you can execute or check something. Contains:
- **[Name]** - Step name.
- **[True target]** - The name of the step to go to in case of success. We can specify NULL or "NEXT" for step +1 or "PREV" for step -1, or a specific step name. "STOP" will end the chain execution.
- **[False target]** - the same as for success, but in case of error.
- **[Exec func]** - function to execute. Must be of type bool (*exec)(void); Its execution result affects the next transition.

**ATL_CHAIN_LOOP_START** - macro for specifying the start of a loop further, contains:
- **[Iterations]** - Number of loop iterations. 0 - Infinite.

**ATL_CHAIN_LOOP_END** - macro for specifying the end of the loop.

**ATL_CHAIN_DELAY** - macro for creating an artificial delay, contains:
- **[ms]** - delay time in ms.

### Parameters of the atl_entity_enqueue Function

```c
atl_chain_t* atl_chain_create(const char* const name, const chain_step_t* const steps, const uint32_t step_count);
```

- **name** - Chain name.
- **steps** - The chain structure itself.
- **step_count** - Number of steps in the chain.

### Features

- As can be seen from the example, nested loops can be created, the library implements and uses its own dynamic stack to work with them. There is support and the possibility of transitions through loops, from loops, into loops with nesting and without, moving both forward and backward, skipping their designating steps, there are no restrictions, however, it should be taken into account that one iteration of the loop occurs only at the moment of executing ATL_CHAIN_LOOP_END for the corresponding loop. Also, it is necessary to ensure the absence of steps with the same names, in which case the transition to such a step will be performed for the first one found in the array.
```
