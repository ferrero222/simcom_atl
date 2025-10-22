/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)03.10.2025 *
 *               ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──       v1.0.0    *
 *               ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                 *
 *               ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                 *
 *               ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                 *
 *               ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                 *  
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
#include "tlsf.h"
#include "ringslice.h"

/*******************************************************************************
 * Config
 ******************************************************************************/
#define ATL_MAX_ITEMS_PER_ENTITY 50

#define ATL_ENTITY_QUEUE_SIZE    10

#define ATL_URC_QUEUE_SIZE       10

#define ATL_MEMORY_POOL_SIZE     2048
 
#define ATL_DEBUG_ENABLED        1

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ATL_CMD_SAVE             "SAVE:"
#define ATL_CMD_CRLF             "\x0d\x0a"
#define ATL_CMD_CR               '\x0d'
#define ATL_CMD_LF               '\x0a'
#define ATL_CMD_CTRL_Z           "\x1a"
#define ATL_CMD_OK               ATL_CMD_CRLF"OK"ATL_CMD_CRLF
#define ATL_CMD_ERROR            ATL_CMD_CRLF"ERROR"ATL_CMD_CRLF

#define ATL_ITEM_SIZE            sizeof(atl_item_t)
#define ATL_URC_SIZE             sizeof(atl_urc_t)

#if ATL_DEBUG_ENABLED
#define ATL_DEBUG(fmt, ...) do { \
    if (atl_get_init().atl_printf) { \
        atl_get_init().atl_printf("[ATL][%s:%d]"fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } \
} while(0)
#else
#define ATL_DEBUG(fmt, ...) ((void)0)
#endif

#define ATL_ITEM(req, prefix, format, retries, timeout, err_step, ok_step, cb, ...) \
{ \
  req, \
  {prefix, format,(void*[]){__VA_ARGS__, NULL}, cb}, \
  {retries, timeout, err_step, ok_step} \
}

#define ATL_ARG(src, field) ((void*)offsetof(src, field))

#define ATL_CRITICAL_ENTER  atl_critical_enter();
#define ATL_CRITICAL_EXIT   atl_critical_exit();

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
typedef void (*answ_parce_cb_t)(ringslice_t data_slice, bool result, void* data);
typedef void (*atl_urc_cb)(ringslice_t urc_slice);   //urc callback type

typedef struct atl_urc_t{
  char* prefix;  // Char prefix to find the URC
  atl_urc_cb cb; // Callback for this URC
} atl_urc_t;

typedef struct atl_item_t
{
  char* req;  //Sended request, could be string or literal
  struct{
    char *prefix;   //Prefix to find in answer
    char *format;   //format for parcing answer
    void **ptrs;          //VA ARGS for format
    answ_parce_cb_t cb;   //Callback by the end
  } answ;
  struct {
    uint16_t wait     : 12;  // wait time (0-4095) in ATL_FREQUENCY
    uint8_t  rpt_cnt  : 4;   // repeat counter (up to 15)
    int8_t   err_step : 6;   // error step (-31 to 31)  
    int8_t   ok_step  : 6;   // success step (-31 to 31)
  } meta;
} atl_item_t;

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
typedef void (*atl_printf_t)(const char *format, ...);

typedef uint16_t (*atl_write_t)(uint8_t buff, //buff where data will bi written
                                uint16_t len); //len of data

typedef void (*atl_entity_cb_t)(bool  result, //result
                                void* ctx,    //passed ctx from @atl_entity_t
                                void* data);  //data ptr from @atl_entity_t

typedef enum
{
  ATL_STATE_READ = 1,
  ATL_STATE_WRITE,
} atl_proc_states_t;

typedef struct atl_init_t{
  bool init;               //init flag
  atl_printf_t atl_printf; //custom printf fucntion
  atl_write_t atl_write;   //custom write function
  tlsf_t atl_tlsf;         //instance for TLSF lib
  uint8_t* ring;           //rx ring buff
  uint16_t ring_len;       //rx ring buff len
  uint16_t* ring_tail;     //rx ring buff tail
  uint16_t* ring_head;     //rx ring buff head
} atl_init_t;

typedef struct atl_entity_t{
  atl_item_t*       item;       //list of items
  uint8_t           item_cnt;   //amount of items
  uint8_t           item_id;    //current id of executionable items
  uint16_t          timer;      //timer
  atl_proc_states_t state;      //state
  atl_entity_cb_t   cb;         //cb for item
  void*             data;       //usefull data from execution
  void*             ctx;        //user data context
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
 ** @param  ring            ptr to RX ring buffer
 ** @param  ring_len        lenght of RX ring buffer. 
 ** @param  ring_tail       tail of RX ring buffer. 
 ** @param  ring_head       head of RX ring buffer. 
 ** @return none
 ******************************************************************************/
void atl_init(const atl_printf_t atl_printf, const atl_write_t atl_write, const uint8_t* const ring, 
              const uint16_t ring_len, const uint16_t* const ring_tail, const uint16_t* const ring_head);

/*******************************************************************************
 ** @brief  DeInit atl lib  
 ** @param  atl_printf pointer to user func of printf
 ** @param  atl_write  pointer to user func of write to uart
 ** @return false: some error is ocur true: ok
 ******************************************************************************/
void atl_deinit(void);

/*******************************************************************************
 ** @brief  Function to get lib init status
 ** @param  none
 ** @return true/false
 ******************************************************************************/
bool atl_is_init(void);

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
bool atl_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, const void* const ctx);

/*******************************************************************************
 ** @brief  Clear first entity from the queue 
 ** @param  none
 ** @return false: some errors; true: ok
 ******************************************************************************/
bool atl_dequeue(void);

/*******************************************************************************
 ** @brief  Function to append URC queue
 ** @param  urc  ptr to your URC.
 ** @return ture/false
 ******************************************************************************/
bool atl_urc_enqueue(const atl_urc_t* const urc);

/*******************************************************************************
 ** @brief  Function to delete URC from queue
 ** @param  urc  ptr to your URC.
 ** @return ture/false
 ******************************************************************************/
bool atl_urc_dequeue(const atl_urc_t* const urc);

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
 ** @return none
 ******************************************************************************/
atl_init_t atl_get_init(void);

/*******************************************************************************
 ** @brief  DBC fault override
 ** @param  none
 ** @return none
 ******************************************************************************/
__attribute__((weak)) DBC_NORETURN void DBC_fault_handler(char const * module, int label);

/*******************************************************************************
 ** @brief  Weak function to enter into critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
__attribute__((weak)) void atl_crit_enter(void);

/*******************************************************************************
 ** @brief  Weak function to exit critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
__attribute__((weak)) void atl_crit_exit(void);

#endif //__ATL_CORE_H