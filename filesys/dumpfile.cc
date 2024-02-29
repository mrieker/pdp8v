
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#include "dumpfile.h"

Word dumpfile (File *file, FILE *out)
{
    // print out its attributes (modtime, eof position, allocated blocks)
    time_t modtimsec = (((time_t) file->modtim[0]) << 24) | (((time_t) file->modtim[1]) << 12) | file->modtim[2];
    struct tm modtimtm = *gmtime (&modtimsec);
    fprintf (out, "  %04d-%02u-%02u@%02u:%02u:%02u.%04u %04o.%04o/%04o\n",
        modtimtm.tm_year + 1900, modtimtm.tm_mon + 1, modtimtm.tm_mday, modtimtm.tm_hour, modtimtm.tm_min, modtimtm.tm_sec,
        file->modtim[3] * 10000 / 4096, file->eofvbn, file->eofwrd, file->allocd);

    // dump out the blocks
    Stream stream (file, true);
    while (true) {
        Word lbn = file->vbntolbn (stream.curvbn);
        fprintf (out, " vbn=%04o wrd=%04o lbn=%04o:\n", stream.curvbn, stream.curwrd, lbn);
        Word buffer[WPB];
        Word rc = stream.read (buffer, WPB);
        if (rc >= ER_MIN) return rc;
        if (rc == 0) return 0;
        for (Word i = 0; i < rc; i += 16) {

            // print words in octal
            fputc (' ', out);
            Word j;
            for (j = 0; (j < 16) && (j + i < rc); j ++) {
                fprintf (out, " %04o", buffer[i+j]);
            }
            for (; j < 16; j ++) fputs ("     ", out);

            // print as 6-bit characters
            fputs ("  <", out);
            for (j = 0; (j < 16) && (j + i < rc); j ++) {
                fputc ((((buffer[i+j] >> 6) & 077) ^ 040) + 040, out);
                fputc  (((buffer[i+j]       & 077) ^ 040) + 040, out);
            }
            fputc ('>', out);
            for (; j < 16; j ++) fputs ("  ", out);

            // print as 8-bit characters
            fputs ("  <", out);
            for (j = 0; (j < 16) && (j + i < rc); j ++) {
                char b = buffer[i+j] & 0177;
                if ((b < ' ') || (b > 126)) b = '.';
                fputc (b, out);
                if (j & 1) {
                    b = ((buffer[i+j-1] >> 4) & 0360) | ((buffer[i+j] >> 8) & 0017);
                    if ((b < ' ') || (b > 126)) b = '.';
                    fputc (b, out);
                }
            }
            fputc ('>', out);
            fputc ('\n', out);
        }
    }
}
