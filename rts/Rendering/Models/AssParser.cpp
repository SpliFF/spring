// AssParser.cpp: implementation of the CAssParser class. Reads 3D formats supported by Assimp lib.
//
//////////////////////////////////////////////////////////////////////
//#include "StdAfx.h"

#include "Util.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "System/Exceptions.h"
#include "Sim/Misc/CollisionVolume.h"
#include "FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include "3DModel.h"
#include "3DModelLog.h"
#include "S3OParser.h"
#include "AssIO.h"
#include "AssParser.h"

#include "assimp.hpp"
#include "aiDefines.h"
#include "aiTypes.h"
#include "aiScene.h"
#include "aiPostProcess.h"
#include "DefaultLogger.h"
#include "Rendering/Textures/S3OTextureHandler.h"

#define IS_QNAN(f) (f != f)
#define DEGTORAD 0.0174532925
//static const float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
//static const float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

// triangulate guarantees the most complex mesh is a triangle
// sortbytype ensure only 1 type of primitive type per mesh is used
#define ASS_POSTPROCESS_OPTIONS \
	aiProcess_FindInvalidData				| \
	aiProcess_CalcTangentSpace				| \
	aiProcess_GenSmoothNormals				| \
	aiProcess_SplitLargeMeshes				| \
	aiProcess_Triangulate					| \
	aiProcess_GenUVCoords             		| \
	aiProcess_SortByPType					| \
	aiProcess_JoinIdenticalVertices

//aiProcess_ImproveCacheLocality


// Convert Assimp quaternion to radians around x, y and z
float3 QuaternionToRadianAngles( aiQuaternion q1 )
{
    float sqw = q1.w*q1.w;
    float sqx = q1.x*q1.x;
    float sqy = q1.y*q1.y;
    float sqz = q1.z*q1.z;
	float unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
	float test = q1.x*q1.y + q1.z*q1.w;
	float3 result(0.0f, 0.0f, 0.0f);

	if (test > 0.499f * unit) { // singularity at north pole
		result.x = 2 * atan2(q1.x,q1.w);
		result.y = PI/2;
	} else if (test < -0.499f * unit) { // singularity at south pole
		result.x = -2 * atan2(q1.x,q1.w);
		result.y = -PI/2;
	} else {
        result.x = atan2(2*q1.y*q1.w-2*q1.x*q1.z , sqx - sqy - sqz + sqw);
        result.y = asin(2*test/unit);
        result.z = atan2(2*q1.x*q1.w-2*q1.y*q1.z , -sqx + sqy - sqz + sqw);
	}
	return result;
}

// Convert float3 rotations in degrees to radians (in-place)
void DegreesToRadianAngles( float3& angles )
{
    angles.x *= DEGTORAD;
    angles.y *= DEGTORAD;
    angles.z *= DEGTORAD;
}

class AssLogStream : public Assimp::LogStream
{
public:
	AssLogStream() {}
	~AssLogStream() {}
	void write(const char* message)
	{
		logOutput.Print (LOG_MODEL_DETAIL, "Assimp: %s", message);
	}
};



