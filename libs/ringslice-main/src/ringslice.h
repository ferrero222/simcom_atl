/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Nikita Maltsev (aleph-five)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef _RINGSLICE_H_
#define _RINGSLICE_H_

#ifdef __cplusplus
extern "C" {
#endif
/*! @file
* @brief Slices of ring buffer with string-like methods
*
* @note
* Useful for embedded applications
*/


#include <stdint.h>
#include <stdbool.h>
#include "dbc_assert.h"
#include "ringslice_config.h"

/// ringslice module name for DBC assertions
#define RINGSLICE_MODULE                                                "ringslice"

/*! @mainpage Ringslice

* @section intro_sec Introduction
* Ringslice is a library for working with slices of ring buffers.
* It provides a set of methods for manipulating slices as if they were strings,
* including searching, comparing, and getting a substring. Additionally, it allows
* for working with slices as if they were simple arrays of bytes, thus allowing
* for working with binary data, not just strings.
* It is particularly useful in embedded applications where memory is limited,
* and you need to work with circular buffers efficiently.

* @section motivation Motivation
* In some applications, receiving data from uart is organized as an interrupt
* service routine (ISR) that fills a ring buffer.
* The data is then processed in the main loop, where it is often necessary to
* search for substrings, compare them, or extract parts of the data.
* There are several approaches for this:
* -# Copy the data to another buffer and work with it as a string. This approach requires
*    additional memory for the buffer and processing time for copying data.
* -# Search for amount of data in the ring buffer and, if found, copy it
*    to another buffer onto stack and then work with it as a string, which
*    requires stack usage and additional processing time for copying data.
*
* Ringslice provides a convenient way to work with such data without copying it
* to another buffer, thus saving memory and processing time.
* 
* @section usage Usage
* - Add the files of src directory to your project.
* - Add the files of config directory to your project (edit ringslice_config.h if you need).
* - Add the DBC_fault_handler() function implementation to your project.
* - Include the header file ringslice.h in your source files where you want to use the library.
* - Use the provided methods to work with slices of ring buffers.
*
* @section example Example of usage
* The following primitive example demonstrates how to use the ringslice library
* to process data received from a UART in an embedded application.
* @code{.c}
#include "ringslice.h"
#include "bsp.h" // Header containing uart related functions,
                 // such as bsp_init(), uart_data_available(), uart_read_byte()
                 // and DBC_fault_handler() implementation
#include "application.h" // Example application header for application logic,
                         // needed for app_process_registration_status() function

#define RINGBUFFER_SIZE 256
static uint8_t ring_buffer[RINGBUFFER_SIZE]; // Example ring buffer
static volatile int head = 0; // Head index for the ring buffer
static volatile int tail = 0; // Tail index for the ring buffer

void uart_isr(void) {
    // Example ISR that fills the ring buffer
    while (uart_data_available()) {
        ring_buffer[head] = uart_read_byte();
        head = (head + 1) % RINGBUFFER_SIZE; // Wrap around
    }
}

void process_data(void) {
    // Create a ringslice from the ring buffer
    ringslice_t rs = ringslice_initializer(ring_buffer, RINGBUFFER_SIZE, tail, head);

    // Check if the ringslice is empty
    if (ringslice_is_empty(&rs)) {
        return; // Nothing to process
    }
    
    static int processed_bytes = 0; // Counter for processed bytes
    

    // Search for an end of line sequence "\r\n"
    ringslice_t found = ringslice_subslice_with_suffix(&rs, processed_bytes, "\r\n");
    if (!ringslice_is_empty(&found)) {
        int arg1, arg2;
        int argc = ringslice_scanf(&found, "+CREG: %d, %d", &arg1, &arg2); // Example of parsing data
        if(argc == 2) {
            // Process the parsed data
            app_process_registration_status(arg1, arg2);
        }
        // Reset processed bytes counter
        processed_bytes = 0;
        // Update tail index after processing
        tail = (tail + ringslice_len(&rs)) % RINGBUFFER_SIZE; // Wrap around
    }
    else {
        processed_bytes = ringslice_len(&rs) - 1; // Update processed bytes counter
                                                  // for optimal next search
    }
}

int main (void) {
    // Initialize UART and other peripherals
    bsp_init();

    // Main loop
    while (1) {
        // Process the data in the ring buffer
        process_data();
        
        // Other application logic...
        app_process();
    }
}
* @endcode

*/

/**
* @defgroup RingsliceTypes Ringslice structures and types
* @{
*/

/// type for counter
typedef int32_t ringslice_cnt_t;

/// ringslice structure
typedef struct
{
    uint8_t *buf;                       ///< Pointer to zeroth element of ring buffer array
    ringslice_cnt_t buf_size;           ///< size of array
    ringslice_cnt_t first;               ///< first (index of the first element that will be processed)
    ringslice_cnt_t last;               ///< last (index of empty place after last element)
}
ringslice_t;

/*!
* @}
*/

/**
* @defgroup RingsliceImmutableMethods Ringslice Immutable Methods
* @{
*/


/*!
* Initializer for ring slice.
* @param[in] buf pointer to zeroth element of ring buffer
* @param[in] buf_size size of buffer
* @param[in] first index of first element
* @param[in] last index of empty place after last element
*
* @return ring slice instance
*
*/
RINGSLICE_INLINE ringslice_t ringslice_initializer(uint8_t *buf, ringslice_cnt_t buf_size, ringslice_cnt_t first, ringslice_cnt_t last) {
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 1, buf);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 2, buf_size > 0);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 3, (0 <= first && first < buf_size));
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 4, (0 <= last && last < buf_size));
    ringslice_t rs = {
        .buf = buf,
        .buf_size = buf_size,
        .first = first,
        .last = last,
    };
    return rs;
}

