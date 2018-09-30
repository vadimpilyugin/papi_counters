#include "unistd.h"

#define SIZE 2048
int a[SIZE*SIZE];

int main() {
  while (1) {
    sleep(5);
    int s = 0;
    for (int i = 0; i < SIZE; i++) {
      s = s + a[i*SIZE+0];
    }
    a[0] = s;
  }
}
