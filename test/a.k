*void malloc(uint64 size);
int puts(*char string);
*void memcpy(*void dest, *void src, uint64 n);
*char strcpy(*char dest, *char src);
int printf(*char format, ...);
uint64 strlen(*char string);
int putchar(int ch);

void string_copy(*char dst, *char src) {
    while (*src) {
        *dst = *src;
        src++;
        dst++;
    }
    *dst = 0;
}

int main() {
    *char source = "ABCD";
    *char dest = malloc(100);

    string_copy(dest, source);

    puts(source);
    puts(dest);

    return 0;
}
