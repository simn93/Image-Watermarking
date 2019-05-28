/**
* Name : Simone
* Surname : Schirinzi
* Matriculation number : 490209
* Software : Image Watermark
*
* I declare that the content of this file is in its entirety the original work of the author.
*
* Simone Schirinzi
*/

#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <thread>
#include "stuff.cpp"
#include "BlockingQueue.cpp"

using namespace std;
using namespace ff;

int par_degree = 4;

void sequentialExecution(vector<string> files,
                         string inDir,
                         string outDir,
                         vector<pair<const unsigned int, const unsigned int>> mark){
    for(int i=0;i<files.size();i++)
        close(filter(open(string(inDir + "/" + files[i])),mark),string(outDir + "/" + files[i]));
}


void concurrentExecution(vector<string> files,
                         string inDir,
                         string outDir,
                         vector<pair<const unsigned int, const unsigned int>> mark,
                         int nw){
    auto queue = new BlockingQueue<string>();
    vector<thread> threads;

    for(int i = 0; i < nw; i++)threads.push_back(thread([&]{while(true){
        string val = queue->pop();
        if(val == "") return;

        close(filter(open(string(inDir + "/" + val)),mark),string(outDir + "/" + val));
    }}));

    queue->push(files);
    for(int i = 0; i <nw; i++)queue->push("");
    for(int i = nw-1; i >=0; i--)threads[i].join();
}

void concurrentPipelineExecution(vector<string> files,
                                 string inDir,
                                 string outDir,
                                 vector<pair<const unsigned int, const unsigned int>> mark,
                                 int nw){
    if(nw < 3) return;

    auto toRead = new BlockingQueue<Image> ();
    auto toMark = new BlockingQueue<Image> ();
    auto toWrite = new BlockingQueue<Image> ();

    auto max = [](int a, int b) -> int{return (a>=b)?a:b;};

    string inPath, outPath;

    int filter_numb = max(1,(int) trunc(nw/(2*par_degree)));
    int loader = (nw-filter_numb)/2;
    int writer = loader;
    if(loader + writer + filter_numb < nw) loader++;

    assert(filter_numb > 0);
    assert(loader > 0);
    assert(writer > 0);
    assert(filter_numb + loader + writer == nw);
    assert(filter_numb <= loader);

    vector<thread> threads;

    for(int t=0;t<loader;t++)threads.push_back(thread([&]{while(true){
            Image i = toRead->pop();
            if(i.inPath == ""){
                toMark->push(Image("","",nullptr));
                return;
            }
            // read op
            i.img = open(i.inPath);
            toMark->push(i);
        }}));

    for(int t=0;t<filter_numb;t++)threads.push_back(thread([&]{while(true) {
            Image i = toMark->pop();
            if(i.inPath == ""){
                for(int j=0;j<(loader/filter_numb + 1);j++)
                    toWrite->push(Image("","",nullptr));
                return;
            }
            // filter op
            i.img = filter(i.img, *i.mark);
            toWrite->push(i);
        }}));

    for(int t=0;t<writer;t++)threads.push_back(thread([&]{while(true){
            Image i = toWrite->pop();
            if(i.inPath == "") return;
            // write op
            close(i.img,i.outPath);
        }}));

    for(int i=0; i<files.size();i++){
        inPath = inDir + "/" + files[i];
        outPath = outDir + "/" + files[i];
        toRead->push(Image(inPath, outPath, &mark));
    }

    for(int i=0; i<loader;i++)toRead->push(Image("","", nullptr));
    for(int i = nw-1; i >=0; i--)threads[i].join();
}

