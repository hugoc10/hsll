#include <ctype.h>
#include <dlfcn.h>
#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TYPES */
// ffi_type_void;
// ffi_type_uint8;
// ffi_type_sint8;
// ffi_type_uint16;
// ffi_type_sint16;
// ffi_type_uint32;
// ffi_type_sint32;
// ffi_type_uint64;
// ffi_type_sint64;
// ffi_type_float;
// ffi_type_double;
// ffi_type_pointer;
// ffi_type_longdouble;

#define MAX_ARGS 16

typedef struct {
        char *func_name;
        ffi_cif cif;
        ffi_type *arg_types[MAX_ARGS];
        void *arg_values[MAX_ARGS];
        size_t arg_value_size;
        size_t arg_type_size;
        ffi_status status;
        ffi_type *return_type;
        ffi_arg result;
} Call;

void
call_free(Call call)
{
        // if (call.arg_types) free(call.arg_types);
        call.arg_value_size = 0;
        // if (call.arg_values) free(call.arg_values);
        call.arg_value_size = 0;
        if (call.func_name) free(call.func_name);
}

void
call_add_arg_type(Call *call, ffi_type *type)
{
        call->arg_types[call->arg_type_size] = type;
        ++call->arg_type_size;
}

void
call_add_arg_value(Call *call, void *value)
{
        call->arg_values[call->arg_value_size] = value;
        ++call->arg_value_size;
}

void
ltrim(char **c)
{
        while (isspace(**c))
                ++*c;
}

char *
getid(char **c)
{
        char *start = *c;
        if (!isalpha(**c) && **c != '_') return NULL;
        ++*c;
        while (isalnum(**c) || **c == '_') {
                ++*c;
        }
        char tmp = **c;
        **c = 0;
        start = strdup(start);
        **c = tmp;
        return start;
}

int
consume(char **s, char c)
{
        if (**s == c) {
                ++*s;
                return 1;
        }
        return 0;
}

int
getint(void **value, char **s)
{
        char *start = *s;
        long n = 0;
        long m = 1;

        if (consume(s, '-')) m = -1;
        if (consume(s, '+')) m = 1;

        if (!isdigit(**s)) {
                *s = start;
                return 1;
        }

        while (isdigit(**s)) {
                n *= 10;
                n += **s - '0';
                ++*s;
        }

        if (isalpha(**s) || **s == '.') {
                *s = start;
                return 1;
        }

        n *= m;
        *value = memcpy(malloc(sizeof n), &n, sizeof n);
        return 0;
}

int
getfloat(void **value, char **s)
{
        char *start = *s;
        double n = 0;
        double d = 1;
        double m = 1;

        if (consume(s, '-')) m = -1;
        if (consume(s, '+')) m = 1;

        if (!isdigit(**s)) {
                *s = start;
                return 1;
        }

        while (isdigit(**s)) {
                n *= 10;
                n += **s - '0';
                ++*s;
        }

        if (!consume(s, '.')) {
                *s = start;
                return 1;
        }

        while (isdigit(**s)) {
                d *= 10;
                n += (**s - '0') / d;
                ++*s;
        }

        *value = memcpy(malloc(sizeof n), &n, sizeof n);
        return 0;
}

int
getstring(void **value, char **s)
{
        char *start = *s;
        if (!consume(s, '"')) {
                *s = start;
                return 1;
        }

        while (!consume(s, '"')) {
                if (!**s) {
                        *s = start;
                        return 1;
                }
                ++*s;
        }

        (*s)[-1] = 0;
        *value = strdup(start + 1);
        *value = memcpy(malloc(sizeof *value), &*value, sizeof *value);
        (*s)[-1] = '"';
        return 0;
}

int
getarg(Call *call, char **line)
{
        void *value;
        if (0)
                ;
        else if (!getint(&value, line)) {
                call_add_arg_value(call, value);
                call_add_arg_type(call, &ffi_type_sint);
                printf("INT ARG: %ld\n", *(long *) value);
        }

        else if (!getfloat(&value, line)) {
                call_add_arg_value(call, value);
                call_add_arg_type(call, &ffi_type_float);
                printf("DOUBLE ARG: %lf\n", *(double *) value);
        }

        else if (!getstring(&value, line)) {
                call_add_arg_value(call, value);
                call_add_arg_type(call, &ffi_type_float);
                printf("STR ARG: %s\n", *(char **) value);
        }

        else {
                printf("Can not parse arg\n");
                return 1;
        }
        return 0;
}

int
parse_function_call(Call *call, char *line)
{
        /* Parse the body as: _ ID _ "(" _ param? [_ "," _ param]* ")" _ ";"
         * where _ is zero or more spaces and param a value that can be parsed
         * using getarg(). ID is the name of the function. */
        ltrim(&line);
        call->func_name = getid(&line);
        ltrim(&line);
        if (!consume(&line, '(')) {
                printf("Invalid function call");
                return 1;
        }
        ltrim(&line);
        while (!consume(&line, ')')) {
                if (getarg(call, &line)) {
                        printf("Can not get arg\n");
                        return 1;
                }
                ltrim(&line);
                if (!consume(&line, ',')) {
                        if (consume(&line, ')')) {
                                break;
                        }
                        printf("Can not get ')'. Invalid function args at %s\n", line);
                        return 1;
                }
                ltrim(&line);
        }
        ltrim(&line);

        /* The 0 is to return success if no semicolon was found */
        return !consume(&line, ';') && 0;
}

int
parse_and_run(char *line)
{
        Call call = { 0 };

        /* No yet handled */
        call.return_type = &ffi_type_sint;

        if (parse_function_call(&call, line)) {
                printf("Error: parse_function_arg_values: "
                       "Invalid line: %s\n",
                       line);
                return 1;
        }

        // Prepare the ffi_cif structure.
        if (ffi_prep_cif(&call.cif,
                         FFI_DEFAULT_ABI,
                         call.arg_value_size,
                         call.return_type,
                         call.arg_types) != FFI_OK) {
                perror("Error: ffi_prep_cif");
                return 1;
        }

        // Invoke the function.
        void *func = dlsym(RTLD_DEFAULT, call.func_name);
        if (!func) {
                printf("Could not find function name `%s`\n", call.func_name);
                return 1;
        }

        ffi_call(&call.cif, func, &call.result, call.arg_values);
        printf("-> %d\n", (int) call.result);

        return 0;
}

void
prompt()
{
        printf(">> ");
        fflush(stdout);
}

int
main(int argc, const char **argv)
{
        char line[1024];
        printf("Wellcome to hsll\n");
        printf("  You can execute C functions\n");
        printf("  The accepted argument types are:\n");
        printf("  - const int\n");
        printf("  - const float\n");
        printf("  - const string\n");
        printf("  The accepted return type is:\n");
        printf("  - const int\n");
        printf("\n");

        while (1) {
                prompt();
                if (!fgets(line, sizeof line, stdin)) break;
                parse_and_run(line);
        }

        return 0;
}
