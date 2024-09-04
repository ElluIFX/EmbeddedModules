#ifndef __XV_EXTEND_H__
#define __XV_EXTEND_H__

#include "embedded_cli.h"
#include "xv.h"

typedef struct xv (*XvExtFunc)(struct xv this, struct xv ident, void* udata);

extern void xv_ex_add_command_to_cli(EmbeddedCli* cli);
extern void xv_ex_add_user_func(const char* name, XvExtFunc func);

#endif /* __XV_EXTEND_H__ */
