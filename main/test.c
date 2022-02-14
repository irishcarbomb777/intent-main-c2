#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

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
uint circular_add(int x, uint val, uint buffer_length);
void circular_memcpy( char *cSourceBuffer, uint uintSourceBufferLength,
                             char *cTargetBuffer, uint *uintTargetBufferIndex, 
                             uint uintTargetBufferLength);
int main (){
  
  // Test Circular Memcpy
  char target[32];
  uint targetIndex = 0;
  char source[] = {1,2,3,4,5};
  int yo = (int) (ReadyState_XL_ODR/RollbackTime_seconds) ;
  printf("Yo is: %d\n", yo);
  for (int i=0; i < 20; i++)
  {
    circular_memcpy((source), 5, target, &targetIndex, 32);
  }

  for (int i=0; i < 32; i++)
  {
    printf("Index: %d , Data: %d\n", i, target[i]);
  }
  // int a = 100;
  // int b = 3;
  // int count = 0;
  // for (int i=0; i < (a*7*b); i+=7*b)
  // {
  //   count++;
  //   printf("%d\n", i);
  // }
  // printf("%d\n", count);
}

void circular_memcpy( char *cSourceBuffer, uint uintSourceBufferLength,
                             char *cTargetBuffer, uint *uintTargetBufferIndex, 
                             uint uintTargetBufferLength)
{
  // Check if Source Buffer fits in current loop of Target Buffer
  if ((*uintTargetBufferIndex+uintSourceBufferLength) < uintTargetBufferLength)
  {
    // If Source Buffer fits, memcpy
    memcpy( (cTargetBuffer+*uintTargetBufferIndex), cSourceBuffer, uintSourceBufferLength);
    *uintTargetBufferIndex += uintSourceBufferLength;
    printf("Target Index: %d\n", *uintTargetBufferIndex);
  }
  else
  {
    uint target_space_remaining = uintTargetBufferLength - *uintTargetBufferIndex;
    memcpy ( (cTargetBuffer+*uintTargetBufferIndex), cSourceBuffer, target_space_remaining);
    uint source_bytes_remaining = uintSourceBufferLength - target_space_remaining;
    memcpy ( cTargetBuffer, (cSourceBuffer+target_space_remaining), source_bytes_remaining);
    *uintTargetBufferIndex = circular_add(uintSourceBufferLength, *uintTargetBufferIndex, uintTargetBufferLength); 
  }
}

int * testfun()
{
  static int testVar = 907;
  testVar++;
  return &testVar;
}

bool is_power_of_two(uint buffer_length)
{
    if (buffer_length == 0)
        return 0;
    while (buffer_length != 1) {
        if (buffer_length % 2 != 0)
            return 0;
        buffer_length = buffer_length / 2;
    }
    return 1;
}
uint circular_add(int x, uint val, uint buffer_length)
{
  // Create Logging Tag
  static const char *TAG = "Circular Add Log";
  static uint rollover;
  if ((val+x)<buffer_length)
  {
    val += x;
    return val;
  }
  else
  {
    val = (val+x)-buffer_length;
    return val;
  }
}







// uint circular_add(int x, uint val, uint buffer_length)
// {
//   // Create Logging Tag
//   static const char *TAG = "Circular Add Log";
//   if (is_power_of_two(buffer_length))
//   {
//     uint mask = ~(buffer_length-1);
//     val = (((val | mask) + x) & (buffer_length-1));
//     return val;
//   }
//   else
//   {
//     ESP_LOGI(TAG, "Buffer Length Specified is not a power of 2");
//     return 0;
//   }
// }


// bool is_power_of_two(uint buffer_length)
// {
//     if (buffer_length == 0)
//         return 0;
//     while (buffer_length != 1) {
//         if (buffer_length % 2 != 0)
//             return 0;
//         buffer_length = buffer_length / 2;
//     }
//     return 1;
// }