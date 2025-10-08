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
#include "atl_core_impl.h"
#include "atl_core_scheduler.h"
#include "atl_mdl_general_impl.h"
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
 ** \brief  None
 ** \param  None
 ** \retval None
 ******************************************************************************/
bool atl_mdl_modem_reset(atl_entity_cb_t cb)
{
  bool result = false;
  atl_item_t items[] = 
  {
     ATL_ITEM("AT+CFUN=1,1"ATL_CMD_CRLF, //[REQ]
               NULL, NULL,               //[PREFIX][CALLBACK]
               2, 15,                    //[REPS][WAIT]
               0, 0),                    //[STEP_ERR][STEP_OK]
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  None
 ** \param  None
 ** \retval None
 ******************************************************************************/
bool atl_mdl_modem_init(atl_entity_cb_t cb)
{
  bool result = false;
  atl_item_t items[] = 
  {
    ATL_ITEM("AT"ATL_CMD_CRLF,   //[REQ]
             NULL, NULL,         //[PREFIX][CALLBACK]
             2, 15,              //[REPS][WAIT]
             0, 1),              //[STEP_ERR][STEP_OK]
    //---------------------------//
    ATL_ITEM("AT"ATL_CMD_CRLF,   //[REQ]
             NULL, NULL,         //[PREFIX][CALLBACK]
             2, 15,              //[REPS][WAIT]
             0, 1),              //[STEP_ERR][STEP_OK]
    //---------------------------//
    ATL_ITEM("ATE1"ATL_CMD_CRLF, //[REQ]
             NULL, NULL,         //[PREFIX][CALLBACK]
             2, 60,              //[REPS][WAIT]
             0, 0),              //[STEP_ERR][STEP_OK]
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  None
 ** \param  None
 ** \retval None
 ******************************************************************************/
static atl_mdl_rtd_t* atl_rtd = NULL;
static atl_rtd_cb_t atl_mdl_rtd_user_cb = NULL;

bool atl_mdl_rtd(atl_rtd_cb_t cb)
{
  DBC_REQUIRE(300, atl_user_init.init);
  bool result = false;
  atl_rtd = tlsf_malloc(atl_user_init.atl_tlsf, atl_mdl_rtd_t);
  if(!atl_rtd) 
  {
    ATL_DEBUG("[ERROR] Failed to allocate memory for %d items\n", item_amount); 
    atl_deinit(); 
    return false; 
  } 
  atl_item_t items[] = 
  {
    ATL_ITEM_EXT("AT+GSN"ATL_CMD_CRLF,                //[REQ]
                 NULL, "%15[^\x0d]",                  //[PREFIX][FORMAT]
                 2, 15,                               //[REPS][WAIT]
                 0, 1,                                //[STEP_ERR][STEP_OK]
                 atl_rtd->modem_imei),                //[ARGS]
    //------------------------------------------------//
    ATL_ITEM_EXT("AT+GMM"ATL_CMD_CRLF,                //[REQ]
                 NULL, "%15[^\x0d]",                  //[PREFIX][FORMAT]
                 2, 15,                               //[REPS][WAIT]
                 0, 1,                                //[STEP_ERR][STEP_OK]
                 atl_rtd->modem_id),                  //[ARGS]
    //------------------------------------------------//
    ATL_ITEM_EXT("AT+GMR"ATL_CMD_CRLF,                //[REQ]
                 NULL, "Revision:%29[^\x0d]",         //[PREFIX][FORMAT]
                 2, 15,                               //[REPS][WAIT]
                 0, 1,                                //[STEP_ERR][STEP_OK]
                 atl_rtd->modem_rev),                 //[ARGS]
    //------------------------------------------------//
    ATL_ITEM_EXT("AT+CCLK?"ATL_CMD_CRLF,              //[REQ]
                 NULL, "+CCLK: \"%21[^\"]\"",         //[PREFIX][FORMAT]
                 2, 15,                               //[REPS][WAIT]
                 0, 1,                                //[STEP_ERR][STEP_OK]
                 atl_rtd->modem_clock),               //[ARGS]
    //------------------------------------------------//
    ATL_ITEM_EXT("AT+CCID"ATL_CMD_CRLF,               //[REQ]
                 "+CCLK", "%21[^\x0d]",               //[PREFIX][FORMAT]
                 2, 15,                               //[REPS][WAIT]
                 0, 1,                                //[STEP_ERR][STEP_OK]
                 atl_rtd->sim_iccid),                 //[ARGS]    
    //------------------------------------------------//
    ATL_ITEM_EXT("AT+COPS?"ATL_CMD_CRLF,              //[REQ]
                 "+COPS", "+COPS: 0, 0,\"%49[^\"]\"", //[PREFIX][FORMAT]
                 2, 15,                               //[REPS][WAIT]
                 0, 1,                                //[STEP_ERR][STEP_OK]
                 atl_rtd->sim_operator),              //[ARGS]  
    //------------------------------------------------//
    ATL_ITEM_EXT("AT+CSQ"ATL_CMD_CRLF,                //[REQ]
                 "+CSQ", "+CSQ: %d",                  //[PREFIX][FORMAT]
                 2, 15,                               //[REPS][WAIT]
                 0, 1,                                //[STEP_ERR][STEP_OK]
                 atl_rtd->sim_rssi),                  //[ARGS]  
    //------------------------------------------------//
    ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,                 //[REQ]
             NULL, NULL,                              //[PREFIX][CALLBACK]
             2, 60,                                   //[REPS][WAIT]
             0, 1),                                   //[STEP_ERR][STEP_OK]        
    //------------------------------------------------//
    ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,                  //[REQ]
             NULL, atl_mdl_general_ceng_cb,           //[PREFIX][CALLBACK]
             2, 60,                                   //[REPS][WAIT]
             0, 0),                                   //[STEP_ERR][STEP_OK]                   
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), atl_mdl_rtd_cb)) return false;
  atl_mdl_rtd_user_cb = cb;
  return true;
}

/**
 *  \brief ceng cb
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
  if(sscanf((char*)data, "+CENG: %d,\"%d,%d,%x,%x,", &idx, &mcc,&mnc,&lac,&cell_id))
  {
    modem_rtd_ref->modem_lbs[modem_rtd_ref->lbs_cnt].cell = idx;
    modem_rtd_ref->modem_lbs[modem_rtd_ref->lbs_cnt].mcc = mcc;
    modem_rtd_ref->modem_lbs[modem_rtd_ref->lbs_cnt].mnc = mnc;
    modem_rtd_ref->modem_lbs[modem_rtd_ref->lbs_cnt].lac = lac;
    modem_rtd_ref->modem_lbs[modem_rtd_ref->lbs_cnt].cell_id = cell_id;
    if(modem_rtd_ref->modem_lbs[modem_rtd_ref->lbs_cnt].cell_id == 0 || modem_rtd_ref->modem_lbs[modem_rtd_ref->lbs_cnt].lac == 0) break;
    ++modem_rtd_ref->lbs_cnt;
  };
}