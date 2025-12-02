/******************************************************************************
 *                              _    ____   ____                              *
 *                   ======    / \  / ___| / ___| ======       (c)03.10.2025  *
 *                   ======   / _ \ \___ \| |     ======           v1.0.0     *
 *                   ======  / ___ \ ___) | |___  ======                      *
 *                   ====== /_/   \_\____/ \____| ======                      *  
 *                                                                            *
 ******************************************************************************/
#ifndef __ASC_CORE_H
#define __ASC_CORE_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h> 
#include <stdbool.h> 
#include <string.h>
#include <assert.h>
#include "o1heap.h"
#include "ringslice.h"

/*******************************************************************************
 * Config
 ******************************************************************************/
#define ASC_MAX_ITEMS_PER_ENTITY   50     //Max amount of AT cmds in one group (if you need to change, check step field sizes also)
  
#define ASC_ENTITY_QUEUE_SIZE      10     //Max amount of groups 
  
#define ASC_URC_QUEUE_SIZE         10     //Amount of handled URC

#define ASC_URC_FREQ_CHECK         10     //Check urc each ASC_URC_FREQ_CHECK*10ms

#define ASC_MEMORY_POOL_SIZE       4096   //Memory pool for custom heap

#ifndef ASC_TEST  
  #define ASC_DEBUG_ENABLED        1      //Recommend to turn on DEBUG logs
#endif

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ASC_CMD_SAVE             "<S>"
#define ASC_CMD_FORCE            "<F>"
#define ASC_CMD_CRLF             "\r\n"
#define ASC_CMD_CR               "\r"
#define ASC_CMD_LF               "\n"
#define ASC_CMD_CTRL_Z           "\x1a"
#define ASC_CMD_OK               ASC_CMD_CRLF"OK"ASC_CMD_CRLF
#define ASC_CMD_ERROR            ASC_CMD_CRLF"ERROR"ASC_CMD_CRLF

#define ASC_ITEM_SIZE            sizeof(asc_item_t)
#define ASC_URC_SIZE             sizeof(asc_urc_queue_t)

#define ASC_ITEM(req_, prefix_, parce_type_, retries_, timeout_, err_step_, ok_step_, cb_, format_, ...) \
{ \
  .req = req_, \
  .parce_type = parce_type_, \
  .answ = \
  { \
    .prefix = prefix_, \
    .format = format_, \
    .ptrs = (void*[]){__VA_ARGS__, ASC_NO_ARG}, \
    .cb = cb_ \
  }, \
  .meta = \
  { \
    .wait = timeout_, \
    .rpt_cnt = retries_, \
    .err_step = err_step_, \
    .ok_step = ok_step_ \
  } \
}

#define ASC_ARG(src, field) ((void*)offsetof(src, field))
#define ASC_NO_ARG           (void*)0xFFFF

#define ASC_CRITICAL_ENTER  _asc_crit_enter();
#define ASC_CRITICAL_EXIT   _asc_crit_exit();

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
typedef void (*answ_parce_cb_t)(ringslice_t data_slice, bool result, void* const data);
typedef void (*asc_urc_cb)(ringslice_t urc_slice);   //urc callback type

typedef uint8_t asc_parce_type_t;
enum
{
  ASC_PARCE_SIMCOM = 1,  //ECHO\r\r\nRES\r\nDATA\r\n
  ASC_PARCE_RAW,         //> RAW DATA
};

typedef struct asc_urc_queue_t{
  char* prefix;  // Char prefix to find the URC
  asc_urc_cb cb; // Callback for this URC
} asc_urc_queue_t;

typedef struct asc_item_t
{
  char* req;  //Sended request, could be string or literal
  asc_parce_type_t parce_type;
  struct{
    char *prefix;        //Prefix to find in answer
    char *format;        //format for parcing answer
    void **ptrs;         //VA ARGS for format, ptr to ptr array
    answ_parce_cb_t cb;  //Callback by the end
  } answ;
  struct {
    uint16_t wait;    // wait time (up to 65535) in 10ms
    uint8_t rpt_cnt;  // repeat counter (up to 255)
    int8_t err_step;  // error step (-127 to 127)  
    int8_t ok_step;   // success step (-127 to 127)
  } meta;
} asc_item_t;

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
typedef void (*asc_printf_t)(const char *string);

typedef uint16_t (*asc_write_t)(uint8_t* buff, //buff where data will be written
                                uint16_t len); //len of data

typedef void (*asc_entity_cb_t)(const bool result,       //result
                                void* const meta,        //passed meta from @asc_entity_t
                                const void* const data); //data ptr from @asc_entity_t

typedef uint8_t asc_proc_states_t;
enum
{
  ASC_STATE_READ = 1,
  ASC_STATE_WRITE,
};

typedef struct {
  uint8_t *buffer;      
  uint16_t size;   
  uint16_t head;       
  uint16_t tail;       
  uint16_t count;     
} asc_ring_buffer_t;

typedef struct asc_init_t{
  asc_printf_t asc_printf;    //custom printf fucntion
  asc_write_t asc_write;      //custom write function
  O1HeapInstance* heap;       //instance for custom heap
  asc_ring_buffer_t* rx_buff; //rx ring buffer 
  bool init;                  //init flag
} asc_init_t; 

