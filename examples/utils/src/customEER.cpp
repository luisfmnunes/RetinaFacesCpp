#include "../include/customEER.h"
#ifdef _WIN32
#include <float.h>
#endif
using namespace std;

int myfunction_descend(MatchPair a, MatchPair b)
{
	if(a.score>b.score) return 1;
	else return 0;
}
int myfunction_ascend(MatchPair a, MatchPair b)
{
	if(a.score<b.score) return 1;
	else return 0;
}

float fvceer(float *frr, float *far,int pos1,int pos2) //frr - false rejection rate, far - false acceptance rate
{
	float eer;
	float gy1 = frr[pos1];
	float gy2 = frr[pos2];
	float iy1 = far[pos1];
	float iy2 = far[pos2];

	if(fabs(gy1-iy1)<1e-10)
		eer = gy1;
	else
		if(fabs(gy2-iy2)<10e-10)
			eer = gy2;
		else
			if((gy1+iy1)<=(gy2+iy2))
				eer = (gy1+iy1)/2;
			else
				eer = (gy2+iy2)/2;
	return eer;
}
float calc_eer2(float *gms,float * ims, int num_gms,int num_ims,char * comentario, char *filename,char *info,char *base,char *o_output, bool save_info) //gms - genuine matching scores, img - impostor matching scores
{
	int i;
    int factor = 10000;
  	int size=factor+1;
	float eer,fmr0,fmr100,fmr1000,fmr10k,fmr100k,fmr1m,fnmr0,fnmr100,fnmrk,fnmr20,fnmr50,fnmr200;
	char tmp[256];
	FILE *f;
	float * frr=(float*)malloc(sizeof(float)*size);
	if(frr == NULL){
		os_error("Failed to allocate frr buffer");
		return -1.0f;
	}
	float * far=(float*)malloc(sizeof(float)*size);
	if(far == NULL){
		os_error("Failed to allocate far buffer");
		return -1.0f;
	}
	memset(frr,0,size*sizeof(float));
	memset(far,0,size*sizeof(float));
	//histograma genuinos
    for(i=0;i<num_gms;i++){
		int val = gms[i]*factor;
        frr[val]=frr[val]+1;
	}
	//histograma impostores
	for(i=0;i<num_ims;i++){
		int val = ims[i]*factor;
        far[val]=far[val]+1;
	}


	//genuinos acumulados
    for(i=1;i<size;i++)
       frr[i]+=frr[i-1];
   
    //impostores acumulados
    for(i=size-2;i>=0;i--)
       far[i]+=far[i+1];


    //normaliza
    for(i=0;i<size;i++){
		frr[i]=frr[i]/num_gms;
		far[i]=far[i]/num_ims;    
	}
    //caclcula EER
   	int n=size;
	i=0;
	while(frr[i]<far[i]){
		i++;
		if(i==n) break;
	}
	int pos=i;
	if(pos>0)
		eer = fvceer(frr,far,pos-1,pos);
	else
		eer = fvceer(frr,far,pos,pos);

	int score_eer=i*100/factor;
   
   //calcula fmr100
   i=0;
   while(far[i]>=0.01) i++;
   //while(fabs(frr[i]-frr[i+1])==0) i++;
   fmr100 =frr[i+1];
   if(far[size-1]>0.01) fmr100=1;
   int score_fmr100=i*100/factor;

  //calcular fmr1000
   i=0;
   while(far[i]>=0.001) i++;
   //while(fabs(frr[i]-frr[i+1])==0) i++;
   fmr1000 =frr[i+1];
   if(far[size-1]>0.001) fmr1000=1;
   int score_fmrk = i*100/factor;

   //calcula fmr10k
   i=0;
   while(far[i]>=0.0001) i++;
   //while(fabs(frr[i]-frr[i+1])==0) i++;
   fmr10k =frr[i+1];
   if(far[size-1]>0.0001) fmr10k=1;
   int score_fmr10k = i*100/factor;

 //calcula fmr100k
   i=0;
   while(far[i]>=0.00001) i++;
   //while(fabs(frr[i]-frr[i+1])==0) i++;
   fmr100k =frr[i+1];
   if(far[size-1]>0.00001) fmr100k=1;
   int score_fmr100k = i*100/factor;


 //calcula fmr1m
   i=0;
   while(far[i]>=0.000001) i++;
   //while(fabs(frr[i]-frr[i+1])==0) i++;
   fmr1m =frr[i+1];
   if(far[size-1]>0.000001) fmr1m=1;
   int score_fmr1m = i*100/factor;
   
   //calcula fmr0
   i=0;
   
   while(far[i]>0.000) i++;
  // while(fabs(frr[i]-frr[i+1])==0) i++;
   fmr0 =frr[i+1];
   if(far[size-1]>0) fmr0=1;
   int score_fmr0 = i*100/factor;

   i=0;   
   while(frr[i]<0.05) i++;
   fnmr20 =far[i];

   i=0;   
   while(frr[i]<0.02) i++;
   fnmr50 =far[i];

   i=0;   
   while(frr[i]<0.01) i++;
   fnmr100 =far[i];

   i=0;   
   while(frr[i]<0.005) i++;
   fnmr200 =far[i];

   i=0;   
   while(frr[i]<0.00000001) i++;
   fnmr0 =far[i];

   i=0;
	time_t     now = time(0);
    struct tm  tstruct;
    tstruct = *localtime(&now);
    strftime(tmp, sizeof(tmp), "%Y-%m-%d.%X", &tstruct);

	f=fopen(filename,"a");
    if(strlen(info)>0){
        	fprintf(f,"%s\n",info);
	}
	fprintf(f,"%s %s \neer=%4.3f%% fmr100=%4.3f%% fmrk=%4.3f%% fmr10k=%4.3f%% fmr100k=%4.3f%% fmr0=%4.3f%% score_eer=%d score_fmr100=%d score_fmrk=%d score_fmr10k=%d score_fmr100k=%d score_fmr0=%d\n\n",tmp,comentario,eer*100,fmr100*100,fmr1000*100,fmr10k*100,fmr100k*100,fmr0*100,score_eer,score_fmr100,score_fmrk,score_fmr10k,score_fmr100k,score_fmr0);
	//fprintf(f,"%s %s \neer=%4.3f%%  fnmr20=%4.3f%% fnmr50=%4.3f%% fnmr100=%4.3f%% fnmr200=%4.3f%% fnmr0=%4.3f%%\n\n",tmp,comentario,eer*100,fnmr20*100,fnmr50*100,fnmr100*100,fnmr200*100,fnmr0*100);
	fclose(f);

	//salva a curva ROC
    char roc_file[1024];
	sprintf(roc_file,"%s_%s_roc",filename,base);
	f=fopen(roc_file,"w");
    for(i=0;i<size;i++){
		fprintf(f,"%f %f %f\n",i*1.0/factor,frr[i],far[i]);
	}
    fclose(f);

	if(save_info){
		FILE *f_out = fopen(o_output, "a");

		fprintf(f_out, "EER: %.3f\n", eer * 100);
		fprintf(f_out, "score_EER: %d\n", score_eer);
		fprintf(f_out, "FMR100: %.3f\n", fmr100 * 100);
		fprintf(f_out, "score_FMR100: %d\n", score_fmr100);
		fprintf(f_out, "FMRk: %.3f\n", fmr1000 * 100);
		fprintf(f_out, "score_FMRk: %d\n", score_fmrk);
		fprintf(f_out, "FMR10k: %.3f\n", fmr10k * 100);
		fprintf(f_out, "score_FMR10k: %d\n", score_fmr10k);
		fprintf(f_out, "FMR100k: %.3f\n", fmr100k * 100);
		fprintf(f_out, "score_FMR100k: %d\n", score_fmr100k);
		fprintf(f_out, "FMR1m: %.3f\n", fmr1m * 100);
		fprintf(f_out, "score_FMR1m: %d\n", score_fmr1m);
		fprintf(f_out, "FMR0: %.3f\n", fmr0 * 100);
		fprintf(f_out, "score_FMR0: %d\n", score_fmr0);
		fprintf(f_out, "FNMR20: %.3f\n", fnmr20 * 100);
		fprintf(f_out, "FNMR50: %.3f\n", fnmr50 * 100);
		fprintf(f_out, "FNMR100: %.3f\n", fnmr100 * 100);
		fprintf(f_out, "FNMR200: %.3f\n", fnmr200 * 100);
		fprintf(f_out, "FNMR0: %.3f\n", fnmr0 * 100);

		fclose(f_out);
	}

	free(frr);
	free(far);
	return pos*1.0/factor; //retorna o score do eer
     
}

