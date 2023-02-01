#define _POSIX_C_SOURCE 200809L // required for PATH_MAX, don't remove, it breaks

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

/* create a new string dir/file */
char *build_path(char *dir, char *file);

/* create an archive of dir in the archive file */
void create(FILE *archive, char *dir, int verb);

/* read through the archive file and recreate files and directories from it */
void extract(FILE *archive, int verb);

/* standard usage message and exit */
void usage();

/* create a new string dir/file */
char *build_path(char *dir, char *file)
{
	// allocate space for dir + "/" + file + \0 
    	int path_length = strlen(dir) + 1 + strlen(file) + 1;
    	char *full_path = (char *)malloc(path_length*sizeof(char));

    	// fill in full_path with dir then "/" then file
    	strcpy(full_path, dir);
    	strcat(full_path, "/");
    	strcat(full_path, file);

    	// free the returned memory
    	return full_path;
}

/* create an archive of dir in the archive file */
void create(FILE *archive, char *dir, int verb)
{
	struct stat finfo;
	struct dirent *de;
	DIR *d = opendir(dir);
	
	//check for NULL
	if (d == NULL || archive == NULL)	
	{
		perror(dir);
		exit(1);
	}
	
	//error checking
	if (stat(dir, &finfo) != 0)
	{
		perror(dir);
		free(dir);
	}
	
	//formatting
	fprintf(archive, "%s\n", dir);	
	fwrite(&finfo, sizeof(struct stat), 1, archive);
	
	//"processing:"
	if (verb == 1)
	{
		printf("%s processing\n", dir);
	}	
	
	//loop through every entry in the directory
	for (de = readdir(d); de != NULL; de = readdir(d))	
	{
		//Skip if .. or .
		if((strcmp(de->d_name, "..") == 0) | (strcmp(de->d_name, ".") == 0))
		{
			continue;
		}
		
		//Build path
		char *full_path = build_path(dir, de->d_name);
		if (stat(full_path, &finfo) != 0)
		{
			perror(full_path);
			free(full_path);
			continue;
		}
		
		//Is a directory
		if(S_ISDIR(finfo.st_mode))	
		{
			//recursively create
			create(archive, full_path, verb);
		}
		
		//It is a file
		else if(S_ISREG(finfo.st_mode))
		{
			char buf[finfo.st_size];

			FILE *fp = fopen(full_path, "w");
			if (fp == NULL)
			{
				perror("open");
				exit(1);
			}

			//if verbose, "processing: path to file"
			if (verb == 1)
			{
				printf("%s: processing\n", full_path);
			}

			fprintf(archive, "%s\n", full_path);
			//write to archive
			fwrite(&finfo, sizeof(struct stat), 1, archive);
			//read
			int read = fread(buf, sizeof(char), finfo.st_size, fp);
			fwrite(buf, sizeof(char), read, archive);
			
			//close
			if (fclose(fp) != 0)
			{
				perror("close");
			}
		}
		free(full_path);
	}

	if (closedir(d) != 0)
	{
		perror("closedir");
	}
	return;
}

/* read through the archive file and recreate files and directories from it */
void extract(FILE *archive, int verb)
{
	struct stat finfo;
	char filename[PATH_MAX];
	//Read first file name
	int read = fscanf(archive, "%s\n", filename);
	//Read first file struct
	read = fread(&finfo, sizeof(struct stat), 1, archive);
	
	//archive must not be null for while loop (and shouldn't be null anyway)
	if (archive == NULL)	
	{
		perror("archive");
	}

	//read check
	if(read < 0)
	{
		perror("read");
		exit(1);
	}

	//feof reached at end of archive (archive is just a file)
	while (feof(archive) == 0)	
	{
		//the file is, in fact, a directory
		if (S_ISDIR(finfo.st_mode))
		{
			int create_check = (mkdir(filename, finfo.st_mode));
			if (create_check == -1)
			{
				printf("mkdir problem");
			}
		}

		//if file is actually a file
		else if (S_ISREG(finfo.st_mode))
		{
			char buf_two[finfo.st_size];
			fread(buf_two, sizeof(char), finfo.st_size, archive);
			FILE *fp_two = fopen(filename, "w");	
			int write = fwrite(buf_two, sizeof(char), finfo.st_size, fp_two);

			//error checking for fwrite and close
			if (write != finfo.st_size)	
			{
				perror("fwrite");
			}

			if (fclose(fp_two) != 0)	
			{
				perror("fclose");
			}
		}

		//processing
		if (verb == 1)	
		{
			printf("%s%s", ": processing\n", filename);
		}
		
		//read out of the archive
		read = fread(&finfo, sizeof(struct stat), 1, archive);
		fscanf(archive, "%s\n", filename);
		
		//read error checking
		if (read < 0)	
		{
			perror("read");
			exit(1);
		}
	}
}

/* standard usage message and exit */
void usage()
{
    printf("usage to create an archive:  tar c[v] ARCHIVE DIRECTORY\n");
    printf("usage to extract an archive: tar x[v] ARCHIVE\n\n");
    printf("  the v parameter is optional, and if given then every file name\n");
    printf("  will be printed as the file is processed\n");
    exit(1);
}

int main(int argc, char **argv)
{
    int verbose;
    FILE *arch;

    // must have exactly 2 or 3 real arguments
    if (argc < 3 || argc > 4)
    {
        usage();
    } 

    // 3 real arguments means we are creating a new archive
    if (argc == 4)
    { 
        // check for the create flag without the verbose flag
        if (strcmp(argv[1],"c") == 0)
        {
            verbose = 0;
        }
        // check for the create flag with the verbose flag
        else if (strcmp(argv[1],"cv") == 0)
        {
            verbose = 1;
        }
        // anything else is an error
        else
        {
            usage();
        }

        // get the archive name and the directory we are going to archive
        char *archive = argv[2];
        char *dir_name = argv[3];

        // remove any trailing slashes from the directory name
        while (dir_name[strlen(dir_name)-1] == '/')
        {
            dir_name[strlen(dir_name)-1] = '\0';
        }

        // open archive
        arch = fopen(archive, "w");
        if (arch == NULL)
        {
            perror(archive);
            exit(1);
        }

        // recursively fill in the archive
        create(arch, dir_name, verbose);
    }

    // 2 real arguments means we are creating a new archive
    else if (argc == 3)
    { 
        // check for the extract flag without the verbose flag
        if (strcmp(argv[1],"x") == 0)
        {
            verbose = 0;
        }
        // check for the extract flag with the verbose flag
        else if (strcmp(argv[1],"xv") == 0)
        {
            verbose = 1;
        }
        // anything else is an error
        else
        {
            usage();
        }

        // get the archive file name
        char *archive = argv[2];

        // open the archive
        arch = fopen(archive, "r");
        if (arch == NULL)
        {
            perror(archive);
            exit(1);
        }

        // recursively start processing archive
        extract(arch, verbose);
    }

    return 0;
}
