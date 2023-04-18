#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"

int main(void)
{
    CHAIN(CHAIN_IN(PATH("examples", "pipe.c")),
          CHAIN_CMD(PATH("tools", "rot13")),
          CHAIN_CMD(PATH("tools", "hex")),
          CHAIN_OUT("output.txt"));
    CMD(PATH("tools", "cat"), "output.txt");
    RM("output.txt");

    return 0;
}
