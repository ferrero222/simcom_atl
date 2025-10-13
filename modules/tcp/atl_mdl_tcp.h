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
 ** \brief  Function init GPRS
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_init(atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_config(atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to connect socket. Use @atl_mdl_gprs_socket_connect_vla
 ** \param  mode  TCP/UDP. 
 ** \param  ip    ip of server. 
 ** \param  port  port. 
 ** \param  cb    cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
   //atl_mdl_gprs_socket_connect_vla(_mode, _ip, _port, _cb)
bool atl_mdl_gprs_socket_connect(char* cipstart, atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to connect socket. Use @atl_mdl_gprs_socket_send_recieve_vla
 ** \param  data  data to send. 
 ** \param  cb    cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
   //atl_mdl_gprs_socket_send_recieve_vla(_data, _cb) 
bool atl_mdl_gprs_socket_send_recieve(char* cipsend, char* data, atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to disconnect from socket
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_disconnect(atl_entity_cb_t cb);

/*******************************************************************************
 ** \brief  Function to deinit gprs
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_deinit(atl_entity_cb_t cb);