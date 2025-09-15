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
#include "atl_new.h"
#include "atl_memory_alloc.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ATL_ENTITY_QUEUE_SIZE    10
#define ATL_ITEM_QUEUE_SIZE      20
#define ATL_CMD_OK               "\r\nOK\r\n"
#define ATL_CMD_ERROR            "\r\nERROR\r\n"
#define ATL_CMD_END              ATL_CMD_CRLF
#define ATL_CMD_START            ATL_CMD_CRLF

#define ATL_RING_GAP(ring_len, first, second) \
     (((first) >= (second)) ? ((ring_len) - ((first) - (second))) : ((second) - (first)))

#define ATL_RING_ID_VALID(idx) ((idx) != UINT16_MAX)

#define ATL_RING_POS(base, offset, len) (((base) + (offset)) % (len))

#define ATL_DEBUG eat_trace
extern void (*const eat_trace)(char *, ...);

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
typedef enum {
  ATL_ITEM_FUN = 1,
  ATL_ITEM_CMD,
} atl_cmd_item_type_t;

typedef enum {
  ATL_WRITE_PHASE = 0,
  ATL_READ_PHASE,
} atl_cmd_item_phase_t;

typedef struct atl_user_init_t{
  bool init;
  atl_printf atl_printf;
  atl_write atl_write;
} atl_user_init_t;

typedef struct {
  atl_item_t item[ATL_ITEM_QUEUE_SIZE];
  uint8_t item_cnt;             //amount of cmds
  uint8_t item_id;              //current id of executionable cmd
  atl_entity_cb_t entity_cb;    //cb for item
} atl_entity_t;

typedef struct {
  atl_entity_t entity[ATL_ENTITY_QUEUE_SIZE];
  uint8_t entity_head;
  uint8_t entity_tail;
  uint8_t entity_cnt;
} atl_entity_queue_t;

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static atl_entity_queue_t atl_entity_queue = {0};
static uint16_t atl_timer = 0;
atl_user_init_t atl_user_init = {0};

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Init atl lib  
 ** \param  atl_printf - pointer to user func of printf
 ** \param  atl_write  - pointer to user func of write to uart
 ** \retval false: some error is ocur true: append success
 ******************************************************************************/
void atl_init(const atl_printf atl_printf, const atl_write atl_write)
{
  assert(atl_write == NULL);
  atl_heap_init();
  atl_user_init.atl_write = atl_write;
  atl_user_init.init = true;
}

/*******************************************************************************
 ** \brief  Default CB function to deal with single at cmd RSP code 
 ** \param  p_rsp_str - string to response for this at cmd
 ** \retval @atl_cmd_rsp_t
 ******************************************************************************/
int atl_enqueue(atl_item_t item[], uint8_t item_amount, atl_entity_cb_t entity_cb)
{
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_tail];
  if(atl_entity_queue.entity_cnt >= ATL_ENTITY_QUEUE_SIZE) return -1;
  if(!item || !item_amount) return -1;
  cur_entity->item_cnt = item_amount;
  cur_entity->entity_cb = entity_cb;
  while(item_amount)
  {
    cur_entity->item[cur_entity->item_cnt -item_amount] = item[cur_entity->item_cnt -item_amount];
    if(item[cur_entity->item_cnt -item_amount].save)
    {
      uint8_t* new_mem = atl_heap_alloc(strlen(item[cur_entity->item_cnt -item_amount].request));
      if(!new_mem)
      {
        atl_heap_init();
        return -1;
      }
      memcpy(new_mem, item[cur_entity->item_cnt -item_amount].request, strlen(item[cur_entity->item_cnt -item_amount].request));
      item[cur_entity->item_cnt -item_amount].request = new_mem;
    }
    --item_amount;
  }
  atl_entity_queue.entity_tail = (atl_entity_queue.entity_tail +1) % ATL_ENTITY_QUEUE_SIZE;
  ++atl_entity_queue.entity_cnt;
}

/*******************************************************************************
 ** \brief  Default CB function to deal with single at cmd RSP code 
 ** \param  p_rsp_str - string to response for this at cmd
 ** \retval @atl_cmd_rsp_t
 ******************************************************************************/
int atl_dequeue(void)
{
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_head];
  uint8_t item_amount = cur_entity->item_cnt;
  if(atl_entity_queue.entity_cnt == 0) return -1;
  while(item_amount)
  {
    if(cur_entity->item[cur_entity->item_cnt -item_amount].save) atl_heap_free(cur_entity->item[cur_entity->item_cnt -item_amount].request);
    --item_amount;
  }
  atl_entity_queue.entity_head = (atl_entity_queue.entity_head +1) % ATL_ENTITY_QUEUE_SIZE;
  --atl_entity_queue.entity_cnt;
}

