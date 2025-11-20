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
#include "atl_port.h"

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ATL_CMD_SAVE             "SAVE:"
#define ATL_CMD_FORCE            "FORCE"
#define ATL_CMD_CRLF             "\r\n"
#define ATL_CMD_CR               "\r"
#define ATL_CMD_LF               "\n"
#define ATL_CMD_CTRL_Z           "\x1a"
#define ATL_CMD_OK               ATL_CMD_CRLF"OK"ATL_CMD_CRLF
#define ATL_CMD_ERROR            ATL_CMD_CRLF"ERROR"ATL_CMD_CRLF

#define ATL_ITEM_SIZE            sizeof(atl_item_t)
#define ATL_URC_SIZE             sizeof(atl_urc_queue_t)

#define ATL_ITEM(req_, prefix_, retries_, timeout_, err_step_, ok_step_, cb_, format_, ...) \
{ \
  .req = req_, \
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

typedef struct atl_urc_queue_t{
  char* prefix;  // Char prefix to find the URC
  atl_urc_cb cb; // Callback for this URC
} atl_urc_queue_t;

typedef struct atl_item_t
{
  char* req;  //Sended request, could be string or literal
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
                                void* const ctx,         //passed ctx from @atl_entity_t
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
  void*             ctx;        //user data context
  uint16_t          data_size;  //usefull data size
  atl_proc_states_t state;      //state
} atl_entity_t;

typedef struct atl_entity_queue_t{
  atl_entity_t entity[ATL_ENTITY_QUEUE_SIZE]; //entity queue
  uint8_t entity_head;  //entitiy head
  uint8_t entity_tail;  //entity tail
  uint8_t entity_cnt;   //entity counter
} atl_entity_queue_t;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Init atl lib  
 ** @param  atl_printf pointer to user func of printf
 ** @param  atl_write  pointer to user func of write to uart
 ** @param  rx_buff    ptr to RX ring buffer
 ** @return none
 ******************************************************************************/
void atl_init(const atl_printf_t atl_printf, const atl_write_t atl_write, atl_ring_buffer_t* rx_buff);

/*******************************************************************************
 ** @brief  DeInit atl lib  
 ** @param  atl_printf pointer to user func of printf
 ** @param  atl_write  pointer to user func of write to uart
 ** @return false: some error is ocur true: ok
 ******************************************************************************/
void atl_deinit(void);

/*******************************************************************************
 ** @brief  Function to append main queue with new group of at cmds
 ** @param  item         ptr to your group of at cmds.
 ** @param  item_amount  amount  of your at cms in group 
 ** @param  cb           ur callback function for the whole group.
 ** @param  data_size    If you`r expecting some usefull data while execution pass here size of them.
 **                      And pass the ptr of this data to VA ARGS of each item with proper format to
 **                      get this data. If no need pass the 0.
 ** @param  ctx          Ptr to some context of execution. Will be called in CB. Can be NULL.
 ** @return true: ok false: error while trying to append
 ******************************************************************************/
bool atl_entity_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, void* const ctx);

/*******************************************************************************
 ** @brief  Clear first entity from the queue 
 ** @param  none
 ** @return false: some errors; true: ok
 ******************************************************************************/
bool atl_entity_dequeue(void);

/*******************************************************************************
 ** @brief  Function to append URC queue
 ** @param  urc  ptr to your URC.
 ** @return ture/false
 ******************************************************************************/
bool atl_urc_enqueue(const atl_urc_queue_t* const urc);

/*******************************************************************************
 ** @brief  Function to delete URC from queue
 ** @param  urc  ptr to your URC.
 ** @return ture/false
 ******************************************************************************/
bool atl_urc_dequeue(char* prefix);

/*******************************************************************************
 ** @brief  Function to proc ATL core proccesses. 
 ** @param  none
 ** @return none
 ******************************************************************************/
void atl_core_proc(void);

/*******************************************************************************
 ** @brief  Function get time in 10ms. Call it each 10ms
 ** @param  none
 ** @return none
 ******************************************************************************/
uint16_t atl_get_cur_time(void);

/*******************************************************************************
 ** @brief  Function get init struct. 
 ** @param  none
 ** @return atl_init_t
 ******************************************************************************/
atl_init_t atl_get_init(void);

/*******************************************************************************
 ** @brief  Function to custom malloc. 
 ** @param  none
 ** @return none
 ******************************************************************************/
void* atl_malloc(size_t size);

/*******************************************************************************
 ** @brief  Function to custom free. 
 ** @param  none
 ** @return none
 ******************************************************************************/
void atl_free(void* ptr);

#ifdef ATL_TEST
void _atl_core_proc(void);
int _atl_cmd_ring_parcer(const atl_entity_t* const entity, const atl_item_t* const item);
void _atl_parcer_process_urcs(const ringslice_t* me);
void _atl_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* const req); 
void _atl_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res);
void _atl_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data); 
int _atl_parcer_post_proc(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, 
                          const ringslice_t* const rs_data, const atl_item_t* const item, const atl_entity_t* const entity); 
int _atl_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern); 
int _atl_cmd_sscanf(const ringslice_t* const rs_data, const atl_item_t* const item); 
atl_entity_queue_t* _atl_get_entity_queue(void); 
atl_urc_queue_t* _atl_get_urc_queue(void); 
atl_init_t _atl_get_init(void);
#endif

#endif //__ATL_CORE_H

