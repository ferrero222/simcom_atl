/*******************************************************************************
 *                           ╔══╗╔════╗╔╗──                      (c)03.10.2025 *
 *                           ║╔╗║╚═╗╔═╝║║──                          v1.0.0    *
 *                           ║╚╝║──║║──║║──                                    *
 *                           ║╔╗║──║║──║║──                                    *
 *                           ║║║║──║║──║╚═╗                                    *
 *                           ╚╝╚╝──╚╝──╚══╝                                    *  
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "atl_core.h"
#include "atl_mdl_general.h"
#include "dbc_assert.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void atl_mdl_general_ceng_cb(ringslice_t rs_data, bool result, void* const data);

/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Function to restart the modem
 ** @param  cb     cb when proc will be done. Here is NULL
 ** @param  param  input param if function is required them. Here is
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_modem_reset(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  (void)param;
  atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM("AT+CFUN=1,1"ATL_CMD_CRLF, NULL, 2, 150, 0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to init modem
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_modem_init(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  (void)param;
  atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM("AT"ATL_CMD_CRLF,   NULL, 5, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT"ATL_CMD_CRLF,   NULL, 5, 100, 0, 1, NULL, NULL, ATL_NO_ARG),  
    ATL_ITEM("ATE1"ATL_CMD_CRLF, NULL, 5, 100, 0, 0, NULL, NULL, ATL_NO_ARG),  
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to get real time data info about modem. @atl_mdl_rtd_t
 **         will be passed to the data paramater in callback. Get it there.
 ** @param  cb     cb when proc will be done. Here is NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_rtd(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  (void)param;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {   
    ATL_ITEM("AT+GSN"ATL_CMD_CRLF,       NULL, 10, 100, 0, 1, NULL,               "%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
    ATL_ITEM("AT+GMM"ATL_CMD_CRLF,       NULL, 10, 100, 0, 1, NULL,               "%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_id)),  
    ATL_ITEM("AT+GMR"ATL_CMD_CRLF,       NULL, 10, 100, 0, 1, NULL,      "Revision:%29[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_rev)),                
    ATL_ITEM("AT+CCLK?"ATL_CMD_CRLF,  "+CCLK", 10, 100, 0, 1, NULL,      "+CCLK: \"%21[^\"]\"", ATL_ARG(atl_mdl_rtd_t, modem_clock)),          
    ATL_ITEM("AT+CCID"ATL_CMD_CRLF,      NULL, 10, 100, 0, 1, NULL,               "%21[^\x0d]", ATL_ARG(atl_mdl_rtd_t, sim_iccid)),             
    ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,  "+COPS", 20, 100, 0, 1, NULL, "+COPS: 0, 0,\"%49[^\"]\"", ATL_ARG(atl_mdl_rtd_t, sim_operator)),            
    ATL_ITEM("AT+CSQ"ATL_CMD_CRLF,     "+CSQ", 10, 100, 0, 1, NULL,                 "+CSQ: %d", ATL_ARG(atl_mdl_rtd_t, sim_rssi)),             
    ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,    NULL, 10, 100, 0, 1, NULL,                       NULL, ATL_NO_ARG),                
    ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,     NULL, 10, 100, 0, 0, atl_mdl_general_ceng_cb,    NULL, ATL_NO_ARG),                         
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_rtd_t), ctx)) return false;
  return true;
}

/**
 *  @brief ceng cb
 */
static void atl_mdl_general_ceng_cb(ringslice_t rs_data, bool result, void* const data)
{
  if(!result) return;
  if(ringslice_is_empty(&rs_data)) return;
  bool header_skip = false;
  atl_mdl_rtd_t* rtd = (atl_mdl_rtd_t*)data;
  while(ringslice_scanf(&rs_data, "+CENG: %d,\"%d,%d,%x,%x,", 
                        &rtd->modem_lbs[rtd->lbs_cnt].cell, &rtd->modem_lbs[rtd->lbs_cnt].mcc,
                        &rtd->modem_lbs[rtd->lbs_cnt].mnc, &rtd->modem_lbs[rtd->lbs_cnt].lac,
                        &rtd->modem_lbs[rtd->lbs_cnt].cell_id))
  {
    if(header_skip && rtd->modem_lbs[rtd->lbs_cnt].cell_id != 0 && rtd->modem_lbs[rtd->lbs_cnt].lac != 0) ++rtd->lbs_cnt;
    if(rtd->lbs_cnt >= 7) break;
    ringslice_t rs_temp = {0}; 
    if(header_skip) rs_temp = ringslice_strstr(&rs_data, "\r\n");
    else            rs_temp = ringslice_strstr(&rs_data, "\r\n\r\n");
    if(ringslice_is_empty(&rs_temp)) break;
    rs_data = ringslice_subslice_after(&rs_data, &rs_temp, 0);
    if(ringslice_is_empty(&rs_data)) break;
    header_skip = true;
  }
}