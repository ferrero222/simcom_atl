//============================================================================
// ET: embedded test; very simple test example
//============================================================================
#include "et.h"  // ET: embedded test

#include "atl_core.h" // Code Under Test (CUT)
#include "tlsf.h" // Code Under Test (CUT)
#include "atl_port.h" // Code Under Test (CUT)
#include "atl_chain.h"  // ET: embedded test
#include "atl_mdl_general.h"
#include <stdio.h>

uint8_t test_buffer[2048] = {0};
uint16_t test_buffer_head = 0;
uint16_t test_buffer_tail = 1024;

uint16_t test_write(uint8_t* buff, uint16_t len) {
  (void)buff;
  (void)len;
  return 1;
}


void setup(void) {
    // executed before *every* non-skipped test
}

void teardown(void) {
    // executed after *every* non-skipped and non-failing test
}

// test group ----------------------------------------------------------------
TEST_GROUP("Basic") {

TEST("TLSF testing") {
  	// printf("..%d, %d, %d, %d, %d, %d..", 
    //        (int)tlsf_size(), (int)tlsf_align_size(), (int)tlsf_block_size_min(), 
    //        (int)tlsf_block_size_max(), (int)tlsf_pool_overhead(), (int)tlsf_alloc_overhead());
    tlsf_t tlsf = tlsf_create_with_pool(test_buffer, ATL_MEMORY_POOL_SIZE); 
    void* ptr1 = tlsf_malloc(tlsf, 111);
    void* ptr2 = tlsf_malloc(tlsf, 345);
    tlsf_free(tlsf, ptr1);
    tlsf_free(tlsf, ptr2);
    VERIFY(tlsf_check(tlsf) == 0);
    tlsf_destroy(tlsf);
}

TEST("atl_init()/atl_deinit() testing") {
    atl_init(printf, test_write, test_buffer, 2048, &test_buffer_tail, &test_buffer_head);
    VERIFY(atl_get_init().init);
    atl_deinit();
    VERIFY(!atl_get_init().init);
}

TEST("atl_entity_enqueue()/atl_entity_dequeue() simple AT`s") {
    atl_init(printf, test_write, test_buffer, 2048, &test_buffer_tail, &test_buffer_head);
    atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
    {
      ATL_ITEM("AT"ATL_CMD_CRLF,   NULL, NULL, 2, 150, 0, 1, NULL, NULL),
      ATL_ITEM("AT"ATL_CMD_CRLF,   NULL, NULL, 2, 150, 0, 1, NULL, NULL),  
      ATL_ITEM("ATE1"ATL_CMD_CRLF, NULL, NULL, 2, 600, 0, 0, NULL, NULL),  
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
    VERIFY(tlsf_check(atl_get_init().atl_tlsf) == 0);
    atl_deinit();
    VERIFY(!atl_get_init().init);
}

TEST("atl_entity_enqueue()/atl_entity_dequeue() composite AT`s") {
    atl_init(printf, test_write, test_buffer, 2048, &test_buffer_tail, &test_buffer_head);
    atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
    {
      ATL_ITEM("AT+GSN"ATL_CMD_CRLF,       NULL,               "%15[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_imei)),
      ATL_ITEM("AT+GMM"ATL_CMD_CRLF,       NULL,               "%15[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_id)),  
      ATL_ITEM("AT+GMR"ATL_CMD_CRLF,       NULL,      "Revision:%29[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_rev)),                
      ATL_ITEM("AT+CCLK?"ATL_CMD_CRLF,     NULL,      "+CCLK: \"%21[^\"]\"", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, modem_clock)),          
      ATL_ITEM("AT+CCID"ATL_CMD_CRLF,   "+CCLK",               "%21[^\x0d]", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_iccid)),             
      ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,  "+COPS", "+COPS: 0, 0,\"%49[^\"]\"", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_operator)),            
      ATL_ITEM("AT+CSQ"ATL_CMD_CRLF,     "+CSQ",                 "+CSQ: %d", 2, 150, 0, 1, NULL, ATL_ARG(atl_mdl_rtd_t, sim_rssi)),             
      ATL_ITEM("AT+CENG=3"ATL_CMD_CRLF,    NULL,                       NULL, 2, 600, 0, 1, NULL, NULL),                
      ATL_ITEM("AT+CENG?"ATL_CMD_CRLF,     NULL,                       NULL, 2, 600, 0, 0, NULL, NULL),  
    };
    uint32_t test = offsetof(atl_mdl_rtd_t, modem_id);
    bool res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
    VERIFY(test);
    res = atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), NULL, sizeof(atl_mdl_rtd_t), NULL);
    VERIFY(res);
    atl_entity_queue_t* queue =_atl_get_entity_queue();
    VERIFY(queue->entity_cnt == 2);
    VERIFY(queue->entity[0].item_cnt == sizeof(items)/sizeof(items[0]));
    VERIFY(queue->entity[1].item_cnt == sizeof(items)/sizeof(items[0]));
    VERIFY(queue->entity[0].data);
    VERIFY(queue->entity[1].data);
    VERIFY((atl_mdl_rtd_t*){queue->entity[0].data}->modem_imei == queue->entity[0].item[0].answ.ptrs[0]);
    VERIFY(queue->entity[0].item->answ.ptrs[1] == NULL);
    VERIFY((atl_mdl_rtd_t*){queue->entity[1].data}->modem_imei == queue->entity[0].item[0].answ.ptrs[0]);
    VERIFY(queue->entity[1].item->answ.ptrs[1] == NULL);
    atl_entity_dequeue();
    VERIFY(queue->entity_cnt == 1);
    atl_entity_dequeue();
    VERIFY(queue->entity_cnt == 0);
    VERIFY(tlsf_check(atl_get_init().atl_tlsf) == 0);
    atl_deinit();
    VERIFY(!atl_get_init().init);
}

} // TEST_GROUP()
