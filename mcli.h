/*
 * mcli was brought to you by matyz
 * licensed under MIT
 */

#ifndef MCLI_H
#define MCLI_H

/* We include <string.h> header for strcmp 
 * You can provide your own strcmp implementation if you want :) */
#include <string.h>
/* For free(), malloc() and realloc() functions */
#include <stdlib.h>
#include <stdio.h>

/* ---------------------------
 *          STRUCTS
 * ---------------------------*/

struct argdef {
    struct {
        char* long_name;
        unsigned char accepts_value;
        char short_name;
    } hdr;
    union {
        int found;
        char* str;
    } value;
};

enum mcli_error_type {
    /* for example: `program --param` where `struct argdef param = argdef_long("param", 1);`*/
    MCLI_ERR_NO_VALUE,
    /* for example: `program -param` (we don't support flags like this) */
    MCLI_ERR_SHORT_ARG_TOO_LONG,
    /* for example: `program -` */
    MCLI_ERR_ONLY_HYPHEN_MINUS,
    MCLI_ERR_UNKNOWN_OPTION,
    MCLI_ERR_VALUE_WITHOUT_OPTION
};

struct mcli_error {
    enum mcli_error_type error_code;
    /*
     * for MCLI_ERR_NO_VALUE:             it's argument's name
     * for MCLI_ERR_SHORT_ARG_TOO_LONG:   it's argument's name
     * for MCLI_ERR_ONLY_HYPHEN_MINUS:    it's NULL
     * for MCLI_ERR_UNKNOWN_OPTION:       it's argument's name
     * for MCLI_ERR_VALUE_WITHOUT_OPTION: it's the value
     */
    char* value;
};

struct mcli_errbuf {
    struct mcli_error* ptr;
    unsigned int len;
};


/* ---------------------------
 *       IMPLEMENTATION
 * ---------------------------*/

int mcli_error_print(struct mcli_error* err, FILE* file) {
    switch (err->error_code) {
        case MCLI_ERR_NO_VALUE:
            return fprintf(file, "%s: no value found\n", err->value);
        case MCLI_ERR_SHORT_ARG_TOO_LONG:
            return fprintf(file, "%s: more than 1 characters are not allowed with only 1 '-'\n", err->value);
        case MCLI_ERR_ONLY_HYPHEN_MINUS:
            return fprintf(file, "error: argument only consists of '-'\n");
        case MCLI_ERR_UNKNOWN_OPTION:
            return fprintf(file, "%s: unknown option\n", err->value);
        case MCLI_ERR_VALUE_WITHOUT_OPTION:
            return fprintf(file, "%s: value without option\n", err->value);
        default:
            return 1;
    }
}

int mcli_errbuf_print(struct mcli_errbuf* errbuf, FILE* file) {
    int i, r;
    for (i = 0; i < errbuf->len; i++) {
        r = mcli_error_print(&errbuf->ptr[i], file);
        if (r) return r;
    }
    return 0;
}

void mcli_errbuf_push(struct mcli_errbuf* errbuf, struct mcli_error err) {
    struct argdef a = {0};
    long size = sizeof(a);
    if (!errbuf->ptr) {
        errbuf->ptr = (struct mcli_error*)malloc(1 * size);
        errbuf->ptr[0] = err;
        errbuf->len = 1;
    } else {
        errbuf->len += 1;
        errbuf->ptr = (struct mcli_error*)realloc(errbuf->ptr, errbuf->len * size);
        errbuf->ptr[errbuf->len-1] = err;
    }
}

void mcli_errbuf_free(struct mcli_errbuf* errbuf) {
    if (errbuf->ptr) free(errbuf->ptr);
}

struct argdef argdef_full(char short_name, char* long_name, int accepts_value) {
    struct argdef argdef = {0};
    argdef.hdr.short_name = short_name;
    argdef.hdr.long_name = long_name;
    argdef.hdr.accepts_value = accepts_value;
    return argdef;
}
struct argdef argdef_long(char* long_name, int accepts_value) {
    struct argdef argdef = {0};
    argdef.hdr.long_name = long_name;
    argdef.hdr.accepts_value = accepts_value;
    return argdef;
}

