
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main ()
{
    int memfd;
    int volatile *leds;
    void *ptr;

    memfd = open ("/dev/mem", O_RDWR);
    if (memfd < 0) {
        fprintf (stderr, "error opening /dev/mem: %m\n");
        return 1;
    }
    ptr = mmap (NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0x43C00000);
    if (ptr == MAP_FAILED) {
        fprintf (stderr, "error mmapping /dev/mem: %m\n");
        return 1;
    }

    leds = (int volatile *)ptr;
    while (1) {
        usleep (100000);
        printf (" %08X %08X %08X %08x\n", leds[0], leds[1], leds[2], leds[3]);
        fflush (stdout);
    }
}
