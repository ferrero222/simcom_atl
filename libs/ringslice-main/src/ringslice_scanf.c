/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 G. Elian Gidoni
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
/*! @file
* @brief Scanf implementation for ringslices.
* 
* This is a modified version of scanf implementation from
* @ref https://github.com/eliangidoni/mfmt.git
*
*/
#include <stdarg.h>
#include <stddef.h>
#include "ringslice_util.h"
#include "ringslice.h"

DBC_MODULE_NAME(RINGSLICE_MODULE)

/*
 * Private functions.
 */

/*!
* Checks whether the character is a whitespace
* @param[in] c character to check
*
* @return 1 if c is whitespace, 0 otherwise
*
*/
static int is_space(char c) {
    return (c == ' ' || c == '\r' || c == '\n' ||
            c == '\t' || c == '\v' || c == '\f');
}

/*!
* Skips all the whitespaces in the ringslice
* @param[in] first index of first byte in ringslice
* @param[in] last index after last byte in ringslice 
* @param[in] buf pointer to buffer of ringslice
* @param[in] size buffer size
*
* @return first encountered index of byte
    in the ringslice that is not a space
*
*/
static ringslice_cnt_t wa_skip_spaces(ringslice_cnt_t first, 
                                        ringslice_cnt_t const last,
                                        uint8_t const buf[],
                                        ringslice_cnt_t size) {
    while (is_space((char)buf[first]) && (first != last)) {
        first = ringslice_index_shift_wrap_around(first, 1, size);
    }
    return first;
}

/*!
* Skips all the whitespaces in the string
* @param[in] str input string
*
* @return pointer to first encountered character
    in the string that is not a space
*
*/
static char * skip_spaces(const char *str) {
    while (is_space(*str)) {
        ++str;
    }
    return (char *)str;
}

/*!
* Converts decimal string to signed value
* @param[in] first index of first byte in ringslice
* @param[in] last index after last byte in ringslice 
* @param[in] buf pointer to buffer of ringslice
* @param[in] size buffer size
* @param[out] out pointer to out value
*
* @return an index after the last read and processed char
    or first on error.
*
*/
static ringslice_cnt_t wa_dec_to_signed(ringslice_cnt_t const first,
                                            ringslice_cnt_t const last,
                                            uint8_t const buf[],
                                            ringslice_cnt_t size,
                                            long *out) {
    ringslice_cnt_t cur = wa_skip_spaces(first, last, buf, size);
    long value = 0;
    int isneg = 0, isempty = 1;
    if (buf[cur] == '+') {
        cur = ringslice_index_shift_wrap_around(cur, 1, size);
    } else if (buf[cur] == '-') {
        cur = ringslice_index_shift_wrap_around(cur, 1, size);
        isneg = 1;
    }
    while (cur != last && buf[cur] >= '0' && buf[cur] <= '9') {
        value = (value * 10) + (buf[cur] - '0');
        isempty = 0;
        cur = ringslice_index_shift_wrap_around(cur, 1, size);
    }
    if (isempty) {
        return first;
    }
    if (isneg) {
        *out = -value;
    } else {
        *out = value;
    }
    return cur;
}

/*!
* Converts decimal string to unsigned value
* @param[in] first index of first byte in ringslice
* @param[in] last index after last byte in ringslice 
* @param[in] buf pointer to buffer of ringslice
* @param[in] size buffer size
* @param[out] out pointer to out value
*
* @return an index after the last read and processed char
    or first on error.
*
*/
static ringslice_cnt_t wa_dec_to_unsigned(ringslice_cnt_t const first,
                                            ringslice_cnt_t const last,
                                            uint8_t const buf[],
                                            ringslice_cnt_t size,
                                            unsigned long *out) {
    ringslice_cnt_t cur = wa_skip_spaces(first, last, buf, size);
    unsigned long value = 0;
    int isempty = 1;
    while (cur != last && buf[cur] >= '0' && buf[cur] <= '9') {
        value = (value * 10) + (buf[cur] - '0');
        isempty = 0;
        cur = ringslice_index_shift_wrap_around(cur, 1, size);
    }
    if (isempty) {
        return first;
    }
    *out = value;
    return cur;
}