typedef struct asc_entity_t{
  asc_item_t*       item;       //list of items
  uint8_t           item_cnt;   //amount of items
  uint8_t           item_id;    //current id of executionable items
  uint16_t          timer;      //timer
  asc_entity_cb_t   cb;         //cb for item
  void*             data;       //usefull data from execution
  void*             meta;       //meta data
  uint16_t          data_size;  //usefull data size
  asc_proc_states_t state;      //state
} asc_entity_t;

typedef struct asc_entity_queue_t{
  asc_entity_t entity[ASC_ENTITY_QUEUE_SIZE]; //entity queue
  uint8_t entity_head;  //entitiy head
  uint8_t entity_tail;  //entity tail
  uint8_t entity_cnt;   //entity counter
} asc_entity_queue_t;

typedef struct asc_context_t {
  asc_entity_queue_t entity_queue; //entity queue
  asc_urc_queue_t urc_queue[ASC_URC_QUEUE_SIZE]; //urc queue
  asc_init_t init_struct; //init struct
  uint8_t mem_pool[ASC_MEMORY_POOL_SIZE] __attribute__((aligned(O1HEAP_ALIGNMENT)));
  uint32_t time;
} asc_context_t;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Init atl lib  
 ** @param  ctx        core context
 ** @param  asc_printf pointer to user func of printf
 ** @param  asc_write  pointer to user func of write to uart
 ** @param  rx_buff    struct to ring buffer
 ** @return none
 ******************************************************************************/
void asc_init(asc_context_t* const ctx, const asc_printf_t asc_printf, const asc_write_t asc_write, asc_ring_buffer_t* rx_buff);

/*******************************************************************************
 ** @brief  DeInit atl lib  
 ** @param  ctx core context
 ** @return none
 ******************************************************************************/
void asc_deinit(asc_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function to append main queue with new group of at cmds
 ** @param  ctx          core context
 ** @param  item         ptr to your group of at cmds.
 ** @param  item_amount  amount  of your at cms in group 
 ** @param  cb           ur callback function for the whole group.
 ** @param  data_size    If you`r expecting some usefull data while execution pass here size of them.
 **                      And pass the ptr of this data to VA ARGS of each item with proper format to
 **                      get this data. If no need pass the 0.
 ** @param  meta         Ptr to some meta data of execution. Will be called in CB. Can be NULL.
 ** @return true: ok false: error while trying to append
 ******************************************************************************/
bool asc_entity_enqueue(asc_context_t* const ctx, const asc_item_t* const item, const uint8_t item_amount, const asc_entity_cb_t cb, uint16_t data_size, void* const meta);

/*******************************************************************************
 ** @brief  Clear first entity from the queue 
 ** @param  ctx core context
 ** @return false: some errors; true: ok
 ******************************************************************************/
bool asc_entity_dequeue(asc_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function to append URC queue
 ** @param  ctx  core context
 ** @param  urc  ptr to your URC.
 ** @return true/false
 ******************************************************************************/
bool asc_urc_enqueue(asc_context_t* const ctx, const asc_urc_queue_t* const urc);

/*******************************************************************************
 ** @brief  Function to delete URC from queue
 ** @param  ctx  core context
 ** @param  prefix  prefix of your URC.
 ** @return true/false
 ******************************************************************************/
bool asc_urc_dequeue(asc_context_t* const ctx, char* prefix);

/*******************************************************************************
 ** @brief  Function to proc ATL core proccesses. 
 ** @param  ctx  core context
 ** @return none
 ******************************************************************************/
void asc_core_proc(asc_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function get time in 10ms. 
 ** @param  ctx  core context
 ** @return time
 ******************************************************************************/
uint32_t asc_get_cur_time(asc_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function get init. 
 ** @param  ctx  core context
 ** @return @asc_init_t
 ******************************************************************************/
asc_init_t asc_get_init(asc_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function to custom malloc. Handle the concurrent state
 ** @param  ctx  core context
 ** @param  size amount to alloc
 ** @return ptr to new memory
 ******************************************************************************/
void* asc_malloc(asc_context_t* const ctx, size_t size);

/*******************************************************************************
 ** @brief  Function to custom free. Handle the concurrent state
 ** @param  ctx  core context
 ** @param  ptr  ptr to delete
 ** @return none
 ******************************************************************************/
void asc_free(asc_context_t* const ctx, void* ptr);

#ifdef ASC_TEST
void _asc_core_proc(asc_context_t* const ctx);
int _asc_cmd_ring_parcer(asc_context_t* const ctx, const asc_entity_t* const entity, const asc_item_t* const item, const ringslice_t rs_me);
void _asc_simcom_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* const req); 
void _asc_simcom_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res);
void _asc_simcom_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data); 
int _asc_simcom_parcer_post_proc(asc_context_t* const ctx, const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, const ringslice_t* const rs_data, const asc_item_t* const item, const asc_entity_t* const entity); 
int _asc_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern); 
int _asc_cmd_sscanf(const ringslice_t* const rs_data, const asc_item_t* const item); 
void _asc_process_urcs(asc_context_t* const ctx, const ringslice_t* me);
asc_entity_queue_t* _asc_get_entity_queue(asc_context_t* const ctx); 
asc_urc_queue_t* _asc_get_urc_queue(asc_context_t* const ctx); 
asc_init_t _asc_get_init(asc_context_t* const ctx);
#endif

#endif //__ASC_CORE_H

