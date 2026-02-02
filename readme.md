<div align=center>
    <h1>mcli</h1>
</div>

## about

`mcli` is a single-header library for argument parsing written in C89.

## example

```c
#include "mcli.h"
#include <stdio.h>

int main(int argv, char **argc) {
    struct argdef output = argdef_short('o', 1);
    struct argdef input = argdef_value();
    struct argdef help = argdef_full('h', "help", 0);
    struct argdef version = argdef_long("version", 0);
    struct argdef* argtable[4] = { &output, &input, &help, &version };
    struct mcli_errbuf errbuf = parse_args(argtable, 4, argv, argc);
    if (errbuf.len) {
        mcli_errbuf_print(stderr, &errbuf);
        return 1;
    }
    /* value is a char*, while found is an int (how many times it was found in the provided arguments) */
    if (output.value != 0)  printf("OUTPUT: %s\n", output.value);
    if (input.value != 0)   printf("INPUT: %s\n", input.value);
    if (help.found != 0)    printf("HELP: %u\n", help.found);
    if (version.found != 0) printf("VERSION: %u\n", version.found);
    mcli_errbuf_free(&errbuf);
    return 0;
}
```

## credits

`mcli` was brought to you by matyz,

licensed under MIT
