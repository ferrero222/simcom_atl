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
#include "atl_sms.h"
#include "atl_sms_cmd.h"
#include "atl_sms_pdu.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ATL_MAX_DIGITS_USSD              (183)

#define ATL_SMS_ADDR_LEN                  (11)
#define ATL_SMS_INVALID_PORT_NUM          (-1)
#define ATL_SMS_ATL_MAX_REG_PORT_NUM      (10)
#define ATL_SMS_MTI_BITS                (0x03)
#define ATL_SMS_ONE_MSG_OCTET            (140)
#define ATL_PART_OF_DELIVER_HEADER_LEN    (13)

#define ATL_SMS_CONC8_MSG_IEI           (0x00)
#define ATL_SMS_CONC16_MSG_IEI          (0x08)
#define ATL_SMS_SPECIAL_MSG_IND_IEI     (0x01)
#define ATL_SMS_ENHANCED_VM_INFO_IEI    (0x23)

/* EMS */
#define ATL_SMS_EMS_TEXT_FORMATL_IEI    (0x0a)   /* Text Formating */
#define ATL_SMS_EMS_PREDEF_SND_IEI      (0x0b)   /* Predefined Sound */
#define ATL_SMS_EMS_USER_DEF_SND_IEI    (0x0c)   /* User Defined Sound */
#define ATL_SMS_EMS_PREDEF_ANM_IEI      (0x0d)   /* Predefined Animation */
#define ATL_SMS_EMS_LARGE_ANM_IEI       (0x0e)   /* Large Animation */
#define ATL_SMS_EMS_SMALL_ANM_IEI       (0x0f)   /* Small Animation */
#define ATL_SMS_EMS_LARGE_PIC_IEI       (0x10)   /* Large Picture */
#define ATL_SMS_EMS_SMALL_PIC_IEI       (0x11)   /* Small Picture */
#define ATL_SMS_EMS_VAR_PIC_IEI         (0x12)   /* Variable Picture */

/* MISC */
#define ATL_SMS_APP_PORT8_IEI           (0x04)   /* application port - 8 bit */
#define ATL_SMS_APP_PORT16_IEI          (0x05)   /* application port - 16 bit */
#define ATL_SMS_NL_SINGLE_IEI           (0x24)   /* National Language single shift */
#define ATL_SMS_NL_LOCKING_IEI          (0x25)   /* National Language looking shift */
#define ATL_SMS_CPHS_VM_ADDR_TYPE       (0xd0)

#define ATL_MASK(_w) ( ((_w)>31) ? 0xffffffff : (0x1u << (_w)) - 1 )

/* _uc is the pointer to the byte stream from which bits to be read */
/* _s is the offset in bits from MSB of the byte stream _uc  */
/* _w is the number of bits to be read  */

#define ATL_GET_BITS_1_8(_uc, _s, _w) (        \
   (((_s)+(_w)>8) ? *(_uc) << ((_s)+(_w)-8) |  \
    *((_uc)+1) >> (16-(_s)-(_w)) :             \
     *(_uc) << (_s) >> (8-(_w)))               \
    & ATL_MASK(_w) )

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
 typedef struct atl_sms_contact_data_t
{
    uint8_t use_data[ATL_SMS_MAX_TPDU_SIZE*2+1];
    uint8_t segement;
    uint8_t use_len;
}atl_sms_contact_data_t;

typedef struct atl_sms_contact_info_array_t
{
    atl_sms_contact_data_t sms_contact_data_info[10];
    uint8_t total;
    uint8_t count;
}atl_sms_contact_info_array_t;

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static atl_sms_contact_info_array_t atl_sms_contact_data = {0};

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** \brief  This function converts TP address to L4 address.
 ** \param  tpdu_addr, raw data of address in TPDU. l4_addr, used as return
 **         length indicates number of bytes in addr_bcd
 ** \retval None
 ** \note
 **     tpdu_addr = {0x0b, 0x91, 0x88, 0x96, 0x93, 0x24, 0x42, 0xf1 }
 **                  len , type,  
 **      tp_addr  = 0x0b  { 0x91, 0x88, 0x96, 0x93, 0x24, 0x42, 0xf1, 0xff }
 ******************************************************************************/
static void atl_sms_tpdu_addr2_sms_tp_addr(uint8_t *tpdu_addr, atl_sms_addr_t *tp_addr)
{
  tp_addr->addr_length = tpdu_addr[0];
  tp_addr->addr_bcd[0] = tpdu_addr[1];
  if (tpdu_addr[0] > 0) memcpy(&tp_addr->addr_bcd[1], &tpdu_addr[2], (tpdu_addr[0] + 1) / 2);
}

/*******************************************************************************
 ** \brief  This is convert_to_digit function of L4C module. convert BCD string 
 **         to ascii string of numbers
 ** \param  source - input BCD string
 ** \param  dest - Output ascii string
 ** \retval number of bytes of the output ascii string
 ******************************************************************************/
