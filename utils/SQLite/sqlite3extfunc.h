#ifndef _SQLITE3EXTFUNC_H_
#define _SQLITE3EXTFUNC_H_

#include "sqlite3.h"

int sqlite3_extmathstrfunc_init(
        sqlite3 *db, char **pzErrMsg, void* ptr);


#endif