/* Returns a pointer after the last read char, or 'str' on error. */
static char *
dec_to_unsigned(const char *str, unsigned long *out) {
    const char *cur = skip_spaces(str);
    unsigned long value = 0;
    int isempty = 1;
    while (*cur != '\0' && *cur >= '0' && *cur <= '9') {
        value = (value * 10) + (*cur - '0');
        isempty = 0;
        ++cur;
    }
    if (isempty) {
        return (char *)str;
    }
    *out = value;
    return (char *)cur;
}

/* Returns an index after the last read char, or first on error. */
// static ringslice_cnt_t
// wa_hex_to_signed(ringslice_cnt_t const first, ringslice_cnt_t const last, uint8_t const buf[], ringslice_cnt_t size, long *out)
// {
//         ringslice_cnt_t cur = wa_skip_spaces(first, last, buf, size);
//         if(last == cur || last == ringslice_index_shift_wrap_around(cur, 1, size))
//         {
//                 return first;
//         }
//         long value = 0;
//         int isneg = 0, isempty = 1;
//         if (buf[cur] == '+'){
//                 cur = ringslice_index_shift_wrap_around(cur, 1, size);
//         }else if(buf[cur] == '-'){
//                 cur = ringslice_index_shift_wrap_around(cur, 1, size);
//                 isneg = 1;
//         }
//         if (buf[cur] == '0'
//                 && ringslice_index_shift_wrap_around(cur, 1, size) != last
//                 && buf[ringslice_index_shift_wrap_around(cur, 1, size)] == 'x'){
//                         cur = ringslice_index_shift_wrap_around(cur, 2, size);
//         }
//         while (cur != last){
//                 if(buf[cur] >= '0' && buf[cur] <= '9'){
//                         value = (value * 16) + (buf[cur] - '0');
//                 }else if (buf[cur] >= 'a' && buf[cur] <= 'f'){
//                         value = (value * 16) + 10 + (buf[cur] - 'a');
//                 }else if (buf[cur] >= 'A' && buf[cur] <= 'F'){
//                         value = (value * 16) + 10 + (buf[cur] - 'A');
//                 }else{
//                         break;
//                 }
//                 isempty = 0;
//                 cur = ringslice_index_shift_wrap_around(cur, 1, size);
//         }
//         if (isempty){
//                 return first;
//         }
//         if (isneg){
//                 *out = -value;
//         }else{
//                 *out = value;
//         }
//         return cur;
// }

/*!
* Converts hex string to unsigned value
* @param[in] first index of first byte in ringslice
* @param[in] last index after last byte in ringslice 
* @param[in] buf pointer to buffer of ringslice
* @param[in] size buffer size
* @param[out] out pointer to out value
*
* @return an index after the last read and processed char
    or first on error.
*
*/
static ringslice_cnt_t wa_hex_to_unsigned(ringslice_cnt_t const first,
                                            ringslice_cnt_t const last,
                                            uint8_t const buf[],
                                            ringslice_cnt_t size,
                                            unsigned long *out) {
    ringslice_cnt_t cur = wa_skip_spaces(first, last, buf, size);
    if (last == cur || last == ringslice_index_shift_wrap_around(cur, 1, size)) {
        return first;
    }
    unsigned long value = 0;
    int isempty = 1;
    if (buf[cur] == '0' && buf[ringslice_index_shift_wrap_around(cur, 1, size)] == 'x') {
        cur += 2;
    }
    while (cur != last) {
        if (buf[cur] >= '0' && buf[cur] <= '9') {
            value = (value * 16) + (buf[cur] - '0');
        } else if (buf[cur] >= 'a' && buf[cur] <= 'f') {
            value = (value * 16) + 10 + (buf[cur] - 'a');
        } else if (buf[cur] >= 'A' && buf[cur] <= 'F') {
            value = (value * 16) + 10 + (buf[cur] - 'A');
        } else {
            break;
        }
        isempty = 0;
        cur = ringslice_index_shift_wrap_around(cur, 1, size);
    }
    if (isempty) {
        return first;
    }
    *out = value;
    return cur;
}