static uint8_t atl_convert_to_general_digit(uint8_t *source, uint8_t *dest, uint8_t type)
{
  uint8_t ch1, ch2;
  uint8_t i = 0, j = 0;
  uint16_t len = ATL_MAX_CC_ADDR_LEN-1;
  if ((source == NULL) || (dest == NULL))
  {
    ATL_DEBUG("atl_convert_to_general_digit failed");    
    return 0;
  }
  /* mtk02514 ***********************************
  * for CONVERT_TO_DIGIT_EXT, 
  * we should use ATL_MAX_DIGITS_USSD as the maximum length
  *********************************************/
  if(type==ATL_CONVERT_TO_DIGIT_EXT) len = ATL_MAX_DIGITS_USSD - 1;
  while((source[i] != 0xff) && j < len)
  {
    ch1 = source[i] & 0x0f;
    ch2 = (source[i] & 0xf0) >> 4;
    /* mtk02514 **************************
    * For CONVERT_TO_DIGIT_24008,
    * we should use get_ch_byte_24008 insead
    * of get_ch_byte
    ***********************************/
    if (type==ATL_CONVERT_TO_DIGIT_24008) *((uint8_t*) dest + j) = atl_get_ch_byte_24008(ch1);
    else *((uint8_t*) dest + j) = atl_get_ch_byte(ch1, type);
    if (ch2 == 0x0f)
    {
      *((uint8_t*) dest + j + 1) = '\0';
        return j + 1;
    }
    else
    {
      if (type==ATL_CONVERT_TO_DIGIT_24008) *((uint8_t*) dest + j + 1) = atl_get_ch_byte_24008(ch2);
      else                                   *((uint8_t*) dest + j + 1) = atl_get_ch_byte(ch2, type);
    }
    i++;
    j += 2;
  }
  *((uint8_t*) dest + j) = '\0';
  return j;
}

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static bool atl_sms_orig_address_convert(const uint8_t* ptr_orig_addr, const uint8_t* ptr_dest_addr)
{
  atl_sms_addr_t dest_addr = {0};
  atl_l4c_number_t addr = {0};
  if((NULL == ptr_orig_addr) || (NULL == ptr_dest_addr )) return false; 
  atl_sms_tpdu_addr2_sms_tp_addr(ptr_orig_addr, &dest_addr);        
  addr.length = atl_convert_to_general_digit(&(dest_addr.addr_bcd[1]), addr.number, ATL_CONVERT_TO_DIGIT_NORMAL);
  addr.type = dest_addr.addr_bcd[0];
  if((addr.type != 0x81) && (addr.type != 0x91) && (addr.type != 0xa1) && (addr.type != 0xb1)) addr.type = 0x81;
  memcpy(ptr_dest_addr,&(addr.number[0]),addr.length);
  return true;
}

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static void atl_sms_reset_tpdu_decode_struct(atl_sms_tpdu_decode_t *tpdu_decode_ptr)
{
  uint8_t i = 0;
  /* Reset TPDU Decode struct */
  tpdu_decode_ptr->offset = 0;
  tpdu_decode_ptr->msg_len = 0;
  tpdu_decode_ptr->udhl = 0;
  /* if sms is not concatenated , set total_seg & seg to 1 */
  tpdu_decode_ptr->concatl_info.total_seg = 1; /* not concat. msg */
  tpdu_decode_ptr->concatl_info.seg = 1;
  tpdu_decode_ptr->concatl_info.ref = 0;
  tpdu_decode_ptr->msg_class = ATL_SMS_CLASS_UNSPECIFIED;
  tpdu_decode_ptr->msg_wait.type_of_info = 0;
  tpdu_decode_ptr->msg_wait.need_store = false;
  tpdu_decode_ptr->msg_wait.is_msg_wait = false;
  for (i = 0; i < ATL_NUM_OF_MSG_WAITING_TYPE; i++)
  {
      tpdu_decode_ptr->msg_wait.is_show_num[i] = false;
      tpdu_decode_ptr->msg_wait.is_clear[i] = false;
      tpdu_decode_ptr->msg_wait.ind_flag[i] = false;
  }
  //tpdu_decode_ptr->msg_wait.mwis = false;
  tpdu_decode_ptr->port.dest_port = ATL_SMS_INVALID_PORT_NUM;
  tpdu_decode_ptr->port.src_port  = ATL_SMS_INVALID_PORT_NUM;
}    

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static void atl_sms_deliver_peer_t_alloc(void *mtk_d)
{
  atl_sms_deliver_peer_t *s = (atl_sms_deliver_peer_t *)mtk_d;
  memset(s, 0, sizeof(atl_sms_deliver_peer_t));
  s->orig_addr_size = 12;
  s->no_orig_addr = 0;
  s->orig_addr = (uint8_t *) malloc (12 * sizeof(uint8_t));
  {
    int i = 0;
    for (i = 0; i < 7; i++)
    {
    }
  }
  s->user_data_size = 140;
  s->no_user_data = 0;
  s->user_data = (uint8_t *) malloc (140 * sizeof(uint8_t));
}

/*******************************************************************************
 ** \brief  This function decodes the data coding scheme.
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static void atl_sms_decode_dcs(uint8_t dcs, 
                               atl_sms_alphabet_t *alphabet_type, 
                               atl_sms_msg_class_t *msg_class,
                               bool *is_compress,
                               atl_sms_msg_waiting_t *msg_wait)
{
  uint8_t coding_group;
  /* Default DCS value */
  *alphabet_type = ATL_SMS_GSM7_BIT;
  *msg_class = ATL_SMS_CLASS_UNSPECIFIED;
  *is_compress = false;
  if (dcs == 0) return;
  coding_group = dcs >> 4;
  if (coding_group == 0x0f)  /* Data Coding/Message Class */
  {
    if ((dcs & 0x08) == 0x08) /* SCR: 2001, ensure the reserved bit is zero*/
    {
      *alphabet_type = ATL_SMS_ALPHABET_UNSPECIFIED;
    }
    else
    {
      *msg_class = (atl_sms_msg_class_t)(dcs & 0x03);
      *alphabet_type = (atl_sms_alphabet_t)((dcs & 0x04) >> 2);
    }
  }
  else if ((coding_group | 0x07) == 0x07) /* General Data Coding Scheme */
  {
    if ((dcs & 0x10) == 0x10) *msg_class = (atl_sms_msg_class_t)(dcs & 0x03);
    else                      *msg_class = ATL_SMS_CLASS_UNSPECIFIED;
    *alphabet_type = (atl_sms_alphabet_t)((dcs >> 2) & 0x03);
    if (((coding_group & 0x02) >> 1) == 1) *is_compress = true;
  }
  else if ((coding_group & 0x0c) == 0x08)
  {
    /*
    * according GSM 23.038 clause 4:
    * "Any reserved codings shall be assumed to be the GSM 7 bit default alphabet."
    */
  }
  else if (((coding_group & 0x0f) == 0x0c)  || /* discard */
            ((coding_group & 0x0f) == 0x0d) || /* store, gsm-7 */
            ((coding_group & 0x0f) == 0x0e))   /* store, ucs2 */
  {
    /* 1110: msg wait ind (store, ucs2) */
    if ((coding_group & 0x0f) == 0x0e) *alphabet_type = ATL_SMS_UCS2;
    if (msg_wait != NULL)
    {
      msg_wait->type_of_info |= ATL_SMS_MSG_WAIT_DCS;
      msg_wait->is_msg_wait = true;
    }
  }
  /*
    * if the reserved bit been set or the alphabet_type uses the reserved one,
    * then according GSM 23.038 clause 4:
    * "Any reserved codings shall be assumed to be the GSM default alphabet."
    * we change it as SMS_GSM7_BIT
    */
  if (*alphabet_type == ATL_SMS_ALPHABET_UNSPECIFIED) *alphabet_type = ATL_SMS_GSM7_BIT;
}                                

