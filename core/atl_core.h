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
#define ATL_ENTITY_QUEUE_SIZE    10

#define ATL_URC_QUEUE_SIZE       10

#define ATL_MEMORY_POOL_SIZE     2048
 
#define ATL_DEBUG_ENABLED        1

#define ATL_FREQUENCY            10 //Hz

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
    if (atl_user_init.atl_printf) { \
        atl_user_init.atl_printf("[ATL][%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } \
} while(0)
#else
#define ATL_DEBUG(fmt, ...) ((void)0)
#endif

#define ATL_ITEM(req, prefix, format, retries, timeout, err_step, ok_step, cb, ...) \
{ \
  .req = req, \
  .answ.prefix = prefix, \
  .answ.format_data.format = format, \
  .answ.format_data.ptrs = (void*[]){__VA_ARGS__, NULL}, \
  .answ.cb = cb, \
  .meta.rpt_cnt = retries, \
  .meta.wait = timeout, \
  .meta.err_step = err_step, \
  .meta.ok_step = ok_step, \
  .meta.type = 0 \
}

#define ATL_CRITICAL_ENTER  atl_critical_enter();
#define ATL_CRITICAL_EXIT   atl_critical_exit();

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
typedef void (*answ_parce_cb_t)(ringslice_t data_slice, int result); 

typedef struct atl_urc_t{
  char* prefix;
  atl_urc_cb cb;
} atl_urc_t;

typedef struct atl_item_t
{
  char* req;
  struct{
    const char *prefix;
    const char *format;
    void **ptrs;
    answ_parce_cb_t cb;
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
typedef void (*atl_printf)(const char *format, ...);
typedef uint16_t (*atl_write)(uint8_t buff, uint16_t len);
typedef void (*atl_entity_cb_t)(bool result);        //at cmd group callback type
typedef void (*atl_urc_cb)(ringslice_t urc_slice);   //urc callback type

typedef enum
{
  ATL_STATE_READ = 1,
  ATL_STATE_WRITE,
} atl_proc_states_t;

typedef struct atl_user_init_t{
  bool init;
  atl_printf atl_printf;
  atl_write atl_write;
  tlsf_t atl_tlfs;
  uint8_t* ring; 
  uint16_t ring_len; 
  uint16_t* ring_tail; 
  uint16_t* ring_head;
} atl_init_t;

typedef struct {
  atl_item_t*       item;
  uint8_t           item_cnt;   //amount of cmds
  uint8_t           item_id;    //current id of executionable cmd
  uint16_t          timer;      //timer
  atl_proc_states_t state;      //state
  atl_entity_cb_t   cb;         //cb for item
} atl_entity_t;

typedef struct {
  atl_entity_t entity[ATL_ENTITY_QUEUE_SIZE];
  uint8_t entity_head;
  uint8_t entity_tail;
  uint8_t entity_cnt;
} atl_entity_queue_t;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Init atl lib  
 ** \param  atl_printf pointer to user func of printf
 ** \param  atl_write  pointer to user func of write to uart
 ** \param  ring            ptr to RX ring buffer
 ** \param  ring_len        lenght of RX ring buffer. 
 ** \param  ring_tail       tail of RX ring buffer. 
 ** \param  ring_head       head of RX ring buffer. 
 ** \retval none
 ******************************************************************************/
void atl_init(const atl_printf atl_printf, const atl_write atl_write, const uint8_t* ring, 
              const uint16_t ring_len, const uint16_t* const ring_tail, const uint16_t* const ring_head);

/*******************************************************************************
 ** \brief  DeInit atl lib  
 ** \param  atl_printf pointer to user func of printf
 ** \param  atl_write  pointer to user func of write to uart
 ** \retval false: some error is ocur true: ok
 ******************************************************************************/
void atl_deinit(void);

/*******************************************************************************
 ** \brief  Function to get lib init status
 ** \param  none
 ** \retval true/false
 ******************************************************************************/
bool atl_is_init(void);

/*******************************************************************************
 ** \brief  Function to append main queue with new group of at cmds
 ** \param  item         ptr to your group of at cmds.
 ** \param  item_amount  amount  of your at cms in group 
 ** \param  entity_cb    ur callback function for the whole group  ptr to your group of at cmds.
 ** \retval true: ok false: error while trying to append
 ******************************************************************************/
bool atl_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t entity_cb);

/*******************************************************************************
 ** \brief  Clear first entity from the queue 
 ** \param  none
 ** \retval false: some errors; true: ok
 ******************************************************************************/
bool atl_dequeue(void);

/*******************************************************************************
 ** \brief  Function to append URC queue
 ** \param  urc  ptr to your URC.
 ** \retval ture/false
 ******************************************************************************/
bool atl_urc_enqueue(const atl_urc_t* const urc);

/*******************************************************************************
 ** \brief  Function to delete URC from queue
 ** \param  urc  ptr to your URC.
 ** \retval ture/false
 ******************************************************************************/
bool atl_urc_dequeue(const atl_urc_t* const urc);

/*******************************************************************************
 ** \brief  Function to proc ATL core proccesses. 
 ** \param  none
 ** \retval none
 ******************************************************************************/
void atl_entity_proc(void);

/*******************************************************************************
 ** \brief  DBC fault override
 ** \param  none
 ** \retval none
 ******************************************************************************/
__attribute__((weak)) DBC_NORETURN void DBC_fault_handler(char const * module, int label);

/*******************************************************************************
 ** \brief  Weak function to enter into critical section
 ** \param  none
 ** \retval none
 ******************************************************************************/
__attribute__((weak)) void atl_crit_enter(void);

/*******************************************************************************
 ** \brief  Weak function to exit critical section
 ** \param  none
 ** \retval none
 ******************************************************************************/
__attribute__((weak)) void atl_crit_exit(void);

#endif //__ATL_H
