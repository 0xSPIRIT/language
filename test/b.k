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

struct Test {
    int a, b, c;
    *char foo;
    [5][3]int grid;
};

int main(int argc, **char argv) {
    return 67;
}

