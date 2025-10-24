/**************************************************************************
 * Author  Daniel S.
 * Date    10/16/2025
 * File    string_lib.h
 * Version V1.0
 * Description 
 * Make string_lib.c's classes and functions visable to others
 *
 *************************************************************************/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------


#include "stm32l476xx.h"


//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

/* Combines 2 Strings
 Arg1 = Front Portion of the string
 Arg2 = Back Portion of the String
 Arg3 = Return String
 Arg4 = Add new line (true; false)
*/
void combine2Strings(char* string1, char* string2, char* returnString, int addNewLine);

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// # Classes
//------------------------------------------------------------------------------

// Forward Def //
typedef struct stringClass stringClass;

// Methods //

/* Returns the Value in the string Class
 Arg1 = The stringClass to get the string from
*/
char* getStr(stringClass *obj);

/* Sets the string's value in the string Class
 Arg1 = The stringClass to set the string to
 Arg2 = The String to set it to
*/
void setStr(stringClass *obj, char* str);

/* Sets the string's maxLength in the string Class
 Arg1 = The stringClass's maxLength to set
 Arg2 = The max length to be set too
*/
void setMaxLength(stringClass *obj, int length);

/* Appends str onto the object's str
 Arg1 = The stringClass append the string onto
 Arg2 = The String to append to it
*/
void appendStr(stringClass *obj, char* str);


/* Constructor
 Arg1 = the initial string value
 Arg2 = the string's max char size (max length)
*/
stringClass* stringClass_Create(char* str, int maxLength);