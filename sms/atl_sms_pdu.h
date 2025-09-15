/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)26.05.2025 *
 *               ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──       v1.0.0    *
 *               ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                 *
 *               ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                 *
 *               ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                 *
 *               ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                 *  
 ******************************************************************************/
#ifndef __ATL_SMS_PDU_H
#define __ATL_SMS_PDU_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "../atl.h"

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ATL_MAX_CC_ADDR_LEN          42

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
typedef enum{
  ATL_CONVERT_TO_DIGIT_NORMAL,
  ATL_CONVERT_TO_DIGIT_WILD,
  ATL_CONVERT_TO_DIGIT_EXT,
  ATL_CONVERT_TO_DIGIT_24008
} atl_convert_to_digit_t;

typedef enum{
  ATL_SMS_MTI_DELIVER          = 0x00,
  ATL_SMS_MTI_DELIVER_REPORT   = 0x00,
  ATL_SMS_MTI_SUBMIT           = 0x01,
  ATL_SMS_MTI_SUBMIT_REPORT    = 0x01,
  ATL_SMS_MTI_STATUS_REPORT    = 0x02,
  ATL_SMS_MTI_COMMAND          = 0x02,
  ATL_SMS_MTI_UNSPECIFIED      = 0x03,  /* MMI shall handle this case, * eg, displaying "does not support",  * or "cannot display", etc */
  ATL_SMS_MTI_ILLEGAL          = 0x04
} atl_sms_mti_t;

typedef enum{
  ATL_SMS_CLASS0 = 0,
  ATL_SMS_CLASS1,
  ATL_SMS_CLASS2,
  ATL_SMS_CLASS3,
  ATL_SMS_CLASS_UNSPECIFIED,
  ATL_SMS_MW_DISCARD, /* follows are for SMS internal use */
  ATL_SMS_MW_STORE,
  ATL_SMS_RCM,
  ATL_NUM_OF_NMI_MSG_ENUM
} atl_sms_msg_class_t;

typedef enum{
  ATL_SMS_GSM7_BIT = 0,    
  ATL_SMS_EIGHT_BIT,
  ATL_SMS_UCS2,       
  ATL_SMS_ALPHABET_UNSPECIFIED       
} atl_sms_alphabet_t;

typedef enum{
  ATL_SMS_MW_VM = 0,
  ATL_SMS_MW_FAX,
  ATL_SMS_MW_EMAIL, 
  ATL_SMS_MW_OTHER,
  #ifdef __REL6__
   ATL_SMS_MW_VIDEO_MSG,       
  #endif
  ATL_NUM_OF_MSG_WAITING_TYPE
} atl_sms_msg_waiting_type_t;

typedef enum{
   ATL_SMS_MSG_WAIT_CPHS  = 0x01 ,
   ATL_SMS_MSG_WAIT_DCS   = 0x02 ,
   ATL_SMS_MSG_WAIT_UDH       = 0x04 ,
   ATL_SMS_MSG_WAIT_UDH_EVM   = 0x08
} atl_sms_type_of_msg_waiting_t;

typedef struct{
  uint8_t addr_length;
  uint8_t addr_bcd[11];
} atl_sms_addr_t;

typedef struct{
  uint8_t type;
  uint8_t length;
  uint8_t number[ATL_MAX_CC_ADDR_LEN];
} atl_l4c_number_t;

typedef struct atl_rtc_time_t{
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} atl_rtc_time_t;

typedef struct atl_rtc_date_t{
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
} atl_rtc_date_t;

typedef struct{
  uint8_t reply_flag;      /* whether reply is sought*/
  uint8_t udh_p;           /* indicates presence of user data header*/
  uint8_t status_rep_flag; /* whether status reports are sought*/
  uint8_t fill_bits;       /* to be ignored*/
  uint8_t mms;             /* more message to send*/
  uint8_t msg_type;        /* sms deliver*/
  uint8_t no_orig_addr;
  uint8_t orig_addr_size;
  uint8_t *orig_addr;      /* assumed to be byte-packed*/
  uint8_t pid;             /* to be bit-wise decoded*/
  uint8_t dcs;             /* to be bit-wise decoded*/
  uint8_t scts[7];
  uint8_t user_data_len;
  uint8_t no_user_data;
  uint8_t user_data_size;
  uint8_t *user_data;      /* can have user data header also*/
} atl_sms_deliver_peer_t;

