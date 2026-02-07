// Implement a lexer parser in this file that splits text into individual tokens.
// You may reuse any functions you write for your main shell.
// 
// The point of this exercise is to get something small working first!
// You may choose to reuse this portion of your code later on.
//
#include <string.h>
#include "dynamicstring.h"
#include <ctype.h>
#include <stdio.h>

// Implement your function parse here
// Parse should accept 1 string as an argument.
// Think carefully about the return type
// --what would be a useful type to store a
// --collection of tokens?





// only thing left to do is if there are
// errors in realloc to account for them and free memroy

char** parse(const char* input){
    if (input == NULL){
        return NULL;
    }

    size_t len = strlen(input);
    size_t capacity = 16;
    char** tokens = malloc(capacity * sizeof(char*));
    if (tokens == NULL){
        return NULL;
    }
    size_t numTokens = 0;

    size_t i = 0;
    while (i < len){
        
        while (i < len && isspace((unsigned char) input[i])){
            i++;
        }

        if (i >= len){
            break;
        }

        if (input[i] == '&' && i + 1 < len && input[i + 1] == '&'){
            i += 2;
            char* temp = strdup("&&");
            if (temp == NULL){
                for (size_t k = 0; k < numTokens; k++){
                    free(tokens[k]);
                }
                free(tokens);
                return NULL;
            }

            if (numTokens >= capacity){
                capacity *= 2;
                char** new_tokens = realloc(tokens, capacity * sizeof(char*));
                if (new_tokens == NULL){
                    free(temp);
                    for (size_t k = 0; k < numTokens; k++){
                        free(tokens[k]);
                    }
                    free(tokens);
                    return NULL;
                }
                tokens = new_tokens;
            }

            tokens[numTokens] = temp;
            numTokens++;
        }
        else if(input[i] == '|' && i + 1 < len && input[i + 1] == '|'){
            i += 2;
            char* temp = strdup("||");
            if (temp == NULL){
                for (size_t k = 0; k < numTokens; k++){
                    free(tokens[k]);
                }
                free(tokens);
                return NULL;
            }

            if (numTokens >= capacity){
                capacity *= 2;
                char** new_tokens = realloc(tokens, capacity * sizeof(char*));
                if (new_tokens == NULL){
                    free(temp);
                    for (size_t k = 0; k < numTokens; k++){
                        free(tokens[k]);
                    }
                    free(tokens);
                    return NULL;
                }
                tokens = new_tokens;
            }

            tokens[numTokens] = temp;
            numTokens++;
        }

        else if (input[i] == '|' || input[i] == ';' || input[i] == '>' || input[i] == '<' || input[i] == '&'){
            char temp[2] = {input[i], '\0'};
            if (numTokens >= capacity){
                capacity *= 2;
                char** new_tokens = realloc(tokens, capacity * sizeof(char*));
                if (new_tokens == NULL){
                    for (size_t k = 0; k < numTokens; k++){
                        free(tokens[k]);
                    }
                    free(tokens);
                    return NULL;
                }
                tokens = new_tokens;
            }
            tokens[numTokens] = strdup(temp);
            if (tokens[numTokens] == NULL){
                for (size_t k = 0; k < numTokens; k++){
                    free(tokens[k]);
                }
                free(tokens);
                return NULL;
            }
            numTokens++;
            i++;
        }


        else{
            dynamicstring_t* tok = DynamicString_Create("");
            if (tok == NULL){
                for (size_t k = 0; k < numTokens; k++){
                    free(tokens[k]);
                }
                free(tokens);
                return NULL;
            }

            while (i < len){
                char c = input[i];

                if (!(c == '\\' || c == '"' || c == '\'')){
                    if (c == '&' || c == '<' || c == '>' || c == ';' || (isspace((unsigned char) c))){
                        break;
                    }
                }

                if (c == '\\'){
                    if (i + 1 < len){
                        i++; //maybe check if dynamicString is 1 or 0 idk
                        DynamicString_AppendChar(tok, input[i]);
                        i++;
                    }
                    else{
                        DynamicString_AppendChar(tok, '\\');
                        i++;
                    }
                }
                else if(c == '"' || c == '\''){
                    char temp = c;
                    i++;
                    while (i < len){
                        if (input[i] == '\\' && i + 1 < len){
                            i++;
                            DynamicString_AppendChar(tok, input[i]);
                            i++;
                        }
                        else if(input[i] == temp){
                            i++;
                            break;
                        }
                        else{
                            DynamicString_AppendChar(tok, input[i]);
                            i++;
                        }
                    }
                }
                else if(isspace((unsigned char) c)){
                    break;
                }
                else{
                    DynamicString_AppendChar(tok, c);
                    i++;
                }
            }
            const char* tempStr = DynamicString_GetCString(tok);
            if (tempStr == NULL){
                tokens[numTokens] = strdup("");
            }
            else{
                tokens[numTokens] = strdup(tempStr);
            }
            if (tokens[numTokens] == NULL){
                DynamicString_Free(tok);
                for (size_t k = 0; k < numTokens; k++){
                    free(tokens[k]);
                }
                free(tokens);
                return NULL;
            }
            DynamicString_Free(tok);
            numTokens++;
        }

        if (numTokens >= capacity){
            capacity *= 2;
            char** tmp = realloc(tokens, capacity * sizeof(char*));
            if (tmp == NULL){
                for (size_t k = 0; k < numTokens; k++){
                    free(tokens[k]);
                }
                free(tokens);
                return NULL;
            }
            tokens = tmp;
        }
    }
    tokens[numTokens] = NULL;
    return tokens;
}

void freeTokens(char** tokens){
    if (tokens == NULL){
        return;
    }
    size_t i = 0;
    while(tokens[i] != NULL){
        free(tokens[i]);
        i++;
    }
    free(tokens);
    return;
}