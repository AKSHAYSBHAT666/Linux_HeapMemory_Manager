#include<stdio.h>
#include<memory.h>
#include<unistd.h>
#include<stdint.h>
#include<sys/mman.h>
#include "mm.h"
#include<assert.h>

#define ANSI_COLOR_BLACK "\e[1;30m"
#define ANSI_COLOR_RED "\e[1;31m"
#define ANSI_COLOR_GREEN "\e[1;32m"
#define ANSI_COLOR_YELLOW "\e[1;33m"
#define ANSI_COLOR_BLUE "\e[1;34m"
#define ANSI_COLOR_MAGENTA "\e[1;35m"
#define ANSI_COLOR_CYAN "\e[1;36m"
#define ANSI_COLOR_RESET "\e[0;37m"

static size_t SYSTEM_PAGE_SIZE = 0;
static vm_page_for_families_t *first_vm_page_for_families = NULL;

void mm_init()
{
	SYSTEM_PAGE_SIZE = getpagesize();	
};

static inline uint32_t mm_max_page_allocatable_memory(int units)
{
    return (uint32_t)((SYSTEM_PAGE_SIZE * units) - OFFSET_OF(vm_page_t, page_memory));
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
		first_vm_page_for_families->vm_page_family[0].first_page = NULL;
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
		first_vm_page_for_families->vm_page_family[0].first_page = NULL;
		init_glthread( &(first_vm_page_for_families->vm_page_family[0].free_block_priority_list_head) );	
		return;
	}
	vm_page_family_curr->struct_size = struct_size;
	strncpy( vm_page_family_curr->struct_name,struct_name,32 );
	vm_page_family_curr->first_page = NULL;
    /**/init_glthread( &(vm_page_family_curr->free_block_priority_list_head) );

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
		//printf("\n%p lookup\n",vm_page_family_curr);
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

void mm_add_free_block_meta_data_to_free_block_list( vm_page_family_t* vm_page_family, block_meta_data_t* free_block )
{

     glthread_priority_insert(&(vm_page_family->free_block_priority_list_head), &free_block->priority_thread_glue ,free_blocks_comparison_function ,OFFSET_OF(block_meta_data_t,priority_thread_glue));
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

static vm_bool_t mm_split_free_data_block_for_allocation( vm_page_family_t* vm_page_family, block_meta_data_t* block_meta_data, uint32_t size )
{	
	uint32_t curr_size          = block_meta_data->block_size;
	uint32_t remaining_size     = block_meta_data->block_size - size;
	block_meta_data->is_free    = MM_FALSE;
	block_meta_data->block_size = size;
	block_meta_data_t* next_block_meta_data = NULL;	
	
	remove_glthread(&block_meta_data->priority_thread_glue);/*detach from active list*/
	
	if(remaining_size < sizeof(block_meta_data_t) || remaining_size == 0) 
	{	
		return MM_TRUE;
	}
	else
	{
		next_block_meta_data = (block_meta_data_t*)(NEXT_META_BLOCK_BY_SIZE(block_meta_data));
		next_block_meta_data->block_size = remaining_size - sizeof(block_meta_data_t);
	        next_block_meta_data->is_free    = MM_TRUE;
		next_block_meta_data->offset     = block_meta_data->offset + size + sizeof(block_meta_data_t);
		
		init_glthread(&next_block_meta_data->priority_thread_glue);
		mm_add_free_block_meta_data_to_free_block_list( vm_page_family, next_block_meta_data );
        	mm_bind_blocks_for_allocation(block_meta_data, next_block_meta_data);
		
		return MM_TRUE;
	}
}

static block_meta_data_t* mm_allocate_free_data_block( vm_page_family_t* vm_page_family,uint32_t req_size )
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
	else
	{
		status  = mm_split_free_data_block_for_allocation( vm_page_family , biggest_block_meta_data , req_size );
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
		memset((char*)(free_block_meta_data+1),0,units*(vm_page_family->struct_size));/**/
		return (void*)(free_block_meta_data+1); 
	}
	printf("ERROR:allocate failed \n");
	return NULL;
}

