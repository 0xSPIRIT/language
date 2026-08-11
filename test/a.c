int puts(*char string);

struct Entity {
    *char String1;
};

void setup(*Entity test) {
}

int main() {
    Entity test;

    setup(&test);
    puts(test.String1);

    return 0;
}
