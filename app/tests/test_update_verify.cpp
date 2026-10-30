#include "update-core.hpp"
#include <iostream>
int main(int argc,char**argv){if(argc!=3)return 2;update::Manifest m;if(!update::parse(update::readText(argv[2]),m)||!update::verify(argv[1],m)){std::cerr<<"FAIL: update verifier rejected executable\n";return 1;}std::cout<<"PASS: actual release executable passes client manifest, SHA-256, NRO bounds, application title and version verification\n";return 0;}
