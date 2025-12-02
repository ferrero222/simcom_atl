//============================================================================
// ET: embedded test; very simple test example
//============================================================================
#include "et.h"  // ET: embedded test

#include "asc_core.h" // 
#include "o1heap.h" // 
#include "asc_port.h" // 
#include "asc_chain.h"  // ET: embedded test
#include "asc_mdl_general.h"
#include <stdio.h>

static asc_context_t test_ctx = {0};

uint8_t test_buffer[2048] = {0};
const uint16_t test_buffer_head = 1024;
const uint16_t test_buffer_tail = 0;

asc_ring_buffer_t asc_ring_buffer = {
  .buffer = test_buffer,
  .count = 0,
  .head = test_buffer_head,
  .tail = test_buffer_tail,
  .size = 2048,
};

uint16_t test_write(uint8_t* buff, uint16_t len) {
  (void)buff;
  (void)len;
  return 1;
}

void test_printf(const char* string) {
  (void)string;
  return;
}

void setup(void) {
    // executed before *every* non-skipped test
}

void teardown(void) {
    // executed after *every* non-skipped and non-failing test
}

void testItemCB(ringslice_t data_slice, bool result, void* const data)
{
  (void)data_slice;
  VERIFY(result);
  asc_mdl_rtd_t* real_data = (asc_mdl_rtd_t*)data;
  VERIFY(strcmp(real_data->modem_imei, "5235") == 0);
}

void testEntityCB(const bool result, void* const meta, const void* const data)
{
  VERIFY(result);
  VERIFY(meta == test_buffer);
  asc_mdl_rtd_t* real_data = (asc_mdl_rtd_t*)data;
  VERIFY(strcmp(real_data->modem_imei, "5235") == 0);
}

void testUrcCB(ringslice_t urc_slice)
{
  VERIFY(ringslice_strncmp(&urc_slice, "+TEST", strlen("TEST")) == 0);
}

bool testChainFunc(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  VERIFY(param == test_buffer);

  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM(NULL, "+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
  };
  bool res = asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, sizeof(asc_mdl_rtd_t), meta);
  VERIFY(res);
  asc_entity_queue_t* queue =_asc_get_entity_queue(ctx);
  while(queue->entity_cnt)
  {
    _asc_core_proc(ctx);
  }
  return res;
}

bool testChainCond(void)
{
  return true;
}

