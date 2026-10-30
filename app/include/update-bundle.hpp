#pragma once
#include "update-core.hpp"
#include <functional>
namespace update {
struct BundleFile {std::string name,sha;uint64_t size=0;};
inline constexpr const char *BundleNames[]={"SwitchTalk.ovl","exefs.nsp","SwitchTalkWorker.nro"};
inline bool bundleParse(const std::string &text,const std::string &expected,std::array<BundleFile,3>&out){
 if(text.size()>1024 || text.find('\0')!=std::string::npos)return false;
 size_t pos=0;auto line=[&](std::string &s){size_t end=text.find('\n',pos);if(end==std::string::npos)return false;s=text.substr(pos,end-pos);pos=end+1;return true;};std::string s;
 if(!line(s)||s!="SWITCHTALK-BUNDLE-1"||!line(s)||s!="version="+expected)return false;
 for(unsigned i=0;i<3;i++){if(!line(s))return false;auto a=s.find(' '),b=s.find(' ',a==std::string::npos?a:a+1);if(a==std::string::npos||b==std::string::npos)return false;
  auto name=s.substr(0,a),size=s.substr(a+1,b-a-1),sha=s.substr(b+1);if(name!=BundleNames[i]||size.empty()||size.size()>8||size.find_first_not_of("0123456789")!=std::string::npos||sha.size()!=64||sha.find_first_not_of("0123456789abcdef")!=std::string::npos)return false;
  uint64_t n=0;for(char c:size)n=n*10+c-'0';if(n<128||n>MaxSize)return false;out[i]={name,sha,n};
 }return pos==text.size();
}
inline bool bundleHash(const std::string &path,const BundleFile &m){
 FILE *f=fopen(path.c_str(),"rb");if(!f)return false;SHA256 hash;uint8_t buf[16384];uint64_t n=0;size_t got;
 while((got=fread(buf,1,sizeof(buf),f))){n+=got;if(n>m.size)break;hash.add(buf,got);}bool ok=!ferror(f)&&n==m.size&&hash.finish()==m.sha;fclose(f);return ok;
}
inline bool copyBundleFile(const std::string &from,const std::string &target,const BundleFile &meta){
 if(bundleHash(target,meta))return true;
 auto temp=target+".swt-update.tmp";FILE *in=fopen(from.c_str(),"rb");if(!in)return false;FILE *out=fopen(temp.c_str(),"wb");if(!out){fclose(in);return false;}
 uint8_t buf[16384];size_t got;bool ok=true;while((got=fread(buf,1,sizeof(buf),in)))if(fwrite(buf,1,got,out)!=got){ok=false;break;}
 ok=ok&&!ferror(in)&&flush(out);if(fclose(out))ok=false;fclose(in);
 if(!ok||!bundleHash(temp,meta)){erase(temp);return false;}
 return replace(temp,target)&&bundleHash(target,meta);
}
// All bundled files are verified before the stop callback or any installation.
// A failed sync leaves the boot marker intact: the stable launcher can choose
// the previous verified app bundle and repair its companions on the next launch.
inline bool syncBundle(const std::string &source,const std::string &version,const std::array<std::string,3>&targets,const std::function<bool()> &stop,std::string &error){
 std::array<BundleFile,3> entries;
 if(!bundleParse(readText(source+"/files.txt"),version,entries)){error="Invalid bundled component manifest";return false;}
 bool needed=false;for(unsigned i=0;i<3;i++){if(!bundleHash(source+"/"+entries[i].name,entries[i])){error="Bundled component failed verification";return false;}needed|=!bundleHash(targets[i],entries[i]);}
 if(!needed)return true;if(!stop()){error="Close the notification worker before installing this update";return false;}
 for(unsigned i=0;i<3;i++)if(!copyBundleFile(source+"/"+entries[i].name,targets[i],entries[i])){error="Component install failed. Reopen SwitchTalk to recover";return false;}
 return true;
}
}
