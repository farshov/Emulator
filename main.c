#include "start.h"


int main(int argc, char * argv[])
{
	FILE * input = fopen(argv[1], "r");
    if(errno)
    {
        perror("INVALID FILE");
        exit(0);
    }
    load_file(input);
    fclose(input);
    run_programme(0x0200, 0x000c);
    return 0;
}