#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <zconf.h>
#include <sys/mman.h>

#include "jrunner.h"
#include "KernelInterface.h"

int main()
{
    int jtagDescriptor = open("/dev/START2_JTAG", O_RDWR);
    unsigned char *rbfData;
    if (jtagDescriptor < 0)
        return -1;
    KernelJTAGSetDescriptor(jtagDescriptor);
    KernelJTAGSetDataAddress();

    FILE *stream = fopen("start_a64.rbf", "rb");
    if (stream == NULL) {
        printf("Error in .rbf file");
        return -2;
    }
    fseek(stream, 0, SEEK_END);
    if (ftell(stream) <= 0) {
        printf("Error in .rbf file");
        return -2;
    }

    size_t rbfLength = (size_t) ftell(stream);
    rbfData = malloc(rbfLength);
    fseek(stream, 0, SEEK_SET);
    fread(rbfData, rbfLength, 1, stream);
    printf("The rbf is read.\n");

    JrunnerMain(rbfData, rbfLength);
    printf("The rbf is imported.\n");

    free(rbfData);
    fclose(stream);
    close(jtagDescriptor);
    return 0;
}