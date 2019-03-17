#ifndef __CPP__EMVIS_ug_bridge_vtu
#define __CPP__EMVIS_ug_bridge_vtu

#include <cstring>
#include <string>
//#include "../scene/lg_object.h"
#include "vtu_ugx_converter.hpp"
#include "lib_grid/file_io/file_io.h"
//#include "lib_grid/file_io/file_io_art.h"
//include "lib_grid/file_io/file_io_dump.h"
//#include "lib_grid/file_io/file_io_ugx.h"

#include "common/util/index_list_util.h"
#include "lib_grid/algorithms/selection_util.h"

using namespace std;
using namespace ug;


struct MySubsetHandlerEntry
{
	MySubsetHandlerEntry(SubsetHandler* s) : sh(s) {}

	ISubsetHandler*			sh;
};

struct MySelectorEntry
{
	MySelectorEntry(Selector* s) : sel(s) {}

	ISelector*				sel;
};

struct MyGridEntry
{
	MyGridEntry() : grid(NULL)	{}

	Grid* 		grid;
	std::vector<MySubsetHandlerEntry>	subsetHandlerEntries;
	std::vector<MySelectorEntry>		selectorEntries;

	std::vector<Vertex*> 		vertices;
	std::vector<Edge*> 			edges;
	std::vector<Face*>				faces;
	std::vector<Volume*>			volumes;
};

template <class TGeomObj>
void subset_handler_elements(ISubsetHandler& shOut,
							 const char* elemNodeName,
							 int subsetIndex,
							 std::vector<TGeomObj*>& vElems){
	for(unsigned i = 0; i < vElems.size(); ++i){
		shOut.assign_subset(vElems[i], subsetIndex);
	}
}

bool LoadVTUObjectFromFile(LGObject* pObjOut, const char* filename)
{
	PROFILE_FUNC();

	UG_LOG("LoadVTUObjectFromFile.\n");

	//PARSE

	PARSE P(filename);
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


	//FILL datastructures

	Grid& grid = pObjOut->grid();
	SubsetHandler& sh = pObjOut->subset_handler();
	Selector& sel = pObjOut->selector(); 

	pObjOut->m_fileName = filename;

	MyGridEntry grid_entry;

	uint gridopts = grid.get_options();
	grid.set_options(GRIDOPT_NONE);

	if(!grid.has_vertex_attachment(aPosition)){
		grid.attach_to_vertices(aPosition);
	}

	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	
	grid_entry.grid = &grid;

	vector<Vertex*>& vertices = grid_entry.vertices;
	vector<Edge*>& edges = grid_entry.edges;
	vector<Face*>& faces = grid_entry.faces;
	vector<Volume*>& volumes = grid_entry.volumes;

	WRITE W("");
	W.assemble_elements(conn, offsets, types);

/*
	create_vertices(vertices, grid, curNode, aaPos);
	create_edges(edges, grid, curNode, vertices);
	create_triangles(faces, grid, curNode, vertices);
	create_quadrilaterals(faces, grid, curNode, vertices);
	create_tetrahedrons(volumes, grid, curNode, vertices);
	//create_hexahedrons(volumes, grid, curNode, vertices); //NOT YET
	create_prisms(volumes, grid, curNode, vertices);
	create_pyramids(volumes, grid, curNode, vertices);
	//create_octahedrons(volumes, grid, curNode, vertices); //NOT YET
*/

	//create vertices

	for(unsigned i = 0; i < points.size(); ++i){
		RegularVertex* vrt = *grid.create<RegularVertex>();
		MathVector<3, double> p;
		for(unsigned j = 0; j < points[i].size(); ++j){ //TODO: no MathVector(std::vector) constructor?!
			p[j] = points[i][j];
		}
		aaPos[vrt] = p;
		vertices.push_back(vrt);
	}

	//create edges

	for(unsigned i = 0; i < W._edges.size(); ++i){
		edges.push_back(*grid.create<RegularEdge>(EdgeDescriptor(vertices[W._edges[i].first], vertices[W._edges[i].second])));
	}

/*
	std::vector<std::vector<unsigned> > _triangles;
	std::vector<std::vector<unsigned> > _quadrilaterals;
	std::vector<std::vector<unsigned> > _tets;
	std::vector<std::vector<unsigned> > _prisms;
	std::vector<std::vector<unsigned> > _pyramids;
*/


	//create triangles

	for(unsigned i = 0; i < W._triangles.size(); ++i){
		faces.push_back(*grid.create<Triangle>(TriangleDescriptor(vertices[W._triangles[i][0]], vertices[W._triangles[i][1]], vertices[W._triangles[i][2]])));
	}

	//create quads

	for(unsigned i = 0; i < W._quadrilaterals.size(); ++i){
		faces.push_back(*grid.create<Quadrilateral>(QuadrilateralDescriptor(vertices[W._quadrilaterals[i][0]], vertices[W._quadrilaterals[i][1]],
																			vertices[W._quadrilaterals[i][2]], vertices[W._quadrilaterals[i][3]])));
	}

	//create tets

	for(unsigned i = 0; i < W._tets.size(); ++i){
		volumes.push_back(*grid.create<Tetrahedron>(TetrahedronDescriptor(vertices[W._tets[i][0]], vertices[W._tets[i][1]],
																		  vertices[W._tets[i][2]], vertices[W._tets[i][3]])));
	}

	//create prisms

	for(unsigned i = 0; i < W._prisms.size(); ++i){
		volumes.push_back(*grid.create<Prism>(PrismDescriptor(vertices[W._prisms[i][0]], vertices[W._prisms[i][1]],
															  vertices[W._prisms[i][2]], vertices[W._prisms[i][3]],
															  vertices[W._prisms[i][4]], vertices[W._prisms[i][5]])));
	}

	//create pyramids

	for(unsigned i = 0; i < W._pyramids.size(); ++i){
		volumes.push_back(*grid.create<Pyramid>(PyramidDescriptor(vertices[W._pyramids[i][0]], vertices[W._pyramids[i][1]],
															  	  vertices[W._pyramids[i][2]], vertices[W._pyramids[i][3]],
															  	  vertices[W._pyramids[i][4]])));
	}


	unsigned subsetInd = 0;

	subset_handler_elements<Vertex>(sh, "vertices",
													 subsetInd,
													 grid_entry.vertices);
	subset_handler_elements<Edge>(sh, "edges",
													 subsetInd,
													 grid_entry.edges);
	subset_handler_elements<Face>(sh, "faces",
												 subsetInd,
												 grid_entry.faces);
	subset_handler_elements<Volume>(sh, "volumes",
												 subsetInd,
												 grid_entry.volumes);

	MySubsetHandlerEntry sh_entry(&sh);
	grid_entry.subsetHandlerEntries.push_back(sh_entry);

/*

	read_selector_elements<Vertex>(selOut, "vertices",
											gridEntry.vertices);

	read_selector_elements<Edge>(selOut, "edges",
											gridEntry.edges);

	read_selector_elements<Face>(selOut, "faces",
										gridEntry.faces);

	read_selector_elements<Volume>(selOut, "volumes",
										gridEntry.volumes);
*/

	MySelectorEntry sel_entry(&sel);
	grid_entry.selectorEntries.push_back(sel_entry);
	

	grid.set_options(gridopts);

	return true; //TODO
}

#endif //guard