/*******************************************************************************
 ** \brief  This function determines the unit of TP-User-Data.
 ** \param  ...
 ** \retval length in octet/length in septet
 ******************************************************************************/
static bool atl_sms_is_len_in8bit(uint8_t dcs)
{
  bool is_compress;
  atl_sms_alphabet_t alphabet_type;
  atl_sms_msg_class_t mclass;
  atl_sms_decode_dcs(dcs, &alphabet_type, &mclass, &is_compress, NULL);
  if ((is_compress == true) || (alphabet_type == ATL_SMS_EIGHT_BIT) || (alphabet_type == ATL_SMS_UCS2)) return true;
  else return false;
}   

/*******************************************************************************
 ** \brief  This function return the number of bytes of TP-User-Data.
 ** \param  ...
 ** \retval length in octets
 ******************************************************************************/
static uint16_t atl_sms_msg_len_in_octet(uint8_t dcs, uint16_t len)
{
  if (atl_sms_is_len_in8bit(dcs) == true) return len;
  else                                      return (len * 7 + 7) / 8;
}                                      

/*******************************************************************************
 ** \brief  This function is used to unpack SMS-SUBMIT.
 ** \param  ...
 ** \retval number of unpacked bits
 ******************************************************************************/
static unsigned int atl_sms_deliver_peer_t_unpack(void *mtk_d, uint8_t * frame, unsigned int bit_offset, unsigned int fr_size, void *err_hndl)
{
  atl_sms_deliver_peer_t *s = (atl_sms_deliver_peer_t*)mtk_d;
  int bits = bit_offset, skip_bits = 0;
  int i = 0;
  s->reply_flag = ATL_GET_BITS_1_8(frame, (bits & 0x07), 1);
  /* frame += (((bits&0x07) + 1) >> 3); */
  bits += 1;
  s->udh_p = ATL_GET_BITS_1_8(frame, (bits & 0x07), 1);
  /* frame += (((bits&0x07) + 1) >> 3); */
  bits += 1;
  s->status_rep_flag = ATL_GET_BITS_1_8(frame, (bits & 0x07), 1);
  /* frame += (((bits&0x07) + 1) >> 3); */
  bits += 1;
  s->fill_bits = ATL_GET_BITS_1_8(frame, (bits & 0x07), 2);
  /* frame += (((bits&0x07) + 2) >> 3); */
  bits += 2;
  s->mms = ATL_GET_BITS_1_8(frame, (bits & 0x07), 1);
  /* frame += (((bits&0x07) + 1) >> 3); */
  bits += 1;
  s->msg_type = ATL_GET_BITS_1_8(frame, (bits & 0x07), 2);
  frame += (((bits & 0x07) + 2) >> 3);
  bits += 2;
  for (i = 0; i < 2; i++) /* unpack length and address type of TP-OA */
  {
    s->orig_addr[i] = ATL_GET_BITS_1_8(frame, (bits & 0x07), 8);
    frame += (((bits & 0x07) + 8) >> 3);
    bits += 8;
  }
  if (s->orig_addr[0] > ((ATL_SMS_ADDR_LEN - 1) * 2)) /* max: 20 digits */
  {
    s->orig_addr[0] = (ATL_SMS_ADDR_LEN - 1) * 2;
  }
  for (i = 0; i < (s->orig_addr[0] + 1) / 2; i++) /* unpack address (BCD number) of TP-OA */
  {
    s->orig_addr[i + 2] = ATL_GET_BITS_1_8(frame, (bits & 0x07), 8);
    frame += (((bits & 0x07) + 8) >> 3);
    bits += 8;
  }
  s->pid = ATL_GET_BITS_1_8(frame, (bits & 0x07), 8);
  frame += (((bits & 0x07) + 8) >> 3);
  bits += 8;
  s->dcs = ATL_GET_BITS_1_8(frame, (bits & 0x07), 8);
  frame += (((bits & 0x07) + 8) >> 3);
  bits += 8;
  for (i = 0; i < 7; i++)
  {
    s->scts[i] = ATL_GET_BITS_1_8(frame, (bits & 0x07), 8);
    frame += (((bits & 0x07) + 8) >> 3);
    bits += 8;
  }
  s->user_data_len = ATL_GET_BITS_1_8(frame, (bits & 0x07), 8);
  frame += (((bits & 0x07) + 8) >> 3);
  bits += 8;
  if (s->user_data_len > 0)
  {
    s->no_user_data = (uint8_t)atl_sms_msg_len_in_octet(s->dcs, s->user_data_len); /* unpack User Data according TP-DCS */
    if (s->no_user_data > ATL_SMS_ONE_MSG_OCTET) s->no_user_data = ATL_SMS_ONE_MSG_OCTET;
    for (i = 0; i < s->no_user_data; i++)
    {
      s->user_data[i] = ATL_GET_BITS_1_8(frame, (bits & 0x07), 8);
      frame += (((bits & 0x07) + 8) >> 3);
      bits += 8;
    }
  }                               
  return bits - bit_offset - skip_bits;
}

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static bool atl_sms_detect_udh(uint8_t* data)
{
  uint8_t udhl = 0;        /* user data header length */
  uint8_t read_byte = 0;   /* already read bytes */
  uint8_t iei;             /* IE identifier */
  uint8_t ie_len;          /* IE length */
  udhl = *data;
  if(udhl < 3 || udhl > ATL_SMS_ONE_MSG_OCTET) return false;
  read_byte += 1;
  while (read_byte <= udhl)
  {
    iei = *(data + read_byte); /* Get IEI */
    ie_len = *(data + read_byte + 1); /* Get IE Length */
    read_byte += 2;
    if (((iei == 2) || iei == 0x23) || ((iei >= 0x26 && iei <=0x6F) || (iei >= 0x80 ))) return false;
    if (ie_len == 0 || ie_len > (udhl + 1 - read_byte)) return false;
    switch (iei)
    {
      case ATL_SMS_NL_SINGLE_IEI:
      case ATL_SMS_NL_LOCKING_IEI:       if(ie_len != 1)                return false; break;                
      case ATL_SMS_SPECIAL_MSG_IND_IEI:
      case ATL_SMS_APP_PORT8_IEI:
      case ATL_SMS_EMS_PREDEF_SND_IEI:
      case ATL_SMS_EMS_PREDEF_ANM_IEI:   if(ie_len != 2)                return false; break;
      case ATL_SMS_CONC8_MSG_IEI:        if(ie_len != 3)                return false; break;
      case ATL_SMS_EMS_TEXT_FORMATL_IEI: if(ie_len != 3 && ie_len != 4) return false; break;
      case ATL_SMS_CONC16_MSG_IEI:
      case ATL_SMS_APP_PORT16_IEI:       if(ie_len != 4)                return false; break;
      case ATL_SMS_EMS_LARGE_PIC_IEI:
      case ATL_SMS_EMS_LARGE_ANM_IEI:    if(ie_len != 129)              return false; break;
      case ATL_SMS_EMS_SMALL_ANM_IEI:
      case ATL_SMS_EMS_SMALL_PIC_IEI:    if(ie_len != 33)               return false; break;
      default: return false;
    }                      
    if((read_byte + ie_len) > ATL_SMS_ONE_MSG_OCTET) return false; /* illegal PDU */
    read_byte += ie_len;
  }                                
  if(read_byte != (udhl + 1)) return false;
  return true;
}    

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static uint8_t atl_sms_decode_udh(uint8_t *data,
                           atl_sms_concatl_t *concatl_info,
                           atl_sms_msg_waiting_t *msg_wait,
                           atl_sms_port_t *port)
{
  uint8_t udhl = 0;       /* user data header length */
  uint8_t read_byte = 0;  /* already read bytes */
  uint8_t iei;            /* IE identifier */
  uint8_t ie_len;         /* IE length */
  #if defined(__REL6__) && defined(__SMS_R6_ENHANCED_VOICE_MAIL__)
    uint8_t ind_num;
  #endif
  if( msg_wait != NULL ) msg_wait->msp = 0; // set default
  if (concatl_info != NULL) concatl_info->total_seg = 1; /* reset concatl_info */
  if (port != NULL)
  {
    port->dest_port = ATL_SMS_INVALID_PORT_NUM;
    port->src_port  = ATL_SMS_INVALID_PORT_NUM;
  }
  udhl = *data;
  if (udhl > ATL_SMS_ONE_MSG_OCTET) return 0;
  read_byte += 1;
  while (read_byte <= udhl)
  {
    iei = *(data + read_byte); /* Get IEI */
    ie_len = *(data + read_byte + 1); /* Get IE Length */
    read_byte += 2;
    switch (iei)
    {
      case ATL_SMS_CONC8_MSG_IEI: //Concatenated short message , 8-bit reference
            if (concatl_info != NULL)
            {
              if ((*(data + read_byte + 2)) > 0) /* seg = 0 is invalid, we shall ignore this IE */
              {
                concatl_info->ref       = *(data + read_byte);
                concatl_info->total_seg = *(data + read_byte +1);
                concatl_info->seg       = *(data + read_byte +2);
              }
            }
            break;
      case ATL_SMS_CONC16_MSG_IEI: //Concatenated short message , 16-bit reference 
            if (concatl_info != NULL)
            {
              if ((*(data + read_byte + 3)) > 0) /* seg = 0 is invalid, we shall ignore this IE */
              {
                concatl_info->ref       = *(data + read_byte) *256 + *(data +read_byte +1);
                concatl_info->total_seg = *(data + read_byte + 2);
                concatl_info->seg       = *(data + read_byte + 3);
              }
            }
            break;
      case ATL_SMS_SPECIAL_MSG_IND_IEI: //Special SMS Message Indication
            if (msg_wait != NULL)
            {
              msg_wait->type_of_info |= ATL_SMS_MSG_WAIT_UDH;
              msg_wait->is_msg_wait   = true;
            }                     
            break;
      #ifdef __SMS_R6_ENHANCED_VOICE_MAIL__
        case ATL_SMS_ENHANCED_VM_INFO_IEI: //Enhanced Voice Mail Information
            if (msg_wait != NULL)
            {
              // decode and get result
              // kal_mem_set(&(msg_wait->evmi), 0, sizeof(sms_evmi_struct));
              ind_num = 0; //sms_decode_evmi(data+read_byte-2, (kal_uint16)(ie_len+2), (sms_evmi_struct *)&(msg_wait->evmi));
              if(ind_num != 0 )
              {
                msg_wait->type_of_info |= ATL_SMS_MSG_WAIT_UDH_EVM;
                msg_wait->is_msg_wait   = true;
              }
            }                      
            break;
      #endif
      case ATL_SMS_APP_PORT8_IEI: //Application Port
          if (port != NULL)
          {
            port->dest_port = *(data + read_byte);
            port->src_port  = *(data + read_byte + 1);
          }
          break;
      case ATL_SMS_APP_PORT16_IEI:
          if (port != NULL)
          {
            port->dest_port = *(data + read_byte) * 256 + *(data + read_byte + 1);
            port->src_port  = *(data + read_byte + 2) * 256 + *(data + read_byte + 3);
          }
          break;
      default:
          break;
    }                     
    if ((read_byte + ie_len) > ATL_SMS_ONE_MSG_OCTET) return 0; /* illegal PDU */
    read_byte += ie_len;
  }                             
  #ifndef __REL4__
  if(msg_wait != NULL) msg_wait->msp = 0;
  #endif
  return udhl;
}   

