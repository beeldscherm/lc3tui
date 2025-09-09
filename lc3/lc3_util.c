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
        "\\0",  "\\01", "\\02", "\\03", "\\04", "\\05", "\\06", "\\a",  "\\b",  "\\t",  "\\n",  "\\v",   "\\f",  "\\r",  "\\0E", "\\0F", 
        "\\10", "\\11", "\\12", "\\13", "\\14", "\\15", "\\16", "\\17", "\\18", "\\19", "\\1A", "\\e",   "\\1C", "\\1D", "\\1E", "\\1F", 
        " ",    "!",    "\"",   "#",    "$",    "%",    "&",    "'",    "(",    ")",    "*",    "+",     ",",    "-",    ".",    "/",     
        "0",    "1",    "2",    "3",    "4",    "5",    "6",    "7",    "8",    "9",    ":",    ";",     "<",    "=",    ">",    "?",     
        "@",    "A",    "B",    "C",    "D",    "E",    "F",    "G",    "H",    "I",    "J",    "K",     "L",    "M",    "N",    "O",     
        "P",    "Q",    "R",    "S",    "T",    "U",    "V",    "W",    "X",    "Y",    "Z",    "[",     "\\\\", "]",    "^",    "_",     
        "`",    "a",    "b",    "c",    "d",    "e",    "f",    "g",    "h",    "i",    "j",    "k",     "l",    "m",    "n",    "o",     
        "p",    "q",    "r",    "s",    "t",    "u",    "v",    "w",    "x",    "y",    "z",    "{",     "|",    "}",    "~",    "\\FF",
    };

    return ((ch >= 0 && ch < 128) ? map[ch] : "\\?");
}
