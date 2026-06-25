struct Test {
    int16 a;
};

struct Entity {
    Test t;
};

int main() {
    Entity b;

    b.t.a = 0;
}