struct argdef argdef_short(char short_name, int accepts_value) {
    struct argdef argdef = {0};
    argdef.hdr.short_name = short_name;
    argdef.hdr.accepts_value = accepts_value;
    return argdef;
}

struct argdef argdef_value() {
    struct argdef argdef = {0};
    argdef.hdr.accepts_value = 1;
    return argdef;
}

/* 0 if it's not a flag
 * 1 if it's a short flag
 * 2 if it's a long flag
 * -1 if it only consists of '-'
 * -2 if it's something like '-param'
 * we assert that flag is not NULL
 */
static int _mcli_flag_type(char *flag) {
    if (flag[0] == '-') {
        return flag[1] == '-' ? (flag[2] == 0 ? -1 : 2) : (flag[1] != 0 ? (flag[2] == 0 ? 1 : -2) : -1);
    } else {
        return 0;
    }
}

struct mcli_errbuf parse_args(struct argdef* arg_array[], unsigned int len, unsigned int argv, char** argc) {
    struct mcli_errbuf errbuf = {0};
    struct mcli_error e;
    int i, j, found;
    /* we start from index 1, because index 0 is program name */
    for (i = 1; i < argv; i++) {
        switch (_mcli_flag_type(argc[i])) {
            case -2:
                e.error_code = MCLI_ERR_SHORT_ARG_TOO_LONG;
                e.value = argc[i];
                mcli_errbuf_push(&errbuf, e);
                break;
            case -1:
                e.error_code = MCLI_ERR_ONLY_HYPHEN_MINUS;
                e.value = NULL;
                mcli_errbuf_push(&errbuf, e);
                break;
            case 0:
                found = 0;
                for (j = 0; j < len; j++) {
                    if (!arg_array[j]->hdr.short_name &&
                        !arg_array[j]->hdr.long_name  &&
                        arg_array[j]->hdr.accepts_value &&
                        !arg_array[j]->value.found)
                    {
                        arg_array[j]->value.str = argc[i];
                        found = 1;
                    }
                }
                if (!found) {
                    e.error_code = MCLI_ERR_VALUE_WITHOUT_OPTION;
                    e.value = argc[i];
                    mcli_errbuf_push(&errbuf, e);
                }
                break;
            case 1:
                found = 0;
                for (j = 0; j < len; j++) {
                    if (arg_array[j]->hdr.short_name == argc[i][1]) {
                        found = 1;
                        if (arg_array[j]->hdr.accepts_value) {
                            if (++i < argv) {
                                arg_array[j]->value.str = argc[i];
                            } else {
                                e.error_code = MCLI_ERR_NO_VALUE;
                                e.value = argc[i - 1];
                                mcli_errbuf_push(&errbuf, e);
                            }
                        } else {
                            arg_array[j]->value.found++;
                        }
                        break;
                    }
                }
                if (!found) {
                    e.error_code = MCLI_ERR_UNKNOWN_OPTION;
                    e.value = argc[i];
                    mcli_errbuf_push(&errbuf, e);
                }
                break;
            case 2:
                found = 0;
                for (j = 0; j < len; j++) {
                    if (arg_array[j]->hdr.long_name && strcmp(arg_array[j]->hdr.long_name, argc[i] + 2) == 0) {
                        found = 1;
                        if (arg_array[j]->hdr.accepts_value) {
                            if (++i < argv) {
                                arg_array[j]->value.str = argc[i];
                            } else {
                                e.error_code = MCLI_ERR_NO_VALUE;
                                e.value = argc[i - 1];
                                mcli_errbuf_push(&errbuf, e);
                            }
                        } else {
                            arg_array[j]->value.found++;
                        }
                        break;
                    }
                }
                if (!found) {
                    e.error_code = MCLI_ERR_UNKNOWN_OPTION;
                    e.value = argc[i];
                    mcli_errbuf_push(&errbuf, e);
                }
                break;
        }
    }
    return errbuf;
}

#endif
