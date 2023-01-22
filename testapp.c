#include<stdio.h>
#include<stdint.h>
#include "uapi_mm.h"

typedef struct emp_{
	char name[32];
	uint32_t emp_id;
}emp_t;

typedef struct student_{
	char name[32];
	uint32_t rollno;
	uint32_t marks_phy;
	uint32_t marks_chem;
	uint32_t marks_math;
	struct student_* next;
}student_t;

int main()
{
	mm_init();
	MM_REG_STRUCT(emp_t);
	MM_REG_STRUCT(student_t);
	mm_print_registered_page_families();

	emp_t *emp1 = XCALLOC(1, emp_t);
    	emp_t *emp2 = XCALLOC(1, emp_t);
    	emp_t *emp3 = XCALLOC(1, emp_t);

    	student_t *stud1 = XCALLOC(1, student_t);
    	student_t *stud2 = XCALLOC(1, student_t);
	
	emp_t *emp4 = XCALLOC(50,emp_t);
	emp_t *emp5 = XCALLOC(50,emp_t);
	emp_t *emp6 = XCALLOC(50,emp_t);
	emp_t *emp7 = XCALLOC(50,emp_t);
	printf(" \nSCENARIO 1 : *********** \n");
        mm_print_memory_usage(0);
        mm_print_block_usage();

	XFREE(emp4);
	XFREE(emp5);
	XFREE(emp6);
	XFREE(emp7);
	printf(" \nSCENARIO 2 : *********** \n");
        mm_print_memory_usage(0);
        mm_print_block_usage();

	return 0;
}
