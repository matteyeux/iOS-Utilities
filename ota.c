#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h> // for mkdir
#include <stdint.h>

#pragma pack(1)
struct entry
{
 
 unsigned int usually_0x10_01_00_00;
 unsigned short  usually_0x00_00; //_00_00;
 unsigned int  fileSize;
 unsigned short whatever;
 unsigned long long timestamp_likely;
 unsigned short _usually_0x20;
 unsigned short nameLen;
 unsigned short uid;
 unsigned short gid;
 unsigned short perms;

 char name[0];
 // Followed by file contents
};

#pragma pack()

uint32_t	
swap32(uint32_t arg)
{
return (ntohl(arg));
}

int g_list = 0;
int g_verbose = 0;
char *g_extract = NULL;

void 
extractFile (char *File, char *Name, uint32_t Size, char *ExtractCriteria)
{
	// MAYBE extract file (depending if matches Criteria, or "*").
	// You can modify this to include regexps, case sensitivity, what not. 
	// presently, it's just strstr()


	if (!ExtractCriteria) return;
	if ((ExtractCriteria[0] != '*') && ! strstr(Name, ExtractCriteria)) return;
	
	// Ok. Extract . This is simple - just dump the file contents to its directory.
	// What we need to do here is parse the '/' and mkdir(2), etc.
	
	char *dirSep = strchr (Name, '/');
	while (dirSep)
	{
		*dirSep = '\0';
		mkdir(Name,0755);
		*dirSep = '/';
		dirSep+=1;
		dirSep = strchr (dirSep, '/');
	}

	// at this point we're out of '/'s
	// go back to the last /, if any
	
	if (g_verbose)
	{
		fprintf(stderr, "Dumping %d bytes to %s\n", Size, Name);
	}
	int fd = open (Name, O_WRONLY| O_CREAT);
	fchmod (fd, 0644);
	write (fd, File, Size);
	close (fd);



} //  end extractFile

int 
main(int argc ,char **argv)
{
	char *filename ="p";
	int i = 0;

	if (argc < 2) {
		fprintf (stderr,"Usage: %s [-v] [-l] [-e file] _filename_\nWhere: -l: list files in update payload\n       -e _file: extract file from update payload (use \"*\" for all files)\n", argv[0]);
		exit(10);
		}
	
	for (i = 1;
	     i < argc -1;
	     i++)
	    {
		// This is super quick/dirty. You might want to rewrite with getopt, etc..
		if (strcmp (argv[i], "-l") == 0) { g_list++;} 
		if (strcmp (argv[i] , "-v") == 0) { g_verbose++;}
		if (strcmp (argv[i], "-e") == 0) { g_extract = argv[i+1]; i++;}

	    }
	
	filename =argv[argc-1];
	//unsigned char buf[4096];

	int fd = open (filename, O_RDONLY);
	if (fd < 0) { perror (filename); exit(1);}


#if 0
 	// Don't need this anymore :-)

	// Cheat - actually use 0x00 0x50 0x81 0xb4 as an anchor
	
	read (fd, buf, 4096);
	for (i = 0 ; i < 4096; i++)
	{
		if ((buf[i] == 0x00) && (buf[i+1] == 0x50) && (buf[i + 2] == 0x81) && (buf[i+3] == 0xb4))
		{
			printf("Got anchor\n");
			lseek(fd, i-26, SEEK_SET);
			break;

			
		}

	}
	if (i == 4096) { fprintf (stderr, "Unable to find anchor\n"); exit(5);}
	// Otherwise, we can start reading entries from here:

#endif
	
	i = 0;
	int bytes_read = 1;

	struct entry *ent = alloca (sizeof(struct entry));
	while(bytes_read ){
	bytes_read = read(fd, (void *) ent, sizeof(struct entry));
	if (bytes_read <= 0) { // fprintf (stderr, "Error reading next entry or EOF (this could be OK if it's the last entry, @TODO implement == 0 or fstat on filesize..)\n"); 
			exit(1);}
	i+= bytes_read;

	uint32_t	size = swap32(ent->fileSize);

	// Get Name (immediately after the entry)
	char *name = alloca(ntohs(ent->nameLen) + 1);
	bytes_read = read (fd , (void *) name, ntohs(ent->nameLen));
	if (bytes_read <= 0) { fprintf (stderr, "Error reading Name. This is not ok\n"); exit(2);}

	i+= bytes_read;
	name[ntohs(ent->nameLen)] = '\0';
	if (g_list){ 
	if (g_verbose) {
	printf ("Entry @0x%d: UID: %d GID: %d Size: %d (0x%x) Namelen: %d Name: ", i,
							ntohs(ent->uid), ntohs(ent->gid),
						     size, size,
						      ntohs(ent->nameLen));
	}
	printf ("%s\n", name);}



	// Get size (immediately after the name)
	uint32_t	fileSize = swap32(ent->fileSize);
	if (fileSize) 
		{
			char *file = malloc (fileSize);
			bytes_read = read (fd, (void *) file, fileSize);
			// Zero length file sizes are apparently ok..
			if (bytes_read <= 0) { fprintf (stderr, "Error reading file. This is not ok \n"); exit(3);}
			i+= bytes_read;
			if (g_extract) { extractFile(file, name, fileSize, g_extract);}
			free(file);
		}




	} // Back to loop


	

	close(fd);

}