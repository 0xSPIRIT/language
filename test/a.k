*void malloc(uint64 size);
int puts(*char string);
*void memcpy(*void dest, *void src, uint64 n);
*char strcpy(*char dest, *char src);
int printf(*char format, ...);

int main() {
    // Issue; The stack isn't 16-aligned before function calls.
    for (int i = 0; i < 10; i++) {
        printf("%d ", i);
    }

    return 0;
}
