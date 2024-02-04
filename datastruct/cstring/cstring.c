#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modules.h"
#include "stdarg.h"

#define cstring_alloc m_alloc
#define cstring_free m_free

static const size_t npos = -1;

typedef struct string_s {
  char *data;
} string_t;

typedef string_t *string;

void string_assign(string dest, const char *src) {
  if (src == NULL) {
    if (dest->data != NULL) cstring_free(dest->data);
    dest->data = NULL;
    return;
  }
  if (dest->data != NULL) {
    cstring_free(dest->data);
    dest->data = NULL;
  }
  dest->data = cstring_alloc(sizeof(char) * strlen(src) + 1);
  strcpy(dest->data, src);
}

string string_create(const char *str) {
  string this = cstring_alloc(sizeof(string_t));
  this->data = NULL;
  string_assign(this, str);
  return this;
}

void string_copy(string dest, const string src) {
  string_assign(dest, src->data);
}

string string_duplicate(const string this) { return string_create(this->data); }

void string_clear(string this) { string_assign(this, ""); }

const char *string_data(const string this) { return this->data; }

const char *string_c_str(const string this) { return this->data; }

size_t string_size(const string this) {
  if (this->data == NULL) {
    return 0;
  }
  return strlen(this->data);
}

size_t string_length(const string this) {
  if (this->data == NULL) {
    return 0;
  }
  return strlen(this->data);
}

int string_compare_c_str(const string this, const char *other) {
  if (this->data == NULL && other == NULL) {
    return 0;
  }
  if (this->data == NULL || other == NULL) {
    return -1;
  }
  return strcmp(this->data, other);
}

int string_compare(const string this, const string other) {
  return string_compare_c_str(this, other->data);
}

char string_at(const string this, size_t pos) {
  if (this->data == NULL) {
    return 0;
  }
  if (pos >= string_length(this)) {
    return 0;
  }
  return this->data[pos];
}

void string_push_back(string this, char insertion) {
  char insertion_str[2];
  insertion_str[0] = insertion;
  insertion_str[1] = '\0';

  if (this->data == NULL) return string_assign(this, insertion_str);

  char *tmp = cstring_alloc(sizeof(char) * (string_length(this) + 2));
  strcpy(tmp, this->data);
  strcat(tmp, insertion_str);
  string_assign(this, tmp);
  cstring_free(tmp);
}

void string_pop_back(string this) {
  size_t len = string_length(this);
  if (len == 0) {
    return;
  }
  this->data[len - 1] = '\0';
}

void string_reverse(string this) {
  if (this->data == NULL) {
    return;
  }
  size_t i;
  size_t size = strlen(this->data);
  size_t j = size - 1;
  char temp;
  for (i = 0; i < size / 2; i++, j--) {
    temp = this->data[i];
    this->data[i] = this->data[j];
    this->data[j] = temp;
  }
  this->data[size] = '\0';
}

void string_swap(string this, string other) {
  char *tmp = cstring_alloc(sizeof(char) * string_length(this) + 1);
  strcpy(tmp, this->data);

  string_assign(this, other->data);
  string_assign(other, tmp);

  cstring_free(tmp);
}

void string_cat(string this, const char *other) {
  if (other == NULL) {
    return;
  }
  char *tmp =
      cstring_alloc(sizeof(char) * (string_length(this) + strlen(other) + 1));
  if (this->data == NULL) {
    string_assign(this, "");
  }
  strcpy(tmp, this->data);
  strcat(tmp, other);
  string_assign(this, tmp);
  cstring_free(tmp);
}

void string_append(string this, const string other) {
  string_cat(this, other->data);
}

void string_insert_c_str(string this, const char *insertion, size_t pos) {
  if (insertion == NULL) {
    return;
  }
  if (pos > string_length(this)) {
    return;
  }
  if (pos == string_length(this)) {
    string_cat(this, insertion);
    return;
  }
  char *tmp = cstring_alloc(sizeof(char) *
                            (string_length(this) + strlen(insertion) + 1));
  strncpy(tmp, this->data, pos);
  tmp[pos] = '\0';
  strcat(tmp, insertion);
  strcat(tmp, this->data + pos);
  string_assign(this, tmp);
  cstring_free(tmp);
}

void string_insert(string this, const string insertion, size_t pos) {
  string_insert_c_str(this, insertion->data, pos);
}

bool string_empty(const string this) {
  if (this->data == NULL) return true;
  if (string_compare_c_str(this, "") == 0) return true;
  return false;
}

void string_destroy(string this) {
  if (this->data != NULL) cstring_free(this->data);
  cstring_free(this);
}

size_t string_find_c_str(const string this, const char *to_find) {
  char *res = strstr(this->data, to_find);
  if (!res) return npos;
  return res - this->data;
}

size_t string_find(const string this, const string to_find) {
  return string_find_c_str(this, to_find->data);
}

char *cstring_strrstr(const char *haystack, const char *needle) {
  char *r = NULL;

  if (!needle[0]) return (char *)haystack + strlen(haystack);
  while (1) {
    char *p = strstr(haystack, needle);
    if (!p) return r;
    r = p;
    haystack = p + 1;
  }
}

size_t string_rfind_c_str(const string this, const char *to_find) {
  char *res = cstring_strrstr(this->data, to_find);
  if (!res) return npos;
  return res - this->data;
}

size_t string_rfind(const string this, const string to_find) {
  return string_rfind_c_str(this, to_find->data);
}

string string_substr(const string this, size_t pos, size_t len) {
  string tmp = string_create("");
  for (size_t i = 0; pos < string_size(this) && i < len; i++, pos++)
    string_push_back(tmp, string_at(this, pos));
  return tmp;
}

size_t string_printf(string this, const char *format, ...) {
  va_list args;
  va_start(args, format);
  size_t size = vsnprintf(NULL, 0, format, args);
  va_end(args);

  char *tmp = cstring_alloc(sizeof(char) * (size + 1));
  va_start(args, format);
  vsnprintf(tmp, size + 1, format, args);
  va_end(args);

  string_assign(this, tmp);
  cstring_free(tmp);
  return size;
}