S3DModel* CAssParser::Load(const std::string& modelFileName)
{
	logOutput.Print (LOG_MODEL, "Loading model: %s\n", modelFileName.c_str() );


    // LOAD METADATA
	// Load the lua metafile. This contains properties unique to Spring models and must return a table
	std::string metaFileName = modelFileName + ".lua";
	CFileHandler* metaFile = new CFileHandler(metaFileName);
	if (!metaFile->FileExists()) {
	    // Try again without the model file extension
        metaFileName = modelFileName.substr(0, modelFileName.find_last_of('.')) + ".lua";
        metaFile = new CFileHandler(metaFileName);
	}
	LuaParser metaFileParser(metaFileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!metaFileParser.Execute()) {
		if (!metaFile->FileExists()) {
			logOutput.Print(LOG_MODEL, "No meta-file '%s'. Using defaults.", metaFileName.c_str());
		} else {
			logOutput.Print(LOG_MODEL, "ERROR in '%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());
		}
	}
	// Get the (root-level) model table
	const LuaTable& metaTable = metaFileParser.GetRoot();
    if (metaTable.IsValid()) logOutput.Print(LOG_MODEL, "Found valid model metadata in '%s'", metaFileName.c_str());


    // LOAD MODEL DATA
	// Create a model importer instance
 	Assimp::Importer importer;

	// Give the importer an IO class that handles Spring's VFS
	importer.SetIOHandler( new AssVFSSystem() );

	// Create a logger for debugging model loading issues
	Assimp::DefaultLogger::create("",Assimp::Logger::VERBOSE);
	const unsigned int severity = Assimp::Logger::DEBUGGING|Assimp::Logger::INFO|Assimp::Logger::ERR|Assimp::Logger::WARN;
	Assimp::DefaultLogger::get()->attachStream( new AssLogStream(), severity );

	// Read the model file to build a scene object
	logOutput.Print(LOG_MODEL, "Importing model file: %s\n", modelFileName.c_str() );
	const aiScene* scene = importer.ReadFile( modelFileName, ASS_POSTPROCESS_OPTIONS );
	if (scene != NULL) {
		logOutput.Print(LOG_MODEL, "Processing scene for model: %s (%d meshes / %d materials / %d textures)", modelFileName.c_str(), scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures );
    } else {
		logOutput.Print (LOG_MODEL, "Model Import Error: %s\n",  importer.GetErrorString());
	}

    S3DModel* model = new S3DModel;
    model->name = modelFileName;
	model->type = MODELTYPE_ASS;
	model->numobjects = 0;
    model->scene = scene;
    //model->meta = &metaTable;

    // Simplified dimensions used for rough calculations
    model->relMidPos = metaTable.GetFloat3("midpos", float3(0.0f,0.0f,0.0f));
    model->radius = metaTable.GetFloat("radius", 1.0f);
    model->height = metaTable.GetFloat("height", 1.0f);
    model->mins = metaTable.GetFloat3("mins", float3(0.0f,0.0f,0.0f));
    model->maxs = metaTable.GetFloat3("maxs", float3(1.0f,1.0f,1.0f));

    // Assign textures
    // The S3O texture handler uses two textures.
    // The first contains diffuse color (RGB) and teamcolor (A)
    // The second contains glow (R), reflectivity (G) and 1-bit Alpha (A).
    model->tex1 = metaTable.GetString("tex1", "default.png");
    model->tex2 = metaTable.GetString("tex2", "");
    model->flipTexY = metaTable.GetBool("fliptextures", true); // Flip texture upside down
    model->invertAlpha = metaTable.GetBool("invertteamcolor", true); // Reverse teamcolor levels

    // Load textures
    logOutput.Print(LOG_MODEL, "Loading textures. Tex1: '%s' Tex2: '%s'", model->tex1.c_str(), model->tex2.c_str());
    texturehandlerS3O->LoadS3OTexture(model);

    // Load all pieces in the model
    logOutput.Print(LOG_MODEL, "Loading pieces from root node '%s'", scene->mRootNode->mName.data);
    LoadPiece( model, scene->mRootNode, metaTable );

    // Update piece hierarchy based on metadata
    BuildPieceHierarchy( model );

    // Verbose logging of model properties
    logOutput.Print(LOG_MODEL_DETAIL, "model->name: %s", model->name.c_str());
    logOutput.Print(LOG_MODEL_DETAIL, "model->numobjects: %d", model->numobjects);
    logOutput.Print(LOG_MODEL_DETAIL, "model->radius: %f", model->radius);
    logOutput.Print(LOG_MODEL_DETAIL, "model->height: %f", model->height);
    logOutput.Print(LOG_MODEL_DETAIL, "model->mins: (%f,%f,%f)", model->mins[0], model->mins[1], model->mins[2]);
    logOutput.Print(LOG_MODEL_DETAIL, "model->maxs: (%f,%f,%f)", model->maxs[0], model->maxs[1], model->maxs[2]);

    logOutput.Print (LOG_MODEL, "Model %s Imported.", model->name.c_str());
    return model;
}

SAssPiece* CAssParser::LoadPiece(S3DModel* model, aiNode* node, const LuaTable& metaTable)
{
	logOutput.Print(LOG_PIECE, "Converting node '%s' to a piece (%d meshes).", node->mName.data, node->mNumMeshes);

	// Create new piece
	model->numobjects++;
	SAssPiece* piece = new SAssPiece;
	piece->type = MODELTYPE_ASS;
	piece->node = node;
	piece->isEmpty = node->mNumMeshes == 0;

    if (node->mParent) {
        piece->name = std::string(node->mName.data);
	} else {
        piece->name = "root"; // The real model root
	}
    // Load additional piece properties from metadata
    const LuaTable& pieceTable = metaTable.SubTable("pieces").SubTable(piece->name);
    if (pieceTable.IsValid()) logOutput.Print(LOG_PIECE, "Found metadata for piece '%s'", piece->name.c_str());

    // Get piece transform data
    float3 rotate, scale, offset;
	aiVector3D _scale, _offset;
 	aiQuaternion _rotate;
	node->mTransformation.Decompose(_scale,_rotate,_offset);

    offset = float3(_offset.x, _offset.y, _offset.z);
	if (pieceTable.KeyExists("offset")) offset = pieceTable.GetFloat3("offset", ZeroVector);
    if (pieceTable.KeyExists("offsetx")) offset.x = pieceTable.GetFloat("offsetx", 0.0f);
    if (pieceTable.KeyExists("offsety")) offset.y = pieceTable.GetFloat("offsety", 0.0f);
    if (pieceTable.KeyExists("offsetz")) offset.z = pieceTable.GetFloat("offsetz", 0.0f);

    if (pieceTable.KeyExists("rotate")) {
        rotate = pieceTable.GetFloat3("rotate", ZeroVector);
        DegreesToRadianAngles(rotate);
    } else {
        rotate = QuaternionToRadianAngles(_rotate);
    }
    if (pieceTable.KeyExists("rotatex")) rotate.x = pieceTable.GetFloat("rotatex", 0.0f) * DEGTORAD;
    if (pieceTable.KeyExists("rotatey")) rotate.y = pieceTable.GetFloat("rotatey", 0.0f) * DEGTORAD;
    if (pieceTable.KeyExists("rotatez")) rotate.z = pieceTable.GetFloat("rotatez", 0.0f) * DEGTORAD;

    scale = float3(_scale.x, _scale.y, _scale.z);
	if (pieceTable.KeyExists("scale")) scale = pieceTable.GetFloat3("scale", float3(1.0f,1.0f,1.0f));
    if (pieceTable.KeyExists("scalex")) scale.x = pieceTable.GetFloat("scalex", 1.0f);
    if (pieceTable.KeyExists("scaley")) scale.y = pieceTable.GetFloat("scaley", 1.0f);
    if (pieceTable.KeyExists("scalez")) scale.z = pieceTable.GetFloat("scalez", 1.0f);

	logOutput.Print(LOG_PIECE, "(%d:%s) Relative offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f,%f,%f)", model->numobjects, node->mName.data,
		offset.x, offset.y, offset.z,
		rotate.x, rotate.y, rotate.z,
		scale.x, scale.y, scale.z
	);
	piece->pos = offset;
	piece->rot = rotate;
	piece->scale = scale;

	piece->mins = float3(0.0f,0.0f,0.0f);
	piece->maxs = float3(1.0f,1.0f,1.0f);

	// Get vertex data from node meshes
	for ( unsigned meshListIndex = 0; meshListIndex < node->mNumMeshes; meshListIndex++ ) {
		unsigned int meshIndex = node->mMeshes[meshListIndex];
		logOutput.Print(LOG_PIECE_DETAIL, "Fetching mesh %d from scene", meshIndex );
		aiMesh* mesh = model->scene->mMeshes[meshIndex];
		std::vector<unsigned> mesh_vertex_mapping;
		// extract vertex data
		logOutput.Print(LOG_PIECE_DETAIL, "Processing vertices for mesh %d (%d vertices)", meshIndex, mesh->mNumVertices );
		logOutput.Print(LOG_PIECE_DETAIL, "Normals: %s Tangents/Bitangents: %s TexCoords: %s",
				(mesh->HasNormals())?"Y":"N",
				(mesh->HasTangentsAndBitangents())?"Y":"N",
				(mesh->HasTextureCoords(0)?"Y":"N")
		);
		for ( unsigned vertexIndex= 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
			SAssVertex vertex;

			// vertex coordinates
			logOutput.Print(LOG_PIECE_DETAIL, "Fetching vertex %d from mesh", vertexIndex );
			aiVector3D& aiVertex = mesh->mVertices[vertexIndex];
			vertex.pos.x = aiVertex.x;
			vertex.pos.y = aiVertex.y;
			vertex.pos.z = aiVertex.z;

			// update piece min/max extents
			piece->mins.x = std::min(piece->mins.x, aiVertex.x);
			piece->mins.y = std::min(piece->mins.y, aiVertex.y);
			piece->mins.z = std::min(piece->mins.z, aiVertex.z);
			piece->maxs.x = std::max(piece->maxs.x, aiVertex.x);
			piece->maxs.y = std::max(piece->maxs.y, aiVertex.y);
			piece->maxs.z = std::max(piece->maxs.z, aiVertex.z);

			logOutput.Print(LOG_PIECE_DETAIL, "vertex position %d: %f %f %f", vertexIndex, vertex.pos.x, vertex.pos.y, vertex.pos.z );

			// vertex normal
			logOutput.Print(LOG_PIECE_DETAIL, "Fetching normal for vertex %d", vertexIndex );
			aiVector3D& aiNormal = mesh->mNormals[vertexIndex];
			vertex.hasNormal = !IS_QNAN(aiNormal);
			if ( vertex.hasNormal ) {
				vertex.normal.x = aiNormal.x;
				vertex.normal.y = aiNormal.y;
				vertex.normal.z = aiNormal.z;
				logOutput.Print(LOG_PIECE_DETAIL, "vertex normal %d: %f %f %f",vertexIndex, vertex.normal.x, vertex.normal.y,vertex.normal.z );
			}

			// vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				logOutput.Print(LOG_PIECE_DETAIL, "Fetching tangent for vertex %d", vertexIndex );
				aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				vertex.hasTangent = !IS_QNAN(aiTangent);
				if ( vertex.hasTangent ) {
					float3 tangent;
					tangent.x = aiTangent.x;
					tangent.y = aiTangent.y;
					tangent.z = aiTangent.z;
					logOutput.Print(LOG_PIECE_DETAIL, "vertex tangent %d: %f %f %f",vertexIndex, tangent.x, tangent.y,tangent.z );
					piece->sTangents.push_back(tangent);
					// bitangent is cross product of tangent and normal
					float3 bitangent;
					if ( vertex.hasNormal ) {
						bitangent = tangent.cross(vertex.normal);
						logOutput.Print(LOG_PIECE_DETAIL, "vertex bitangent %d: %f %f %f",vertexIndex, bitangent.x, bitangent.y,bitangent.z );
						piece->tTangents.push_back(bitangent);
					}
				}
			} else {
				vertex.hasTangent = false;
			}

			// vertex texcoords
			if (mesh->HasTextureCoords(0)) {
				vertex.textureX = mesh->mTextureCoords[0][vertexIndex].x;
				vertex.textureY = mesh->mTextureCoords[0][vertexIndex].y;
				logOutput.Print(LOG_PIECE_DETAIL, "vertex texcoords %d: %f %f", vertexIndex, vertex.textureX, vertex.textureY );
			}

			mesh_vertex_mapping.push_back(piece->vertices.size());
			piece->vertices.push_back(vertex);
		}

		// extract face data
		if ( mesh->HasFaces() ) {
			logOutput.Print(LOG_PIECE_DETAIL, "Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);
			for ( unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++ ) {
                aiFace& face = mesh->mFaces[faceIndex];
				// get the vertex belonging to the mesh
				for ( unsigned vertexListID = 0; vertexListID < face.mNumIndices; vertexListID++ ) {
					unsigned int vertexID = mesh_vertex_mapping[face.mIndices[vertexListID]];
					logOutput.Print(LOG_PIECE_DETAIL, "face %d vertex %d", faceIndex, vertexID );
					piece->vertexDrawOrder.push_back(vertexID);
				}
			}
		}
	}

	// update model min/max extents
	model->mins.x = std::min(piece->mins.x, model->mins.x);
	model->mins.y = std::min(piece->mins.y, model->mins.y);
	model->mins.z = std::min(piece->mins.z, model->mins.z);
	model->maxs.x = std::max(piece->maxs.x, model->maxs.x);
	model->maxs.y = std::max(piece->maxs.y, model->maxs.y);
	model->maxs.z = std::max(piece->maxs.z, model->maxs.z);

	// collision volume for piece (not sure about these coords)
	const float3 cvScales = (piece->maxs) - (piece->mins);
	const float3 cvOffset = (piece->maxs - piece->pos) + (piece->mins - piece->pos);
	//const float3 cvOffset(piece->offset.x, piece->offset.y, piece->offset.z);
	piece->colvol = new CollisionVolume("box", cvScales, cvOffset, CollisionVolume::COLVOL_HITTEST_CONT);

    // Get parent name from metadata or model
    if (pieceTable.KeyExists("parent")) {
        piece->parentName = pieceTable.GetString("parent", "");
    } else if (node->mParent) {
        if (node->mParent->mParent) {
            piece->parentName = std::string(node->mParent->mName.data);
        } else { // my parent is the root, which gets renamed
            piece->parentName = "root";
        }
    } else {
        piece->parentName = "";
    }

	// Recursively process all child pieces
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		LoadPiece(model, node->mChildren[i], metaTable);
	}

	logOutput.Print (LOG_PIECE_DETAIL, "Loaded model piece: %s with %d meshes\n", piece->name.c_str(), node->mNumMeshes );
	model->pieces[piece->name] = piece;
	return piece;
}


