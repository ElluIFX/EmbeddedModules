#include "xv_extend.h"

#include <math.h>
#include <string.h>

#include "modules.h"
#include "udict.h"

#define LOG_MODULE "xve"
#include "log.h"

static char err_buf[48];
static struct xv_env global_env = {.ref = NULL, .udata = NULL};
static UDICT xve_userfunc_dict = NULL;

static struct xv xve_get_ref(struct xv this, struct xv ident, void* udata);

static bool strcmpn(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i])
            return false;
    }
    return true;
}

static size_t strcpyn(char* dst, const char* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];
        if (src[i] == 0)
            return i;
    }
    return n;
}

#define CHECK_ARG_LEN(n)                                                 \
    if (xv_array_length(args) != (n)) {                                  \
        sprintf(err_buf, "Invalid argument: expects %d but %d given", n, \
                xv_array_length(args));                                  \
        return xv_new_error(err_buf);                                    \
    }

static struct xv xve_abs(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(fabs(x));
}

static struct xv xve_sqrt(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(sqrt(x));
}

static struct xv xve_exp(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(exp(x));
}

static struct xv xve_log(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(log(x));
}

static struct xv xve_log10(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(log10(x));
}

static struct xv xve_pow(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(2);
    double x = xv_double(xv_array_at(args, 0));
    double y = xv_double(xv_array_at(args, 1));
    return xv_new_double(pow(x, y));
}

static struct xv xve_sin(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(sin(x));
}

static struct xv xve_cos(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(cos(x));
}

static struct xv xve_tan(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(tan(x));
}

static struct xv xve_asin(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(asin(x));
}

static struct xv xve_acos(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(acos(x));
}

static struct xv xve_atan(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_double(atan(x));
}

static struct xv xve_round(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    int64_t y = (int64_t)(x + 0.5);
    return xv_new_int64(y);
}

static struct xv xve_floor(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_int64(floor(x));
}

static struct xv xve_ceil(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(1);
    double x = xv_double(xv_array_at(args, 0));
    return xv_new_int64(ceil(x));
}

static struct xv xve_rand(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(0);
    return xv_new_double((double)rand() / RAND_MAX);
}

static struct xv xve_randint(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(2);
    int64_t a = xv_int64(xv_array_at(args, 0));
    int64_t b = xv_int64(xv_array_at(args, 1));
    if (a > b) {
        int64_t t = a;
        a = b;
        b = t;
    }
    return xv_new_int64(a + rand() % (b - a + 1));
}

static struct xv xve_clock(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(0);
    return xv_new_int64(m_tick());
}

static struct xv xve_time(struct xv this, struct xv args, void* udata) {
    CHECK_ARG_LEN(0);
    return xv_new_double((double)m_tick() / (double)m_tick_clk);
}

typedef struct xve_lambda_t {
    struct xve_lambda_t* next;
    UDICT locals;
    char* statement;
    int argc;
    char* args[];
} xve_lambda_t;

static xve_lambda_t* xve_lambda_list = NULL;

static struct xv xve_func_exec(struct xv this, struct xv args, void* udata) {
    xve_lambda_t* lmb = (xve_lambda_t*)udata;
    CHECK_ARG_LEN(lmb->argc);
    UDICT data =
        lmb->locals != NULL ? lmb->locals : udict_copy((UDICT)global_env.udata);
    if (!data) {
        return xv_new_error("lambda error: out of memory");
    }
    struct xv_env env = {.ref = xve_get_ref, .udata = data};
    struct xv value;
    for (int i = 0; i < lmb->argc; i++) {
        value = xv_array_at(args, i);
        udict_set_copy(data, lmb->args[i], &value, sizeof(struct xv));
    }
    value = xv_eval(lmb->statement, &env);
    if (lmb->locals == NULL)
        udict_free(data);
    return value;
}

