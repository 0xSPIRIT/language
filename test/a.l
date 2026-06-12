void test() {
    [5]int x;
    x[w*y+z] = 1;
}

struct Entity {
    int a;
    string b;
    *char cstr;
    *Entity left, right;
};

*Entity make_entity() {
    *Entity e = new(Entity);

    e.a = 1;
    e.b = "Test";
    e.c = "C String";
    e.left = new(Entity);
    e.right = null;

    return e;
}

int main() {
    *Entity e = make_entity();

    print(e.a);
    print(e.b);
    print(e.c);
    print(e.left, e.right);
}