//Because of metadata overrides we don't know the true hierarchy until all pieces have been loaded
void CAssParser::BuildPieceHierarchy( S3DModel* model )
{
    // Loop through all pieces and create missing hierarchy info
    PieceMap::const_iterator end = model->pieces.end();
    for (PieceMap::const_iterator it = model->pieces.begin(); it != end; ++it)
    {
        S3DModelPiece* piece = it->second;
        if (piece->name == "root") {
            piece->parent = NULL;
            model->rootobject = piece;
        } else if (piece->parentName != "") {
            piece->parent = model->FindPiece(piece->parentName);
            if (piece->parent == NULL) {
                logOutput.Print( LOG_PIECE, "Error: Missing piece '%s' declared as parent of '%s'.\n", piece->parentName.c_str(), piece->name.c_str() );
            } else {
                piece->parent->childs.push_back(piece);
            }
        } else {
            // A piece with no parent that isn't the root (orphan)
            piece->parent = model->FindPiece("root");
            if (piece->parent == NULL) {
                logOutput.Print( LOG_PIECE, "Error: Missing root piece.\n" );
            } else {
                piece->parent->childs.push_back(piece);
            }
        }
    }
}

/*
    // Loop over the pieces defined in the metadata
    const LuaTable& pieceTable = model->meta.SubTable("pieces");
	std::vector<std::string> metaPieceNames;
	piecesTable.GetKeys(metaPieceNames);
	if (!metaPieceNames.empty()) {

	}
*/

