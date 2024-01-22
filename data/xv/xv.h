#ifndef XV_H
#define XV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "modules.h"

// 结构体 xv 是一个表达式值。
// 这通常作为 xv_eval 函数的结果创建，但也可以使用 xv_new_*() 函数进行创建。
struct xv {
  uint64_t priv[2];
};

// 枚举 xv_type 是从 xv_type() 函数返回的。
enum xv_type {
  XV_UNDEFINED,
  XV_NULL,
  XV_STRING,
  XV_NUMBER,
  XV_BOOLEAN,
  XV_FUNCTION,
  XV_OBJECT,
};

// struct xv_env 是一个提供给 xv_eval 的自定义环境。
struct xv_env {
  // no_case 告诉 xv_eval 执行不区分大小写的比较。
  bool no_case;
  // udata 是自定义用户数据。
  void *udata;
  // ref 是一个回调函数，它返回未知标识符、属性和函数的引用值。
  struct xv (*ref)(struct xv this, struct xv ident, void *udata);
};

// xv_eval 函数用来评估一个表达式并返回结果值。
//
// 如果 xv_eval 的结果出现错误，例如语法错误或系统内存耗尽，
// 则可以使用 xv_is_err() 来检查结果。
struct xv xv_eval(const char *expr, struct xv_env *env);
struct xv xv_evaln(const char *expr, size_t len, struct xv_env *env);

// xv_cleanup 函数用于重置环境并释放可能在 xv_eval 过程中分配的任何内存。
//
// 当你完全完成了 xv_eval 调用的结果值使用后，应该调用此函数。
// 因此，每一个 xv_eval 调用应该对应一个 xv_cleanup。
//
// 调用 xv_cleanup 后，不要再使用返回的'struct xv' 结果，
// 否则你将面临不确定的行为风险。
//
// 关于线程安全性。
// xv_eval 和 xv_cleanup 是使用线程局部变量和分配的线程安全函数。
void xv_cleanup(void);

// xv_string_copy 将值作为字符串表示复制到提供的 C 字符串缓冲区中。
//
// 返回存储字符串表示到 C 字符串缓冲区所需的字符数，不包括空字符终止符。
// 如果返回的长度大于 nbytes-1，则只发生了部分复制，例如：
//
//    char buf[64];
//    size_t len = xv_string_copy(value, str, sizeof(str));
//    if (len > sizeof(str)-1) {
//        // ... 复制未完成 ...
//    }
//
size_t xv_string_copy(struct xv value, char *dst, size_t n);

// xv_string 返回值的字符串表示。
//
// 该操作将分配新的内存，调用者有责任在以后的某个时候释放这个字符串。
//
// 或者，可以使用 xv_string_copy 来避免分配内存。
//
// 如果系统内存不足，返回 NULL。
char *xv_string(struct xv value);

// xv_string_length 返回值的字符串表示的长度。
size_t xv_string_length(struct xv value);

// xv_double 返回值的双精度浮点数表示。
double xv_double(struct xv value);

// xv_int64 返回值的 int64_t（64位整数）表示。
int64_t xv_int64(struct xv value);

// xv_uint64 返回值的 uint64_t（64位无符号整数）表示。
uint64_t xv_uint64(struct xv value);

// xv_object 返回用户定义的对象，如果没有则返回 NULL。
const void *xv_object(struct xv value);

// xv_object_tag 返回用户定义对象的标签，如果没有则返回 0。
uint32_t xv_object_tag(struct xv value);

// xv_bool 返回值的布尔表示。
bool xv_bool(struct xv value);

// xv_type 返回值的类型。
enum xv_type xv_type(struct xv value);

// xv_is_undefined 返回 true 如果值未定义。
bool xv_is_undefined(struct xv value);

// xv_is_global 函数如果值是全局变量则返回 true。
//
// 这主要用在 ref 回调中。当 'this' 值是全局的，
// 则 'ident' 应该是一个全局变量，否则 'ident' 应该是 'this' 的属性。
bool xv_is_global(struct xv value);

// xv_is_error 函数如果执行发生错误则返回 true。
bool xv_is_error(struct xv value);

// xv_is_oom 函数如果系统内存不足则返回 true。
bool xv_is_oom(struct xv value);

// xv_array_length
// 返回数组值中的元素数量，如果值不是数组或者没有元素，则返回零。
size_t xv_array_length(struct xv value);

// xv_array_at
// 返回数组值中索引位置的元素，如果值不是数组或者索引超出范围，则返回未定义。
struct xv xv_array_at(struct xv value, size_t index);

// xv_string_compare 将值与一个 C 字符串进行比较。
//
// 这个函数执行的是二进制字符比较。逐个比较两个字符串中的每个字符。
// 如果它们等于彼此，则一直进行比较，直到字符不同，或者到达值的结束或 C
// 字符串中的空字符终止符。 如果结果是小于、等于、大于则返回 < 0、0、> 0。
int xv_string_compare(struct xv value, const char *str);
int xv_string_comparen(struct xv value, const char *str, size_t len);

int xv_string_equal(struct xv value, const char *str);
int xv_string_equaln(struct xv value, const char *str, size_t len);

// value construction
struct xv xv_new_string(const char *str);
struct xv xv_new_stringn(const char *str, size_t len);
struct xv xv_new_object(const void *ptr, uint32_t tag);
struct xv xv_new_json(const char *json);
struct xv xv_new_jsonn(const char *json, size_t len);
struct xv xv_new_double(double d);
struct xv xv_new_int64(int64_t i);
struct xv xv_new_uint64(uint64_t u);
struct xv xv_new_boolean(bool t);
struct xv xv_new_undefined(void);
struct xv xv_new_null(void);
struct xv xv_new_error(const char *msg);
struct xv xv_new_array(const struct xv *const *values, size_t nvalues);
struct xv xv_new_function(struct xv (*func)(struct xv this,
                                            const struct xv args, void *udata));

// struct xv_memstats 是 xv_memstats 返回的结构体
struct xv_memstats {
  size_t thread_total_size;  // 线程局部内存空间的总大小
  size_t thread_size;        // 已使用的线程局部内存空间的字节数
  size_t thread_allocs;      // 线程局部内存空间的分配次数
  size_t heap_allocs;        // 线程局部堆分配次数
  size_t heap_size;          // 已使用的线程局部堆字节数
};

// xv_memstats 返回由于 xv_eval 调用产生的各种内存统计信息。
//
// 这些统计信息可以通过调用 xv_cleanup 来重置。
struct xv_memstats xv_memstats(void);

#endif  // XV_H
