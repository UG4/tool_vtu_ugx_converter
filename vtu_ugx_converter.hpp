#ifndef __HPP__EMVIS_vtu_ugx_converter
#define __HPP__EMVIS_vtu_ugx_converter

#include <iostream>
#include <istream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string.h>
#include <assert.h>

inline unsigned myatoi(std::string line, unsigned& v, char end=0){
	unsigned idx = 0;
	v = line[idx]-'0';
	++idx;
	while(line[idx]!=end && idx < line.size()){
		v *= 10;
		v += line[idx]-'0';
		++idx;
	}
	return idx;
}

class PARSE{
public:
	PARSE(const std::string filename)	: _is(new std::ifstream(filename)){}

	std::string get_line(){
		getline(*_is, _line);

		if(_is->eof()){
			return std::string();
		}
		else{
			skip_initial_spaces();
			return _line;
		}
	}

	void skip_initial_spaces(){
		unsigned i = 0;
		while(_line.size() && _line[i] == ' '){
			++i;
		}
		_line = _line.substr(i, _line.size());
	}

	std::string get_value(std::string &line, std::string token){
		unsigned pos = line.find(token);
		pos = line.find("\"", pos);
		unsigned pos2 = line.find("\"", pos+1);
		return line.substr(pos+1, pos2-pos-1);
	}

	void expect_exact(std::string token, bool rem=true){
		if(!_line.size()){
			get_line();
		}
		if(_line != token){
			std::cerr << "expected \"" << token << "\"" << std::endl;
			std::cerr << "got \"" << _line << "\"" << std::endl;
			throw "runtime error";
		}

		if(rem){
			_line.clear();
		}
	}

	void expect_exact(std::string ref, std::string token, bool rem=true){
		if(!_line.size()){
			get_line();
		}
		if(ref != token){
			std::cerr << "expected \"" << token << "\"" << std::endl;
			std::cerr << "got \"" << ref << "\"" << std::endl;
			throw "runtime error";
		}

		if(rem){
			_line.clear();
		}
	}

	void expect_contained(std::string token, bool rem=true){
		if(!_line.size()){
			get_line();
		}
		if(_line.find(token) == std::string::npos){
			std::cerr << "expected \"" << token << "\"" << std::endl;
			std::cerr << "got \"" << _line << "\"" << std::endl;
			throw "runtime error";
		}

		if(rem){
			_line.clear();
		}
	}

	void skip_until(std::string token){
		while(_line.find(token) == std::string::npos){
			get_line();
		}
	}

	std::pair<unsigned, unsigned> parse_header(){
		expect_contained("<VTKFile type=\"UnstructuredGrid\"");
		expect_exact("<UnstructuredGrid>");

		get_line();

		unsigned num_points;
		unsigned num_cells;

		std::string s_num_points = get_value(_line, "NumberOfPoints");
		std::string s_num_cells = get_value(_line, "NumberOfCells");

		myatoi(s_num_points, num_points);
		myatoi(s_num_cells, num_cells);

		_line.clear();

		return std::make_pair(num_points, num_cells);
	}

	void parse_point_data(std::vector<std::vector<double> > &point_data, unsigned num_points){
		get_line();

		if(_line != "<PointData>"){
			return;
		}

		expect_exact("<PointData>");
		expect_contained("<DataArray type", false);

		std::string s_name = get_value(_line, "Name");
		std::string s_dim = get_value(_line, "NumberOfComponents");
		unsigned dim;
		myatoi(s_dim, dim);

		std::cout << "found PointData entry: " << "Name: " << s_name << " NumberOfComponents: " << s_dim << std::endl; 

		token_iterator tIt(*this, " ", false);

		for(unsigned i = 0; i < num_points; ++i){
			point_data[i].resize(dim);
			for(unsigned j = 0; j < dim; ++j){
				++tIt;
				std::cout.precision((*tIt).size());
				point_data[i][j] = std::stod(*tIt);
			}
		}

		get_line();
		expect_exact("</DataArray>");
		expect_exact("</PointData>");

		expect_exact("<CellData>");
		skip_until("</CellData>");
		expect_exact("</CellData>");
	}

	void parse_points(std::vector<std::vector<double> > &points, unsigned num_points){
		expect_exact("<Points>");
		expect_contained("<DataArray type=\"Float32\" Name=\"Points\"", false);

		std::string s_name = get_value(_line, "Name");
		std::string s_dim = get_value(_line, "NumberOfComponents");
		unsigned dim;
		myatoi(s_dim, dim);

		std::cout << "found Points entry: " << "Name: " << s_name << " NumberOfComponents: " << s_dim << std::endl; 


		token_iterator tIt(*this, " ", false);

		for(unsigned i = 0; i < num_points; ++i){
			points[i].resize(dim);
			for(unsigned j = 0; j < dim; ++j){
				++tIt;
				std::cout.precision((*tIt).size());
				points[i][j] = std::stod(*tIt);
			}
		}

		get_line();
		expect_exact("</DataArray>");
		expect_exact("</Points>");
	}

