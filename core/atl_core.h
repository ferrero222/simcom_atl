/*******************************************************************************
 *                           ╔══╗╔════╗╔╗──                      (c)03.10.2025 *
 *                           ║╔╗║╚═╗╔═╝║║──                          v1.0.0    *
 *                           ║╚╝║──║║──║║──                                    *
 *                           ║╔╗║──║║──║║──                                    *
 *                           ║║║║──║║──║╚═╗                                    *
 *                           ╚╝╚╝──╚╝──╚══╝                                    *  
 ******************************************************************************/
#ifndef __ATL_CORE_H
#define __ATL_CORE_H

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
#define ATL_MAX_ITEMS_PER_ENTITY   50     //Max amount of AT cmds in one group (if you need to change, check step field sizes also)
  
#define ATL_ENTITY_QUEUE_SIZE      10     //Max amount of groups 
  
#define ATL_URC_QUEUE_SIZE         10     //Amount of handled URC

#define ATL_URC_FREQ_CHECK         10     //Check urc each ATL_URC_FREQ_CHECK*10ms

#define ATL_MEMORY_POOL_SIZE       4096   //Memory pool for custom heap

#ifndef ATL_TEST  
  #define ATL_DEBUG_ENABLED        1      //Recommend to turn on DEBUG logs
#endif

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ATL_CMD_SAVE             "<S>"
#define ATL_CMD_FORCE            "<F>"
#define ATL_CMD_CRLF             "\r\n"
#define ATL_CMD_CR               "\r"
#define ATL_CMD_LF               "\n"
#define ATL_CMD_CTRL_Z           "\x1a"
#define ATL_CMD_OK               ATL_CMD_CRLF"OK"ATL_CMD_CRLF
#define ATL_CMD_ERROR            ATL_CMD_CRLF"ERROR"ATL_CMD_CRLF

#define ATL_ITEM_SIZE            sizeof(atl_item_t)
#define ATL_URC_SIZE             sizeof(atl_urc_queue_t)