void fastFlowExecution(vector<string> files,
                       string inDir,
                       string outDir,
                       vector<pair<const unsigned int, const unsigned int>> mark,
                       int nw){
    class worker: public ff_node {
    public:
        void* svc(void* task){
            auto img = (Image*)task;
            close(filter(open(img->inPath),*(img->mark)),img->outPath);
            return (void*)img;
        }
    };
    class emitter: public ff_node {
    public:
        emitter(vector<string> files,
                string inDir,
                string outDir,
                vector<pair<const unsigned int, const unsigned int>> mark)
                :files(files),inDir(inDir),outDir(outDir),mark(mark) {};

        void * svc(void * task) {
            for(int i=0;i<files.size();i++)
                ff_send_out(new Image(string(inDir+"/"+files[i]),string(outDir+"/"+files[i]),&mark));
            ff_send_out(EOS);
            return NULL;
        }

    private:
        vector<string> files;
        string inDir;
        string outDir;
        vector<pair<const unsigned int, const unsigned int>> mark;
    };

    ff_farm<> farm;
    emitter e(files,inDir,outDir,mark);
    farm.add_emitter(&e);
    std::vector<ff_node *> w;
    for(int i=0;i<nw;++i) w.push_back(new worker);
    farm.add_workers(w);
    farm.remove_collector();
    farm.run_and_wait_end();

    #if defined(TRACE_FASTFLOW)
    farm.ffStats(cout);
    #endif
    return;
}

void fastFlowPipelineExecution(vector<string> files,
                               string inDir,
                               string outDir,
                               vector<pair<const unsigned int, const unsigned int>> mark,
                               int nw){
    if(nw < 3)return;

    class emitter: public ff_node {
    public:
        emitter(vector<string> files,
                string inDir,
                string outDir,
                vector<pair<const unsigned int, const unsigned int>> mark)
                :files(files),inDir(inDir),outDir(outDir),mark(mark) {};

        void * svc(void * task) {
            for(int i=0;i<files.size();i++) ff_send_out(new Image(string(inDir+"/"+files[i]),string(outDir+"/"+files[i]),&mark));
            ff_send_out(EOS);
            return NULL;
        }

    private:
        vector<string> files;
        string inDir;
        string outDir;
        vector<pair<const unsigned int, const unsigned int>> mark;
    };
    class opener: public ff_node {
    public:
        void* svc(void* task){
            auto img = (Image*)task;
            img->img = open(img->inPath);
            return (void*)img;
        }
    };
    class worker: public ff_node {
    public:
        void* svc(void* task){
            auto img = (Image*)task;
            img->img=filter(img->img,*(img->mark));
            return (void*)img;
        }
    };
    class closer: public ff_node {
    public:
        void* svc(void* task){
            auto img = (Image*)task;
            close(img->img,img->outPath);
            return (void*)img;
        }
    };

    int filterSize = max(1,(int) trunc(nw/(2*par_degree)));
    int openSize = (nw - filterSize)/2;
    int closeSize = openSize;
    if(filterSize + openSize + closeSize < nw) openSize++;
    assert(filterSize + openSize + closeSize == nw);

    ff_farm<> farmOpen, farmFilter, farmClose;
    vector<ff_node*> o;

    emitter e(files,inDir,outDir,mark);
    farmOpen.add_emitter(&e);
    for(int q=0;q<openSize;q++)o.push_back(new opener);
    farmOpen.add_workers(o);
    farmOpen.remove_collector();

    o.clear();
    for(int q=0;q<filterSize;q++)o.push_back(new worker);
    farmFilter.add_workers(o);
    farmFilter.setMultiInput();
    farmFilter.remove_collector();
    farmFilter.set_scheduling_ondemand(10);

    o.clear();
    for(int q=0;q<closeSize;q++)o.push_back(new closer);
    farmClose.add_workers(o);
    farmClose.remove_collector();
    farmClose.setMultiInput();
    farmClose.set_scheduling_ondemand(10);

    ff_pipeline pipe;
    pipe.add_stage(&farmOpen);
    pipe.add_stage(&farmFilter);
    pipe.add_stage(&farmClose);

    pipe.run_and_wait_end();

    #if defined(TRACE_FASTFLOW)
    pipe.ffStats(cout);
    #endif
    return;
}

