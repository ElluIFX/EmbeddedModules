# About the project
This is the custom implementation of function malloc for embedded systems, that defragmentate memory after using it. Good solution when you need to allocate memory dynamically, but memory fragmentation is the problem. On this allocator based such project as [uvector](https://github.com/SkyEng1neering/uvector), [ustring](https://github.com/SkyEng1neering/ustring) and [usmartpointer](https://github.com/SkyEng1neering/usmartpointer).

## What is the problem that [dalloc](https://github.com/SkyEng1neering/dalloc) solves?
### Here is an illustration of memory fragmentation problem, while using default malloc function:    
![this](https://github.com/SkyEng1neering/files/blob/main/default_malloc.drawio%20(1).png)
On the last step there are 7 kB of free memory but not in solid region, so allocating of 4 kB fails
<br></br>

### And how dalloc works:
![this](https://github.com/SkyEng1neering/files/blob/main/defrag_malloc.drawio.png)

## How it works
When you call allocate function, it saves the pointer to the pointer of allocated memory area and when corresponding memory region is freed - the defragmentation procedure executes, an the content of pointer to the memory area is replaced by new actual value. So using of dalloc allocator is not the same as default malloc.

# Usage
## Simple examples of using dalloc
In your project you can use only one heap area, and several heap areas as well. 
If you want to use only one heap memory area in your project, you can do it like this:
```c++
/* File dalloc_conf.h */
#define USE_SINGLE_HEAP_MEMORY
#define SINGLE_HEAP_SIZE				4096UL //define heap size that you want to have
```

```c++
#include "dalloc.h"

int main(){
  /* Allocate memory region in heap memory */
  uint8_t *data_ptr = NULL;
  uint32_t allocation_size = 8;
  def_dalloc(allocation_size, (void**)&data_ptr);

  /* Check if allocation is successful */
  if(data_ptr == NULL){
    printf("Can't allocate %u bytes\n", allocation_size);
    return 0;
  }
 
  /* Now you can use allocated memory */
  for(uint32_t i = 0; i < allocation_size; i++){
    data_ptr[i] = 0xDE;
  }
 
  /* Free allocated memory */
  def_dfree((void**)&data_ptr);
 
  return 0;
}  
  
```

If you want to use several different heap areas, you can define it explicitly:

```c++
/* File dalloc_conf.h */
//#define USE_SINGLE_HEAP_MEMORY
```

```c++
#include "dalloc.h"

#define HEAP_SIZE			32

/* Declare an array that will be used for dynamic memory allocations */
uint8_t heap_array[HEAP_SIZE];

/* Declare an dalloc heap structure, it contains all allocations info */
heap_t heap;

int main(){
  /* Init heap memory */
  heap_init(&heap, (void*)heap_array, HEAP_SIZE);

  /* Allocate memory region in heap memory */
  uint8_t *data_ptr = NULL;
  uint32_t allocation_size = 8;
  dalloc(&heap, allocation_size, (void**)&data_ptr);

  /* Check if allocation is successful */
  if(data_ptr == NULL){
    printf("Can't allocate %u bytes\n", allocation_size);
    return 0;
  }
 
  /* Now you can use allocated memory */
  for(uint32_t i = 0; i < allocation_size; i++){
    data_ptr[i] = 0xDE;
  }
 
  /* Free allocated memory */
  dfree(&heap, (void**)&data_ptr, USING_PTR_ADDRESS);
 
  return 0;
}  
  
```
## Limitations
As the address of pointer variable being saved and the value of the pointer variable is being updated when defragmentation is running - the pointer that was passed to **dalloc** function must exist before dfree function call, so you **can't** do something like this:

```c++
uint8_t* func_that_use_dalloc(heap_t *heap_ptr, uint32_t some_data_len){
  uint8_t* data_ptr = NULL;
  dalloc(heap, some_data_len, (void**)&data_ptr);
  return data_ptr;
}
```
Because when you call this function - the memory is allocated, but after this the pointer variable **data_ptr** which address that saves in heap_t structure isn't exist. So when you call **dfree** function, when defragmentation start you can get hard fault exception.

So the pointer which contains allocated memory address should exist in memory after allocating procedure. You can do this for example:

```c++
uint8_t* data_ptr = NULL;//data_ptr is just global variable

bool func_that_use_dalloc(heap_t *heap_ptr, uint32_t some_data_len){
  dalloc(heap, some_data_len, (void**)&data_ptr);
  
  /* Check if allocation is successful */
  if(data_ptr == NULL){
    return false;
  }
  return true;
}
```

Or this:

```c++
uint8_t* data_ptr = NULL;//data_ptr is the variable somewhere in your code

bool func_that_use_dalloc(heap_t *heap_ptr, uint8_t** data_ptr_address, uint32_t some_data_len){
  dalloc(heap, some_data_len, (void**)data_ptr_address);
  
  /* Check if allocation is successful */
  if(*data_ptr_address == NULL){
    return false;
  }
  return true;
}

func_that_use_dalloc(&heap, &data_ptr, some_data_len);
```

### If you need to allocate memory in some function, and return pointer to allocated memory from it - just use [usmartpointer](https://github.com/SkyEng1neering/usmartpointer), this is an c++ wrapper for dalloc, and you can use it more comfortable:

```c++
SmartPointer<uint8_t> func_that_use_dalloc(heap_t *heap_ptr, uint32_t some_data_len){
  SmartPointer<uint8_t> ptr;
  ptr.assignAllocMemPointer(heap_ptr);
  ptr.allocate(some_data_len);
  
  /* Maybe some logic */
  
  return ptr;
}
```
All memory moments here is managed automatically