	void parse_connectivity(std::vector<unsigned> &conn){
		expect_exact("<Cells>");
		expect_contained("<DataArray type=\"Int64\" Name=\"connectivity\"", false);

		std::string s_name = get_value(_line, "Name");
		std::cout << "found Cells entry: " << "Name: " << s_name << std::endl; 

		token_iterator tIt(*this, " ", false);
		//TODO how many?
		while(true){
			++tIt;
			if((*tIt).find("<") != std::string::npos){
				break;
			}
			myatoi(*tIt, _uintval);
			conn.push_back(_uintval);
		}

		expect_exact(*tIt, "</DataArray>");
	}

	void parse_offsets(std::vector<unsigned> &offsets, unsigned num_cells){
		expect_contained("<DataArray type=\"Int64\" Name=\"offsets\"", false);

		std::string s_name = get_value(_line, "Name");
		std::cout << "found Cells entry: " << "Name: " << s_name << std::endl; 

		token_iterator tIt(*this, " ", false);
		//TODO how many?
		while(true){
			++tIt;
			if((*tIt).find("<") != std::string::npos){
				break;
			}
			myatoi(*tIt, _uintval);
			offsets.push_back(_uintval);
		}

		expect_exact("</DataArray>");
	}

	void parse_types(std::vector<unsigned> &types){
		expect_contained("<DataArray type=\"UInt8\" Name=\"types\"", false);

		std::string s_name = get_value(_line, "Name");
		std::cout << "found Cells entry: " << "Name: " << s_name << std::endl; 

		token_iterator tIt(*this, " ", false);
		//TODO how many?
		while(true){
			++tIt;
			if((*tIt).find("<") != std::string::npos){
				break;
			}
			myatoi(*tIt, _uintval);
			types.push_back(_uintval);
		}

		expect_exact("</DataArray>");
	}

	void end_file(){
		expect_exact("</Cells>");
		expect_exact("</Piece>");
		expect_exact("</UnstructuredGrid>");
		expect_exact("</VTKFile>");
	}

private:
	std::ifstream* _is;
	std::string _line;
	unsigned _uintval;

public:
	class token_iterator{
	public:
		token_iterator(PARSE &p, std::string delimiter=" ", bool end=false) : _p(p), _d(delimiter){
			_line = "";
			_token = "";
			_pos = 0;
		}

		bool operator==(const token_iterator& o) const{
			return _token == o._token;
		}

		bool operator!=(const token_iterator& o) const{
			return !operator==(o);
		}

		token_iterator& operator++(){
			if(!_line.size()){
				_p.get_line();
				_line = _p._line;
			}
			_pos = _line.find(_d);
			if(_pos != std::string::npos){
				_token = _line.substr(0, _pos);
				_line.erase(0, _pos + _d.length());
			}
			return *this;
		}

		std::string const& operator*() const{
			return _token;
		}

	private:
		PARSE& _p;
		std::string _token;
		std::string _d;
		std::string _line;
		unsigned _pos;
	}; // token_iterator

	token_iterator begin(std::string delimiter=" "){
		return token_iterator(*this, delimiter);
	}

	token_iterator end(std::string delimiter=" "){
		return token_iterator(*this, delimiter, true);
	}
}; // PARSE


class WRITE{
public:
	WRITE(const std::string filename)	: _fout(filename){}

	void write_header(std::string gridname = "defGrid"){
		_fout << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
		_fout << "<grid name=\"" << gridname << "\">" << std::endl;
	}

	void write_points(std::vector<std::vector<double> > &points, unsigned dim=3){
		_fout << "	<vertices coords=\"" << dim << "\">";
		for(unsigned i = 0; i < points.size(); ++i){
			for(unsigned j = 0; j < dim; ++j){
				_fout << points[i][j] << " ";
			}
		}
		_fout << "</vertices>" << std::endl;
	}