#define ATL_ITEM(req_, prefix_, parce_type_, retries_, timeout_, err_step_, ok_step_, cb_, format_, ...) \
{ \
  .req = req_, \
  .parce_type = parce_type_, \
  .answ = \
  { \
    .prefix = prefix_, \
    .format = format_, \
    .ptrs = (void*[]){__VA_ARGS__, ATL_NO_ARG}, \
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

#define ATL_ARG(src, field) ((void*)offsetof(src, field))
#define ATL_NO_ARG           (void*)0xFFFF

#define ATL_CRITICAL_ENTER  _atl_crit_enter();
#define ATL_CRITICAL_EXIT   _atl_crit_exit();

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
typedef void (*answ_parce_cb_t)(ringslice_t data_slice, bool result, void* const data);
typedef void (*atl_urc_cb)(ringslice_t urc_slice);   //urc callback type

typedef uint8_t atl_parce_type_t;
enum
{
  ATL_PARCE_SIMCOM = 1,  //ECHO\r\r\nRES\r\nDATA\r\n
  ATL_PARCE_RAW,         //> RAW DATA
};

typedef struct atl_urc_queue_t{
  char* prefix;  // Char prefix to find the URC
  atl_urc_cb cb; // Callback for this URC
} atl_urc_queue_t;

typedef struct atl_item_t
{
  char* req;  //Sended request, could be string or literal
  atl_parce_type_t parce_type;
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
} atl_item_t;

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
typedef void (*atl_printf_t)(const char *string);

typedef uint16_t (*atl_write_t)(uint8_t* buff, //buff where data will be written
                                uint16_t len); //len of data

typedef void (*atl_entity_cb_t)(const bool result,       //result
                                void* const meta,        //passed meta from @atl_entity_t
                                const void* const data); //data ptr from @atl_entity_t

typedef uint8_t atl_proc_states_t;
enum
{
  ATL_STATE_READ = 1,
  ATL_STATE_WRITE,
};

typedef struct {
  uint8_t *buffer;      
  uint16_t size;   
  uint16_t head;       
  uint16_t tail;       
  uint16_t count;     
} atl_ring_buffer_t;

typedef struct atl_init_t{
  atl_printf_t atl_printf;    //custom printf fucntion
  atl_write_t atl_write;      //custom write function
  O1HeapInstance* heap;       //instance for custom heap
  atl_ring_buffer_t* rx_buff; //rx ring buffer 
  bool init;                  //init flag
} atl_init_t; 

typedef struct atl_entity_t{
  atl_item_t*       item;       //list of items
  uint8_t           item_cnt;   //amount of items
  uint8_t           item_id;    //current id of executionable items
  uint16_t          timer;      //timer
  atl_entity_cb_t   cb;         //cb for item
  void*             data;       //usefull data from execution
  void*             meta;       //meta data
  uint16_t          data_size;  //usefull data size
  atl_proc_states_t state;      //state
} atl_entity_t;

typedef struct atl_entity_queue_t{
  atl_entity_t entity[ATL_ENTITY_QUEUE_SIZE]; //entity queue
  uint8_t entity_head;  //entitiy head
  uint8_t entity_tail;  //entity tail
  uint8_t entity_cnt;   //entity counter
} atl_entity_queue_t;

typedef struct atl_context_t {
  atl_entity_queue_t entity_queue; //entity queue
  atl_urc_queue_t urc_queue[ATL_URC_QUEUE_SIZE]; //urc queue
  atl_init_t init_struct; //init struct
  uint8_t mem_pool[ATL_MEMORY_POOL_SIZE] __attribute__((aligned(O1HEAP_ALIGNMENT)));
  uint32_t time;
} atl_context_t;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Init atl lib  
 ** @param  ctx        core context
 ** @param  atl_printf pointer to user func of printf
 ** @param  atl_write  pointer to user func of write to uart
 ** @param  rx_buff    struct to ring buffer
 ** @return none
 ******************************************************************************/
void atl_init(atl_context_t* const ctx, const atl_printf_t atl_printf, const atl_write_t atl_write, atl_ring_buffer_t* rx_buff);

/*******************************************************************************
 ** @brief  DeInit atl lib  
 ** @param  ctx core context
 ** @return none
 ******************************************************************************/
void atl_deinit(atl_context_t* const ctx);

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
bool atl_entity_enqueue(atl_context_t* const ctx, const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, void* const meta);

/*******************************************************************************
 ** @brief  Clear first entity from the queue 
 ** @param  ctx core context
 ** @return false: some errors; true: ok
 ******************************************************************************/
bool atl_entity_dequeue(atl_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function to append URC queue
 ** @param  ctx  core context
 ** @param  urc  ptr to your URC.
 ** @return true/false
 ******************************************************************************/
bool atl_urc_enqueue(atl_context_t* const ctx, const atl_urc_queue_t* const urc);

/*******************************************************************************
 ** @brief  Function to delete URC from queue
 ** @param  ctx  core context
 ** @param  prefix  prefix of your URC.
 ** @return true/false
 ******************************************************************************/
bool atl_urc_dequeue(atl_context_t* const ctx, char* prefix);

/*******************************************************************************
 ** @brief  Function to proc ATL core proccesses. 
 ** @param  ctx  core context
 ** @return none
 ******************************************************************************/
void atl_core_proc(atl_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function get time in 10ms. 
 ** @param  ctx  core context
 ** @return time
 ******************************************************************************/
uint32_t atl_get_cur_time(atl_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function get init. 
 ** @param  ctx  core context
 ** @return @atl_init_t
 ******************************************************************************/
atl_init_t atl_get_init(atl_context_t* const ctx);

/*******************************************************************************
 ** @brief  Function to custom malloc. Handle the concurrent state
 ** @param  ctx  core context
 ** @param  size amount to alloc
 ** @return ptr to new memory
 ******************************************************************************/
void* atl_malloc(atl_context_t* const ctx, size_t size);

/*******************************************************************************
 ** @brief  Function to custom free. Handle the concurrent state
 ** @param  ctx  core context
 ** @param  ptr  ptr to delete
 ** @return none
 ******************************************************************************/
void atl_free(atl_context_t* const ctx, void* ptr);

#ifdef ATL_TEST
void _atl_core_proc(atl_context_t* const ctx);
int _atl_cmd_ring_parcer(atl_context_t* const ctx, const atl_entity_t* const entity, const atl_item_t* const item, const ringslice_t rs_me);
void _atl_simcom_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* const req); 
void _atl_simcom_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res);
void _atl_simcom_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data); 
int _atl_simcom_parcer_post_proc(atl_context_t* const ctx, const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, const ringslice_t* const rs_data, const atl_item_t* const item, const atl_entity_t* const entity); 
int _atl_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern); 
int _atl_cmd_sscanf(const ringslice_t* const rs_data, const atl_item_t* const item); 
void _atl_process_urcs(atl_context_t* const ctx, const ringslice_t* me);
atl_entity_queue_t* _atl_get_entity_queue(atl_context_t* const ctx); 
atl_urc_queue_t* _atl_get_urc_queue(atl_context_t* const ctx); 
atl_init_t _atl_get_init(atl_context_t* const ctx);
#endif

#endif //__ATL_CORE_H