/*!
* Calculates the length of subslice
* @param[in] me ringslice instance
*
* @return number of bytes stored in ringslice
*
*/
RINGSLICE_INLINE ringslice_cnt_t ringslice_len(ringslice_t const * const me) {
    return (me->buf_size + me->last - me->first) % me->buf_size;
}

/*!
* Checks whether the ringslice is empty
* @param[in] me ringslice instance
*
* @return true if empty, otherwise false
*
*/
RINGSLICE_INLINE bool ringslice_is_empty(ringslice_t const * const me) {
    return (me->first == me->last);
}

/*!
* Byte at relative index n
* @param[in] me pointer to ring slice instance
* @param[in] n relative index 
*
* @return byte at position n from first index of ring slice
*
* @note n must be greater than 0 and less than length of ring slice
*
*/
RINGSLICE_INLINE uint8_t ringslice_nth_byte(ringslice_t const * const me, ringslice_cnt_t n)
{
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 104, n < ringslice_len(me));
    ringslice_cnt_t idx = (me->first + n) % me->buf_size;
    return me->buf[idx];
}

/*!
* Subslice of ring slice.
* @param[in] me pointer to ring slice instance that is copied
* @param[in] first relative index of the first element. For example,
*    if first equals to 0, then first element of returned slice will be the first of me slice. 
* @param[in] last relative index of the last element, analogous to first
*
* @return ring slice instance
*
*/
RINGSLICE_INLINE ringslice_t ringslice_subslice(ringslice_t const * const me, ringslice_cnt_t rel_first, ringslice_cnt_t rel_last) {
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 101, rel_first <= rel_last);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 102, rel_first < ringslice_len(me));
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 103, rel_last <= ringslice_len(me));
    ringslice_t rs;
    rs.buf = me->buf;
    rs.buf_size = me->buf_size;
    rs.first = (me->first + rel_first) % me->buf_size;
    rs.last = (me->first + rel_last) % me->buf_size;
    return rs;
}

