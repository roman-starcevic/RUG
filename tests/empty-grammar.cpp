#include "inc/Rug.hpp"
#include <iostream>
int main(int argc, char **argv) {
    if(argc!=2) return 2;
    try { Rug rug(argv[1]); rug.build(); std::cerr << "unexpected acceptance\n"; return 1; }
    catch(const RugBuildError& error) {
        if(!error.diagnostics().has_errors()) return 1;
        error.print(std::cout,false); return 0;
    }
    catch(const std::exception& error) { std::cerr << "wrong exception: " << error.what() << '\n'; return 1; }
}
