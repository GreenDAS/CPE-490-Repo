/**************************************************************************
 * Author  Daniel S.
 * Date    10/16/2025
 * File    string_lib.c
 * Version V1.0
 * Description 
 * A Helpful library of String Functions & the stringClass
 *
 ************************************************************************/

//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "stm32l476xx.h"
#include "stdlib.h"
#include "string.h"

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/* Combines 2 Strings
 Arg1 = Front Portion of the string
 Arg2 = Back Portion of the String
 Arg3 = Return String
 Arg4 = Add new line (true; false)
*/
void combine2Strings(char* string1, char* string2, char* returnString, int addNewLine){
	strcpy(returnString,string1);
	if(addNewLine)returnString[strlen(string1) + 1] = '\n'; // Add's New Line
	strcat(returnString,string2);
}


//------------------------------------------------------------------------------
// Classes
//------------------------------------------------------------------------------



/* String Class Definition
Descriptionn:

*/
typedef struct StringClass{
	//*-Parents-*//
	
	//*-Properties-*//
	char* str; // Holds the Strings char data
	int length; // Holds how long the string is
	int maxLength; // Holds max Length of string

	//*-Function Pointers-*//
	
}StringClass;
// Methods //
// Public --
/* Returns the Value in the string Class
 Arg1 = The stringClass to get the string from
*/
char* getStr(StringClass *obj){
	return obj->str;
}

/* Sets the string's value in the string Class
 Arg1 = The stringClass to set the string to
 Arg2 = The String to set it to
*/
void setStr(StringClass *obj, char* str){
	obj->length = strlen(str)+1; // Updates object's string's length
	free(obj->str); // frees up the old string
	obj->str = malloc(obj->length); // Reserves the space for the string
	if(!obj->str)while(1){}; // Traps the program if malloc Fails
	strncpy(obj->str,str,obj->maxLength); // Copies str to the objects str
}

/* Sets the string's maxLength in the string Class
 Arg1 = The stringClass's maxLength to set
 Arg2 = The max length to be set too
*/
void setMaxLength(StringClass *obj, int length){
	obj->maxLength = length; // Updates object's string's maxLength
}

/* Appends str onto the object's str
 Arg1 = The stringClass append the string onto
 Arg2 = The String to append to it
*/
void appendStr(StringClass *obj, char* str){
	obj->length = (strlen(obj->str) + 1); // holds the size of the old string
	free(obj->str); // frees up the old string
	obj->str = malloc((strlen(str) + 1) + obj->length); // Reserves the space for the new string
	strncat(obj->str,str,obj->maxLength); // appends the string onto the new string
}

// Private --

// Constructor //
StringClass* StringClass_Create(char* str, int maxLength){
	StringClass* self = malloc(sizeof(StringClass));
	self->maxLength = maxLength;
	self->str = malloc(sizeof(char*));
	setStr(self,str);
	return self;
}