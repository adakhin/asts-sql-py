#include "asts_connection.h"

namespace ad::asts {

bool AstsConnection::Connect(const std::string & system, const std::string & params, std::string & errmsg) {
  if(handles_.find(system) == handles_.end())
    throw std::runtime_error("Invalid system "+system);
  if(handles_[system] >= 0)
    throw std::runtime_error("System "+system+" is already connected!");

  char ErrMsg[256];
  handles_[system] = MTEConnect((char*)params.c_str(), ErrMsg);

  if(handles_[system] < 0) {
    errmsg = "MTEConnect returned an error: "+std::to_string(handles_[system])+" "+std::string(ErrMsg);
    return false;
  }

  interfaces_[system] = std::make_shared<AstsInterface>();
  if(!interfaces_[system]->LoadInterface(handles_[system], errmsg, debug_)) {
//    errmsg = "Unable to load interface!";
    return false;
  }

  return true;
}

void AstsConnection::Disconnect(const std::string & system) {
  if(handles_.find(system) == handles_.end())
    return;
  if(handles_[system] >= 0) {
    MTEDisconnect(handles_[system]);
    handles_[system] = -1;
  }
}

}
