int main() {
    int a = 5;
    *int b = malloc(sizeof(int));
    *(++b) = 0;
    free(b);
    b = &a;
}
