*void malloc(uint64 size);
int puts(*char string);
*void memcpy(*void dest, *void src, uint64 n);
*char strcpy(*char dest, *char src);
int printf(*char format, ...);
uint64 strlen(*char string);
int putchar(int ch);

int main() {
    *char string = "ABCD";

    char output = *(string + (3-1));

    putchar(output);
    putchar(10);

    return 0;
}
