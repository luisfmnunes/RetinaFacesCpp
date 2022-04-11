#include <customUtils.h>
#include <facelib_api.h>
#include <RetinaModel.h>
#include <cassert>
#include <iomanip>
#include <thread>
#include <mutex>
#include <math.h>
#include <random>

#include <opencv/cv.h>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
bool debug = false;
bool result = false;
bool file_id = false, dir_id = false;
unsigned int num_threads = 4;
mutex locker;
bool save_info = false;
string o_output; // string for file saving diverse info

void check_id(vector<string> keys, bool &dir_id, bool &file_id, char delimiter = '_');

void parseHeader(FILE* &reader, string &path){
	string line;
	
    char dir[10000];
	(void)!fscanf(reader,"%s ", dir);
    line = dir;
	if(line.find("PATH=")!=string::npos)
		path = line.substr(line.find_first_of('=')+1) + '/';
	else {
		fseek(reader,0,SEEK_SET);
	}
}

string swap_extension(string src, const string &ext) {
  string dest;
  string::size_type p;
  p = src.rfind(".");
  if (p == string::npos) {
    dest = src;
    if (ext[0] != '.') dest += '.';
    dest += ext;
  } else {
    dest = src.substr(0,p);
    if (ext[0] != '.') dest += '.';
    dest += ext;
  }
  return dest;
}

FL_ErrorCode retrieve_files (const char *arg, vector<string> &out) {
    if(!is_file(arg)) return FL_ERR_FAILED_TO_OPEN_FILE;
    string path;

    FILE *fin = fopen(arg,"r");
    os_log("Openning File: ", arg);
    parseHeader(fin,path);

    char line[5000];
    while(fscanf(fin," %s",line)!= EOF){
        string x(line);
        out.push_back(path+x);
    }

    if(!out.size()) return FL_ERR_INVALID_DATA;

    return FL_ERR_NONE;
}

FL_ErrorCode retrieve_file_pairs (const char *arg, vector<pair<string,string> > &out){
    if(!is_file(arg)) return FL_ERR_FAILED_TO_OPEN_FILE;
    string path;

    FILE *fin = fopen(arg,"r");
    _log("Openning File: %s", arg);
    parseHeader(fin,path);

    char probe[5000], sample[5000];
    while(fscanf(fin," %s %s",probe,sample) != EOF){
        string front(probe);
        string back(sample);

        if(front.empty() or back.empty()) continue;
        out.emplace_back(path+front,path+back);
    }

    if(!out.size()) return FL_ERR_INVALID_DATA;

    _log("Collected Pairs");
    return FL_ERR_NONE;
}

void *read_file(const string &path ,int64_t &size){
    char *R = NULL;
    FILE *f;
    size = file_size(path);
    if(size > 0){
        R = (char*) malloc(size);
        if(R == NULL){
            size = 0;
            throw(ENOMEM);
        }
        f = fopen(path.c_str(),"rb");
        if(f==NULL){
            size = 0;
            free(R);
            throw(errno);
        }
        if(fread(R,1,size,f) != (size_t) size){
            free(R);
            throw(errno);
        }
        fclose(f);
        return ((void *)R);
    }
    
    return NULL;
    
}

void write_file(const string &path, const void *buffer, int size) {
  FILE *f;
  if (buffer==NULL) throw(EINVAL);
  f = fopen(path.c_str(),"wb");
  if (f==NULL) throw(errno);
  if (fwrite(buffer, 1, size, f) != (size_t) size) { fclose(f); throw(errno); }
  fclose(f);
}

FL_ErrorCode create_image(FL_Session *s, string path, FL_Image *&img){
    int64_t size = 0;
    void* data = NULL;

    try{
        data = read_file(path,size);
    } catch(int ecode) {
        _warn("** unable to read file %s, errno=%d", path.c_str(), ecode);
        return FL_ERR_FAILED_TO_OPEN_FILE;
    }

    FL_ErrorCode err = fl_create_image(s, reinterpret_cast<byte *>(data), size, &img);
    free(data);
    return err;
}

template<typename Iter, typename RandomGenerator> Iter select_randomly(Iter start, Iter end, RandomGenerator &g){
    std::uniform_int_distribution<> dis(0,std::distance(start,end) - 1);
    std::advance(start,dis(g));
    return start;
}

template<typename Iter> Iter select_randomly(Iter start, Iter end){
    static std::random_device rd;
    static std::mt19937 gen(42);//gen(rd());
    return select_randomly(start,end,gen);
}

void drawLandmarks(cv::Mat &image, Grid<float> grid, int thickness = -1){
    // grid[:,5:15] are the landmarks idx
    for(int i = 0; i < grid.rows(); i++){
        cv::circle(image, cv::Point(grid(5,i), grid(6,i)), 2, cv::Scalar(0, 0, 255), thickness);
        cv::circle(image, cv::Point(grid(7,i), grid(8,i)), 2, cv::Scalar(0, 255, 255), thickness);
        cv::circle(image, cv::Point(grid(9,i), grid(10,i)), 2, cv::Scalar(255, 0, 255), thickness);
        cv::circle(image, cv::Point(grid(11,i), grid(12,i)), 2, cv::Scalar(0, 255, 0), thickness);
        cv::circle(image, cv::Point(grid(13,i), grid(14,i)), 2, cv::Scalar(255, 0, 0), thickness);
    }
}