int main(int argc, char **argv) {
    ff::ffTime(ff::START_TIME);

    string inDir = "", wat = "", outDir = "./markedImages";
    int nw = 0;
    bool multiFarm = false, fullTest = false;

    for(int i = 1; i < argc; i+=2){
        switch (argv[i][1]){
            case 'd':
                inDir = argv[i+1];
                break;
            case 'm':
                wat = argv[i+1];
                break;
            case 'w':
                nw = (unsigned) atoi(argv[i+1]);
                break;
            case 'f':
                multiFarm = true;
                i--;
                break;
            case 't' :
                fullTest = true;
                i--;
                break;
            case 'p' :
                par_degree = (unsigned) atoi(argv[i+1]);
                break;
            case 'o' :
                outDir = argv[i+1];
                break;
            default:
                cout << "Usage: " << argv[0] << " -d <jpg directory> -m <jpg watermark> -w <parallelism degree>" << endl;
                break;
        }
    }

    assert(nw != 0);
    assert(inDir != "");
    assert(wat != "");

    // vector of point to mark
    auto mark = parseWatermark(wat);

    // vector of files to mark
    vector<string> files;
    dirToJpgList(inDir, files);

    assert(mark.size() > 0);
    assert(files.size() > 0);

    // check output dir
    DIR *dp;
    if((dp = opendir(outDir.c_str())) == NULL){system(string("mkdir "+outDir).data());} else {closedir(dp);}


    ff::ffTime(ff::STOP_TIME);
    cout << "Initial load require: " << ff::ffTime(ff::GET_TIME) << " (ms)" << endl;
    cout << "Images to process: " << files.size() << endl;

    // full test: all function call
    if(fullTest){
        ff::ffTime(ff::START_TIME);
        sequentialExecution(files, inDir, outDir, mark);
        ff::ffTime(ff::STOP_TIME);
        cout << "Sequential Execution: " << ff::ffTime(ff::GET_TIME) << " (ms)" << endl;

        int size = nw;
        for(nw = 1; nw <= size; nw*=2) {
            cout << "######     " << nw << " nw" << "     ######" << endl;

            ff::ffTime(ff::START_TIME);
            concurrentExecution(files, inDir, outDir, mark, nw);
            ff::ffTime(ff::STOP_TIME);
            cout << "Farm Execution (ms):" << ff::ffTime(ff::GET_TIME) << endl;

            ff::ffTime(ff::START_TIME);
            concurrentPipelineExecution(files, inDir, outDir, mark, nw);
            ff::ffTime(ff::STOP_TIME);
            cout << "Pipeline farm Execution (ms):" << ff::ffTime(ff::GET_TIME) << endl;

            ff::ffTime(ff::START_TIME);
            fastFlowExecution(files, inDir, outDir, mark, nw);
            ff::ffTime(ff::STOP_TIME);
            cout << "FastFlow farm Execution (ms):" <<  ff::ffTime(ff::GET_TIME) << endl;

            ff::ffTime(ff::START_TIME);
            fastFlowPipelineExecution(files, inDir, outDir, mark, nw);
            ff::ffTime(ff::STOP_TIME);
            cout << "FastFlow pipeline farm Execution (ms):" << ff::ffTime(ff::GET_TIME) << endl;

            sleep(2);
        }
        return 0;
    }

    // single test function call
    cout << "######     " << nw << " nw" << "     ######" << endl;
    ff::ffTime(ff::START_TIME);

    if (nw == 1) {
        sequentialExecution(files, inDir, outDir, mark);
        cout << "Sequential Execution: ";
        return 0;
    }

    if(multiFarm){
        if (nw > 1){
            concurrentPipelineExecution(files, inDir, outDir, mark, nw);
            cout << "Pipeline farm Execution ";
        }
        if (nw < 0){
            fastFlowPipelineExecution  (files, inDir, outDir, mark, -nw);
            cout << "FastFlow pipeline farm Execution ";
        }
    } else {
        if (nw > 1){
            concurrentExecution        (files, inDir, outDir, mark, nw);
            cout << "Farm Execution ";
        }
        if (nw < 0){
            fastFlowExecution          (files, inDir, outDir, mark, -nw);
            cout << "FastFlow farm Execution ";
        }
    }

    ff::ffTime(ff::STOP_TIME);
    cout << ff::ffTime(ff::GET_TIME) << " (ms)" << endl;
    return 0;
}