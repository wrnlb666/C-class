#include "src/ffiutil.h"


void foo(int a, int b, char* s) {
    printf("%d, %d, %s\n", a, b, s);
}

void foo_wrapper(ffi_cif* cif, void* ret, void** args, void* userdata) {
    (void) cif;
    (void) ret;

    int a = *(int*)args[0];
    int b = *(int*)args[1];
    char* s = userdata;

    foo(a, b, s);
}

int main(void) {
    ffi_cif* cif = ffiutil_parse_cif("\
{\
    \"atypes\": [\"int\", \"i32\"],\
    \"rtype\": \"void\",\
}");
    printf("cif ptr: %p\n", cif);

    char* str = "Hello World";

    ffi_closure* closure;
    void* code;
    closure = ffi_closure_alloc(sizeof (ffi_closure), &code);
    ffi_prep_closure_loc(closure, cif, foo_wrapper, str, code);

    void (*bar)(int, int) = code;
    bar(420, 69);

    ffi_closure_free(code);
    ffiutil_free_cif(cif);
}
