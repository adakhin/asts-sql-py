#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

namespace ad::util {

inline std::string join(const std::vector<std::string>& v, const std::string& delimiter) {
  std::string result;
  for (size_t i = 0; i < v.size(); ++i)
    result += (i ? delimiter : "") + v[i];
  return result;
}

//----- MTESRL buffer routines -----------------------------

inline int* ReadValueFromBuf(int* pointer, int & result) {
  result = *pointer;
  return (int32_t *)(pointer + 1);
}

inline int* ReadValueFromBuf(int* pointer, std::string & result) {
  int datalen = 0;
  datalen = *pointer;
  char* data = (char *)(pointer + 1);
  result = std::string(data, datalen);
  return (int *)(data + datalen);
}

inline int* SkipIntFromBuf(int* pointer) {
  return (int32_t *)(pointer + 1);
}

inline int* SkipStringFromBuf(int* pointer) {
  return (int *)((char *)(pointer + 1) + (int)*pointer);
}

}
#endif // UTIL_H
