#ifndef ASTS_INTERFACE_H
#define ASTS_INTERFACE_H

#include <map>
#include <string>

namespace ad::asts {

class AstsInterface {
private:
    std::string prefix_="RE$";
public:
    std::string name_="";
    std::string caption_="";
    std::string description_="";

    int* ReadFromBuf(int * pointer);
    bool LoadInterface(int handle, std::string & errmsg, bool debug = false);
    void Dump(void);

    std::string GetSystemType();
};


}
#endif // ASTS_INTERFACE_H
