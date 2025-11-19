/*******************************************************************************
 *                           ╔══╗╔════╗╔╗──                      (c)03.10.2025 *
 *                           ║╔╗║╚═╗╔═╝║║──                          v1.0.0    *
 *                           ║╚╝║──║║──║║──                                    *
 *                           ║╔╗║──║║──║║──                                    *
 *                           ║║║║──║║──║╚═╗                                    *
 *                           ╚╝╚╝──╚╝──╚══╝                                    *  
 ******************************************************************************/
#ifndef __ATL_MDL_GENERAL_H
#define __ATL_MDL_GENERAL_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h> 
#include <stdbool.h> 
#include <string.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
typedef struct modem_lbs_t {
  int cell;
  int mcc;
  int mnc;
  int lac;
  int cell_id;
} atl_mdl_rtd_lbs_t;

typedef struct atl_mdl_rtd_t {
  char modem_imei[16];  
  char modem_id[16];     
  char modem_rev[30];     
  char modem_clock[22];
  char sim_iccid[22];
  char sim_operator[50];
  int sim_rssi;
  atl_mdl_rtd_lbs_t modem_lbs[7];
  int lbs_cnt;
} atl_mdl_rtd_t;

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Function to restart the modem
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_modem_reset(const atl_entity_cb_t cb, const void* const param, void* const ctx);

/*******************************************************************************
 ** @brief  Function to init modem
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_modem_init(const atl_entity_cb_t cb, const void* const param, void* const ctx);

/*******************************************************************************
 ** @brief  Function to get real time data info about modem. @atl_mdl_rtd_t
 **         will be passed to the data paramater in callback. Get it there.
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_rtd(const atl_entity_cb_t cb, const void* const param, void* const ctx);

 #endif //__ATL_MDL_GENERAL_H 