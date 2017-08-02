#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#define _GNU_SOURCE 1 
#include <string.h>

#include <sys/stat.h> // for mkdir
#include <sys/mman.h> // for mmap

#undef ntohl
#undef ntohs


#ifdef LINUX
typedef unsigned long uint64_t;
typedef unsigned short uint16_t;
extern void *memmem (const void *__haystack, size_t __haystacklen,
                     const void *__needle, size_t __needlelen);


#endif
/**
 *  Apple iOS OTA unpacker - by Jonathan Levin,
 *  http://NewOSXBook.com/
 *  
 *  Free for anyone to use, modify, etc. I won't complain :-), but I'd appreciate a mention
 *
 * Changelog: 02/08/16 - Replaced alloca with malloc () (full OTAs with too many files would have popped stack..)
 *
 *            02/17/16 - Increased tolerance for corrupt OTA - can now seek to entry in a file
 *
 *            08/31/16 - Added search in OTA.
 * 
 *
 */
	uint64_t pos = 0;
typedef 	unsigned int	uint32_t;

#pragma pack(1)
struct entry
{
 
 unsigned int usually_0x210_or_0x110;
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

extern int ntohl(int);
extern short ntohs(short);
uint32_t	
swap32(uint32_t arg)
{
return (ntohl(arg));
}

int g_list = 0;
int g_verbose = 0;
char *g_extract = NULL;
char *g_search = NULL;

void 
extractFile (char *File, char *Name, uint32_t Size, short Perms, char *ExtractCriteria)
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
	fchmod (fd, Perms);
	write (fd, File, Size);
	close (fd);



} //  end extractFile

void showPos()
{
	fprintf(stderr, "POS is %lld\n", pos);
}
int 
main(int argc ,char **argv)
{

	signal (2, showPos);
	char *filename ="p";
	int i = 0;

	if (argc < 2) {
		fprintf (stderr,"Usage: %s [-v] [-l] [-e file] _filename_\nWhere: -l: list files in update payload\n"
				"       -e _file: extract file from update payload (use \"*\" for all files)\n"
				"       -s _string _file: Look for occurences of _string_ in file\n", argv[0]);
		exit(10);
		}
	
	for (i = 1;
	     i < argc -1;
	     i++)
	    {
		// This is super quick/dirty. You might want to rewrite with getopt, etc..
		if (strcmp (argv[i], "-l") == 0) { g_list++;} 
		if (strcmp (argv[i] , "-v") == 0) { g_verbose++;}
		if (strcmp (argv[i], "-e") == 0) { 
			if (i == argc -1) { fprintf(stderr, "-e: Option requires an argument (what to extract)\n");
					    exit(5); }

			g_extract = argv[i+1]; i++;}

		// Added 08/31/16:
		if (strcmp (argv[i], "-s") == 0) { 
			if (i == argc -1) { fprintf(stderr, "-s: Option requires an argument (search string)\n");
					    exit(5); }
			g_search = argv[i+1];
			i++;	 }
			

	    }
	
	filename =argv[argc-1];
	//unsigned char buf[4096];

	int fd = open (filename, O_RDONLY);
	if (fd < 0) { perror (filename); exit(1);}

	// 02/17/2016 - mmap
	
	struct stat stbuf;
	int rc = fstat(fd, &stbuf);

	char *mmapped =  mmap(NULL, // void *addr,
			      stbuf.st_size ,	// size_t len, 
			      PROT_READ,        // int prot,
			      MAP_PRIVATE,                //  int flags,
			      fd,               // int fd, 
			      0);               // off_t offset);


	if (mmapped == MAP_FAILED)  { perror ("mmap"); exit(1);}
	i = 0;

	struct entry *ent = alloca (sizeof(struct entry));

	while(pos + 3*sizeof(struct entry) < stbuf.st_size) {
	
	ent = (struct entry *) (mmapped + pos );
	
	pos += sizeof(struct entry);

	if ((ent->usually_0x210_or_0x110 != 0x210 && ent->usually_0x210_or_0x110 != 0x110 &&
		ent->usually_0x210_or_0x110 != 0x310) || 
		ent->usually_0x00_00)
	{
		fprintf (stderr,"Corrupt entry (0x%x at pos %llu).. skipping\n", ent->usually_0x210_or_0x110,pos);
		int skipping = 1;

		while (skipping)
		{
		   ent = (struct entry *) (mmapped + pos ) ;
		   while (ent->usually_0x210_or_0x110 != 0x210 && ent->usually_0x210_or_0x110 != 0x110)
		   {
		     // #@$#$%$# POS ISN'T ALIGNED!
		     pos ++;
		    ent = (struct entry *) (mmapped + pos ) ;
		   }
		   // read rest of entry
		    int nl = ntohs(ent->nameLen);

		    if (ent->usually_0x00_00 || !nl) {
		//	 fprintf(stderr,"False positive.. skipping %d\n",pos);
			pos+=1;
			

			}
		    else { skipping =0;
			   pos += sizeof(struct entry); }
	 	}

	}

	uint32_t	size = swap32(ent->fileSize);

// fprintf(stdout," Here - ENT at pos %d: %x and 0 marker is %x namelen: %d, fileSize: %d\n", pos, ent->usually_0x210_or_0x110, ent->usually_0x00_00, ntohs(ent->nameLen), size);

	uint32_t	nameLen = ntohs(ent->nameLen);
	// Get Name (immediately after the entry)
	//
	// 02/08/2016: Fixed this from alloca() - the Apple jumbo OTAs have so many files in them (THANKS GUYS!!)
	// that this would exceed the stack limits (could solve with ulimit -s, or also by using
	// a max buf size and reusing same buf, which would be a lot nicer)

	
	// Note to AAPL: Life would have been a lot nicer if the name would have been NULL terminated..
	// What's another byte per every file in a huge file such as this?
	// char *name = (char *) (mmapped+pos);

	char *name = malloc (nameLen+1);

	strncpy(name, mmapped+pos , nameLen);
	name[nameLen] = '\0';
	//printf("NAME IS %s\n", name);

	pos += ntohs(ent->nameLen);
	uint32_t	fileSize = swap32(ent->fileSize);
	uint16_t	perms = ntohs(ent->perms);	
	if (g_list){ 
	if (g_verbose) {
	printf ("Entry @0x%d: UID: %d GID: %d Mode: %o Size: %d (0x%x) Namelen: %d Name: ", i,
							ntohs(ent->uid), ntohs(ent->gid),
						     perms, size, size,
						      ntohs(ent->nameLen));
	}
	printf ("%s\n", name);}

	// Get size (immediately after the name)
	if (fileSize) 
		{
			if (g_extract) { extractFile(mmapped +pos, name, fileSize, perms, g_extract);}

	

			// Added 08/31/16 - And I swear I should have this from the start.
			// So darn simple and sooooo useful!
			if (g_search){
				char *found = memmem (mmapped+pos, fileSize, g_search, strlen(g_search));
				while (found != NULL)
				{
				int relOffset = found - mmapped - pos;
				
				fprintf(stdout, "Found in Entry: %s, relative offset: %llx (Absolute: %lx)\n",
					name,
					relOffset,
					found - mmapped);

				// keep looking..
				 found = memmem (found + 1, fileSize - relOffset , g_search, strlen(g_search));
				} // end while
				
			} // end g_search

			pos +=fileSize;
		}



	
	free (name);

	} // Back to loop


	

	close(fd);

}
