/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)03.10.2025 *
 *               ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──       v1.0.0    *
 *               ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                 *
 *               ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                 *
 *               ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                 *
 *               ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                 *  
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "atl_core.h"
#include "atl_mdl_sms.h"
#include "dbc_assert.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_MDL_SMS");

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
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
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_format_set(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  DBC_REQUIRE(101, param);
  char cmgf[32] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cmgf, sizeof(cmgf), "%sAT+CMGF=%d%s", ATL_CMD_SAVE, sms->format, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM(cmgf, NULL, NULL, 2, 150, 0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to set sc
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_sc_set(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  DBC_REQUIRE(201, param);
  char csca[64] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(csca, sizeof(csca), "%sAT+CSCA=%d%s", ATL_CMD_SAVE, sms->num, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM(csca, NULL, NULL, 2, 150, 0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to send text sms
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_send_text(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  DBC_REQUIRE(301, param);
  char cmgs[64] = {0};
  char text[161] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cmgs, sizeof(cmgs), "%sAT+CMGS=\"%s\"%s", ATL_CMD_SAVE, sms->num, ATL_CMD_CRLF); 
  snprintf(text, sizeof(text), "%s%s%s", ATL_CMD_SAVE, sms->num, ATL_CMD_CTRL_Z); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CMGF=1"ATL_CMD_CRLF,         NULL, NULL, 1, 150, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CSCS=\"GSM\""ATL_CMD_CRLF,   NULL, NULL, 2, 150, 0, 1, NULL, NULL),
    ATL_ITEM(cmgs,                             ">", NULL, 1, 150, 0, 1, NULL, NULL),
    ATL_ITEM(text,                            NULL, NULL, 2, 500, 0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to read SMS
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_read(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  DBC_REQUIRE(401, param);
  char cmgr[64] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cmgr, sizeof(cmgr), "%sAT+CMGR=%d,%d%s", ATL_CMD_SAVE, sms->index, true, ATL_CMD_CRLF);
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM("AT+CMGF=1"ATL_CMD_CRLF, NULL, NULL, 1, 15, 0, 1, NULL, NULL),
    ATL_ITEM(cmgr, "+CMGR: \"%*[^\"]\",\"%[^\"]\",%*[^\x0d]\x0d\x0a%[^\x0d]", NULL, 2, 150, 0, 0, NULL, ATL_ARG(atl_mdl_sms_msg_t, num), ATL_ARG(atl_mdl_sms_msg_t, msg)),

  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_sms_msg_t), ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to delete SMS
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_delete(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  DBC_REQUIRE(501, param);
  char cmgd[64] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cmgd, sizeof(cmgd), "%sAT+CMGD=%d%s", ATL_CMD_SAVE, sms->index, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM(cmgd, NULL, NULL, 1, 150, 0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to indicate SMS
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_indicate(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  DBC_REQUIRE(601, param);
  char cnmi[64] = {0};
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)param;
  snprintf(cnmi, sizeof(cnmi), "%sAT+CNMI=%d%s", ATL_CMD_SAVE, sms->index, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM(cnmi, NULL, NULL, 1, 150, 0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  URC cb example for catch SMS
 ** @param  urc_slice  slice of this urc instance. 
 ** @param  cb         cb when proc will be done. 
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
void atl_mdl_sms_urc_cb(const ringslice_t urc_slice)
{
  uint16_t index = 0;
  atl_mdl_sms_msg_t sms = {0};
  ringclise_scanf(urc_slice, "+CMTI:%*[^,],%d\x0d", &sms.index);
  if(0 != index) atl_mdl_sms_read(NULL, (void*)&(atl_mdl_sms_msg_t){.index = sms.index}, NULL);
}