vector< MatchScores> load_matching2(char * filename)
{
	 int index;
     string tmp;
	 string query,ref;
	 float score;
	 ifstream f(filename);
     vector< MatchScores > data;
	 if(!f.good()){
		printf("cannot load %s",filename);
		return data;
	 }
	 int countwords=0;
	 int count=0;
	 while(!f.eof()){
		 string str;
		 if(!getline(f,str)) break;                
                 if(str=="") break;//para linha em branco no final
		 stringstream ss(str);
		 if(countwords==0){
			 stringstream sss(str);
	         countwords = std::distance(std::istream_iterator<std::string>(sss), std::istream_iterator<std::string>());
			 printf("countwords=%d\n",countwords);
		 }
		 ss >> index;
        ss >> query; ss>>ref;// ss>>tmp;
	
		 MatchScores match;
         match.query=query;
		 match.ref=ref;
         for(int i=0;i<countwords-2;i++){
			ss >> score;	
			//if(query=="007678541813_1.tpt" && ref=="038494921023_1.tpt" && i<7){
			//	//	printf(" found :%s\n",str.c_str());
			//	 score=0;			
			//}
			//cout << score << " " ;
			match.scores.push_back(score);
			//if(data.size()<10) cout << index <<" "<<query<<" " << ref <<" "<< score <<endl;
					
		 }
		 //cout <<endl;
		 data.push_back(match);	
		 count++;
//		if(count>10000000) break;
		cout << count <<endl;	
		
	}
	
	f.close();
    return data;
}
void SaveRankList(vector< MatchScores > &genuines, vector< MatchScores > &impostors, vector <MatchPair> gen_list, vector <MatchPair> imp_list,char * output,float eer_score,int ind,char *base)
{
    FILE *f;
	int i;
    char rank_file[1024];
	sprintf(rank_file,"%s_%s_rank%d",output,base,ind);
	f=fopen(rank_file,"w");
	i=0;
	std::sort(gen_list.begin(),gen_list.end(),myfunction_ascend);
	std::sort(imp_list.begin(),imp_list.end(),myfunction_descend);
	fprintf(f,"GENUINOS: eer_score=%f fmr0_score=%f\n",eer_score,imp_list[0].score);
	printf("eer_score=%f fmr0_score=%f\n",eer_score,imp_list[0].score);
	while(gen_list[i].score<=1*imp_list[0].score){
		//fprintf(f,"%s  %s  %0.3f\n",gen_list[i].query.c_str(),gen_list[i].ref.c_str(),gen_list[i].score);
		fprintf(f,"%s  %s  %0.3f  ",gen_list[i].query.c_str(),gen_list[i].ref.c_str(),gen_list[i].score);
		///guarda a pontuacao de todos os matchers
		for(int j=0;j<genuines[0].scores.size();j++)  
			fprintf(f,"%0.3f  ",genuines[gen_list[i].index].scores[j]);
		fprintf(f,"\n");
	
		i++;
		if(i>=gen_list.size()) break;
	}
      
	i=0;
	fprintf(f,"IMPOSTORES %f\n", imp_list[0].score);
	while(imp_list[i].score>=eer_score){
		//fprintf(f,"%s  %s  %0.3f\n",imp_list[i].query.c_str(),imp_list[i].ref.c_str(),imp_list[i].score);
		fprintf(f,"%s  %s  %0.3f  ",imp_list[i].query.c_str(),imp_list[i].ref.c_str(),imp_list[i].score);
		///guarda a pontuacao de todos os matchers
		for(int j=0;j<impostors[0].scores.size();j++)
			fprintf(f,"%0.3f  ",impostors[imp_list[i].index].scores[j]);
		fprintf(f,"\n");
		i++;
	}
	fclose(f);
	
}