/*******************************************************************************
 * \brief Functions for matching pattern in ring buffer with offset
 * \param ring        ptr to ring buffer
 * \param ring_len    ring buffer len
 * \param start       id in buff where data will be compared
 * \param pattern     pattern to compare with
 * \param pattern_len pattern len
 * \retval true       pattern found
 * \retval false      pattern not found
 ******************************************************************************/
static bool atl_ring_match(const uint8_t* ring, uint16_t ring_len, uint16_t start, const uint8_t* pattern, uint16_t pattern_len) 
{
  for (uint16_t i = 0; i < pattern_len; ++i) 
  {
    uint16_t pos = (start + i) % ring_len;
    if (ring[pos] != pattern[i]) return false;
  }
  return true;
}

/*******************************************************************************
 ** \brief  Functions for parcing answer 
 ** \param  ring      - ptr to ring buffer
 ** \param  ring_len  - ring buffer len
 ** \param  ring_tail - index for new data in buff
 ** \param  usr_req   - sended request with CR LF
 ** \param  usr_answ  - expected answer in data field of answer
 ** \param  usr_data  - ptr to place where expected data will be placed. Len should be enough
 ** \retval 1 - all done success, 0 - some issues occurs, -1 - got error answer for this req
 ******************************************************************************/
static int atl_ring_parcer(const uint8_t* ring, const uint16_t ring_len, const uint16_t ring_tail, const char* usr_req, const char* usr_answ, uint8_t* usr_data, const uint16_t user_data_len)
{
  const uint8_t usr_req_len    = strlen(usr_req) -1; // echo returns only CR, so we ignore LF
  const uint8_t usr_answ_len   = strlen(usr_answ);
  const uint8_t resp_ok_len    = strlen(ATL_CMD_OK);
  const uint8_t resp_error_len = strlen(ATL_CMD_ERROR);
  const uint8_t crlf_len       = strlen(ATL_CMD_CRLF);

  uint16_t request_idx = UINT16_MAX; // \r\nREQ\r\n
  uint16_t res_idx     = UINT16_MAX; // \r\nRES\r\n
  uint16_t data_idx    = UINT16_MAX; // \r\nDATA\r\n

  if(!ring || ring_len == 0 || !usr_req) return 0;

  if(true) //REQUEST, RES fields find and proc
  {
    uint8_t crlf_cnt = 0;
    for(uint16_t i = 0; i < ring_len; ++i)
    {
      if(!ATL_RING_ID_VALID(request_idx)) //try to find echo request one single time
      {
        if(atl_ring_match(ring, ring_len, ATL_RING_POS(ring_tail,i,ring_len), usr_req, usr_req_len)) request_idx = (ring_tail +i)%ring_len;
      }
      if(ATL_RING_ID_VALID(request_idx) && !ATL_RING_ID_VALID(res_idx)) //echo request was found, now try to find OK or ERROR
      {
        if(atl_ring_match(ring, ring_len, ATL_RING_POS(ring_tail,i,ring_len), ATL_CMD_CRLF, crlf_len)) ++crlf_cnt;
        if(crlf_cnt > 3) return 0; //here is to much CRLF already been found before RES, smthk is wrong, return error
        if(atl_ring_match(ring, ring_len, ATL_RING_POS(ring_tail,i,ring_len), ATL_CMD_OK, resp_ok_len)) res_idx = (ring_tail +i +crlf_len)%ring_len; //found OK
        if(atl_ring_match(ring, ring_len, ATL_RING_POS(ring_tail,i,ring_len), ATL_CMD_ERROR, resp_error_len)) return -1; //found ERROR
      }
      if(ATL_RING_ID_VALID(request_idx) && ATL_RING_ID_VALID(res_idx)) break;
    }
  }

  if(true) //DATA field find and proc
  {
    uint16_t tmp_data_idx   = UINT16_MAX;   // \r\nDATA\r\n
    uint16_t tmp_data_start = UINT16_MAX; // DATA\r\n
    uint16_t tmp_data_end   = UINT16_MAX;   // \r\n
    if(true)//Searching DATA field, depends on REQUEST and RES
    {
      if(!ATL_RING_ID_VALID(request_idx) && !ATL_RING_ID_VALID(res_idx)) //(\r\nDATA\r\n)
      {
        if(!usr_answ && !usr_data) return 0; //check
        tmp_data_idx = ring_tail;
      }
      else if(ATL_RING_ID_VALID(request_idx) && !ATL_RING_ID_VALID(res_idx)) //(REQ\r\r\nDATA\r\n)
      {
        if(!usr_answ && !usr_data) return 0; //check
        tmp_data_idx = ATL_RING_POS(request_idx,usr_req_len,ring_len); 
        data_idx = tmp_data_idx;
      }
      else // (ATL_RING_ID_VALID(request_idx) && ATL_RING_ID_VALID(res_idx)
      {
        bool gap = ATL_RING_GAP(ring_len, res_idx, ATL_RING_POS(request_idx,usr_req_len,ring_len)) > crlf_len*2; //there is some gap between res and req or not
        if(!usr_answ && !usr_data) return 1; //check
        if(!gap) tmp_data_idx = ATL_RING_POS(res_idx,resp_ok_len,ring_len);     //(REQ\r\r\nOK\r\n\r\nDATA\r\n)
        else     tmp_data_idx = ATL_RING_POS(request_idx,usr_req_len,ring_len); //(REQ\r\r\nDATA\r\n\r\nOK\r\n)
        data_idx = tmp_data_idx;
      }
    }
    if(true)//We found DATA field and have input pararms to compare it or save it, so lets do it
    {
      uint16_t tmp_data_len = 0;
      for(uint16_t i = 0; i < ring_len; ++i)
      {
        if(atl_ring_match(ring, ring_len, ATL_RING_POS(data_idx,i,ring_len), ATL_CMD_CRLF, crlf_len))
        {
          if(!ATL_RING_ID_VALID(tmp_data_start)) 
          {
            tmp_data_start = ATL_RING_POS(data_idx,i+crlf_len,ring_len); 
            continue;
          }
          if(ATL_RING_ID_VALID(tmp_data_start) && !ATL_RING_ID_VALID(tmp_data_end)) 
          {
            tmp_data_end = ATL_RING_POS(data_idx,i,ring_len); 
          }
        }
        if(ATL_RING_ID_VALID(tmp_data_start) && ATL_RING_ID_VALID(tmp_data_end)) break;
      }
      if(!ATL_RING_ID_VALID(tmp_data_start) || !ATL_RING_ID_VALID(tmp_data_end)) return 0; //we didnt found \r\n....\r\n of data
      if(tmp_data_start == tmp_data_end) return 0;
      else tmp_data_len = ATL_RING_GAP(ring_len, tmp_data_start, tmp_data_end);
      if(usr_answ) //check user answer with data
      {
        bool match = false;
        if(usr_answ_len > tmp_data_len) return 0; //answer is bigger then the rest of the data
        for(uint16_t i = 0; i < tmp_data_len; ++i)
        {
          if(atl_ring_match(ring, ring_len, ATL_RING_POS(tmp_data_start,i,ring_len), usr_answ, usr_answ_len))
          {
            match = true;
            break;
          } 
        }
        if(!match) return 0; //didnt found similar answer in data
      }
      if(usr_data)
      {
        if(user_data_len < tmp_data_len) return 0;
        for(uint16_t i = 0; i < tmp_data_len; ++i)
        {
          usr_data[i] = ring[ATL_RING_POS(tmp_data_start,i,ring_len)];
        }
      }
    }
  }
  return 1; //all done
}

