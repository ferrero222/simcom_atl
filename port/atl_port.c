/*******************************************************************************
 *                           ╔══╗╔════╗╔╗──                      (c)03.10.2025 *
 *                           ║╔╗║╚═╗╔═╝║║──                          v1.0.0    *
 *                           ║╚╝║──║║──║║──                                    *
 *                           ║╔╗║──║║──║║──                                    *
 *                           ║║║║──║║──║╚═╗                                    *
 *                           ╚╝╚╝──╚╝──╚══╝                                    *  
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "atl_port.h"
#include "atl_core.h"

#ifndef ATL_TEST
  #include "stdarg.h"
  #include <stdio.h>
 // #include "hc32f460_utility.h"
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
static void atl_crit_enter(void) 
{ 
 // __disable_irq();
}

/*******************************************************************************
 ** @brief  Weak function to exit critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
static void atl_crit_exit(void)  
{
//  __enable_irq();
}

/*******************************************************************************
 ** @brief  Printf
 ** @param  none
 ** @return none
 ******************************************************************************/
#ifndef ATL_TEST
void atl_printf_safe(const char *fmt, ...) 
{
  atl_init_t atl = atl_get_init(); 
  if(!atl.atl_printf) return;
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
      atl.atl_printf(escaped);
    }
    total_processed += fragment_len;
    va_end(args);
    va_start(args, fmt);
  }
  va_end(args);
}
#endif

/*******************************************************************************
 ** @brief  Critical handlers
 ** @param  none
 ** @return none
 ******************************************************************************/
#ifndef ATL_TEST
static volatile uint32_t atl_crit_counter = 0;
#endif

void _atl_crit_enter(void) 
{ 
  #ifndef ATL_TEST
  atl_crit_enter();
  atl_crit_counter++;
  #endif
}

void _atl_crit_exit(void)  
{
  #ifndef ATL_TEST
  if(atl_crit_counter > 0) 
  {
    atl_crit_counter--;
    if (atl_crit_counter == 0) atl_crit_exit();
  }
  #endif
}
