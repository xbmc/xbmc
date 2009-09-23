/*
 * Really Slick XScreenSavers
 * Copyright (C) 2002-2006  Michael Chapman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************
 *
 * This is a Linux port of the Really Slick Screensavers,
 * Copyright (C) 2002 Terence M. Welsh, available from www.reallyslick.com
 */
#include <common.hh>

#include <implicit.hh>
#include <vector.hh>

unsigned int Implicit::_width, Implicit::_height, Implicit::_length;
unsigned int Implicit::_width1, Implicit::_height1, Implicit::_length1;
Vector Implicit::_lbf;
float Implicit::_cw;

unsigned int Implicit::_cubeTable[256][17];
bool Implicit::_crawlTable[256][6];

#define WHL(X, Y, Z) (((X) * _height1 + (Y)) * _length1 + (Z))

void Implicit::init(
		unsigned int width, unsigned int height, unsigned int length, float cw
) {
	_width   = width;
	_height  = height;
	_length  = length;
	_width1  = width + 1;
	_height1 = height + 1;
	_length1 = length + 1;
	_lbf = Vector(width, height, length) * cw * -0.5;
	_cw = cw;

	static unsigned int ec[12][2] = {
		{ 0, 1 }, { 0, 2 }, { 1, 3 }, { 2, 3 }, { 0, 4 }, { 1, 5 },
		{ 2, 6 }, { 3, 7 }, { 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 }
	};
	static unsigned int next[8][12] = {
		{
			       1,        4, UINT_MAX, UINT_MAX,
			       0, UINT_MAX, UINT_MAX, UINT_MAX,
			UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX
		},
		{
			       5, UINT_MAX,        0, UINT_MAX,
			UINT_MAX,        2, UINT_MAX, UINT_MAX,
			UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX
		},
		{
			UINT_MAX,        3, UINT_MAX,        6,
			UINT_MAX, UINT_MAX,        1, UINT_MAX,
			UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX
		},
		{
			UINT_MAX, UINT_MAX,        7,        2,
			UINT_MAX, UINT_MAX, UINT_MAX,        3,
			UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX
		},
		{
			UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX,
			       9, UINT_MAX, UINT_MAX, UINT_MAX,
			       4,        8, UINT_MAX, UINT_MAX
		},
		{
			UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX,
			UINT_MAX,        8, UINT_MAX, UINT_MAX,
			      10, UINT_MAX,        5, UINT_MAX
		},
		{
			UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX,
			UINT_MAX, UINT_MAX,       11, UINT_MAX,
			UINT_MAX,        6, UINT_MAX,        9
		},
		{
			UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX,
			UINT_MAX, UINT_MAX, UINT_MAX,       10,
			UINT_MAX, UINT_MAX,       11,        7
		}
	};

	for (unsigned int i = 0; i < 256; ++i) {
		// impCubeTables::makeTriStripPatterns
		bool vertices[8];	// true if on low side of gradient (outside of surface)
		for (unsigned int j = 0; j < 8; ++j)
			vertices[j] = i & (1 << j);

		bool edges[12];
		bool edgesDone[12];
		for (unsigned int j = 0; j < 12; ++j) {
			edges[j] = vertices[ec[j][0]] ^ vertices[ec[j][1]];
			edgesDone[j] = false;
		}

		unsigned int totalCount = 0;

		// Construct lists of edges that form triangle strips
		// try starting from each edge (no need to try last 2 edges)
		for (unsigned int j = 0; j < 10; ++j) {
			unsigned int edgeList[7];
			unsigned int edgeCount = 0;
			for (
				unsigned int currentEdge = j;
				edges[currentEdge] && !edgesDone[currentEdge];
			) {
				edgeList[edgeCount++] = currentEdge;
				edgesDone[currentEdge] = true;
				unsigned int currentVertex = vertices[ec[currentEdge][0]] ?
					ec[currentEdge][0] : ec[currentEdge][1];
				currentEdge = next[currentVertex][currentEdge];
				while (!edges[currentEdge]) {
					currentVertex = (currentVertex != ec[currentEdge][0]) ?
						ec[currentEdge][0] : ec[currentEdge][1];
					currentEdge = next[currentVertex][currentEdge];
				}
			}
			if (edgeCount) {
				_cubeTable[i][totalCount++] = edgeCount;
				switch (edgeCount) {
				case 3:
					_cubeTable[i][totalCount++] = edgeList[0];
					_cubeTable[i][totalCount++] = edgeList[1];
					_cubeTable[i][totalCount++] = edgeList[2];
					break;
				case 4:
					_cubeTable[i][totalCount++] = edgeList[0];
					_cubeTable[i][totalCount++] = edgeList[1];
					_cubeTable[i][totalCount++] = edgeList[3];
					_cubeTable[i][totalCount++] = edgeList[2];
					break;
				case 5:
					_cubeTable[i][totalCount++] = edgeList[0];
					_cubeTable[i][totalCount++] = edgeList[1];
					_cubeTable[i][totalCount++] = edgeList[4];
					_cubeTable[i][totalCount++] = edgeList[2];
					_cubeTable[i][totalCount++] = edgeList[3];
					break;
				case 6:
					_cubeTable[i][totalCount++] = edgeList[0];
					_cubeTable[i][totalCount++] = edgeList[1];
					_cubeTable[i][totalCount++] = edgeList[5];
					_cubeTable[i][totalCount++] = edgeList[2];
					_cubeTable[i][totalCount++] = edgeList[4];
					_cubeTable[i][totalCount++] = edgeList[3];
					break;
				case 7:
					_cubeTable[i][totalCount++] = edgeList[0];
					_cubeTable[i][totalCount++] = edgeList[1];
					_cubeTable[i][totalCount++] = edgeList[6];
					_cubeTable[i][totalCount++] = edgeList[2];
					_cubeTable[i][totalCount++] = edgeList[5];
					_cubeTable[i][totalCount++] = edgeList[3];
					_cubeTable[i][totalCount++] = edgeList[4];
					break;
				}
				_cubeTable[i][totalCount] = 0;
			}
		}

		// impCubeTables::makeCrawlDirections
		_crawlTable[i][0] = edges[0] || edges[1] || edges[2] || edges[3];
		_crawlTable[i][1] = edges[8] || edges[9] || edges[10] || edges[11];
		_crawlTable[i][2] = edges[0] || edges[4] || edges[5] || edges[8];
		_crawlTable[i][3] = edges[3] || edges[6] || edges[7] || edges[11];
		_crawlTable[i][4] = edges[1] || edges[4] || edges[6] || edges[9];
		_crawlTable[i][5] = edges[2] || edges[5] || edges[7] || edges[10];
	}
}

