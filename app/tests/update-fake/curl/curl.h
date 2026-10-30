#pragma once
#include <map>
#include <string>
#include <type_traits>
#include <algorithm>
#include <cassert>
using curl_off_t=long long;
using WriteFn=size_t(*)(char*,size_t,size_t,void*);
using ProgressFn=int(*)(void*,curl_off_t,curl_off_t,curl_off_t,curl_off_t);
enum {CURLOPT_URL,CURLOPT_USERAGENT,CURLOPT_CAINFO,CURLOPT_SSL_VERIFYPEER,CURLOPT_SSL_VERIFYHOST,CURLOPT_PROTOCOLS,CURLOPT_REDIR_PROTOCOLS,CURLOPT_FOLLOWLOCATION,CURLOPT_MAXREDIRS,CURLOPT_CONNECTTIMEOUT,CURLOPT_TIMEOUT,CURLOPT_LOW_SPEED_LIMIT,CURLOPT_LOW_SPEED_TIME,CURLOPT_NOSIGNAL,CURLOPT_FAILONERROR,CURLOPT_IPRESOLVE,CURLOPT_WRITEFUNCTION,CURLOPT_WRITEDATA,CURLOPT_NOPROGRESS,CURLOPT_XFERINFOFUNCTION,CURLOPT_XFERINFODATA};
constexpr int CURLPROTO_HTTPS=2,CURL_IPRESOLVE_V4=1,CURLE_OK=0,CURLINFO_RESPONSE_CODE=1,CURL_GLOBAL_DEFAULT=3;
namespace fakeHttp {struct Response{std::string body;long code=200;int failAfter=-1;};inline std::map<std::string,Response>responses;inline bool abortTransfer=false;inline int requests=0;inline std::map<int,long>options;}
struct CURL{std::map<int,std::string>strings;std::map<int,long>numbers;std::map<int,void*>pointers;WriteFn write=nullptr;ProgressFn progress=nullptr;long code=0;};
inline CURL*curl_easy_init(){return new CURL;}
template<class T>int curl_easy_setopt(CURL*c,int op,T value){if constexpr(std::is_integral_v<T>)c->numbers[op]=long(value);else if constexpr(std::is_convertible_v<T,const char*>)c->strings[op]=value;else if constexpr(std::is_same_v<T,WriteFn>)c->write=value;else if constexpr(std::is_same_v<T,ProgressFn>)c->progress=value;else c->pointers[op]=static_cast<void*>(value);return 0;}
inline int curl_easy_perform(CURL*c){fakeHttp::requests++;fakeHttp::options=c->numbers;assert(c->numbers[CURLOPT_SSL_VERIFYPEER]==1&&c->numbers[CURLOPT_SSL_VERIFYHOST]==2&&c->numbers[CURLOPT_PROTOCOLS]==CURLPROTO_HTTPS&&c->numbers[CURLOPT_REDIR_PROTOCOLS]==CURLPROTO_HTTPS&&c->numbers[CURLOPT_MAXREDIRS]==5);assert(c->strings[CURLOPT_CAINFO]=="romfs:/update/cacert.pem");auto it=fakeHttp::responses.find(c->strings[CURLOPT_URL]);if(it==fakeHttp::responses.end())return 6;auto&r=it->second;c->code=r.code;if(r.code>=400)return 22;for(size_t pos=0;pos<r.body.size();){if(fakeHttp::abortTransfer||(c->progress&&c->progress(c->pointers[CURLOPT_XFERINFODATA],0,0,0,0)))return 42;if(r.failAfter>=0&&pos>=size_t(r.failAfter))return 56;size_t n=std::min<size_t>(257,r.body.size()-pos);if(c->write(const_cast<char*>(r.body.data()+pos),1,n,c->pointers[CURLOPT_WRITEDATA])!=n)return 23;pos+=n;}return 0;}
inline int curl_easy_getinfo(CURL*c,int,long*value){*value=c->code;return 0;}
inline void curl_easy_cleanup(CURL*c){delete c;}
inline int curl_global_init(long){return 0;}
inline void curl_global_cleanup(){}
