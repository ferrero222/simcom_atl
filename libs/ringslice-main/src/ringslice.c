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
#include "ringslice_util.h"
#include "ringslice.h"

#warning "UPD: scanf +wrap check for %[] + %*[] \
               ringslice_equals \
               ringslice_subslice_gap \
               ringslice_strncmp \
               ringslice_subslice_get_content \
               ringslice_subslice_after \

DBC_MODULE_NAME(RINGSLICE_MODULE)

ringslice_t ringslice_strstr(ringslice_t const *const me, char const *substr) {
    ringslice_t substr_slice = ringslice_initializer(me->buf, me->buf_size, me->first, me->first);  // initialize with empty slice
    uint8_t const *const buf_end = &(me->buf[me->buf_size]);
    uint8_t const *const buf_start = &(me->buf[0]);
    uint8_t const *first_ptr = &(me->buf[me->first]);
    uint8_t const *last_ptr = &(me->buf[me->last]);
    bool found = false;
    ringslice_cnt_t cmp_pos = 0;
    while (first_ptr != last_ptr) {
        char cmp = substr[cmp_pos];

        if (cmp == '\0') {
            found = true;
            break;
        }

        if (*first_ptr == cmp) {
            cmp_pos++;
        } else {
            first_ptr = ringslice_ptr_decrement_wrap_around(first_ptr, cmp_pos, buf_start, buf_end);
            cmp_pos = 0;
        }

        first_ptr = ringslice_ptr_increment_wrap_around(first_ptr, 1, buf_start, buf_end);
    }

    if ((first_ptr == last_ptr) && (substr[cmp_pos] == '\0')) {
        found = true;
    }

    if (found) {
        ringslice_cnt_t suffix_idx_last = (ringslice_cnt_t)(first_ptr - buf_start);
        ringslice_cnt_t suffix_idx_first = (suffix_idx_last >= cmp_pos) ? (suffix_idx_last - cmp_pos) : (me->buf_size + suffix_idx_last - cmp_pos);

        DBC_ENSURE(901, 0 <= suffix_idx_first && suffix_idx_first < substr_slice.buf_size);
        DBC_ENSURE(902, 0 <= suffix_idx_last && suffix_idx_last < substr_slice.buf_size);
        substr_slice.first = suffix_idx_first;
        substr_slice.last = suffix_idx_last;
    }

    return substr_slice;
}

int ringslice_strcmp(ringslice_t const *const me, char const *str) {
    uint8_t const *first_ptr = &(me->buf[me->first]);
    uint8_t const *last_ptr = &(me->buf[me->last]);
    uint8_t const *const buf_end = &(me->buf[me->buf_size]);
    uint8_t const *const buf_start = &(me->buf[0]);
    uint8_t const *chr = (uint8_t const *)str;

    while (first_ptr != last_ptr) {
        int diff = (int)*first_ptr - (int)*chr;
        if (diff) {
            return diff;
        }

        chr++;
        first_ptr = ringslice_ptr_increment_wrap_around(first_ptr, 1, buf_start, buf_end);
    }

    return -(int)*chr;
}

int ringslice_strncmp(ringslice_t const *const me, char const *str) { //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    uint8_t const *first_ptr = &(me->buf[me->first]);
    uint8_t const *last_ptr = &(me->buf[me->last]);
    uint8_t const *const buf_end = &(me->buf[me->buf_size]);
    uint8_t const *const buf_start = &(me->buf[0]);
    uint8_t const *chr = (uint8_t const *)str;

    while (first_ptr != last_ptr) {
        int diff = (int)*first_ptr - (int)*chr;
        if (diff) {
            return diff;
        }

        chr++;
        first_ptr = ringslice_ptr_increment_wrap_around(first_ptr, 1, buf_start, buf_end);
    }

    return -(int)*chr;
}

ringslice_t ringslice_subslice_with_suffix(ringslice_t const *const me, ringslice_cnt_t from_idx, char const *suffix) {
    ringslice_cnt_t const rs_len = ringslice_len(me);
    DBC_ASSERT(204, from_idx <= ringslice_len(me));

    ringslice_t resp_slice = ringslice_initializer(me->buf, me->buf_size, me->first, me->first);  // initialize with empty slice
    ringslice_t search_slice = ringslice_subslice(me, from_idx, rs_len);
    ringslice_t suffix_slice = ringslice_strstr(&search_slice, suffix);

    if (ringslice_is_empty(&suffix_slice)) {
        // resp_slice already initialized with empty slice
    } else {
        resp_slice.last = suffix_slice.last;
    }

    return resp_slice;
}

