
// g++ -o dumprk05 dumprk05.cc

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define NBLKS 6496  // total blocks in the .rk05 file
#define BLKSZ 256   // number of 12-bit words per block

static char sixbit (uint16_t word)
{
    word &= 077;
    return (word < 040) ? (word + 0100) : word;
}

static char eightbit (uint16_t word)
{
    word &= 0177;
    return ((word < ' ') || (word == 0177)) ? '.' : word;
}

int main (int argc, char **argv)
{
    uint16_t disk[NBLKS*BLKSZ];

    int fd = open (argv[1], O_RDONLY);
    if (fd < 0) {
        fprintf (stderr, "error opening %s: %m\n", argv[1]);
        return 1;
    }

    int rc = read (fd, disk, sizeof disk);
    if (rc < sizeof disk) {
        if (rc < 0) {
            fprintf (stderr, "error reading %s: %m\n", argv[1]);
        } else {
            fprintf (stderr, "only read %d of %u bytes from %s\n", rc, sizeof disk, argv[1]);
        }
        return 1;
    }

    for (int i = 0; i < NBLKS; i ++) {
        printf ("block %4u = %05o:\n", i, i);
        uint16_t *blk = disk + i * BLKSZ;
        for (int j = 0; j < BLKSZ; j += 16) {
            printf (" %04o:", j);
            for (int k = 0; k < 16; k ++) {
                printf (" %04o", blk[j+k]);
            }
            printf ("  <");
            for (int k = 0; k < 16; k ++) {
                uint16_t word = blk[j+k];
                putchar (sixbit (word >> 6));
                putchar (sixbit (word));
            }
            printf (">  <");
            for (int k = 0; k < 16; k += 2) {
                uint16_t word0 = blk[j+k+0];
                uint16_t word1 = blk[j+k+1];
                putchar (eightbit (word0));
                putchar (eightbit (word1));
                putchar (eightbit (((word0) >> 4 & 0xF0) | ((word1 >> 8) & 0x0F)));
            }
            printf (">  <");
            for (int k = 0; k < 16; k ++) {
                uint16_t word = blk[j+k];
                putchar (eightbit (word));
            }
            printf (">\n");
        }
    }

    return 0;
}
