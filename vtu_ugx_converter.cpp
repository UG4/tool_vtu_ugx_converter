#include "vtu_ugx_converter.hpp"

void help(){
	std::cout << "usage: ./converter <fin> <fout>" << std::endl;
}

int main(int argc, char **argv){
	if(argc < 3){
		help();
		return -1;
	}

	bool combine = false;

	for(unsigned i = 1; i < argc; ++i){
		if(strcmp(argv[i], "-c") == 0){
			combine = true;
			continue;
		}
		std::string fin = argv[i];
		unsigned pos = fin.find(".vtu");
		std::string fout = fin;
		fout.replace(pos, pos+4, ".ugx");
		std::cout << "converting " << fin << "..." << std::endl; 
		do_it(fin, fout, combine);	
	}
}
