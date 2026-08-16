*void malloc(uint64 size);
int puts(*char string);
*void memcpy(*void dest, *void src, uint64 n);
*char strcpy(*char dest, *char src);
int printf(*char format, ...);
uint64 strlen(*char string);
int putchar(int ch);

void nop() {}

int main() {
    *char source = "ABCD";
    *char dst = malloc(50);
    strcpy(dst, source);
    nop();
    *dst = *(source+1); // The issue is that RAX is getting clobbered and it isn't using a scratch register.
    nop();
    puts(dst);

    return 0;
}
