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
typedef struct atl_mdl_sms_msg_t {
  uint8_t format;
  char num[64];  
  char msg[161];     
  uint16_t index;
  uint8_t mode;
} atl_mdl_sms_msg_t;

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
bool atl_mdl_sms_format_set(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta);

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
bool atl_mdl_sms_sc_set(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta);

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
bool atl_mdl_sms_send_text(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta);

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
bool atl_mdl_sms_read(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta);

/*******************************************************************************
 ** @brief  Function to delete SMS
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_sms_msg_t
 **                Should exist only when this function is executing
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_sms_delete(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta);

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
bool atl_mdl_sms_indicate(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta);

#endif // __ATL_MDL_SMS_H