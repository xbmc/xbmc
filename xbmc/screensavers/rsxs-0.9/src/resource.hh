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
#ifndef _RESOURCE_HH
#define _RESOURCE_HH

#include <common.hh>

#include <pngimage.hh>

class ResourceManager {
public:
	typedef Common::Exception Exception;
private:
	class _ResourceBase {
	protected:
		_ResourceBase() {}
	public:
		virtual ~_ResourceBase() {}
	};
	std::list<_ResourceBase*> _resources;
public:
	template <typename Ret>
	class Resource : public _ResourceBase {
	protected:
		Resource() {}
	public:
		virtual ~Resource() {}
		virtual Ret operator()() const = 0;
	};
public:
	template <typename Ret>
	Ret manage(Resource<Ret> *resource) {
		_resources.push_front(resource);
		return (*resource)();
	}
private:
	class DisplayLists : public Resource<GLuint> {
	private:
		GLuint _base;
		GLsizei _num;
	public:
		DisplayLists(GLsizei num) : _base(glGenLists(num)), _num(num) {
			if (_base == 0) throw Exception("No more available GL display lists");
		}
		~DisplayLists() { glDeleteLists(_base, _num); }
		GLuint operator()() const { return _base; }
	};

	class Texture : public Resource<GLuint> {
	private:
		GLuint _texture;
	public:
		Texture(GLenum target, GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT) {
			glGenTextures(1, &_texture);
			glBindTexture(target, _texture);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapS);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapT);
		}
		~Texture() { glDeleteTextures(1, &_texture); }
		GLuint operator()() const { return _texture; }
	};
public:
	~ResourceManager() { stdx::destroy_all_ptr(_resources); }

	GLuint genLists(GLsizei num) {
		return manage(new DisplayLists(num));
	}

	GLuint genTexture(
		GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT
	) {
		return manage(new Texture(
			GL_TEXTURE_2D, minFilter, magFilter, wrapS, wrapT
		));
	}

	GLuint genTexture(
		GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT,
		GLint numComponents, GLsizei width, GLsizei height,
		GLenum format, GLenum type, const GLvoid* data, bool mipmap = true
	) {
		GLuint result = genTexture(minFilter, magFilter, wrapS, wrapT);
		if (mipmap)
			gluBuild2DMipmaps(
				GL_TEXTURE_2D, numComponents, width, height,
				format, type, data
			);
		else
			glTexImage2D(
				GL_TEXTURE_2D, 0, numComponents, width, height, 0,
				format, type, data
			);
		return result;
	}

	GLuint genTexture(
		GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT,
		const PNG& png, bool mipmap = true
	) {
		return genTexture(
			minFilter, magFilter, wrapS, wrapT,
			png.numComponents(), png.width(), png.height(),
			png.format(), png.type(), png.data(), mipmap
		);
	}

#ifdef GL_ARB_texture_cube_map

	GLuint genCubeMapTexture(
		GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT
	) {
		return manage(new Texture(
			GL_TEXTURE_CUBE_MAP_ARB, minFilter, magFilter, wrapS, wrapT
		));
	}

	GLuint genCubeMapTexture(
		GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT,
		GLint numComponents, GLsizei width, GLsizei height,
		GLenum format, GLenum type, const GLvoid* data, bool mipmap = true
	) {
		GLuint result = genCubeMapTexture(minFilter, magFilter, wrapS, wrapT);
		if (mipmap) {
			gluBuild2DMipmaps(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, numComponents, width, height,
				format, type, data
			);
			gluBuild2DMipmaps(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, numComponents, width, height,
				format, type, data
			);
			gluBuild2DMipmaps(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, numComponents, width, height,
				format, type, data
			);
			gluBuild2DMipmaps(
				GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, numComponents, width, height,
				format, type, data
			);
			gluBuild2DMipmaps(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, numComponents, width, height,
				format, type, data
			);
			gluBuild2DMipmaps(
				GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, numComponents, width, height,
				format, type, data
			);
		} else {
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, 0, numComponents, width, height, 0,
				format, type, data
			);
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 0, numComponents, width, height, 0,
				format, type, data
			);
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, 0, numComponents, width, height, 0,
				format, type, data
			);
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, 0, numComponents, width, height, 0,
				format, type, data
			);
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, 0, numComponents, width, height, 0,
				format, type, data
			);
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, 0, numComponents, width, height, 0,
				format, type, data
			);
		}
		return result;
	}

	GLuint genCubeMapTexture(
		GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT,
		const PNG& png, bool mipmap = true
	) {
		return genCubeMapTexture(
			minFilter, magFilter, wrapS, wrapT,
			png.numComponents(), png.width(), png.height(),
			png.format(), png.type(), png.data()
		);
	}

#endif // GL_ARB_texture_cube_map
};

#endif // _RESOURCE_HH
