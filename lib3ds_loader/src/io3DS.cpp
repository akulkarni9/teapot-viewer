/****************************************************************************
* Copyright (C) 2007-2010 by E.Heidt  http://teapot-viewer.sourceforge.net/ *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
****************************************************************************/

#include <SceneIO.h>
using namespace eh;

#include <boost/filesystem/path.hpp>

#include <lib3ds.h>

#include <string.h>
#include <math.h>
#include <map>

class C3DSLoader: public SceneIO::IPlugIn
{
private:
	float m_nCount;
	float m_iCount;

	boost::filesystem::wpath m_path;

	void set_path( const boost::filesystem::wpath& _path )
	{
		m_path = _path;
		m_path.remove_filename();
	}

	std::wstring abs_path(const boost::filesystem::wpath& file)
	{
		if(!file.is_complete())
			return m_path.directory_string() + file.file_string();
		else
			return file.string();
	}
	std::wstring abs_path(const std::string& file)
	{
		return abs_path( std::wstring( file.begin(), file.end() ) );
	}

	IVertexBuffer::ptr m_pVB;
public:

	C3DSLoader():
		m_nCount(0), 
		m_iCount(0), 
		m_pVB( createVertexBuffer( sizeof(Float)*8 ))
	{
	}
	virtual ~C3DSLoader()
	{

	}

	void countNodes(Lib3dsFile *f, Lib3dsNode *first_node) 
	{
		Lib3dsNode *p;
		for (p = first_node; p; p = p->next) {
			if (p->type == LIB3DS_NODE_MESH_INSTANCE) 
			{
				if (Lib3dsMesh *mesh = lib3ds_file_mesh_for_node(f, (Lib3dsNode*)p)) 
					m_nCount += mesh->nfaces;

				countNodes(f, p->childs);
			}
		}
	}

	void makeNodes(Lib3dsFile *f, Lib3dsNode *first_node, SceneNode::vec& nodes, SceneIO::progress_callback& progress)
	{
		Lib3dsNode *p;
		for (p = first_node; p; p = p->next) {
			if (p->type == LIB3DS_NODE_MESH_INSTANCE) 
			{
				if(SceneNode::ptr node = makeNode(f, (Lib3dsMeshInstanceNode*)p, progress))
					nodes.push_back( node );

				makeNodes(f, p->childs, nodes, progress);
			}
		}
	}
	SceneNode::ptr makeNode(Lib3dsFile *f, Lib3dsMeshInstanceNode *node, SceneIO::progress_callback& progress)
	{
		Lib3dsMesh *mesh = lib3ds_file_mesh_for_node(f, (Lib3dsNode*)node);
		if (!mesh || !mesh->vertices) 
			return NULL;

		//fprintf(o, "# object %s\n", node->base.name);
		//fprintf(o, "g %s\n", node->instance_name[0]? node->instance_name : node->base.name);

		Poly normals(mesh->nvertices);

		for (int i = 0; i < mesh->nfaces; ++i) 
		{
			progress(++m_iCount/m_nCount);
			
			const Vec3& va = reinterpret_cast<const Vec3&>(mesh->vertices[mesh->faces[i].index[0]]);
			const Vec3& vb = reinterpret_cast<const Vec3&>(mesh->vertices[mesh->faces[i].index[1]]);
			const Vec3& vc = reinterpret_cast<const Vec3&>(mesh->vertices[mesh->faces[i].index[2]]);

			Vec3 n = calcNormal(va,vb,vc);
			normals[mesh->faces[i].index[0]] = (normals[mesh->faces[i].index[0]]+n).normalized();
			normals[mesh->faces[i].index[1]] = (normals[mesh->faces[i].index[1]]+n).normalized();
			normals[mesh->faces[i].index[2]] = (normals[mesh->faces[i].index[2]]+n).normalized();
		}

		std::map<int, Uint_vec > faces;
		Uint_vec edges;

		for (int i = 0; i < mesh->nfaces; ++i) 
		{
			Uint_vec& indices = faces[mesh->faces[i].material];

			const Vec3& va = reinterpret_cast<const Vec3&>(mesh->vertices[mesh->faces[i].index[0]]);
			const Vec3& vb = reinterpret_cast<const Vec3&>(mesh->vertices[mesh->faces[i].index[1]]);
			const Vec3& vc = reinterpret_cast<const Vec3&>(mesh->vertices[mesh->faces[i].index[2]]);

			const Vec3& na = reinterpret_cast<const Vec3&>(normals[mesh->faces[i].index[0]]);
			const Vec3& nb = reinterpret_cast<const Vec3&>(normals[mesh->faces[i].index[1]]);
			const Vec3& nc = reinterpret_cast<const Vec3&>(normals[mesh->faces[i].index[2]]);

			Vec3 ta = Vec3::Null();
			Vec3 tb = Vec3::Null();
			Vec3 tc = Vec3::Null();

			if (mesh->texcos != 0)
			{
				ta.x = mesh->texcos[mesh->faces[i].index[0]][0];
				ta.y = mesh->texcos[mesh->faces[i].index[0]][1];

				tb.x = mesh->texcos[mesh->faces[i].index[1]][0];
				tb.y = mesh->texcos[mesh->faces[i].index[1]][1];

				tc.x = mesh->texcos[mesh->faces[i].index[2]][0];
				tc.y = mesh->texcos[mesh->faces[i].index[2]][1];
			}

			indices.push_back( m_pVB->addVertex( va, na, ta) );
			indices.push_back( m_pVB->addVertex( vb, nb, tb) );
			indices.push_back( m_pVB->addVertex( vc, nc, tc) );

		}

		ShapeNode::ptr pShape = ShapeNode::create();

		if(edges.size() > 0)
			pShape->addGeometry( Material::Black(), Geometry::create(Geometry::LINES, m_pVB, edges) );

		for(std::map<int, Uint_vec >::const_iterator it = faces.begin(); it != faces.end(); it++)
		{
			if(it->first != -1)
			{
				Lib3dsMaterial* mat = f->materials[it->first];

				Material::ptr pMat = Material::create();

				pMat->setDiffuse( RGBA(mat->diffuse[0],mat->diffuse[1],mat->diffuse[2], mat->transparency) );
				pMat->setAmbient( RGBA(mat->ambient[0],mat->ambient[1],mat->ambient[2], 0) );
				pMat->setSpecular( RGBA(mat->specular[0],mat->specular[1],mat->specular[2],0) );
				pMat->setEmission( RGBA(0,0,0,0) );
				pMat->setSpecularFactor(0);
				
				if(strlen(mat->texture1_map.name) > 0)
				{
					if(pMat->getDiffuse().isBlack())
						pMat->setDiffuse( RGBA(1,1,1) );

					pMat->setTexture( Texture::createFromFile( abs_path(mat->texture1_map.name) ) );
				}

				//CString sTextureFile2( mat->texture2_map.name );
				//if(!sTextureFile2.IsEmpty())
				//{
				//	pMat->addTexture( Texture::createFromFile( m_sPath + sTextureFile2, true ) );
				//}

				pShape->addGeometry( pMat, Geometry::create(Geometry::TRIANGLES, m_pVB, it->second) );
			}
			else
				pShape->addGeometry( Material::White(), Geometry::create(Geometry::TRIANGLES, m_pVB, it->second ) );
		}


		float inv_matrix[4][4], M[4][4];

		lib3ds_matrix_copy(M, node->base.matrix);
		lib3ds_matrix_translate(M, -node->pivot[0], -node->pivot[1], -node->pivot[2]);
		lib3ds_matrix_copy(inv_matrix, mesh->matrix);
		lib3ds_matrix_inv(inv_matrix);
		lib3ds_matrix_mult(M, M, inv_matrix);

		return GroupNode::create( pShape, reinterpret_cast<Matrix&>(M) );
	}

