#ifndef __FFIUTIL_H__
#define __FFIUTIL_H__

#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


ffi_cif*    ffiutil_parse_cif(const char* json);
void        ffiutil_free_cif(ffi_cif* cif);


#endif  // __FFIUTIL_H__
