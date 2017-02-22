#include "stdio.h"
#include "src/list.h"


void printList(List *l){
    for(Node* n = l->back; n != NULL; n = n->next_node){
        printf("%d ;", *(int*)n->data);
        printf("{%p , %p}\n", n->prev_node, n->next_node);
    }
    printf("\n");

}


int main(){

    List l;
    init_list(&l);
    int *x1 = malloc(sizeof(int)); *x1 = 1;
    int *x2 = malloc(sizeof(int)); *x2 = 2;
    int *x3 = malloc(sizeof(int)); *x3 = 3;
    int *x4 = malloc(sizeof(int)); *x4 = 4;
    int *x5 = malloc(sizeof(int)); *x5 = 5;
    add_to_front(&l,x1);
    add_to_front(&l,x2);
    add_to_front(&l,x3);
    add_to_front(&l,x4);
    add_to_front(&l,x5);
    printList(&l);

    remove_node(&l, (&l)->back->next_node->next_node, &free);
    printList(&l);
    
    /*do{
        printList(&l);
        remove_from_back(&l,&free);
        
    }while(!is_empty(&l));
    */
    
    //printList(&l);
    //remove_node(NULL);
    destroy_list(&l, &free);
    

    return 0;
}