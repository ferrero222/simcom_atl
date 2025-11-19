/*******************************************************************************
 *                           ╔══╗╔════╗╔╗──                      (c)03.10.2025 *
 *                           ║╔╗║╚═╗╔═╝║║──                          v1.0.0    *
 *                           ║╚╝║──║║──║║──                                    *
 *                           ║╔╗║──║║──║║──                                    *
 *                           ║║║║──║║──║╚═╗                                    *
 *                           ╚╝╚╝──╚╝──╚══╝                                    *  
 ******************************************************************************/
#ifndef __ATL_PORT_H
#define __ATL_PORT_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h> 
#include <stdbool.h> 
#include <string.h>
#include <assert.h>
#include "dbc_assert.h"

/*******************************************************************************
 * Config
 ******************************************************************************/
#define ATL_MAX_ITEMS_PER_ENTITY   50     //Max amount of AT cmds in one group (if you need to change, check step field sizes also)
  
#define ATL_ENTITY_QUEUE_SIZE      10     //Max amount of groups 
  
#define ATL_URC_QUEUE_SIZE         10     //Amount of handled URC
  
#define ATL_MEMORY_POOL_SIZE       4096   //Memory pool for custom heap
   
#ifndef ATL_TEST  
  #define ATL_DEBUG_ENABLED        1      //Recommend to turn on DEBUG logs
#endif

#if ATL_DEBUG_ENABLED
  #define ATL_DEBUG(fmt, ...) atl_printf_safe(fmt, __VA_ARGS__)
#else
  #define ATL_DEBUG(fmt, ...) ((void)0)
#endif

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** @brief  DBC fault override
 ** @param  none
 ** @return none
 ******************************************************************************/
DBC_NORETURN void DBC_fault_handler(char const* module, int label); 

/*******************************************************************************
 ** @brief  Weak function to enter into critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
void _atl_crit_enter(void); 

/*******************************************************************************
 ** @brief  Weak function to exit critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
void _atl_crit_exit(void);

/*******************************************************************************
 ** @brief  Printf
 ** @param  none
 ** @return none
 ******************************************************************************/
void atl_printf_safe(const char *fmt, ...);

#endif
