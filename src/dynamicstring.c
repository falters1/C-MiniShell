#include "dynamicstring.h"
#include <stdint.h>
// NOTE:
//      You are not allowed to include <string.h> in this file.

//  #include <stdio.h>
// You should not include output in your final version
// from these functions. It may be useful while debugging
// however to print out messages.
// Remember --your GDB however, it's a good idea to use a debugger!

dynamicstring_t* DynamicString_Create(const char* input){
	if (input == NULL){
		return NULL; //if input null do we treat as "" or NULL
	}
	size_t len = 0;
	const char *temp = input;
	while(*temp != '\0'){
		len++;
		temp++;
	}

	dynamicstring_t* str = malloc(sizeof(*str));
	if (str == NULL){
		return NULL;
	}

	str->buf = malloc(sizeof(char) * (len + 1));
	if (str->buf ==NULL){
		free(str);
		return NULL;
	}

	const char* temp2 = input;
	char *buffer = str->buf;
	for (size_t i = 0; i < len + 1; i++){
		buffer[i] = temp2[i];
	}

	str->length = len;
	str->capacity = len+1;

	return str;
}

// Free's the dynamic string and free's the underlying memory of the dynamicstring_t.
int DynamicString_Free(dynamicstring_t* str){
	if (str == NULL){
		return 0;
	}
	if (str->buf == NULL){
		return 0;
	}
	free(str->buf);
	free(str);

	return 1;
}

// Retrieve the string length
size_t DynamicString_GetLength(dynamicstring_t* str){
	if (str == NULL){
		return 0; //look into this maybe we wnat to change?
	}
	return str->length;
}

// Retrieve the string capacity
size_t DynamicString_GetCapacity(dynamicstring_t* str){
	if (str == NULL){
		return 0; //look into this maybe we want to change?
	}
	return str->capacity;
}

// Get Character at a specific index
char DynamicString_Get(dynamicstring_t* str, int index){
	if (str == NULL){
		return '\0';
	}
	if (index >= str->length){
		return '\0'; //is this what its supposed to return?
	}
	if (index < 0){
		return '\0';
	}
	return str->buf[index];
}

// Retrieve the string buffer
const char* DynamicString_GetCString(dynamicstring_t* str){
	if (str == NULL){
		return NULL; //maybe debug later
	}
	return str->buf;
}

// Set the string to a specific new string
void DynamicString_Set(dynamicstring_t* str, char* input){
	if (input == NULL){
		return;
	}
	if (str == NULL){
		return;
	}


	size_t len = 0;
	char *temp = input;
	while(*temp != '\0'){
		len++;
		temp++;
	}

	size_t prevCapacity;
	prevCapacity = DynamicString_GetCapacity(str);
	
	if (len + 1 > prevCapacity){
		char *tmp = malloc(sizeof(char) * (len+1));
		if (tmp == NULL){
			return;
		}
		for (size_t i = 0; i < len + 1; i++){
			tmp[i] = input[i];
		}
		free(str->buf);
		str->capacity = len + 1;
		str->buf = tmp;
		str->length = len;
		
		return;
	}

	for (size_t i = 0; i < len + 1; i++){
		str->buf[i] = input[i];
	}
	str->length = len;
	return;

}

// Append a single character to our dynamic string
int DynamicString_AppendChar(dynamicstring_t* str, char c){
	int result = 1; // Indicating success
	if (str == NULL){
		return 0;
	}
	if (c == '\0'){
		return result;
	}

	size_t len = DynamicString_GetLength(str);
	size_t capacity = DynamicString_GetCapacity(str);
	if (len + 1 >= capacity){
		size_t newCapacity = capacity;
		if (newCapacity != 0){
			newCapacity *=  2;
		}
		else{
			newCapacity = 8;
		}

		while (newCapacity <= len + 1){
			if (newCapacity > (SIZE_MAX / 2)){
				newCapacity = len+2;
				break;
			}
			newCapacity *= 2;
		}


		char *temp = realloc(str->buf, newCapacity);
		if (temp == NULL){
			return 0;
		}
		str->buf = temp;
		str->capacity = newCapacity;

	}

	str->buf[len] = c;
	str->length++;
	str->buf[len+1] = '\0';
	
	return result;
}

