#include "ffiutil.h"

#define JSMN_STATIC
#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#include "jsmn.h"

static inline bool stringcmp(const char* s1, const char* s2, int len) {
    int len2 = strlen(s2);
    if (len != len2) {
        return false;
    }
    for (int i = 0; i < len; i++) {
        if (s1[i] != s2[i]) {
            return false;
        }
    }
    return true;
}

static void ffiutil_free_type(ffi_type* type) {
    if (type == NULL || type->elements == NULL) {
        return;
    }
    for (int i = 0; type->elements[i] != NULL; i++) {
        ffiutil_free_type(type->elements[i]);
    }
    free(type);
}

static ffi_type* ffiutil_parse_type(const char* json, jsmntok_t* tok, int* i) {
    ffi_type* type = NULL;
    jsmntok_t* curr = tok + *i;
    const char* str = json + curr->start;
    int len = curr->end - curr->start;
    if (curr->type == JSMN_STRING) {
        // primitive types
        if (stringcmp(str, "void", len)) {
            // void type
            type = &ffi_type_void;
        } else if (stringcmp(str, "ptr", len)) {
            // `void*`, pointer
            type = &ffi_type_pointer;
        } else if (stringcmp(str, "uchar", len)) {
            // unsigned char 
            type = &ffi_type_uchar;
        } else if (stringcmp(str, "char", len)) {
            // char 
            type = &ffi_type_schar;
        } else if (stringcmp(str, "ushort", len)) {
            // unsigned short
            type = &ffi_type_ushort;
        } else if (stringcmp(str, "short", len)) {
            // short 
            type = &ffi_type_sshort;
        } else if (stringcmp(str, "uint", len)) {
            // unsigned int 
            type = &ffi_type_uint;
        } else if (stringcmp(str, "int", len)) {
            // int 
            type = &ffi_type_sint;
        } else if (stringcmp(str, "ulong", len)) {
            // unsigned long int 
            type = &ffi_type_ulong;
        } else if (stringcmp(str, "long", len)) {
            // long int 
            type = &ffi_type_slong;
        } else if (stringcmp(str, "u8", len)) {
            // uint8_t
            type = &ffi_type_uint8;
        } else if (stringcmp(str, "i8", len)) {
            // int8_t
            type = &ffi_type_sint8;
        } else if (stringcmp(str, "u16", len)) {
            // int16_t
            type = &ffi_type_uint16;
        } else if (stringcmp(str, "i16", len)) {
            // int16_t
            type = &ffi_type_sint16;
        } else if (stringcmp(str, "u32", len)) {
            // uint32_t
            type = &ffi_type_uint32;
        } else if (stringcmp(str, "i32", len)) {
            // int32_t
            type = &ffi_type_sint32;
        } else if (stringcmp(str, "u64", len)) {
            // uint64_t
            type = &ffi_type_uint64;
        } else if (stringcmp(str, "i64", len)) {
            // int64_t
            type = &ffi_type_sint64;
        } else if (stringcmp(str, "f32", len)) {
            // float
            type = &ffi_type_float;
        } else if (stringcmp(str, "f64", len)) {
            // double
            type = &ffi_type_double;
        } else if (stringcmp(str, "f128", len)) {
            // long double, which might not actually be 128 bits on some platforms
            type = &ffi_type_longdouble;
        } else {
            // handle error 
            return NULL;
        }
    } else if (curr->type == JSMN_ARRAY) {
        // struct
        // allocate ffi_type for struct
        type = malloc(sizeof (ffi_type));
        type->size = 0;
        type->alignment = 0;
        type->type = FFI_TYPE_STRUCT;
        // get fields count and allocate memory
        int fields = curr->size;
        type->elements = malloc(sizeof (ffi_type) * (fields + 1));
        // parse array 
        *i += 1;
        for (int j = 0; j < fields; j++) {
            ffi_type* temp = ffiutil_parse_type(json, tok, i);
            if (temp == NULL) {
                // handle error 
                return NULL;
            }
            type->elements[j] = temp;
        }
        // set last element to `NULL`
        type->elements[fields] = NULL;
    } else {
        // handle error 
        return NULL;
    }

    *i += 1;
    return type;
}

#define TOK_SIZE 64
ffi_cif* ffiutil_parse_cif(const char* json) {
    // init variables
    ffi_cif* res = NULL;
    ffi_type* rtype = NULL;
    ffi_type** atypes = NULL;
    int nargs = 0;
    
    // init jsmn
    jsmn_parser parser;
    size_t tok_size = TOK_SIZE;
    jsmntok_t* tok = malloc(sizeof (jsmntok_t) * tok_size);
    jsmn_init(&parser);
    size_t json_len = strlen(json);
    int count;
    while ((count = jsmn_parse(&parser, json, json_len, tok, tok_size)) == JSMN_ERROR_NOMEM) {
        tok_size  = tok_size * 2;
        tok = realloc(tok, sizeof (jsmntok_t) * tok_size);
    }
    if (count < 0) {
        goto handle_error;
    }

    // parse json
    res = malloc(sizeof (ffi_cif));
    memset(res, 0, sizeof (ffi_cif));

    jsmntok_t* curr_tok;
    for (int i = 0; i < count;) {
        curr_tok = tok + i;

        if (curr_tok->type == JSMN_STRING) {
            const char* str = json + curr_tok->start;
            int len = curr_tok->end - curr_tok->start;
            int child = curr_tok->size;

            if (stringcmp(str, "atypes", len)) {
                // handle argument types
                if (child != 1 || atypes != NULL) {
                    // handle error
                    goto handle_error;
                }
                // child should be an array
                i += 1;
                curr_tok = tok + i;
                if (curr_tok->type != JSMN_ARRAY) {
                    // handle error
                    goto handle_error;
                }
                // get nargs and allocate memory
                nargs = curr_tok->size;
                atypes = malloc(sizeof (ffi_type*) * nargs);
                // parse array 
                i += 1;
                for (int j = 0; j < nargs; j++) {
                    ffi_type* temp = ffiutil_parse_type(json, tok, &i);
                    if (temp == NULL) {
                        // handle error 
                        goto handle_error;
                    }
                    atypes[j] = temp;
                }
            } else if (stringcmp(str, "rtype", len)) {
                // handle return type
                if (child != 1 || rtype != NULL) {
                    // handle error
                    goto handle_error;
                }
                // child should be string or array
                i += 1;
                ffi_type* temp = ffiutil_parse_type(json, tok, &i);
                if (temp == NULL) {
                    // handle error 
                    goto handle_error;
                }
                rtype = temp;
            } else {
                // ignored
                i += 1;
            }
        } else {
            i += 1;
        }
    }

    if (ffi_prep_cif(res, FFI_DEFAULT_ABI, nargs, rtype, atypes) != FFI_OK) {
        // handle error
        res = NULL;
        goto handle_error;
    }

    goto jsmn_cleanup;

handle_error:
    ffiutil_free_type(rtype);
    for (int i = 0; i < nargs; i++) {
        ffiutil_free_type(atypes[i]);
    }
    free(rtype);
    free(atypes);
    free(res);
    res = NULL;

jsmn_cleanup: 
    free(tok);

    return res;
}

void ffiutil_free_cif(ffi_cif* cif) {
    if (cif == NULL) {
        return;
    }
    ffiutil_free_type(cif->rtype);
    for (size_t i = 0; i < cif->nargs; i++) {
        ffiutil_free_type(cif->arg_types[i]);
    }
    // free(cif->rtype);
    free(cif->arg_types);
    free(cif);
}
