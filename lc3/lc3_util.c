#include "lc3_util.h"

// String functions
vaAllocFunction(String, char, newString, ;, va.ptr[0] = '\0')
vaClearFunction(String, char, clearString,,, va->ptr[0] = '\0')

vaAppendFunction(String, char, addchar,
    // Tomfoolery to make the null terminator exist
    va->ptr[va->sz] = el;
    va->sz++;
    el = '\0';
    ,
    va->sz--;
)

// String array functions
vaAllocFunction(StringArray, String, newStringArray, ;, ;)
vaAppendFunction(StringArray, String, addString, ;, ;)
vaFreeFunction(StringArray, String, freeStringArray, lc_free(el.ptr), ;, ;)


// Strings for escaped characters
const char *charString(int ch) {
    static const char *map[] = {
        "\\0",   "\\x01", "\\x02", "\\x03", "\\x04", "\\x05", "\\x06", "\\a",   "\\b",   "\\t",   "\\n",   "\\v",   "\\f",   "\\r",   "\\x0E", "\\x0F", 
        "\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17", "\\x18", "\\x19", "\\x1A", "\\e",   "\\x1C", "\\x1D", "\\x1E", "\\x1F", 
        " ",     "!",     "\"",    "#",     "$",     "%",     "&",     "'",     "(",     ")",     "*",     "+",     ",",     "-",     ".",     "/",     
        "0",     "1",     "2",     "3",     "4",     "5",     "6",     "7",     "8",     "9",     ":",     ";",     "<",     "=",     ">",     "?",     
        "@",     "A",     "B",     "C",     "D",     "E",     "F",     "G",     "H",     "I",     "J",     "K",     "L",     "M",     "N",     "O",     
        "P",     "Q",     "R",     "S",     "T",     "U",     "V",     "W",     "X",     "Y",     "Z",     "[",     "\\\\",  "]",     "^",     "_",     
        "`",     "a",     "b",     "c",     "d",     "e",     "f",     "g",     "h",     "i",     "j",     "k",     "l",     "m",     "n",     "o",     
        "p",     "q",     "r",     "s",     "t",     "u",     "v",     "w",     "x",     "y",     "z",     "{",     "|",     "}",     "~",     "\\xFF",
    };

    return ((ch > 0 && ch < 128) ? map[ch] : "\\?");
}
