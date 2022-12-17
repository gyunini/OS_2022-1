//
//  list.c
//  pa2
//
//  Created by 이균 on 2022/05/28.
//

#include <stdio.h>
typedef unsigned char bool;
#include "list_head.h"


LIST_HEAD(tcbs);

struct tcb {
    struct list_head list;
    int num;
};

int main(){
    struct tcb *temp, *temp1;
    struct tcb a, b, c;
    a.num = -1;
    b.num = 0;
    c.num = 1;
    
    list_add(&a.list, &tcbs);
    list_add(&b.list, &tcbs);
    list_add(&c.list, &tcbs);
    
    list_for_each_entry(temp, &tcbs, list) {
        printf("%d ", temp->num);
        //temp = list_first_entry(&tcbs, struct tcb, list);
    }
    printf("\n");
    list_for_each_entry_safe(temp, temp1, &tcbs, list) {
        printf("%d ", temp->num);
        if(temp->num == 0 || temp->num == 1)
        {
            list_del(&temp->list);
            temp = list_first_entry(&tcbs, struct tcb, list);
        }
    }
    list_for_each_entry(temp, &tcbs, list) {
        printf("%d ", temp->num);
    }
    return 0;
}



struct tcb *rr_scheduling(struct tcb *next) {

    /* TODO: You have to implement this function. */
    struct tcb* temp;

    if(next == list_first_entry(&tcbs, struct tcb, list))
    {
        temp = list_last_entry(&tcbs, struct tcb, list);
    }
    else
    {
        temp =  list_prev_entry(next, list);
        if(temp->state == TERMINATED)
        {
            temp =  list_prev_entry(temp, list);
        }
        temp->lifetime -= 1;
        if(temp->lifetime == 0)
        {
            temp->state = TERMINATED;
        }
    }
    return temp;
}
