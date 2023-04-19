#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "recover.h"

int main(int argc, char **argv){
    return recover(argc, argv);
}


/* Reference
    Milestone 1:

    https://stackoverflow.com/questions/44435549/how-to-make-getopt-long-print-nothing-when-there-is-error-command-line-argumen
    https://stackoverflow.com/questions/30723916/using-access-in-c
*/
