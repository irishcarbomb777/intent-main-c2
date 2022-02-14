#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

char * fun1();

#define TEST_STR "string"
int main ()
{
  char *p_arr = fun1();
  printf("element 1: %d\n", *p_arr);
  printf("element 2: %d\n", *(p_arr+1));
  const char *newstring = "String is:" TEST_STR "\n\0";
  printf(newstring);
  return 1;
}

char * fun1()
{
  static char arr[50] = {55};
  return arr;
}