/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)26.05.2025 *
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
 ** \brief  Function init GPRS
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_format_set(uint8_t format, atl_entity_cb_t cb)
{
  char cmgf[32] = {0};
  snprintf(cmgf, sizeof(cmgf), "%sAT+CMGF=%d%s", ATL_CMD_SAVE, format, ATL_CMD_CRLF); \
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM(cmgf, NULL, NULL, 2, 15, 0, 0, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function init GPRS
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_sc_set(char* num, atl_entity_cb_t cb)
{
  char csca[64] = {0};
  snprintf(csca, sizeof(csca), "%sAT+CSCA=%d%s", ATL_CMD_SAVE, num, ATL_CMD_CRLF); \
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM(csca, NULL, NULL, 2, 15, 0, 0, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_send_text(char* num, char* msg, atl_entity_cb_t cb)
{
  char cmgs[64] = {0};
  char text[161] = {0};
  snprintf(cmgs, sizeof(cmgs), "%sAT+CMGS=\"%s\"%s", ATL_CMD_SAVE, num, ATL_CMD_CRLF); \
  snprintf(csca, sizeof(csca), "%s%s%s", ATL_CMD_SAVE, num, ATL_CMD_CTRL_Z); \
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CMGF=1"ATL_CMD_CRLF,         NULL, NULL, 1, 15, 0, 1, NULL),
    ATL_ITEM("AT+CSCS=\"GSM\""ATL_CMD_CRLF,   NULL, NULL, 2, 15, 0, 1, NULL),
    ATL_ITEM(cmgs,                             ">", NULL, 1, 15, 0, 1, NULL),
    ATL_ITEM(text,                            NULL, NULL, 2, 50, 0, 0, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
static atl_mdl_sms_msg_t* atl_sms = NULL;
static atl_mdl_sms_msg_cb_t atl_mdl_sms_msg_user_cb = NULL;
 
bool atl_mdl_sms_read(uint16_t index, atl_mdl_sms_msg_cb_t cb)
{
  DBC_REQUIRE(101, atl_is_init());
  char cmgs[64] = {0};
  snprintf(cmgr, sizeof(cmgs), "%sAT+CMGR=%d,%d%s", ATL_CMD_SAVE, index, true, ATL_CMD_CRLF);
  atl_sms = (atl_mdl_sms_msg_t*)tlsf_malloc(atl_user_init.atl_tlsf, sizeof(atl_mdl_sms_msg_t));
  if(!atl_sms) 
  {
    ATL_DEBUG("[ERROR] Failed to allocate memory\n"); 
    atl_deinit(); 
    return false; 
  } 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM("AT+CMGF=1"ATL_CMD_CRLF, NULL, NULL, 1, 15, 0, 1, NULL),
    ATL_ITEM(cmgr, "+CMGR: \"%*[^\"]\",\"%[^\"]\",%*[^\x0d]\x0d\x0a%[^\x0d]", NULL, 2, 15, 0, 1, NULL, atl_sms->num, atl_sms->msg),

  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), atl_mdl_sms_msg_cb)) return false;
  if(cb) atl_mdl_sms_msg_user_cb_t = cb;
  return true;
}

/**
 *  \brief rtd cb
 */
void atl_mdl_sms_msg_cb(bool result)
{
  if(atl_mdl_sms_msg_user_cb_t) atl_mdl_sms_msg_user_cb_t(result, atl_sms);
  tlsf_free(atl_user_init.atl_tlsf, atl_sms);
}

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_delete(uint16_t index, atl_entity_cb_t cb)
{
  char cmgd[64] = {0};
  snprintf(cmgd, sizeof(cmgd), "%sAT+CMGD=%d%s", ATL_CMD_SAVE, index, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM(cmgd, NULL, NULL, 1, 15, 0, 0, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), atl_mdl_sms_msg_cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_indicate(uint8_t mode, atl_entity_cb_t cb)
{
  char cnmi[64] = {0};
  snprintf(cnmi, sizeof(cmgd), "%sAT+CNMI=%d%s", ATL_CMD_SAVE, index, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM(cnmi, NULL, NULL, 1, 15, 0, 0, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), atl_mdl_sms_msg_cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
void atl_mdl_sms_urc_cb(ringslice_t urc_slice, atl_mdl_sms_msg_cb_t cb)
{
  uint16_t index = 0;
  ringclise_scanf(urc_slice, "+CMTI:%*[^,],%d\x0d", &index);
  if(0 != index) atl_mdl_sms_read(index, cb);
}
