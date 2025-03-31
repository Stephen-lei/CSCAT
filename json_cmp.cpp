#include "data_encode.h"
#include <iostream>
#include <fstream>
using namespace std;
using namespace SA;
using json = nlohmann::json;
int main(int argc,char **argv) {
    cout<<"argc:"<<argc<<endl;
    if(argc != 3) {
        cout<<"there must be 2 args"<<endl;
        return 1;
    }
    cout<<argv[1]<<" "<<argv[2]<<endl;
    ifstream f1(argv[1]),f2(argv[2]);
    json data1 = json::parse(f1);
    json data2 = json::parse(f2);
    // cout<<setw(4)<<data1<<endl;
    // cout<<setw(4)<<data2<<endl;
    jsonFile file1 = data1;
    jsonFile file2 = data2;
    // cout<<"----------"<<argv[1]<<"------------"<<endl;
    // file1.dumpJsonFile();
    // cout<<"----------"<<argv[2]<<"------------"<<endl;
    // file2.dumpJsonFile();
    cout<<"-----------compare------------"<<endl;
    // cout<<"result:"<<(file1==file2)<<endl;
    jsonFile::DiffJsonFile(file1, file2);
    return 0;
}