map<string,FL_Template> extract_templates(FL_Session *s, RetinaModel model, vector<string> files, string extract_dir,string log_dir, bool confidence, bool skip = false, bool time = false, string time_output = ""){
    map<string,FL_Template> out;
    vector<pair<string,string>> perr(0);
    vector<custom_time> times;
    vector<custom_time> det_times;
    int count = 0;
    double p25 = files.size()*0.25, p50 = files.size()*0.5, p75 = files.size()*0.75;
    ofstream conf_os;
    if (confidence) conf_os.open("/tmp/"+to_string(getpid())+"_confidence.txt");
    auto init_time = std::chrono::high_resolution_clock::now();

    for (auto file : files){
        if(debug) os_debug("Opening File:",file);
        FL_Image *flimg;
        cv::Mat im = cv::imread(file);

        FL_ErrorCode err = create_image(s,file,flimg);
        count++;
        if(err!=FL_ERR_NONE){
            _warn("Couldn't Create FL_Image from %s",file.c_str());
            if(debug) os_debug(fl_err_str(err));
            perr.emplace_back(file,fl_err_str(err));
            continue;
        }
        FL_Face face;
        Grid<float> infs;
        TICK(retina_detect);
        try{
            if(model.getInference(im, infs)){
                os_warn("Couldn't Run RetinaFace inference in",file);
                perr.emplace_back(file, fl_err_str(FL_ERR_FAILED_TO_OPEN_FILE));
                fl_destroy_image(s, flimg);
                continue;
            }
            if(!infs.rows()){
                os_warn("Couldn't find faces in",file);
                perr.emplace_back(file, fl_err_str(FL_ERR_NO_FACE_FOUND));
                fl_destroy_image(s, flimg);
                continue;
            }

            // os_log("Image Size:",im.cols,im.rows);
            for(int i = 0; i < 5; i++){
                face.landmarks[i].x = infs((i*2)+5, 0);
                face.landmarks[i].y = infs((i*2)+6, 0);
            }
        } catch (const char* msg){
            os_error(msg, "at", __FILE__, __LINE__); 
            perr.emplace_back(file, fl_err_str(FL_ERR_NO_FACE_FOUND));
            fl_destroy_image(s, flimg);
            continue;
        }

        if(time)
            det_times.push_back(get_execution_time(retina_detect));

        // os_log("Aligning Face");
        err = fl_align(s, face, &flimg);
        FL_Template tpt;
        // os_log("Extracting Template");
        TICK(single_extract);
        err = fl_extract_one(s,flimg,skip,&tpt,&face);

        if(err!=FL_ERR_NONE){
            _warn("Couldn't extract FL_Template from %s",file.c_str());
            if(debug) os_debug(fl_err_str(err));
            perr.emplace_back(file,fl_err_str(err));
            err = fl_destroy_image(s,flimg);
            assert(err == FL_ERR_NONE);
            continue;
        }

        if(time)
            times.push_back(get_execution_time(single_extract));
            

        if(confidence){
            conf_os << get_filename(file) << " " << face.confidence << endl;
            err = fl_destroy_image(s,flimg);
            assert(err == FL_ERR_NONE);
            continue;
        }

        byte *ser = NULL;
        int ser_size = 0;

        // os_log("Serializing Template");
        err = fl_serialize(s,&tpt,&ser,&ser_size);
        if(err!=FL_ERR_NONE){
            _warn("Couldn't serialize extracted FL_Template from %s",file.c_str());
            perr.emplace_back(file,fl_err_str(err));
            err = fl_destroy_image(s,flimg);
            assert(err == FL_ERR_NONE);
            continue;
        }

        if(extract_dir.empty()) extract_dir = get_filedir(file) + "tpt";
        //if(extract_dir.find("/home/")==string::npos) extract_dir = get_filedir(file) + extract_dir; //TODO Fix this problem where /home/ is required in path else the directory is doubled in the final extract_dir

        if(!is_dir(extract_dir)) assert(!mkdir(extract_dir));

        try{
            write_file(extract_dir+'/'+swap_extension(get_filename(file),".ftg"),ser,ser_size);
            if(debug) os_debug("Writting template file to",extract_dir+'/'+swap_extension(get_filename(file),".ftg"));
        } catch (int ecode){
            _warn("** unable to write template to %s, errno=%d", (extract_dir+get_filename(file)).c_str(), ecode);
            stringstream ss; ss << "Exception Code: " << ecode;
            perr.emplace_back(file,ss.str());
        }
        
        if(count <= p25 && count+1 > p25) os_log("Template Extraction Progress: 25%");
        if(count <= p50 && count+1 > p50) os_log("Template Extraction Progress: 50%");
        if(count <= p75 && count+1 > p75) os_log("Template Extraction Progress: 75%");

        out.emplace(dir_id ? get_filedir(get_filename(file))+'/'+get_filename(remove_extension(file)) : get_filename(remove_extension(file)),tpt);
        assert(fl_destroy_image(s,flimg)==FL_ERR_NONE);
        assert(fl_destroy_byte_array(s,ser)==FL_ERR_NONE);
        
        // os_log("Image Size:",im.cols,im.rows);
        // drawLandmarks(im, infs);
        // cv::imshow("teste",im);
        // cv::waitKey(0);
    }

    if(!perr.empty()){
        if(get_filedir(files.front()).empty()) log_dir = "/tmp";
        else if(log_dir.empty()) log_dir = get_filedir(files.front())+"extract_error.txt";
        else if(log_dir.find("/home/")==string::npos) log_dir = get_filedir(files.front())+log_dir;
        else if(!is_dir(log_dir) && !has_extension(log_dir)) mkdir(log_dir);
        if(!has_extension(log_dir)) log_dir.back()=='/' ? log_dir+="extract_error.txt" : log_dir+="/extract_error.txt";

        ofstream os(log_dir);
        for(auto pair : perr)
            os << pair.first << " " << pair.second << endl;
        os << "Extraction Rate: " << setprecision(4) << (double)(perr.size()/files.size()) << endl;
        os.close();
        os_log("Extract Error Log saved at",log_dir);
    }

    if(!times.empty()){
        if(time_output.empty()) time_output = "/tmp/extraction_times.txt";
        else if(!has_extension(time_output)){
            if(!is_dir(time_output)) 
                mkdir(time_output);
        }
        else if(has_extension(time_output)){
            if(!is_dir(get_filedir(time_output))) mkdir(get_filedir(time_output));
        }

        ofstream time_file(time_output);
        for(auto i = 0; i < files.size() && i < times.size(); i++){
            time_file << files[i] << "\t" << "Detection: " << setw(2) << setfill('0') << det_times[i].hours << ':' << setw(2) << setfill('0') << det_times[i].minutes << ":" << setw(2) << setfill('0') << det_times[i].seconds << ":" << setw(6) << setfill('0') << det_times[i].microseconds << 
            "\tExtraction: " << setw(2) << setfill('0') << times[i].hours << ':' << setw(2) << setfill('0') << times[i].minutes << ":" << setw(2) << setfill('0') << times[i].seconds << ":" << setw(6) << setfill('0') << times[i].microseconds << "\n";
        }
        time_file.close();
        os_log("Execution Time Saved to", time_output);
    }

    conf_os.close();

    if(save_info){
        FILE *f_out = fopen(o_output.c_str(),"a");

        fprintf(f_out, "EXTRACT: -----------------------\n");
        fprintf(f_out, "IMGS: %d\n",count);
        fprintf(f_out, "T_EXT[ms]: %.2f\n",static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - init_time).count()) / (count * 1000.0));

        fclose(f_out);
    }


    return out;
}

static FL_ErrorCode read_templ_from_file(FL_Session *s,const char *filename, FL_Template &tpt) {
    byte * buffer = 0;
    long length;

	if(debug) _debug("Trying to read file %s.", filename);
    if(!is_file(filename)) return FL_ERR_FAILED_TO_OPEN_FILE;
    FILE * f = fopen (filename, "rb");

    if (f) {
		length = 2053;
		buffer = (byte *) malloc (length);
        if (buffer) {
            if(!fread (buffer, 1, length, f)) return FL_ERR_FAILED_TO_OPEN_FILE;
        }
        fclose (f);
    }

	// FL_Template tpt = { 0 };
    if (buffer) {
        FL_ErrorCode err = fl_deserialize(s, buffer, length, &tpt);
		if(err != FL_ERR_NONE) return err;
    }

    return FL_ERR_NONE;
}

void read_templates(FL_Session *s,vector<pair<string,string>> pairs, map<string,FL_Template> &out){

    for(auto pair : pairs){
        FL_Template tmp;
        if(out.find(pair.first)==out.end()){
            FL_ErrorCode err = read_templ_from_file(s,pair.first.c_str(),tmp);
            if(err!=FL_ERR_NONE){
                os_warn("Error trying to read template ", pair.first,fl_err_str(err));
                continue;
            }
            out.emplace(pair.first,tmp);
        }
        if(out.find(pair.second)==out.end()){
            FL_ErrorCode err = read_templ_from_file(s,pair.second.c_str(),tmp);
            if(err!=FL_ERR_NONE){
                os_warn("Error trying to read template ", pair.second, fl_err_str(err));
                continue;
            }
            out.emplace(pair.second,tmp);
        }
    }
}

void read_templates(FL_Session *s, vector<string> files, map<string,FL_Template> &out, bool only_name = true){
    for(auto file : files){
        FL_Template tmp;
        if(out.find(file) == out.end()){
            if(read_templ_from_file(s,file.c_str(),tmp)!=FL_ERR_NONE){
                os_warn("Error trying to read template",file);
                continue;
            }
            only_name ? out.emplace(get_filename(remove_extension(file)),tmp) : out.emplace(file,tmp);
        }
    }
}