	//rtn = vector <edges.size(), triangles.size(), quadrilaterals.size()>
	std::vector<unsigned> assemble_elements(std::vector<unsigned> &conn, std::vector<unsigned> &offsets, std::vector<unsigned> &types){
		assert(offsets.size() == types.size());

		unsigned j = 0;

		std::pair<unsigned, unsigned> edge;
		std::vector<unsigned> triangle(3);
		std::vector<unsigned> quadrilateral(4);
		std::vector<unsigned> prism(6);
		std::vector<unsigned> tet(4);
		std::vector<unsigned> pyramid(5);

		unsigned cnt = 0;

		for(unsigned i = 0; i < types.size(); ++i){
			switch(types[i]){
				case 10: //VTK_TETRA
					edge.first = conn[j]; edge.second = conn[j+1]; _edges.push_back(edge);
					edge.first = conn[j]; edge.second = conn[j+2]; _edges.push_back(edge);
					edge.first = conn[j+1]; edge.second = conn[j+2]; _edges.push_back(edge);
					edge.first = conn[j]; edge.second = conn[j+3]; _edges.push_back(edge);
					edge.first = conn[j+1]; edge.second = conn[j+3]; _edges.push_back(edge);
					edge.first = conn[j+2]; edge.second = conn[j+3]; _edges.push_back(edge);

					triangle[0] = conn[j]; triangle[1] = conn[j+1]; triangle[2] = conn[j+3]; _triangles.push_back(triangle);
					triangle[0] = conn[j]; triangle[1] = conn[j+2]; triangle[2] = conn[j+3]; _triangles.push_back(triangle);
					triangle[0] = conn[j+1]; triangle[1] = conn[j+2]; triangle[2] = conn[j+3]; _triangles.push_back(triangle);

					for(unsigned k = 0; k < 4; ++k){
						tet[k] = conn[j+k];
					}
					_tets.push_back(tet);

					cnt++;

					j+= 4;

					break;

				case 13: //VTK_WEDGE
					edge.first = conn[j]; edge.second = conn[j+1]; _edges.push_back(edge);
					edge.first = conn[j]; edge.second = conn[j+2]; _edges.push_back(edge);
					edge.first = conn[j+1]; edge.second = conn[j+2]; _edges.push_back(edge);
					edge.first = conn[j]; edge.second = conn[j+3]; _edges.push_back(edge);
					edge.first = conn[j+1]; edge.second = conn[j+4]; _edges.push_back(edge);
					edge.first = conn[j+2]; edge.second = conn[j+5]; _edges.push_back(edge);
					edge.first = conn[j+3]; edge.second = conn[j+4]; _edges.push_back(edge);
					edge.first = conn[j+3]; edge.second = conn[j+5]; _edges.push_back(edge);
					edge.first = conn[j+4]; edge.second = conn[j+5]; _edges.push_back(edge);

					triangle[0] = conn[j]; triangle[1] = conn[j+1]; triangle[2] = conn[j+2]; _triangles.push_back(triangle);
					triangle[0] = conn[j+3]; triangle[1] = conn[j+4]; triangle[2] = conn[j+5]; _triangles.push_back(triangle);

					quadrilateral[0] = conn[j]; quadrilateral[1] = conn[j+1]; quadrilateral[2] = conn[j+4]; quadrilateral[3] = conn[j+3]; _quadrilaterals.push_back(quadrilateral);
					quadrilateral[0] = conn[j]; quadrilateral[1] = conn[j+2]; quadrilateral[2] = conn[j+5]; quadrilateral[3] = conn[j+3]; _quadrilaterals.push_back(quadrilateral);
					quadrilateral[0] = conn[j+1]; quadrilateral[1] = conn[j+2]; quadrilateral[2] = conn[j+5]; quadrilateral[3] = conn[j+4]; _quadrilaterals.push_back(quadrilateral);

					for(unsigned k = 0; k < 6; ++k){
						prism[k] = conn[j+k];
					}
					_prisms.push_back(prism);

					j+=6;

					cnt++;

					break;

				case 14: //VTK_PYRAMID
					edge.first = conn[j]; edge.second = conn[j+1]; _edges.push_back(edge);
					edge.first = conn[j+1]; edge.second = conn[j+2]; _edges.push_back(edge);
					edge.first = conn[j+2]; edge.second = conn[j+3]; _edges.push_back(edge);
					edge.first = conn[j]; edge.second = conn[j+3]; _edges.push_back(edge);
					edge.first = conn[j]; edge.second = conn[j+4]; _edges.push_back(edge);
					edge.first = conn[j+1]; edge.second = conn[j+4]; _edges.push_back(edge);
					edge.first = conn[j+2]; edge.second = conn[j+4]; _edges.push_back(edge);
					edge.first = conn[j+3]; edge.second = conn[j+4]; _edges.push_back(edge);

					triangle[0] = conn[j]; triangle[1] = conn[j+1]; triangle[2] = conn[j+4]; _triangles.push_back(triangle);
					triangle[0] = conn[j+1]; triangle[1] = conn[j+2]; triangle[2] = conn[j+4]; _triangles.push_back(triangle);
					triangle[0] = conn[j+2]; triangle[1] = conn[j+3]; triangle[2] = conn[j+4]; _triangles.push_back(triangle);
					triangle[0] = conn[j]; triangle[1] = conn[j+3]; triangle[2] = conn[j+4]; _triangles.push_back(triangle);

					quadrilateral[0] = conn[j]; quadrilateral[1] = conn[j+1]; quadrilateral[2] = conn[j+2]; quadrilateral[3] = conn[j+3]; _quadrilaterals.push_back(quadrilateral);

					for(unsigned k = 0; k < 5; ++k){
						pyramid[k] = conn[j+k];
					}
					_pyramids.push_back(pyramid);

					j+=5;

					cnt++;

					break;

				default:
					break;
			}
		}

		//rem doubles
		std::sort(_edges.begin(), _edges.end());
		_edges.erase(std::unique(_edges.begin(), _edges.end()), _edges.end());

		std::sort(_triangles.begin(), _triangles.end());
		_triangles.erase(std::unique(_triangles.begin(), _triangles.end()), _triangles.end());

		std::sort(_quadrilaterals.begin(), _quadrilaterals.end());
		_quadrilaterals.erase(std::unique(_quadrilaterals.begin(), _quadrilaterals.end()), _quadrilaterals.end());


		std::vector<unsigned> sizes(6);
		sizes[0] = _edges.size();
		sizes[1] = _triangles.size();
		sizes[2] = _quadrilaterals.size();
		sizes[3] = _tets.size();
		sizes[4] = _prisms.size();
		sizes[5] = _pyramids.size();

		return sizes;

	}

