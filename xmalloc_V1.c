#include<unistd.h>
#include<stdio.h>
#include<memory.h>
#include<assert.h>

void* xmalloc(int size)
{
	void*p= sbrk(0);
	
	if(sbrk(size)==NULL)
	{
		return NULL;
	}
	return p;
}

void xfree(int nbyte)
{
	assert(nbyte>0);
	sbrk(nbyte*(-1));
}

int main()
{	
	char*ptr=xmalloc(15);
	strncpy(ptr, "This is tutorialspoint.com\n",15);
	printf("%s",ptr);
	return 0;
}