void parallel_pairing(ofstream &os,vector<pair<string,string>> &input,vector<string> keys, unsigned int begin, unsigned int end, bool same, int num_imp, bool dir_id, char delimiter){
    os_log("Thread(",this_thread::get_id(),") Initializing",(same ? "Genuines" : "Impostors"),"Pairing. Begin Index:",begin, "End Index:",end > keys.size()-1 ? keys.size()-1 : end);
    vector<string>::iterator it;
    for(it = keys.begin()+begin; it!= keys.end() && keys.begin()+end-it > 0; it++){
        string id = !dir_id ? get_filename(*it).substr(0,get_filename(*it).find_first_of(delimiter)) : get_filename(get_filedir(*it));
        vector<string>::iterator aux_it;
        vector<string> aux_copy;
        copy(it+1,keys.end(),back_inserter(aux_copy));
        if(same){
            for(aux_it = aux_copy.begin(); aux_it != aux_copy.end(); aux_it++){
                string next_id = !dir_id ? get_filename(*aux_it).substr(0,get_filename(*aux_it).find_first_of(delimiter)) : get_filename(get_filedir(*aux_it));
                // os_debug(id,next_id);
                if(id != next_id) continue;
                locker.lock();
                os << *it << " " << *aux_it << endl;
                input.emplace_back(*it,*aux_it);
                locker.unlock();
            }
        }
        else
        {
            random_shuffle(aux_copy.begin(),aux_copy.end());
            for(aux_it = aux_copy.begin(); (aux_it != aux_copy.end()) && (num_imp==-1 ? true : aux_it - aux_copy.begin() < num_imp); aux_it++){
                string next_id = !dir_id ? get_filename(*aux_it).substr(0,get_filename(*aux_it).find_first_of(delimiter)) : get_filename(get_filedir(*aux_it));
                if(id == next_id) continue;
                locker.lock();
                os << *it << " " << *aux_it << endl;
                input.emplace_back(*it,*aux_it);
                locker.unlock();
            }
        }
        
    }
    os_log("Thread(",this_thread::get_id(),") Finished Pairing Process.");
}

vector<pair<string,string>> get_pairs(vector<string> keys, bool same, int num_imp, bool dir_id, char delimiter = '_', string group = ""){
    vector<pair<string,string>> pairs;
    stringstream log; log << "/tmp/" << getpid() << (same ? "_gen" : "_imp") << "_pairs_" << group+".txt";
    ofstream os(log.str());
    if(keys.empty()){
        os_error("Empty key vector, possible failure loading templates.");
        return pairs;
    }
    unsigned int chuck_size = round((float)keys.size()/(float)num_threads)+1;
    vector<thread> threads;
    for(unsigned int i = 0; i < num_threads; i++){
        threads.emplace_back(parallel_pairing,std::ref(os),std::ref(pairs),keys,chuck_size*i,chuck_size*(i+1),same,num_imp,dir_id,delimiter);
    }
    for(auto &t : threads)
        t.join();
    
    os.close();
    os_result("Pairing results stored on log:",log.str());

    os_log("Sorting Pairs by reference");
    sort(pairs.begin(),pairs.end(),[](const pair<string,string> &p1, const pair<string,string> &p2){
        return p1 < p2;
    });
    return pairs;
}

void check_templates_corruption(map<string,FL_Template> tpts){
    if(!tpts.empty()){
        stringstream ss;
        ss << "/tmp/" << getpid() << "_check.txt";
        ofstream os(ss.str());
        for(auto tpt : tpts){
            bool corruption = false;
            for(auto i=0; i < FL_TEMPLATE_DATA_SIZE; i++){
                if(tpt.second.data[i]<-1 || tpt.second.data[i]>1){
                    os << get_filename(tpt.first) << '(' << i << ',' << tpt.second.data[0] << ") ";
                    corruption = true;
                }
            }
            if(corruption)
                os << endl;
        }
        os_log("Templates Corruption Check stored in",ss.str());
        os.close();
    }
}

void check_id(vector<string> keys, bool &dir_id, bool &file_id, char delimiter){
    string path = get_filedir(keys.front());
    string id = get_filename(keys.front());
    int option = 0;
    id = id.substr(0,id.find_first_of(delimiter));

    os_log("Checking the input filenames to determine if ID is located on filename or directory");
    for(auto key : keys){
        if(!path.empty() and get_filedir(key)!=path) dir_id = true;
        if(!id.empty() and get_filename(key).substr(0,get_filename(key).find_first_of(delimiter))!=id) file_id = true;
    }

    // os_debug("dir_id:",dir_id ? "True" : "False", ", file_id:",file_id ? "True" : "False");
    //Ask if the id is in the file or in the directory
    if(dir_id and file_id){
        _warn("Both different directory and different ID found on filenames.");
        _log("Proceed with: \n\t\t1. Id on directory\n\t\t2. Id on file\n");
        cout << "\t\tOption: ";
        cin >> option;
        do{
            switch (option)
            {
            case 1:
                file_id = false;
                break;
            case 2:
                dir_id = false;
                break;
            default:
                _warn("Invalid Option");
                _log("Proceed with: \n\t\t1. Id on directory\n\t\t2. Id on file\nOption:");
                cin >> option;
                break;
            }
        } while(option != 1 && option != 2);
    }

    if (!dir_id && !file_id) file_id = true;
}

//unfinished
void build_matching_lists(map<string,FL_Template> const &templates, vector<pair<string,string>> &gen_pairs, vector<pair<string,string>> &imp_pairs, char delimiter = '_',int num_imp = -1){
    vector<string> keys;
    for(auto file : templates){
        keys.push_back(file.first);
    }
    
    gen_pairs = get_pairs(keys,true,num_imp,dir_id,delimiter);
    imp_pairs = get_pairs(keys,false,num_imp,dir_id,delimiter);

    if(result) os_debug("Gen Pairs:",gen_pairs.size());
    if(result) os_debug("Imp Pairs:",imp_pairs.size());

}

void compare_batch(FL_Session *s, vector<pair<string,string>> pairs, map<string,FL_Template> tpt_table, const char *group, bool cosine,vector<MatchPair> &out){
    FL_ErrorCode err = FL_ERR_NONE;
    int index = 0;
    for(auto pair : pairs){
        float score = 0.0;
        if(tpt_table.find(pair.first) == tpt_table.end() or tpt_table.find(pair.second) == tpt_table.end()){
            _warn("Couldn't find template of pair on template table.\n\t*%s: %s\n\t*%s: %s",pair.first.c_str(), tpt_table.find(pair.first) == tpt_table.end() ? "Not Found" : "Found" ,pair.second.c_str(), tpt_table.find(pair.second) == tpt_table.end() ? "Not Found" : "Found");
            continue;
        }
        if(!cosine){
            err = fl_compare(s,tpt_table[pair.first],tpt_table[pair.second],&score);
            if(err != FL_ERR_NONE){
                _warn("Couldn't compare templates %s,%s. FL_Error: %s",get_filename(pair.first).c_str(),get_filename(pair.second).c_str(),fl_err_str(err));
                continue;
            }
        } else {
            err = fl_compare_cosine(s,tpt_table[pair.first],tpt_table[pair.second],&score);
            if(err != FL_ERR_NONE){
                _warn("Couldn't compare templates %s,%s. FL_Error: %s",get_filename(pair.first).c_str(),get_filename(pair.second).c_str(),fl_err_str(err));
                continue;
            }
        }
        if(result) os_result(pair.first,pair.second,group,score);
        out.push_back(MatchPair(index++,pair.first,pair.second,score));
    }
}

void compare_batch(FL_Session *s, vector<pair<string,string>> pairs, map<string,FL_Template> tpt_table, const char *group, bool cosine,vector<MatchPair> &out, vector<custom_time> &times){
    FL_ErrorCode err = FL_ERR_NONE;
    int index = 0;
    for(auto pair : pairs){
        custom_time time(0,0,0,0);
        float score = 0.0;
        if(tpt_table.find(pair.first) == tpt_table.end() or tpt_table.find(pair.second) == tpt_table.end()){
            _warn("Couldn't find template of pair on template table.\n\t*%s: %s\n\t%s: %s",pair.first.c_str(), tpt_table.find(pair.first) == tpt_table.end() ? "Not Found" : "Found" ,pair.second.c_str(), tpt_table.find(pair.second) == tpt_table.end() ? "Not Found" : "Found");
            continue;
        }
        if(!cosine){
            TICK(for_match);
            err = fl_compare(s,tpt_table[pair.first],tpt_table[pair.second],&score);
            time = get_execution_time(for_match);
            if(err != FL_ERR_NONE){
                _warn("Couldn't compare templates %s,%s. FL_Error: %s",get_filename(pair.first).c_str(),get_filename(pair.second).c_str(),fl_err_str(err));
                continue;
            }
        } else {
            TICK(for_match);
            err = fl_compare_cosine(s,tpt_table[pair.first],tpt_table[pair.second],&score);
            time = get_execution_time(for_match);
            if(err != FL_ERR_NONE){
                _warn("Couldn't compare templates %s,%s. FL_Error: %s",get_filename(pair.first).c_str(),get_filename(pair.second).c_str(),fl_err_str(err));
                continue;
            }
        }
        if(result) os_result(pair.first,pair.second,group,score);
        out.push_back(MatchPair(index++,pair.first,pair.second,score));
        // stringstream tss;
        // tss << "Compare Time: " << time.seconds << " seconds " << setfill('0') << setw(6) << time.microseconds << " microseconds" << endl;
        // os_debug(tss.str());

        times.push_back(time);
    }
}

