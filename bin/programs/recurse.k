#include "cstdlib.k"

int fibonacci(int n) {
    if n <= 1  return n;

    return fibonacci(n - 1) + fibonacci(n - 2);
}

int factorial(int n) {
    if n <= 1  return n;
    return n * factorial(n - 1);
}

int main() {
    int n;

    printf("n = ");
    scanf("%d", &n);

    printf("fibonacci(%d) = %d\n", n, fibonacci(n));
    printf("factorial(%d) = %d\n", n, factorial(n));

    return 0;
}