/*******************************************************************************
 ** \brief  This function decodes the TPDU and update the entry of message box.
 ** \param    a  IN/OUT   tpdu_decode_ptr, store the decoded results
 **           b  IN       data, pointer to TPDU
 **           c  IN       len, length of TPDU
 ** \retval success/fail
 ** \note 
 **    Octet:  0                   len-1 
 **            +--------------------+
 **            |        TPDU        |
 **            +--------------------+ 
 ******************************************************************************/
static bool atl_sms_decode_tpdu(atl_sms_tpdu_decode_t *tpdu_decode_ptr, uint8_t *data, uint8_t len)
{
  uint8_t addr_len = 0;
  uint8_t msg_offset = 0;
  if(tpdu_decode_ptr == NULL)
  {
    ATL_DEBUG("atl_sms_decode_tpdu(),failed");    
    return false;
  }
  //ASSERT(tpdu_decode_ptr != NULL);
  tpdu_decode_ptr->mti = (atl_sms_mti_t) (ATL_SMS_MTI_BITS & (*data));
  if (tpdu_decode_ptr->mti == ATL_SMS_MTI_UNSPECIFIED) tpdu_decode_ptr->mti = ATL_SMS_MTI_DELIVER;
  tpdu_decode_ptr->msg_wait.msp = 0; // set default
  switch (tpdu_decode_ptr->mti) /* Get Message Type */
  {
    case ATL_SMS_MTI_DELIVER:
    {
      atl_sms_deliver_peer_t *deliver_ptr;
      deliver_ptr = &tpdu_decode_ptr->data.deliver_tpdu;
      if(deliver_ptr->user_data_size == 0)
      {
        atl_sms_deliver_peer_t_alloc((void *)deliver_ptr);
        atl_sms_deliver_peer_t_unpack((void *)deliver_ptr, data, 0, len, NULL); /* Unpack the pdu to access the message contents */
      }
      tpdu_decode_ptr->fo = *data;
      addr_len = (deliver_ptr->orig_addr[0] + 1) / 2;
      msg_offset += (ATL_PART_OF_DELIVER_HEADER_LEN + addr_len); /* offset : fo, pid, dcs, scts, udl, oa_len, oa_type, oa */
      /*
        * NOTE that decoding DCS shall precedes decoding UDH,
        * * because message waiting in UDH shall override the info in DCS 
        * * if there is conflict 
        */
      atl_sms_decode_dcs(deliver_ptr->dcs, &tpdu_decode_ptr->alphabet_type, &tpdu_decode_ptr->msg_class,  &tpdu_decode_ptr->is_compress, &tpdu_decode_ptr->msg_wait);
      if((deliver_ptr->udh_p == false) && (*(deliver_ptr->user_data) <= (deliver_ptr->no_user_data - 1))) deliver_ptr->udh_p = atl_sms_detect_udh(deliver_ptr->user_data);
      if (deliver_ptr->udh_p == true)
      {
        /*
          * Decode User Data Header, 
          * * get the concatenated infomation if present 
          */
        /* 
          * if any following condition meet, this UDH is invalid:               
          *  UDHL > no. bytes of user data - 1
          */
        if (*(deliver_ptr->user_data) <= (deliver_ptr->no_user_data - 1))
        {
          tpdu_decode_ptr->udhl = atl_sms_decode_udh(deliver_ptr->user_data, &tpdu_decode_ptr->concatl_info, &tpdu_decode_ptr->msg_wait, &tpdu_decode_ptr->port);
          if (tpdu_decode_ptr->udhl > 0) tpdu_decode_ptr->udhl += 1; /* udhl + udh */
        }
        else
        {
          /* invalid UDH, set message to null */
          deliver_ptr->udh_p = 0;
          /* keep message content and send to MMI */
          /* deliver_ptr->user_data_len = 0; */
          /* deliver_ptr->no_user_data  = 0; */
        }
      }
      #if defined (__CPHS__)
        /* 
          * check whether this message is a voice mail, 
          * check following by CPHS:
          * 1) length of TP-OA = 4
          * 2) type of TP-OA = 1 101 0000 
          * 3) 01111110 (7e)
          *                 |set/clear
          *     x001000x
          * 4) 00111111 (3f)
          *     xx000000
          */
        if ((deliver_ptr->orig_addr[0] == 4) &&
            (deliver_ptr->orig_addr[1] == ATL_SMS_CPHS_VM_ADDR_TYPE) &&
            ((deliver_ptr->orig_addr[2] & 0x7e) == 0x10) &&
            ((deliver_ptr->orig_addr[3] & 0x3f) == 0x00))
        {
          tpdu_decode_ptr->msg_wait.type_of_info |= ATL_SMS_MSG_WAIT_CPHS;
          tpdu_decode_ptr->msg_wait.is_msg_wait = true;
        }
      #endif
      tpdu_decode_ptr->msg_len = deliver_ptr->no_user_data;
      break;
    } /* case */
    default:  /* wrong message type */
      /*
      * in this case , the message type is reserved,
      * * we shall ignore this message but do not delete it 
      */
      break;
  }  /* switch (mti) */
  tpdu_decode_ptr->offset = msg_offset;
  return true;
}    