// test group ----------------------------------------------------------------
TEST_GROUP("ATL") {

  { //ASC_CORE=====================================================================
    TEST("o1heap lib") {
      O1HeapInstance* heap = o1heapInit(test_buffer, ASC_MEMORY_POOL_SIZE); 
      bool res = o1heapDoInvariantsHold(heap);
      VERIFY(res);
      void* ptr1 = o1heapAllocate(heap, 111);
      VERIFY(ptr1);
      void* ptr2 = o1heapAllocate(heap, 345);
      VERIFY(ptr2);
      o1heapFree(heap, ptr1);
      o1heapFree(heap, ptr2);
      VERIFY(o1heapGetDiagnostics(heap).allocated == 0);
    }

    TEST("asc_init()/asc_deinit() testing") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);
      VERIFY(_asc_get_init(&test_ctx).init);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_entity_enqueue()/asc_entity_dequeue() simple AT`s") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT"ASC_CMD_CRLF,   NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL, NULL, ASC_NO_ARG),
        ASC_ITEM("AT"ASC_CMD_CRLF,   NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL, NULL, ASC_NO_ARG),  
        ASC_ITEM("ATE1"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 600, 0, 0, NULL, NULL, ASC_NO_ARG),  
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, 0, NULL);
      VERIFY(res);
      res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, 0, NULL);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      VERIFY(queue->entity_cnt == 2);
      VERIFY(queue->entity[0].item_cnt == sizeof(items)/sizeof(items[0]));
      VERIFY(queue->entity[1].item_cnt == sizeof(items)/sizeof(items[0]));
      asc_entity_dequeue(&test_ctx);
      VERIFY(queue->entity_cnt == 1);
      asc_entity_dequeue(&test_ctx);
      VERIFY(queue->entity_cnt == 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_entity_enqueue()/asc_entity_dequeue() composite AT`s") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM(ASC_CMD_SAVE"AT+GSN"ASC_CMD_CRLF,       NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,               "%15[^\x0d]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
        ASC_ITEM(ASC_CMD_SAVE"AT+GMM"ASC_CMD_CRLF,       NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,               "%15[^\x0d]", ASC_ARG(asc_mdl_rtd_t, modem_id)),  
        ASC_ITEM("AT+GMR"ASC_CMD_CRLF,                   NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,      "Revision:%29[^\x0d]", ASC_ARG(asc_mdl_rtd_t, modem_rev)),                
        ASC_ITEM("AT+CCLK?"ASC_CMD_CRLF,                 NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,      "+CCLK: \"%21[^\"]\"", ASC_ARG(asc_mdl_rtd_t, modem_clock)),          
        ASC_ITEM("AT+CCID"ASC_CMD_CRLF,   ASC_CMD_SAVE"+CCLK", ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,               "%21[^\x0d]", ASC_ARG(asc_mdl_rtd_t, sim_iccid)),             
        ASC_ITEM("AT+COPS?"ASC_CMD_CRLF,  ASC_CMD_SAVE"+COPS", ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL, "+COPS: 0, 0,\"%49[^\"]\"", ASC_ARG(asc_mdl_rtd_t, sim_operator)),            
        ASC_ITEM("AT+CSQ"ASC_CMD_CRLF,     ASC_CMD_SAVE"+CSQ", ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,                 "+CSQ: %d", ASC_ARG(asc_mdl_rtd_t, sim_rssi)),             
        ASC_ITEM("AT+CENG=3"ASC_CMD_CRLF,                NULL, ASC_PARCE_SIMCOM, 2, 600, 0, 1, NULL,                       NULL, ASC_NO_ARG),                
        ASC_ITEM("AT+CENG?"ASC_CMD_CRLF,                 NULL, ASC_PARCE_SIMCOM, 2, 600, 0, 0, NULL,                       NULL, ASC_NO_ARG),  
      };         
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      VERIFY(queue->entity_cnt == 2);
      VERIFY(queue->entity[0].item_cnt == sizeof(items)/sizeof(items[0]));
      VERIFY(queue->entity[1].item_cnt == sizeof(items)/sizeof(items[0]));
      VERIFY(queue->entity[0].data);
      VERIFY(queue->entity[1].data);
      VERIFY((asc_mdl_rtd_t*){queue->entity[0].data}->modem_clock == queue->entity[0].item[3].answ.ptrs[0]);
      VERIFY(queue->entity[0].item[0].answ.ptrs[1] == ASC_NO_ARG);
      VERIFY((asc_mdl_rtd_t*){queue->entity[1].data}->sim_iccid == queue->entity[1].item[4].answ.ptrs[0]);
      VERIFY(queue->entity[1].item[0].answ.ptrs[1] == ASC_NO_ARG);
      asc_entity_dequeue(&test_ctx);
      VERIFY(queue->entity_cnt == 1);
      asc_entity_dequeue(&test_ctx);
      VERIFY(queue->entity_cnt == 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_urc_enqueue()/asc_urc_dequeue()") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);
      asc_urc_queue_t urc = {"+SMS", NULL};
      asc_urc_enqueue(&test_ctx, &urc);
      asc_urc_queue_t* urc_queue = _asc_get_urc_queue(&test_ctx);  
      VERIFY(strcmp(urc_queue->prefix, urc.prefix) == 0);
      urc = (asc_urc_queue_t){"+CMD", NULL};
      asc_urc_enqueue(&test_ctx, &urc);
      VERIFY(strcmp(urc_queue[1].prefix, urc.prefix) == 0);
      VERIFY(asc_urc_dequeue(&test_ctx, "+SMS"));
      VERIFY(urc_queue[0].prefix == NULL);
      VERIFY(asc_urc_dequeue(&test_ctx, "+CMD"));
      VERIFY(urc_queue[1].prefix == NULL);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_cmd_sscanf()") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);
      char test[128] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+GSN"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"%15[^\x0d]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t data = ringslice_initializer((uint8_t*)test, 128, 0, sizeof(test)-1);
      int res_d = _asc_cmd_sscanf(&data, queue->entity[0].item);
      VERIFY(res_d);
      VERIFY(strncmp((asc_mdl_rtd_t*){queue->entity[0].data}->modem_imei, test, 15) == 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_string_boolean_ops()") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      char test[] = "+GSN1234+TRASH+END";
      ringslice_t data = ringslice_initializer((uint8_t*)test, sizeof(test), 0, strlen(test));
      int res_d = _asc_string_boolean_ops(&data, "+GSN1234");
      VERIFY(res_d);
      res_d = _asc_string_boolean_ops(&data, "+GSN1234");
      VERIFY(res_d);
      res_d = _asc_string_boolean_ops(&data, "EMPTY|+GSN1234");
      VERIFY(res_d);
      res_d = _asc_string_boolean_ops(&data, "+GSN1234&+END");
      VERIFY(res_d);
      res_d = _asc_string_boolean_ops(&data, "+GSN1234&EMPTY");
      VERIFY(res_d <= 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(),  NULL NULL NULL  - unhandle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = {0};
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), REQ NULL NULL  - unhandle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_req_impl[128]  = "AT+TEST?";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = {0};
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), REQ RES NULL(expect) - handle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_res_impl[128]  = "\r\nOK\r\n";
      char rs_me_impl[128]   = "EMPTY";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), REQ RES NULL(no expect) - handle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL, NULL, ASC_NO_ARG),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, 0, NULL);
      VERIFY(res);
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_res_impl[128]  = "\r\nOK\r\n";
      char rs_me_impl[128]   = "EMPTY";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), REQ NULL DATA - handle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM(ASC_CMD_SAVE"AT+TEST?"ASC_CMD_CRLF, ASC_CMD_SAVE"+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_me_impl[128]   = "EMPTY";
      char rs_data_impl[128] = "+TEST: 523566, text";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), REQ RES(OK) DATA - handle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_data_impl[128] = "+TEST: 523566, text";
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_res_impl[128]  = "\r\nOK\r\n";
      char rs_me_impl[128]   = "EMPTY";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d);
      VERIFY(strcmp((asc_mdl_rtd_t*){queue->entity[0].data}->modem_imei, "5235") == 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), REQ RES(ERROR) DATA  - handle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_data_impl[128] = "+TEST: 523566, text";
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_res_impl[128]  = "\r\nERROR\r\n";
      char rs_me_impl[128]   = "EMPTY";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), NULL RES NULL  - unhandle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_res_impl[128]  = "\r\nOK\r\n";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = {0};
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), NULL RES DATA  - unhandle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_res_impl[128]  = "\r\nOK\r\n";
      char rs_data_impl[128] = "+TEST: 523566, text";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = {0};
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), NULL NULL DATA(expect)  - handle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM(NULL, "+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_data_impl[128] = "+TEST: 523566, text";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = {0};
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d);
      VERIFY(strcmp((asc_mdl_rtd_t*){queue->entity[0].data}->modem_imei, "5235") == 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_post_proc(), NULL NULL DATA(no expect)  - handle state") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL, NULL, ASC_NO_ARG),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, 0, NULL);
      VERIFY(res);
      char rs_data_impl[128] = "+TEST: 523566, text";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = {0};
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      int res_d = _asc_simcom_parcer_post_proc(&test_ctx, &rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_find_rs_data() RES before DATA") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\nOK\r\n\r\n+TEST: 523566, text\r\nFFFFFFF";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4+strlen("AT+TEST?\r"), 4+strlen("AT+TEST?\r")+strlen("\r\nOK\r\n"));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _asc_simcom_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data);
      VERIFY(ringslice_is_empty(&rs_data) != 1);
      VERIFY(ringslice_strncmp(&rs_data, "+TEST: 523566", strlen("+TEST: 523566")) == 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_find_rs_data() RES after DATA") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nOK\r\nFFFFFFF";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 
                                                  4+strlen("AT+TEST?\r")+strlen("\r\n+TEST: 523566, text\r\n"),
                                                  4+strlen("AT+TEST?\r")+strlen("\r\n+TEST: 523566, text\r\n")+strlen("\r\nOK\r\n"));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _asc_simcom_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data);
      VERIFY(ringslice_is_empty(&rs_data) != 1);
      VERIFY(ringslice_strncmp(&rs_data, "+TEST: 523566", strlen("+TEST: 523566")) == 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_find_rs_data() no RES and REQ") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "\r\n+TEST: 523566, text\r\nFFFFFFF";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _asc_simcom_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data);
      VERIFY(ringslice_is_empty(&rs_data) != 1);
      VERIFY(ringslice_strncmp(&rs_data, "+TEST: 523566", strlen("+TEST: 523566")) == 0);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_find_rs_data() no RES") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\nFFFFFFF";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _asc_simcom_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data);
      VERIFY(ringslice_is_empty(&rs_data) != 1);
      VERIFY(ringslice_strncmp(&rs_data, "+TEST: 523566", strlen("+TEST: 523566")) == 0);   
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_simcom_parcer_find_rs_res() OK") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nOK\r\nFFFFFFF";
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _asc_simcom_parcer_find_rs_res(&rs_me, &rs_req, &rs_res);
      VERIFY(ringslice_is_empty(&rs_res) != 1);
      VERIFY(ringslice_strcmp(&rs_res, "\r\nOK\r\n") == 0);   
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_simcom_parcer_find_rs_res() ERROR") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nERROR\r\nFFFFFFF";
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _asc_simcom_parcer_find_rs_res(&rs_me, &rs_req, &rs_res);
      VERIFY(ringslice_is_empty(&rs_res) != 1);
      VERIFY(ringslice_strcmp(&rs_res, "\r\nERROR\r\n") == 0);   
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_simcom_parcer_find_rs_req()") {
      asc_init(&test_ctx, test_printf, test_write, &asc_ring_buffer);      
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128] = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nOK\r\nFFFFFFF";
      ringslice_t rs_req  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _asc_simcom_parcer_find_rs_req(&rs_me, &rs_req, items[0].req);
      VERIFY(ringslice_is_empty(&rs_req) != 1);
      VERIFY(ringslice_strcmp(&rs_req, "AT+TEST?\r") == 0);   
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() SIMCOM format RES after data") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nOK\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };

      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, "+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() SIMCOM format RES before data") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\nOK\r\n\r\n+TEST: 523566, text\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };

      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, "+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() SIMCOM format RES before data without prefix check") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\nOK\r\n\r\n+TEST: 523566, text\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() SIMCOM no format no prefix, just sending and wait echo with OK") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\nOK\r\n\r\n+TEST: 523566, text\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 0, 1, NULL, NULL, ASC_NO_ARG),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, 0, test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() SIMCOM full check without RES") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);;

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST?"ASC_CMD_CRLF, "+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() SIMCOM full check without RES and REQ") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM(NULL, "+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() SIMCOM no Item Req, only answer") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM(NULL, "+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() RAW with req") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST=1", "+TEST", ASC_PARCE_RAW, 2, 150, 0, 1, testItemCB, "\r\n+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_cmd_ring_parcer() RAW no req") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM(NULL, "+TEST", ASC_PARCE_RAW, 2, 150, 0, 1, testItemCB,"\r\n+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }  

  TEST("asc_cmd_ring_parcer() RAW No prefix and arg") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM("AT+TEST=1", NULL, ASC_PARCE_RAW, 2, 150, 0, 1, NULL, NULL, ASC_NO_ARG),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), NULL, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      ringslice_t rs_me = ringslice_initializer((uint8_t*)parce_buffer, 2048, parce_buffer_tail, parce_buffer_head);
      int res_p = _asc_cmd_ring_parcer(&test_ctx, &queue->entity[0], &queue->entity->item[0], rs_me);
      VERIFY(res_p);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }   

  TEST("asc_process_urcs()") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_urc_queue_t urc = {"+TEST", testUrcCB};
      asc_urc_enqueue(&test_ctx, &urc);
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)parce_buffer, 2048, 0, strlen(parce_buffer));
      _asc_process_urcs(&test_ctx, &rs_me);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

  TEST("asc_core_proc() first cmd fail, second success") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);
      asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ASC_ITEM(ASC_CMD_SAVE"AT+GSN"ASC_CMD_CRLF, NULL, ASC_PARCE_SIMCOM, 2, 150, 1, 1, NULL, NULL, ASC_NO_ARG),
        ASC_ITEM(NULL, "+TEST", ASC_PARCE_SIMCOM, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ASC_ARG(asc_mdl_rtd_t, modem_imei)),
      };
      bool res = asc_entity_enqueue(&test_ctx, items, sizeof(items)/sizeof(items[0]), testEntityCB, sizeof(asc_mdl_rtd_t), test_buffer);
      VERIFY(res);
      asc_entity_queue_t* queue =_asc_get_entity_queue(&test_ctx);
      while(queue->entity_cnt)
      {
        _asc_core_proc(&test_ctx);
      }
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }
  } //ASC_CORE=====================================================================

  { //ASC_CHAIN====================================================================
    TEST("asc_chain_create() simple straight chain") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);

      chain_step_t server_steps[] = 
      {   
        ASC_CHAIN("MODEM INIT",        "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("GPRS INIT",         "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("SOCKET CONFIG",     "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("SOCKET CONNECT",    "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("SOCKET DISCONNECT", "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("GPRS DEINIT",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("MODEM RESET",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
      };
      asc_chain_t* chain = asc_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t), &test_ctx);
      VERIFY(chain->step_count == sizeof(server_steps)/sizeof(chain_step_t));
      asc_chain_start(chain);
      VERIFY(chain->is_running);
      while(asc_chain_is_running(chain))
      {
        bool res = asc_chain_run(chain);
        if(asc_chain_is_running(chain)) VERIFY(res);
      }
      asc_chain_destroy(chain);
      asc_deinit(&test_ctx);
      VERIFY(!_asc_get_init(&test_ctx).init);
    }

    TEST("asc_chain_create() one loop") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);

      chain_step_t server_steps[] = 
      {   
        ASC_CHAIN("MODEM INIT",     "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("GPRS INIT",      "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("SOCKET CONFIG",  "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("SOCKET CONNECT", "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),

        ASC_CHAIN_LOOP_START(5), 
          ASC_CHAIN("MODEM RTD",          "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
          ASC_CHAIN("SOCKET SEND RECIVE", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN_LOOP_END,

        ASC_CHAIN("SOCKET DISCONNECT", "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("GPRS DEINIT",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("MODEM RESET",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
      };
      asc_chain_t* chain = asc_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t), &test_ctx);
      VERIFY(chain->step_count == sizeof(server_steps)/sizeof(chain_step_t));
      asc_chain_start(chain);
      VERIFY(chain->is_running);
      while(asc_chain_is_running(chain))
      {
        bool res = asc_chain_run(chain);
        if(asc_chain_is_running(chain)) VERIFY(res);
      }
      asc_chain_destroy(chain);
      asc_deinit(&test_ctx);
      VERIFY(!asc_get_init(&test_ctx).init);
    }

    TEST("asc_chain_create() nested loops with delay, exec and prev") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);

      chain_step_t server_steps[] = 
      {   
        ASC_CHAIN("MODEM INIT",     "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("GPRS INIT",      "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("SOCKET CONFIG",  "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("SOCKET CONNECT", "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),

        ASC_CHAIN_LOOP_START(3), 
            ASC_CHAIN("MODEM RTD", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
            ASC_CHAIN_LOOP_START(3), 
                ASC_CHAIN("MODEM RTD", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
                ASC_CHAIN_DELAY(50),
                ASC_CHAIN("SOCKET SEND RECIVE", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
            ASC_CHAIN_LOOP_END,
            ASC_CHAIN("SOCKET SEND RECIVE", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN_LOOP_END,

        ASC_CHAIN_EXEC("CHECK CONN", "NEXT", "MODEM RESET", testChainCond),
        ASC_CHAIN("SOCKET DISCONNECT", "NEXT", "PREV", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("GPRS DEINIT",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("MODEM RESET",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
      };

      asc_chain_t* chain = asc_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t), &test_ctx);
      VERIFY(chain->step_count == sizeof(server_steps)/sizeof(chain_step_t));
      asc_chain_start(chain);
      VERIFY(chain->is_running);
      while(asc_chain_is_running(chain))
      {
        bool res = asc_chain_run(chain);
        if(asc_chain_is_running(chain)) VERIFY(res);
      }
      asc_chain_destroy(chain);
      asc_deinit(&test_ctx);
      VERIFY(!asc_get_init(&test_ctx).init);
    }

    TEST("asc_chain_create() nested loop with backward and forward falltrough") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      asc_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      asc_init(&test_ctx, test_printf, test_write, &ring);

      chain_step_t server_steps[] = 
      {   
        ASC_CHAIN("1",     "4", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("2",      "3", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),

        ASC_CHAIN_LOOP_START(3), 
            ASC_CHAIN("3", "5", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
            ASC_CHAIN_LOOP_START(3), 
                ASC_CHAIN("4", "2", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
            ASC_CHAIN_LOOP_END,
        ASC_CHAIN_LOOP_END,

        ASC_CHAIN_EXEC("5",  "NEXT", "MODEM RESET", testChainCond),
        ASC_CHAIN("6",       "NEXT", "PREV", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("7",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ASC_CHAIN("8",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
      };

      asc_chain_t* chain = asc_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t), &test_ctx);
      VERIFY(chain->step_count == sizeof(server_steps)/sizeof(chain_step_t));
      asc_chain_start(chain);
      VERIFY(chain->is_running);
      while(asc_chain_is_running(chain))
      {
        bool res = asc_chain_run(chain);
        if(asc_chain_is_running(chain)) VERIFY(res);
      }
      VERIFY(chain->loop_stack_ptr == 0);
      asc_chain_destroy(chain);
      asc_deinit(&test_ctx);
      VERIFY(!asc_get_init(&test_ctx).init);
    }

  } //ASC_CHAIN====================================================================

} // TEST_GROUP()
