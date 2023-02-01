#define _POSIX_C_SOURCE 200809L // required for strdup() on cslab
#define _DEFAULT_SOURCE // required for strsep() on cslab
#define _BSD_SOURCE // required for strsep() on cslab

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "pid_list.h"

#define MAX_ARGS 32


void redirect(char **command_line_words, size_t num_args);

void redirect(char **command_line_words, size_t num_args)
{
	int i = 0;
	for(i = 0; i < num_args; i++)
	{

		if(strcmp(command_line_words[i], "<") == 0) 
		{
			command_line_words[i] = NULL;

			int write_fd = open(command_line_words[i+1], O_RDONLY);
			if (write_fd == -1)
			{
				perror("open");
				exit(1);
			}

			if (dup2(write_fd, STDIN_FILENO) == -1)
			{
				perror("dup2");
				exit(1);
			}
			close(write_fd);
			continue;
		}
		if (strcmp(command_line_words[i], ">") == 0)
		{
			command_line_words[i] = NULL;

			int write_fd = open(command_line_words[i+1], O_WRONLY | O_TRUNC | O_CREAT, 0644);	//truncate any existing file, or create
			if (write_fd == -1)
			{
				perror("open");
				exit(1);
			}
			if (dup2(write_fd, STDOUT_FILENO) == 0)
			{	
				perror("dup2");
				exit(1);
			}
			close(write_fd);
			continue;
		}
		if (strcmp(command_line_words[i], ">>") == 0)
		{
			command_line_words[i] = NULL;

			int write_fd = open(command_line_words[i+1], O_WRONLY | O_APPEND | O_CREAT, 0644);	//appends to file if it exists, or creates
			if (write_fd == -1)
			{
				perror("open");
				exit(1);
			}
			if (dup2(write_fd, STDOUT_FILENO) == 0)
			{	
				perror("dup2");
				exit(1);
			}
			close(write_fd);
		}

	}
}	

char **get_next_command(size_t *num_args)
{
    	// print the prompt
	printf("cssh$ ");

    	// get the next line of input
    	char *line = NULL;
   	size_t len = 0;
    	getline(&line, &len, stdin);
    	if (ferror(stdin))
    	{
    		perror("getline");
    	    	exit(1);
    	}
    	if (feof(stdin))
    	{
        	return NULL;
    	}

    	// turn the line into an array of words
    	char **words = (char **)malloc(MAX_ARGS*sizeof(char *));
    	int i=0;

    	char *parse = line;
    	while (parse != NULL)
    	{
        	char *word = strsep(&parse, " \t\r\f\n");
        	if (strlen(word) != 0)
        	{
            		words[i++] = strdup(word);
        	}
    	}
    	*num_args = i;
    	for (; i<MAX_ARGS; ++i)
    	{
        	words[i] = NULL;
    	}

    	// all the words are in the array now, so free the original line
    	free(line);

    	return words;
}
void free_command(char **words)		//free memory
{
    	for (int i=0; i<MAX_ARGS; ++i)
    	{
        	if (words[i] == NULL)
        	{
            		break;
        	}
        	free(words[i]);
    	}
    	free(words);
}

int main()
{
    	size_t num_args;
    	// get the next command
    	char **command_line_words = get_next_command(&num_args);
	int background_flag = 0;
	int i = 0;
	node *head = new_list();
	while (command_line_words != NULL)
	{
		for(i = 0; i < num_args; i++)
		{
			//leave shell
			if(strcmp(command_line_words[i], "exit") == 0)
			{
				exit(0);
   			}
		}

		if (num_args == 0)
		{
			free(command_line_words);
			command_line_words = get_next_command(&num_args);
		    	continue;
		}	

		pid_t pid = fork();
	
		if (pid == -1)
		{
			perror("fork error");
			exit(1);
		}
		background_flag = 0;
		if (strcmp(command_line_words[num_args - 1], "&") == 0)
		{
			background_flag = 1;
			command_line_words[num_args - 1] = NULL;
		}
	
		if (pid == 0)
		{
			redirect(command_line_words, num_args);
			
			execvp(command_line_words[0], command_line_words);
			{
				perror("exec c");
			}
	//		redirect(command_line_words, num_args);
		}
		if (background_flag != 0)
		{
			add_node(head, pid);
		}
	
		if (waitpid(pid, NULL, 0) == -1)
		{
			perror("wait");
			exit(1);
		}
	
		free_command(command_line_words);
	
			// get the next command
		command_line_words = get_next_command(&num_args);
	
	}

	//list manipulation
	node *cur = head -> next;
	while(cur != head)
	{
		int value;
		pid_t pid = waitpid(cur -> pid, &value, WNOHANG);	//WNOHANG- parent does not wait on child processes
		if (pid == -1)
		{
			perror("waitpid");
			exit(1);
		}
		else if (pid > 0)
		{	
			pid_t tmp = cur -> pid;
			cur = cur -> next;
			remove_node(head, tmp);
		}
		cur = cur -> next;
	}
	    // free the memory for the last command
	    free_command(command_line_words);

    return 0;
}
