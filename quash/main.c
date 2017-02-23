#include "stdio.h"
#include "src/list.h"



void printList(List *l){
    for(Node* n = l->back;; n = n->next_node){
        printf("%d ;", *(int*)n->data);
        printf("{%p , %p}\n", n->prev_node, n->next_node);
        if(n == l->front) break;
    }
    printf("\n");

}



int main(){

    List l;
    init_list(&l);
    int x1 = 1, x2 = 2, x3 = 3, x4 = 4;
    
    add_to_front(&l,&x1);
    add_to_front(&l,&x2);
    add_to_front(&l,&x3);
    add_to_front(&l,&x4);
    printList(&l);  
    //foo(&l);
    //printList(&l);

    
    /*do{
        printList(&l);
        remove_from_back(&l,&free);
        
    }while(!is_empty(&l));
    */
    
    //printList(&l);
    //remove_node(NULL);
    destroy_list(&l, NULL);
    

    return 0;
}