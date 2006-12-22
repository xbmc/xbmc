/*********************************************************************NVMH4****
Path:  
File:  

Copyright NVIDIA Corporation 2003
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.


Comments:
	send questions or comments to sdietrich@nvidia.com, cbrewer@nvidia.com  

    MeshMender's main purpose is to generate a tangent space basis for
  per pixel lighting.  Given a set of vertices and normal map texture coordinates,
  MeshMender will return a set of normals, binormals, and tangents taking into
  account texture mirroring.  
  
	Meshmender also lets you choose a minimum angle between neighboring 
  triangles vectors to determine whether or not they can smooth together.
  If they can't smooth together, then MeshMender automagically mends the mesh
  so that each unsmoothable neighbor has it's own set of vectors in a unique vertex.



How to use MeshMender:
	assumes that you have an array of vertices in myVerts and an array
	of indices in myIndices.
	the verts should be of the format
		float x, float y, float z, 
		float s,
		float t

------------------------


	std::vector< MeshMender::Vertex > theVerts;
	std::vector< unsigned int > theIndices;
	std::vector< unsigned int > mappingNewToOld;

	//fill up the vectors with your mesh's data
	for(DWORD i = 0; i < numVerts; ++i)
	{
		MeshMender::Vertex v;
		v.pos = myVerts[i].pos;
		v.s = myVerts[i].s;
		v.t = myVerts[i].t;
		//meshmender will computer normals, tangents, and binormals, no need to fill those in.
		//however, if you do not have meshmender compute the normals, you _must_ pass in valid
		//normals to meshmender
		theVerts.push_back(v);
	}

	for(DWORD ind= 0 ; ind< numIndices; ++ind)
	{
		theIndices.push_back(myIndices[ind]);
	}

    //pass it in to the mender to do it's stuff
	mender.Mend( theVerts,  theIndices, mappingNewToOld,
				  minNormalCreaseCos,
				  minTangentCreaseCos,
				  minBinormalCreaseCos,
				  weightNormalsByArea,
				  MeshMender::CALCULATE_NORMALS,
				  MeshMender::DONT_RESPECT_SPLITS
				  MeshMender::DONT_FIX_CYLINDRICAL);

    //then update your mesh with the data provided in the Vertex vector
	//NOTE that MeshMender may add vertices to your mesh if needs to split
	//because of smoothing groups.  So you can't just reuse your old vertex
	//and index data. you have to update it.
	//to help associate any special per vertex data you had in your original mesh
	//you can use the mappingNewToOld ( see notes in comments below for Mend )




******************************************************************************/
#ifndef NV_MESH_MENDER_H
#define NV_MESH_MENDER_H

#pragma warning( disable : 4786)
#pragma warning( disable : 4100)


class MeshMender
{

	public:
		

		class Vertex
		{
		public:
			D3DXVECTOR3 pos;
			D3DXVECTOR3 normal;
			float       s;
			float       t;
			D3DXVECTOR3 tangent;
			D3DXVECTOR3 binormal;
			enum 
			{
				FVF = D3DFVF_XYZ |
					  D3DFVF_NORMAL |
					  D3DFVF_TEX3 |
					  D3DFVF_TEXCOORDSIZE2( 0 ) |
					  D3DFVF_TEXCOORDSIZE3( 1 ) |
					  D3DFVF_TEXCOORDSIZE3( 2 )
			};
			Vertex::Vertex():pos(0.0f ,0.0f ,0.0f )
										,normal(0.0f ,0.0f ,0.0f )
										,s(0.0f )
										,t(0.0f )
										,tangent(0.0f ,0.0f ,0.0f )
										,binormal(0.0f ,0.0f ,0.0f ){}
		};

		enum NormalCalcOption{ DONT_CALCULATE_NORMALS , CALCULATE_NORMALS };
		enum ExistingSplitOption{ DONT_RESPECT_SPLITS , RESPECT_SPLITS };
		enum CylindricalFixOption{ DONT_FIX_CYLINDRICAL , FIX_CYLINDRICAL };

