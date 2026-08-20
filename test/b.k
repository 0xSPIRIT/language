#include "cstdlib.k"

*void malloc(uint64 size);
void free(*void ptr);
int puts(*char string);
*void memcpy(*void dest, *void src, uint64 n);
*char strcpy(*char dest, *char src);
int printf(*char format, ...);
int scanf(*char format, ...);
uint64 strlen(*char string);
int putchar(int ch);
int atoi(*char str);

void nop() {}

int main(int argc, **char argv) {
    int params = argc - 1;
    *int a = malloc(params * sizeof(*a));

    printf("We got %d arguments!\n", params);

    for (int i = 1; i < argc; i++) {
        a[i - 1] = atoi(argv[i]);
    }

    for (int i = 0; i < params; i++)
        printf("%d ", a[i]);

    return 67;
}
