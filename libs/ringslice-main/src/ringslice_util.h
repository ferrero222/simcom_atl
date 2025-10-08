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
#ifndef _RINGSLICE_UTIL_H_
#define _RINGSLICE_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif
/*! @file
* @brief Utility functions for ringslices
*
*/


#include <stdint.h>
#include <stdbool.h>
#include "dbc_assert.h"
#include "ringslice.h"

/**
 * @defgroup RingsliceUtilities Ringslice Utility functions
 * @{
 */

 /*!
 * Increments pointer with wrapping around
 * @param[in] curr pointer to increment
 * @param[in] absolute_offset offset to increment
 * @param[in] start start pointer of wrapping
 * @param[in] end end pointer of wrapping
 *
 * @return pointer between start (including) and end (not including)
 */
RINGSLICE_INLINE uint8_t *ringslice_ptr_increment_wrap_around(uint8_t const *curr, ringslice_cnt_t absolute_offset, uint8_t const *const start, uint8_t const *const end) {
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 666, start <= curr && curr < end);
    uint8_t *next_unwrapped = (uint8_t *)&(curr[absolute_offset]);
    uint8_t *ret = (next_unwrapped < end) ? (next_unwrapped) : (uint8_t *)(&(start[next_unwrapped - end]));
    DBC_MODULE_ENSURE(RINGSLICE_MODULE, 999, start <= ret && ret < end);
    return ret;
}

/*!
 * Decrements pointer with wrapping around
 * @param[in] curr pointer to decrement
 * @param[in] absolute_offset offset to decrement
 * @param[in] start start pointer of wrapping
 * @param[in] end end pointer of wrapping
 *
 * @return pointer between start (including) and end (not including)
 */
RINGSLICE_INLINE uint8_t *ringslice_ptr_decrement_wrap_around(uint8_t const *curr, ringslice_cnt_t absolute_offset, uint8_t const *const start, uint8_t const *const end) {
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 667, start <= curr && curr < end);
    uint8_t *prev_unwrapped = (uint8_t *)&(curr[-absolute_offset]);
    uint8_t *ret = (prev_unwrapped >= start) ? (prev_unwrapped) : (uint8_t *)(&(end[prev_unwrapped - start]));
    DBC_MODULE_ENSURE(RINGSLICE_MODULE, 998, start <= ret && ret < end);
    return ret;
}

/*!
 * Shifts index (forward or backward) with wrapping around
 * @param[in] idx index to increment
 * @param[in] inc increment value (positive or negative)
 * @param[in] mod module to wrap around
 *
 * @return new index between 0 and mod - 1
 */

RINGSLICE_INLINE ringslice_cnt_t ringslice_index_shift_wrap_around(ringslice_cnt_t idx, ringslice_cnt_t inc, ringslice_cnt_t mod) {
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 11, 0 <= idx && idx < mod);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 12, mod + inc > 0);
    return (idx + mod + inc) % mod;
}

/*!
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // _RINGSLICE_UTIL_H_