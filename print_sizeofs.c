#include <stdio.h>

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  printf("#define SIZEOF_CHAR_P %d\n", (int)sizeof(char*));
  printf("#define SIZEOF_INT %d\n", (int)sizeof(int));
  printf("#define SIZEOF_LONG %d\n", (int)sizeof(long));
  printf("#define SIZEOF_LONG_LONG %d\n", (int)sizeof(long long));
  printf("#define SIZEOF_SHORT %d\n", (int)sizeof(short));
  printf("#define SIZEOF_VOID_P %d\n", (int)sizeof(void*));
  return 0;
}
