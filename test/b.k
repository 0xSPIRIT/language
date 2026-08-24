#include "cstdlib.k"

void nop() {}

void read_entire_file(*char filepath) {
    *FILE fp = fopen(filepath, "r");

    if (fp == 0) {
        printf("Couldn't open the file.");
    }

    fseek(fp, 0, 2); // SEEK_SET = 2

    uint64 size = ftell(fp);

    rewind(fp);

    *char buffer = malloc(size + 1);

    uint64 count = fread(buffer, 1, size, fp);

    fclose(fp);

    puts(buffer);
}

void linked_list() {
    struct ListNode {
        int data;
        *ListNode next;
    };

    *ListNode head = malloc(sizeof(ListNode));

    head.data = 5;
    head.next = malloc(sizeof(ListNode));
    head.next.data = 10;
    head.next.next = 0;

    printf("%p\n", head);
    printf("%d\n", head.data);
    printf("%p\n", head.next);
    printf("%d\n", head.next.data);
}

int main(int argc, **char argv) {
    if argc != 2 {
        printf("Usage: %s filename\n", argv[0]);
        return 0;
    }

    read_entire_file(argv[1]);
    linked_list();

    return 67;
}

