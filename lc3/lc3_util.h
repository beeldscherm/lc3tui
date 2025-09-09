#pragma once
#include "lib/leakcheck/lc.h"
#define VA_MALLOC lc_malloc
#define VA_REALLOC lc_realloc
#define VA_FREE lc_free
#include "lib/va_template.h"


// Variable-length string(array) type
vaTypedef(char, String);
vaTypedef(String, StringArray);

// String functions
vaAllocFunctionDefine(String, newString);
vaAppendFunctionDefine(String, char, addchar);

// String array functions
vaAllocFunctionDefine(StringArray, newStringArray);
vaAppendFunctionDefine(StringArray, String, addString);
vaFreeFunctionDefine(StringArray, freeStringArray);

/*
 * Returns a string representing the inputted character
 * For normal characters, this is just the character
 * For escape charactrs, its string representation is returned (e.g. "\\n")
 * Otherwise, the string "\\?" is returned
 */
const char *charString(int ch);
