#define _POSIX_C_SOURCE 200809L // required for strdup() on cslab

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//Sorry about the strange tabbing! Not sure what caused that, and I wasn't able to fix it in an efficient manner.
/* Use 16-bit code words */
#define NUM_CODES 4096 //changes

/* allocate space for and return a new string s+t */
char *strappend_str(char *s, char *t);

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c);

/* look for string s in the dictionary
 * return the code if found
 * return NUM_CODES if not found 
 */
unsigned int find_encoding(char *dictionary[], char *s);

/* write the code for string s to file */
void write_code(FILE* fd, char *dictionary[], char *s);

/* compress in_file_name to out_file_name */
void compress(char *in_file_name, char *out_file_name);

unsigned int pword = NUM_CODES;
//unsigned int bytes[3];
//unsigned int holder[1];
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: zip file\n");
        exit(1);
    }

    char *in_file_name = argv[1];
    char *out_file_name = strappend_str(in_file_name, ".zip");

    compress(in_file_name, out_file_name);

    // have to free the memory for out_file_name since strappend_str malloc()'ed it
    free(out_file_name);

    return 0;
}

/* allocate space for and return a new string s+t */
char *strappend_str(char *s, char *t)
{
    if (s == NULL || t == NULL)
    {
        return NULL;
    }

    // reminder: strlen() doesn't include the \0 in the length
    int new_size = strlen(s) + strlen(t) + 1;
    char *result = (char *)malloc(new_size*sizeof(char));
    strcpy(result, s);
    strcat(result, t);

    return result;
}

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c)
//void create(FILE *archive, char *dir, int verb);
{
    if (s == NULL)
    {
        return NULL;
    }

    // reminder: strlen() doesn't include the \0 in the length
    int new_size = strlen(s) + 2;
    char *result = (char *)malloc(new_size*sizeof(char));
    strcpy(result, s);
    result[new_size-2] = c;
    result[new_size-1] = '\0';

    return result;
}

/* look for string s in the dictionary
 * return the code if found
 * return NUM_CODES if not found 
 */
unsigned int find_encoding(char *dictionary[], char *s)
{
    if (dictionary == NULL || s == NULL)
    {
        return NUM_CODES;
    }

    for (unsigned int i=0; i<NUM_CODES; ++i)
    {
        /* code words are added in order, so if we get to a NULL value 
         * we can stop searching */
        if (dictionary[i] == NULL)
        {
            break;
        }

        if (strcmp(dictionary[i], s) == 0)
        {
            return i;
        }
    }
    return NUM_CODES;
}

/* write the code for string s to file */
void write_code(FILE* fd, char *dictionary[], char *s)
{
	unsigned char bytes[3];
	dictionary[4095] = 0;
    	if (dictionary == NULL || s == NULL)
    	{
    		return;
    	}

	unsigned int actual_code = find_encoding(dictionary, s);
        // should never call write_code() unless s is in the dictionary
	if (actual_code == NUM_CODES)
    	{
        	printf("Algorithm error!\n");
        	exit(1);
	}

	if (pword == NUM_CODES)
	{
		pword = actual_code;
		return;
	}

	else	
	{
		bytes[0] = ((pword&0xff0) >> 4); //first byte
		bytes[1] = ((pword  & 0x00f) << 4) | ((actual_code & 0xf00) >>8);
		bytes[2] = actual_code & 0x0ff;
		//in the bytes array now hold 3 8 bit chunks. ^..^

    		if (fwrite(bytes, sizeof(unsigned char), 3, fd) != 3)	
    		{
			printf("problem here\n");
	    		perror("write");
	    		exit(1);
		}
		pword = NUM_CODES;
	}
}

/* compress in_file_name to out_file_name */
void compress(char *in_file_name, char *out_file_name)
{
    // check for bad args
    if (in_file_name == NULL || out_file_name == NULL)
    {
        printf("Programming error!\n");
        exit(1);
    }

    // open both files
    FILE *in_fd = fopen(in_file_name, "r");
    if (in_fd == NULL)
    {
        perror(in_file_name);
        exit(1);
    }

    FILE *out_fd = fopen(out_file_name, "w+");
    if (out_fd == NULL)
    {
        perror(out_file_name);
        exit(1);
    }

    // use an array as the dictionary
    char *dictionary[NUM_CODES];

    // initialize the first 256 codes mapped to their ASCII equivalent as a string
    unsigned int next_code;
    for (next_code=0; next_code<256; ++next_code)
    {
        dictionary[next_code] = strappend_char("\0", (char)next_code);
    }

    // initialize the rest of the dictionary to be NULLs
    for (unsigned int i=256; i<NUM_CODES; ++i)
    {
        dictionary[i] = NULL;
    }

    char *cur_str;
    char cur_char;

    // CurrentString = first input character
    fread(&cur_char, sizeof(char), 1, in_fd);
    if (ferror(in_fd))
    {
        perror("fread");
        exit(1);
    }
	if (feof(in_fd))
	{
	    return;
	}
    cur_str = strappend_char("\0", cur_char);

    // CurrentChar = next input character
    fread(&cur_char, sizeof(char), 1, in_fd);
    if (ferror(in_fd))
    {
        perror("fread2");
        exit(1);
    }

    // While there are more input characters
    while (feof(in_fd) == 0)
    {
        // hold CurrentString+CurrentChar in a temp variable
        char *tmp = strappend_char(cur_str, cur_char);

        // If CurrentString+CurrentChar is in Dictionary
        unsigned int code = find_encoding(dictionary, tmp);
        
	if (code != NUM_CODES)
        {
            // CurrentString = CurrentString+CurrentChar
            char *hold = strappend_char(cur_str, cur_char);

            // have to free the memory for cur_str before we reassign it
            free(cur_str);
            cur_str = hold;
        }

        else
        {
            // only add tmp to dictionary if there are codes left 
            if (next_code < NUM_CODES)
            {
                // Add CurrentString+CurrentChar to Dictionary
                dictionary[next_code] = strdup(tmp);
                ++next_code;
            }

            // CodeWord = CurrentString's code in Dictionary
            // Output CodeWord
            write_code(out_fd, dictionary, cur_str);
            free(cur_str);

            // CurrentString = CurrentChar
            cur_str = strappend_char("\0", cur_char);
        }

        // don't forget to free tmp
        free(tmp);
        // read the next char
        fread(&cur_char, sizeof(char), 1, in_fd);
        if (ferror(in_fd))
        {
            perror("read");
            exit(1);
        }
    }
    write_code(out_fd, dictionary, cur_str);

    if (pword != NUM_CODES)
    {
        write_code(out_fd, dictionary, NULL);
    }

    // free the last memory allocated for cur_str
    free(cur_str);

    // close the files
    if (fclose(in_fd) != 0)
    {
        perror("close");
    }

    if (fclose(out_fd) != 0)
    {
        perror("close");
    }

    // free all memory in the dictionary
    for (unsigned int i=0; i<NUM_CODES; ++i)
    {
        if (dictionary[i] == NULL)
        {
            break;
        }

        free(dictionary[i]);
    }

    return;
}
