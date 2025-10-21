# ATL Documentation

## Table of Contents
1. [Features](#features)
2. [Setup and Prerequisites](#setup-and-prerequisites)
3. [Sending a Single Command](#sending-a-single-command)
4. [Sending Command Groups](#sending-command-groups)
5. [Creating Logical Chains](#creating-logical-chains)
6. [URC Handling](#urc-handling)
7. [Advanced Features](#advanced-features)

## 1. Features

ATL (AT Command Library) provides a comprehensive solution for working with cellular modems:

- **Command Management**: Methods for sending AT commands in varying quantities and constructing complex logical chains
- **Memory Management**: Custom TLSF dynamic memory allocator with fragmentation control, alignment, and static buffer allocation
- **Error Handling**: Design by Contract (DBC) for robust error checking
- **Thread Safety**: Critical section protection for multithreaded applications
- **URC Support**: Unsolicited Result Code handling for asynchronous events

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
    your_printf_function,    // Custom printf for debugging
    your_uart_write_function, // UART write function  
    rx_ring_buffer,          // RX ring buffer
    sizeof(rx_ring_buffer),  // Buffer size
    &ring_tail,              // Tail pointer
    &ring_head               // Head pointer
);
```

### Critical Section Configuration

Implement critical section handlers to protect against concurrent access:

```c
// Define your critical section handlers
__attribute__((weak)) void atl_crit_enter(void) 
{
    /* Disable interrupts if needed */
    __disable_irq();
}

__attribute__((weak)) void atl_crit_exit(void)  
{
    /* Enable interrupts if needed */
    __enable_irq();
}
```

### Runtime Processing

Call the processing function every 10ms in your system timer:

```c
void timer_10ms_handler(void) {
    atl_core_proc();
}
```

### Configuration Options

Modify `atl_core.h` to customize ATL behavior:

```c
/*******************************************************************************
 * Config
 ******************************************************************************/
#define ATL_MAX_ITEMS_PER_ENTITY 50    // Maximum commands per group
#define ATL_ENTITY_QUEUE_SIZE    10    // Command queue size
#define ATL_URC_QUEUE_SIZE       10    // URC handler queue size
#define ATL_MEMORY_POOL_SIZE     2048  // TLSF memory pool size
#define ATL_DEBUG_ENABLED        1     // Enable debug output
#define ATL_FREQUENCY            100   // Hz (fixed at 1 per 10ms)
```

## 3. Sending a Single Command

### Basic Command

Send a simple AT command without expecting a response:

```c
bool atl_mdl_modem_reset(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
      ATL_ITEM("AT+CFUN=1,1" ATL_CMD_CRLF, NULL, NULL, 2, 150, 0, 0, NULL, NULL),
  };
  
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}
```

### Command with Response Parsing

Send a command and parse the response:

```c

bool atl_mdl_rtd(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM(ATL_CMD_SAVE "AT+COPS?" ATL_CMD_CRLF, "+COPS", "+COPS: 0, 0,\"%49[^\"]\"", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_operator)),            
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_rtd_t), ctx)) return false; 
  return true;
}
```

### Command Parameters Explained

- **ATL_CMD_SAVE**: Saves command to dynamic memory (required for non-literal strings)
- **Response Prefix**: String to search for in response (`+COPS`)
  - Use `|` for OR operations: `"+CREG: 0,1|+CREG: 0,5"`
  - Use `&` for AND operations: `"+IPD&SEND OK"`
- **Format String**: SSCANF-style format for parsing responses
- **ATL_ARG**: Maps parsed data to structure fields

## 4. Sending Command Groups

Execute sequences of AT commands with conditional logic:

```c
bool atl_mdl_gprs_socket_config(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPMODE?"ATL_CMD_CRLF,                 "+CIPMODE: 0", NULL, 1, 150, 0, 2, NULL, NULL),
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
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}
```

## 5. Creating Logical Chains

Combine command groups into complex execution workflows with error handling and loops:

### Chain Definition

```c
atl_chain_t* atl_mld_tcp_server_create(atl_mdl_tcp_server_t* tcp)
{
    chain_step_t server_steps[] = {
        // Initialization sequence
        ATL_CHAIN("MODEM INIT",     "GPRS INIT",      "MODEM RESET", atl_mdl_modem_init, NULL, NULL, 3),
        ATL_CHAIN("GPRS INIT",      "SOCKET CONFIG",  "MODEM RESET", atl_mdl_gprs_init, NULL, NULL, 3),
        ATL_CHAIN("SOCKET CONFIG",  "SOCKET CONNECT", "GPRS DEINIT", atl_mdl_gprs_socket_config, NULL, NULL, 3),
        ATL_CHAIN("SOCKET CONNECT", "MODEM RTD",      "GPRS DEINIT", atl_mdl_gprs_socket_connect, NULL, NULL, 3),

        // Main data loop
        ATL_CHAIN_LOOP_START(0), 
        ATL_CHAIN("MODEM RTD",          "SOCKET SEND RECEIVE", "SOCKET DISCONNECT", atl_mdl_rtd, NULL, NULL, 3),
        ATL_CHAIN("SOCKET SEND RECEIVE", "MODEM RTD",          "SOCKET DISCONNECT", atl_mdl_gprs_socket_send, NULL, NULL, 3),
        ATL_CHAIN_LOOP_END,

        // Error recovery
        ATL_CHAIN("SOCKET DISCONNECT", "SOCKET CONFIG", "MODEM RESET", atl_mdl_gprs_socket_disconnect, NULL, NULL, 3),
        ATL_CHAIN("GPRS DEINIT",       "GPRS INIT",     "MODEM RESET", atl_mdl_gprs_deinit, NULL, NULL, 3),
        ATL_CHAIN("MODEM RESET",       "GPRS INIT",     "MODEM RESET", atl_mdl_modem_reset, NULL, NULL, 3),
    };
    
    atl_chain_t *chain = atl_chain_create("ServerTCP", server_steps, 
                                         sizeof(server_steps)/sizeof(server_steps[0]), 
                                         tcp);
    return chain;
}
```

### Chain Execution

```c
// Create and start the chain
atl_chain_t *chain = atl_chain_create("Data Transmission", steps, sizeof(steps)/sizeof(steps[0]),NULL);                                
atl_chain_start(chain);
atl_chain_run(chain);
```
