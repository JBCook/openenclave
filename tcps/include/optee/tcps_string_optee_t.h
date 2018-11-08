/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */
#pragma once
#ifndef _OE_ENCLAVE_H
# include <openenclave/enclave.h>
#endif
#ifndef OE_USE_OPTEE
# error tcps_string_optee_t.h should only be included with OE_USE_OPTEE
#endif

char* strrchr(const char* s, int c);
char* strncpy(char* strDest, const char* strSource, size_t count);  
char* strncat(char* destination, const char* source, size_t num);
long strtol(const char* nptr, char** endptr, int base);
unsigned long strtoul(const char* nptr, char** endptr, int base);
int atoi(const char* str);
void* bsearch(  
    const void* key,
    const void* base,
    size_t num,
    size_t width,
    int (*compare)(const void*, const void*));
char* strstr(
    const char* str1,
    const char* str2);
char* strerror(int errorCode);