void save_match(vector<MatchPair> in, string out_dir, string filename, vector<custom_time> times = vector<custom_time>()){
    
    if(out_dir.empty()) { out_dir = get_filedir(filename);  filename = "compare_" + get_filename(filename);}
    if(out_dir.empty()){ _warn("No output directory given to save match result, saving on /tmp"); out_dir = "/tmp";};
    if(!has_extension(out_dir)){
        if(!is_dir(out_dir)) assert(mkdir(out_dir));
        out_dir.back()=='/' ? out_dir+=filename : out_dir+='/'+filename;
    }
    else{
        if(!is_dir(get_filedir(out_dir))) assert(mkdir(get_filedir(out_dir)));
    }

    ofstream os(out_dir);
    vector<custom_time>::iterator tit = times.begin();
    for(auto match : in){
        os << match.index << " " << match.query << " " << match.ref << " " << match.score;
        if(tit!=times.end()){os << " " << tit->seconds*1000000 + tit++->microseconds;}
        if(match.index != in.back().index) os << endl;
    }

    os.close();
    if(!in.empty())
        os_log("Matching results saved to",out_dir);
}

void save_match_norm(vector<MatchPair> in, string out_dir, string filename){
    if(out_dir.empty()) { out_dir = get_filedir(filename);  filename = "normalized_compare_" + get_filename(filename);}
    if(out_dir.empty()){ _warn("No output directory given to save match result, saving on /tmp"); out_dir = "/tmp";};
    if(!has_extension(out_dir)){
        if(!is_dir(out_dir)) assert(mkdir(out_dir));
        out_dir.back()=='/' ? out_dir+=filename : out_dir+='/'+filename;
    }
    else{
        if(!is_dir(get_filedir(out_dir))) assert(mkdir(get_filedir(out_dir)));
    }

    ofstream os(out_dir);
    for(auto match : in){
        os << match.index << " " << match.query << " " << match.ref << " " << match.score;
        if(match.index != in.back().index) os << endl;
    }

    os.close();
    os_log("Normalized matching results saved to",out_dir);
}

void save_rejected(vector<MatchPair> in, string out_dir, string filename, float threshold){
    if(out_dir.empty()) { out_dir = get_filedir(filename); filename.empty() ? filename = "rejected_"+to_string(getpid())+".txt" : filename="rejected_"+get_filename(filename);}
    if(out_dir.empty()){ os_warn("No output directory given to save match result, saving on /tmp"); out_dir="/tmp";};
    if(!has_extension(out_dir)){
        if(!is_dir(out_dir)) assert(mkdir(out_dir));
        out_dir.back()=='/' ? out_dir+=filename : out_dir+='/'+filename;
    } else 
        if(!is_dir(get_filedir(out_dir))) assert(mkdir(get_filedir(out_dir)));
    
    ofstream os(out_dir);
    for(auto match : in){
        if(match.score < threshold){
            os << match.index << " " << match.query << " " << match.ref << " " << setprecision(2) << match.score << endl;
        }
    }

    os.close();
    os_log("Rejected Genuines results saved to",out_dir);
}

void normalize_matches(vector<MatchPair> &gen, vector<MatchPair> &imp){
    // float min = __FLT_MAX__, max = 1.0;
    float min = -1.0, max = 1.0;
    for(auto g : gen){
        if(g.score < min) min = g.score;
    }
    for(auto i: imp){
        if(i.score < min) min = i.score;
    }

    for(auto &g : gen){
        g.score = (g.score-min)/(max-min);
        // if(result) os_result(g.score);
    }
    for(auto &i : imp){
        i.score = (i.score-min)/(max-min);
        // if(result) os_result(i.score);
    }
}

void accuracy_test(vector<MatchPair> genuines, vector<MatchPair> impostors, unsigned int rank, char delimiter = '_'){
    //criar map <int,string> para mapear os templates reduzindo custo de memória?
    map<string,map<string,float >> accuracy_map; 
    map<string,int> rank_map;
    float acc = 0.0;
    int count = 0;

    os_debug("Gen Scores:",genuines.size(),"Imp Scores:",impostors.size());
    if(genuines.empty()){
        os_error("Empty genuines MatchPairs");
        return;
    }

    _log("Populating the accuracy map and Inserting Genuine Scores");
    for(auto gen : genuines){
        if(accuracy_map.find(gen.query)==accuracy_map.end())
            accuracy_map.emplace(gen.query,map<string,float>());
        if(accuracy_map.find(gen.ref)==accuracy_map.end())
            accuracy_map.emplace(gen.ref,map<string,float>());
        else{
            accuracy_map[gen.query].emplace(gen.ref,gen.score);
            accuracy_map[gen.ref].emplace(gen.query,gen.score);
        }
    }

    _log("Inserting Impostors Scores");
    for(auto imp : impostors){
        if(accuracy_map.find(imp.query)!=accuracy_map.end()){
            accuracy_map[imp.query].emplace(imp.ref,imp.score);
        }
        if(accuracy_map.find(imp.ref)!=accuracy_map.end()){
            accuracy_map[imp.ref].emplace(imp.query,imp.score);
        }
    }

    _log("Sorting the scores and retrieving accuracies");
    

    for(auto i : accuracy_map){
        
        vector<pair<string,float>> order(0);
        for(auto pair : i.second){
            order.push_back(pair);
        }

        sort(order.begin(),order.end(),[](pair<string,float> p1, pair<string,float> p2){
            return p1.second > p2.second;
        });

        float score = __FLT_MAX__;
        unsigned int same_score = 0;
        for(unsigned int j = 0; j < order.size(); j++){
            string id1 = "1", id2 = "2";
            if(dir_id){
                id1 = get_filename(get_filedir(i.first));
                id2 = get_filename(get_filedir(order[j].first));
            }
            if(file_id){
                id1 = get_filename(i.first).substr(0,get_filename(i.first).find_first_of(delimiter));
                id2 = get_filename(order[j].first).substr(0,get_filename(order[j].first).find_first_of(delimiter));
            }
            if(order[j].second == score){
                same_score++;
            }
            if(id1 == id2){
                // cout << id1 << " " << id2 << " " << order[j].second << endl;
                if(j-same_score<rank) count++;
                rank_map.emplace(i.first,j-same_score);
                break;
            }
            score = order[j].second;
        }

        // if(debug) os_debug(i.first," pairs vector size = ",order.size());
    }

    acc = (float) count/(float) accuracy_map.size();
    os_result("Accuracy",setprecision(4),acc*100,"%");

    stringstream ss;
    ss << "/tmp/" << getpid() << "_rank.txt";
    ofstream os(ss.str());
    os_log("Saving Output log of rank in",ss.str());
    for(auto key : rank_map){
        os << get_filename(remove_extension(key.first)) << " " << key.second + 1 << endl;
    }
    os.close();
}

