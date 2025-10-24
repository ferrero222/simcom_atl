/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)03.10.2025 *
 *               ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──       v1.0.0    *
 *               ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                 *
 *               ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                 *
 *               ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                 *
 *               ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                 *  
 ******************************************************************************/
#ifndef __ATL_MDL_TCP_H
#define __ATL_MDL_TCP_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
/*******************************************************************************
 * Config
 ******************************************************************************/
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
typedef struct atl_mdl_tcp_server_t {
  char mode[4]; 
  char ip[256]; 
  char port[6]; 
  char* data; //ptr to your data to send (string)
} atl_mdl_tcp_server_t;

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Function init GPRS
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_init(const atl_entity_cb_t cb, const void* const param, void* const ctx);

/*******************************************************************************
 ** @brief  Function to create and config socket. (SINGLE SOCKET)
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_config(const atl_entity_cb_t cb, const void* const param, void* const ctx);

/*******************************************************************************
 ** @brief  Function to connect socket.
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_tcp_server_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_connect(const atl_entity_cb_t cb, const void* const param, void* const ctx);

/*******************************************************************************
 ** @brief  Function to connect socket.
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_tcp_server_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_send_recieve(const atl_entity_cb_t cb, const void* const param, void* const ctx);

/*******************************************************************************
 ** @brief  Function to disconnect from socket
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_disconnect(const atl_entity_cb_t cb, const void* const param, void* const ctx);

/*******************************************************************************
 ** @brief  Function to deinit gprs
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_deinit(const atl_entity_cb_t cb, const void* const param, void* const ctx);

#endif //__ATL_MDL_TCP_H