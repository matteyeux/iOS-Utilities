#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
// Changelog:
// 02/17/2016 - Fixed so it works with Apple TV OTA PBZX

#define PBZX_MAGIC      "pbzx"

int main(int argc, const char * argv[])
{

    // Dumps a pbzx to stdout. Can work as a filter if no argument is specified

    char buffer[1024];
    int fd = 0;
    int minChunk = 0;

    if (argc < 2) { fd  = 0 ;}
    else { fd = open (argv[1], O_RDONLY);
           if (fd < 0) { perror (argv[1]); exit(5); }
         }

    if (argc ==3) {
        minChunk = atoi(argv[2]);
        fprintf(stderr,"Starting from Chunk %d\n", minChunk);

        }

    read (fd, buffer, 4);
    if (memcmp(buffer, PBZX_MAGIC, 4)) { fprintf(stderr, "Can't find pbzx magic\n"); exit(0);}

    // Now, if it IS a pbzx

    uint64_t length = 0, flags = 0;

    read (fd, &flags, sizeof (uint64_t));
    flags = __builtin_bswap64(flags);

    fprintf(stderr,"Flags: 0x%llx\n", flags);

    int i = 0;
    int off = 0;

    int warn = 0 ;
    int skipChunk = 0;

    while (flags &   0x01000000) { // have more chunks
    i++;
    read (fd, &flags, sizeof (uint64_t));
    flags = __builtin_bswap64(flags);
    read (fd, &length, sizeof (uint64_t));
    length = __builtin_bswap64(length);

    skipChunk = (i < minChunk);
    fprintf(stderr,"Chunk #%d (flags: %llx, length: %lld bytes) %s\n",i, flags,length,
     skipChunk? "(skipped)":"");

    // Let's ignore the fact I'm allocating based on user input, etc..
    char *buf = malloc (length);
    read (fd, buf, length);

   // We want the XZ header/footer if it's the payload, but prepare_payload doesn't have that,
    // so just warn.


    if (memcmp(buf, "\xfd""7zXZ", 6))  { warn++;
                fprintf (stderr, "Warning: Can't find XZ header. Instead have 0x%x(?).. This is likely not XZ data.\n",
                        (* (uint32_t *) buf ));

                }
    else // if we have the header, we had better have a footer, too
    if (strncmp(buf + length -2, "YZ", 2)) { warn++; fprintf (stderr, "Warning: Can't find XZ footer. This is bad.\n"); }
        if (!warn && !skipChunk)
        {
        write (1, buf, length);
        }
        warn = 0;

    }

    return 0;
}
