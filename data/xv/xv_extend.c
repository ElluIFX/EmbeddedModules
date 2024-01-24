#include "xv_extend.h"

#include <math.h>
#include <string.h>

#include "log.h"
#include "modules.h"
#include "udict.h"

#define CHECK_ARG_LEN(n) \
  if (xv_array_length(args) != n) return xv_new_error("Invalid argument")

static struct xv xve_abs(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(fabs(x));
}

static struct xv xve_sqrt(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(sqrt(x));
}

static struct xv xve_exp(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(exp(x));
}

static struct xv xve_log(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(log(x));
}

static struct xv xve_log10(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(log10(x));
}

static struct xv xve_pow(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(2);
  double x = xv_double(xv_array_at(args, 0));
  double y = xv_double(xv_array_at(args, 1));
  return xv_new_double(pow(x, y));
}

static struct xv xve_sin(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(sin(x));
}

static struct xv xve_cos(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(cos(x));
}

static struct xv xve_tan(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(tan(x));
}

static struct xv xve_asin(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(asin(x));
}

static struct xv xve_acos(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(acos(x));
}

static struct xv xve_atan(struct xv this, struct xv args, void *udata) {
  CHECK_ARG_LEN(1);
  double x = xv_double(xv_array_at(args, 0));
  return xv_new_double(atan(x));
}

static bool strcmpn(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i]) return false;
  }
  return true;
}

static bool avail_varname(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

static bool find_var_assign(const char *cmd, char *varname, char **varvalue) {
#define CHECK_ZERO() \
  if (*cmd == 0) return false;
#define SKIP_SPACE()    \
  while (*cmd == ' ') { \
    cmd++;              \
    CHECK_ZERO();       \
  }
  SKIP_SPACE();
  uint8_t i = 0;
  while (avail_varname(*cmd)) {
    varname[i] = *cmd++;
    varname[++i] = 0;
    CHECK_ZERO();
  }
  SKIP_SPACE();
  if (*cmd != '=') return false;
  cmd++;
  SKIP_SPACE();
  *varvalue = (char *)cmd;
  return true;
}

static struct xv_env env = {.ref = NULL, .udata = NULL};

static struct xv xve_get_ref(struct xv this, struct xv ident, void *udata) {
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
      if (xv_string_compare(ident, xve_map[i].name) == 0) {
        return xv_new_function(xve_map[i].func);
      }
    }
    if (xv_string_compare(ident, "pi") == 0) {
      return xv_new_double(3.14159265358979323846);
    } else if (xv_string_compare(ident, "e") == 0) {
      return xv_new_double(2.71828182845904523536);
    }
    UDICT dict = (UDICT)env.udata;
    struct xv *value = udict_get(dict, xv_string(ident));
    if (value) return *value;
  }
  return xv_new_undefined();
}

static void xve_on_cmd(EmbeddedCli *cli, CliCommand *command) {
  char *str = (char *)command->name;
  if (strcmpn(str, "exit", 4) || strcmpn(str, "quit", 4)) {
    embeddedCliExitSubInterpreter(cli);
    return;
  }
  str[strlen(str)] = ' ';  // \0 -> ' '

  char varname[32];
  char *varvalue;
  bool is_assign = find_var_assign(str, varname, &varvalue);
  if (is_assign) str = varvalue;

  struct xv value = xv_eval(str, &env);

  char *result = xv_string(value);
  if (xv_type(value) != XV_ERROR && is_assign) {
    UDICT dict = (UDICT)env.udata;
    udict_set_copy(dict, varname, &value, sizeof(value));
    LOG_RAWLN("[Varible '%s'] = %s", varname, result);
  } else {
    LOG_RAWLN("%s", result);
  }
  m_free(result);
}

static void xve_on_exit(EmbeddedCli *cli) {
  xv_cleanup();
  udict_free((UDICT)env.udata);
  LOG_RAWLN("Xv: Interpreter cleaned up");
}

static void xve_enter(EmbeddedCli *cli, char *args, void *context) {
  env.ref = xve_get_ref;
  env.udata = (void *)udict_create();
  embeddedCliEnterSubInterpreter(cli, xve_on_cmd, xve_on_exit, "XV > ");
}

void XVExtend_AddCmdToCli(EmbeddedCli *cli) {
  static CliCommandBinding xv_cmd = {
      .name = "xv",
      .usage = "xv",
      .help = "Enter Xv Interpreter",
      .context = NULL,
      .autoTokenizeArgs = 0,
      .func = xve_enter,
  };
  embeddedCliAddBinding(cli, xv_cmd);
}
