#include <stdio.h>

int main(int argc, char *argv[]) {

    if (argc < 5) {
        printf("USE: ./bankdb --accounts= --trace= --deadlock= --tick-ms --verbose");
        return 1;
    }
    
    printf("argc: %d\n", argc);

    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }



    return 0;
}