/*******************************************************************************
 ** \brief  Default CB function to deal with single at cmd RSP code 
 ** \param  p_rsp_str - string to response for this at cmd
 ** \retval @atl_cmd_rsp_t
 ** \note   !non-reentrancy
 ******************************************************************************/
void atl_entity_proc(const uint8_t* ring, const uint8_t* ring_head, const uint16_t ring_len, const uint16_t ring_tail)
{
  static uint8_t phase = ATL_WRITE_PHASE;
  if(!atl_entity_queue.entity_cnt) return;
  atl_entity_t* entity = &atl_entity_queue.entity[atl_entity_queue.entity_head];
  switch(phase)
  {
    case ATL_WRITE_PHASE:
         {
           atl_item_t* item = &entity->item[entity->item_id];
           if(item->request) atl_user_init.atl_write(item->request, strlen(item->request));
           atl_timer = item->wait;
           ++phase;
         }
         break;
    case ATL_READ_PHASE:
         {
           atl_item_t* item = &entity->item[entity->item_id];
           if(atl_ring_parcer(ring, ring_len, ring_tail, item->request, item->answer, item->data, item->data_len)) 
           {
             if(entity->item_id >= entity->item_cnt - 1) 
             {
               entity->entity_cb(true);
               atl_dequeue();
             } 
             else 
             {
               ++entity->item_id;
             }
           } 
           else if(atl_timer) 
           {
             --atl_timer;
             if(!atl_timer && item->rpt_cnt) 
             {
               --item->rpt_cnt;
               if(!item->rpt_cnt) 
               {
                 if(item->request) 
                 {
                   if(entity->item_id >= entity->item_cnt - 1) 
                   {
                     entity->entity_cb(false);
                     atl_dequeue();
                   }
                 } 
                 else 
                 {
                   ++entity->item_id;
                 }
               }
             }
           }
           --phase;
         }
         break;
  }
}

