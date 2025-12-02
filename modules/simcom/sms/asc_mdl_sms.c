/******************************************************************************
 *                              _    ____   ____                              *
 *                   ======    / \  / ___| / ___| ======       (c)03.10.2025  *
 *                   ======   / _ \ \___ \| |     ======           v1.0.0     *
 *                   ======  / ___ \ ___) | |___  ======                      *
 *                   ====== /_/   \_\____/ \____| ======                      *  
 *                                                                            *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "asc_core.h"
#include "asc_mdl_sms.h"
#include "dbc_assert.h"
#include "ringslice.h" 
#include <stdio.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ASC_MDL_SMS")

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void asc_mdl_sms_cmgr_cb(ringslice_t rs_data, bool result, void* const data);

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
 ** @param  param  input param if function is required them. Here is @asc_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_sms_format_set(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(101, param);
  char cmgf[32] = {0};
  asc_mdl_sms_msg_t* sms = (asc_mdl_sms_msg_t*)param;
  snprintf(cmgf, sizeof(cmgf), "%sAT+CMGF=%d%s", ASC_CMD_SAVE, sms->format, ASC_CMD_CRLF); 
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM(cmgf, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 0, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to set sc
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @asc_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_sms_sc_set(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(201, param);
  char csca[64] = {0};
  asc_mdl_sms_msg_t* sms = (asc_mdl_sms_msg_t*)param;
  snprintf(csca, sizeof(csca), "%sAT+CSCA=%s%s", ASC_CMD_SAVE, sms->num, ASC_CMD_CRLF); 
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM(csca, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 0, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to send text sms
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @asc_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_sms_send_text(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(301, param);
  char cmgs[64] = {0};
  char text[161] = {0};
  asc_mdl_sms_msg_t* sms = (asc_mdl_sms_msg_t*)param;
  snprintf(cmgs, sizeof(cmgs), "%sAT+CMGS=\"%s\"%s", ASC_CMD_SAVE, sms->num, ASC_CMD_CRLF); 
  snprintf(text, sizeof(text), "%s%s%s", ASC_CMD_SAVE, sms->msg, ASC_CMD_CTRL_Z); 
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM("AT+CMGF=1"ASC_CMD_CRLF,       NULL, ASC_PARCE_SIMCOM, 1, 150, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CSCS=\"GSM\""ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 500, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM(cmgs,                           ">",    ASC_PARCE_RAW, 1, 150, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM(text,                          NULL,    ASC_PARCE_RAW, 2, 500, 0, 0, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to read SMS
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @asc_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_sms_read(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(401, param);
  char cmgr[64] = {0};
  asc_mdl_sms_msg_t* sms = (asc_mdl_sms_msg_t*)param;
  snprintf(cmgr, sizeof(cmgr), "%sAT+CMGR=%d,%d%s", ASC_CMD_SAVE, sms->index, true, ASC_CMD_CRLF);
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ASC_ITEM("AT+CMGF=1"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 1, 150, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM(cmgr,                    NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 0, asc_mdl_sms_cmgr_cb, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, sizeof(asc_mdl_sms_msg_t), meta)) return false;
  return true;
}

/**
 *  @brief cmgr cb
 */
static void asc_mdl_sms_cmgr_cb(ringslice_t rs_data, bool result, void* const data)
{
  if(!result) return;
  if(ringslice_is_empty(&rs_data)) return;
  asc_mdl_sms_msg_t* sms = (asc_mdl_sms_msg_t*)data;
  ringslice_scanf(&rs_data, "+CMGR: \"%*[^\"]\",\"%[^\"]\",%*[^\x0d]\x0d\x0a%[^\x0d]", sms->num, sms->msg);
}

/*******************************************************************************
 ** @brief  Function to delete SMS
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @asc_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_sms_delete(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(501, param);
  char cmgd[64] = {0};
  asc_mdl_sms_msg_t* sms = (asc_mdl_sms_msg_t*)param;
  snprintf(cmgd, sizeof(cmgd), "%sAT+CMGD=%d%s", ASC_CMD_SAVE, sms->index, ASC_CMD_CRLF); 
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ASC_ITEM(cmgd, NULL, ASC_PARCE_SIMCOM, 1, 150, 0, 0, NULL, NULL,ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to indicate SMS
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @asc_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_sms_indicate(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(601, param);
  char cnmi[64] = {0};
  asc_mdl_sms_msg_t* sms = (asc_mdl_sms_msg_t*)param;
  snprintf(cnmi, sizeof(cnmi), "%sAT+CNMI=%d%s", ASC_CMD_SAVE, sms->index, ASC_CMD_CRLF); 
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ASC_ITEM(cnmi, NULL, ASC_PARCE_SIMCOM, 1, 150, 0, 0, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}


