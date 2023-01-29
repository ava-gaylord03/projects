#define _XOPEN_SOURCE 500 // needed for sbrk() on cslab

#include <unistd.h>

typedef struct freenode
{
	size_t size;
    	struct freenode *next;
} freenode;

#define HEAP_CHUNK_SIZE 4096
#define MIN_CHUNK_SIZE 32

// head node for the freelist
freenode *freelist = NULL;
freenode * split(freenode *node, size_t size);

/* allocate size bytes from the heap */
void *malloc(size_t size)
{
    // can't have less than 1 byte requested
	if (size < 1)
    	{
		return NULL;
    	}

    // add 8 bytes for bookkeeping
    	size += 8;

    // 32 bytes is the minimum allocation
    	if (size < 32)
    	{
    		size = 32;
   	}

    // round up to the nearest 16-byte multiple
	else if (size%16 != 0)
    	{
    		size = ((size/16)+1)*16;
    	}	

	// if we have no memory, grab one chunk to start
	if (freelist == NULL)
	{	
		void *tmp = sbrk(HEAP_CHUNK_SIZE);
		if (tmp == (void *)-1)
		{
			return NULL;
		}

		//skip first 8 bytes for bookkeeping
		tmp += 8;
		freelist = (freenode *)tmp;
		freelist -> size = HEAP_CHUNK_SIZE - 8;
		freelist -> next = NULL;
	}
	
	freenode *cur = freelist;
	freenode *previous;
    // look for a freenode that's large enough for this request
	
	while (cur != NULL)
	{
		if (cur -> size >= size)
		{
			break;
		}
		previous = cur;
		cur = cur -> next;
	}
    // have to track the previous node also for list manipulation later
    // if there is no freenode that's large enough, allocate more memory
	
	if (cur == NULL)	//second case
	{
		size_t needed_bytes = size;
		int total_new_bytes = ((needed_bytes / HEAP_CHUNK_SIZE) + 1) * HEAP_CHUNK_SIZE;
		void *tmp = sbrk(total_new_bytes);
		
		if (tmp == (void *) - 1)
		{
			return NULL;
		}

		freenode *new_node = (freenode *)(tmp + 8);
		new_node -> size = total_new_bytes - 8;
		new_node -> next = NULL;
		previous -> next = new_node;		
		cur = new_node;
	}

	if (cur -> size >= size + MIN_CHUNK_SIZE)
	{
		previous = cur;	
		freenode *save_chunk = split(cur, size);
		
		if (cur != freelist)
		{
			previous -> next = save_chunk;
		}

		else
		{
			freelist = save_chunk;
		}

		cur -> next = NULL;
		cur -> size = size;
	}	

	else 
	{	
		if(cur != freelist)
		{
			previous -> next = cur -> next;
		}

		else
		{
			freelist = cur -> next;
		}
	}
    
	void *ptr_to_usr;
	ptr_to_usr = (void *)cur + 8;	
	return ptr_to_usr;
}

freenode * split(freenode *node, size_t size)		//To split a node
{
	void *tmp = (void *)node;
	freenode *new_node;
	new_node = (freenode*)(tmp + size);
	new_node -> size = node -> size - size;
	new_node -> next = node -> next;
	return new_node;
}

/* return a previously allocated memory chunk to the allocator */
void free(void *ptr)
{
	if (ptr == NULL)
    	{
    		return;
    	}

    // make a new freenode starting 8 bytes before the provided address
    	freenode *new_node = (freenode *)(ptr-8);

    // the size is already in memory at the right location (ptr-8)

    // add this memory chunk back to the beginning of the freelist
    	new_node->next = freelist;
    	freelist = new_node;

    	return;
}
