#include "reader.h"

int read(std::string filename, std::string fileinfo, std::vector<unsigned char> &data) {
    // get file name without extension
    std::string::size_type idx = filename.rfind('.');
    std::string datafile_name;
    if(idx != std::string::npos){
        datafile_name = filename.substr(0, idx);
    }
    std::cout << datafile_name << std::endl;
    
    idx = fileinfo.rfind('.');
    std::string infofile_name;
    if(idx != std::string::npos){
        infofile_name = fileinfo.substr(0, idx);
    }
    std::cout << infofile_name << std::endl;

    if(datafile_name.empty() || infofile_name.empty()){
        std::cerr << "Error: invalid file name" << std::endl;
        return -1;
    }
    if(datafile_name.compare(infofile_name) != 0){
        std::cerr << "Error: file names do not match" << std::endl;
        return -1;
    }

    std::ifstream
    file(filename
    , std::ios::in | std::ios::binary | std::ios::ate);
    if(file.is_open()){
        std::streampos size = file.tellg();
        data.resize(size);
        file.seekg(0, std::ios::beg);
        file.read((char*)&data[0], size);
        file.close();
    }else{
        std::cerr << "Error: could not open file" << std::endl;
        return -1;
    }
    return 0;
}