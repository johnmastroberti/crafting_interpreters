#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

struct String {
    char* data;
    size_t size;
};


struct List {
    struct List* prev;
    struct List* next;
    struct String value;
};

void string_alloc(struct String* str, const char* data, size_t size) {
    if (data) {
        size = strlen(data) + 1;
    }

    if (!size) return;

    str->data = malloc(size);
    if (str->data && data)
        memcpy(str->data, data, size);
}

void string_free(struct String *s) {
    if (s->data)
        free(s->data);
    s->data = NULL;
}

bool string_compare(struct String s1, struct String s2) {
    if (s1.size != s2.size) return false;
    return !strcmp(s1.data, s2.data);
}

void list_insert_before(struct List* node, struct String s) {
    struct List* new_node = malloc(sizeof(struct List));
    new_node->prev = node->prev;
    new_node->next = node;
    new_node->value = s;
    node->prev = new_node;
}

void list_insert_after(struct List* node, struct String s) {
    struct List* new_node = malloc(sizeof(struct List));
    new_node->prev = node;
    new_node->next = node->next;
    new_node->value = s;
    node->next = new_node;
}

struct List* list_find(struct List* list, struct String s) {
    if (!list) return NULL;
    if (string_compare(list->value, s)) return list;
    return list_find(list->next, s);
}

void list_remove(struct List* node) {
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    string_free(&node->value);
}

void print_list(struct List* list) {
    if (!list) {
        puts("<end of list>");
        return;
    }
    printf("%s\n", list->value.data);
    print_list(list->next);
}

int main() {
    struct List* list = malloc(sizeof(struct List));
    list->prev = NULL;
    list->next = NULL;
    string_alloc(&list->value, "string 1", 0);
    print_list(list);

    struct String s2, s3, s4;
    string_alloc(&s2, "string 2", 0);
    string_alloc(&s3, "string 3", 0);
    string_alloc(&s4, "string 4", 0);

    list_insert_after(list, s2);
    struct List* node = list->next;
    list_insert_after(node, s3);
    node = node->next;
    list_insert_after(node, s4);

    print_list(list);

    struct List* node3 = list_find(list, s3);
    if (node3 == list->next->next)
        puts("Found the correct node");
    else
        puts("Did not find the correct node");

    list_remove(node3);

    print_list(list);

}


