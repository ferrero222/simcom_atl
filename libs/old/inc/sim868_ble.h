/*******************************************************************************
 *                                 v1.0                                        *
 *                           GSM main procedure                                *
 ******************************************************************************/
#ifndef __SIM868_BLE_H
#define __SIM868_BLE_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
  ** @brief server params
**/ 
typedef struct sim_ble_server_info_struct
{
  char server_index; 
  char user_id[32]; 
} sim_ble_server_info_struct;

/**
  ** @brief client params
**/ 
typedef struct sim_ble_client_info_struct
{
  char client_index; 
  char user_id[32]; 
} sim_ble_client_info_struct;

/**
  ** @brief Struct for GSM timers
**/ 
typedef struct sim_ble_timers_struct 
{
  uint32_t scann_timer;
  uint8_t scann_timer_flag;
} sim_ble_timers_struct;

/**
  ** @brief advertising parameters
**/ 
typedef struct sim_ble_advert_param_struct
{
  uint8_t scan_rsp;            // Include flag paramater or not (0/1)
  uint8_t bt_name;             // Include ble name (0/1)
  uint8_t tx_power;            // Include Tx pwoer level (0/1)
  uint16_t appearance;         // Set appearance (0~16384)
  char manufactured_data[56];  // Set manufacturer, A Hex value string, each char of it should in set { �0�~�9�,�a�~�f�,�A�~�F�}.Max length of it is 56.
  char service_data[32];       // Set service_data uuid, A Hex value string, each char of it should in set { �0�~�9�,�a�~�f�,�A�~�F�}.The length of it should be 0 or 4~32.
  char service_uuid[32];       // Set complete services uuid, A Hex value string, each char of it should in set { �0�~�9�,�a�~�f�,�A�~�F�}. The length of it should be 0 or 4~32.
} sim_ble_advert_param_struct;
  
extern sim_ble_timers_struct sim_ble_timers;

uint8_t sim_ble_init(void);
uint8_t sim_ble_deinit(void);
uint8_t sim_ble_server_init_and_advert(sim_ble_advert_param_struct* advert_param);
uint8_t sim_ble_client_init_and_scan(char* scanned, uint16_t buf_size, uint16_t time_for_scan);

#endif //__SIM868_BLE_H