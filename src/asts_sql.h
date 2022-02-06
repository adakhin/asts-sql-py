#ifndef ASTS_SQL_H
#define ASTS_SQL_H

#include <string>
#include <map>
#include <memory>

#include "mtesrl.h"
#include "mteerr.h"

#include "asts_interface.h"

namespace ad::asts {

class AstsConnection {
private:
  std::map<std::string, int> handles_ { {"TE", -1}, {"RE", -1}, {"RFS", -1}, {"ALGO", -1} };
  std::map<std::string, std::shared_ptr<AstsInterface> > interfaces_;
  bool debug_ = true;
public:
  bool Connect(const std::string & system, const std::string & params, std::string & errmsg);
  void Disconnect(const std::string & system);
};

}
#endif // ASTS_SQL_H