/*******************************************************************************
 ** \brief  This function decodes the short message read from SIM/NVM,
 **          update the entry of message box.
 ** \param   a  IN       data[]
 **             Octet:  0         1        X       X+1       Y               174
 **                     +----------+-------+-----------------------------------+
 **                     |  SCA LEN | TOSCA |   SCA   |          TPDU           |
 **                     +----------+-------+-----------------------------------+
 **          b  IN       length (must equals to 175)
 **          c  IN/OUT   sms_pdu, contain the decoded result
 ** \retval none
 ******************************************************************************/
static bool atl_decode_sms_pdu(uint8_t *pdu, uint8_t pdu_len, atl_sms_pdu_decode_t *sms_pdu)
{
  bool retval;
  if((pdu == NULL) || (pdu_len == 0) || (sms_pdu == NULL))
  {
    atl_trace("[LOGS|LINE:%d|FUNC:%s]>>  atl_decode_sms_pdu(),failed",__LINE__ , __FUNCTION__);    
    return false;
  }
  //ASSERT((pdu != NULL) && (pdu_len != 0) && (sms_pdu != NULL));
  sms_pdu->pdu_len = 0;
  sms_pdu->tpdu_len = 0;
  //Get SC address 
  if(*pdu > ATL_SMS_ADDR_LEN) /* length exceed max sc address length */
  {
    sms_pdu->tpdu.mti = ATL_SMS_MTI_ILLEGAL;
    return false;
  }
  sms_pdu->sca.addr_length = *pdu;
  if (*pdu > 0) memcpy(sms_pdu->sca.addr_bcd, (pdu + 1), sms_pdu->sca.addr_length);
  sms_pdu->pdu_len += (1 + *pdu);     /* for sca length */
  atl_sms_reset_tpdu_decode_struct(&(sms_pdu->tpdu));
  sms_pdu->tpdu.mti = (atl_sms_mti_t) (*(pdu + 1 + sms_pdu->sca.addr_length) & ATL_SMS_MTI_BITS); /* reset user_data_size */
  if (sms_pdu->tpdu.mti == ATL_SMS_MTI_UNSPECIFIED) sms_pdu->tpdu.mti = ATL_SMS_MTI_DELIVER;
  if (sms_pdu->tpdu.mti == ATL_SMS_MTI_DELIVER)     sms_pdu->tpdu.data.deliver_tpdu.user_data_size = 0;
  else if (sms_pdu->tpdu.mti == ATL_SMS_MTI_SUBMIT) sms_pdu->tpdu.data.submit_tpdu.user_data_size = 0;
  if ((retval = atl_sms_decode_tpdu(&(sms_pdu->tpdu), (pdu + 1 + sms_pdu->sca.addr_length), (uint8_t) (pdu_len - 1 - sms_pdu->sca.addr_length))) == true)
  {
    sms_pdu->tpdu_len = sms_pdu->tpdu.offset +sms_pdu->tpdu.msg_len;
    sms_pdu->pdu_len += sms_pdu->tpdu_len;
    sms_pdu->tpdu.offset += (1 + *pdu);     /* for sca */
  }
  if (sms_pdu->tpdu.msg_wait.is_msg_wait == true)
  {
    atl_trace("[LOGS|LINE:%d|FUNC:%s]>>  atl_decode_sms_pdu,is_msg_wait is true!!! ",__LINE__ , __FUNCTION__);    
    //sms_message_waiting_handler(sms_pdu);
  }
  return retval;
}  

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static void atl_sms_deliver_peer_t_dealloc(void *mtk_d)
{
  atl_sms_deliver_peer_t *s = (atl_sms_deliver_peer_t *) mtk_d;
  if(s->orig_addr != NULL)
  {
    free (s->orig_addr);
    s->orig_addr = 0;
  }
  if (s->user_data != NULL)
  {
    free (s->user_data);
    s->user_data = 0;
    s->user_data_size = 0;
  }
}

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static void atl_sms_free_tpdu_decode_struct(atl_sms_tpdu_decode_t *tpdu_decode_ptr)
{
  if (tpdu_decode_ptr->mti == ATL_SMS_MTI_DELIVER)
  {
    /* Even enable MCD, this function shall be called !! */
    atl_sms_deliver_peer_t_dealloc((void *)&tpdu_decode_ptr->data.deliver_tpdu);
  }
}

