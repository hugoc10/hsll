#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>

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

typedef struct {
        char *func_name;
        ffi_cif cif;
        ffi_type **arg_types;
        void **arg_values;
        size_t arg_size;
        ffi_status status;
        ffi_type *return_type;
        ffi_arg result;
} Call;

void
call_free(Call call)
{
        if (call.arg_types) free(call.arg_types);
        call.arg_size = 0;
        if (call.arg_values) free(call.arg_values);
        call.arg_size = 0;
        if (call.func_name) free(call.func_name);
}

void
call_add_arg_type(Call call, ffi_type *type)
{
        call.arg_types[call.arg_size] = type;
        ++call.arg_size;
}

void
call_add_arg_value(Call call, void *type)
{
        call.arg_values[call.arg_size] = type;
        ++call.arg_size;
}

int
parse_and_run(char *line)
{
        Call call = { 0 };
        if (parse_function_call(&call, line)) {
                printf("Error: parse_function_arg_values: "
                       "Invalid line: %s\n",
                       line);
                return 1;
        }

        // Prepare the ffi_cif structure.
        if (ffi_prep_cif(&call.cif,
                         FFI_DEFAULT_ABI,
                         call.arg_size,
                         call.return_type,
                         call.arg_types) != FFI_OK) {
                perror("Error: ffi_prep_cif");
                return 1;
        }

        // Invoke the function.
        ffi_call(&cif, FFI_FN(foo), &result, arg_values);

        // The ffi_arg 'result' now contains the unsigned char returned from foo(),
        // which can be accessed by a typecast.
}

int
main(int argc, const char **argv)
{
        parse_and_run();

        return 0;
}
