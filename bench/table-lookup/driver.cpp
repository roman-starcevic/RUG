// C++98 audit driver: independent build / session timing, reset, binary-safe input.
#include "inc/Rug.hpp"
#include "inc/RugSession.hpp"
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <ctime>
#include <sys/resource.h>
static double now() { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static size_t rss() {
    std::ifstream status("/proc/self/status"); std::string key;
    while(status >> key) {
        if(key=="VmRSS:") { size_t kib=0; status >> kib; return kib; }
        std::string line; std::getline(status,line);
    }
    return 0;
}
int main(int argc, char **argv) {
    if (argc != 6) return 2;
    try {
        std::ifstream in(argv[2], std::ios::binary);
        if (!in) return 2;
        std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        size_t chunk = std::strtoul(argv[3], NULL, 10);
        int loops = std::atoi(argv[4]);
        bool expected = std::atoi(argv[5]) != 0;
        if (!chunk || loops <= 0) return 2;
        double start = now(); Rug rug(argv[1]); rug.build(); double built = now();
        RugSession session(rug, NULL, 4096);
        size_t rss_built = rss();
        double parse = now();
        for (int repeat=0; repeat<loops; ++repeat) {
            session.reset(); e_parse_result result=PARSE_CONSUMED;
            for(size_t offset=0; offset<data.size() && result==PARSE_CONSUMED;) {
                size_t n = std::min(chunk, data.size()-offset);
                result=session.push(reinterpret_cast<const unsigned char*>(data.data()+offset), n);
                offset+=n;
            }
            if(result==PARSE_CONSUMED) result=session.finish();
            if (result != (expected ? PARSE_ACCEPTED : PARSE_ERROR)) { std::cerr << "unexpected result " << result << '\n'; return 1; }
        }
        double end=now(); struct rusage usage; getrusage(RUSAGE_SELF,&usage);
        std::cout << "{\"build_seconds\":" << built-start << ",\"parse_seconds\":" << end-parse
                  << ",\"input_bytes\":" << data.size() << ",\"grammar_image_bytes\":" << rug.required_alloc_size()
                  << ",\"rss_after_build_kib\":" << rss_built << ",\"rss_after_parse_kib\":" << rss()
                  << ",\"loops\":" << loops << ",\"chunk\":" << chunk << ",\"peak_rss_kib\":" << usage.ru_maxrss << "}\n";
    } catch(const RugBuildError& e) { e.print(std::cerr,false); return 3; }
      catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 4; }
}