// Append a c-style string to our dynamic string
int DynamicString_AppendDynamicString(dynamicstring_t* str, const char* input){
	int result = 1; // Indicating success

	if (str == NULL){
		return 0;
	}
	if (input == NULL){
		return 1;
	}

	size_t lenStr = DynamicString_GetLength(str);
	size_t capacity = DynamicString_GetCapacity(str);
	size_t lenInput = 0;
	const char *temp = input;
	while(*temp != '\0'){
		lenInput++;
		temp++;
	}
	size_t requiredSize = lenInput + lenStr + 1;
	size_t newCapacity = capacity;
	if (capacity <= requiredSize){
		if (capacity == 0){
			newCapacity = lenInput + 1;
		}
		while (newCapacity < requiredSize){
			if (newCapacity > (SIZE_MAX / 2)){
				newCapacity = requiredSize + 1;
				break;
			}
			newCapacity *= 2;
		}


		char *tmp = realloc(str->buf, newCapacity);
		if (tmp == NULL){
			return 0;
		}
		str->buf = tmp;
		str->capacity = newCapacity;

	}
	size_t i = 0;
	for (i = 0; i < lenInput; i++){
		str->buf[i + lenStr] = input[i];
	}
	str->buf[i+lenStr] = '\0';
	str->length = lenStr + i;



	return result;
}

// Allocates a new 'string'
dynamicstring_t* DynamicString_NewStringFromSlice(dynamicstring_t* str, int start, int end){
	if (start > end){
		return NULL;
	}

	if (start < 0){
		return NULL;
	}
	size_t len = DynamicString_GetLength(str);
	if (end > len){
		return NULL;
	}

	size_t newLength = (size_t) (end - start);
	char* temp = malloc(newLength + 1);
	for (size_t i = 0; i < newLength; i++){
		temp[i] = str->buf[start + i];
	}
	temp[newLength] = '\0';
	dynamicstring_t* result = DynamicString_Create(temp);
	free(temp);

	return result;

}

int isDelimeter(char ch, const char* delimeters){
	if (delimeters == NULL){
		return 0;
	}
	const char *d = delimeters;
	while (*d != '\0'){
		if (*d == ch){
			return 1;
		}
		d++;
	}
	return 0;
}

void freeArray(dynamicstring_t** array, size_t count){
	if (array == NULL){
		return;
	}
	for (size_t i = 0; i < count; i++){
		if (array[i] != NULL){
			DynamicString_Free(array[i]);
		}
	}
	free(array);
}


// Split a dynamic string into multiple dynamic strings that are returned in the output parameter.
int DynamicString_Split(dynamicstring_t* input, const char* delimeters, dynamicstring_t*** array, int* size){
	int result = 1; // Indicating success


	if (array == NULL || size == NULL || input == NULL || delimeters == NULL){
		return 0;
	}
	*array = NULL;
	*size = 0;

	const char* buffer = DynamicString_GetCString(input);
	if (buffer == NULL){
		return 0;
	}
	size_t len = DynamicString_GetLength(input);
	if (len == 0){
		*array = NULL;
		*size = 0;
		return 1;
	}

	size_t capacity = 8;
	dynamicstring_t **res = malloc(sizeof(dynamicstring_t) * capacity);
	if (res == NULL){
		return 0;
	}
	size_t outputCount = 0;

	size_t i = 0;
	while (i < len){
		while (i < len && (isDelimeter(buffer[i], delimeters) == 1)){
			i++;
		}
		if (i >= len){
			break;
		}

		size_t start = i;
		while ( i < len && !(isDelimeter(buffer[i], delimeters) == 1)){
			i++;
		}
		size_t end = i;
		dynamicstring_t *token = DynamicString_NewStringFromSlice(input, start, end); 
		if (token == NULL){ //might have to free some memory
			freeArray(res, outputCount);
			return 0;
		}

	
		if (outputCount == capacity){
			size_t newCapacity = capacity;
			if (newCapacity >= (SIZE_MAX / 2)){
				newCapacity++;
			}
			else{
				size_t newCapacity = capacity * 2;
				if (newCapacity <= capacity){
					DynamicString_Free(token);
					return 0;
				}
			}

			dynamicstring_t **temp = realloc(res, sizeof(dynamicstring_t) * outputCount);
			if (temp == NULL){
				//free memory
				freeArray(res, outputCount);
				DynamicString_Free(token);
				return 0;
			}
			res = temp;
			capacity = newCapacity;
		}

		res[outputCount] = token;
		outputCount++;
	}

	if (outputCount == 0){
		res[outputCount] = input;
		*array = res;
		*size = 1;
		return 1;
	}

	*array = res;
	*size = outputCount;

	
	return result;
}