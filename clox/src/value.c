#include <stdio.h>
#include "value.h"

VECTOR_IMPL(ValueArray, Value)

void printValue(Value value) {
  printf("%g", value);
}