/*!
* scanf implementation for ringslice
* @param[in] rs ringslice instance
* @param[in] fmt format string
* @param[out] ... additional arguments, depending on the format string
*
* @return a number of receiving arguments succesfully assigned
    or first on error.
*
*/
RINGSLICE_INLINE ringslice_t ringslice_subslice_after(ringslice_t const *const subslice, ringslice_t const *const me, const ringslice_cnt_t len) { //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 202, subslice->buf == me->buf);
  DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 203, subslice->buf_size == me->buf_size);
  if(len) DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 204, len < subslice->buf_size);
  if(!len) return ringslice_initializer(me->buf, me->buf_size, (subslice->last+1)%subslice->buf_size, me->last);
  else     return ringslice_initializer(me->buf, me->buf_size, (subslice->last+1)%subslice->buf_size, (subslice->last+1 +len)%subslice->buf_size); 
}

/*!
* scanf implementation for ringslice
* @param[in] rs ringslice instance
* @param[in] fmt format string
* @param[out] ... additional arguments, depending on the format string
*
* @return a number of receiving arguments succesfully assigned
    or first on error.
*
*/
RINGSLICE_INLINE ringslice_t ringslice_subslice_get_content(ringslice_t const *const subslice, uint8_t* const dst, const ringslice_cnt_t dst_size) {
  for(uint8_t i = 0; i < ringslice_len(subslice); ++i){
    dst[i] = subslice->buf[(subslice->first +i) % subslice->buf_size]; //СДЕЛАТЬ ЧТОБЫ ПИСАЛОСЬ ТОЛЬКО СТОЛЬКО СКОЛЬКО ПОМЕЩАЕТСЯ В БУФЕР ИЛИ ВЕРНУТЬ ОШИБКУ //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  }
}

/*!
* Compares ringslice instance with string lexicographically
* @param[in] me ringslice instance which is compared with string
* @param[in] str string for compare
*
* @return 0 if are equal,
*   negative value if ringslice appears before str in lexicographical order,
*   positive value if ringslice appears after str in lexicographical order
*
*/
int ringslice_strcmp(ringslice_t const * const me, char const * str);

/*!
* Compares ringslice instance with string lexicographically
* @param[in] me ringslice instance which is compared with string
* @param[in] str string for compare
*
* @return 0 if are equal,
*   negative value if ringslice appears before str in lexicographical order,
*   positive value if ringslice appears after str in lexicographical order
*
*/
int ringslice_strncmp(ringslice_t const * const me, char const * str); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*!
* Searches for substring in ringslice instance
* @param[in] me ringslice instance where substring is searched for
* @param[in] substr searched substring
*
* @return subslice of me slice containing substring, otherwise empty ringslice
*
* @note if substr is empty string, then copy of me slice will be returned
*
*/
ringslice_t ringslice_strstr(ringslice_t const * const me, char const * substr);

/*!
* Searches for subslice with suffix in ringslice instance
* @param[in] me ringslice instance where suffix is searched for
* @param[in] from_idx start index for searching; if 0, then search from beginning of the slice
* @param[in] suffix suffix that is searched for
*
* @return subslice of me slice with suffix, otherwise empty ringslice
*
* @note if suffix is empty string, then copy of me slice will be returned
*
*/
ringslice_t ringslice_subslice_with_suffix(ringslice_t const * const me, ringslice_cnt_t from_idx, char const * suffix);

/*!
* scanf implementation for ringslice
* @param[in] rs ringslice instance
* @param[in] fmt format string
* @param[out] ... additional arguments, depending on the format string
*
* @return a number of receiving arguments succesfully assigned
    or first on error.
*
*/
int ringslice_scanf(ringslice_t const * const rs, const char *fmt, ...);

/**
* @brief Gets gap slice between two subslices from one buffer. slice1 should be before slice2.
*        Slices shouldnt be overlaped.
* @param[in] slice1 first ringslice instance
* @param[in] slice2 second ringslice instance
* @return gap slice instance
*/
ringslice_t ringslice_subslice_gap(ringslice_t const *const slice1, ringslice_t const *const slice2); //!!!!!!!!!!!!!!!!!!!!!!!!


/*!
* Checks if two ringslices are equal (same buffer, same parameters and same content)
* @param[in] slice1 first ringslice instance
* @param[in] slice2 second ringslice instance
* @return true if slices are equal, false otherwise
*/
bool ringslice_equals(ringslice_t const *const slice1, ringslice_t const *const slice2); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*!
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // _RINGSLICE_H_
