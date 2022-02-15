#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

char * fun1();

#define TEST_STR "string"
int main ()
{
  char a = 5;
  char *b;
  char *c;
  char **d;
  d = &c;
  b = &a;
  // c = &b;
  char arr2[5] = {1,2,3,4,5};
  c = &arr2[3];
  d = &c;
  printf("Deref C  : %d\n", *c);
  printf("Address C: %p\n", &c);
  printf("C Itself :   %p\n", c);
  printf("Address of arr2[3]: %p\n", &arr2[3]);
  c++;
  printf("Post Increment\n");
  printf("Deref C  : %d\n", *c);
  printf("Address C: %p\n", &c);
  printf("C Itself :   %p\n", c);
  printf("Address of arr2[4]: %p\n", &arr2[4]);
  printf("Done\n");
  printf("Address of c is:%p\n ", *d);
  // printf("Address?xxx %p\n", c);
  // printf("Address %p\n", &d);
  // printf("Address %p\n", d);
  // printf("Address %p\n", *d);
  // printf("Size of char address: %ld\n", sizeof(&b));
  // printf("Side of char ptr: %ld\n", sizeof(char*));
  // printf("Size of int: %ld\n", sizeof(int));
  // printf("Size of long int: %ld\n", sizeof(long int));
  char arr[50] = {1,2,3,4,5,6,7,8};
  long int address = (arr+5);
  printf("Value is: %d\n", *( (char*)address ));
  char *p_arr = fun1();
  printf("element 1: %d\n", *p_arr);
  printf("element 2: %d\n", *(p_arr+1));
  const char *newstring = "String is:" TEST_STR "\n\0";
  // printf(newstring);
  return 1;
}

char * fun1()
{
  static char arr[50] = {55};
  return arr;
}