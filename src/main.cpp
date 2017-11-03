#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <vector>
#include "boost/algorithm/string/predicate.hpp"
#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/uuid/sha1.hpp"
#include <iterator>
#include <fstream>
#include <algorithm>
#include <map>
#include <stack>

void log(const char* msg){
	std::cout << msg << std::endl;
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

std::string sha(std::string const& file, size_t bufferSize){
	std::ifstream stream{file, std::ios::binary | std::ios::ate};

	auto i{stream.tellg()};

	boost::uuids::detail::sha1 sha1;

	stream.seekg(0, stream.beg); // Get back at the beggining

	char* buffer = new char[bufferSize];

	while(i > 0){
		size_t toRead{i >= bufferSize ? bufferSize : i};
		memset(buffer, 0, bufferSize);
		stream.read(buffer, toRead);
		sha1.process_bytes(buffer, toRead);
		i -= bufferSize;
	}

	delete[] buffer;

	uint32_t hash[5] = {0};
	sha1.get_digest(hash);

	char finalHash[41] = {0};

	for(int i{0}; i < 5; ++i){
		// Write hash to hex format (1 element in hash takes 4 bytes (8 hex chars))
		std::sprintf(finalHash + (i << 3), "%08x", hash[i]);
	}

	return std::string{finalHash};
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

size_t getSize(std::string const& file){
	std::ifstream f{file, std::ios::binary | std::ios::ate};
	return f.tellg();
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
		directories.emplace(directory, files);
	}

	unsigned totalFiles{0};

	std::vector<boost::filesystem::path> toAvoid;

	for(std::map<std::string, std::vector<boost::filesystem::path>>::const_iterator dirsIt(directories.begin()); dirsIt != directories.end(); ++dirsIt){
		auto const& files = dirsIt->second;
		totalFiles += files.size();

		std::map<size_t, std::vector<boost::filesystem::path>> sizes;

		// Make a list associating sizes and number of files having this same size
		for(std::vector<boost::filesystem::path>::const_iterator i(files.begin());  i != files.end(); ++i){
			size_t size{getSize(i->string())};
			bool added{sizes.emplace(size, std::initializer_list<boost::filesystem::path>{*i}).second};
			if(!added){
				std::vector<boost::filesystem::path>& sameSizeFiles = sizes[size];
				sameSizeFiles.push_back(*i);
			}
		}

		std::vector<boost::filesystem::path> toCheck;
		std::vector<std::string> shas;


		// Add all groups of all same-sized files to a single vector
		for(std::map<size_t, std::vector<boost::filesystem::path>>::iterator it{sizes.begin()}; it != sizes.end(); ++it){

			// If the file has not unique size, so there might be possible duplicates
			if((it->second).size() > 1){
				for(auto const& path : it->second){
					toCheck.push_back(path);
				}
			}
		}

		std::cout << "Found " << files.size() - sizes.size() << " possible duplicate(s)" << std::endl;

		std::cout << "Processing SHA" << std::endl;

		// Only keep one file with same SHA
		for(auto const& pathSHACheck : toCheck){
			std::string sha1{sha(pathSHACheck.string(), 16)};
			if(inArray(shas, sha1)){
				toAvoid.push_back(pathSHACheck.string());
			}else shas.push_back(sha1);
		}

	}

	std::cout << "Found " << toAvoid.size() << " duplicate(s)" << std::endl;
	for(auto const& avoidFile : toAvoid){
		std::cout << avoidFile.string() << std::endl;
	}

	std::cout << "Found " << totalFiles - toAvoid.size() << " to keep" << std::endl;

	for(boost::filesystem::path path : toAvoid){
		try{
			boost::filesystem::remove_all(path);
		}catch(std::exception &e){
			log(e.what());
		}
	}

	return 0;
}
