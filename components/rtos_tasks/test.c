#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum FRUIT_ENUM {
    apple, orange, grape, banana,
};

int main()
{
  char *array = (char*)malloc(sizeof(char)*10);
  char string[20];
  *(array)='h';
  *(array+1)='e';
  *(array+2)='l';
  *(array+3)='l';
  *(array+4)='o';

  memcpy(string, array, 5);
  string[5] = '\0';
  if (!strncmp(array, "hello", 5))
    printf("yes\n");



  printf("enum apple as a string: %s\n", array);
}
