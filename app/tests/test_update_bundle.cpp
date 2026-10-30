#include "update-bundle.hpp"
#include <filesystem>
#include <cassert>
#include <iostream>
using namespace update;
void put(const std::string &p,const std::string &s){assert(writeText(p,s));}
BundleFile file(const std::string&name,const std::string&data){SHA256 h;h.add(data.data(),data.size());return {name,h.finish(),data.size()};}
int main(){std::string root="work/switchtalk-bundle-tests";std::filesystem::create_directories(root+"/source");std::filesystem::create_directories(root+"/target");std::array<std::string,3> targets;std::string manifest="SWITCHTALK-BUNDLE-1\nversion=1.1.0\n";
 for(unsigned i=0;i<3;i++){auto name=BundleNames[i];std::string data(256,char('a'+i));auto m=file(name,data);put(root+"/source/"+name,data);targets[i]=root+"/target/"+name;erase(targets[i]);manifest+=m.name+" "+std::to_string(m.size)+" "+m.sha+"\n";}put(root+"/source/files.txt",manifest);
 std::string error;int stops=0;auto stop=[&]{stops++;return true;};std::array<BundleFile,3> parsed;assert(bundleParse(manifest,"1.1.0",parsed));assert(!bundleParse(manifest,"1.1.1",parsed));assert(!bundleParse(manifest+"junk","1.1.0",parsed));auto bad=manifest;bad.replace(bad.find("SwitchTalk.ovl"),13,"../badfile.ovl");assert(!bundleParse(bad,"1.1.0",parsed));
 put(root+"/source/exefs.nsp",std::string(256,'x'));assert(!syncBundle(root+"/source","1.1.0",targets,stop,error)&&stops==0&&!exists(targets[0]));put(root+"/source/exefs.nsp",std::string(256,'b'));
 assert(!syncBundle(root+"/source","1.1.0",targets,[&]{stops++;return false;},error)&&stops==1&&!exists(targets[0]));assert(syncBundle(root+"/source","1.1.0",targets,stop,error)&&stops==2);assert(syncBundle(root+"/source","1.1.0",targets,stop,error)&&stops==2);
 // Interrupted companion installation is repaired from the verified app bundle.
 put(targets[1],"partial");assert(syncBundle(root+"/source","1.1.0",targets,stop,error)&&stops==3);for(unsigned i=0;i<3;i++)assert(bundleHash(targets[i],parsed[i]));
 auto broken=targets;put(targets[0],"old");broken[1]=root+"/missing/exefs.nsp";assert(!syncBundle(root+"/source","1.1.0",broken,stop,error));assert(syncBundle(root+"/source","1.1.0",targets,stop,error));
 std::cout<<"PASS: matched bundled companions, strict names/version/size/hash, all files verified before worker stop, stop failure keeps originals, no rewrite when current, interrupted-install repair\n";}
