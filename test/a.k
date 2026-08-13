*void malloc(int size);
int puts(*char string);
*void memcpy(*void dest, *void src, uint64 n);
*char strcpy(*char dest, *char src);

int main() {
    *char String = "Hello there matey!";

    for (int i = 0; i < 10; i++) {
        puts(String + i);
    }

    return 67;
}
