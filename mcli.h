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
        char* value;
    };
};

enum mcli_error_type {
    /* for example: `program --param` where `struct argdef param = argdef_long("param", 1);`*/
    MCLI_ERR_NO_VALUE,
    /* for example: `program -param` (we don't support flags like this) */
    MCLI_ERR_SHORT_ARG_TOO_LONG,
    /* for example: `program -` */
    MCLI_ERR_ONLY_HYPHEN_MINUS,
};

struct mcli_error {
    enum mcli_error_type error_code;
    /*
     * for MCLI_ERR_NO_VALUE:           it's argument's name
     * for MCLI_ERR_SHORT_ARG_TOO_LONG: it's argument's name
     * for MCLI_ERR_ONLY_HYPHEN_MINUS:  it's NULL
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

static int mcli_error_print(FILE* file, struct mcli_error* err) {
    switch (err->error_code) {
        case MCLI_ERR_NO_VALUE:
            return fprintf(file, "%s: no value found\n", err->value);
        case MCLI_ERR_SHORT_ARG_TOO_LONG:
            return fprintf(file, "%s: more than 1 characters are not allowed with only 1 '-'\n", err->value);
        case MCLI_ERR_ONLY_HYPHEN_MINUS:
            return fprintf(file, "ERR: argument only consists of '-'\n");
        default:
            return 1;
    }
}

static int mcli_errbuf_print(FILE* file, struct mcli_errbuf* errbuf) {
    int i, r;
    for (i = 0; i < errbuf->len; i++) {
        r = mcli_error_print(file, &errbuf->ptr[i]);
        if (r) return r;
    }
    return 0;
}

static void mcli_errbuf_push(struct mcli_errbuf* errbuf, struct mcli_error err) {
    if (!errbuf->ptr) {
        errbuf->ptr = malloc(1 * sizeof((struct argdef){0}));
        errbuf->ptr[0] = err;
        errbuf->len = 1;
    } else {
        errbuf->len += 1;
        errbuf->ptr = realloc(errbuf->ptr, errbuf->len);
        errbuf->ptr[errbuf->len-1] = err;
    }
}

static void mcli_errbuf_free(struct mcli_errbuf* errbuf) {
    if (errbuf->ptr) free(errbuf->ptr);
}

static struct argdef argdef_full(char short_name, char* long_name, int accepts_value) {
    struct argdef argdef = {0};
    argdef.hdr.short_name = short_name;
    argdef.hdr.long_name = long_name;
    argdef.hdr.accepts_value = accepts_value;
    return argdef;
}
static struct argdef argdef_long(char* long_name, int accepts_value) {
    struct argdef argdef = {0};
    argdef.hdr.long_name = long_name;
    argdef.hdr.accepts_value = accepts_value;
    return argdef;
}

static struct argdef argdef_short(char short_name, int accepts_value) {
    struct argdef argdef = {0};
    argdef.hdr.short_name = short_name;
    argdef.hdr.accepts_value = accepts_value;
    return argdef;
}

static struct argdef argdef_value() {
    struct argdef argdef = {0};
    argdef.hdr.accepts_value = 1;
    return argdef;
}

/* 0 if it's not a flag
 * 1 if it's a short flag
 * 2 if it's a long flag
 * we assert that flag is not NULL
 */
static int _mcli_flag_type(const char const* flag) {
    if (flag[0] == '-') {
        return flag[1] == '-' ? 2 : (flag[1] == 0 ? 0 : 1);
    } else {
        return 0;
    }
}

static struct mcli_errbuf parse_args(struct argdef* arg_array[], unsigned int len, unsigned int argv, char** argc) {
    struct mcli_errbuf errbuf = {0};
    int i, j;
    char c0;
    /* we start from index 1, because index 0 is program name */
    for (i = 1; i < argv; i++) {
        c0 = argc[i][0];
        if (c0 == '-') {
            c0 = argc[i][1];
            /* ERROR: only '-' in argument */
            if (c0 == 0) {
                mcli_errbuf_push(&errbuf, (struct mcli_error){MCLI_ERR_ONLY_HYPHEN_MINUS, 0});
            }
            /* Long Flag */
            else if (c0 == '-') {
                for (j = 0; j < len; j++) {
                    if (!arg_array[j]->hdr.long_name) continue;
                    if (strcmp(arg_array[j]->hdr.long_name, argc[i] + 2) == 0) {
                        if (arg_array[j]->hdr.accepts_value) {
                            if (++i < argv) {
                                arg_array[j]->value = argc[i];
                            } else {
                                /* ERROR: expected value, but arguments ended 
                                 * Situations like: `program --param ` where `struct argdef param = argdef_long("param", 1);`*/
                                mcli_errbuf_push(&errbuf, (struct mcli_error){MCLI_ERR_NO_VALUE, argc[i - 1]});
                            }
                        } else {
                            arg_array[j]->found++;
                        }
                        break;
                    }
                }
            }
            /* Short Flag */
            else {
                if (argc[i][2] != 0) {
                    /* Situations like: `program -param` (we don't support arguments like this)*/
                    mcli_errbuf_push(&errbuf, (struct mcli_error){MCLI_ERR_SHORT_ARG_TOO_LONG});
                    continue;
                }
                for (j = 0; j < len; j++) {
                    if (arg_array[j]->hdr.short_name != 0 && arg_array[j]->hdr.short_name == c0) {
                        if (arg_array[j]->hdr.accepts_value) {
                            if (++i < argv) {
                                arg_array[j]->value = argc[i];
                            } else {
                                /* ERROR: expected value, but arguments ended 
                                 * Situations like: `program -o ` where `struct argdef output = argdef_short('o', 1);`*/
                                mcli_errbuf_push(&errbuf, (struct mcli_error){MCLI_ERR_NO_VALUE, argc[i - 1]});
                            }
                        } else {
                            arg_array[j]->found++;
                        }
                        break;
                    }
                }
            }
        } else {
            for (j = 0; j < len; j++) {
                if (!arg_array[j]->hdr.short_name && 
                    !arg_array[j]->hdr.long_name &&
                     arg_array[j]->hdr.accepts_value && 
                    !arg_array[j]->value) 
                {
                    arg_array[j]->value = argc[i];
                    break;
                }
            }
        }
    }
    return errbuf;
}

#endif
