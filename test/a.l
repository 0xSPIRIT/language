int main() {
    return 0;
}

int return_literal() {
    return 123;
}

char return_char() {
    return 'x';
}

string return_string() {
    return "hello";
}

int parameters(int a, char b, string c) {
    return a;
}

int declarations() {
    int a;
    char b;
    string c;
    return 0;
}

int declarations_with_init() {
    int a = 5;
    char b = 'z';
    string c = "test";
    return a;
}

int assignments() {
    int a;
    int b;

    a = 5;
    b = 10;

    a = b;
    b = a;

    return a;
}

int function_calls() {
    print();
    print();
    return 0;
}

int function_calls_with_args() {
    foo(1);
    foo(2);
    foo(3);

    bar(a);
    bar("hello");
    bar('c');

    return 0;
}

int mixed() {
    int a = 5;
    int b;

    b = a;

    print(a);
    print(b);

    return b;
}

int many_statements() {
    int a;
    int b;
    int c;

    a = 1;
    b = 2;
    c = 3;

    foo(a);
    foo(b);
    foo(c);

    return c;
}
