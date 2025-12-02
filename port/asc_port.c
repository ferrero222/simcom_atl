/******************************************************************************
 *                              _    ____   ____                              *
 *                   ======    / \  / ___| / ___| ======       (c)03.10.2025  *
 *                   ======   / _ \ \___ \| |     ======           v1.0.0     *
 *                   ======  / ___ \ ___) | |___  ======                      *
 *                   ====== /_/   \_\____/ \____| ======                      *  
 *                                                                            *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "asc_port.h"
#include "asc_core.h"

#ifndef ASC_TEST
  #include "stdarg.h"
  #include <stdio.h>
  #include "hc32f460_utility.h"
#endif

/*******************************************************************************
 * Config
 ******************************************************************************/
/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** @brief  DBC fault override
 ** @param  none
 ** @return none
 ******************************************************************************/
DBC_NORETURN void DBC_fault_handler(char const* module, int label) 
{
  (void)module;
  (void)label;
  while (1) 
  {
    /* Typically you would trigger a system reset or safe state here */
  }
}

/*******************************************************************************
 ** @brief  Weak function to enter into critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
static void asc_crit_enter(void) 
{ 
  //__disable_irq();
}

/*******************************************************************************
 ** @brief  Weak function to exit critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
static void asc_crit_exit(void)  
{
  //__enable_irq();
}

/*******************************************************************************
 ** @brief  Printf
 ** @param  none
 ** @return none
 ******************************************************************************/
#ifndef ASC_TEST
void asc_printf_safe(asc_context_t* const ctx, const char *fmt, ...) 
{
  if(!ctx) return;
  asc_init_t atl = asc_get_init(ctx); 
  if(!atl.asc_printf) return;
  va_list args;
  va_start(args, fmt);
  char buffer[512];
  int total_processed = 0;
  va_list args_len;
  va_copy(args_len, args);
  int total_len = vsnprintf(NULL, 0, fmt, args_len);
  va_end(args_len);
  if(total_len < 0) 
  {
    va_end(args);
    return;
  }
  while(total_processed < total_len) 
  {
    int fragment_len = vsnprintf(buffer, sizeof(buffer), fmt + total_processed, args);
    if(fragment_len <= 0) break;
    
    char escaped[sizeof(buffer)]; 
    int escaped_pos = 0;
    
    for(int i = 0; i < fragment_len && buffer[i] != '\0'; i++) 
    {
      char c = buffer[i];
      switch (c) 
      {
        case '\r': if(escaped_pos < sizeof(escaped)-3) { escaped[escaped_pos++] = '\\'; escaped[escaped_pos++] = 'r';  } break;
        case '\n': if(escaped_pos < sizeof(escaped)-3) { escaped[escaped_pos++] = '\\'; escaped[escaped_pos++] = 'n';  } break;
        case '\t': if(escaped_pos < sizeof(escaped)-3) { escaped[escaped_pos++] = '\\'; escaped[escaped_pos++] = 't';  } break;
        case '\\': if(escaped_pos < sizeof(escaped)-3) { escaped[escaped_pos++] = '\\'; escaped[escaped_pos++] = '\\'; } break;
        default: 
            if (c >= 32 && c <= 126) 
            {
              if(escaped_pos < sizeof(escaped)-1) escaped[escaped_pos++] = c;
            } 
            else 
            {
              if(escaped_pos < sizeof(escaped)-5) 
              {
                escaped[escaped_pos++] = '\\';
                escaped[escaped_pos++] = 'x';
                escaped[escaped_pos++] = "0123456789ABCDEF"[(c >> 4) & 0xF];
                escaped[escaped_pos++] = "0123456789ABCDEF"[c & 0xF];
              }
            }
            break;
      }
    }
    
    if(escaped_pos > 0) 
    {
      if(escaped_pos < sizeof(escaped)-2) 
      {
        escaped[escaped_pos++] = '\n';
        escaped[escaped_pos] = '\0';
      } 
      else 
      {
        escaped[sizeof(escaped)-2] = '\n';
        escaped[sizeof(escaped)-1] = '\0';
      }
      atl.asc_printf(escaped);
    }
    total_processed += fragment_len;
    va_end(args);
    va_start(args, fmt);
  }
  va_end(args);
}

/*******************************************************************************
 ** @brief  Printf
 ** @param  none
 ** @return none
 ******************************************************************************/
void asc_printf_from_ring(asc_context_t* const ctx, ringslice_t rs_me, char* text)
{
  int data_len = ringslice_len(&rs_me);
  int wrap_len = (rs_me.first + data_len > rs_me.buf_size) ? rs_me.first + data_len - rs_me.buf_size : 0;
  int first_len = data_len - wrap_len;
  ASC_DEBUG(ctx, "[ASC][INFO] %s %.*s%.*s", text, first_len, &rs_me.buf[rs_me.first], wrap_len, &rs_me.buf[0]);
}

#endif

/*******************************************************************************
 ** @brief  Critical handlers
 ** @param  none
 ** @return none
 ******************************************************************************/
#ifndef ASC_TEST
static volatile uint32_t asc_crit_counter = 0;
#endif

void _asc_crit_enter(void) 
{ 
  #ifndef ASC_TEST
  asc_crit_enter();
  asc_crit_counter++;
  #endif
}

void _asc_crit_exit(void)  
{
  #ifndef ASC_TEST
  if(asc_crit_counter > 0) 
  {
    asc_crit_counter--;
    if (asc_crit_counter == 0) asc_crit_exit();
  }
  #endif
}
