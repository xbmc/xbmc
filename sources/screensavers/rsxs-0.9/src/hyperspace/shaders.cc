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
 * Copyright (C) 2005 Terence M. Welsh, available from www.reallyslick.com
 */
#include <common.hh>

#include <extensions.hh>
#include <shaders.hh>

namespace Shaders {
	GLuint gooVP, gooFP, tunnelVP, tunnelFP;
};

#if USE_GL_EXTENSIONS

static std::string gooVPasm(
	"!!ARBvp1.0\n"

	"PARAM mvp[4] = { state.matrix.mvp };\n"
	"PARAM mvit[4] = { state.matrix.modelview.invtrans };\n"

	"TEMP oPos, fogtemp;\n"

	"DP4 oPos.x, mvp[0], vertex.position;\n"
	"DP4 oPos.y, mvp[1], vertex.position;\n"
	"DP4 oPos.z, mvp[2], vertex.position;\n"
	"DP4 oPos.w, mvp[3], vertex.position;\n"
	"MOV result.position, oPos;\n"

	"MOV result.color, vertex.color;\n"

	"MOV result.texcoord[0], vertex.normal;\n"

	// vector pointing at eye
	"SUB result.texcoord[1], mvit[3], vertex.position;\n"

	"SUB fogtemp, state.fog.params.z, oPos.z;\n"
	"MUL result.fogcoord.x, fogtemp.x, state.fog.params.w;\n"

	"END\n"
);

static std::string gooFPasm(
	"!!ARBfp1.0\n"

	"TEMP total, temp, eye_vec, norm, ref_vec, cube_vec, alpha;\n"

	// get normal from normal map
	"TEX norm, fragment.texcoord[0], texture[0], CUBE;\n"
	// remap to {-1, 1}
	"MAD norm, norm, 2.0, -1.0;\n"
	// get normal from normal map
	"TEX temp, fragment.texcoord[0], texture[1], CUBE;\n"
	// remap to {-1, 1}
	"MAD temp, temp, 2.0, -1.0;\n"
	// lerp between normals
	"LRP norm, fragment.color.a, temp, norm;\n"

	// normalize eye vector
	"DP3 temp.x, fragment.texcoord[1], fragment.texcoord[1];\n"
	"RSQ temp.x, temp.x;\n"
	"MUL eye_vec, fragment.texcoord[1], temp.x;\n"

	// calculate reflection vector
	"DP3 temp.x, eye_vec, norm;\n"
	"MUL temp.x, temp.x, 2.0;\n"
	"MUL temp, norm, temp.x;\n"
	"SUB ref_vec, temp, eye_vec;\n"

	// use reflection vector to find fragment color
	"TEX total, ref_vec, texture[2], CUBE;\n"

	// modulate with vertex color
	"MUL_SAT total, total, fragment.color;\n"

	// fresnel alpha
	"DP3 alpha.a, norm, eye_vec;\n"
	"ABS alpha.a, alpha.a;\n"
	"SUB alpha.a, 1.0, alpha.a;\n"
	"MUL_SAT total.a, alpha.a, alpha.a;\n"

	// fog
	"LRP result.color, fragment.fogcoord.x, total, state.fog.color;\n"

	"END\n"
);

static std::string tunnelVPasm(
	"!!ARBvp1.0\n"

	"PARAM mvp[4] = { state.matrix.mvp };\n"

	"TEMP temppos, fogtemp;\n"

	"DP4 result.position.x, mvp[0], vertex.position;\n"
	"DP4 result.position.y, mvp[1], vertex.position;\n"
	"DP4 temppos.z, mvp[2], vertex.position;\n"
	"MOV result.position.z, temppos.z;\n"
	"DP4 result.position.w, mvp[3], vertex.position;\n"

	"MOV result.color, vertex.color;\n"
	"MOV result.texcoord[0], vertex.texcoord[0];\n"

	"SUB fogtemp, state.fog.params.z, temppos.z;\n"
	"MUL result.fogcoord.x, fogtemp.x, state.fog.params.w;\n"

	"END\n"
);

static std::string tunnelFPasm(
	"!!ARBfp1.0\n"

	"TEMP temp, vertcolor, color, tex0, tex1, coord;\n"

	// texture
	"TEX tex0, fragment.texcoord[0], texture[0], 2D;\n"
	"TEX tex1, fragment.texcoord[0], texture[1], 2D;\n"
	"LRP tex0, fragment.color.a, tex1, tex0;\n"
	"MUL color, tex0, fragment.color;\n"

	// higher resolution texture
	"MUL coord, fragment.texcoord[0], 3.0;\n"
	"TEX tex0, coord, texture[0], 2D;\n"
	"TEX tex1, coord, texture[1], 2D;\n"
	"LRP tex0, fragment.color.a, tex1, tex0;\n"
	"MUL tex0, tex0, 0.5;\n"
	"MUL vertcolor, fragment.color, fragment.color;\n"
	"MAD color, tex0, vertcolor, color;\n"

	// set alpha
	"MOV color.a, 1.0;\n"

	// fog
	"LRP result.color, fragment.fogcoord.x, color, state.fog.color;\n"

	"END\n"
);

class Program : public ResourceManager::Resource<GLuint> {
private:
	GLuint _program;
public:
	Program(
		GLenum target, GLenum format, const std::string& program
	) {
		Extensions::glGenProgramsARB(1, &_program);
		Extensions::glBindProgramARB(target, _program);
		Extensions::glProgramStringARB(
			target, format, program.length(), program.c_str()
		);
	}
	~Program() { Extensions::glDeleteProgramsARB(1, &_program); }
	GLuint operator()() const { return _program; }
};

void Shaders::init() {
	Extensions::init();

	gooVP = Common::resources->manage(new Program(
		GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, gooVPasm
	));
	gooFP = Common::resources->manage(new Program(
		GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, gooFPasm
	));

	tunnelVP = Common::resources->manage(new Program(
		GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, tunnelVPasm
	));
	tunnelFP = Common::resources->manage(new Program(
		GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, tunnelFPasm
	));
}

#else // !USE_GL_EXTENSIONS

void Shaders::init() {
	throw Common::Exception("No shader support");
}

#endif // !USE_GL_EXTENSIONS
