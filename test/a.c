int puts(*char s);

int main() {
    *char string = "Hello everybody my name is Markiplier!";

    for (int i = 0; i < 10; i++) {
        puts(string + i);
    }

    return 67;
}
