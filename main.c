#include <stdio.h>


#include "so_scheduler.h"


void test(unsigned int prioritate){

    printf("\nMERGE\n");
}

int main(){

    so_init(1,1);


    so_fork(test,1);

    //sleep(3);
    printf("\nAICI");
    so_end();
    //printf("da main"); 
    //so_signal(1);
}