int build_list_eval2(vector<MatchScores > &genuines,std::vector<MatchPair> &gen_list, float *gms, double *tempo_gms, int col,int limiar)
{
	float score = 0;
	vector< MatchScores >::iterator vit;
	float extras;
	int j=0;	
    double tempo=0;
    float minscore=9999,maxscore=-1;
	for(vit=genuines.begin();vit!=genuines.end();++vit){
		
			score = vit->scores[col];
			//if(score>1) printf("ind=%d, score=%f ",j,score);
			//if(vit->scores[3]>=0 && vit->scores[3]<limiar) score=0;
			//else  
			gen_list.push_back(MatchPair(j,vit->query,vit->ref,score));//vit->scores[i]
			if(vit->scores.size()>1) tempo+=vit->scores[1];

			if(score<minscore) minscore = score;
			if(score>maxscore) maxscore= score;
            gms[j]=score;				
//			gen_list.push_back(MatchPair(j,vit->query,vit->ref,score));//vit->scores[i]
			j++;
	};
	*tempo_gms=tempo;
        printf("minscore=%f, maxscore=%f\n",minscore,maxscore);
	if(maxscore>1){	//normalizar	
		for(int i=0;i<gen_list.size();i++){
			if(gms[i]>100) gms[i]=100;
			if(gms[i]>0) gms[i]=gms[i]/100;

			gen_list[i].score = gms[i];
		}
		
	}
	return j;
}

