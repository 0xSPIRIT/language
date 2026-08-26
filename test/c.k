#include "cstdlib.k"

struct Vector3 {
    int x, y, z;
};

Vector3 test;

void print_vec(*Vector3 v) {
    printf("(%d %d %d)\n", v.x, v.y, v.z);
}

int main() {
    test.x = 1;
    test.y = 2;
    test.z = 3;

    print_vec(&test);

    return 0;
}
