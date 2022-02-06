#include "asts_interface.h"

#include "mtesrl.h"
#include "mteerr.h"

namespace ad::asts {

bool AstsInterface::LoadInterface(int handle, std::string & errmsg, bool debug) {
  MTEMSG *ifacedata = nullptr;
  int interface=0;
  interface = MTEStructureEx(handle, 3, &ifacedata);
  if (interface < 0) {
    errmsg = std::string("MTEStructureEx returned an error: ")+MTEErrorMsg(interface);
    return false;
  }
  int * pointer = (int *)ifacedata->Data;
  ReadFromBuf(pointer);
//if(debug)
//  Dump();
  return true;
}

int* AstsInterface::ReadFromBuf(int * pointer) {
  return pointer;
}

}
