#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	int length = strlen(str);

	for (int i=0;i<length/2;i++){
		Swap((str+i), (str+length-1-i));
	}
}

void Swap(char *left, char *right)
{
	char temp = *left;
	*left = *right;
	*right = temp;
}

