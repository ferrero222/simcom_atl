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
#include "atl_core_scheduler.h"
#include "atl_mdl_general.h"
#include "dbc_assert.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_MDL_GENERAL");

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
 ** \brief  Function to restart the modem
 ** \param  cb  cb when proc will be done
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_modem_reset(atl_entity_cb_t cb)
{
  bool result = false;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CFUN=1,1"ATL_CMD_CRLF, NULL, NULL, 2, 15, 0, 1, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to init modem
 ** \param  cb  cb when proc will be done
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_modem_init(atl_entity_cb_t cb)
{
  bool result = false;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT"ATL_CMD_CRLF,   NULL, NULL, 2, 15, 0, 1, NULL),
    ATL_ITEM("AT"ATL_CMD_CRLF,   NULL, NULL, 2, 15, 0, 1, NULL),  
    ATL_ITEM("ATE1"ATL_CMD_CRLF, NULL, NULL, 2, 60, 0, 0, NULL),  
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to get real time data info about modem. This function
 **         creates an instance of @atl_mdl_rtd_t in atl heap and then pass
 **         that instance to your callback. When callback execution will be done 
 **         instance will be free, so if you need that for longer time just
 **         do copy.
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
static atl_mdl_rtd_t* atl_rtd = NULL;
static atl_rtd_cb_t atl_mdl_rtd_user_cb = NULL;

bool atl_mdl_rtd(atl_rtd_cb_t cb)
{
  DBC_REQUIRE(101, atl_is_init());
  bool result = false;
  atl_rtd = tlsf_malloc(atl_user_init.atl_tlsf, atl_mdl_rtd_t);
  if(!atl_rtd) 
  {
    ATL_DEBUG("[ERROR] Failed to allocate memory for %d items\n", item_amount); 
    atl_deinit(); 
    return false; 
  } 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {   
    ATL_ITEM("AT+GSN"ATL_CMD_CRLF,       NULL,               "%15[^\x0d]", 2, 15,  0, 1, NULL, atl_rtd->modem_imei),
    ATL_ITEM("AT+GMM"ATL_CMD_CRLF,       NULL,               "%15[^\x0d]", 2, 15, 0, 1, NULL, atl_rtd->modem_id),  
    ATL_ITEM("AT+GMR"ATL_CMD_CRLF,       NULL,      "Revision:%29[^\x0d]", 2, 15, 0, 1, NULL, atl_rtd->modem_rev),                
    ATL_ITEM("AT+CCLK?"ATL_CMD_CRLF,     NULL,      "+CCLK: \"%21[^\"]\"", 2, 15, 0, 1, NULL, atl_rtd->modem_clock          
    ATL_ITEM("AT+CCID"ATL_CMD_CRLF,   "+CCLK",               "%21[^\x0d]", 2, 15, 0, 1, NULL, atl_rtd->sim_iccid),             
    ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,  "+COPS", "+COPS: 0, 0,\"%49[^\"]\"", 2, 15, 0, 1, NULL, atl_rtd->sim_operator),            
    ATL_ITEM("AT+CSQ"ATL_CMD_CRLF,     "+CSQ",                 "+CSQ: %d", 2, 15, 0, 1, NULL, atl_rtd->sim_rssi),             
    ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,    NULL,                       NULL, 2, 60, 0, 1, NULL),                
    ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,     NULL,                       NULL, 2, 60, 0, 0, atl_mdl_general_ceng_cb),                         
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), atl_mdl_rtd_cb)) return false;
  atl_mdl_rtd_user_cb = cb;
  return true;
}

/**
 *  \brief rtd cb
 */
void atl_mdl_rtd_cb(bool result)
{
  atl_mdl_rtd_user_cb(result, atl_rtd);
  tlsf_free(atl_user_init.atl_tlsf, atl_rtd);
}

/**
 *  \brief ceng cb
 */
void atl_mdl_general_ceng_cb(const ringslice_t rs_data)
{
  while(ringslice_scanf(rs_data, "+CENG: %d,\"%d,%d,%x,%x,", 
                        &atl_rtd->modem_lbs[atl_rtd->lbs_cnt].cell, &atl_rtd->modem_lbs[atl_rtd->lbs_cnt].mcc,
                        &atl_rtd->modem_lbs[atl_rtd->lbs_cnt].mnc, &atl_rtd->modem_lbs[atl_rtd->lbs_cnt].lac,
                        &atl_rtd->modem_lbs[atl_rtd->lbs_cnt].cell_id))
  {
    if(atl_rtd->modem_lbs[atl_rtd->lbs_cnt].cell_id != 0 && atl_rtd->modem_lbs[modem_rtd_ref->lbs_cnt].lac != 0) ++atl_rtd->lbs_cnt;;
    rs_data = ringslice_subslice(rs_data, strlen("+CENG:", 0));
  };
}