Implicit::Implicit(ImplicitField field) : _serial(0), _field(field) {
	stdx::construct_n(_info, _width1 * _height1 * _length1);
	for (unsigned int i = 0; i < _width1; ++i) {
		for (unsigned int j = 0; j < _height1; ++j) {
			for (unsigned int k = 0; k < _length1; ++k) {
				unsigned int xyz = WHL(i, j, k);
				_info[xyz].cube.serial = 0;
				_info[xyz].corner.serial = 0;
				_info[xyz].corner.XYZ = _lbf + Vector(i, j, k) * _cw;
				_info[xyz].edge[X_AXIS].serial = 0;
				_info[xyz].edge[Y_AXIS].serial = 0;
				_info[xyz].edge[Z_AXIS].serial = 0;
			}
		}
	}
}

void Implicit::update(float threshold, const CrawlPointVector& crawlPoints) {
	_threshold = threshold;

	++_serial;

	_vertices.reset();
	_indices.reset();
	_lengths.reset();

	// crawl from every crawl point to create the surface
	CrawlPointVector::const_iterator e = crawlPoints.end();
	for (
		CrawlPointVector::const_iterator it = crawlPoints.begin();
		it != e;
		++it
	) {
		// find cube corresponding to crawl point
		Vector cube((*it - _lbf) / _cw);
		unsigned int x = Common::clamp((unsigned int)cube.x(), 0u, _width - 1);
		unsigned int y = Common::clamp((unsigned int)cube.y(), 0u, _height - 1);
		unsigned int z = Common::clamp((unsigned int)cube.z(), 0u, _length - 1);

		while (true) {
			unsigned int xyz = WHL(x, y, z);

			if (_info[xyz].cube.serial == _serial)
				break;	// escape if a finished cube

			// find mask for this cube
			unsigned char mask = calculateCube(xyz);

			if (mask == 255)
				break;	// escape if outside surface

			if (mask == 0) {
				// this cube is inside volume
				_info[xyz].cube.serial = _serial;
				if (--x < 0)
					break;
			} else {
				crawl(x, y, z);
				break;
			}
		}
	}
}

void Implicit::update(float threshold) {
	_threshold = threshold;

	++_serial;

	_vertices.reset();
	_indices.reset();
	_lengths.reset();

	// find gradient value at every corner
	unsigned int xyz = 0;
	for (unsigned int i = 0; i < _width; ++i) {
		for (unsigned int j = 0; j < _height; ++j) {
			for (unsigned int k = 0; k < _length; ++k) {
				calculateCube(xyz);
				polygonize(xyz);
				++xyz;
			}
			++xyz;
		}
		xyz += _length1;
	}
}

