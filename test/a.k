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

void read_line_into(*char into) {
    scanf(" %[^\n]", into);
}

void string_copy(*char dst, *char src) {
    while (*src) *dst++ = *src++;
}

void string_concat(*char dst, *char src) {
    while (*dst) ++dst;
    while (*src) *dst++ = *src++;
}

uint64 string_length(*char str) {
    uint64 result = 0;

    while (*(str + result))
        result++;

    return result;
}

int main(int argc, **char argv) {
    int a, b;

    if (argc != 3) {
        printf("Usage: %s <a> <b>\n", argv[0]);
        return 1;
    }

    a = atoi(argv[1]);
    b = atoi(argv[2]);

    if (a || b) {
        puts("Either a or b is nonzero.");
    } else {
        puts("Both a and b are zero.");
    }

    *int ptr = &a;

    printf("I got my value as %d\n", *ptr);

    return 0;
}