/*!
* Macro for generation a wrapper function for converter
* @param FROM_TYPE output type of wrapped function (long, unsigned long)
* @param TO_TYPE output type of wrapper function (int , unsigned int)
* @param WRAPPER_NAME name of generated function
* @param WRAPPED_FUNCTION name of function that is wrapped
*
*/
#define MFMT_WA_TO_TYPE(FROM_TYPE, TO_TYPE, WRAPPER_NAME, WRAPPED_FUNCTION)\
    static ringslice_cnt_t                            \
        WRAPPER_NAME(ringslice_cnt_t const first, \
                         ringslice_cnt_t const last,  \
                         uint8_t const buf[],         \
                         ringslice_cnt_t size,        \
                         TO_TYPE *out) {                 \
        FROM_TYPE v;                                       \
        ringslice_cnt_t cur = WRAPPED_FUNCTION(first, \
                                               last,  \
                                               buf,   \
                                               size,  \
                                               &v);   \
        if (cur != first) {                           \
            *out = (TO_TYPE)v;                           \
        }                                             \
        return cur;                                   \
    }

#define MFMT_DEC_TO_UNSIGNED(TYPE, NAME)            \
    static char *                                   \
        dec_to_##NAME(const char *str, TYPE *out) { \
        unsigned long v;                            \
        char *cur = dec_to_unsigned(str, &v);       \
        if (cur != str) {                           \
            *out = (TYPE)v;                         \
        }                                           \
        return cur;                                 \
    }

MFMT_WA_TO_TYPE(         long,          int,  wa_dec_to_int, wa_dec_to_signed)
MFMT_WA_TO_TYPE(unsigned long, unsigned int, wa_dec_to_uint, wa_dec_to_unsigned)
MFMT_WA_TO_TYPE(unsigned long, unsigned int, wa_hex_to_uint, wa_hex_to_unsigned)

MFMT_DEC_TO_UNSIGNED(unsigned int, uint)

/*!
* Checks whether a character in a scanset
* @param[in] c character for check
* @param[in] scanset pointer to scanset (all the characters after "[" or "[^")
*
* @return 1 if c in the scanset, 0 otherwise
*
*/
static int is_in_scanset(char c, const char *scanset) {
    if(*scanset == ']')  {
        if(*scanset == c) {
            return 1;
        }
        ++scanset;
    }
    if(*scanset == '-')
    {
        if(*scanset == c) {
            return 1;
        }
        ++scanset;
    }
    while (*scanset != ']') {
        if(*scanset == '-') {
            if(scanset[-1] <= c && c <= scanset[1]) {
                return 1;
            }
            ++scanset;
        }
        else if (*scanset == c) {
            return 1;
        }
        ++scanset;
    }
    return 0;
}

/*!
* Returns scanset end
* @param[in] scanset pointer to scanset (all the characters after "[" or "[^")
*
* @return pointer to first encountered ']' character or
*   pointer to null-character of scanset string
*
*/
static char const * get_scanset_end(const char *scanset) {
    // if the first character is a right bracket, it is a scanset character,
    // not the end of scanset
    if(*scanset == ']')  {
        ++scanset;
    }
    while (1) {
        if (*scanset == ']' || *scanset == '\0') {
            return scanset;
        }
        ++scanset;
    }
}