static struct xv xve_func(struct xv this, struct xv args, void* udata) {
    int argc = xv_array_length(args);
    if (argc < 1) {
        return xv_new_error("lambda error: no valid statement");
    }
    for (int i = 0; i < argc; i++) {
        struct xv arg = xv_array_at(args, i);
        if (xv_type(arg) != XV_STRING) {
            return xv_new_error("lambda error: statement should be string");
        }
    }
    xve_lambda_t* lmb =
        m_alloc(sizeof(xve_lambda_t) + (argc - 1) * sizeof(char*));
    memset(lmb, 0, sizeof(xve_lambda_t));
    lmb->argc = argc - 1;
    for (int i = 0; i < lmb->argc; i++) {
        lmb->args[i] = xv_string(xv_array_at(args, i));
    }
    lmb->statement = xv_string(xv_array_at(args, argc - 1));
    lmb->next = xve_lambda_list;
    xve_lambda_list = lmb;
    return xv_new_function_wrapper(xve_func_exec, lmb);
}

static struct xv xve_lambda(struct xv this, struct xv args, void* udata) {
    struct xv ret = xve_func(this, args, udata);
    if (xv_type(ret) == XV_FUNCTION) {
        xve_lambda_list->locals = udict_copy((UDICT)global_env.udata);
        if (!xve_lambda_list->locals) {
            return xv_new_error("lambda error: out of memory");
        }
    }
    return ret;
}

static void xve_lambda_list_free(void) {
    xve_lambda_t* lmb = xve_lambda_list;
    while (lmb) {
        xve_lambda_t* next = lmb->next;
        for (int i = 0; i < lmb->argc; i++) {
            m_free(lmb->args[i]);
            if (lmb->locals) {
                udict_free(lmb->locals);
            }
        }
        m_free(lmb->statement);
        m_free(lmb);
        lmb = next;
    }
    xve_lambda_list = NULL;
}

static struct xv xve_get_ref(struct xv this, struct xv ident, void* udata) {
    const struct {
        const char* name;
        struct xv (*func)(struct xv this, struct xv args, void* udata);
    } xve_map[] = {
        {"abs", xve_abs},     {"sqrt", xve_sqrt},       {"exp", xve_exp},
        {"log", xve_log},     {"log10", xve_log10},     {"pow", xve_pow},
        {"sin", xve_sin},     {"cos", xve_cos},         {"tan", xve_tan},
        {"asin", xve_asin},   {"acos", xve_acos},       {"atan", xve_atan},
        {"round", xve_round}, {"floor", xve_floor},     {"ceil", xve_ceil},
        {"rand", xve_rand},   {"randint", xve_randint}, {"clock", xve_clock},
        {"time", xve_time},   {"lambda", xve_lambda},   {"func", xve_func},
    };

    if (xv_is_global(this)) {
        /**** Find internal function and const ****/
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
        UDICT dict = (UDICT)udata;
        char* key = xv_string(ident);
        struct xv* value;
        if (dict) {
            value = (struct xv*)udict_get(dict, key);
            /**** Find ans ****/
            if (xv_string_compare(ident, "ans") == 0) {
                m_free(key);
                if (value) {
                    return *value;
                } else {
                    return xv_new_int64(0);
                }
            }
            /**** Find variables ****/
            if (value) {
                m_free(key);
                return *value;
            }
        }
        /**** Find user defined function ****/
        if (xve_userfunc_dict) {
            XvExtFunc func = (XvExtFunc)udict_get(xve_userfunc_dict, key);
            m_free(key);
            if (func) {
                return xv_new_function(func);
            }
        }
        m_free(key);
    }
    return xv_new_undefined();
}

