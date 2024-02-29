
/*
    test program that starts Focal-69 running on the DE0 via RasPi
    run it on a RasPi that has a DE0 connected and programmed
    it fork/execs raspictl that starts focal
    then it stuffs a simple test program into focal and runs it
    it makes quite a few typing mistakes sending the program to focal

    cc -o focal69test.armv6l focal69test.c
    ./focal69test.armv6l
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

char const *const codelines[] = {
    "*1.10 ASK \"HOW MUCH BORROW? \",PRIN\r",
    "*1.20 ASK \"HOW MANY YEARS? \",TERM\r",
    "*1.30 FOR RATE=4.0,.5,10;DO 2.0\r",
    "*1.40 QUIT\r",
    "*2.10 SET INT=PRIN*(RATE/100)*TERM\r",
    "*2.20 TYPE \"RATE\",RATE,\"  \",\"INTEREST\",INT,!\r",
    //  "*WRITE\r", "*", "*",
    "*GO\r",
    ":1000\r",
    ":5\r",
    NULL
};

static void readecho (int fd, char ch);
static char readchar (int fd);
static void writechar (int fd, char ch);

int main ()
{
    char ch;
    int kbfd, i, j, pid, prfd;

    setlinebuf (stdout);

    int seed = time (NULL);
    printf ("seed=%d\n", seed);
    srand (seed);

    unlink ("focal69test.kb");
    unlink ("focal69test.pr");
    if (mkfifo ("focal69test.kb", 0600) < 0) abort ();
    if (mkfifo ("focal69test.pr", 0600) < 0) abort ();

    pid = fork ();
    if (pid < 0) abort ();
    if (pid == 0) {
        char const *argv[] = {
            "../driver/raspictl",
            "-binloader",
            "-cpuhz", "52000",
            "-haltstop",
            "-quiet",
            "-startpc", "200",
            "bins/focal69.bin",
            NULL };
        char const *envp[] = {
            "iodevptaperdr=",
            "iodevptaperdr_debug=0",
            "iodevtty=focal69test.kb|focal69test.pr",
            "iodevtty_debug=0",
            "switchregister=0",
            NULL };
        execve (argv[0], (char **) argv, (char **) envp);
        fprintf (stderr, "error forking %s: %m\n", argv[0]);
        exit (255);
    }

    kbfd = open ("focal69test.kb", O_WRONLY);
    if (kbfd < 0) abort ();
    prfd = open ("focal69test.pr", O_RDONLY);
    if (prfd < 0) abort ();

    for (i = 0; codelines[i] != NULL; i ++) {
        char const *codeline = codelines[i];
        do {
            ch = readchar (prfd);
            writechar (1, ch);
        } while (ch != codeline[0]);
        for (j = 0; codeline[++j] != 0;) {
            ch = codeline[j];
            if ((codeline[0] == '*') && (ch >= ' ') && ((rand () & 15) == 0)) {
                writechar (kbfd, ch + 1);
                readecho (prfd, ch + 1);
                writechar (kbfd, 0377);
                readecho (prfd, '\\');
            }
            writechar (kbfd, ch);
            readecho (prfd, ch);
        }
    }

    do {
        ch = readchar (prfd);
        writechar (1, ch);
    } while (ch != '*');
    return 0;
}

static void readecho (int fd, char ch)
{
    char ch2;

    ch2 = readchar (fd);
    writechar (1, ch2);
    if (ch2 != ch) {
        fprintf (stderr, "bad echo char <%c> should be <%c>\n", ch2, ch);
        abort ();
    }
}

static char readchar (int fd)
{
    char ch;
    int rc;

    rc = read (fd, &ch, 1);
    if (rc <= 0) {
        if (rc == 0) errno = EPIPE;
        fprintf (stderr, "readchar: error reading from printer: %m\n");
        abort ();
    }
    return ch;
}

static void writechar (int fd, char ch)
{
    int rc;

    if (ch != 0) {
        rc = write (fd, &ch, 1);
        if (rc <= 0) {
            if (rc == 0) errno = EPIPE;
            fprintf (stderr, "writechar: error writing to keyboard: %m\n");
            abort ();
        }
    }
}
