#pragma once

#include <stdbool.h>
#include <stddef.h>

#define CSTRING_DEBUG 0

static const size_t npos = -1;

typedef struct string_t *string;

/**
 *  Change the value of a [string] object from a [char *]
 */
extern void string_assign(string dest, const char *src);
/**
 *  Change the value of a [string] object from another [string] object
 */
extern void string_copy(string dest, const string src);
/**
 *  Duplicate a string into a new alloced one
 *  @param this string to be duplicate.
 *  @returns string with a new adress with the same content as the string in
 * parameter
 */
extern string string_duplicate(const string this);
/**
 *  Clear a [string] object
 */
extern void string_clear(string this);
/**
 *  Return the current value of a [string] object as a [const char *]
 */
extern const char *string_data(const string this);
/**
 *  Return the current value of a [string] object as a [const char *]
 */
extern const char *string_c_str(const string this);
/**
 *  Return the size of the current value of a [string] object
 */
extern size_t string_size(const string this);
/**
 *  Return the lenght of the current value of a [string] object
 */
extern size_t string_length(const string this);
/**
 *  Compare a [string] and a [char *]. the result is the same as strcmp()
 */
extern int string_compare_c_str(const string this, const char *other);
/**
 *  Compare two [string] objects. the result is the same as strcmp()
 */
extern int string_compare(const string this, const string other);
/**
 *  Return the [char] located at this[pos]
 */
extern char string_at(const string this, size_t pos);
/**
 *  Add a [char] at the end of a [string] object
 */
extern void string_push_back(string this, char insertion);
/**
 *  Remove the last [char] of a [string] object
 */
extern void string_pop_back(string this);
/**
 *  Reverse the entire [string]
 */
extern void string_reverse(string this);
/**
 *  Swap the content of two [string] objects
 */
extern void string_swap(string this, string other);
/**
 *  Concatenate the value of a [string] object and a [char *]
 */
extern void string_cat(string this, const char *other);
/**
 *  Add the value of "new_string" at the end of "this"
 *  (Use string_cat to concatenate a [string] with a [char *])
 */
extern void string_append(string this, const string other);
/**
 *  Insert a [char *] at the position "pos" into a [string] object
 */
extern void string_insert_c_str(string this, const char *insertion, size_t pos);
/**
 *  Insert a [string] at the position "pos" into a [string] object
 */
extern void string_insert(string this, const string insertion, size_t pos);
/**
 *  Creates a [string] object with the content of [str]
 *  @param str content that will be stored by the [string]
 *  @returns a new string
 */
extern string string_create(const char *str);
/**
 *  Destroy a [string] object
 *  @param this string to be destroyed
 */
extern void string_destroy(string this);
/**
 *  @param this string struct
 *  @returns true if the string is empty / false otherwise.
 */
extern bool string_empty(const string this);
/**
 *  Searches the string for the first occurrence of the sequence specified by
 * its arguments.
 *  @param to_find A c_string with the subject to search for.
 *  @returns The position of the first character of the first match.
 */
extern size_t string_find_c_str(const string this, const char *to_find);
/**
 *  Searches the string for the first occurrence of the sequence specified by
 * its arguments.
 *  @param to_find Another string with the subject to search for.
 *  @returns The position of the first character of the first match.
 */
extern size_t string_find(const string this, const string to_find);
/**
 *  Searches the string for the last occurrence of the sequence specified by its
 * arguments.
 *  @param to_find A c_string with the subject to search for.
 *  @returns The position of the first character of the last match.
 */
extern size_t string_rfind_c_str(const string this, const char *to_find);
/**
 *  Searches the string for the last occurrence of the sequence specified by its
 * arguments.
 *  @param to_find Another string with the subject to search for.
 *  @returns The position of the first character of the last match.
 */
extern size_t string_rfind(const string this, const string to_find);
/**
 *  Returns a newly constructed string object with its value initialized to a
 * copy of a substring of this object
 *  @param pos Position of the first character to be copied as a substring
 *  @param len Number of characters to include in the substring (npos indicates
 * all characters until the end of the string)
 *  @returns A string object with a substring of this object
 */
extern string string_substr(const string this, size_t pos, size_t len);
/**
 * @brief Write content into string like printf
 */
extern size_t string_printf(string this, const char *format, ...);