		//Mend - given a mesh, output the new data complete with smoothed
		//		  normals, binormals, and tangents
		//
		//RETURNS true on success, false on failure
		//
		//theVerts  - should be initialized with your mesh data, NOTE that when
		//			  mesh mender is done with it, the number of vertices may grow
		//			  and it will be filled with normals, tangents and binormals
		//
		//theIndices - should be initialized with your mesh indices
		//				will contain the new indices..we are not adding triangles, 
		//				  so the number of indices passed back should be the same as the 
		//				  number of indices passed in, but they may point to new vertices now.
		//
		//mappingNewToOldVert - this should be passed in as an empty vector. after mending
		//				it will contain a mapping of newvertexindex -> oldvertexindex
		//				so it could be used to map any per vertex data you had in your original
		//				mesh to the new mesh like so:
		//				
		//					for each new vertex index
		//						newVert[index]->myData = oldVert[ mappingNewToOldVert[index]]->myData;
		//				
		//				where myData is some custom vertex data in your original mesh.
		//
		//minNormalsCreaseCosAngle - the minimum cosine of the angle between normals
		//							 so that they are allowed to be smoothed together
		//							 ranges between -1.0 and +1.0
		//							 this is ignored if computeNormals is set to DONT_CALCULATE_NORMALS
		//							 
		//
		//minTangentsCreaseCosAngle - the minimum cosine of the angle between tangents
		//							  so that they are allowed to be smoothed together
		//							  ranges between -1.0 and +1.0
		//
		//minBinormalsCreaseCosAngle - the minimum cosine of the angle between binormals
		//							   so that they are allowed to be smoothed together
		//		 					   ranges between -1.0 and +1.0
		//
		//weightNormalsByArea - an ammount to blend the normalized face normal, and the 
		//						unnormalized face normal together.  Thus weighting the
		//						normal by the face area by a given ammount
		//						ranges between 0.0 and +1.0
		//						0.0 means use the normalized face normals (not weighted by area)
		//						1.0 means use the unnormalized face normal(weighted by area)
		//						this is ignored if computeNormals is set to DONT_CALCULATE_NORMALS
		//
		//computeNormals - should mesh mender calculate normals? If this is set to DONT_CALCULATE_NORMALS 
		//					then the vertex normals after mesh mender is called will be the 
		//					same ones you pass in.  If you are automatically calculating normals yourself,
		//					you may find that meshmender provides greater control over how normals are smoothed
		//					together. I've been able to get better results using the Crease angle with 
		//					meshmender's smoothing groups
		//
		//respectExistingSplits - DONT_RESPECT_SPLITS means that neighboring triangles for smoothing will be determined 
		//						  based on position and not on indices.
		//						  RESPECT_SPLITS means that neighboring triangles will be determined based on the indices of the
		//						  triangle and not the positions of the vertices.
		//						  you can usually get better smoothing by not respecting existing splits
		//						  only respect them if you know they should be respected.  
		//
		//fixCylindricalWrapping - DONT_FIX_CYLINDRICAL means take the texture coordinates as they come
		//						   FIX_CYLINDRICAL means we might need to split the verts
		//						   at that point and generate the proper texture coordinate.
		//						   for instance, if we have tex coords   0.9 -> 0.0-> 0.2 we would need to add
		//						   a new vert so that we have       0.9 -> 1.0  0.0-> 0.2
		//						   this is only supported for texture coordinates in the range [ 0.0f , 1.0f ]
		//						   NOTE: don't leave this on for all meshes, only use it when you know
		//							you need it. If you have polygons that map to a large area in texture space
		//							this option could mess up the texture coordinates
		bool Mend( 		  std::vector< Vertex >&    theVerts,
						  std::vector< unsigned int >& theIndices,
						  std::vector< unsigned int >& mappingNewToOldVert,
						  const float minNormalsCreaseCosAngle = 0.0f,
						  const float minTangentsCreaseCosAngle = 0.0f ,
						  const float minBinormalsCreaseCosAngle = 0.0f,
						  const float weightNormalsByArea = 1.0f,
						  const NormalCalcOption computeNormals = CALCULATE_NORMALS,
						  const ExistingSplitOption respectExistingSplits = DONT_RESPECT_SPLITS,
						  const CylindricalFixOption fixCylindricalWrapping = DONT_FIX_CYLINDRICAL);	

		MeshMender();

		~MeshMender();

	protected:

		float MinNormalsCreaseCosAngle; 
		float MinTangentsCreaseCosAngle;
		float MinBinormalsCreaseCosAngle;
		float WeightNormalsByArea;
		ExistingSplitOption  m_RespectExistingSplits;

		class CanSmoothChecker;
		friend class CanSmoothChecker;
		friend class CanSmoothNormalsChecker;
		friend class CanSmoothTangentsChecker;
		friend class CanSmoothBinormalsChecker;
		
		//sets up any internal data structures needed
		void SetUpData(	std::vector< Vertex >&			theVerts,
						std::vector< unsigned int >&		theIndices,
						std::vector< unsigned int >& mappingNewToOldVert,
						const NormalCalcOption computeNormals);