int build_list_eval2(vector<MatchPair> &matches, float *ms, stringstream &ss){
	float score = 0.0;
	int j = 0;
#ifndef _WIN32
	float minscore=__FLT_MAX__, maxscore = -1.0;
#else
	float minscore = FLT_MAX, maxscore = -1.0;
#endif

	for(auto match : matches){
		score = match.score;
		if(score<minscore) minscore = score;
		if(score>maxscore) maxscore = score;

		ms[j++] = score;
	}
	_result("minscore= %.2f, maxscore= %.2f",minscore,maxscore);
	if(maxscore>1){
		for(int i=0; i<matches.size(); i++){
			if(ms[i]>100) ms[i]=100;
			if(ms[i]>0) ms[i]=ms[i]/100;

			matches[i].score = ms[i];
		}
	}
	ss.precision(2);
	ss << maxscore << " " << minscore << " ";

	return j;
}

void eval2(char *file1,char *file2, char * output, char *info,bool save_rank,char *base,int col,int limiar,char *o_output,bool save_info)
{
	char str1[1024],str2[1024];
	FILE *f;
	int id,tempo=0;
	float score;
	int i=0,j;
    float extras;
	
	char comentario[1024];
    double  tempo_gms=0;
	double  tempo_ims=0;
   
	
	//genuino
     //   vector< vector<float> > genuines = load_matching(file1);
	 vector< MatchScores > genuines = load_matching2(file1);
     int num_gms = genuines.size();
     printf("genuinos=%d\n",num_gms);
	
	//impostor
	//vector<vector<float> > impostors = load_matching(file2);
    vector<MatchScores > impostors = load_matching2(file2);
    int num_ims=impostors.size();
    printf("impostors=%d\n",num_ims);

	int counts = genuines[0].scores.size();

	float * gms = new float[num_gms];
	float *ims = new float[num_ims];
	//vector< vector<float> >::iterator vit;
	vector< MatchScores >::iterator vit;	 	

	
//	for(i=0;i<counts-4;i++){
		std::vector<MatchPair> gen_list;
		std::vector<MatchPair> imp_list;
		j=0;
		float factor=1.0/100;
	 	
		num_gms=build_list_eval2(genuines,gen_list,gms,&tempo_gms,col,limiar);
		num_ims=build_list_eval2(impostors,imp_list,ims,&tempo_ims,col,limiar);
		
        int tempo_medio = 0;// 0.5*(tempo_gms/genuines.size() + tempo_ims/impostors.size());
		sprintf(comentario,"%s  %s  %ld  %ld  %4.2f  %4.2f ",file1,file2,gen_list.size(), imp_list.size(),tempo_gms*1.0/gen_list.size(), tempo_ims*1.0/imp_list.size());
		
		float eer_score=calc_eer2(gms,ims,num_gms,num_ims,comentario,output,info,base,o_output,save_info);
		
		//printf("eer_score=%f\n",eer_score);
		if(save_rank)
			SaveRankList(genuines,impostors,gen_list, imp_list,output,eer_score,i,base);
//	}
	printf("OK\n");
	
	delete gms;
	delete ims;
}

void eval2(std::vector<MatchPair> &genuines, std::vector<MatchPair> &impostors, char *output, char *info, bool save_rank, char *base, stringstream &ss, char *o_output, bool save_info){
	int num_gms = genuines.size();
	int num_ims = impostors.size();

	os_log("Genuines: ",num_gms);
	os_log("Impostors: ",num_ims);

	float *gms = new float[num_gms];
	float *ims = new float[num_ims];

	ss << genuines.size() << " " << impostors.size() << " ";

	num_gms = build_list_eval2(genuines,gms,ss);
	num_ims = build_list_eval2(impostors,ims,ss);

	string comentario = ss.str();

	float err_score = calc_eer2(gms,ims,num_gms,num_ims,&comentario[0],output,info,base, o_output, save_info);

	os_log("ERR Evaluation OK");

	delete gms;
	delete ims;

}

