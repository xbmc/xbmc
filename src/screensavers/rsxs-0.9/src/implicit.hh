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
#ifndef _IMPLICIT_HH
#define _IMPLICIT_HH

#include <common.hh>

#include <color.hh>
#include <memory>
#include <vector.hh>

typedef float (*ImplicitField)(const Vector&);
typedef std::list<Vector> CrawlPointVector;

template <typename T> class LazyVector {
private:
	T* _data;
	unsigned int _used, _capacity;
public:
	LazyVector() : _data(new T[1000]), _used(0), _capacity(1000) {}
	LazyVector(const LazyVector& lv) : _data(new T[lv._capacity]), 
		_used(lv._used), _capacity(lv._capacity) {
		std::copy(lv._data, lv._data + _used, _data);
	}
	~LazyVector() { delete[] _data; }

	void reset() { _used = 0; }
	unsigned int size() const { return _used; }

	typedef const T* const_iterator;
	const T* begin() const { return _data; }
	const T* end() const { return _data + _used; }

	void push_back(const T& v) {
		if (_used == _capacity) {
			_capacity += 1000;
			T* temp = new T[_capacity];
			std::uninitialized_copy(_data, _data + _used, temp);
			delete[] _data;
			_data = temp;
		}
		_data[_used++] = v;
	}
};

enum Axis {
	X_AXIS,
	Y_AXIS,
	Z_AXIS
};

class Implicit : public ResourceManager::Resource<void> {
private:
	static unsigned int _width, _height, _length;
	static unsigned int _width1, _height1, _length1;
	static Vector _lbf;
	static float _cw;

	static unsigned int _cubeTable[256][17];
	static bool _crawlTable[256][6];
public:
	static void init(unsigned int, unsigned int, unsigned int, float);
	void operator()() const {}
private:
	struct Info {
		struct Cube {
			unsigned int serial;
			unsigned char mask;
		} cube;
		struct Corner {
			unsigned int serial;
			float value;
			Vector XYZ;
		} corner;
		struct Edge {
			unsigned int serial;
			unsigned int index;
		} edge[3];
	};
	std::vector<Info> _info;

	struct VertexData {
		float nx, ny, nz;
		float x, y, z;
	};

	unsigned int _serial;

	ImplicitField _field;
	float _threshold;

	LazyVector<VertexData> _vertices;
	LazyVector<unsigned int> _indices;
	LazyVector<unsigned int> _lengths;

	inline unsigned char calculateCube(unsigned int);
	inline void crawl(unsigned int, unsigned int, unsigned int);
	inline void polygonize(unsigned int);
	inline void addVertex(Axis, unsigned int);
public:
	Implicit(ImplicitField);

	void update(float, const CrawlPointVector&);
	void update(float);
	void draw(GLenum = GL_TRIANGLE_STRIP) const;
};

#endif // _IMPLICIT_HH
