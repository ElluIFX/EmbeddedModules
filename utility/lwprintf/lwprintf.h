/**
 * \file            lwprintf.h
 * \brief           Lightweight stdio manager
 */

/*
 * Copyright (c) 2023 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of LwPRINTF - Lightweight stdio manager library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.0.3
 */
#ifndef LWPRINTF_HDR_H
#define LWPRINTF_HDR_H

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "modules.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        LWPRINTF_OPT Configuration
 * \brief           LwPRINTF options
 * \{
 */
#if !KCONFIG_AVAILABLE
/**
 * \brief           Enables `1` or disables `0` support for `long long int`
 * type, signed or unsigned.
 *
 */
#ifndef LWPRINTF_CFG_SUPPORT_LONG_LONG
#define LWPRINTF_CFG_SUPPORT_LONG_LONG 1
#endif

/**
 * \brief           Enables `1` or disables `0` support for any specifier
 * accepting any kind of integer types. This is enabling `%d, %b, %u, %o, %i,
 * %x` specifiers
 *
 */
#ifndef LWPRINTF_CFG_SUPPORT_TYPE_INT
#define LWPRINTF_CFG_SUPPORT_TYPE_INT 1
#endif

/**
 * \brief           Enables `1` or disables `0` support `%p` pointer print type
 *
 * When enabled, architecture must support `uintptr_t` type, normally available
 * with C11 standard
 */
#ifndef LWPRINTF_CFG_SUPPORT_TYPE_POINTER
#define LWPRINTF_CFG_SUPPORT_TYPE_POINTER 1
#endif

/**
 * \brief           Enables `1` or disables `0` support `%f` float type
 *
 */
#ifndef LWPRINTF_CFG_SUPPORT_TYPE_FLOAT
#define LWPRINTF_CFG_SUPPORT_TYPE_FLOAT 1
#endif

/**
 * \brief           Enables `1` or disables `0` support for `%e` engineering
 * output type for float numbers
 *
 * \note            \ref LWPRINTF_CFG_SUPPORT_TYPE_FLOAT has to be enabled to
 * use this feature
 *
 */
#ifndef LWPRINTF_CFG_SUPPORT_TYPE_ENGINEERING
#define LWPRINTF_CFG_SUPPORT_TYPE_ENGINEERING 1
#endif

/**
 * \brief           Enables `1` or disables `0` support for `%s` for string
 * output
 *
 */
#ifndef LWPRINTF_CFG_SUPPORT_TYPE_STRING
#define LWPRINTF_CFG_SUPPORT_TYPE_STRING 1
#endif

/**
 * \brief           Enables `1` or disables `0` support for `%k` for hex byte
 * array output
 *
 */
#ifndef LWPRINTF_CFG_SUPPORT_TYPE_BYTE_ARRAY
#define LWPRINTF_CFG_SUPPORT_TYPE_BYTE_ARRAY 1
#endif

/**
 * \brief           Specifies default number of precision for floating number
 *
 * Represents number of digits to be used after comma if no precision
 * is set with specifier itself
 *
 */
#ifndef LWPRINTF_CFG_FLOAT_DEFAULT_PRECISION
#define LWPRINTF_CFG_FLOAT_DEFAULT_PRECISION 6
#endif

/**
 * \brief           Enables `1` or disables `0` optional short names for
 * LwPRINTF API functions.
 *
 * It adds functions for default instance: `lwprintf`, `lwsnprintf` and others
 */
#ifndef LWPRINTF_CFG_ENABLE_SHORTNAMES
#define LWPRINTF_CFG_ENABLE_SHORTNAMES 1
#endif /* LWPRINTF_CFG_ENABLE_SHORTNAMES */

/**
 * \brief           Enables `1` or disables `0` C standard API names
 *
 * Disabled by default not to interfere with compiler implementation.
 * Application may need to remove standard C STDIO library from linkage
 * to be able to properly compile LwPRINTF with this option enabled
 */
#ifndef LWPRINTF_CFG_ENABLE_STD_NAMES
#define LWPRINTF_CFG_ENABLE_STD_NAMES 0
#endif /* LWPRINTF_CFG_ENABLE_SHORTNAMES */

#endif /* !KCONFIG_AVAILABLE */

/**
 * \}
 */

/**
 * \defgroup        LWPRINTF Lightweight stdio manager
 * \brief           Lightweight stdio manager
 * \{
 */

/**
 * \brief           Unused variable macro
 * \param[in]       x: Unused variable
 */
#define LWPRINTF_UNUSED(x) ((void)(x))

/**
 * \brief           Calculate size of statically allocated array
 * \param[in]       x: Input array
 * \return          Number of array elements
 */
#define LWPRINTF_ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

/**
 * \brief           Forward declaration for LwPRINTF instance
 */
struct lwprintf;

/**
 * \brief           Callback function for character output
 * \param[in]       ch: Character to print
 * \param[in]       lwobj: LwPRINTF instance
 * \return          `ch` on success, `0` to terminate further string processing
 */
typedef int (*lwprintf_output_fn)(int ch, struct lwprintf* lwobj);

/**
 * \brief           LwPRINTF instance
 */
typedef struct lwprintf {
  lwprintf_output_fn out_fn; /*!< Output function for direct print operations */
  void* arg;                 /*!< Output function argument */
} lwprintf_t;

uint8_t lwprintf_init_ex(lwprintf_t* lwobj, lwprintf_output_fn out_fn);
int lwprintf_vprintf_ex(lwprintf_t* const lwobj, const char* format,
                        va_list arg);
int lwprintf_printf_ex(lwprintf_t* const lwobj, const char* format, ...);
int lwprintf_vsnprintf_ex(lwprintf_t* const lwobj, char* s, size_t n,
                          const char* format, va_list arg);
int lwprintf_snprintf_ex(lwprintf_t* const lwobj, char* s, size_t n,
                         const char* format, ...);

/**
 * \brief           Write formatted data from variable argument list to sized
 * buffer \param[in,out]   lwobj: LwPRINTF instance. Set to `NULL` to use
 * default instance \param[in]       s: Pointer to a buffer where the resulting
 * C-string is stored. The buffer should have a size of at least `n` characters
 * \param[in]       format: C string that contains a format string that follows
 * the same specifications as format in printf \param[in]       ...: Optional
 * arguments for format string \return          The number of characters that
 * would have been written, not counting the terminating null character.
 */
#define lwprintf_sprintf_ex(lwobj, s, format, ...) \
  lwprintf_snprintf_ex((lwobj), (s), SIZE_MAX, (format), ##__VA_ARGS__)

/**
 * \brief           Initialize default LwPRINTF instance
 * \param[in]       out_fn: Output function used for print operation
 * \return          `1` on success, `0` otherwise
 * \sa              lwprintf_init_ex
 */
#define lwprintf_init(out_fn) lwprintf_init_ex(NULL, (out_fn))

/**
 * \brief           Print formatted data from variable argument list to the
 * output with default LwPRINTF instance \param[in]       format: C string that
 * contains the text to be written to output \param[in]       arg: A value
 * identifying a variable arguments list initialized with `va_start`. `va_list`
 * is a special type defined in `<cstdarg>`. \return          The number of
 * characters that would have been written if `n` had been sufficiently large,
 *                      not counting the terminating null character.
 */
#define lwprintf_vprintf(format, arg) lwprintf_vprintf_ex(NULL, (format), (arg))

/**
 * \brief           Print formatted data to the output with default LwPRINTF
 * instance \param[in]       format: C string that contains the text to be
 * written to output \param[in]       ...: Optional arguments for format string
 * \return          The number of characters that would have been written if `n`
 * had been sufficiently large, not counting the terminating null character.
 */
#define lwprintf_printf(format, ...) \
  lwprintf_printf_ex(NULL, (format), ##__VA_ARGS__)

/**
 * \brief           Write formatted data from variable argument list to sized
 * buffer with default LwPRINTF instance \param[in]       s: Pointer to a buffer
 * where the resulting C-string is stored. The buffer should have a size of at
 * least `n` characters \param[in]       n: Maximum number of bytes to be used
 * in the buffer. The generated string has a length of at most `n - 1`, leaving
 * space for the additional terminating null character \param[in]       format:
 * C string that contains a format string that follows the same specifications
 * as format in printf \param[in]       arg: A value identifying a variable
 * arguments list initialized with `va_start`. `va_list` is a special type
 * defined in `<cstdarg>`. \return          The number of characters that would
 * have been written if `n` had been sufficiently large, not counting the
 * terminating null character.
 */
#define lwprintf_vsnprintf(s, n, format, arg) \
  lwprintf_vsnprintf_ex(NULL, (s), (n), (format), (arg))

/**
 * \brief           Write formatted data from variable argument list to sized
 * buffer with default LwPRINTF instance \param[in]       s: Pointer to a buffer
 * where the resulting C-string is stored. The buffer should have a size of at
 * least `n` characters \param[in]       n: Maximum number of bytes to be used
 * in the buffer. The generated string has a length of at most `n - 1`, leaving
 * space for the additional terminating null character \param[in]       format:
 * C string that contains a format string that follows the same specifications
 * as format in printf \param[in]       ...: Optional arguments for format
 * string \return          The number of characters that would have been written
 * if `n` had been sufficiently large, not counting the terminating null
 * character.
 */
#define lwprintf_snprintf(s, n, format, ...) \
  lwprintf_snprintf_ex(NULL, (s), (n), (format), ##__VA_ARGS__)

/**
 * \brief           Write formatted data from variable argument list to sized
 * buffer with default LwPRINTF instance \param[in]       s: Pointer to a buffer
 * where the resulting C-string is stored. The buffer should have a size of at
 * least `n` characters \param[in]       format: C string that contains a format
 * string that follows the same specifications as format in printf \param[in]
 * ...: Optional arguments for format string \return          The number of
 * characters that would have been written, not counting the terminating null
 * character.
 */
#define lwprintf_sprintf(s, format, ...) \
  lwprintf_sprintf_ex(NULL, (s), (format), ##__VA_ARGS__)

#if LWPRINTF_CFG_ENABLE_SHORTNAMES || __DOXYGEN__

/**
 * \copydoc         lwprintf_printf
 * \note            This function is equivalent to \ref lwprintf_printf
 *                      and available only if \ref
 * LWPRINTF_CFG_ENABLE_SHORTNAMES is enabled
 */
#define lwprintf lwprintf_printf

/**
 * \copydoc         lwprintf_vprintf
 * \note            This function is equivalent to \ref lwprintf_vprintf
 *                      and available only if \ref
 * LWPRINTF_CFG_ENABLE_SHORTNAMES is enabled
 */
#define lwvprintf lwprintf_vprintf

/**
 * \copydoc         lwprintf_vsnprintf
 * \note            This function is equivalent to \ref lwprintf_vsnprintf
 *                      and available only if \ref
 * LWPRINTF_CFG_ENABLE_SHORTNAMES is enabled
 */
#define lwvsnprintf lwprintf_vsnprintf

/**
 * \copydoc         lwprintf_snprintf
 * \note            This function is equivalent to \ref lwprintf_snprintf
 *                      and available only if \ref
 * LWPRINTF_CFG_ENABLE_SHORTNAMES is enabled
 */
#define lwsnprintf lwprintf_snprintf

/**
 * \copydoc         lwprintf_sprintf
 * \note            This function is equivalent to \ref lwprintf_sprintf
 *                      and available only if \ref
 * LWPRINTF_CFG_ENABLE_SHORTNAMES is enabled
 */
#define lwsprintf lwprintf_sprintf

#endif /* LWPRINTF_CFG_ENABLE_SHORTNAMES || __DOXYGEN__ */

#if LWPRINTF_CFG_ENABLE_STD_NAMES || __DOXYGEN__

/**
 * \copydoc         lwprintf_printf
 * \note            This function is equivalent to \ref lwprintf_printf
 *                      and available only if \ref LWPRINTF_CFG_ENABLE_STD_NAMES
 * is enabled
 */
#define printf lwprintf_printf

/**
 * \copydoc         lwprintf_vprintf
 * \note            This function is equivalent to \ref lwprintf_vprintf
 *                      and available only if \ref LWPRINTF_CFG_ENABLE_STD_NAMES
 * is enabled
 */
#define vprintf lwprintf_vprintf

/**
 * \copydoc         lwprintf_vsnprintf
 * \note            This function is equivalent to \ref lwprintf_vsnprintf
 *                      and available only if \ref LWPRINTF_CFG_ENABLE_STD_NAMES
 * is enabled
 */
#define vsnprintf lwprintf_vsnprintf

/**
 * \copydoc         lwprintf_snprintf
 * \note            This function is equivalent to \ref lwprintf_snprintf
 *                      and available only if \ref LWPRINTF_CFG_ENABLE_STD_NAMES
 * is enabled
 */
#define snprintf lwprintf_snprintf

/**
 * \copydoc         lwprintf_sprintf
 * \note            This function is equivalent to \ref lwprintf_sprintf
 *                      and available only if \ref LWPRINTF_CFG_ENABLE_STD_NAMES
 * is enabled
 */
#define sprintf lwprintf_sprintf

#endif /* LWPRINTF_CFG_ENABLE_STD_NAMES || __DOXYGEN__ */

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWPRINTF_HDR_H */