/*!
* Parse argument depending on format
* @param[in] first index of first byte in ringslice
* @param[in] last index after last byte in ringslice 
* @param[in] buf pointer to buffer of ringslice
* @param[in] size buffer size
* @param[in] fmt part of format string after 
*
* @return an index after the last read and processed char
    or first on error.
*
*/
static ringslice_cnt_t wa_parse_arg(ringslice_cnt_t const first,
                                    ringslice_cnt_t const last,
                                    uint8_t const buf[],
                                    ringslice_cnt_t size,
                                    const char * fmt,
                                    va_list *const args) {
    int *intp, intv = 0;
    unsigned int *uintp, uintv = 0, width = 0;
    char *charp;
    ringslice_cnt_t cur = first;
    
    // Проверяем, нужно ли пропускать поле (args == NULL)
    bool skip_field = (args == NULL);
    
    fmt = dec_to_uint(fmt, &width);
    
    if (*fmt == 'd') {
        cur = wa_dec_to_int(first, last, buf, size, &intv);
        if (cur != first && !skip_field) {
            intp = va_arg(*args, int *);
            *intp = intv;
        }
    } else if (*fmt == 'u') {
        cur = wa_dec_to_uint(first, last, buf, size, &uintv);
        if (cur != first && !skip_field) {
            uintp = va_arg(*args, unsigned int *);
            *uintp = uintv;
        }
    } else if (*fmt == 'x' || *fmt == 'X') {
        cur = wa_hex_to_uint(first, last, buf, size, &uintv);
        if (cur != first && !skip_field) {
            uintp = va_arg(*args, unsigned int *);
            *uintp = uintv;
        }
    } else if (*fmt == 'c') {
        if (!skip_field) {
            charp = va_arg(*args, char *);
            if (width == 0) {
                width = 1;
            }
            while (cur != last && uintv < width) {
                charp[uintv] = buf[cur];
                cur = ringslice_index_shift_wrap_around(cur, 1, size);
                ++uintv;
            }
        } else {
            // Пропускаем символы
            if (width == 0) {
                width = 1;
            }
            while (cur != last && uintv < width) {
                cur = ringslice_index_shift_wrap_around(cur, 1, size);
                ++uintv;
            }
        }
    } else if (*fmt == 's') {
        if (!skip_field) {
            charp = va_arg(*args, char *);
            while (cur != last && !is_space(buf[cur]) &&
                   (width == 0 || uintv < width)) {
                charp[uintv] = buf[cur];
                cur = ringslice_index_shift_wrap_around(cur, 1, size);
                ++uintv;
            }
            charp[uintv] = '\0';
        } else {
            // Пропускаем строку
            while (cur != last && !is_space(buf[cur]) &&
                   (width == 0 || uintv < width)) {
                cur = ringslice_index_shift_wrap_around(cur, 1, size);
                ++uintv;
            }
        }
    } else if (*fmt == '[') {
        fmt++;
        if (!skip_field) {
            charp = va_arg(*args, char *);
            int inversed = 0;
            if(*fmt == '^') {
                inversed = 1;
                ++fmt;
            }

            char const * scanset_end = get_scanset_end(fmt);
            if(*scanset_end == '\0') {
                // No closing bracket found, it is an error
                return cur;
            }

            while (cur != last && (inversed ^ is_in_scanset(buf[cur], fmt)) &&
                   (width == 0 || uintv < width)) {
                charp[uintv] = buf[cur];
                cur = ringslice_index_shift_wrap_around(cur, 1, size);
                ++uintv;
            }
            charp[uintv] = '\0';
        } else {
            // Пропускаем scanset
            int inversed = 0;
            if(*fmt == '^') {
                inversed = 1;
                ++fmt;
            }

            char const * scanset_end = get_scanset_end(fmt);
            if(*scanset_end == '\0') {
                return cur;
            }

            while (cur != last && (inversed ^ is_in_scanset(buf[cur], fmt)) &&
                   (width == 0 || uintv < width)) {
                cur = ringslice_index_shift_wrap_around(cur, 1, size);
                ++uintv;
            }
        }
    } else if (*fmt == '%' && buf[cur] == '%') {
        cur = ringslice_index_shift_wrap_around(cur, 1, size);
    }
    return cur;
}

/*
 * Public functions.
 */

int ringslice_scanf(ringslice_t const * const rs, const char * fmt, ...) {
    DBC_REQUIRE(444, rs);
    ringslice_cnt_t const first = rs->first;
    ringslice_cnt_t const last = rs->last;
    ringslice_cnt_t const size = rs->buf_size;
    uint8_t const *const buf = rs->buf;

    ringslice_cnt_t cur = first;

    int ret = 0;
    va_list args;
    va_start(args, fmt);

    while (fmt[0] != '\0' && cur != last) {
        if (fmt[0] == '%') {
            bool skip_field = false;
            if (fmt[1] == '*') {
                skip_field = true;
                fmt++; 
            }
            ringslice_cnt_t tmp = wa_parse_arg(cur, last, buf, size, &fmt[1], skip_field ? NULL : &args);
            if (tmp == cur) {
                break;
            }
            if (!skip_field && fmt[1] != '%') {
                ++ret;
            }
            ++fmt;
            while (fmt[0] >= '0' && fmt[0] <= '9') {
                ++fmt;
            }
            if(fmt[0] == '[') {
                fmt++;
                if(*fmt == '^') fmt++;
                if(*fmt == ']') fmt++;
                while(*fmt != ']') fmt++;
                fmt++;
            } else {
                ++fmt;
            }
            cur = tmp;
        } else if (is_space(fmt[0])) {
            ++fmt;
            cur = wa_skip_spaces(cur, last, buf, size);
        } else if (fmt[0] == buf[cur]) {
            ++fmt;
            cur = ringslice_index_shift_wrap_around(cur, 1, size);
        } else {
            break;
        }
    }

    va_end(args);
    return ret;
}