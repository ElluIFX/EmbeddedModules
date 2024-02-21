/*
 *
 * MINCTEST - Minimal C Test Library - 0.3.0
 *
 * Copyright (c) 2014-2021 Lewis Van Winkle
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

/*
 * MINCTEST - Minimal testing library for C
 *
 *
 * Example:
 *
 *      void test1() {
 *           lassert('a' == 'a');
 *      }
 *
 *      void test2() {
 *           lequal(5, 6);
 *           lfequal(5.5, 5.6);
 *      }
 *
 *      int main() {
 *           lrun("test1", test1);
 *           lrun("test2", test2);
 *           lresults();
 *           return _lfails != 0;
 *      }
 *
 *
 *
 * Hints:
 *      All functions/variables start with the letter 'l'.
 *
 */

#ifndef __MINCTEST_H__
#define __MINCTEST_H__
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "modules.h"

/* How far apart can floats be before we consider them unequal. */
#ifndef LTEST_FLOAT_TOLERANCE
#define LTEST_FLOAT_TOLERANCE 0.00001
#endif

/* Track the number of passes, fails. */
/* NB this is made for all tests to be in one file. */
static size_t _ltests = 0;
static size_t _lfails = 0;

/* Display the test results. */
#define lresults()                                                          \
  do {                                                                      \
    if (_lfails == 0) {                                                     \
      LOG_RAWLN("ALL TESTS PASSED (%zu/%zu)", _ltests, _ltests);            \
    } else {                                                                \
      LOG_RAWLN("SOME TESTS FAILED (%zu/%zu)", _ltests - _lfails, _ltests); \
    }                                                                       \
  } while (0)

/* Run a test. Name can be any string to print out, test is the function name to
 * call. */
#define lrun(name, test)                                     \
  do {                                                       \
    const size_t ts = _ltests;                               \
    const size_t fs = _lfails;                               \
    const m_time_t start = m_time_us();                      \
    LOG_RAWLN("Test: %s", name);                             \
    test();                                                  \
    LOG_RAWLN(" -- pass: %-4zu fail: %-4zu cost: %ldus",     \
              (_ltests - ts) - (_lfails - fs), _lfails - fs, \
              (m_time_us() - start));                        \
  } while (0)

/* Assert a true statement. */
#define lassert(test)                                        \
  do {                                                       \
    ++_ltests;                                               \
    if (!(test)) {                                           \
      ++_lfails;                                             \
      LOG_RAWLN(" %s:%d (assert fail)", __FILE__, __LINE__); \
    }                                                        \
  } while (0)

/* Prototype to assert equal. */
#define _lequal_base(equality, a, b, format)                                  \
  do {                                                                        \
    ++_ltests;                                                                \
    if (!(equality)) {                                                        \
      ++_lfails;                                                              \
      LOG_RAWLN(" %s:%d (" format " != " format ")", __FILE__, __LINE__, (a), \
                (b));                                                         \
    }                                                                         \
  } while (0)

/* Assert two integers are equal. */
#define lequal(a, b) _lequal_base((a) == (b), a, b, "%d")

/* Assert two floats are equal (Within a given tolerance). */
#define lfequal_t(a, b, tol)                                                  \
  _lequal_base(                                                               \
      fabs((double)(a) - (double)(b)) <= tol &&                               \
          fabs((double)(a) - (double)(b)) == fabs((double)(a) - (double)(b)), \
      (double)(a), (double)(b), "%f")

/* Assert two floats are equal (Within LTEST_FLOAT_TOLERANCE). */
#define lfequal(a, b) lfequal_t(a, b, LTEST_FLOAT_TOLERANCE)

/* Assert two strings are equal. */
#define lsequal(a, b) _lequal_base(strcmp(a, b) == 0, a, b, "%s")

#endif /*__MINCTEST_H__*/
