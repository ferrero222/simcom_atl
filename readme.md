# ATL Library Documentation

## Table of Contents
1. [Setup and Prerequisites](#setup-and-prerequisites)
2. [Sending a Single Command](#sending-a-single-command)
3. [Sending Command Groups](#sending-command-groups)
4. [Creating Logical Chains](#creating-logical-chains)
5. [Nuances and Capabilities](#nuances-and-capabilities)
6. [Complete Example](#complete-example)

## 1. Setup and Prerequisites

### Initialization
```c
// Define your ring buffer and pointers
uint8_t rx_ring_buffer[1024];
uint16_t ring_head = 0;
uint16_t ring_tail = 0;

// Initialize ATL library
atl_init(
    your_printf_function,    // Custom printf
    your_uart_write_function, // UART write function  
    rx_ring_buffer,          // RX buffer
    sizeof(rx_ring_buffer),  // Buffer size
    &ring_tail,              // Tail pointer
    &ring_head               // Head pointer
);
```

### Required Callbacks
```c
// Call this function every 10ms in your main loop
void your_main_loop(void) {
    atl_core_proc();
}

// Update ring buffer when data arrives
void uart_rx_callback(uint8_t* data, uint16_t len) {
    // Copy data to ring buffer
    // Update ring_head pointer
}
```

## 2. Sending a Single Command

### Basic Command
```c
// Simple AT command with OK/ERROR check
static const atl_item_t single_cmd = {
    "AT",
    NULL,           // No prefix check
    NULL,           // No data parsing
    {500, 3, -1, 1} // 5s timeout, 3 retries
};

// Send command
atl_entity_enqueue(&single_cmd, 1, command_callback, 0, NULL);
```

### Command with Response Parsing
```c
// Data structure for parsed response
typedef struct {
    int rssi;
    int ber;
} signal_data_t;

// Command with response parsing
static const atl_item_t csq_cmd = ATL_ITEM(
    "AT+CSQ",
    "+CSQ:",
    "%d,%d",
    3,      // retries
    100,    // timeout (1s)
    -1,     // error step
    1,      // success step
    NULL,
    ATL_ARG(signal_data_t, rssi),
    ATL_ARG(signal_data_t, ber)
);

// Usage
signal_data_t data;
atl_entity_enqueue(&csq_cmd, 1, csq_callback, sizeof(signal_data_t), &data);
```

## 3. Sending Command Groups

### Sequential Command Group
```c
// Multiple commands executed sequentially
static const atl_item_t init_commands[] = {
    ATL_ITEM("AT", NULL, NULL, 3, 100, -1, 1, NULL),
    ATL_ITEM("ATE0", NULL, NULL, 3, 100, -1, 1, NULL),
    ATL_ITEM("AT+CFUN=1", NULL, NULL, 3, 100, -1, 1, NULL),
    ATL_ITEM("AT+CREG?", "+CREG:", "%*d,%d", 3, 100, -1, 1, NULL,
             ATL_ARG(network_data_t, reg_status))
};

// Enqueue group
atl_entity_enqueue(init_commands, 4, init_complete_callback, 
                  sizeof(network_data_t), &context);
```

### Conditional Command Flow
```c
// Commands with different success/error paths
static const atl_item_t conditional_cmds[] = {
    // Step 1: Check network registration
    ATL_ITEM("AT+CREG?", "+CREG:", "%*d,%d", 3, 100, 2, 1, NULL,
             ATL_ARG(context_t, net_status)),
    
    // Step 2: Success path - send data
    ATL_ITEM("AT+CGDATA=1", "CONNECT", NULL, 3, 500, -1, 1, NULL),
    
    // Step 3: Error path - retry registration
    ATL_ITEM("AT+COPS=0", NULL, NULL, 3, 100, -1, 1, NULL)
};
```

## 4. Creating Logical Chains

### Simple Chain
```c
// Define chain steps
chain_step_t steps[] = {
    ATL_CHAIN("Check Signal", "Connect", "Retry", check_signal_function, signal_cb, NULL, 3),
    ATL_CHAIN("Connect", "Send Data", "Disconnect", connect_function, connect_cb, NULL, 2),
    ATL_CHAIN_COND("Check Connection", "Send Data", "Reconnect", check_connection_func),
    ATL_CHAIN_LOOP_START(3),  // Loop 3 times
    ATL_CHAIN("Send Data", NULL, "Disconnect", send_data_function, data_cb, NULL, 1),
    ATL_CHAIN_LOOP_END,
    ATL_CHAIN_DELAY(1000)  // 1 second delay
};

// Create and start chain
atl_chain_t *chain = atl_chain_create("Data Transmission", steps, 
                                     sizeof(steps)/sizeof(steps[0]));
atl_chain_start(chain);
```

### Chain with Error Recovery
```c
chain_step_t recovery_steps[] = {
    ATL_CHAIN("Initialize", "Check Network", "Reset", init_modem, init_cb, NULL, 2),
    ATL_CHAIN_COND("Check Network", "Send SMS", "Register Network", check_network_func),
    ATL_CHAIN("Register Network", "Send SMS", "Reset", register_network, reg_cb, NULL, 3),
    ATL_CHAIN("Send SMS", NULL, "Retry SMS", send_sms_function, sms_cb, NULL, 2),
    ATL_CHAIN("Retry SMS", "Send SMS", "Reset", NULL, NULL, NULL, 1),
    ATL_CHAIN("Reset", "Initialize", NULL, reset_modem, reset_cb, NULL, 1)
};
```

## 5. Nuances and Capabilities

### URC (Unsolicited Result Code) Handling
```c
// Define URC handler
void incoming_sms_handler(ringslice_t data) {
    // Process incoming SMS notification
    ATL_DEBUG("New SMS received!\n");
}

// Register URC
atl_urc_t sms_urc = {
    "+CMTI:",  // URC prefix
    incoming_sms_handler
};
atl_urc_enqueue(&sms_urc);
```

### Memory Management
- **TLSF Allocator**: Efficient memory allocation for dynamic data
- **Automatic Cleanup**: Memory freed when entities complete
- **Pool Size**: Configurable via `ATL_MEMORY_POOL_SIZE`

### Advanced Features

#### Custom Response Processing
```c
void custom_parser(ringslice_t data_slice, bool result, void* user_data) {
    if (result) {
        // Custom parsing logic
        char buffer[64];
        ringslice_copy(&data_slice, buffer, sizeof(buffer));
        ATL_DEBUG("Custom parse: %s\n", buffer);
    }
}

static const atl_item_t custom_cmd = {
    "AT+CUSTOM",
    "+CUSTOM:",
    NULL,
    {100, 1, -1, 1},
    .answ.cb = custom_parser
};
```

#### Step Control
```c
// Control command flow with step modifiers
static const atl_item_t controlled_cmds[] = {
    // On error, jump to step 3 (index 2)
    ATL_ITEM("AT+CMD1", NULL, NULL, 3, 100, 2, 1, NULL),
    
    // On success, skip next command (jump 2 steps)
    ATL_ITEM("AT+CMD2", NULL, NULL, 3, 100, -1, 2, NULL),
    
    // On error, go back to previous command
    ATL_ITEM("AT+CMD3", NULL, NULL, 3, 100, -1, 1, NULL)
};
```