		typedef size_t NeighborhoodID;
		typedef size_t TriID;
		typedef std::vector<TriID> TriangleList;
		
		struct Triangle
		{
			size_t indices[3];

			//per face values
			D3DXVECTOR3 normal;
			D3DXVECTOR3 tangent;
			D3DXVECTOR3 binormal;

			//helper flags
			bool handled; 
			NeighborhoodID group;
			void Reset();
			
			TriID myID;//a global id used to keep track of tris'

		};
		
		std::vector<Triangle> m_Triangles;

		//each vertex has a set of triangles that contain it.
		//those triangles are considered to be that vertex's children
		typedef std::map<D3DXVECTOR3, TriangleList > VertexChildrenMap;
		VertexChildrenMap m_VertexChildrenMap;

		//a neighbor group is defined to be the list of traingles
		//that all fall arround a single vertex, and can smooth with
		//eachother
		typedef std::vector<TriangleList > NeighborGroupList;

		size_t m_originalNumVerts; 

		//sets up the normal, binormal, and tangent for a triangle
		//assumes the triangle indices are set to match whats in the verts
		void SetUpFaceVectors(	Triangle& t ,
								const std::vector< Vertex >&verts,
								const NormalCalcOption computeNormals);

		//function responsible for growing the neighbor hood groups
		//arround a vertex
		void BuildGroups(	Triangle* tri, //the tri of interest
							TriangleList& possibleNeighbors, //all tris arround a vertex
							NeighborGroupList& neighborGroups, //the neighbor groups to be updated
							std::vector< Vertex >& theVerts,
							CanSmoothChecker* smoothChecker,
							const float& minCreaseAngle);

		//given 2 triangles, fill the two neighbor pointers with either
		//null or valid Triangle pointers.
		void FindNeighbors(	Triangle* tri, 
							TriangleList&possibleNeighbors, 
							Triangle** neighbor1, 
							Triangle** neighbor2,
							std::vector< Vertex >& theVerts);
		
		bool SharesEdge(Triangle* triA, 
						Triangle* triB,
						std::vector< Vertex >& theVerts);
		
		bool SharesEdgeRespectSplits(	Triangle* triA, 
										Triangle* triB,
						std::vector< Vertex >& theVerts);

		//calculates the tangent and binormal per face
		void GetGradients( const MeshMender::Vertex& v0,
                           const MeshMender::Vertex& v1,
                           const MeshMender::Vertex& v2,
                           D3DXVECTOR3& tangent,
                           D3DXVECTOR3& binormal) const;

		void OrthogonalizeTangentsAndBinormals( 
						std::vector< Vertex >&   theVerts );

		void UpdateTheIndicesWithFinalIndices(std::vector< unsigned int >& theIndices );

		void FixCylindricalWrapping(	std::vector< Vertex >& theVerts , 
										std::vector< unsigned int >& theIndices,
										std::vector< unsigned int >& mappingNewToOldVert);

		bool TriHasEdge(const D3DXVECTOR3& p0,
						const D3DXVECTOR3& p1,
						const D3DXVECTOR3& triA,
						const D3DXVECTOR3& triB,
						const D3DXVECTOR3& triC);

		bool TriHasEdge(const size_t& p0,
						const size_t& p1,
						const size_t& triA,
						const size_t& triB,
						const size_t& triC);

		void ProcessNormals(TriangleList& possibleNeighbors,
							std::vector< Vertex >&    theVerts,
							std::vector< unsigned int >& mappingNewToOldVert,
							D3DXVECTOR3 workingPosition);
		
		void ProcessTangents(TriangleList& possibleNeighbors,
								std::vector< Vertex >&    theVerts,
								std::vector< unsigned int >& mappingNewToOldVert,
								D3DXVECTOR3 workingPosition);
		
		void ProcessBinormals(TriangleList& possibleNeighbors,
								std::vector< Vertex >&    theVerts,
								std::vector< unsigned int >& mappingNewToOldVert,
								D3DXVECTOR3 workingPosition);

		
		//make any triangle that used the oldIndex use the newIndex instead
		void UpdateIndices(	const size_t oldIndex , 
							const size_t newIndex , 
							TriangleList& curGroup);



		//adds a new mapping entry,
		//takes into account that we may be mapping a new vertex to another new vertex,
		//and uses the original old vertex index....is that confusing?
		void AppendToMapping(	const size_t oldIndex,
								const size_t originalNumVerts,
								std::vector< unsigned int >& mappingNewToOldVert);

};



#endif
