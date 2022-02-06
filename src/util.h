#ifndef UTIL_H
#define UTIL_H

#include <string>

namespace ad::util {

static int* ReadValueFromBuf(int* pointer, int & result) {
  result = *pointer;
  return (int32_t *)(pointer + 1);
}

static int* ReadValueFromBuf(int* pointer, std::string & result) {
  int datalen = 0;
  datalen = *pointer;
  char* data = (char *)(pointer + 1);
  result = std::string(data, datalen);
  return (int *)(data + datalen);
}

static int* SkipIntFromBuf(int* pointer) {
  return (int32_t *)(pointer + 1);
}

static int* SkipStringFromBuf(int* pointer) {
  return (int *)((char *)(pointer + 1) + (int)*pointer);
}

}
#endif // UTIL_H
