#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
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
 *            02/28/18 - It's been a while - and ota now does diff!
 *                       Also tidied up and made neater
 *
 *  To compile: gcc otaa.c -o ota
 *  		Remember to add '-DLINUX' if on Linux
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



// Since I now diff and use open->mmap(2) on several occasions, refactored
// into its own function
//
void *mmapFile(char *FileName, uint64_t *FileSize)
{

	int fd = open (FileName, O_RDONLY);
	if (fd < 0) { perror (FileName); exit(1);}

	// 02/17/2016 - mmap

	struct stat stbuf;
	int rc = fstat(fd, &stbuf);

	char *mmapped =  mmap(NULL, // void *addr,
			      stbuf.st_size ,	// size_t len,
			      PROT_READ,        // int prot,
			      MAP_PRIVATE,                //  int flags,
			      fd,               // int fd,
			      0);               // off_t offset);


	if (mmapped == MAP_FAILED)  { perror (FileName); exit(1);}

	if (FileSize) *FileSize = stbuf.st_size;

	close (fd);
	return (mmapped);
}
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
	fprintf(stderr, "POS is %lu\n", pos);
}

struct entry *getNextEnt (char *Mapping, uint64_t Size, uint64_t *Pos)
{
	// Return entry at Mapping[Pos],
	// and advance Pos to point to next one

	int pos = 0;
	struct entry *ent =(struct entry *) (Mapping + *Pos );

	if (*Pos > Size) return (NULL);
	*Pos += sizeof(struct entry);

	uint32_t entsize = swap32(ent->fileSize);
	uint32_t nameLen = ntohs(ent->nameLen);
        // Get Name (immediately after the entry)
        //char *name = malloc (nameLen+1);
        // strncpy(name, Mapping+ *Pos , nameLen);
        //name[nameLen] = '\0';
	//printf("NAME %p IS %s, Size: %d\n", Mapping, name, entsize);
	//free (name);
	*Pos += nameLen;
	*Pos += entsize;

	return (ent);

} // getNextEnt


