//============================================================================
// ET: embedded test; very simple test example
//============================================================================
#include "et.h"  // ET: embedded test

#include "atl_core.h" // 
#include "o1heap.h" // 
#include "atl_port.h" // 
#include "atl_chain.h"  // ET: embedded test
#include "atl_mdl_general.h"
#include <stdio.h>

uint8_t test_buffer[2048] = {0};
const uint16_t test_buffer_head = 1024;
const uint16_t test_buffer_tail = 0;

atl_ring_buffer_t atl_ring_buffer = {
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
  atl_mdl_rtd_t* real_data = (atl_mdl_rtd_t*)data;
  VERIFY(strcmp(real_data->modem_imei, "5235") == 0);
}

void testEntityCB(const bool result, void* const ctx, const void* const data)
{
  VERIFY(result);
  VERIFY(ctx == test_buffer);
  atl_mdl_rtd_t* real_data = (atl_mdl_rtd_t*)data;
  VERIFY(strcmp(real_data->modem_imei, "5235") == 0);
}

void testUrcCB(ringslice_t urc_slice)
{
  VERIFY(ringslice_strcmp(&urc_slice, "+TEST") == 0);
}

bool testChainFunc(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  VERIFY(param == test_buffer);

  atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM(NULL, "+TEST", 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
  };
  bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, sizeof(atl_mdl_rtd_t), ctx);
  VERIFY(res);
  atl_entity_queue_t* queue =_atl_get_entity_queue();
  while(queue->entity_cnt)
  {
    _atl_core_proc();
  }
  return res;
}

bool testChainCond(void)
{
  return true;
}

