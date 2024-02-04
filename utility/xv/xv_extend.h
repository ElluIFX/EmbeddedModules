#ifndef XV_EXTEND_H
#define XV_EXTEND_H

#include "embedded_cli.h"
#include "xv.h"

typedef struct xv (*XvExtFunc)(struct xv this, struct xv ident, void *udata);

extern void XVExtend_AddCmdToCli(EmbeddedCli *cli);
extern void XVExtend_AddUserFunc(const char *name, XvExtFunc func);

#endif