/**
* @brief Gets gap slice between two subslices from one buffer. slice1 should be before slice2.
*        Slices shouldnt be overlaped.
* @param[in] slice1 first ringslice instance
* @param[in] slice2 second ringslice instance
* @return gap slice instance
*/
ringslice_t ringslice_subslice_gap(ringslice_t const *const slice1, ringslice_t const *const slice2) { //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    DBC_ASSERT(705, slice1->buff != slice2->buff || slice1->buff_size != slice2->buff_size); //different buffers

    ringslice_cnt_t const rs_buff = slice1->buf;
    ringslice_cnt_t const rs_buff_len = slice1->buf_size;

    DBC_ASSERT(706, slice1->first >= rs_buff_len || slice1->last >= rs_buff_len || //out of bounds
                    slice2->first >= rs_buff_len || slice2->last >= rs_buff_len);
    
    ringslice_cnt_t a_first = slice1->first;
    ringslice_cnt_t a_last = (slice1->last < slice1->first) ? slice1->last + rs_buff_len : slice1->last;
    ringslice_cnt_t b_first = slice2->first;
    ringslice_cnt_t b_last = (slice2->last < slice2->first) ? slice2->last + rs_buff_len : slice2->last;
    
    DBC_ASSERT(707, (a_first <= b_first && b_first <= a_last) || //slices is overlap
                    (a_first <= b_last  && b_last  <= a_last) ||
                    (b_first <= a_first && a_first <= b_last) ||
                    (b_first <= a_last  && a_last  <= b_last));

    DBC_ASSERT (708, b_last < a_first); //slice2 before slice1

    return ringslice_initializer(rs_buff, rs_buff_len, slice1->last, slice2->first);
}

/**
* @brief Checks if two ringslices are equal (same buffer, same parameters and same content)
* @param[in] slice1 first ringslice instance
* @param[in] slice2 second ringslice instance
* @return true if slices are equal, false otherwise
*/
bool ringslice_subslice_equals(ringslice_t const *const slice1, ringslice_t const *const slice2) { //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 801, slice1 != NULL);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 802, slice2 != NULL);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 803, slice1->buf != NULL);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 804, slice2->buf != NULL);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 805, slice1->buf_size > 0);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 806, slice2->buf_size > 0);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 807, slice1->first >= 0 && slice1->first < slice1->buf_size);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 808, slice1->last >= 0 && slice1->last < slice1->buf_size);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 809, slice2->first >= 0 && slice2->first < slice2->buf_size);
    DBC_MODULE_REQUIRE(RINGSLICE_MODULE, 810, slice2->last >= 0 && slice2->last < slice2->buf_size);
    DBC_ASSERT(811, slice1->buf != slice2->buf || slice1->buf_size != slice2->buf_size);

    if(slice1 == slice2) return true;
    
    ringslice_cnt_t len1 = ringslice_len(slice1);
    ringslice_cnt_t len2 = ringslice_len(slice2);
    
    DBC_ENSURE(901, len1 >= 0 && len1 <= slice1->buf_size);
    DBC_ENSURE(902, len2 >= 0 && len2 <= slice2->buf_size);
    
    if(len1 != len2) return false;
  
    if (len1 == 0) {
        DBC_ENSURE(903, ringslice_is_empty(slice1));
        DBC_ENSURE(904, ringslice_is_empty(slice2));
        return true;
    }
    
    uint8_t const *const buf_end = &(slice1->buf[slice1->buf_size]);
    uint8_t const *const buf_start = &(slice1->buf[0]);
    uint8_t const *ptr1 = &(slice1->buf[slice1->first]);
    uint8_t const *ptr2 = &(slice2->buf[slice2->first]);
    
    ringslice_cnt_t remaining = len1;
    
    DBC_ASSERT(901, ptr1 >= buf_start && ptr1 < buf_end);
    DBC_ASSERT(902, ptr2 >= buf_start && ptr2 < buf_end);
    
    while (remaining > 0) {
        DBC_ASSERT(903, ptr1 >= buf_start && ptr1 < buf_end);
        DBC_ASSERT(904, ptr2 >= buf_start && ptr2 < buf_end);
        if(*ptr1 != *ptr2) return false;
        ptr1 = ringslice_ptr_increment_wrap_around(ptr1, 1, buf_start, buf_end);
        ptr2 = ringslice_ptr_increment_wrap_around(ptr2, 1, buf_start, buf_end);
        remaining--;
        DBC_ASSERT(905, remaining >= 0);
    }
    DBC_ENSURE(905, remaining == 0);
    return true;
}