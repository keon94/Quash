#ifndef SRC_LIST_H
#define SRC_LIST_H

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

typedef struct Node{
    void* data;
    struct Node *next_node;
    struct Node *prev_node;
} Node;


typedef struct List{
    Node* front;
    Node* back;
    int size;
} List;

static Node* create_node(void* data, Node* prev_node, Node* next_node){
    Node* n = malloc(sizeof(Node));
    n->data = data;
    n->prev_node = prev_node;
    n->next_node = next_node;
    return n;
}

//initializes the list - must always be called before anything else!
void init_list(List* l){
    assert(l != NULL);
    l->front = l->back = NULL;
    l->size = 0;
}

bool is_empty(List* l){
    assert(l != NULL);
    return l->size == 0;
}

// adds a node with data to the front of the list
void add_to_front(List* l, void* data){
    assert(l != NULL);
    if(is_empty(l)){
        l->front =  l->back = create_node(data, NULL, NULL);
        l->size = 1;
    }
    else{
        Node *new_front = create_node(data, l->front, NULL);
        l->front->next_node = new_front;
        l->front = new_front;
        l->size++;
    }
}

//removes node n, and links its previous Node to its next. If no destructor is specified it returns the data, otherwise 
//the data is freed and it returns NULL
void* remove_node(List* l, Node* n, void(*destructor)(void*)){
    assert(l != NULL);
    assert(n != NULL);
    assert(!is_empty(l));
    void* n_data = n->data;
    if(destructor){
        destructor(n->data);
        n_data = NULL;
    }
    if(n->prev_node)
        n->prev_node->next_node = n->next_node;
    else
        l->back = n->next_node;  
    if(n->next_node)
        n->next_node->prev_node = n->prev_node;
    else
        l->front = n->prev_node;
    free(n);
    l->size--;
    return n_data;
}


void* peek(Node* n){
    assert(n != NULL);
    return n->data;
}

void* peek_back(List* l){
    assert(l != NULL);
    assert(!is_empty(l));
    return peek(l->back);
}

void* peek_front(List* l){
    assert(l != NULL);
    assert(!is_empty(l));
    return peek(l->front);
}

//removes the rearmost node using the provided destructor function pointer and returns its associated data
void* remove_from_back(List* l, void(*destructor)(void*)){
    return remove_node(l, l->back, destructor);
}


void destroy_list(List *l, void(*destructor)(void*)){
    assert(l != NULL);
    while(!is_empty(l))
        remove_from_back(l, destructor);
}


//inserts a new Node with data between n and n->next - Not needed for the quash project
/*
void insert_node(Node* n, void* data){
}
*/


#endif