#define LBF 0x01
#define LBN 0x02
#define LTF 0x04
#define LTN 0x08
#define RBF 0x10
#define RBN 0x20
#define RTF 0x40
#define RTN 0x80

unsigned char Implicit::calculateCube(unsigned int xyz) {
	unsigned char mask = 0;
	if (_info[xyz + WHL(0, 0, 0)].corner.serial != _serial) {
		_info[xyz + WHL(0, 0, 0)].corner.value = _field(_info[xyz + WHL(0, 0, 0)].corner.XYZ);
		_info[xyz + WHL(0, 0, 0)].corner.serial = _serial;
	}
	if (_info[xyz + WHL(0, 0, 0)].corner.value < _threshold)
		mask |= LBF;
	if (_info[xyz + WHL(0, 0, 1)].corner.serial != _serial) {
		_info[xyz + WHL(0, 0, 1)].corner.value = _field(_info[xyz + WHL(0, 0, 1)].corner.XYZ);
		_info[xyz + WHL(0, 0, 1)].corner.serial = _serial;
	}
	if (_info[xyz + WHL(0, 0, 1)].corner.value < _threshold)
		mask |= LBN;
	if (_info[xyz + WHL(0, 1, 0)].corner.serial != _serial) {
		_info[xyz + WHL(0, 1, 0)].corner.value = _field(_info[xyz + WHL(0, 1, 0)].corner.XYZ);
		_info[xyz + WHL(0, 1, 0)].corner.serial = _serial;
	}
	if (_info[xyz + WHL(0, 1, 0)].corner.value < _threshold)
		mask |= LTF;
	if (_info[xyz + WHL(0, 1, 1)].corner.serial != _serial) {
		_info[xyz + WHL(0, 1, 1)].corner.value = _field(_info[xyz + WHL(0, 1, 1)].corner.XYZ);
		_info[xyz + WHL(0, 1, 1)].corner.serial = _serial;
	}
	if (_info[xyz + WHL(0, 1, 1)].corner.value < _threshold)
		mask |= LTN;
	if (_info[xyz + WHL(1, 0, 0)].corner.serial != _serial) {
		_info[xyz + WHL(1, 0, 0)].corner.value = _field(_info[xyz + WHL(1, 0, 0)].corner.XYZ);
		_info[xyz + WHL(1, 0, 0)].corner.serial = _serial;
	}
	if (_info[xyz + WHL(1, 0, 0)].corner.value < _threshold)
		mask |= RBF;
	if (_info[xyz + WHL(1, 0, 1)].corner.serial != _serial) {
		_info[xyz + WHL(1, 0, 1)].corner.value = _field(_info[xyz + WHL(1, 0, 1)].corner.XYZ);
		_info[xyz + WHL(1, 0, 1)].corner.serial = _serial;
	}
	if (_info[xyz + WHL(1, 0, 1)].corner.value < _threshold)
		mask |= RBN;
	if (_info[xyz + WHL(1, 1, 0)].corner.serial != _serial) {
		_info[xyz + WHL(1, 1, 0)].corner.value = _field(_info[xyz + WHL(1, 1, 0)].corner.XYZ);
		_info[xyz + WHL(1, 1, 0)].corner.serial = _serial;
	}
	if (_info[xyz + WHL(1, 1, 0)].corner.value < _threshold)
		mask |= RTF;
	if (_info[xyz + WHL(1, 1, 1)].corner.serial != _serial) {
		_info[xyz + WHL(1, 1, 1)].corner.value = _field(_info[xyz + WHL(1, 1, 1)].corner.XYZ);
		_info[xyz + WHL(1, 1, 1)].corner.serial = _serial;
	}
	if (_info[xyz + WHL(1, 1, 1)].corner.value < _threshold)
		mask |= RTN;

	_info[xyz].cube.mask = mask;
	return mask;
}

void Implicit::crawl(unsigned int x, unsigned int y, unsigned int z) {
	unsigned int xyz = WHL(x, y, z);
	if (_info[xyz].cube.serial == _serial)
		return;

	unsigned char mask = calculateCube(xyz);

	if (mask == 0 || mask == 255)
		return;

	// polygonize this cube if it intersects surface
	polygonize(xyz);

	// mark this cube as completed
	_info[xyz].cube.serial = _serial;

	// polygonize adjacent cubes
	if (_crawlTable[mask][0] && x > 0)
		crawl(x - 1, y, z);
	if (_crawlTable[mask][1] && x < _width - 1)
		crawl(x + 1, y, z);
	if (_crawlTable[mask][2] && y > 0)
		crawl(x, y - 1, z);
	if (_crawlTable[mask][3] && y < _height - 1)
		crawl(x, y + 1, z);
	if (_crawlTable[mask][4] && z > 0)
		crawl(x, y, z - 1);
	if (_crawlTable[mask][5] && z < _length - 1)
		crawl(x, y, z + 1);
}

