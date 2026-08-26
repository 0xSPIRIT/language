#include "cstdlib.k"

struct Vector3 {
    int x, y, z;
};

Vector3 test;

void print_vec(*Vector3 v) {
    printf("The pointer is %p and the value is (%d %d %d)\n", v, v.x, v.y, v.z);
}

int main() {
    test.x = 1;
    test.y = 2;
    test.z = 3;

    test.x += test.z;
    test.x += test.y;

    if (test.x < 5) {
        printf("You're a wizard, Harry!\n");
    } else {
        printf("You're a fucking muggle, Harry!\n");
    }

    print_vec(&test);

    return 0;
}
