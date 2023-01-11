#include<stdio.h>
#include<memory.h>
#include<unistd.h>
#include<stdint.h>
#include<sys/mman.h>
#include "mm.h"

static size_t SYSTEM_PAGE_SIZE = 0;
static vm_page_for_families_t *first_vm_page_for_families = NULL;

void mm_init()
{
	SYSTEM_PAGE_SIZE = getpagesize();	
};


/*Function to GET VM PAGE from KERNEL*/
static void* mm_get_new_vm_page_from_kernel(int units)
{
	char* vm_page= mmap
		(0 , 
		units*SYSTEM_PAGE_SIZE , 
		PROT_READ | PROT_WRITE | PROT_EXEC ,
		MAP_ANON | MAP_PRIVATE,
		0 ,
		0);
	
	if( vm_page == MAP_FAILED )
	{
		printf("ERROR: Can't Request VM PAGE\n");
		return NULL;
	}
	else
	{
		memset( vm_page , 0 , units*SYSTEM_PAGE_SIZE );
		return (void*)vm_page;
	}
};

static void mm_return_vm_page_to_kernel(void *vm_page , int units)
{
	munmap( vm_page , units*SYSTEM_PAGE_SIZE );
};

void mm_instantiate_new_page_family( char *struct_name , uint32_t struct_size )
{
	vm_page_family_t *vm_page_family_curr = NULL;
       	vm_page_for_families_t *new_vm_page_for_families = first_vm_page_for_families;

	if( !first_vm_page_for_families )
	{
		first_vm_page_for_families = (vm_page_for_families_t*)mm_get_new_vm_page_from_kernel(1);
		new_vm_page_for_families = first_vm_page_for_families;
		first_vm_page_for_families -> next = NULL;

		// now insert the records
		first_vm_page_for_families->vm_page_family[0].struct_size = struct_size;
		strncpy( first_vm_page_for_families->vm_page_family[0].struct_name ,struct_name , 32);
		
		return;
	}

	uint32_t count = 0;
	
	ITERATE_PAGE_FAMILIES_BEGIN(new_vm_page_for_families , vm_page_family_curr)
	{
		count++;
	}ITERATE_PAGE_FAMILIES_END(new_vm_page_for_families , vm_page_family_curr);

	if( count == MAX_FAMILIES_PER_VM_PAGE )
	{
		new_vm_page_for_families = (vm_page_for_families_t*)mm_get_new_vm_page_from_kernel(1);
	        new_vm_page_for_families->next = first_vm_page_for_families;
		first_vm_page_for_families = new_vm_page_for_families;
		
		//insert records
		first_vm_page_for_families->vm_page_family[0].struct_size = struct_size;
                strncpy( first_vm_page_for_families->vm_page_family[0].struct_name,struct_name, 32);
		
		return;
	}
	vm_page_family_curr->struct_size = struct_size;
	strncpy( vm_page_family_curr->struct_name,struct_name,32 );

	return;
}

void mm_print_registered_page_families()
{
	vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *vm_page_for_families_curr = NULL;

    for(vm_page_for_families_curr = first_vm_page_for_families;
            vm_page_for_families_curr;
            vm_page_for_families_curr = vm_page_for_families_curr->next){

        ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_curr,
                vm_page_family_curr){

            printf("Page Family : %s, Size = %u\n",
                    vm_page_family_curr->struct_name,
                    vm_page_family_curr->struct_size);

        } ITERATE_PAGE_FAMILIES_END(vm_page_for_families_curr,
                vm_page_family_curr);
    }
	return;
}

static void mm_union_free_blocks(block_meta_data_t *first, block_meta_data_t *second)
{
        if(second==NULL) return;
        else
        {
                if(second->next_block)
                {
                        first->next_block = second->next_block;
                        second->next_block->prev_block = first;

                        first->block_size = first->block_size + second->block_size + sizeof(block_meta_data_t);
                        return;
                }
                else
                {
                        /**/
                        first->next_block = NULL;

                        first->block_size = first->block_size + second->block_size + sizeof(block_meta_data_t);
                        return;
                }
        }
}

vm_bool_t mm_is_vm_page_empty( vm_page_t *vm_page )
{
	if( vm_page->block_meta_data.is_free && vm_page->block_meta_data.next_block==NULL && vm_page->block_meta_data.prev_block==NULL )
	{
		return MM_TRUE;
	}
	else
	{
		return MM_FALSE;
	}
}

static inline uint32_t mm_max_page_allocatable_memory(int units)
{
	return (uint32_t)((SYSTEM_PAGE_SIZE*units)-OFFSET_OF(vm_page_t,page_memory));
}

vm_page_t *allocate_vm_page( vm_page_family_t *vm_page_family )
{
	if( vm_page_family->first_page == NULL )
	{
		vm_page_family->first_page = (vm_page_t*)mm_get_new_vm_page_from_kernel(1);
	        vm_page_family->first_page->next = NULL;
		vm_page_family->first_page->prev = NULL;
		vm_page_family->first_page->pg_family = vm_page_family;
		MARK_VM_PAGE_EMPTY( vm_page_family->first_page );	
	}
	else
	{
		vm_page_t *temp = (vm_page_t*)mm_get_new_vm_page_from_kernel(1);
		temp->next = vm_page_family->first_page;
                temp->prev = NULL;
		temp->pg_family = vm_page_family;
		vm_page_family->first_page->prev = temp;
		vm_page_family->first_page = temp;
		MARK_VM_PAGE_EMPTY( vm_page_family->first_page );
	}

	vm_page_family->first_page->block_meta_data.is_free = MM_TRUE; /**/
	vm_page_family->first_page->block_meta_data.offset = OFFSET_OF(vm_page_t,block_meta_data);
	vm_page_family->first_page->block_meta_data.block_size = mm_max_page_allocatable_memory(1);
	return vm_page_family->first_page;
}

void mm_vm_page_delete_and_free( vm_page_t *vm_page )
{
	vm_page_family_t *vm_page_family = vm_page->pg_family;

    	/*If the page being deleted is the head of the linked list */
    	if(vm_page_family->first_page == vm_page)
    	{
        	vm_page_family->first_page = vm_page->next;
        	if(vm_page->next) vm_page->next->prev = NULL;
        	vm_page->next = NULL;
        	vm_page->prev = NULL;
        	mm_return_vm_page_to_kernel((void *)vm_page, 1);
        	return;
    	}

   	 /*If we are deleting the page from middle or end of linked list*/
   	if(vm_page->next) vm_page->next->prev = vm_page->prev;
    	vm_page->prev->next = vm_page->next;
   	mm_return_vm_page_to_kernel((void *)vm_page, 1);
}

