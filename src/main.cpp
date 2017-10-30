#include <iostream>
#include <string>
#include <vector>
#include "boost/algorithm/string/predicate.hpp"
#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"
#include <iterator>
#include <fstream>
#include <algorithm>
#include <map>
#include <stack>

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

std::vector<boost::filesystem::path> listFiles(std::string const& dir, bool addDirs = false){
	boost::filesystem::path p{dir};
	boost::filesystem::directory_iterator end_dir_it;

	std::vector<boost::filesystem::path> paths;

	for(boost::filesystem::directory_iterator dir_it{p}; dir_it != end_dir_it; ++dir_it){ // Iterate over files in directory
		if(addDirs || (!addDirs && boost::filesystem::is_regular_file(dir_it->path().string()))){
		   paths.push_back(dir_it->path());
	   }
	}

	return paths;
}

typedef struct args_s{
	std::map<std::string, std::string> named_args;
	std::vector<std::string> rest;
	int named_arg_exists(std::string const& key_search) const{
		return named_args.find(key_search) != named_args.end();
	}
	static args_s make(int argc, char* argv[]){
		args_s final_args;
		for(int i{1}; i < argc; ++i){
			if(boost::starts_with(argv[i], "--")){
				std::string value;
				if((i < argc - 1 && boost::starts_with(argv[i + 1], "--")) || i == argc - 1){
					value = "1";
				}else{
					value = argv[i + 1];
				}
				final_args.named_args.emplace(std::string{argv[i]}.substr(2), value);
				++i;
			}else{
				final_args.rest.push_back(argv[i]);
			}
		}
		return final_args;
	}
} args_s;

std::map<std::string, std::vector<boost::filesystem::path>> listFilesRecursive(std::string const& dir){

	std::map<std::string, std::vector<boost::filesystem::path>> paths;

	std::stack<std::pair<std::string, boost::filesystem::path>> stack;

	std::vector<boost::filesystem::path> baseFiles{listFiles(dir, true)};

	for(auto const& file : baseFiles){ // Iterate over files in directory
	   stack.emplace(dir, file);
	}

	while(!stack.empty()){
		std::pair<std::string, boost::filesystem::path> const path{stack.top()};
		stack.pop();
		if(boost::filesystem::is_directory(path.second.string())){
			std::vector<boost::filesystem::path> subFiles{listFiles(path.second.string(), true)};
			for(const auto file : subFiles){ // Iterate over files in directory
			   stack.emplace(path.second.string(), file);
			}
		}else{
			bool inserted{paths.emplace(path.first, std::initializer_list<boost::filesystem::path>{path.second}).second};
			if(!inserted){
				std::vector<boost::filesystem::path> &elem = paths[path.first];
				elem.push_back(path.second);
			}
		}
	}

	return paths;
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

	const args_s args{args_s::make(argc, argv)};

	if(args.rest.size() < 1){
		log("You must provide a directory");
		return -1;
	}

	const std::string directory{args.rest[0]};

	if(!boost::filesystem::is_directory(directory)){
		log("You did not provide a valid directory");
		return -1;
	}

	bool directoriesRecursive{args.named_arg_exists("recursive") ? boost::lexical_cast<bool>(args.named_args.at("recursive")) : false};

	std::map<std::string, std::vector<boost::filesystem::path>> directories;

	if(directoriesRecursive){
		directories = listFilesRecursive("\\\\?\\" + std::string{directory}); // \\?\ is for long-filenames support on windows
	}else{
		const auto files = listFiles(directory);
		directories.emplace(directory, listFiles(directory));
	}

	unsigned processed{0};

	for(std::map<std::string, std::vector<boost::filesystem::path>>::const_iterator dirsIt(directories.begin()); dirsIt != directories.end(); ++dirsIt){

		auto const& files = dirsIt->second;

		std::vector<boost::filesystem::path> toAvoid;
		std::vector<std::string> toKeep;

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
	}

	return 0;
}
