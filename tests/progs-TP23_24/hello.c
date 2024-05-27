#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char** argv){

    if(argc!=2){
        perror("Wrong Number of Arguments!");
        return -1;
    }
    
    int time = atoi(argv[1]);
    if(time < 1){
        perror("Time (s) must be higher than 1");
        return -1;
    }

    for(int i=0;i<time*1000; i++){
        printf("%d: Hello, cosmos! May our paths intertwine in endless discovery\n", i);
        usleep(1000);
    }

    return 0;
}