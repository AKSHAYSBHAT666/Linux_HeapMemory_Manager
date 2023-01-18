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

static inline uint32_t mm_max_page_allocatable_memory(int units)
{
    return (uint32_t)((SYSTEM_PAGE_SIZE * units) - offset_of(vm_page_t, page_memory));
}

#define MAX_PAGE_ALLOCATABLE_MEMORY(units) \
    (mm_max_page_allocatable_memory(units))

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

static int mm_get_hard_internal_memory_frag_size( block_meta_data_t *first,block_meta_data_t *second )
{
    block_meta_data_t *next_block = NEXT_META_BLOCK_BY_SIZE(first);
    return (int)((unsigned long)second - (unsigned long)(next_block));
}


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
		init_glthread( &(first_vm_page_for_families->vm_page_family[0].free_block_priority_list_head) );
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
		init_glthread( &(first_vm_page_for_families->vm_page_family[0].free_block_priority_list_head) );	
		return;
	}
	vm_page_family_curr->struct_size = struct_size;
	strncpy( vm_page_family_curr->struct_name,struct_name,32 );
    /**/init_glthread( &(first_vm_page_for_families->vm_page_family[count].free_block_priority_list_head) );

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

vm_page_family_t* lookup_page_family_by_name(char *struct_name)
{
    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *vm_page_for_families_curr = NULL;

    for(vm_page_for_families_curr = first_vm_page_for_families;
            vm_page_for_families_curr;
            vm_page_for_families_curr = vm_page_for_families_curr->next){

        ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_curr, vm_page_family_curr){

            if(strncmp(vm_page_family_curr->struct_name,
                        struct_name,
                        MM_MAX_STRUCT_NAME) == 0){

                return vm_page_family_curr;
            }
        } ITERATE_PAGE_FAMILIES_END(vm_page_for_families_curr, vm_page_family_curr);
    }
    return NULL;
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
/*
static inline uint32_t mm_max_page_allocatable_memory(int units)
{
	return (uint32_t)((SYSTEM_PAGE_SIZE*units)-OFFSET_OF(vm_page_t,page_memory));
}*/

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
	init_glthread( &(vm_page_family->first_page->block_meta_data.priority_thread_glue) );
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

static int free_blocks_comparison_function( void* _block_meta_data1 , void* _block_meta_data2 )
{
	block_meta_data_t* block1 = (block_meta_data_t*)_block_meta_data1;
	block_meta_data_t* block2 = (block_meta_data_t*)_block_meta_data2;

	if( block1->block_size > block2->block_size )
	{
		return -1;
	}
	else if( block1->block_size < block2->block_size )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static vm_page_t* mm_family_new_page_add(vm_page_family_t *vm_page_family)
{
    vm_page_t *vm_page = allocate_vm_page(vm_page_family);

    if(!vm_page)
        return NULL;

    /* The new page is like one free block, add it to the
     * free block list*/
    mm_add_free_block_meta_data_to_free_block_list( vm_page_family, &vm_page->block_meta_data );

    return vm_page;
}

static void mm_add_free_block_meta_data_to_free_block_list( vm_page_family_t* vm_page_family, block_meta_data_t* free_block )
{
     glthread_priority_insert(&(vm_page_family->free_block_priority_list_head), free_block->priority_thread_glue ,free_blocks_comparison ,OFFSET_OF(block_meta_data_t,priority_thread_glue));
}



static block_meta_data_t* mm_allocate_free_meta_data( vm_page_family_t* vm_page_family,uint32_t req_size )
{
	vm_page_t* vm_page = NULL;
	vm_bool_t  status  = MM_FALSE;
	block_meta_data_t* block_meta_data = NULL;
	block_meta_data_t* biggest_block_meta_data = mm_get_biggest_free_block_page_family( vm_page_family );
	
	if( !biggest_block_meta_data || biggest_block_meta_data->block_size < req_size )
	{
		vm_page = mm_family_new_page_add(vm_page_family);
		status  = mm_split_free_data_block_for_allocation( vm_page_family , &vm_page->block_meta_data , req_size );
		
		if(!status) return NULL;
		else 	    return &vm_page->block_meta_data;
	}	
	else if( biggest_block_meta_data )
	{
		status  = mm_split_free_data_block_for_allocation( vm_page_family , &vm_page->block_meta_data , req_size );

                if(!status) return NULL;
                else        return biggest_block_meta_data;
	}
}

void* xcalloc( char* struct_name , int units )
{
	vm_page_family_t* vm_page_family = lookup_page_family_by_name(struct_name);
	if( vm_page_family == NULL ) {printf("ERROR: Page family Not registered\n");return NULL;}
	
	if( units*(vm_page_family->struct_size ) > MAX_PAGE_ALLOCATABLE_MEMORY(1)) {printf("EROOR:exceeds limit's\n");return NULL;}	
	/*Find the Page which can fulfill request*/
	block_meta_data_t* free_block_meta_data = NULL;
	free_block_meta_data = mm_allocate_free_data_block( vm_page_family,units*(vm_page_family->struct_size) );

	if(free_block_meta_data)
	{
		memset((char*)(free_block_meta_data+1),0, vm_page_family,units*(vm_page_family->struct_size));/**/
		(void*)(free_block_meta_data+1); 
	}
	printf("ERROR:allocate failed \n");
	return NULL;
}