static bool avail_varname(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static bool find_var_assign(const char* cmd, char* varname, char** varvalue) {
#define CHECK_ZERO() \
    if (*cmd == 0)   \
        return false;
#define SKIP_SPACE()      \
    while (*cmd == ' ') { \
        cmd++;            \
        CHECK_ZERO();     \
    }

    SKIP_SPACE();
    uint8_t i = 0;
    if (*cmd >= '0' && *cmd <= '9')  //start with number
        return false;
    while (avail_varname(*cmd)) {
        varname[i] = *cmd++;
        varname[++i] = 0;
        CHECK_ZERO();
    }
    SKIP_SPACE();
    if (*cmd != '=' || *(cmd + 1) == '=')  // ignore ==
        return false;
    cmd++;
    SKIP_SPACE();
    *varvalue = (char*)cmd;
    return true;
}

static void xve_on_cmd(EmbeddedCli* cli, CliCommand* command) {
    char* str = (char*)command->name;
    str[strlen(str)] = ' ';  // \0 -> ' '

    bool ti = false;
    if (strcmpn(str, "exit", 4) || strcmpn(str, "quit", 4)) {
        embeddedCliExitSubInterpreter(cli);
        return;
    } else if (strcmpn(str, "reset", 5)) {
        xv_cleanup();
        udict_clear((UDICT)global_env.udata);
        PRINTLN("Xv: Environment cleared");
        return;
    } else if (strcmpn(str, "del ", 4)) {
        char* varname = str + 4;
        while (*varname == ' ')
            varname++;
        if (*varname == 0)
            return;
        UDICT dict = (UDICT)global_env.udata;
        if (!udict_has(dict, varname)) {
            PRINTLN("Xv: Variable '%s' not found", varname);
            return;
        }
        udict_del(dict, varname);
        return;
    }

    if (strcmpn(str, "%timeit ", 8)) {
        ti = true;
        str += 8;
    }

    char varname[32];
    char* varvalue;
    bool is_assign = find_var_assign(str, varname, &varvalue);
    if (is_assign)
        str = varvalue;
    struct xv value;
    if (!ti) {
        value = xv_eval(str, &global_env);
    } else {
        timeit("cost") {
            value = xv_eval(str, &global_env);
        }
    }
    char* result = xv_string(value);
    UDICT dict = (UDICT)global_env.udata;
    if (xv_type(value) != XV_ERROR && xv_type(value) != XV_UNDEFINED) {
        if (is_assign) {
            udict_set_copy(dict, varname, &value, sizeof(value));
        }
        udict_set_copy(dict, "ans", &value, sizeof(value));
    }
    if (is_assign) {
        PRINTLN("(%s)%s", varname, result);
    } else {
        PRINTLN("%s", result);
    }
    m_free(result);
}

static void xve_on_exit(EmbeddedCli* cli) {
    xv_cleanup();
    udict_free((UDICT)global_env.udata);
    xve_lambda_list_free();
    global_env.udata = NULL;
    PRINTLN("Xv: Interpreter cleaned up");
}

static void xve_enter(EmbeddedCli* cli, char* args, void* context) {
    if (strlen(args) > 0) {
        struct xv value = xv_eval(args, &global_env);
        char* result = xv_string(value);
        PRINTLN("%s", result);
        m_free(result);
        return;
    }
    global_env.udata = (void*)udict_new();
    embeddedCliEnterSubInterpreter(cli, xve_on_cmd, xve_on_exit, "Xv > ");
    PRINTLN("Xv: Interpreter initialized");
}

void xv_ex_add_command_to_cli(EmbeddedCli* cli) {
    global_env.ref = xve_get_ref;
    static CliCommandBinding xv_cmd = {
        .name = "xv",
        .usage = "xv [expression]",
        .help = "Enter Xv Interpreter",
        .context = NULL,
        .autoTokenizeArgs = 0,
        .func = xve_enter,
    };
    embeddedCliAddBinding(cli, xv_cmd);
}

void xv_ex_add_user_func(const char* name, XvExtFunc func) {
    if (!xve_userfunc_dict)
        xve_userfunc_dict = udict_new();
    udict_set(xve_userfunc_dict, name, (void*)func);
}
