#include "orange_sodium.h"
#include <iostream>

using namespace OrangeSodium;
int main(int argc, char** argv){
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " <script_path>" << std::endl;
        return -1;
    }
    Synthesizer* synth = createSynthesizerFromScript(argv[1]);
    synth->buildSynthFromProgram();
    return 0;
}