	// Interface //

	virtual bool read(const std::wstring& sFile, Scene::ptr pScene, SceneIO::progress_callback& progress)
	{
		set_path(sFile);
		std::string sFileName(sFile.begin(), sFile.end());

		m_nCount = 0;
		m_iCount = 0;

		Lib3dsFile* f = lib3ds_file_open(sFileName.c_str());
		if (!f)
			return false;

		if (!f->nodes)
			lib3ds_file_create_nodes_for_meshes(f);

		lib3ds_file_eval(f, 0);

		SceneNode::vec nodes;

		countNodes(f, f->nodes);
		makeNodes(f, f->nodes, nodes, progress);

		for(size_t i = 0; i < nodes.size(); i++)
			pScene->insertNode( nodes[i] );

		for(int i = 0; i < f->cameras_size; i++)
		{
			Lib3dsCamera* c = f->cameras[i];
			Float h = 2.f* (tan(DEG2RAD(c->fov/2.f)) * c->near_range);
			pScene->addCamera( Camera::create(c->name, h, h, c->near_range, c->far_range,
					reinterpret_cast<Vec3&>(c->position[0]), reinterpret_cast<Vec3&>(c->target[0]), Vec3(0,0,1)));
		}

		lib3ds_file_free(f);

		return true;

	}

	virtual std::wstring about() const
	{
		return L"lib3ds_loader";
	}
	virtual Uint file_type_count() const
	{
		return 1;
	}
	virtual std::wstring file_type(Uint i) const
	{
		return L"3D Studio Max Models";
	}
	virtual std::wstring file_exts(Uint i) const
	{
		return L"*.3ds";
	}
	virtual std::wstring rpath() const
	{
		return L"";
	}
	virtual bool canWrite(Uint i) const
	{
		return false;
	}

	virtual bool canRead(Uint i) const
	{
		return true;
	}

	virtual bool write(const std::wstring& sFile, Scene::ptr pScene, SceneIO::progress_callback& progress)
	{
		return false;
	}
};

extern "C"
#if defined(_MSC_VER)
__declspec(dllexport)
#endif
SceneIO::IPlugIn* XcreatePlugIn()
{
	return new C3DSLoader();
}