	void write_elements(){
		_fout << "	<edges>";
		for(unsigned i = 0; i < _edges.size(); ++i){
			_fout << _edges[i].first << " " << _edges[i].second << " ";
		}
		_fout << "	</edges>" << std::endl;

		_fout << "	<triangles>";
		for(unsigned i = 0; i < _triangles.size(); ++i){
			for(unsigned j = 0; j < 3; ++j){
				_fout << _triangles[i][j] << " ";
			}
		}
		_fout << "	</triangles>" << std::endl;

		_fout << "	<quadrilaterals>";
		for(unsigned i = 0; i < _quadrilaterals.size(); ++i){
			for(unsigned j = 0; j < 4; ++j){
				_fout << _quadrilaterals[i][j] << " ";
			}
		}
		_fout << "	</quadrilaterals>" << std::endl;


		_fout << "	<tetrahedrons>";
		for(unsigned i = 0; i < _tets.size(); ++i){
			for(unsigned j = 0; j < 4; ++j){
				_fout << _tets[i][j] << " ";
			}
		}
		_fout << "	</tetrahedrons>" << std::endl;

		_fout << "	<prisms>";
		for(unsigned i = 0; i < _prisms.size(); ++i){
			for(unsigned j = 0; j < 6; ++j){
				_fout << _prisms[i][j] << " ";
			}
		}
		_fout << "	</prisms>" << std::endl;

		_fout << "	<pyramids>";
		for(unsigned i = 0; i < _pyramids.size(); ++i){
			for(unsigned j = 0; j < 5; ++j){
				_fout << _pyramids[i][j] << " ";
			}
		}
		_fout << "	</pyramids>" << std::endl;
	}

	void write_subset_handler(unsigned num_points, std::vector<unsigned> &sizes){
		_fout << "	<subset_handler name=\"defSH\">" << std::endl;
		_fout << "		<subset name=\"Inner\" color=\"0 0 0\" state=\"393216\">" << std::endl;

		_fout << "			<vertices>";
		for(unsigned i = 0; i < num_points; ++i){
			_fout << i << " ";
		}
		_fout << "</vertices>" << std::endl;

		_fout << "			<edges>";
		for(unsigned i = 0; i < sizes[0]; ++i){
			_fout << i << " ";
		}
		_fout << "</edges>" << std::endl;

		_fout << "			<faces>";
		for(unsigned i = 0; i < sizes[1]+sizes[2]; ++i){
			_fout << i << " ";
		}
		_fout << "</faces>" << std::endl;

		_fout << "			<volumes>";
		for(unsigned i = 0; i < sizes[3]+sizes[4]+sizes[5]; ++i){
			_fout << i << " ";
		}
		_fout << "</volumes>" << std::endl;


		_fout << "		</subset>" << std::endl;
		_fout << "	</subset_handler>" << std::endl;
	}

	void write_eof(){
		_fout << "</grid>" << std::endl;
	}

public: //TODO
	std::ofstream _fout;
	std::vector<std::pair<unsigned, unsigned> > _edges;
	std::vector<std::vector<unsigned> > _triangles;
	std::vector<std::vector<unsigned> > _quadrilaterals;
	std::vector<std::vector<unsigned> > _tets;
	std::vector<std::vector<unsigned> > _prisms;
	std::vector<std::vector<unsigned> > _pyramids;
};


#endif //guard