/*******************************************************************************
 ** \brief  This is get_ch_byte_24008 function of L4C module.
 **         convert a BCD byte (lower byte only) to ascii char
 **         this obey spec 24.008 10.5.118 table diff form 11.11 SIM spec
 ** \param  Input BCd byte
 ** \retval output ascii char
 ******************************************************************************/
static uint8_t atl_get_ch_byte_24008(uint8_t bcd)
{
  uint8_t digit;
  if (bcd <= 9)
  {
    digit = bcd + '0';
  }
  else
  {
    switch (bcd)
    {
      case 0x0a: digit = '*';  break;
      case 0x0b: digit = '#';  break;
      case 0x0c: digit = 'a';  break;
      case 0x0d: digit = 'b';  break;
      case 0x0e: digit = 'c';  break;
      default:   digit = '\0'; break;
    }
  }
  return digit;
}

/*******************************************************************************
 ** \brief  This is get_ch_byte function of L4C module. convert a BCD byte
 **         (lower byte only) to ascii char
 **         this obey spec 24.008 10.5.118 table diff form 11.11 SIM spec
 ** \param  bcd         [IN]        Input BCd byte
 **         type        [IN]        Type of definition
 **         value 0: for original defintion(MTK make use of 0xd) , value 1: new definition(with wild char support)(?)
 ** \retval output ascii char
 ******************************************************************************/