typedef struct {
  uint8_t reply_flag;          /* whether reply is sought*/
  uint8_t udh_p;               /* indicates presence of user data header*/
  uint8_t status_rep_flag;     /* whether status reports are sought*/
  uint8_t vp_flag;             /* indicates presence of validity period*/
  uint8_t rej_dup_flag;
  uint8_t msg_type;            /* sms submit*/
  uint8_t msg_ref;             /* to be read from SIM*/
  uint8_t no_dest_addr;
  uint8_t dest_addr_size;
  uint8_t *dest_addr;          /* assumed to be byte-packed*/
  uint8_t pid;                 /* to be bit-wise encoded*/
  uint8_t dcs;                 /* to be bit-wise encoded*/
  uint8_t no_validity_period;
  uint8_t validity_period_size;
  uint8_t *validity_period;    /* to be bit/byte-wise encoded, cannot be even represnted using CHOICE*/
  uint8_t user_data_len;
  uint8_t no_user_data;
  uint8_t user_data_size;
  uint8_t *user_data;          /* can have user data header also*/
} atl_sms_submit_peer_t;

typedef struct{
  uint8_t fill_bits1;      /* to be ignored*/
  uint8_t udh_p;           /* indicates presence of user data header*/
  uint8_t status_rep_type; /* sms command or sms submit*/
  uint8_t fill_bits2;      /* to be ignored*/
  uint8_t mms;
  uint8_t msg_type;        /* sms status report*/
  uint8_t msg_ref;
  uint8_t no_rcpnt_addr;
  uint8_t rcpnt_addr_size;
  uint8_t *rcpnt_addr;
  uint8_t scts[7];
  uint8_t dis_time[7];
  uint8_t status;         /* actual status of submit*/
  uint8_t params_p;       /* depicts presence of optional parameters*/
  uint8_t pid;            /* to be bit-wise decoded*/
  uint8_t dcs;            /* to be bit-wise decoded*/
  uint8_t user_data_len;
  uint8_t no_user_data;
  uint8_t user_data_size;
  uint8_t *user_data;     /* can have user data header also*/
} atl_sms_status_report_peer_t;

typedef struct {
  uint16_t ref;      /* concat. message reference*/
  uint8_t total_seg; /* total segments*/
  uint8_t seg;       /* indicate which segment*/
} atl_sms_concatl_t;

typedef struct{
//kal_uint8 waiting_ind;
  uint8_t waiting_num[ATL_NUM_OF_MSG_WAITING_TYPE];
} atl_sms_mwis_t;

typedef struct{
  uint8_t type_of_info;
  bool need_store;
  bool is_msg_wait;
  bool is_show_num[ATL_NUM_OF_MSG_WAITING_TYPE];
  bool is_clear[ATL_NUM_OF_MSG_WAITING_TYPE];
  bool ind_flag[ATL_NUM_OF_MSG_WAITING_TYPE];
  atl_sms_mwis_t mwis;    
  uint8_t msp;
  uint8_t line_no;
} atl_sms_msg_waiting_t;

typedef struct{
  uint32_t dest_port; /* -1: invalid port */
  uint32_t src_port;
} atl_sms_port_t;

typedef struct{
  union{
    atl_sms_deliver_peer_t deliver_tpdu;
    atl_sms_submit_peer_t submit_tpdu;
    atl_sms_status_report_peer_t report_tpdu;
  } data;
  atl_sms_concatl_t concatl_info;
  atl_sms_mti_t mti;      
  uint8_t fo;       /* first octet */
  uint8_t offset;   /* offset to message content */
  uint8_t msg_len;  /* length of user data */   
  uint8_t udhl;     /* for calculating offset to unpack */
  /* for decoding DCS */   
  atl_sms_msg_class_t msg_class;    
  atl_sms_alphabet_t alphabet_type;
  bool is_compress;
  atl_sms_msg_waiting_t msg_wait;
  atl_sms_port_t port;
} atl_sms_tpdu_decode_t;

typedef struct{
   atl_sms_addr_t sca;
   atl_sms_tpdu_decode_t tpdu; 
   uint8_t pdu_len;   /* length of PDU */
   uint8_t tpdu_len;  /* length of TPDU */
} atl_sms_pdu_decode_t;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Function to decode sms pdu data
 ** \param  data - ptr to pdu data
 ** \retval bool
 ******************************************************************************/
extern bool atl_sms_decode_pdu(uint8_t* data);

#endif