void split_accuracy_test(vector<MatchPair> genuines, vector<MatchPair> impostors, int rank, char delimiter = '_'){
    float acc = 0.0;
    int count = 0;
    int samples = 0;
    unsigned int gen_count = genuines.size();
    stringstream ss; ss << "/tmp/" << getpid() << "_rank.txt";
    ofstream os(ss.str());

    if(!gen_count){
        os_error("No Genuines found, check the class identifier delimiter (current",delimiter,") to avoid errors");
        return;
    }

    vector<MatchPair>::iterator it = genuines.begin();
    os_debug("Last pair",genuines.back().query,genuines.back().ref);
    os_debug("Genuine Pairs:",genuines.size());
    os_debug("Impostors Pairs:",impostors.size());

    for(; it != genuines.end(); it++){
        string current = it->query;
        vector<pair<string,float>> scores(0);
        vector<MatchPair>::iterator auxit;

        if(debug) os_debug(it-genuines.begin(),"Preparing",current, "list of scores");
        for(auxit = genuines.begin(); auxit != genuines.end(); auxit++){
            if(auxit->query == current)
                scores.emplace_back(auxit->ref,auxit->score);
            if(auxit->ref == current)
                scores.emplace_back(auxit->query,auxit->score);
        }

        for(auxit = impostors.begin(); auxit != impostors.end(); auxit++){
            if(auxit->query == current)
                scores.emplace_back(auxit->ref,auxit->score);
            if(auxit->ref == current)
                scores.emplace_back(auxit->ref,auxit->score);
        }

        sort(scores.begin(),scores.end(),[](pair<string,float> first, pair<string,float> second){
            return first.second > second.second;
        });

        vector<string> keys(0);
        for(auto key : scores)
            keys.push_back(key.first);
        samples = keys.size();
        keys.clear();

        float score = __FLT_MAX__;
        int same_score = 0;

        for(auto i = 0; i < samples; i++){
            string id1 = "1", id2 = "2";
            if(dir_id){
                id1 = get_filename(get_filedir(current));
                id2 = get_filename(get_filedir(scores[i].first));
            }
            if(file_id){
                id1 = get_filename(current).substr(0,get_filename(current).find_first_of(delimiter));
                id2 = get_filename(scores[i].first).substr(0,get_filename(scores[i].first).find_first_of(delimiter));
            }
            if(scores[i].second == score) same_score++;
            if(it == genuines.end()-1)
                os_result(id1,id2,scores[i].second);
            if(id1 == id2){
                if(i-same_score < rank)
                    count++;
                os << current << " " << i-same_score+1 << endl;
                break;
            }

        }

    }

    acc = (float) count / (float) gen_count;
    os_result("Accuracy",setprecision(4),acc*100,"%");
    os_result("In Rank",rank,":",count,"Total Genuines:",gen_count,"Samples",samples);

    os_log("Saving Output log of rank in",ss.str());
    os.close();
}

void refence_selection(map<string,FL_Template> &in, map<string,bool> is_genuines, map<string,vector<string>> &groupedby_id, map<string,FL_Template> &refs,char delimiter = '_'){
    stringstream list;
    list << "/tmp/" << getpid() << "_references.txt";
    for(auto tpt : in){
        string id;
        if(dir_id)
                id = get_filename(get_filedir(tpt.first));
        if(file_id)
                id = get_filename(tpt.first).substr(0,get_filename(tpt.first).find_first_of(delimiter));
        groupedby_id[id].push_back(tpt.first);            
    }
    //TODO
    //for each id select random sample as ref - DONE
    ofstream os(list.str());
    for (auto samples : groupedby_id){
        vector<string>::iterator it = select_randomly(samples.second.begin(),samples.second.end());
        os << samples.first << " " << *it << endl;
    
        refs[*it] = in[*it];
        // in.erase(*it);
    }
    //remove ref from in, and append to refs - DONE

    os.close();
    os_log("Reference list saved to",list.str());

}

void get_map_accuracy(FL_Session *s,map<string,FL_Template> split,map<string,FL_Template> all, map<string,bool> is_genuine, bool cosine, unsigned int rank, unsigned int &count, unsigned int &gen_count, ofstream &os, ofstream &of, char delimiter, map<string,vector<string>> groupedby_id, bool nranks, vector<unsigned int> &ranks){
    os_log("Thread(",this_thread::get_id(),") Initializing accuracy operation. Subset Size:",split.size());
    map<string,FL_Template>::iterator it;
    unsigned int thread_count = 0;
    unsigned int gen_thread_count = 0;

    for(map<string,FL_Template>::iterator it = split.begin(); it != split.end(); it++){
        unsigned int expected = 0;
        if(is_genuine.find(it->first)==is_genuine.end()) continue;
        gen_thread_count++;
        vector<pair<map<string,FL_Template>::iterator,float>> scores;
        for(map<string,FL_Template>::iterator aux = all.begin(); aux != all.end(); aux++){
            float score = 0.0;
            FL_ErrorCode err;
            if(it->first == aux->first) continue;

            if(!cosine){
                err = fl_compare(s,it->second,aux->second,&score);
            } else {
                err = fl_compare_cosine(s,it->second,aux->second,&score);
            }
            if(err != FL_ERR_NONE){
                os_warn("Failed to compare Templates",it->first, aux->first,'.',fl_err_str(err));
                continue;
            }
            scores.emplace_back(aux,score);
        }

        sort(scores.begin(),scores.end(),[](pair<map<string,FL_Template>::iterator,float> p1,pair<map<string,FL_Template>::iterator,float> p2){
            return p1.second > p2.second;
        });

        for(unsigned int i = 0; i < scores.size(); i++){
            string id1="1", id2="2";
            if(dir_id){
                id1 = get_filename(get_filedir(it->first));
                id2 = get_filename(get_filedir(scores[i].first->first));
            }
            if(file_id){
                id1 = get_filename(it->first).substr(0,get_filename(it->first).find_first_of(delimiter));
                id2 = get_filename(scores[i].first->first).substr(0,get_filename(scores[i].first->first).find_first_of(delimiter));
            }
            if(id1==id2){
                locker.lock();
                gen_count++;
                locker.unlock();
                if(debug) os_debug(id1,id2);
                if(i < rank){
                    locker.lock();
                    count++;
                    thread_count++;
                    locker.unlock();
                } 
                locker.lock();
                os << it->first << " " << i+1 << " " << scores[i].first->first << endl;
                locker.unlock();

                if(nranks){
                    locker.lock();
                    ranks.push_back(i + 1);
                    locker.unlock();
                }
            
                for(; i < scores.size() && i < rank; i++){ //Get the number os samples found on rank N and the total samples of the ID
                    id2 = file_id ? get_filename(scores[i].first->first).substr(0,get_filename(scores[i].first->first).find_first_of(delimiter)) : get_filename(get_filedir(scores[i].first->first));
                    if(id1 == id2)
                        expected++;
                }
                locker.lock();
                of << it->first << " " << expected << " " << groupedby_id[id1].size()-1 << endl;
                locker.unlock();
                break;
            }
        }

    }
    os_log("Thread(",this_thread::get_id(),") Finished task.",thread_count,"samples found in Rank",rank, "of", gen_thread_count,"genuines.");
}