static uint8_t atl_get_ch_byte(uint8_t bcd, uint8_t type)
{
  uint8_t digit;
  uint8_t value_d_char = 'w';       // original defnition
  /* mtk02514 ************************
  * When type = CONVERT_TO_DIGIT_WILD,
  * we should convert 0x0d to wild char
  *********************************/
  //mtk01616_080428: new definition (with wild character support), other part is the same as original definition
  if (type == ATL_CONVERT_TO_DIGIT_WILD) value_d_char = '?';
  if (bcd <= 9)digit = bcd + '0';
  else
  {
    switch (bcd)
    {
      case 0x0a: digit = '*';          break;
      case 0x0b: digit = '#';          break;
      case 0x0c: digit = 'p';          break;
      case 0x0d: digit = value_d_char; break;
      case 0x0e: digit = '+';          break;
      default:   digit = '\0';         break;
    }
  }
  return digit;
}

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static void atl_to_upper(uint8_t *str)
{
  uint8_t *ptr;
  if(str == NULL)
  {
    ATL_DEBUG("atl_to_upper(),failed");    
    return ;
  }
  //ASSERT(str != NULL);
  ptr = str;
  while (*ptr != 0)
  {
    if (ATL_RMMI_IS_LOWER(*ptr)) *ptr += 'A' - 'a';
    ptr++;
  }
}

/*******************************************************************************
 ** \brief  same as check_hex_value, but make sure that input string combined of "0"~"9","A"~"F"
 **         "1234" -> 0x1234
 ** \param  str     [IN]        String
 **         val     [OUT]       Hex value.
 ** \retval true/false depends on success or fail.
 ******************************************************************************/
static uint16_t atl_check_hex_value_ext(uint8_t *str, uint8_t *val)
{
  uint16_t i = 0, j = 0;
  uint16_t temp;
  if((str == NULL) || (val == NULL))
  {
    ATL_DEBUG("atl_check_hex_value_ext(),failed");    
    return 0;
  }
  //ASSERT((str != NULL) && (val != NULL));
  atl_to_upper(str);
  while (str[i] != '\0')
  {
      if (ATL_RMMI_IS_NUMBER(str[i]))              temp = str[i] - '0';
      else if ((str[i] >= 'A') && (str[i] <= 'F')) temp = (str[i] - 'A') + 10;
      else                                         return 0;
      if (ATL_RMMI_IS_NUMBER(str[i + 1]))                  temp = (temp << 4) + (str[i + 1] - '0');
      else if ((str[i + 1] >= 'A') && (str[i + 1] <= 'F')) temp = (temp << 4) + ((str[i + 1] - 'A') + 10);
      else                                                 return 0;
      val[j] = temp;
      i += 2;
      j++;
  }
  val[j] = 0;
  ATL_DEBUG("atl_check_hex_value_ext(), len = %d", j);    
  return j;
}

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static void atl_sms_scts_to_data_time(uint8_t *scts,atl_rtc_date_t* Date,atl_rtc_time_t* Time)
{
  uint8_t dest[25]   = {0};
  uint8_t year[5]    = {0};
  uint8_t month[3]   = {0};
  uint8_t day[3]     = {0};
  uint8_t hours[3]   = {0};
  uint8_t minutes[3] = {0};
  uint8_t sec[3]     = {0};
  char* ptr = NULL;
  ptr = &dest[0];
  if(scts == NULL)
  {
    atl_trace("[LOGS|LINE:%d|FUNC:%s]>>  atl_sms_scts_to_data_time failed",__LINE__ , __FUNCTION__);    
    return;
  }
  if(strcmp((char*)scts, "\0\0\0\0\0\0\0") == 0)
  {
    dest[0] = '\0';
    return;
  }
	sprintf((uint8_t*) dest,
          "20%d%d%d%d%d%d%d%d%d%d%d%d%d%d\0",// "20%d%d/%d%d/%d%d,%d%d:%d%d:%d%d%c%d%d\0",
          scts[0] & 0xf, (scts[0] >> 4),
          scts[1] & 0xf, (scts[1] >> 4),
          scts[2] & 0xf, (scts[2] >> 4),
          scts[3] & 0xf, (scts[3] >> 4),
          scts[4] & 0xf, (scts[4] >> 4),
          scts[5] & 0xf, (scts[5] >> 4),
          scts[6] & 0x7, (scts[6] >> 4));
  memset((void *)Date,0,sizeof(atl_rtc_date_t));
  strncpy(year,ptr,4);
  ptr +=4;
  Date->year = atoi(year);
  strncpy(month,ptr,2);
  ptr +=2;
  Date->month = atoi(month);
  strncpy(day,ptr,2);
  ptr +=2;
  Date->day = atoi(day);
  memset((void *)Time,0,sizeof(atl_rtc_time_t));
  strncpy(hours,ptr,2);
  ptr +=2;
  Time->hours = atoi(hours);
  strncpy(minutes,ptr,2);
  ptr +=2;
  Time->minutes= atoi(minutes);
  strncpy(sec,ptr,2);
  Time->seconds = atoi(sec);
}

/*******************************************************************************
 ** \brief  ...
 ** \param  ...
 ** \retval ...
 ******************************************************************************/
static void atl_check_sms_pdu_string(uint16_t length, uint8_t *bytes, uint8_t *str)
{
  uint16_t i = 0;
  uint16_t j = 0;
  if((str == NULL) || (bytes == NULL))
  {
    ATL_DEBUG("atl_check_sms_pdu_string() failed");    
    return;    
  }
  while (i < length)
  {
    j += sprintf((char*)str + j, "%02x", bytes[i]);
    i++;
  }
  str[j] = 0;
  atl_to_upper(str);
}

