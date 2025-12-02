/******************************************************************************
 *                              _    ____   ____                              *
 *                   ======    / \  / ___| / ___| ======       (c)03.10.2025  *
 *                   ======   / _ \ \___ \| |     ======           v1.0.0     *
 *                   ======  / ___ \ ___) | |___  ======                      *
 *                   ====== /_/   \_\____/ \____| ======                      *  
 *                                                                            *
 ******************************************************************************/
#ifndef __ASC_MDL_GENERAL_H
#define __ASC_MDL_GENERAL_H

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
} asc_mdl_rtd_lbs_t;

typedef struct asc_mdl_rtd_t {
  char modem_imei[16];  
  char modem_id[16];     
  char modem_rev[30];     
  char modem_clock[22];
  char sim_iccid[22];
  char sim_operator[50];
  int sim_rssi;
  asc_mdl_rtd_lbs_t modem_lbs[7];
  int lbs_cnt;
} asc_mdl_rtd_t;

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Function to restart the modem
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Here is NULL
 ** @param  param  input param if function is required them. Here is
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_modem_reset(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta);

/*******************************************************************************
 ** @brief  Function to init modem
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_modem_init(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta);

/*******************************************************************************
 ** @brief  Function to get real time data info about modem. @asc_mdl_rtd_t
 **         will be passed to the data paramater in callback. Get it there.
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Here is NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_rtd(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta);

 #endif //__ASC_MDL_GENERAL_H 