/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id$
 *
 * $Log$
 */

#ifndef COMMON_HPP
#define COMMON_HPP
#include <vector>
#include <typeinfo>
#include <cstdarg>
#include <cassert>
#ifdef _MSC_sVER
#define strcasecmp(s, t) _strcmpi(s, t)
#endif

#ifdef DEBUG
//extern FILE *debugFile;
#endif

#ifdef MACOS
#include <cstdio>
extern FILE *fmemopen(void *buf, size_t len, const char *pMode);
#endif /** MACOS */

#include "dlldefs.h"

#define DEFAULT_FONT_PATH "/home/carm/fonts/courier1.glf"
#define MAX_TOKEN_SIZE 512
#define MAX_PATH_SIZE 4096

#define STRING_BUFFER_SIZE 1024*150
#define STRING_LINE_SIZE 1024


#ifdef LINUX
#include <cstdlib>
#define projectM_isnan isnan

#endif

#ifdef WIN32
#define projectM_isnan(x) ((x) != (x))
#endif

#ifdef MACOS
#define projectM_isnan(x) ((x) != (x))
#endif

#ifdef LINUX
#define projectM_fmax fmax
#endif

#ifdef WIN32
#define projectM_fmax(x,y) ((x) >= (y) ? (x): (y))
#endif

#ifdef MACOS
#define projectM_fmax(x,y) ((x) >= (y) ? (x): (y))
#endif

#ifdef LINUX
#define projectM_fmin fmin
#endif

#ifdef WIN32
#define projectM_fmin(x,y) ((x) <= (y) ? (x): (y))
#endif

#ifdef MACOS
#define projectM_fmin(x,y) ((x) <= (y) ? (x): (y))
#endif

#ifndef TRUE
#define TRUE true
#endif

#ifndef FALSE
#define FALSE false
#endif


#define MAX_DOUBLE_SIZE  10000000.0
#define MIN_DOUBLE_SIZE -10000000.0

#define MAX_INT_SIZE  10000000
#define MIN_INT_SIZE -10000000

/* default float initial value */
#define DEFAULT_DOUBLE_IV 0.0

/* default float lower bound */
#define DEFAULT_DOUBLE_LB MIN_DOUBLE_SIZE

/* default float upper bound */
#define DEFAULT_DOUBLE_UB MAX_DOUBLE_SIZE

#ifdef WIN32
#include <float.h>
#define isnan _isnan
#endif /** WIN32 */

/** Per-platform path separators */
#define WIN32_PATH_SEPARATOR '\\'
#define UNIX_PATH_SEPARATOR '/'
#ifdef WIN32
#define PATH_SEPARATOR WIN32_PATH_SEPARATOR
#else
#define PATH_SEPARATOR UNIX_PATH_SEPARATOR
#endif /** WIN32 */
#include <string>

const unsigned int NUM_Q_VARIABLES(32);
const std::string PROJECTM_FILE_EXTENSION("prjm");
const std::string MILKDROP_FILE_EXTENSION("milk");
const std::string PROJECTM_MODULE_EXTENSION("so");

 template <class TraverseFunctor, class Container>
  void traverse(Container & container)
  {

    TraverseFunctor functor;

    for (typename Container::iterator pos = container.begin(); pos != container.end(); ++pos)
    {
      assert(pos->second);
      functor(pos->second);
    }

  }


  template <class TraverseFunctor, class Container>
  void traverseVector(Container & container)
  {

    TraverseFunctor functor;

    for (typename Container::iterator pos = container.begin(); pos != container.end(); ++pos)
    {
      assert(*pos);
      functor(*pos);
    }

  }

  template <class TraverseFunctor, class Container>
  void traverse(Container & container, TraverseFunctor & functor)
  {

    for (typename Container::iterator pos = container.begin(); pos != container.end(); ++pos)
    {
      assert(pos->second);
      functor(pos->second);
    }

  }

  namespace TraverseFunctors
  {
    template <class Data>
    class Delete
    {

    public:

      void operator() (Data * data)
      {
        assert(data);
        delete(data);
      }

    };
  }


inline std::string parseExtension(const std::string & filename) {

std::size_t start = filename.find_last_of('.');

if (start == std::string::npos || start >= (filename.length()-1))
	return "";
else
	return filename.substr(start+1, filename.length());

}


inline double meanSquaredError(const double & x, const double & y) {
		return (x-y)*(x-y);
}


enum PresetRatingType {
	FIRST_RATING_TYPE = 0,
	HARD_CUT_RATING_TYPE = FIRST_RATING_TYPE,
	SOFT_CUT_RATING_TYPE,
	LAST_RATING_TYPE = SOFT_CUT_RATING_TYPE,
	TOTAL_RATING_TYPES = SOFT_CUT_RATING_TYPE+1
};


typedef std::vector<int> RatingList;

#endif


