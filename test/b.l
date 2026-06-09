int empty() {
}

int only_decl() {
    int a;
}

int only_call() {
    foo();
}

int only_assign() {
    a = 5;
}

int only_return() {
    return x;
}

int lots_of_params(
    int a,
    int b,
    int c,
    int d,
    int e
) {
    return a;
}
