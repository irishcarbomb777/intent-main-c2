#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ReadyState_XL_ODR 6667
#define RollbackTime_seconds 3
#define RollbackBuffer_length (ReadyState_XL_ODR*RollbackTime_seconds)

typedef struct {
  int a;
  int b;
} thing1_t;

typedef struct thing2_t {
  int *p_a;
  int *p_b;
} thing2_t;

int * testfun();

int main (){
  // Declare thing1
  thing1_t thing1 = {
    .a = 5,
    .b = 6,
  };
  thing2_t thing2 = {
    .p_a = &(thing1.a),
    .p_b = &(thing1.b),
  };
  printf(" A from thing2 is: %d", *(thing2.p_a));


  int cheese[5] = {1,2,3,4,5};
  printf("%d\n\n", *(cheese+2));

  char taps_detected = 0;
  char tap_delta = 5;
  char Tap_MinDelta = 4;
  taps_detected += (tap_delta > Tap_MinDelta) ? 1 : 0;
  printf("%d\n\n", taps_detected);


  uint circleBuffer = 0;
  for (int i = 0; i < 100; i++)
  {
    circleBuffer = (((circleBuffer | 0xFFF0) - 1) & 0x000F);
    printf("Output: %d\n", circleBuffer); 
  }
  printf("%d\n", RollbackBuffer_length);

  int *p_testVar = testfun();
  printf("%d\n", *p_testVar);

  p_testVar = testfun();
  printf("%d\n", *p_testVar);

}

int * testfun()
{
  static int testVar = 907;
  testVar++;
  return &testVar;
}




