#ifndef DYNAMICSTRING_H
#define DYNAMICSTRING_H

#include <stdlib.h> 
#include <stddef.h> 

typedef struct dynamicstring{
	char* buf;  // 'buf' short for 'buffer'
				// Holds the individual characters of the string.
	size_t capacity; // How 'big' the buffer is, including the '\0' character
	size_t length;   // The length of the string, NOT including the '\0' character

}dynamicstring_t;

// Input is a legal NULL terminated C style string
dynamicstring_t* DynamicString_Create(const char* input);

// Free's the dynamic string and free's the underlying memory of the dynamicstring_t.
// The underlying memory should be freed first in 'buf'
// prior to freeing a heap allocated dynamicstring.
// Returns '1' on success, or 0 in failure.
int DynamicString_Free(dynamicstring_t* str);

// Retrieve the string length
// Operations on NULL dynamicstrings return a length of 0.
size_t DynamicString_GetLength(dynamicstring_t* str);

// Retrieve the string capacity
// Operations on NULL dynamicstrings return a capacity of 0.
size_t DynamicString_GetCapacity(dynamicstring_t* str);

// Get Character at a specific index
// Operations on NULL dynamicstrings return the null character.
char DynamicString_Get(dynamicstring_t* str, int index);

// Retrieve the string buffer
// Operations on NULL dynamicstrings return the null character.
const char* DynamicString_GetCString(dynamicstring_t* str);

// Set the string to a specific new string
// Operations on NULL dynamicstrings return the null character.
void DynamicString_Set(dynamicstring_t* str, char* input);

// Append a single 'char' to our dynamicstring_t.
// This function ensures that the final character is also the '\0' and the 'buf' is thus a null terminated string.
// returns '1' on success
// returns '0' on failure
//   - An example failure is an operation on a NULL 'str' which would return 0.
//   - An example failure would be if malloc or realloc cannot allocate memory
int DynamicString_AppendChar(dynamicstring_t* str, char c);

// Append a c-style string to our dynamicstring_t 
// Note: If the 'capacity' is not big enough to store the string,
//       than the underlying 'buf' must be reallocated.
// returns '1' on success
// returns '0' on failure
//   - An example failure is an operation on a NULL 'str' which would return 0.
//   - An example failure would be if malloc or realloc cannot allocate memory
int DynamicString_AppendDynamicString(dynamicstring_t* str, const char* input);

// Allocates a new 'dynamicstring' based from a substring of an existing string.
// The 'slice' or portion of the string is a new heap allocated dynamicstring.
// Note: Functionally this is different than D or Rust's slicing -- because we get a brand new dynamicstring in our 
//       implementation. This is for 'ease' of implementation so we do not have to worry about ownership.
//
// The new slice should always appends a null terminator at the end of the Slice to ensure 
// the new slice (i.e. new dynamicstring) is a valid null-terminated string.
// (i.e. make sure that 'buf' in dynamicstring_t is null-terminated)
//
// 'start' indicates the position to start and is included in the string
// 'end' is the last index, and first to not be included.
// i.e. [start,end)
// 'Testing' with a [0,2) would produce 'Te'
//
// returns a new dynamicstring_t on success.
// returns NULL if there is a failure.
//   - An example failure would be operating on a NULL 'str'
//   - An example failure would be operating on out of bounds start/end pairs
//   - An example failure would be operating on reversed start/end pairs (i.e. end > start).
dynamicstring_t* DynamicString_NewStringFromSlice(dynamicstring_t* str, int start, int end);

// Split a dynamicstring based on delimeters. 
//
// This is similar to the c string library function 'strtok', but otherwise produces
// an 'array of arrays' where each entry is a null-terminated dynamicstring_t.
//
// 'array' is a pointer to an array of dynamic strings. Think of this as a 'chunk' of memory 
// of 'size' continuous blocks of memory allocated for dynamicstring_t's.
//
// 'array' is an output parameter to an array of arrays, so we need an extra asterisk otherwise to 'modify' the
// parameter.
// 'size' is also an output paramater. It is simpler, in that we are only 'modifying' one value otherwise.
// 'size' represents the number of entries in our 'array' (i.e. how many 'tokens' or 'dynamicstrings' we have in our 'array'.
//
// You can think of the 'output paramters' 'array' and 'size' as a pair of paramaters that otherwise work together.
//
// Note:
//    Again observe with this function one thing that is different is that some of the function parameters are
//    'output' paramaters mixed with input parameters.
// Note:
//    You should not have any 'empty' values returned (i.e. delimeters alone should not
//    be in your returned array of array of dynamicstring_t's);
//
// Returns '0' if there is an error, and 1 on success
//   - An example failure would be operating on a NULL 'input'
//   - An example failure would be operating on a NULL 'delimeters'
int DynamicString_Split(dynamicstring_t* input, const char* delimeters, dynamicstring_t*** array, int* size);

#endif