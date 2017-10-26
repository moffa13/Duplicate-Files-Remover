#include <iostream>
#include <string>
#include <vector>
#include "boost/filesystem.hpp"
#include <iterator>
#include <fstream>
#include <algorithm>

void log(const char* msg){
	std::cout << msg << std::endl;
}

template<typename INPUT_IT1, typename INPUT_IT2>
bool equalFiles(INPUT_IT1 first1, INPUT_IT1 last1,
                INPUT_IT2 first2, INPUT_IT2 last2)
{
    while(first1 != last1 && first2 != last2) { // None of them reached the end
        if(*first1 != *first2) return false;
        ++first1;
        ++first2;
    }
    return (first1 == last1) && (first2 == last2); // If one reached EOF, be sure the other too

}

std::vector<boost::filesystem::path> listFiles(std::string const& dir){
	boost::filesystem::path p{dir};
	boost::filesystem::directory_iterator end_dir_it;

	std::vector<boost::filesystem::path> pictures_paths;
	for(boost::filesystem::directory_iterator dir_it{p}; dir_it != end_dir_it; ++dir_it){ // Iterate over files in directory
	   if(boost::filesystem::is_regular_file(dir_it->path().string())){
		   pictures_paths.push_back(dir_it->path());
	   }
	}

	return pictures_paths;
}

bool compareFiles(std::string const& file1, std::string const& file2){
    std::ifstream f1{file1, std::ios::binary | std::ios::ate};
    std::ifstream f2{file2, std::ios::binary | std::ios::ate};

    if(f1.tellg() != f2.tellg()){
        return false;
    }

    f1.seekg(0);
    f2.seekg(0);

    std::istreambuf_iterator<char> it1{f1};
    std::istreambuf_iterator<char> it2{f2};
    std::istreambuf_iterator<char> end;

    return equalFiles(it1, end, it2, end);

}

template<typename T>
bool inArray(std::vector<T> const& vec, T const& elem){
    return std::find(vec.begin(), vec.end(), elem) != vec.end();
}

int main(int argc, char* argv[]){


	if(argc < 2){
		log("You must provide a directory");
		return -1;
	}

    if(!boost::filesystem::is_directory(argv[1])){
        log("You did not provide a valid directory");
        return -1;
    }

    const auto files = listFiles(argv[1]);

    std::vector<boost::filesystem::path> toAvoid;
    std::vector<std::string> toKeep;
    unsigned processed{0};

    for(std::vector<boost::filesystem::path>::const_iterator i(files.begin());  i != files.end(); ++i){
        if(!inArray(toAvoid, *i)){
            toKeep.push_back(i->string());
            for(std::vector<boost::filesystem::path>::const_iterator i2(files.begin());  i2 != files.end(); ++i2){
                if(i->string() != i2->string() && !inArray(toAvoid, *i2) && compareFiles(i->string(), i2->string())){
                    toAvoid.push_back(*i2);
                }
            }
        }
        std::cout << "\rProcessed " << ++processed << " file(s)" << "\r";
        std::cout << std::flush;
    }

    for(boost::filesystem::path path : toAvoid){
        try{
            boost::filesystem::remove_all(path);
        }catch(std::exception &e){
            log(e.what());
        }
    }

    std::cout << "Found " << toAvoid.size() << " duplicate(s)" << std::endl;
    std::cout << "Found " << toKeep.size() << " to keep" << std::endl;

	return 0;

}
