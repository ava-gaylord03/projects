#define _POSIX_C_SOURCE 200809L // required for strdup() on cslab

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Use 16-bit code words */
#define NUM_CODES 65536

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c);

/* read the next code from fd
 * return NUM_CODES on EOF
 * return the code read otherwise
 */
unsigned int read_code(int fd);

/* uncompress in_file_name to out_file_name */
void uncompress(char *in_file_name, char *out_file_name);

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: unzip file\n");
		exit(1);
	}

	char *in_file_name = argv[1];
	char *out_file_name = strdup(in_file_name);
	out_file_name[strlen(in_file_name)-4] = '\0';

	uncompress(in_file_name, out_file_name);

	free(out_file_name);

	return 0;
}

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c)
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

/* read the next code from fd
 * return NUM_CODES on EOF
 * return the code read otherwise
 */
unsigned int read_code(int fd)
{
	// code words are 16-bit unsigned shorts in the file
	unsigned short actual_code;
	int read_return = read(fd, &actual_code, sizeof(unsigned short));
	if (read_return == 0)
	{
		return NUM_CODES;
	}
	if (read_return != sizeof(unsigned short))
	{
		printf("read_ret bef\n");
		perror("read");
		exit(1);
	}
	return (unsigned int)actual_code;
}

/* uncompress in_file_name to out_file_name */
void uncompress(char *in_file_name, char *out_file_name)

{
	unsigned int cur_c = 0;
	char* cur_s = strappend_char("\0", cur_c);
	int fd;
	int fd_two;
	int container = 256;
	char *dictionary[NUM_CODES];


	if ((in_file_name == NULL) | (out_file_name == NULL))
	{
		printf("file name is NULL!\n");
		exit(1);
	}
		
	fd = open(in_file_name, O_RDONLY);
	if (fd == -1)
	{
		perror("error opening file\n");
		exit(1);
	}
	
	fd_two = open(out_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd_two == -1)
	{
		perror("error opening out file\n");
		exit(1);
	}


	unsigned int i;
	for(i = 0; i < 256; i++)
	{
		dictionary[i] = strappend_char("\0", (char)i);
	}
	unsigned int j;
	for(j = 256; j < NUM_CODES; j++)
	{
		dictionary[j] = NULL;
	}	
	
	if (write(fd_two, cur_s, strlen(cur_s)) < 0)
	{
		perror("write\n");
		exit(1);
	}
	printf("before cur read\n");
	cur_c = read_code(fd);	
	printf("after cur read\n");
	while (cur_c != NUM_CODES)
	{
		printf("before read code\n");
		unsigned int next_code = read_code(fd);	
		printf("after read code\n");
		if (dictionary[next_code] == NULL)
		{
			dictionary[cur_c] = strappend_char(cur_s, cur_c);
		}

		else 
		{
			cur_s = dictionary[next_code];
		}

		if(write(fd_two, cur_s, strlen(cur_s)) < 0)
		{
			perror("write\n");
			exit(1);
		}

		cur_c = cur_s[0];
		char *old_s = dictionary[cur_c];
		dictionary[container] = strappend_char(old_s, cur_c);
		cur_c = next_code;
		container++;
	}

	if (close(fd) < 0)
	{
		perror("close fd\n");
		exit(1);
	}

	if (close(fd_two) < 0)	
	{
		perror("close fd_two\n");
		exit(1);
	}
	printf("before return\n");
	return;
}


/*
{
	char *dictionary[NUM_CODES];

	// check for NULL
	if (in_file_name == NULL || out_file_name == NULL)
	{
		printf("Programming error!\n");
		exit(1);
	}

	// open files
	int in_fd = open(in_file_name, O_RDONLY);
	if (in_fd < 0)
	{
		perror(in_file_name);
		exit(1);
	}

	int out_fd = open(out_file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (out_fd < 0)
	{
		perror(out_file_name);
		exit(1);
	}

	//first 256 codes to ascii
	unsigned int next_code;
	for (next_code = 0; next_code < 256; next_code++)
	{
		dictionary[next_code] = strappend_char("\0", (char)next_code);
	}

	//set rest of dict to NULL
	for (unsigned int i = 256; i < NUM_CODES; ++i)
	{
		dictionary[i] = NULL;
	}

	unsigned int cur_code = read_code(in_fd);
	if (cur_code == NUM_CODES)
	{
		// return if at end
		return;
	}
	//cur string to current code in dict
	char *cur_str = dictionary[cur_code];
	if (cur_str == NULL)
	{
		printf("Algorithm error!\n");
		exit(1);
	}
	int cur_str_length = strlen(cur_str);
	if (cur_str_length != 1)
	{
		printf("Algorithm error!\n");
		exit(1);
	}

	// Output CurrentChar
	if (write(out_fd, cur_str, cur_str_length) != cur_str_length)
	{
		perror("write");
		exit(1);
	}

	char cur_char = cur_str[0];

	// have to track if cur_str needs to be freed each iteration
	int need_free = 0;
	unsigned int new_code = read_code(in_fd);
	
	//while more code words in dict
	while (new_code != NUM_CODES)
	{
		// check if cur_str needs to be freed
		if (need_free)
		{
			free(cur_str);
			need_free = 0;
		}

		// If NextCode is NOT in Dictionary
		if (dictionary[new_code] == NULL)
		{
			cur_str = dictionary[cur_code];
			// CurrentString = CurrentString+CurrentChar
			cur_str = strappend_char(cur_str, cur_char);
			// track that cur_str was allocated and needs to be freed later
			need_free = 1;
		}
		else
		{
			cur_str = dictionary[new_code];
		}

		// Output CurrentString
		cur_str_length = strlen(cur_str);
		if (write(out_fd, cur_str, cur_str_length) != cur_str_length)
		{
			perror("write");
			exit(1);
		}

		cur_char = cur_str[0];
		
		if(next_code < NUM_CODES)
		{
			dictionary[next_code] = strappend_char(dictionary[cur_code], cur_char);
			++next_code;
		}

		cur_code = new_code;
		new_code = read_code(in_fd);
	}

	// close files
	if (close(in_fd) < 0)
	{
		perror("close");
	}
	if (close(out_fd) < 0)
	{
		perror("close");
	}

	// free dictionary
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


*/