void memory_accuracy(FL_Session *s,map<string,FL_Template> extractions,bool cosine, unsigned int rank, bool nranks, char delimiter = '_'){
    float acc = 0.0;
    unsigned int count = 0;
    unsigned int gen_count = 0;
    unsigned int gen_pairs = 0;

    vector<string> keys;
    for(auto pair : extractions) keys.push_back(pair.first);

    _log("Building Genuine Pairs");
    vector<pair<string,string> >genuines = get_pairs(keys,true,-1,dir_id,delimiter);
    keys.clear();

    map<string,bool> is_genuine;
    for(auto gen : genuines){
        is_genuine[gen.first] = true;
        is_genuine[gen.second] = true;
    }
    gen_pairs = genuines.size();
    // gen_count = genuines.size();
    // if(!gen_count){
    //     os_error("No genuine pairs found on extracted templates. Check id delimiter (current",delimiter,") is correctly set for used dataset");
    //     return;
    // }
    genuines.clear();

    if(save_info){
        FILE *f_out = fopen(o_output.c_str(), "a");
        fprintf(f_out, "ACCURACY: ----------------------\n");
        fprintf(f_out, "QUERY: %lu\n", extractions.size());
        fprintf(f_out, "REF: %lu\n", extractions.size());
        fclose(f_out);

    }

    map<string,FL_Template> refs;
    map<string,vector<string>> groupedby_id;
    refence_selection(extractions,is_genuine,groupedby_id,refs,delimiter);

    {
        stringstream ss;
        ss << "/tmp/" << getpid() << "_grouped_by_id.txt";
        ofstream os(ss.str());
        for(auto group : groupedby_id){
            os << group.first << " ";
            for (auto sample : group.second){
                os << sample << " ";
            }
            os << endl;
        }
        os.close();
        os_log("Saving grouped by result in",ss.str());
    }

    stringstream ss; ss << "/tmp/" << getpid() << "_rank.txt";
    stringstream ss_multiacc; ss_multiacc << "/tmp/" << getpid() << "_expected.txt";
    ofstream os(ss.str());
    ofstream of(ss_multiacc.str());
    of << "Rank= " << rank << endl;
    os_log("Computing genuines matching Rank");

    auto init_time = std::chrono::high_resolution_clock::now();
    
    //IMPLEMENT THREADS HERE!
    vector<thread> threads;
    size_t subset_size = round((float) extractions.size()/ (float) num_threads) + 1;
    size_t ref = 0;
    map<string,FL_Template>::iterator rstart = extractions.begin();
    map<string,FL_Template>::iterator rend = extractions.begin();
    vector<unsigned int> ranks; // Vector to store search ranks in case of 'nrakns'.
    for(unsigned int k = 0; k < num_threads; k++){
        if(rend != extractions.begin()) rstart = rend;
        while(ref < subset_size*(k+1) && ref < extractions.size()) {ref++, rend++;/*os_debug("k=",k,"ref=",ref)*/;}
        map<string,FL_Template> subset{rstart,rend};
        threads.emplace_back(get_map_accuracy,s,subset,extractions,is_genuine,cosine,rank,std::ref(count),std::ref(gen_count),std::ref(os),std::ref(of),delimiter, groupedby_id, nranks, std::ref(ranks));
    }
    // get_map_accuracy(s,extractions,is_genuine,cosine,rank,acc,count,gen_count,os,delimiter);

    for(auto &t : threads){
        t.join();
    }    

    int duration_ms = (int) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - init_time).count();

    // Evaluate matches per second and matches per second per thread.
    int mps = duration_ms != 0 ? 1000 * is_genuine.size() * (extractions.size() - 1) / duration_ms : 0;
    int mpspt = mps / num_threads;

    if(save_info){
        FILE *f_out = fopen(o_output.c_str(), "a");
        fprintf(f_out, "MPS: %d\n", mps);
        fprintf(f_out, "MPSPT: %d\n", mpspt);
        fclose(f_out);
    }

    // Evaluate accuracy:
    if(!nranks){
        acc = 100.0 * (float) count/(float) gen_count;
        os_debug("acc =",count,"/",gen_count);
        os_result("RANK",rank,"Accuracy",setprecision(4),acc,"%");

    } else {
        map<unsigned int, float> ranks_percentage;
        vector<unsigned int> rank_values = {1, 10, 20, 30, 100, 1000};
    
        for(auto val : rank_values){

            unsigned int accum = 0;
            for(vector<unsigned int>::iterator it = ranks.begin(); it != ranks.end(); it++){
                if(*it <= val) accum++;
            }
            acc = 100.0 * (float) accum / ranks.size();
            ranks_percentage.emplace(val, acc);
            os_result("RANK",val,"Accuracy:",setprecision(4),ranks_percentage[val],"%");
        }

        if(save_info){
            FILE *f_out = fopen(o_output.c_str(), "a");
            for(auto val : rank_values){
                fprintf(f_out, "RANK%u: %.2f\n", val, ranks_percentage[val]);
            }
            fclose(f_out);
        }
    }

    acc = (float) count/(float) gen_count;
    os_debug("acc =",count,"/",gen_count);
    os_result("Accuracy",setprecision(4),acc*100,"%");
    os_result("Genuine Pairs:",gen_pairs);
    os_result("Impostors Pairs:",(extractions.size()*(extractions.size()-1)/2)-gen_pairs);
    os_log("Saving results of expected samples vs in_rank samples in",ss_multiacc.str());
    os_log("Saving results of rank in",ss.str());
    of.close();
    os.close();

}

cv::Mat create_image_from_buffer(void* buffer, int64_t size){
    if(!buffer){
        os_warn("Something wrong with image buffer");
    }
    cv::Mat rawData(1, size, CV_8UC1, buffer);
    cv::Mat original = cv::imdecode(rawData, cv::IMREAD_COLOR);

    return original;
}

void flip_test(FL_Session *s, vector<string> files){
    string out_file = "/tmp/flipped_test.txt";
    ofstream os(out_file);
    uint count = 0;

    os_log("Initializing Flip Test");
    
    for(auto file : files){
        void* buffer;
        int64_t sz;
        float score;

        // Read raw file from disk, decode image and flip original
        buffer = read_file(file, sz);
        cv::Mat mat = create_image_from_buffer(buffer, sz);
        cv::Mat flipped;
        cv::flip(mat, flipped, 1);

        // Encode image to jpg ir order to create FL_Image
        vector<uchar> original_encoded, flipped_encoded;
        cv::imencode(".jpg",mat,original_encoded);
        cv::imencode(".jpg",flipped, flipped_encoded);

        // Create FL_Images and check for errors
        FL_Image *img_original, *img_flipped;
        FL_ErrorCode err = fl_create_image(s, original_encoded.data(), original_encoded.size(), &img_original);
        if(err != FL_ERR_NONE){
            os_warn("Cannot create a FL_Image of file", file);
            continue;
        }
        err = fl_create_image(s, flipped_encoded.data(), flipped_encoded.size(), &img_flipped);
        if(err != FL_ERR_NONE){
            os_warn("Cannot create a flipped FL_Image of file", file);
            continue;
        }

        // Extract Templates from FL_Images and check for errors
        FL_Template tpt_original, tpt_flipped;
        FL_Face face_original, face_flipped;
        err = fl_extract_one(s, img_original, false, &tpt_original, &face_original);
        if(err != FL_ERR_NONE){
            os_warn("Cannot extract template from file", file);
            continue;
        }
        err = fl_extract_one(s, img_flipped, false, &tpt_flipped, &face_flipped);
        if(err != FL_ERR_NONE){
            os_warn("Cannot extract template from flipped of file", file);
            continue;
        }

        // Calculates the score between the templates and check for errors
        err = fl_compare_cosine(s, tpt_original, tpt_flipped, &score);
        if(err != FL_ERR_NONE){
            os_warn("Cannot compare templates of original and flipped of file", file);
            continue;
        }

        os << count++ << "\t" << file << "\t" << setprecision(2) << score << endl;

        fl_destroy_image(s,img_original);
        fl_destroy_image(s,img_flipped);
    }

    os.close();
    os_log("Saving results of flip test to", out_file);

}