// polygonize an individual cube
void Implicit::polygonize(unsigned int xyz) {
	unsigned char mask = _info[xyz].cube.mask;

	unsigned int counter = 0;
	unsigned int numEdges = _cubeTable[mask][counter];

	while (numEdges != 0) {
		_lengths.push_back(numEdges);
		for (unsigned int i = 0; i < numEdges; ++i) {
			// generate vertex position and normal data
			switch (_cubeTable[mask][i + counter + 1]) {
			case 0:
				addVertex(Z_AXIS, xyz + WHL(0, 0, 0));
				break;
			case 1:
				addVertex(Y_AXIS, xyz + WHL(0, 0, 0));
				break;
			case 2:
				addVertex(Y_AXIS, xyz + WHL(0, 0, 1));
				break;
			case 3:
				addVertex(Z_AXIS, xyz + WHL(0, 1, 0));
				break;
			case 4:
				addVertex(X_AXIS, xyz + WHL(0, 0, 0));
				break;
			case 5:
				addVertex(X_AXIS, xyz + WHL(0, 0, 1));
				break;
			case 6:
				addVertex(X_AXIS, xyz + WHL(0, 1, 0));
				break;
			case 7:
				addVertex(X_AXIS, xyz + WHL(0, 1, 1));
				break;
			case 8:
				addVertex(Z_AXIS, xyz + WHL(1, 0, 0));
				break;
			case 9:
				addVertex(Y_AXIS, xyz + WHL(1, 0, 0));
				break;
			case 10:
				addVertex(Y_AXIS, xyz + WHL(1, 0, 1));
				break;
			case 11:
				addVertex(Z_AXIS, xyz + WHL(1, 1, 0));
				break;
			}
		}
		counter += numEdges + 1;
		numEdges = _cubeTable[mask][counter];
	}
}

void Implicit::addVertex(Axis axis, unsigned int xyz) {
	const Info::Corner& corner = _info[xyz].corner;
	Info::Edge& edge           = _info[xyz].edge[axis];
	if (edge.serial == _serial) {
		_indices.push_back(edge.index);
		return;
	}

	// find position of vertex along this edge
	edge.serial = _serial;
	_indices.push_back(edge.index = _vertices.size());
	struct VertexData data;
	switch (axis) {
	case X_AXIS:
		data.x = corner.XYZ.x() +
			_cw * ((_threshold - corner.value)
			/ (_info[xyz + WHL(1, 0, 0)].corner.value - corner.value)),
		data.y = corner.XYZ.y();
		data.z = corner.XYZ.z();
		break;
	case 1:	// y-axis
		data.x = corner.XYZ.x();
		data.y = corner.XYZ.y() +
			_cw * ((_threshold - corner.value)
			/ (_info[xyz + WHL(0, 1, 0)].corner.value - corner.value));
		data.z = corner.XYZ.z();
		break;
	case 2:	// z-axis
		data.x = corner.XYZ.x();
		data.y = corner.XYZ.y();
		data.z = corner.XYZ.z() +
			_cw * ((_threshold - corner.value)
			/ (_info[xyz + WHL(0, 0, 1)].corner.value - corner.value));
		break;
	default:
		abort();
	}

	// find normal vector at vertex along this edge
	// first find normal vector origin value
	Vector pos(data.x, data.y, data.z);
	float no = _field(pos);
	// then find values at slight displacements and subtract
	data.nx = _field(pos - Vector(0.01f, 0.0f, 0.0f)) - no;
	data.ny = _field(pos - Vector(0.0f, 0.01f, 0.0f)) - no;
	data.nz = _field(pos - Vector(0.0f, 0.0f, 0.01f)) - no;
	float normalizer = 1.0f / std::sqrt(data.nx * data.nx + data.ny * data.ny + data.nz * data.nz);
	data.nx *= normalizer;
	data.ny *= normalizer;
	data.nz *= normalizer;

	// Add this vertex to surface
	_vertices.push_back(data);
}

void Implicit::draw(GLenum mode) const {
	glInterleavedArrays(GL_N3F_V3F, 0, _vertices.begin());
	LazyVector<unsigned int>::const_iterator index = _indices.begin();
	LazyVector<unsigned int>::const_iterator e = _lengths.end();
	for (LazyVector<unsigned int>::const_iterator it = _lengths.begin(); it < e; ++it) {
		glDrawElements(mode, *it, GL_UNSIGNED_INT, index);
		index += *it;
	}
}
