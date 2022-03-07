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

struct PointerHelper {
  int * _ptr;
  PointerHelper(int * pointer):_ptr(pointer) {}
  int ReadInt() {
    static_assert(sizeof(int) == 4); // we'll deal with other cases later
    int result = *_ptr;
    ++_ptr;
    return result;
  }
  unsigned char ReadChar() {
    unsigned char result;
    char * temp = (char*)_ptr;
    result = *temp;
    ++temp;
    _ptr = (int*)temp;
    return result;
  }
  std::string ReadString(int size) {
    std::string result = std::string((char *)_ptr, size);
    _ptr = (int *)((char *)_ptr + size);
    return result;
  }
  void Rewind(int c=1) {
    char* temp = (char*)_ptr;
    temp += c;
    _ptr = (int*)temp;
  }
};


inline int* ReadValueFromBuf(int* pointer, int & result) {
  static_assert(sizeof(int32_t) == 4); // we'll deal with other cases later
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
  static_assert(sizeof(int32_t) == 4); // we'll deal with other cases later
  return (int32_t *)(pointer + 1);
}

inline int* SkipStringFromBuf(int* pointer) {
  return (int *)((char *)(pointer + 1) + (int)*pointer);
}

//----- END MTESRL buffer routines -------------------------

inline std::string rpad(std::string s, size_t length) {
  if(s.size() < length) {
    size_t actl = s.length();
    s.insert(actl, length - actl, ' ');
  }
  return s;
}


inline std::string lpad(std::string s, size_t length) {
  if(s.size() < length) {
    size_t actl = s.length();
    s.insert(0, length - actl, ' ');
  }
  return s;
}

}
#endif // UTIL_H
