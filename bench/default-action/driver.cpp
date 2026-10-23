#include "inc/Rug.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
static double now(){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int main(int argc,char**argv){
 if(argc!=3)return 2;
 int loops=std::atoi(argv[2]);if(loops<1)return 2;
 double start=now();size_t size=0;
 try{for(int i=0;i<loops;++i){Rug rug(argv[1]);rug.build();size+=rug.required_alloc_size();}}
 catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
 std::cout<<"{\"build_seconds\":"<<now()-start<<",\"loops\":"<<loops<<",\"image_bytes_sum\":"<<size<<"}\n";
}
