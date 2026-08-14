*void malloc(uint64 size);
int puts(*char string);
*void memcpy(*void dest, *void src, uint64 n);
*char strcpy(*char dest, *char src);
int printf(*char format, ...);

void clrstr(*char str) *str = 0;

void string_copy(*char dst, *char src, int count) {
    for (int i = 0; i < count; i++) {
        *(dst + i) = *(src + i);
    }

    *(dst + count) = 0;
}

int main() {
    *char test = "Hello there";
    *char temp = malloc(100);

    string_copy(temp, test, 11);

    puts(test);
    puts(temp);

    return 0;
}
