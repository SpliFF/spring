/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Defines a framework which can be used for modelling Sim or UI purposes */

#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <vector>
#include <string>
#include <set>
#include "assimp.hpp"

const int
	MODELTYPE_3DO   = 0,
	MODELTYPE_S3O   = 1,
	MODELTYPE_OBJ   = 2,
	MODELTYPE_ASS	= 3, // Model loaded by Assimp library
	MODELTYPE_OTHER	= 4; // For future use. Still used in some parts of code.

const int
	MODELAXIS_X = 0,
	MODELAXIS_Y = 1,
	MODELAXIS_Z = 2;

class CModel;

/** Unsynced mesh base class

This class defines a mesh which may be used by one or more models. Each mesh instance can have its own textures and shaders
For efficiency the class inherits directly from aiMesh. This avoids unessesary data copying. S30/3DO loaders can create these structs easily enough.
**/

class CMesh: public aiMesh
{
public:
    int id;                                 //! Mesh ID
    CModel* model;                          //! Points back to user of this mesh instance. Can be NULL
	std::vector<CTexture*> textures;        //! Textures assigned to this mesh
	std::vector<CShader*> shaders;          //! Shaders assigned to this mesh
}

/** Unsynced piece base class

This class combines all rendering related properties from the old S3D(Local)Model and S3D(Local)ModelPiece structs into a universal generic base class
Instances of this class can be used for rendering the same model in different parts of the screen/world and for creating new hierarchies
All *Abs* functions account for accumulated transforms in ancestor models (the models parent, and its parents right up to the root)
All *Iter* functions repeat for all descendents of a node
A model contains metadata which can be set in a metadata file and accessed from unsynced scripts
If you want to change class members directly rather than use the helper functions you can as long as you remember to set the changed flag when you are done.
The Draw* functions build render queues with per-mesh texture lookup, allowing multiple textures per model.
The On* functions are virtuals which could be used to synchronise animation events with unit or UI scripts
A Lua wrapper is provided to manipulate models from scripts
**/


class CModelPiece: public aiNode
{
private:
	int id;                                         //! ID, starting with 1. Currently used for FarTexture creation
	int type;                                       //! MODELTYPE_*
	std::string name;                               //! Used to identify this model

public:
    bool changed;                                   //! Set when GL buffers need to be updated
    bool hidden;                                    //! Set if model shouldn't be rendered
    CMatrix44f mat;                                 //! Current transformation relative to parent

	// Associated objects
    LuaTable& meta;                                 //! Lua metadata. Defines special/custom model properties in Lua format
	std::vector<CMesh*> meshes;                     //! Meshes used by this model

    // Rendering
    void Setup() const;                             //! Create OpenGL buffers/arrays
    void Update() const;                            //! Update OpenGL buffers/arrays

    void Show() const;                              //! Change visibility and activate Lua callbacks
    void Hide() const;
    void Toggle() const;

	void DrawUI() const;                            //! Render buffers using current transform in UI coordinate space
	void DrawUI( CMatrix44f mat ) const;            //! Render buffers with specific transform in UI coordinate space
	void DrawUI( float3 pos, float3 rot, float3 sca ) const;
	void DrawUI( float3 pos ) const;
	void DrawUI( float x, float y ) const;          //! Render buffers with 2D offset in UI/screen (X,Y) coordinate space
	void DrawWorld() const;                         //! Render buffers using current transform in world/map space
	void DrawWorld( CMatrix44f mat ) const;         //! Render buffers with specific transform in world/map space
	void DrawWorld( float3 pos, float3 rot, float3 sca ) const;
	void DrawWorld( float3 pos ) const;
	void DrawWorld( float x, float z ) const;       //! Render buffers with 2D offset in world/map space. Y is set to ground height

	// Callback hooks to be implemented by higher-level classes
	virtual OnHide() const;
	virtual OnShow() const;
    virtual OnUpdate() const;
}

/** Unsynced animated mesh/piece classes

These classes implement more complex pieces using imported Assimp data structures. These pieces can be animated in unsynced code.
Instances of this class can be used for rendering the same model in different parts of the screen/world and for creating new hierarchies
A piece corresponds to one Assimp "aiNode", a mesh to an "aiMesh". A model file will generally contain multiple nodes
Pieces can be assigned an animation channel and animations can be controlled from script. Animation tracks are read-only and unsynced
Pieces reference the aiScene and aiNode Assimp objects that created them. These objects are used to access additional read-only properties
The On* functions are virtuals which could be used to synchronise animation events with unit or UI scripts
A Lua wrapper is provided to manipulate models from scripts
**/

class CAnim: public aiAnimation
{

	aiAnim* anim;                                   //! Assimp animation
	int frame;                                      //! Current animation frame

	const aiScene* scene;                           //! Assimp scene. Contains data shared by nodes. Read-only
	const aiNode* node;                             //! Assimp node. Includes mesh, bone and animation references. Read-only

	// Animation helpers
	GetAnimName() const;                            //! Get current animation name
	GetAnimChannelNum() const;                      //! Get/Set current animation channel/track
	SetAnimChannelNum( int n ) const;
	GetAnimFrameNum() const;                        //! Get/Set current animation channel frame
	SetAnimFrameNum( int n ) const;
	SkipFrames( int n, bool wrap = true ) const;    //! Skip (or backtrack) n frames of animation. Can bet set to wrap or stop at start/end

	// Animation callback hooks
    virtual OnAnimationStart() const;
	virtual OnAnimationStep() const;
	virtual OnAnimationEnd() const;
}


#endif /* _3DMODEL_H */