//unfinished
int main(int argc, char** argv){
    GParser parser(
        "An implementation of a FaceLib full example to extract, match and retrieve the equal error rate (EER) analysis", //description
        "Extraction: ./fl_full -e --in_dir dir || --in_file file [options] \n\tMatching: ./fl_full -m -GI genuine_file impostors_file [options] \n\tAnalysis: ./fl_full -a -GI gen_scores_file imp_scores_file -A output_file [options]", //usage
        false); //parser debugmode

    string input, //input file or directory
        e_output, //output directory of extracted templates
        m_output, //output directory or file of matching results
        a_output, //output directory or file of analysis
        gen_file, //input genuines file in case of match
        imp_file, //input impostors file in case of match
        log_dir, //output dir of logs
        filter, //string for file filtering (in case of input is_dir)
        acc_output, //string for file saving the accuracy results
        exec_time_output; //string for file saving the execution time outputs
    bool extract = false, match = false, analyse = false, accuracy = false, confidence = false; //flags of operations
    bool avoid_overflow = false;
    bool cosine = false; //set if metric distance between templates is Cosine
    bool save_rank = false;
    bool nranks = false;
    bool byreference = false; //Flag to determine if accuracy should be by reference (1:N) or by samples (1:M), where N = number of identities and M = number of samples
    bool skip = true;
    bool fl_debug = false;
    bool version = false; //Flag to determine if FaceNet or ResNet (ArcFaces Model) will be used
    bool time = false;
    bool flip = false; // Flag to evaluate the similarity of flipped images
    int nimps = 5;
    unsigned int rank = 1;
    float threshold = 0.0;

    map<string,FL_Template> templates; //In memory templates of extract
    vector<MatchPair> gen_scores;
    vector<MatchPair> imp_scores;
    vector<custom_time> gen_match_time, imp_match_time;

    char base[1024] = "", info[1024] = "", delimiter = '_';

    FL_Session *s; //FaceLib Session
    RetinaModel retina; //RetinaFace model class

    GPARSER_ADD_HELP(parser,"-h,--help");
    GPARSER_ADD_FLAG(parser,"-D","Activates script Debug",debug);
    GPARSER_ADD_FLAG(parser,"-R","Activates result printing",result);
    GPARSER_ADD_OPTION(parser, "-i,--in_dir,--in_file","An input directory or file for desired operations", input);
    GPARSER_ADD_OPTION(parser,"-G,--genuines,--gen_file","An input file of genuines pairs for matching",gen_file);
    GPARSER_ADD_OPTION(parser,"-I,--impostors,--imp_file","An input file of impostors pairs for matching",imp_file);
    GPARSER_ADD_OPTION(parser,"-E,--extract_out","A directory to output the templates extracted",e_output);
    GPARSER_ADD_OPTION(parser,"-M,--match_out","A directory or filename to output the matching result",m_output);
    GPARSER_ADD_OPTION(parser,"-A,--analysis_out","A directory or file to output the analysis result",a_output);
    GPARSER_ADD_OPTION(parser,"-T,--accuracy_out","A directory or file to output the Rank-N accuracy test",acc_output);
    GPARSER_ADD_OPTION(parser,"-O,--info_out", "A directory or file to output information about extraction, matching, analysis and accuracy", o_output)
    GPARSER_ADD_OPTION(parser,"-f,--filter","A string expression to filter files during directory file retrieval",filter);
    GPARSER_ADD_OPTION(parser,"-l,--log_dir","A directory or file to output logs",log_dir);
    GPARSER_ADD_OPTION(parser,"-n,--num_imp","The numbers of impostors (per sample) in case of automatic match list generation",nimps);
    GPARSER_ADD_OPTION(parser,"-r,--rank","The rank level of the accuracy test",rank);
    GPARSER_ADD_OPTION(parser,"-d,--delimiter","A delimiter for class extraction from filename",delimiter);
    GPARSER_ADD_OPTION(parser,"-p,--threads","Number of threads to be used in the O(n²) match",num_threads);
    GPARSER_ADD_OPTION(parser,"-t,--threshold","A threshold to reject matches",threshold);
    GPARSER_ADD_OPTION(parser,"","A filename (or path) to save the results of execution time",exec_time_output);
    GPARSER_ADD_FLAG(parser,"-P,--time", "A flag to register extracting time of templates.", time);
    GPARSER_ADD_FLAG(parser,"--refs","A flag that determines if the accuracy is measured by reference or by samples total",byreference);
    GPARSER_ADD_FLAG(parser,"-F,--facelib_debug","Sets a flag to put facelib on debug mode", fl_debug);
    GPARSER_ADD_FLAG(parser,"-e,--extract","Sets the operation extract to be done",extract);
    GPARSER_ADD_FLAG(parser,"-m,--match","Sets the operation match to be done",match);
    GPARSER_ADD_FLAG(parser,"-a,--analyse","Sets the operation of EER analysis to be done",analyse);
    GPARSER_ADD_FLAG(parser,"-x,--acc","Sets the operation of Rank-N accuracy to be done",accuracy);
    GPARSER_ADD_FLAG(parser,"-o,--out", "Sets saving diverse info to be done", save_info);
    GPARSER_ADD_FLAG(parser,"-k,--confidence","Sets a flag to retrieve confidence of detected faces",confidence);
    GPARSER_ADD_FLAG(parser,"-c,--cosine","Sets the cosine distance metric to be used on matching",cosine);
    GPARSER_ADD_FLAG(parser,"-v,--version", "Flag to set the extractor model version. Flag False: FaceNet, Flag True: ResNet100",version);
    GPARSER_ADD_FLAG(parser,"--memory","Sets the memory flag to avoid memory overflow on accuracy \n\t\t(Note: This increases computational time considerably but is safer).",avoid_overflow);
    GPARSER_ADD_FLAG(parser,"--flip","Flag to set the flip test", flip);
    GPARSER_ADD_FLAG(parser,"--nranks", "Sets accuracy operation to evaluate ranks 1, 10, 20, 30, 100 and 1000 accuracy", nranks);

    GPARSER_PARSE(parser,argc,argv);
        
    os_log("Initializing Retina Face Model");
    retina.init("model/retinaface_dynamic.onnx", debug, ORT_ENABLE_ALL, 0);

    os_log("Initializing FaceLib");
    fl_init(FL_PRESET_DEFAULT,fl_debug ? FL_LOG_LEVEL_DEBUG : FL_LOG_LEVEL_NONE,&s);

    if(flip){
        TICK(Flip)
        if(input.empty()){
            os_error("No input given to test flipped results. Ending script execution.");
            return EXIT_FAILURE;
        }

        vector<string> files;
        if(is_dir(input)){
            os_log("Retrieving images for flip test from directory", input);
            if(str_split(filter,",").size() > 1){
                for(auto f : str_split(filter,",")){
                    vector<string> aux = list_files_from_directory_sorted(input, f);
                    files.insert(files.end(), aux.begin(), aux.end());
                }
            }
            else files = list_files_from_directory_sorted(input, filter);
            flip_test(s, files);
        }
        else if(is_file(input)){
            os_log("Retrieving Files from list",input);
            if(retrieve_files(input.c_str(),files)!=FL_ERR_NONE){
                os_error("Error retrieving files from file ",input);
                return EXIT_FAILURE;
            }

            os_log("Flip testing for retrieved files");
            flip_test(s, files);
        }
        TOCK(Flip)
    }

// check for save info location
    if(save_info){
        if(o_output.empty()){
            _warn("No directory passed for output for diverse info. Storing info in /tmp");
            o_output = "/tmp/info.txt";
        }
        else if(is_dir(o_output)){
            o_output += (o_output.back() == '/' ? "info.txt" : "/info.txt");
            os_log("Saving diverse info in",o_output);
        }
        else if(has_extension(o_output)){
            os_log("Saving diverse info in",o_output);
        }
        else{
            _error("Given output file %s is not a directory nor a file", o_output.c_str());
            return EXIT_FAILURE;
        }

        // empty output file
        FILE *f_out = fopen(o_output.c_str(),"w");
        fclose(f_out);
    }

    if(extract || confidence){
        TICK(Extract)
        if(input.empty()){
            os_error("No input given for extraction operation. Ending script execution.");
            fl_clean_up(s);
            return EXIT_FAILURE;
        }
        
        vector<string> files;
        if(is_dir(input)){
            os_log("Extracting Templates from directory",input);
            if(str_split(filter,",").size() > 1) {
                for(auto f : str_split(filter,",")){
                    vector<string> aux = list_files_from_directory_sorted(input,f);
                    files.insert(files.end(),aux.begin(),aux.end());
                }
            }
            else files = list_files_from_directory_sorted(input,filter);
            check_id(files,dir_id,file_id,delimiter);
            templates = extract_templates(s,retina,files,e_output,log_dir,confidence,skip, time, exec_time_output);
        }
        else if(is_file(input)){
            os_log("Retrieving Files from list",input);
            if(retrieve_files(input.c_str(),files)!=FL_ERR_NONE){
                os_error("Error retrieving files from file ",input);
                return EXIT_FAILURE;
            }
            check_id(files,dir_id,file_id,delimiter);
            os_log("Extracting templates from retrieved files");
            templates = extract_templates(s,retina,files,e_output,log_dir,confidence,skip, time, exec_time_output);
        }
        else{
            _error("Given input %s is not a directory nor a file.",input.c_str());
            return EXIT_FAILURE;
        }

        TOCK_H(Extract)
        os_log("Number of input files for extraction:",files.size());
    }

    if(match){
        TICK(Match)
        vector<pair<string,string>> gen_pairs;
        vector<pair<string,string>> imp_pairs;
        if(!templates.empty() && gen_file.empty() && imp_file.empty()){
            build_matching_lists(templates,gen_pairs,imp_pairs,delimiter,nimps);
        } 
        
        else { //todo receive directory, filter template files with template extension using list_files_from_directory from custom_utils and use build_matching_list to auto generate the matching pairs
            if(gen_file.empty() or imp_file.empty()){
                _error("Matching Requires two files \n\t*Genuines: %s \n\t*Impostors: %s",!gen_file.empty() ? gen_file.c_str() : "None",!imp_file.empty() ? imp_file.c_str() : "None");
                return EXIT_FAILURE;
            }
            if(retrieve_file_pairs(gen_file.c_str(),gen_pairs)!=FL_ERR_NONE){
                os_error("Couldn't retrieve pairs from file ", gen_file);
                return EXIT_FAILURE;
            }
            if(retrieve_file_pairs(imp_file.c_str(),imp_pairs)!=FL_ERR_NONE){
                os_error("Couldn't retrieve pairs from file ", imp_file);
                return EXIT_FAILURE;
            }

            read_templates(s,gen_pairs,templates);            
            read_templates(s,imp_pairs,templates);
            if(templates.empty()){
                os_error("Couldn't read templates from file ", imp_file);
                return EXIT_FAILURE;
            }
            
            // if(debug){
            //     os_debug("first template:");
            //     for(auto data : templates.begin()->second.data)
            //         cout << data << " ";
            //     cout << endl;
            // }
            
        }

        auto init_time = std::chrono::high_resolution_clock::now();

        compare_batch(s,gen_pairs,templates,"G",cosine,gen_scores,gen_match_time);
        compare_batch(s,imp_pairs,templates,"I",cosine,imp_scores,imp_match_time);

        if(save_info){
            FILE *f_out = fopen(o_output.c_str(),"a");

            fprintf(f_out, "MATCH: -------------------------\n");
            fprintf(f_out,"GEN: %ld\n",gen_scores.size());
            fprintf(f_out,"IMP: %ld\n",imp_scores.size());
            fprintf(f_out, "T_MAT[us]: %.2f\n",static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - init_time).count()) / ((gen_pairs.size() + imp_pairs.size())));

            fclose(f_out);
        }

        save_match(gen_scores,has_extension(m_output) ? get_filedir(m_output)+"gen_"+get_filename(m_output) : m_output, gen_file.empty() ? "genuines.txt" : gen_file, gen_match_time);
        save_match(imp_scores,has_extension(m_output) ? get_filedir(m_output)+"imp_"+get_filename(m_output) : m_output, imp_file.empty() ? "impostors.txt" : imp_file, imp_match_time);
        save_rejected(gen_scores,"","",threshold);
        
        TOCK_MIN(Match)
    }

    check_templates_corruption(templates);

    if(accuracy){
        TICK(Accuracy);

        if(gen_scores.empty()){ //If there is no matching performed before
            if(templates.empty()){ //If there is no matching or extraction performed before (or no template generated) 
                if(input.empty()){
                    os_error("No templates and matching result found, exitting application.");
                    return EXIT_FAILURE;
                } else {
                    vector<string> files;
                    vector<string> filters = str_split(filter,",");
                    if(is_dir(input)){
                        os_log("Retrieving Templates from directory",input);
                        if(filters.size() > 1) {
                            for(auto f : filters){
                                vector<string> aux = list_files_from_directory_sorted(input,f);
                                files.insert(files.end(),aux.begin(),aux.end());
                            }
                        }
                        else files = list_files_from_directory_sorted(input,filter);
                        check_id(files,dir_id,file_id,delimiter);
                    }
                    else if(is_file(input)){
                        os_log("Retrieving Files from list",input);
                        if(retrieve_files(input.c_str(),files)!=FL_ERR_NONE){
                            os_error("Error retrieving files from file ",input);
                            return EXIT_FAILURE;
                        }
                        check_id(files,dir_id,file_id,delimiter);
                    }
                    
                    read_templates(s,files,templates);
                    if(templates.empty()){
                        os_error("Couldn't Retrieve templates from input.");
                        return EXIT_FAILURE;
                    }
                }
            }

            os_log("Templates obtained for accuracy test:",templates.size());

            if(!avoid_overflow){
                os_log("Building matching lists");
                vector<pair<string,string>> gen_pairs;
                vector<pair<string,string>> imp_pairs;
                build_matching_lists(templates,gen_pairs,imp_pairs,delimiter);

                _log("Matching Template pairs for accuracy test");
                compare_batch(s,gen_pairs,templates,"G",cosine,gen_scores);
                compare_batch(s,imp_pairs,templates,"I",cosine,imp_scores);
            }
        }
        if(!avoid_overflow)
            accuracy_test(gen_scores,imp_scores,rank,delimiter);
        else
            memory_accuracy(s,templates,cosine,rank, nranks, delimiter);
        
        TOCK_H(Accuracy);
    }

    if(analyse){ //Todo search in input dir compare file lists
        TICK(Analysis)
        if(a_output.empty()){
            if(input.empty()) {_warn("No directory passed as output for analysis. Storing results in /tmp"); a_output="/tmp/analysis.txt";}
            else a_output = get_filedir(input)+"analysis.txt";
        } 
        else if (is_dir(a_output)) a_output+= a_output.back()=='/' ? "analysis.txt" : "/analysis.txt";
        else if (has_extension(a_output) && !get_filedir(a_output).empty())
            if(!is_dir(get_filedir(a_output))) 
                assert(mkdir(get_filedir(a_output)));
        
        if(gen_file.empty() && imp_file.empty()){
            if(gen_scores.empty() || imp_scores.empty()){
                _error("Empty Pairs and scores in either of genuines or impostors list:\n\t*Genuines: %s\n\t*Impostors: %s", gen_scores.empty()?"Empty":"Not Empty",imp_scores.empty()?"Empty":"Not Empty");
                return EXIT_FAILURE;
            }
        }
        stringstream ss;
        ss << gen_file << " " << imp_file << " ";
        // if(cosine){
        //     // normalize_matches(gen_scores,imp_scores);
        //     save_match_norm(gen_scores,has_extension(m_output) ? get_filedir(m_output)+"gen_"+get_filename(m_output) : m_output, gen_file.empty() ? "genuines.txt" : gen_file);
        //     save_match_norm(imp_scores,has_extension(m_output) ? get_filedir(m_output)+"imp_"+get_filename(m_output) : m_output, imp_file.empty() ? "impostors.txt" : imp_file);
            
        // }
        if(gen_scores.empty() && imp_scores.empty())
            eval2(&gen_file[0u], &imp_file[0u], &a_output[0u], info, save_rank, base, 0, 0, &o_output[0u], save_info);
        else
            eval2(gen_scores, imp_scores, &a_output[0u], info, save_rank, base, ss, &o_output[0u], save_info);
        os_log("Analysis result saved to:",a_output);
        TOCK_MIN(Analysis)
    }

    fl_clean_up(s);
    retina.release();
    // delete retina;
    return EXIT_SUCCESS;

}