/*******************************************************************************
 ** \brief  Function to decode sms pdu data
 ** \param  data - ptr to pdu data
 ** \retval bool
 ******************************************************************************/
bool atl_sms_decode_pdu(uint8_t* data)
{
  atl_sms_pdu_decode_t sms_pdu = {0};
  bool ret_val = false;
  uint16_t pdu_len = 0;
  uint8_t ptr_pdu[ATL_SMS_MAX_TPDU_SIZE * 2] = {0};
  uint8_t use_data[ATL_SMS_MAX_TPDU_SIZE * 2] = {0};
  uint8_t szDestAddr[43] = {0};
  atl_rtc_date_t Date = {0};
  atl_rtc_time_t Time = {0};
  uint8_t dateTime[40] = {0};
  uint16_t i =0;
  uint8_t dcs = 0;
  uint16_t userDataLen = 0;
  atl_sms_concatl_t concatInfo = {0};
  uint8_t* ptr = NULL;
  atl_sms_alphabet_t alphabet_type;
  memset(&sms_pdu,0,sizeof(atl_sms_pdu_decode_t));
  ATL_DEBUG("atl_sms_decode_pdu(), sms pdu (%s),(%d)", data, strlen(data));    
  pdu_len = atl_check_hex_value_ext(data, ptr_pdu);
  atl_trace("[LOGS|LINE:%d|FUNC:%s]>>  atl_sms_decode_pdu(), pdu_len (%d)",__LINE__ , __FUNCTION__,pdu_len);    
  if(atl_decode_sms_pdu(ptr_pdu, pdu_len,&sms_pdu))
  {
    dcs = sms_pdu.tpdu.data.deliver_tpdu.dcs;//dcs ֵ
    alphabet_type = sms_pdu.tpdu.alphabet_type;//ǰű뷽ʽ
    concatInfo.ref = sms_pdu.tpdu.concatl_info.ref;//Ӷűʶ
    concatInfo.seg = sms_pdu.tpdu.concatl_info.seg;//Ӷк
    concatInfo.total_seg = sms_pdu.tpdu.concatl_info.total_seg;//Ӷܸ
    userDataLen = sms_pdu.tpdu.data.deliver_tpdu.user_data_len;//ǰŵݳ
    atl_sms_orig_address_convert(sms_pdu.tpdu.data.deliver_tpdu.orig_addr, szDestAddr);//ȡ绰
    atl_sms_scts_to_data_time(sms_pdu.tpdu.data.deliver_tpdu.scts,&Date,&Time);//ȡʱ
    sprintf(dateTime,"%04d-%02d-%02d,%02d:%02d:%02d",Date.year, Date.month, Date.day, Time.hours,Time.minutes, Time.seconds);
    ATL_DEBUG("atl_sms_decode_pdu(), date time (%s)",dateTime);    
    ATL_DEBUG("atl_sms_decode_pdu(), number (%s)",szDestAddr);    
    ATL_DEBUG("atl_sms_decode_pdu(), total (%d),seg (%d),ref(%d)",concatInfo.total_seg,concatInfo.seg,concatInfo.ref);    
    ATL_DEBUG("atl_sms_decode_pdu(), dcs(%d)",dcs);    
    ATL_DEBUG("atl_sms_decode_pdu(), alphabet_type(%d)",alphabet_type);    
    if(concatInfo.total_seg > 1)//ʾǰӶ
    {
      atl_sms_contact_data.total = concatInfo.total_seg;
      atl_sms_contact_data.sms_contact_data_info[concatInfo.seg].segement = concatInfo.seg;
      ptr = sms_pdu.tpdu.data.deliver_tpdu.user_data + 6;
      userDataLen -= 6;
      atl_sms_contact_data.sms_contact_data_info[concatInfo.seg].use_len = userDataLen;
      //atl_sms_contact_data.sms_contact_data_info[seg].use_data
      memcpy(atl_sms_contact_data.sms_contact_data_info[concatInfo.seg].use_data, ptr, userDataLen);
      atl_sms_contact_data.count++;
      if(atl_sms_contact_data.count == atl_sms_contact_data.total)//ʾյӶ
      {
        ATL_DEBUG("atl_sms_decode_pdu(), total = %d",atl_sms_contact_data.total);    
        for(i = 1; i <= atl_sms_contact_data.total;i++ )//ж
        {
          atl_check_sms_pdu_string(atl_sms_contact_data.sms_contact_data_info[i].use_len,atl_sms_contact_data.sms_contact_data_info[i].use_data,use_data);
          ATL_DEBUG("atl_sms_decode_pdu(), user data(%s)",use_data);    
          memset(use_data,0, ATL_SMS_MAX_TPDU_SIZE * 2);
        }
        memset(&atl_sms_contact_data, 0, sizeof(atl_sms_contact_info_array_t));
      }
    }
    else
    {
      atl_sms_contact_data.total = 1;
      atl_sms_contact_data.sms_contact_data_info[0].segement = 1;
      //atl_sms_contact_data.sms_contact_data_info[0].use_data
      memcpy(atl_sms_contact_data.sms_contact_data_info[0].use_data, sms_pdu.tpdu.data.deliver_tpdu.user_data, userDataLen);
      atl_check_sms_pdu_string(userDataLen,atl_sms_contact_data.sms_contact_data_info[0].use_data,use_data);
      ATL_DEBUG("atl_sms_decode_pdu(), user data len (%d)", userDataLen);    
      ATL_DEBUG("atl_sms_decode_pdu(), user data(%s)", use_data);    
      memset(&atl_sms_contact_data, 0, sizeof(atl_sms_contact_info_array_t));
    }
    atl_sms_free_tpdu_decode_struct(&sms_pdu.tpdu);
    ret_val = true;
  }
  else
  {
    atl_sms_free_tpdu_decode_struct(&sms_pdu.tpdu);
  }
  return ret_val;
}
