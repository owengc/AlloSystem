#ifndef INCLUDE_AL_GRAPHICS_MODEL_HPP
#define INCLUDE_AL_GRAPHICS_MODEL_HPP

/*
 *  A collection of functions and classes related to application mainloops
 *  AlloSphere Research Group / Media Arts & Technology, UCSB, 2009
 */

/*
	Copyright (C) 2006-2008. The Regents of the University of California (REGENTS). 
	All Rights Reserved.

	Permission to use, copy, modify, distribute, and distribute modified versions
	of this software and its documentation without fee and without a signed
	licensing agreement, is hereby granted, provided that the above copyright
	notice, the list of contributors, this paragraph and the following two paragraphs 
	appear in all copies, modifications, and distributions.

	IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
	SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
	OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
	BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
	PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
	HEREUNDER IS PROVIDED "AS IS". REGENTS HAS  NO OBLIGATION TO PROVIDE
	MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*/

#include <string>
#include <vector>
#include <map>

#include "allocore/protocol/al_Graphics.hpp"
#include "allocore/graphics/al_Common.hpp"
#include "allocore/graphics/al_Light.hpp"

#define MODEL_PARSER_BUF_LEN (256)

namespace al {
namespace gfx{

/// an object with internal GraphicsData and Material. 
class MaterialObject : public Drawable {
public:
	MaterialObject() : Drawable() {}
	virtual ~MaterialObject() {}
	
	virtual void draw(Graphics& gl) {}
	
	void name(std::string n) { mName = n; }

protected:	
	std::string     mName;
	GraphicsData	mData;			/* vertex data */
	Material *		mMaterial;		/* material used */
};

/// OBJ file reader. Parses .obj (and .mtl) files. A separate object will be created per group and/or usemtl command.
/// it stores the OBJ data intenally, divided per object. 
/// any of these objects can be instantiated (factory style) into GraphicsData and Materials
/// the names of objects/materials are available for iteration. 
/// Typically, after parsing the OBJ and instantiating the desired objects and materials, the OBJReader itself can be discarded. 
class OBJReader {
public:

	OBJReader() {}
	OBJReader(std::string path, std::string filename) { readOBJ(path, filename); }
	~OBJReader() {}
	
	void readOBJ(std::string path, std::string filename);
	
	
	
	
	struct Group {
		std::vector<int> indices;
		std::string material;
	};
	
	typedef std::map<std::string, Group>::iterator GroupIterator;
	GroupIterator groupsBegin() { return mGroups.begin(); }
	GroupIterator groupsEnd() { return mGroups.end(); }
	GroupIterator groupsFind(std::string name) { return mGroups.find(name); }
	
	// returns NULL if the group is not found, or has no vertices
	GraphicsData * createGraphicsData(GroupIterator group);
	
	struct Mtl {
		float shininess, optical_density;
		Color diffuse, ambient, specular;
		std::string diffuseMap, ambientMap, specularMap, bumpMap;
	};
	
	typedef std::map<std::string, Mtl>::iterator MtlIterator;
	MtlIterator materialsBegin() { return mMaterials.begin(); }
	MtlIterator materialsEnd() { return mMaterials.end(); }
	MtlIterator materialsFind(std::string name) { return mMaterials.find(name); }
	
	
	// returns NULL if the group is not found, or has no vertices
	Material * createMaterial(MtlIterator mtl);

protected:
	
	std::string mPath;				/* path to this model */
	std::string mFilename;				/* path to this model */
	std::string mMaterialLib;       /* name of the material library */
	
	FILE * file;
	char buf[MODEL_PARSER_BUF_LEN];
	
	struct FaceVertex {
		int vertex, texcoord, normal;
		
		FaceVertex(unsigned int vertex, unsigned int texcoord, unsigned int normal) : vertex(vertex), texcoord(texcoord), normal(normal) {}
		FaceVertex(const FaceVertex& cpy) : vertex(cpy.vertex), texcoord(cpy.texcoord), normal(cpy.normal) {}
	};
	
	std::map<std::string, Group> mGroups;
	std::map<std::string, Mtl> mMaterials;
	
	// parser:
	void readToken();
	bool hasToken();
	void readLine();
	void eatLine();
	std::string parseMaterial();
	std::string parseMaterialLib();
	std::string parseGroup();
	void parseVertex();
	void parseTexcoord();
	void parseNormal();
	void parseColor(Color& c);
	void parseFace(Group * g);
	
	void readMTL(std::string path, std::string name);
	
	int findFaceVertex(std::string s);
	int addFaceVertex(std::string buf, int v, int t, int n);
	void addTriangle(Group * g, unsigned int id0, unsigned int id1, unsigned int id2);
	
	std::vector<GraphicsData::Vertex> vertices;
	std::vector<GraphicsData::TexCoord2> texcoords;
	std::vector<GraphicsData::Normal> normals;
	std::vector<FaceVertex> face_vertices;
	
	// maps face vertices (as string) to corresponding this->vertices index
	// this way, avoid inserting the same vertex/tex/norm combo twice,
	// and use index buffer instead.
	std::map<std::string, int> vertexMap;
	
	

};



class Model {
public:

//	struct Triangle {
//		unsigned int indices[3];
//		GraphicsData::Normal normal;
//	};

	struct Group {
	public:
		Group(std::string name = "default") 
		: mName(name), mMaterial("default") {}
		
		std::string name() { return mName; }
		std::string material() { return mMaterial; }
		GraphicsData& data() { return mData; }
		
		void name(std::string n) { mName = n; }
		void material(std::string m) { mMaterial = m; }
		Vec3f& center() { return mCenter; }
		
		void draw(Graphics& gl) {
			gl.draw(data());
		}
		
	protected:
		friend class Model;
		std::string     mName;           /* name of this group */
		std::string     mMaterial;       /* index to material for group */
		GraphicsData	mData;
		Vec3f			mCenter;	
		//std::vector<Triangle> mTriangles;
	};
	
	Model() {}
	Model(std::string name) { readOBJ(name); }
	~Model() {}
	
	void readOBJ(std::string filename);
	
	void draw(Graphics& gl) {
		std::map<std::string, Group>::iterator iter = mGroups.begin();
		while (iter != mGroups.end()) {	
			Group& gr = iter->second;
			// apply materials:
			mMaterials[gr.material()]();
			// render data:
			gl.draw(gr.data());
			iter++;
		}
	}
	
	Material& material(std::string name);
	Group& group(std::string name);
	
	typedef std::map<std::string, Group>::iterator groupIterator;
	groupIterator groupsBegin() { return mGroups.begin(); }
	groupIterator groupsEnd() { return mGroups.end(); }
	
protected:

	void readMTL(std::string path);
	Group* addGroup(std::string name);

	std::map<std::string, Group> mGroups;
	std::map<std::string, Material> mMaterials;
	
	std::string mPath;				/* path to this model */
	std::string mMaterialLib;       /* name of the material library */
};

} // ::al::gfx
} // ::al

#endif