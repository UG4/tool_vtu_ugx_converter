#include "vtu_ugx_converter.hpp"


void do_it(std::string fin, std::string fout, bool combine=false){
	PARSE P(fin);
	std::pair<unsigned, unsigned> num_data = P.parse_header();

	std::vector<std::vector<double> > point_data(num_data.first);
	P.parse_point_data(point_data, num_data.first);

	std::vector<std::vector<double> > points(num_data.first);
	P.parse_points(points, num_data.first);

	std::vector<unsigned> conn; //(num_data.second);
	P.parse_connectivity(conn);

	std::vector<unsigned> offsets;
	P.parse_offsets(offsets, num_data.second);

	std::vector<unsigned> types;
	P.parse_types(types);

	P.end_file();

	WRITE W(fout);
	W.write_header();
	W.write_points(points);
	std::vector<unsigned> sizes = W.assemble_elements(conn, offsets, types);
	W.write_elements();
	W.write_subset_handler(num_data.first, sizes);
	W.write_eof();

	if(combine){
		std::cout << "combine point data with points position" << std::endl;

		for(unsigned i = 0; i < points.size(); ++i){
			for(unsigned j = 0; j < points[i].size(); ++j){
				points[i][j] += point_data[i][j];
			}
		}

		fout = fout+"c";

		WRITE Wc(fout);
		Wc.write_header();
		Wc.write_points(points);
		sizes = Wc.assemble_elements(conn, offsets, types);
		Wc.write_elements();
		Wc.write_subset_handler(num_data.first, sizes);
		Wc.write_eof();
	}
}


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
