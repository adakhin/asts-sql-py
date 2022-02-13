#ifndef ASTS_CONNECTION_H
#define ASTS_CONNECTION_H

#include <string>
#include <map>
#include <memory>

#include "mtesrl.h"
#include "mteerr.h"

#include "asts_interface.h"

namespace ad::asts {

template<typename storage_engine_t> class AstsConnection {
private:
  std::map<std::string, int> handles_ { {"TE", -1}, {"RE", -1}, {"RFS", -1}, {"ALGO", -1} };
  std::map<std::string, std::shared_ptr<AstsInterface> > interfaces_;
  bool debug_ = true;
  storage_engine_t engine_;
public:
  bool Connect(const std::string & system, const std::string & params, std::string & errmsg){
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
    engine_.AddInterface(interfaces_[system]);

    return true;

  }
  void Disconnect(const std::string & system){
    if(handles_.find(system) == handles_.end())
      return;
    if(handles_[system] >= 0) {
      MTEDisconnect(handles_[system]);
      handles_[system] = -1;
    }
  }
};

}
#endif // ASTS_CONNECTION_H