// test group ----------------------------------------------------------------
TEST_GROUP("ATL") {

  { //ATL_CORE=====================================================================
    TEST("o1heap lib") {
      O1HeapInstance* heap = o1heapInit(test_buffer, ATL_MEMORY_POOL_SIZE); 
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

    TEST("atl_init()/atl_deinit() testing") {
      atl_init(test_printf, test_write, &atl_ring_buffer);
      VERIFY(_atl_get_init().init);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_entity_enqueue()/atl_entity_dequeue() simple AT`s") {
      atl_init(test_printf, test_write, &atl_ring_buffer);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT"ATL_CMD_CRLF,   NULL, 2, 150, 0, 1, NULL, NULL, ATL_NO_ARG),
        ATL_ITEM("AT"ATL_CMD_CRLF,   NULL, 2, 150, 0, 1, NULL, NULL, ATL_NO_ARG),  
        ATL_ITEM("ATE1"ATL_CMD_CRLF, NULL, 2, 600, 0, 0, NULL, NULL, ATL_NO_ARG),  
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, 0, NULL);
      VERIFY(res);
      res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, 0, NULL);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      VERIFY(queue->entity_cnt == 2);
      VERIFY(queue->entity[0].item_cnt == sizeof(items)/sizeof(items[0]));
      VERIFY(queue->entity[1].item_cnt == sizeof(items)/sizeof(items[0]));
      atl_entity_dequeue();
      VERIFY(queue->entity_cnt == 1);
      atl_entity_dequeue();
      VERIFY(queue->entity_cnt == 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_entity_enqueue()/atl_entity_dequeue() composite AT`s") {
      atl_init(test_printf, test_write, &atl_ring_buffer);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM(ATL_CMD_SAVE"AT+GSN"ATL_CMD_CRLF,    NULL, 2, 150, 0, 1, NULL,               "%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
        ATL_ITEM(ATL_CMD_SAVE"AT+GMM"ATL_CMD_CRLF,    NULL, 2, 150, 0, 1, NULL,               "%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_id)),  
        ATL_ITEM("AT+GMR"ATL_CMD_CRLF,                NULL, 2, 150, 0, 1, NULL,      "Revision:%29[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_rev)),                
        ATL_ITEM("AT+CCLK?"ATL_CMD_CRLF,              NULL, 2, 150, 0, 1, NULL,      "+CCLK: \"%21[^\"]\"", ATL_ARG(atl_mdl_rtd_t, modem_clock)),          
        ATL_ITEM("AT+CCID"ATL_CMD_CRLF,            "+CCLK", 2, 150, 0, 1, NULL,               "%21[^\x0d]", ATL_ARG(atl_mdl_rtd_t, sim_iccid)),             
        ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,           "+COPS", 2, 150, 0, 1, NULL, "+COPS: 0, 0,\"%49[^\"]\"", ATL_ARG(atl_mdl_rtd_t, sim_operator)),            
        ATL_ITEM("AT+CSQ"ATL_CMD_CRLF,              "+CSQ", 2, 150, 0, 1, NULL,                 "+CSQ: %d", ATL_ARG(atl_mdl_rtd_t, sim_rssi)),             
        ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,             NULL, 2, 600, 0, 1, NULL,                       NULL, ATL_NO_ARG),                
        ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,              NULL, 2, 600, 0, 0, NULL,                       NULL, ATL_NO_ARG),  
      };         
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      VERIFY(queue->entity_cnt == 2);
      VERIFY(queue->entity[0].item_cnt == sizeof(items)/sizeof(items[0]));
      VERIFY(queue->entity[1].item_cnt == sizeof(items)/sizeof(items[0]));
      VERIFY(queue->entity[0].data);
      VERIFY(queue->entity[1].data);
      VERIFY((atl_mdl_rtd_t*){queue->entity[0].data}->modem_clock == queue->entity[0].item[3].answ.ptrs[0]);
      VERIFY(queue->entity[0].item[0].answ.ptrs[1] == ATL_NO_ARG);
      VERIFY((atl_mdl_rtd_t*){queue->entity[1].data}->sim_iccid == queue->entity[1].item[4].answ.ptrs[0]);
      VERIFY(queue->entity[1].item[0].answ.ptrs[1] == ATL_NO_ARG);
      atl_entity_dequeue();
      VERIFY(queue->entity_cnt == 1);
      atl_entity_dequeue();
      VERIFY(queue->entity_cnt == 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_urc_enqueue()/atl_urc_dequeue()") {
      atl_init(test_printf, test_write, &atl_ring_buffer);
      atl_urc_queue_t urc = {"+SMS", NULL};
      atl_urc_enqueue(&urc);
      atl_urc_queue_t* urc_queue = _atl_get_urc_queue();  
      VERIFY(strcmp(urc_queue->prefix, urc.prefix) == 0);
      urc = (atl_urc_queue_t){"+CMD", NULL};
      atl_urc_enqueue(&urc);
      VERIFY(strcmp(urc_queue[1].prefix, urc.prefix) == 0);
      VERIFY(atl_urc_dequeue("+SMS"));
      VERIFY(urc_queue[0].prefix == NULL);
      VERIFY(atl_urc_dequeue("+CMD"));
      VERIFY(urc_queue[1].prefix == NULL);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_cmd_sscanf()") {
      atl_init(test_printf, test_write, &atl_ring_buffer);
      char test[128] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+GSN"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"%15[^\x0d]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      ringslice_t data = ringslice_initializer((uint8_t*)test, 128, 0, sizeof(test)-1);
      int res_d = _atl_cmd_sscanf(&data, queue->entity[0].item);
      VERIFY(res_d);
      VERIFY(strncmp((atl_mdl_rtd_t*){queue->entity[0].data}->modem_imei, test, 15) == 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_string_boolean_ops()") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      char test[] = "+GSN1234+TRASH+END";
      ringslice_t data = ringslice_initializer((uint8_t*)test, sizeof(test), 0, strlen(test));
      int res_d = _atl_string_boolean_ops(&data, "+GSN1234");
      VERIFY(res_d);
      res_d = _atl_string_boolean_ops(&data, "+GSN1234");
      VERIFY(res_d);
      res_d = _atl_string_boolean_ops(&data, "EMPTY|+GSN1234");
      VERIFY(res_d);
      res_d = _atl_string_boolean_ops(&data, "+GSN1234&+END");
      VERIFY(res_d);
      res_d = _atl_string_boolean_ops(&data, "+GSN1234&EMPTY");
      VERIFY(res_d <= 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(),  NULL NULL NULL  - unhandle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = {0};
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), REQ NULL NULL  - unhandle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_req_impl[128]  = "AT+TEST?";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = {0};
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), REQ RES NULL(expect) - handle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_res_impl[128]  = "\r\nOK\r\n";
      char rs_me_impl[128]   = "EMPTY";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), REQ RES NULL(no expect) - handle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL, NULL, ATL_NO_ARG),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, 0, NULL);
      VERIFY(res);
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_res_impl[128]  = "\r\nOK\r\n";
      char rs_me_impl[128]   = "EMPTY";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), REQ NULL DATA - handle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_me_impl[128]   = "EMPTY";
      char rs_data_impl[128] = "+TEST: 523566, text";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), REQ RES(OK) DATA - handle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_data_impl[128] = "+TEST: 523566, text";
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_res_impl[128]  = "\r\nOK\r\n";
      char rs_me_impl[128]   = "EMPTY";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d);
      VERIFY(strcmp((atl_mdl_rtd_t*){queue->entity[0].data}->modem_imei, "5235") == 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), REQ RES(ERROR) DATA  - handle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_data_impl[128] = "+TEST: 523566, text";
      char rs_req_impl[128]  = "AT+TEST?";
      char rs_res_impl[128]  = "\r\nERROR\r\n";
      char rs_me_impl[128]   = "EMPTY";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_req_impl, 128, 0, strlen(rs_req_impl));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), NULL RES NULL  - unhandle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_res_impl[128]  = "\r\nOK\r\n";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = {0};
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), NULL RES DATA  - unhandle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_res_impl[128]  = "\r\nOK\r\n";
      char rs_data_impl[128] = "+TEST: 523566, text";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_res_impl, 128, 0, strlen(rs_res_impl));
      ringslice_t rs_me   = {0};
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), NULL NULL DATA(expect)  - handle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM(NULL, "+TEST", 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_data_impl[128] = "+TEST: 523566, text";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = {0};
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d);
      VERIFY(strcmp((atl_mdl_rtd_t*){queue->entity[0].data}->modem_imei, "5235") == 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_post_proc(), NULL NULL DATA(no expect)  - handle state") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL, NULL, ATL_NO_ARG),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, 0, NULL);
      VERIFY(res);
      char rs_data_impl[128] = "+TEST: 523566, text";
      ringslice_t rs_data = ringslice_initializer((uint8_t*)rs_data_impl, 128, 0, strlen(rs_data_impl));
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = {0};
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_d = _atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, &queue->entity[0].item[0], &queue->entity[0]);
      VERIFY(res_d <= 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_find_rs_data() RES before DATA") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\nOK\r\n\r\n+TEST: 523566, text\r\nFFFFFFF";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4+strlen("AT+TEST?\r"), 4+strlen("AT+TEST?\r")+strlen("\r\nOK\r\n"));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _atl_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data);
      VERIFY(ringslice_is_empty(&rs_data) != 1);
      VERIFY(ringslice_strncmp(&rs_data, "+TEST: 523566", strlen("+TEST: 523566")) == 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_find_rs_data() RES after DATA") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nOK\r\nFFFFFFF";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 
                                                  4+strlen("AT+TEST?\r")+strlen("\r\n+TEST: 523566, text\r\n"),
                                                  4+strlen("AT+TEST?\r")+strlen("\r\n+TEST: 523566, text\r\n")+strlen("\r\nOK\r\n"));
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _atl_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data);
      VERIFY(ringslice_is_empty(&rs_data) != 1);
      VERIFY(ringslice_strncmp(&rs_data, "+TEST: 523566", strlen("+TEST: 523566")) == 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_find_rs_data() no RES and REQ") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "\r\n+TEST: 523566, text\r\nFFFFFFF";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = {0};
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _atl_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data);
      VERIFY(ringslice_is_empty(&rs_data) != 1);
      VERIFY(ringslice_strncmp(&rs_data, "+TEST: 523566", strlen("+TEST: 523566")) == 0);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_find_rs_data() no RES") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\nFFFFFFF";
      ringslice_t rs_data = {0};
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _atl_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data);
      VERIFY(ringslice_is_empty(&rs_data) != 1);
      VERIFY(ringslice_strncmp(&rs_data, "+TEST: 523566", strlen("+TEST: 523566")) == 0);   
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_parcer_find_rs_res() OK") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nOK\r\nFFFFFFF";
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _atl_parcer_find_rs_res(&rs_me, &rs_req, &rs_res);
      VERIFY(ringslice_is_empty(&rs_res) != 1);
      VERIFY(ringslice_strcmp(&rs_res, "\r\nOK\r\n") == 0);   
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_parcer_find_rs_res() ERROR") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128]   = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nERROR\r\nFFFFFFF";
      ringslice_t rs_req  = ringslice_initializer((uint8_t*)rs_me_impl, 128, 4, 4+strlen("AT+TEST?\r"));
      ringslice_t rs_res  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _atl_parcer_find_rs_res(&rs_me, &rs_req, &rs_res);
      VERIFY(ringslice_is_empty(&rs_res) != 1);
      VERIFY(ringslice_strcmp(&rs_res, "\r\nERROR\r\n") == 0);   
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_parcer_find_rs_req()") {
      atl_init(test_printf, test_write, &atl_ring_buffer);      
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
      VERIFY(res);
      char rs_me_impl[128] = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nOK\r\nFFFFFFF";
      ringslice_t rs_req  = {0};
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)rs_me_impl, 128, 0, strlen(rs_me_impl));
      _atl_parcer_find_rs_req(&rs_me, &rs_req, items[0].req);
      VERIFY(ringslice_is_empty(&rs_req) != 1);
      VERIFY(ringslice_strcmp(&rs_req, "AT+TEST?\r") == 0);   
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_cmd_ring_parcer() format RES after data") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\n\r\nOK\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };

      atl_init(test_printf, test_write, &ring);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, "+TEST", 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), test_buffer);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_p = _atl_cmd_ring_parcer(&queue->entity[0], &queue->entity->item[0]);
      VERIFY(res_p);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_cmd_ring_parcer() format RES before data") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\nOK\r\n\r\n+TEST: 523566, text\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };

      atl_init(test_printf, test_write, &ring);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, "+TEST", 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), test_buffer);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_p = _atl_cmd_ring_parcer(&queue->entity[0], &queue->entity->item[0]);
      VERIFY(res_p);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_cmd_ring_parcer() format RES before data without prefix check") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\nOK\r\n\r\n+TEST: 523566, text\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), test_buffer);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_p = _atl_cmd_ring_parcer(&queue->entity[0], &queue->entity->item[0]);
      VERIFY(res_p);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_cmd_ring_parcer() no format no prefix, just sending and wait echo with OK") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\nOK\r\n\r\n+TEST: 523566, text\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, NULL, 2, 150, 0, 1, NULL, NULL, ATL_NO_ARG),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, 0, test_buffer);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_p = _atl_cmd_ring_parcer(&queue->entity[0], &queue->entity->item[0]);
      VERIFY(res_p);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_cmd_ring_parcer() full check without RES") {
      char parce_buffer[2048] = "FFFFAT+TEST?\r\r\n+TEST: 523566, text\r\nFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);;

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM("AT+TEST?"ATL_CMD_CRLF, "+TEST", 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), test_buffer);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_p = _atl_cmd_ring_parcer(&queue->entity[0], &queue->entity->item[0]);
      VERIFY(res_p);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_cmd_ring_parcer() full check without RES and REQ") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM(NULL, "+TEST", 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), test_buffer);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_p = _atl_cmd_ring_parcer(&queue->entity[0], &queue->entity->item[0]);
      VERIFY(res_p);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_cmd_ring_parcer() no Item Req, only answer") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM(NULL, "+TEST", 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), test_buffer);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      int res_p = _atl_cmd_ring_parcer(&queue->entity[0], &queue->entity->item[0]);
      VERIFY(res_p);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_parcer_process_urcs()") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);
      atl_urc_queue_t urc = {"+TEST", testUrcCB};
      atl_urc_enqueue(&urc);
      ringslice_t rs_me   = ringslice_initializer((uint8_t*)parce_buffer, 2048, 0, strlen(parce_buffer));
      _atl_parcer_process_urcs(&rs_me);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

  TEST("atl_core_proc() first cmd fail, second success") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);
      atl_item_t items[] = //[REQ][PREFIX][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
      {
        ATL_ITEM(ATL_CMD_SAVE"AT+GSN"ATL_CMD_CRLF, NULL, 2, 150, 1, 1, NULL, NULL, ATL_NO_ARG),
        ATL_ITEM(NULL, "+TEST", 2, 150, 0, 1, testItemCB,"+TEST: %4[^,]", ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      };
      bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), testEntityCB, sizeof(atl_mdl_rtd_t), test_buffer);
      VERIFY(res);
      atl_entity_queue_t* queue =_atl_get_entity_queue();
      while(queue->entity_cnt)
      {
        _atl_core_proc();
      }
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }
  } //ATL_CORE=====================================================================

  { //ATL_CHAIN====================================================================
    TEST("atl_chain_create() simple straight chain") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);

      chain_step_t server_steps[] = 
      {   
        ATL_CHAIN("MODEM INIT",        "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("GPRS INIT",         "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("SOCKET CONFIG",     "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("SOCKET CONNECT",    "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("SOCKET DISCONNECT", "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("GPRS DEINIT",       "NEXT",     "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("MODEM RESET",       "NEXT",     "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
      };
      atl_chain_t* chain = atl_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t));
      VERIFY(chain->step_count == sizeof(server_steps)/sizeof(chain_step_t));
      atl_chain_start(chain);
      VERIFY(chain->is_running);
      while(atl_chain_is_running(chain))
      {
        bool res = atl_chain_run(chain);
        if(atl_chain_is_running(chain)) VERIFY(res);
      }
      atl_chain_destroy(chain);
      atl_deinit();
      VERIFY(!_atl_get_init().init);
    }

    TEST("atl_chain_create() one loop") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);

      chain_step_t server_steps[] = 
      {   
        ATL_CHAIN("MODEM INIT",     "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("GPRS INIT",      "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("SOCKET CONFIG",  "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("SOCKET CONNECT", "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),

        ATL_CHAIN_LOOP_START(5), 
//      {
          ATL_CHAIN("MODEM RTD",          "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
          ATL_CHAIN("SOCKET SEND RECIVE", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
          ATL_CHAIN_LOOP_END,
//      }
        ATL_CHAIN("SOCKET DISCONNECT", "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("GPRS DEINIT",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("MODEM RESET",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
      };
      atl_chain_t* chain = atl_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t));
      VERIFY(chain->step_count == sizeof(server_steps)/sizeof(chain_step_t));
      atl_chain_start(chain);
      VERIFY(chain->is_running);
      while(atl_chain_is_running(chain))
      {
        bool res = atl_chain_run(chain);
        if(atl_chain_is_running(chain)) VERIFY(res);
      }
      atl_chain_destroy(chain);
      atl_deinit();
      VERIFY(!atl_get_init().init);
    }

    TEST("atl_chain_create() nested loops with delay, condition and prev") {
      char parce_buffer[2048] = "\r\n+TEST: 523566, text\r\nFFFFFFFFFFF";
      uint16_t parce_buffer_tail = 0;
      uint16_t parce_buffer_head = strlen(parce_buffer);

      atl_ring_buffer_t ring = {
        .buffer = (uint8_t*)parce_buffer,
        .count = 0,
        .head = parce_buffer_head,
        .tail = parce_buffer_tail,
        .size = 2048,
      };
      atl_init(test_printf, test_write, &ring);

      chain_step_t server_steps[] = 
      {   
        ATL_CHAIN("MODEM INIT",     "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("GPRS INIT",      "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("SOCKET CONFIG",  "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("SOCKET CONNECT", "NEXT", "GPRS DEINIT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),

        ATL_CHAIN_LOOP_START(3), 
//      {
          ATL_CHAIN("MODEM RTD", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
          ATL_CHAIN_LOOP_START(3), 
//        {
            ATL_CHAIN("MODEM RTD", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
            ATL_CHAIN_DELAY(50),
            ATL_CHAIN("SOCKET SEND RECIVE", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
            ATL_CHAIN_LOOP_END,
//        {        
          ATL_CHAIN("SOCKET SEND RECIVE", "NEXT", "SOCKET DISCONNECT", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
          ATL_CHAIN_LOOP_END,
//      }
        ATL_CHAIN_COND("CHECK CONN", "NEXT", "MODEM RESET", testChainCond),
        ATL_CHAIN("SOCKET DISCONNECT", "NEXT", "PREV", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("GPRS DEINIT",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
        ATL_CHAIN("MODEM RESET",       "NEXT", "MODEM RESET", testChainFunc, testEntityCB, test_buffer, test_buffer, 3),
      };

      atl_chain_t* chain = atl_chain_create("TCP", server_steps, sizeof(server_steps)/sizeof(chain_step_t));
      VERIFY(chain->step_count == sizeof(server_steps)/sizeof(chain_step_t));
      atl_chain_start(chain);
      VERIFY(chain->is_running);
      while(atl_chain_is_running(chain))
      {
        bool res = atl_chain_run(chain);
        if(atl_chain_is_running(chain)) VERIFY(res);
      }
      atl_chain_destroy(chain);
      atl_deinit();
      VERIFY(!atl_get_init().init);
    }
  } //ATL_CHAIN====================================================================

} // TEST_GROUP()