int doDiff (char *File1, char *File2, int Exists)
{

	// There are two ways to do diff:
	// look at both files as archives, find diffs, then figure out diff'ing entry,
	// or look at file internal entries individually, then compare each of them
	// I chose the latter. This also (to some extent) survives file ordering

	// Note I'm still mmap(2)ing BOTH files. This contributes to speed, but does
	// have the impact of consuming lots o'RAM. That said, this is to be run on a
	// Linux/MacOS, and not on an i-Device, so we should be ok.

	uint64_t file1Size = 0;

	char *file1Mapping = mmapFile(File1, &file1Size);
	uint64_t file2Size = 0;
	char *file2Mapping = mmapFile(File2, &file2Size);


	uint64_t file1pos = 0;
	uint64_t file2pos = 0;

	struct entry *file1ent = getNextEnt (file1Mapping, file1Size, &file1pos);
	struct entry *file2ent  = getNextEnt (file2Mapping,file2Size, &file2pos);

	uint64_t lastFile1pos, lastFile2pos = 0;

	while (file1ent && file2ent) {

		lastFile1pos = file1pos;
		lastFile2pos = file2pos;

		file1ent = getNextEnt (file1Mapping, file1Size, &file1pos);
		file2ent = getNextEnt (file2Mapping,file2Size, &file2pos);

		char *ent1Name = file1ent->name;
		char *ent2Name = file2ent->name;

		// Because I'm lazy: skip last entry
		if (file1pos > file1Size - 1000000) break;

		int found = 1;

		char *n1 = strndup(file1ent->name, ntohs(file1ent->nameLen));
		if (strncmp(ent1Name, ent2Name, ntohs(file1ent->nameLen)))
			{
				// Stupid names aren't NULL terminated (AAPL don't read my comments,
				// apparently), so we have to copy both names in:

				// But that's the least of our problems: We don't know if n1 has been removed
				// from n2, or n2 is a new addition:
				uint64_t seekpos = file2pos;
				// seek n1 in file2:

				found = 0;
				int i = 0;

				struct entry *seek2ent;
				while (1) {
					seek2ent = getNextEnt (file2Mapping,file2Size, &seekpos);

					if (!seek2ent) { break; } // {printf("EOF\n");break;}

					if (memcmp(seek2ent->name,file1ent->name, ntohs(seek2ent->nameLen)) == 0) {

						found++; break;
					}
					else {
/*
						i++;
						if (i < 200) {
						char *n2 = strndup(seek2ent->name, ntohs(seek2ent->nameLen));

						printf("check: %s(%d) != %s(%d) -- %d\n",n2, ntohs(seek2ent->nameLen),n1, strlen(n1),
					memcmp(seek2ent->name,file1ent->name, ntohs(seek2ent->nameLen) ));
						free(n2);

						}
*/
					  }
				} // end while

				if (!found) {
						printf("%s: In file1 but not file2\n", n1);
						// rewind file2pos so we hit the entry again..
						file2pos = lastFile2pos;
					    }
				else {
						// Found it - align (all the rest to this point were not in file1)
						file2pos = seekpos;
					}


			} // name mismatch

		if (found) {
			// Identical entries - check for diffs unless we're only doing existence checks

			// if the sizes diff, obviously:

			if (!Exists) {
			if (file1pos - lastFile1pos != file2pos - lastFile2pos)
				{ fprintf(stdout,"%s (different sizes)\n", n1); }
			else
				// if sizes are identical, maybe - but ignore timestamp!
			if (memcmp (((unsigned char *)file1ent) + sizeof(struct entry),
				    ((unsigned char *)file2ent) + sizeof(struct entry), file1pos - lastFile1pos - sizeof(struct entry)))
			{ fprintf(stdout,"%s\n", n1); }

			}
		free (n1);
		}

	} // end file1pos
	return 0;
}
int 
main(int argc ,char **argv)
{

	char *filename ="p";
	int i = 0;

	if (argc < 2) {
		fprintf (stderr,"Usage: %s [-v] [-l] [-e file] _filename_\nWhere: -l: list files in update payload\n"
				"       -e _file: extract file from update payload (use \"*\" for all files)\n"
				"       -s _string _file: Look for occurences of _string_ in file\n"
				"       [-n] -d _file1 _file2: Point out differences between OTA _file1 and _file2\n"
				"                              -n to only diff names\n", argv[0]);
		exit(10);
		}
	
	int exists = 0;

	for (i = 1;
	     i < argc -1;
	     i++)
	    {
		// This is super quick/dirty. You might want to rewrite with getopt, etc..
		if (strcmp(argv[i], "-n") == 0) {
					exists++;
			}
		if (strcmp (argv[i] , "-d") == 0) {
		  // make sure we have argv[i+1] and argv[i+2]...

		  if (i != argc - 3)
			{
				fprintf(stderr,"-d needs exactly two arguments - two OTA files to compare\n");
				exit(6);
			}

		  // that the files exist...
		  if (access (argv[i+1], F_OK)) { fprintf(stderr,"%s: not a file\n", argv[i+1]); exit(11); }
		  if (access (argv[i+2], F_OK)) { fprintf(stderr,"%s: not a file\n", argv[i+2]); exit(12); }

		  // then do diff
		  return ( doDiff (argv[i+1],argv[i+2], exists));

		}
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


	// Another little fix if user forgot filename, rather than try to open
	if (argv[argc-1][0] == '-') {
		fprintf(stderr,"Must supply filename\n"); exit(5);
	}

	filename = argv[argc-1];

	//unsigned char buf[4096];

	uint64_t fileSize;

	char *mmapped = mmapFile(filename, &fileSize);

	i = 0;

	struct entry *ent = alloca (sizeof(struct entry));

	while(pos + 3*sizeof(struct entry) < fileSize) {
	ent = (struct entry *) (mmapped + pos );
	pos += sizeof(struct entry);

	if ((ent->usually_0x210_or_0x110 != 0x210 && ent->usually_0x210_or_0x110 != 0x110 &&
		ent->usually_0x210_or_0x110 != 0x310) || 
		ent->usually_0x00_00)
	{
		fprintf (stderr,"Corrupt entry (0x%x at pos %lu).. skipping\n", ent->usually_0x210_or_0x110,pos);
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
				
				fprintf(stdout, "Found in Entry: %s, relative offset: 0x%x (Absolute: %lx)\n",
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
}