void DrawPiecePrimitive( const S3DModelPiece* o)
{
	if (o->isEmpty) {
		return;
	}
	logOutput.Print(LOG_PIECE_DETAIL, "Compiling piece %s", o->name.c_str());
	// Add GL commands to the pieces displaylist

	const SAssPiece* so = static_cast<const SAssPiece*>(o);
	const SAssVertex* sAssV = static_cast<const SAssVertex*>(&so->vertices[0]);


	// pass the tangents as 3D texture coordinates
	// (array elements are float3's, which are 12
	// bytes in size and each represent a single
	// xyz triple)
	// TODO: test if we have this many texunits
	// (if not, could only send the s-tangents)?

	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->sTangents[0].x);
	}
	if (!so->tTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->tTangents[0].x);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SAssVertex), &sAssV->textureX);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(SAssVertex), &sAssV->pos.x);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(SAssVertex), &sAssV->normal.x);

	// since aiProcess_SortByPType is being used, we're sure we'll get only 1 type here, so combination check isn't needed, also anything more complex than triangles is being split thanks to aiProcess_Triangulate
	glDrawElements(GL_TRIANGLES, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);

	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (!so->tTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	logOutput.Print(LOG_PIECE_DETAIL, "Completed compiling piece %s", o->name.c_str());
}

void CAssParser::Draw( const S3DModelPiece* o) const
{
	DrawPiecePrimitive( o );
}

void SAssPiece::DrawList() const
{
	DrawPiecePrimitive(this);
}
