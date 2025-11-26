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
#include "atl_mdl_sms.h"
#include "dbc_assert.h"
#include "ringslice.h" 
#include <stdio.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_MDL_SMS")

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void atl_mdl_sms_cmgr_cb(ringslice_t rs_data, bool result, void* const data);

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
 ** @brief  Function to set sms format
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_format_set(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(101, param);
  char cmgf[32] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cmgf, sizeof(cmgf), "%sAT+CMGF=%d%s", ATL_CMD_SAVE, sms->format, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM(cmgf, NULL, ATL_PARCE_SIMCOM, 2, 150, 0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to set sc
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_sc_set(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(201, param);
  char csca[64] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(csca, sizeof(csca), "%sAT+CSCA=%s%s", ATL_CMD_SAVE, sms->num, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM(csca, NULL, ATL_PARCE_SIMCOM, 2, 150, 0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to send text sms
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_send_text(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(301, param);
  char cmgs[64] = {0};
  char text[161] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cmgs, sizeof(cmgs), "%sAT+CMGS=\"%s\"%s", ATL_CMD_SAVE, sms->num, ATL_CMD_CRLF); 
  snprintf(text, sizeof(text), "%s%s%s", ATL_CMD_SAVE, sms->msg, ATL_CMD_CTRL_Z); 
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM("AT+CMGF=1"ATL_CMD_CRLF,       NULL, ATL_PARCE_SIMCOM, 1, 150, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CSCS=\"GSM\""ATL_CMD_CRLF, NULL, ATL_PARCE_SIMCOM, 2, 500, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM(cmgs,                           ">",    ATL_PARCE_RAW, 1, 150, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM(text,                          NULL,    ATL_PARCE_RAW, 2, 500, 0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to read SMS
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_read(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(401, param);
  char cmgr[64] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cmgr, sizeof(cmgr), "%sAT+CMGR=%d,%d%s", ATL_CMD_SAVE, sms->index, true, ATL_CMD_CRLF);
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ATL_ITEM("AT+CMGF=1"ATL_CMD_CRLF, NULL, ATL_PARCE_SIMCOM, 1, 150, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM(cmgr,                    NULL, ATL_PARCE_SIMCOM, 2, 150, 0, 0, atl_mdl_sms_cmgr_cb, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_sms_msg_t), meta)) return false;
  return true;
}

/**
 *  @brief cmgr cb
 */
static void atl_mdl_sms_cmgr_cb(ringslice_t rs_data, bool result, void* const data)
{
  if(!result) return;
  if(ringslice_is_empty(&rs_data)) return;
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)data;
  ringslice_scanf(&rs_data, "+CMGR: \"%*[^\"]\",\"%[^\"]\",%*[^\x0d]\x0d\x0a%[^\x0d]", sms->num, sms->msg);
}

/*******************************************************************************
 ** @brief  Function to delete SMS
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_delete(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(501, param);
  char cmgd[64] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cmgd, sizeof(cmgd), "%sAT+CMGD=%d%s", ATL_CMD_SAVE, sms->index, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ATL_ITEM(cmgd, NULL, ATL_PARCE_SIMCOM, 1, 150, 0, 0, NULL, NULL,ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to indicate SMS
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_indicate(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(601, param);
  char cnmi[64] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cnmi, sizeof(cnmi), "%sAT+CNMI=%d%s", ATL_CMD_SAVE, sms->index, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ATL_ITEM(cnmi, NULL, ATL_PARCE_SIMCOM, 1, 150, 0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}