void mm_print_vm_page_details(vm_page_t *vm_page)
{
   	
    printf("\t\t next = %p, prev = %p\n", vm_page->next, vm_page->prev);
    printf("\t\t page family = %s\n", vm_page->pg_family->struct_name);

    uint32_t j = 0;
    block_meta_data_t *curr;
    ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page, curr){

        printf("\t\t\t%-14p Block %-3u %s  block_size = %-6u  "
                "offset = %-6u  prev = %-14p  next = %p\n",
                curr,
                j++, curr->is_free ? "F R E E D" : "ALLOCATED",
                curr->block_size, curr->offset,
                curr->prev_block,
                curr->next_block);
    } ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page, curr);
}

void mm_print_block_usage()
{

    vm_page_t *vm_page_curr;
    vm_page_family_t *vm_page_family_curr;
    block_meta_data_t *block_meta_data_curr;
    uint32_t total_block_count, free_block_count, occupied_block_count;
    uint32_t application_memory_usage;

    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr){

        total_block_count = 0;
        free_block_count = 0;
        application_memory_usage = 0;
        occupied_block_count = 0;
        ITERATE_VM_PAGE_BEGIN(vm_page_family_curr, vm_page_curr){

            ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page_curr, block_meta_data_curr){

                total_block_count++;

                /* Sanity Checks */
                if(block_meta_data_curr->is_free == MM_FALSE){
                    assert(IS_GLTHREAD_LIST_EMPTY(&block_meta_data_curr->\
                                priority_thread_glue));
                }
                if(block_meta_data_curr->is_free == MM_TRUE){
                    assert(!IS_GLTHREAD_LIST_EMPTY(&block_meta_data_curr->\
                                priority_thread_glue));
                }

                if(block_meta_data_curr->is_free == MM_TRUE){
                    free_block_count++;
                }
                else{
                    application_memory_usage +=
                        block_meta_data_curr->block_size + \
                        sizeof(block_meta_data_t);
                    occupied_block_count++;
                }
            } ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page_curr, block_meta_data_curr);
        } ITERATE_VM_PAGE_END(vm_page_family_curr, vm_page_curr);

        printf("%-20s   TBC : %-4u    FBC : %-4u    OBC : %-4u AppMemUsage : %u\n",
                vm_page_family_curr->struct_name, total_block_count,
                free_block_count, occupied_block_count, application_memory_usage);

    } ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);

}


void mm_print_memory_usage(char *struct_name)
{
     uint32_t i = 0;
    vm_page_t *vm_page = NULL;
    vm_page_family_t *vm_page_family_curr;
    uint32_t number_of_struct_families = 0;
    uint32_t cumulative_vm_pages_claimed_from_kernel = 0;

    printf("\nPage Size = %zu Bytes\n", SYSTEM_PAGE_SIZE);

    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr){

        if(struct_name){
            if(strncmp(struct_name, vm_page_family_curr->struct_name,
                        strlen(vm_page_family_curr->struct_name))){
                continue;
            }
        }

        number_of_struct_families++;

        printf(ANSI_COLOR_GREEN "vm_page_family : %s, struct size = %u\n"
                ANSI_COLOR_RESET,
                vm_page_family_curr->struct_name,
                vm_page_family_curr->struct_size);
        i = 0;

        ITERATE_VM_PAGE_BEGIN(vm_page_family_curr, vm_page){

            cumulative_vm_pages_claimed_from_kernel++;
            mm_print_vm_page_details(vm_page);

        } ITERATE_VM_PAGE_END(vm_page_family_curr, vm_page);
        printf("\n");
    } ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr);

    printf(ANSI_COLOR_MAGENTA "# Of VM Pages in Use : %u (%lu Bytes)\n" \
            ANSI_COLOR_RESET,
            cumulative_vm_pages_claimed_from_kernel,
            SYSTEM_PAGE_SIZE * cumulative_vm_pages_claimed_from_kernel);

    float memory_app_use_to_total_memory_ratio = 0.0;

    printf("Total Memory being used by Memory Manager = %lu Bytes\n",
            cumulative_vm_pages_claimed_from_kernel * SYSTEM_PAGE_SIZE);
}
