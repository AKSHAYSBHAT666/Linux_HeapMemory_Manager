
# Linux Heap Memory Allocator

This is a custom memory allocator which has api's written in C for linux and related systems.
One can include the repo contents as a library into your C projects and use it for custom memory allocation. The implementation is done in a way as to remove chances of MEMORY LEAKS, as this allocator gives LIVE STATS about memory which is allocated and what is freed etc.

The idea is to minimize memory fragmentation and utilise as many of existing freed blocks providing better performance and tracking ability of objects.

using mmap() to map a file into memory (MAP_FILE or equivalent), the pages can be backed by the file content. In this case, the kernel typically allocates pages to represent the file's content. Modifications to this memory can be written back to the file.





## API Reference

#### XCALLOC

```http
  GET memory for any object 
  Usage : void* ptr  = XCALLOC( 'number of objects', 'object type');
```


#### mm_print_memory_usage

```http
  Check memory usage PER STRUCT/OBJECT
  Usage : mm_print_memory_usage(char* name_to_see_usage);
```
#### XFREE

```http
  GET memory for any object 
  Usage : XFREE('pointer to object');
```

#### mm_print_registered_page_families

```http
  GET all the objects/struct names for which memory is used.
  Usage : mm_print_registered_page_families();
```
#### mm_print_memory_usage

```http
  Check memory usage PER STRUCT/OBJECT
  Usage : mm_print_memory_usage(char* name);

  ```
#### mm_print_block_usage

```http
  GET the status with count of blocks in each VM page.
  Usage : mm_print_memory_usage();
```




## Compilation
1) clone the git repo into your folder.
2) navigate to the folder and build the system with your respective files.

ex: to compile the testapp.c in the repo,
```http
  gcc glthreads/glthread.c mm.c testapp.c -o exe
```
3. use the standard GCC distributions.


## Sample Output( testapp.c )

![App Screenshot](https://github.com/AKSHAYSBHAT666/linux_memory_manager_output/blob/main/Screenshot(197).jpg?raw=true)

