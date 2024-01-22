#include "xv_extend.h"

#include <math.h>
#include <string.h>

#include "log.h"
#include "modules.h"

struct xv xve_abs(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(fabs(x));
}

struct xv xve_sqrt(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(sqrt(x));
}

struct xv xve_exp(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(exp(x));
}

struct xv xve_log(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(log(x));
}

struct xv xve_log10(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(log10(x));
}

struct xv xve_pow(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  double y = xv_double(xv_array_at(args, 1));
  return xv_new_double(pow(x, y));
}

struct xv xve_sin(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(sin(x));
}

struct xv xve_cos(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(cos(x));
}

struct xv xve_tan(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(tan(x));
}

struct xv xve_asin(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(asin(x));
}

struct xv xve_acos(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(acos(x));
}

struct xv xve_atan(struct xv this, struct xv args, void *udata) {
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(atan(x));
}

struct xv xve_get_ref(struct xv this, struct xv ident, void *udata) {
  const struct {
    const char *name;
    struct xv (*func)(struct xv this, struct xv args, void *udata);
  } xve_map[] = {
      {"abs", xve_abs},   {"sqrt", xve_sqrt},   {"exp", xve_exp},
      {"log", xve_log},   {"log10", xve_log10}, {"pow", xve_pow},
      {"sin", xve_sin},   {"cos", xve_cos},     {"tan", xve_tan},
      {"asin", xve_asin}, {"acos", xve_acos},   {"atan", xve_atan},
  };

  if (xv_is_global(this)) {
    for (size_t i = 0; i < sizeof(xve_map) / sizeof(xve_map[0]); i++) {
      if (strcmp(xve_map[i].name, xv_string(ident)) == 0) {
        return xv_new_function(xve_map[i].func);
      }
    }
    if (strcmp("pi", xv_string(ident)) == 0) {
      return xv_new_double(3.14159265358979323846);
    } else if (strcmp("e", xv_string(ident)) == 0) {
      return xv_new_double(2.71828182845904523536);
    }
  } else {
    LOG_DEBUG("xve_get_local_ref: %s", xv_string(ident));
  }
  return xv_new_undefined();
}

void xve_on_cmd(EmbeddedCli *cli, CliCommand *command) {
  char *str = (char *)command->name;
  str[strlen(str)] = ' ';
  struct xv_env env = {.ref = xve_get_ref};
  struct xv value = xv_eval(str, &env);
  char *result = xv_string(value);
  LOG_RAWLN("%s", result);
  m_free(result);
  xv_cleanup();
}

void XVExtend_AddToCli(EmbeddedCli *cli) { cli->onCommand = xve_on_cmd; }
