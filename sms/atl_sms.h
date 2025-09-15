/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)26.05.2025 *
 *               ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──       v1.0.0    *
 *               ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                 *
 *               ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                 *
 *               ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                 *
 *               ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                 *  
 ******************************************************************************/
#ifndef __ATL_SMS_H
#define __ATL_SMS_H   
   
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "../atl.h"

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ATL_SMS_MAX_TPDU_SIZE (175)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** \brief  URC "Sms ready" callback handler for modem ready to sms.
 ** \param  p_rsp_str - ptr to data string of urc
 ** \param  len       - len of data
 ** \retval None
 ** \note   Use this function to catch "Sms ready" URC. Use @atl_urc_queue_append()
 **         at the runtime or @atl_urc_entity_queue at the start of programm to
 **         proc this func. You can add your custom code here.
 ******************************************************************************/
extern void atl_urc_sms_ready_cb(uint8_t* p_rsp_str, uint16_t len);

/*******************************************************************************
 ** \brief  URC "+CMTI" callback handler for new incoming sms
 ** \param  p_rsp_str - ptr to data string of urc
 ** \param  len       - len of data
 ** \retval None
 ** \note   Use this function to catch "+CMTI" URC. Use @atl_urc_queue_append()
 **         at the runtime or @atl_urc_entity_queue at the start of programm to
 **         proc this func. You can add your custom code here.
 **         ATTENTION: this handle using @atl_sms_msg_read() to immediately read
 **         sms msg, if you dont want to or you want to get callback after this
 **         read msg, edit this code.
 ******************************************************************************/
extern void atl_urc_new_sms_ind_cb(uint8_t* p_rsp_str, uint16_t len);

/*******************************************************************************
 ** \brief  URC "+CMT" callback handler for new incoming sms with data parse in pdu
 ** \param  p_rsp_str - ptr to data string of urc
 ** \param  len       - len of data
 ** \retval None
 ** \note   Use this function to catch "+CMT" URC. Use @atl_urc_queue_append()
 **         at the runtime or @atl_urc_entity_queue at the start of programm to
 **         proc this func. You can add your custom code here.
 **         ATTENTION: This function also parce incoming URC data, you can edit 
 **         this code to get some of this data if you need them.
 ******************************************************************************/
extern void atl_urc_flash_new_sms_cb(uint8_t* p_rsp_str, uint16_t len);

/*******************************************************************************
 ** \brief  Function to set PDU or TEXT mode for sms
 ** \param  n_format_type - 0 - PDU, 1 - TEXT.
 ** \param  p_result_cb   - callback for the end of this process.
 ** \retval bool
 ******************************************************************************/
extern bool atl_sms_formatl_set(const uint8_t n_format_type, const atl_cmd_fun_rsp_cb p_result_cb);

/*******************************************************************************
 ** \brief  Function to set PDU or TEXT mode for sms
 ** \param  number      - RP SC address.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func
 ******************************************************************************/
extern bool atl_sms_sc_set(const char* number, const atl_cmd_fun_rsp_cb p_result_cb);

/*******************************************************************************
 ** \brief  Function to send sms in TEXT mode
 ** \param  number      - phone number where sms will be send
 ** \param  msg         - sms message.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func
 ******************************************************************************/
extern bool atl_sms_text_send(const char* number, const char* msg, const atl_cmd_fun_rsp_cb p_result_cb);

/*******************************************************************************
 ** \brief  Function to send sms in PDU mode
 ** \param  ptr_pdu     - ready-done pdu data.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func.
 ******************************************************************************/
extern bool atl_sms_pdu_send(const char* ptr_pdu, const atl_cmd_fun_rsp_cb p_result_cb);

/*******************************************************************************
 ** \brief  Function to read sms message
 ** \param  index       - index of message.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func.
 **         ATTENTION: This function puts read msg data into @atl_sms_cmgr_data
 **         use @atl_sms_cmgr_data_get() to get this data once.
 ******************************************************************************/
extern bool atl_sms_msg_read(const uint16_t index, const atl_cmd_fun_rsp_cb p_result_cb);

/*******************************************************************************
 ** \brief  Function to delete sms message
 ** \param  index       - index of message.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func.
 ******************************************************************************/
extern bool atl_sms_msg_delete(const uint16_t index, const atl_cmd_fun_rsp_cb p_result_cb);

/*******************************************************************************
 ** \brief  Function to set new SMS message inddication
 ** \param  mode - bfr  - check AT manual of CNMI
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ******************************************************************************/
extern bool atl_sms_cnmi_set(const uint8_t mode, const uint8_t mt, const uint8_t bm, const uint8_t ds, const uint8_t bfr, const atl_cmd_fun_rsp_cb p_result_cb);

#endif //__ATL_SMS_H
