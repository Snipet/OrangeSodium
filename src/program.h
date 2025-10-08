//Class containing information and functions for loading and working with programs (Lua)

#include <string>

namespace OrangeSodium{

class Program{
public:
    Program();
    ~Program();

private:
    std::string program_path;
    std::string program_name;
};

}