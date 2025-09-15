/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)26.05.2025 *
 *               ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──       v1.0.0    *
 *               ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                 *
 *               ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                 *
 *               ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                 *
 *               ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                 *  
 ******************************************************************************/
#ifndef __ATL_H
#define __ATL_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h> 
#include <stdbool.h> 
#include <string.h>
#include <assert.h>

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ATL_CMD_CRLF      "\x0d\x0a"
#define ATL_CMD_CR        '\x0d'
#define ATL_CMD_LF        '\x0a'
#define ATL_CMD_CTRL_Z    "\x1a"

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
typedef void (*atl_printf)(const char *format, ...);
typedef uint16_t (*atl_write)(uint8_t buff, uint16_t len);
typedef void (*atl_entity_cb_t)(bool result);                 //at cmd group callback type
typedef void (*atl_urc_cb)(uint8_t *p_urc_str, uint16_t len);  //urc callback type
 
typedef struct atl_item_t{
  char* request;            //string literal to at request
  bool save;                //flag to save request into memory
  char* answer;             //string literal to expected answer
  uint8_t* data;            //ptr to data storage where usefull data will be placed
  uint16_t data_len;        //data storage len
  uint8_t  rpt_cnt;         //repeat counter
  uint8_t  fun_step_error;  //step in cmd queue if error occurs
  uint8_t  fun_step_finish; //step in cmd queue if all done successfully
  uint16_t wait;            //wait for answer
} atl_item_t; 

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/

#endif //__ATL_H
