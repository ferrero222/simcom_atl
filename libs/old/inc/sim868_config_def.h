/**
  ******************************************************************************
  * @file    sim868_driver.c
  * @version V0.0.1
  * @date    14-february-2024
  * @brief   This file contains all the functions prototypes for the USART 
  *          firmware library.
  ******************************************************************************
  */ 
#ifndef __SIM868_CONFIG_H
#define __SIM868_CONFIG_H

/*******************************************************************************
 * Include files(USER)
 ******************************************************************************/
#include "boot.h"
#include "timers.h"

/*******************************************************************************
 * Pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
  ** @brief Pointers
  ** @def uart_pointers_macro
  ** @example #define SIM_TIMER100MS_PTR   &drv_sim_100ms_flag
**/ 
     #define SIM_TIMER100MS_PTR       USER_MACRO   // Pointer to the variable is the 100ms timer trigger flag
/**
  ** @brief UART channels macro 
  ** @def uart_channels_macro
  ** @example #define SIM_UARTGSM_CH   M4_USART1
**/ 
     #define SIM_USARTGSM_CH          USER_MACRO   // UARTGSM peripheral declaration in memory(pointer)
     #define SIM_USARTGNSS_CH         USER_MACRO   // UARTGPS peripheral declaration in memory(pointer)
     #define SIM_CHANNEl_TYPE         USER_MACRO   // typedef of usart channel
/**
  ** @brief UART interrupts macro 
  ** @def uart_interrupts_macro
  ** @example #define SIM_UsartTxEmptyInt UsartTxEmptyInt
**/ 
     #define SIM_USART_TX_EMPTY_INT   USER_MACRO   // TX empty interrupt enum/definition   
     #define SIM_USART_TX_CMPLT_INT   USER_MACRO   // TX complete interrupt enum/definition    
     #define SIM_USART_TX             USER_MACRO   // TX function enum/definition (OPTIONAL)
     #define SIM_USART_RX_INT         USER_MACRO   // RX no empty interrupt enum/definition    
     #define SIM_USART_RX             USER_MACRO   // RX function enum/definition (OPTIONAL)
     #define SIM_IT_FUNC_TYPE         USER_MACRO   // Type of FUNCTION IT macro variables(could be UINT or ENUM or...)
/**
  ** @brief UART interrupts macro 
  ** @def uart_interrupts_macro
  ** @example #define SIM_USART_RX_NO_EMPTY UsartRxNoEmpty
**/ 
     #define SIM_USART_RX_NO_EMPTY    USER_MACRO  // RX no empty status interrupt enum/definition   
     #define SIM_USART_TX_COMPLETE    USER_MACRO  // TX complete status interrupt enum/definition   
     #define SIM_USART_TX_EMPTY       USER_MACRO  // TX empty status enum/definition
     #define SIM_IT_STAT_TYPE         USER_MACRO  // Type of STATUS IT macro variables(could be uint or enum or...)
/**
  ** @brief UART interrupts macro 
  ** @def uart_interrupts_macro
  ** @example #define SIM_UsartTxEmptyInt UsartTxEmptyInt
  ** @note could be skipped
**/ 
     #define SIM_FUNC_DISABLE        USER_MACRO  // disable enum
     #define SIM_FUNC_ENABLE         USER_MACRO  // enable enum
     #define SIM_FUNC_STAT_TYPE      USER_MACRO  // type of enum for func status (Could be UINT or ENUM)
/**
  ** @brief Pinout functions macro
  ** @def pinout_function_macro
  ** @example #define SIM_GSMVBAT_TOGGLE(TIME) (PORT_Toggle(PortC, Pin06))
**/ 
     #define SIM_GPSVBAT_TOGGLE  USER_MACRO()  // GPSVBAT pinout toggle user function (OPTIONAL)
     #define SIM_GPSVBAT_RESET   USER_MACRO()  // GPSVBAT pinout reset user function (OPTIONAL)
     #define SIM_GSMVBAT_TOGGLE  USER_MACRO()  // GSMVBAT pinout toggle user function (OPTIONAL)
     #define SIM_GSMVBAT_RESET   USER_MACRO()  // GSMVBAT pinout reset user function (OPTIONAL)
     #define SIM_GNSSEN_TOGGLE   USER_MACRO()  // GNSSEN pinout toggle user function
     #define SIM_GNSSEN_RESET    USER_MACRO()  // GNSSEN pinout reset user function
     #define SIM_PWRKEY_TOGGLE   USER_MACRO()  // PWRKEY pinout toggle user function
     #define SIM_PWRKEY_RESET    USER_MACRO()  // PWRKEY pinout reset user function
     #define SIM_CTSUART_TOGGLE  USER_MACRO()  // CTS UART pinout toggle user function
     #define SIM_CTSUART_RESET   USER_MACRO()  // CTS UART pinout reset user function
     #define SIM_RTSUART_TOGGLE  USER_MACRO()  // RTS UART pinout toggle user function
     #define SIM_RTSUART_RESET   USER_MACRO()  // RTS UART pinout reset user function
     #define SIM_STATUS_GET_BIT  USER_MACRO()  // Get bit status from the STATUS pin(OPTIONAL)
/**
  ** @brief UART comm functions
  ** @def uart_comm_funct_macro
  ** @example #define SIM_USART_FUNCTCMD(CHANNEL, INTERRUPT, STATE) USART_FuncCmd(CHANNEL, INTERRUPT, STATE);
**/ 
     #define SIM_USART_SENDDATA(CHANNEL,DATA)               USER_MACRO()  // USART function sending 1 byte of data, should get 2 params, which is: 
                                                                                          // CHANNEL -   @ref uart_channels_macro
                                                                                          // DATA - pointer to array of sending data       

     #define SIM_USART_FUNCTCMD(CHANNEL, INTERRUPT, STATE)  USER_MACRO()  // USART function to CMD hardw, should get 3 param, which is: 
                                                                                          // CHANNEL -   @ref uart_channels_macro
                                                                                          // INTERRUPT - @ref uart_interrupts_macro
                                                                                          // STATE -     @ref sim_functional_state

     #define SIM_USART_RECDATA(CHANNEL)                     USER_MACRO()  // USART function recieve 1 byte of data, should get 1 params, which is: 
                                                                                          // CHANNEL -   @ref uart_channels_macro

     #define SIM_USART_GETSTAT(CHANNEL, INTERRUPT)          USER_MACRO()  // USART function geting status of interrupt, should get 2 params, which is: 
                                                                                          // CHANNEL -   @ref uart_interrupts_macro
                                                                                          // INTERRUPT - @ref pointer to array of sending data       

     #define SIM_USART_SETBAUDRATE(CHANNEL, BAUDRATE)       USER_MACRO()  // USART function change speed of UART, should get 2 params, which is: 
                                                                                          // CHANNEL -   @ref uart_interrupts_macro
#endif //__SIM868_CONFIG_H
