#include<unistd.h>
#include<stdio.h>
#include<memory.h>

void* xmalloc(int size)
{
	void*p= sbrk(0);
	printf("\n%p\n",p);
	
	if(sbrk(size)==NULL)
	{
		return NULL;
	}
	p=sbrk(0);
	printf("\n%p\n",p);
	return p;
}

int main()
{	
	char*ptr=xmalloc(15);
	strncpy(ptr, "This is tutorialspoint.com\n",15);
	printf("%s",ptr);
	return 0;
}
