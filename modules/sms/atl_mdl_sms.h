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
#ifndef __ATL_MDL_SMS_H
#define __ATL_MDL_SMS_H

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
typedef void (*atl_mdl_sms_msg_cb_t)(bool result, atl_mdl_sms_msg_t* atl_mdl_sms_msg);

typedef struct atl_mdl_sms_msg_t {
  char num[64];  
  char msg[161];     
} atl_mdl_sms_msg_t;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Function init GPRS
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_format_set(uint8_t format, atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function init GPRS
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_sc_set(char* num, atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_send_text(char* num, char* msg, atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_read(uint16_t index, atl_mdl_sms_msg_cb_t cb);

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_delete(uint16_t index, atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_indicate(uint8_t mode, atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
void atl_mdl_sms_urc_cb(ringslice_t urc_slice, atl_mdl_sms_msg_cb_t cb);

#endif // __ATL_MDL_SMS_H