#include<unistd.h>
#include<stdio.h>
#include<memory.h>
#include<assert.h>

struct metablock_t{
	int size;
	struct metablock_t* next;
	int isfree;
};

void* xmalloc(int nbytes)
{
	struct metablock_t *ptr=sbrk(0);
	
	if(sbrk(sizeof(struct metablock_t)+nbytes)==NULL)
		return NULL;
		
	ptr->size=nbytes;
	ptr->next=NULL;
	ptr->isfree=0;
	
	return ptr+1;
}


int main()
{
	return 0;
}
