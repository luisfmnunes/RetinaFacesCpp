#include <math.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>
#include <time.h>
#include <cstddef>

#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <iterator> 

#include "customLog.h"
#include "customStringOps.h"

#ifdef _WIN32
#undef far
#endif

struct MatchScores{
    std::string query;
    std::string ref;
    std::vector<float> scores;
};
struct MatchPair{
    std::string query;
    std::string ref; 
    float score;
    int index;
    MatchPair(int ind,std::string q, std::string r, float sc):query(q),ref(r),score(sc),index(ind){};
};


int myfunction_descend(MatchPair a, MatchPair b);
int myfunction_ascend(MatchPair a, MatchPair b);
float fvceer(float *frr, float *far,int pos1,int pos2);
float calc_eer2(float *gms,float * ims, int num_gms,int num_ims,char * comentario, char *filename,char *info,char *base,char *o_output,bool save_info);
std::vector< MatchScores> load_matching2(char * filename);
void SaveRankList(std::vector< MatchScores > &genuines, std::vector< MatchScores > &impostors, std::vector <MatchPair> gen_list, std::vector <MatchPair> imp_list,char * output,float eer_score,int ind,char *base);
int build_list_eval2(std::vector<MatchScores > &genuines,std::vector<MatchPair> &gen_list, float *gms, double *tempo_gms, int col,int limiar);
void eval2(char *file1,char *file2, char * output, char *info,bool save_rank,char *base,int col,int limiar,char *o_output,bool save_info);
void eval2(std::vector<MatchPair> &genuines, std::vector<MatchPair> &impostors, char *output, char *info, bool save_rank , char *base, std::stringstream &ss, char *o_output, bool save_info);
