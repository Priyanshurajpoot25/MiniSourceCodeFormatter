#include <stdio.h>
#include <stdlib.h>
#define MAX_ITEMS 5

// Multi-feature C code testing loops, pointers, and structures
struct Item {
    int id;
    char name[32];
};

void process_item(struct Item *item) {
    if (item->id > 0) {
        printf("Item: %d\n", item->id);
    } else {
        printf("Invalid\n");
    }
}

int main() {
    int values[MAX_ITEMS] = { 10, 20, 30, 40, 50 };
    int total = 0;

    for (int i = 0; i < MAX_ITEMS; i++) {
        total += values[i];
    }
    /* Display results */
    printf("Total sum: %d\n", total);
    char msg[] = "Braces {inside} string; should not break!";
    printf("%s\n", msg);

    return 0;
}
