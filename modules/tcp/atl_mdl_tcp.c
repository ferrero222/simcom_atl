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
#include "atl_mdl_tcp.h"
#include "dbc_assert.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_MDL_TCP");

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
 ** \brief  Function to get real time data info about modem. This function
 **         creates an instance of @atl_mdl_rtd_t in atl heap and then pass
 **         that instance to your callback. When callback execution will be done 
 **         instance will be free, so if you need that for longer time just
 **         do copy.
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_modem_init(atl_entity_cb_t cb)
{
  bool result = false;
  atl_item_t items[] = 
  {
    ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,        //[REQ]
             "+COPS: 0", NULL,              //[PREFIX][CALLBACK]
             2, 15,                         //[REPS][WAIT]
             0, 1),                         //[STEP_ERR][STEP_OK]
    //--------------------------------------//
    ATL_ITEM("AT+COPS=0"ATL_CMD_CRLF,       //[REQ]
             NULL, NULL,                    //[PREFIX][CALLBACK]
             2, 15,                         //[REPS][WAIT]
             0, 1),                         //[STEP_ERR][STEP_OK]
    //--------------------------------------//
    ATL_ITEM("AT+CREG?"ATL_CMD_CRLF,        //[REQ]
             "+CREG: 0,1|+CREG: 0,5", NULL, //[PREFIX][CALLBACK]
             5, 60,                         //[REPS][WAIT]
             0, 0),                         //[STEP_ERR][STEP_OK]
    //--------------------------------------//
    ATL_ITEM("AT+CGATT?"ATL_CMD_CRLF,       //[REQ]
             "+CGATT: 1", NULL,             //[PREFIX][CALLBACK]
             10, 60,                        //[REPS][WAIT]
             0, 0),                         //[STEP_ERR][STEP_OK]   
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}
