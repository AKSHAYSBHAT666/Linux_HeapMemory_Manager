#include<stdint.h>

typedef enum{ MM_TRUE, MM_FALSE } vm_bool_t;
typedef struct vm_page_ vm_page_t;
typedef struct vm_page_family_ vm_page_family_t;


typedef struct block_meta_data_
{
        vm_bool_t is_free;
        uint32_t  block_size;
        uint32_t  offset;
        struct block_meta_data_ *next_block;
        struct block_meta_data_ *prev_block;
}block_meta_data_t;

struct vm_page_{
        struct vm_page_ *next;
        struct vm_page_ *prev;
/*line*/vm_page_family_t *pg_family;
        block_meta_data_t block_meta_data;
        char page_memory[0];
};

struct vm_page_family_
{
	char struct_name[32];
	uint32_t struct_size;
	vm_page_t *first_page;
};

typedef struct vm_page_for_families_
{
	struct vm_page_for_families_ *next;
	vm_page_family_t vm_page_family[0];
}vm_page_for_families_t;

vm_bool_t mm_is_vm_page_empty( vm_page_t *vm_page );
vm_page_t *allocate_vm_page( vm_page_family_t *vm_page_family );
void mm_vm_page_delete_and_free( vm_page_t *vm_page );

#define OFFSET_OF(struct_type,field_name)       \
        (long)&(((struct_type*)0)->field_name)

#define MM_GET_PAGE_FROM_META_BLOCK( block_meta_data_ptr )      \
        (void*)((char*)block_meta_data_ptr-(block_meta_data_ptr->offset))

#define NEXT_META_BLOCK( block_meta_data_ptr )  \
        block_meta_data_ptr->next_block

#define PREV_META_BLOCK( block_meta_data_ptr )  \
        block_meta_data_ptr->prev_block

#define NEXT_META_BLOCK_BY_SIZE( block_meta_data_ptr )  \
        (void*)( (char*)block_meta_data_ptr + block_meta_data_ptr->block_size + block_meta_data_ptr->offset )

#define mm_bind_blocks_for_allocation(allocated_meta_block, free_meta_block)    \
        free_meta_block->next_block = allocated_meta_block->next_block;         \
        allocated_meta_block->next_block = free_meta_block;                     \
        free_meta_block->prev_block = allocated_meta_block;                     \
        if(free_meta_block->next_block)                                         \
        free_meta_block->next_block->prev_block = free_meta_block;      

#define MARK_VM_PAGE_EMPTY( vm_page_t_ptr )                     \
        vm_page_t_ptr->block_meta_data.is_free = MM_TRUE;       \
        vm_page_t_ptr->block_meta_data.next_block = NULL;       \
        vm_page_t_ptr->block_meta_data.prev_block = NULL;               

#define MAX_FAMILIES_PER_VM_PAGE					\
	(SYSTEM_PAGE_SIZE-sizeof(struct vm_page_for_families_t*))/	\
	sizeof(vm_page_family_t) 

#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr,curr) 			\
{											\
	uint32_t _count = 0;								\
	for( curr = (vm_page_family_t*)&(vm_page_for_families_ptr->vm_page_family[0]); 	\
	_count < MAX_FAMILIES_PER_VM_PAGE && (curr->struct_size) ;			\
	_count++, curr++ ){						
#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr,curr)   }}

#define ITERATE_VM_PAGE_BEGIN( vm_page_family_ptr,curr )			\
{	vm_page_t *next;							\
	for( curr = vm_page_family_ptr->first_page;curr!=NULL;curr=next ){	\
	next = curr->next;							
#define ITERATE_VM_PAGE_END(vm_page_family_ptr,curr)  }}

#define ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page_ptr,curr)			\
{	block_meta_data_t *next;						\
	for( curr = &vm_page_ptr->block_meta_data;curr!=NULL;curr=next ){	\
	next = curr->next_block;
#define ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page_ptr,curr)	}}		


