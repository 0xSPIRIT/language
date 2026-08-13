*void malloc(uint64 size);
int puts(*char string);
*void memcpy(*void dest, *void src, uint64 n);
*char strcpy(*char dest, *char src);

void clrstr(*char string) *string = 0;

int main() {
    *char Test = malloc(100);

    strcpy(Test, "This is a first one!");
    puts(Test);

    *char B = Test;

    puts(B);

    strcpy(Test, "What");

    puts(B);

    return 67;
}
