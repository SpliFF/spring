/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <vector>
#include <string>
#include <set>
#include <map>
#include "Matrix44f.h"


const int
	MODELTYPE_3DO   = 0,
	MODELTYPE_S3O   = 1,
	MODELTYPE_OBJ   = 2,
	MODELTYPE_ASS	= 3, // Model loaded by Assimp library
	MODELTYPE_OTHER	= 4; // For future use. Still used in some parts of code.

struct CollisionVolume;
struct S3DModel;
struct S3DModelPiece;
struct LocalModel;
struct LocalModelPiece;
struct aiScene;
class LuaTable;

typedef std::map<std::string, S3DModelPiece*> PieceMap;

struct S3DModelPiece {
	std::string name;
	int type;               //! MODELTYPE_*
	//LuaTable* meta;
    S3DModel* model;
    std::string parentName;
	S3DModelPiece* parent;
    std::vector<S3DModelPiece*> childs;

	bool isEmpty;
	unsigned int displist;

	// defaults to a box
	CollisionVolume* colvol;

	// float3 dir;    // TODO?
	float3 mins;
	float3 maxs;
	float3 goffset;   // wrt. root

	float3 pos;
	float3 rot; //! in radian
	float3 scale;

    S3DModelPiece();
	~S3DModelPiece();
	virtual void DrawList() const = 0;
	virtual int GetVertexCount() const { return 0; }
	virtual int GetNormalCount() const { return 0; }
	virtual int GetTxCoorCount() const { return 0; }
	virtual void SetMinMaxExtends() {}
	virtual void SetVertexTangents() {}
	virtual const float3& GetVertexPos(int) const = 0;
	virtual void Shatter(float, int, int, const float3&, const float3&) const {}
	void DrawStatic() const;
};


struct S3DModel
{
	int id; //! unsynced ID, starting with 1
    std::string name;
    //LuaTable* meta;
	int type;               //! MODELTYPE_*

	// TODO: Move next 5 fields into S3DModelPiece for per-piece texturing (or remove entirely and put data into textures array)
	int textureType;        //! FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or OBJ)
	int flipTexY;			//! Turn both textures upside down before use
	int invertAlpha;		//! Invert teamcolor alpha channel in S3O texture 1
	std::string tex1;
	std::string tex2;

	int numobjects;
	float radius;
	float height;

	float3 mins;
	float3 maxs;
	float3 relMidPos;

	S3DModelPiece* rootobject;  //! The piece at the base of the model hierarchy
	PieceMap pieces;   //! Lookup table for pieces by name

	const aiScene* scene; //! For Assimp models. Contains imported data. NULL for s3o/3do.

    S3DModel();
    ~S3DModel();
    S3DModelPiece* FindPiece( std::string name );
	inline void DrawStatic() const { rootobject->DrawStatic(); };
};


struct LocalModelPiece
{
	// TODO: add (visibility) maxradius!

	float3 pos;
	float3 rot; //! in radian
	float3 scale;

	bool updated; //FIXME unused?
	bool visible;

	//! MODELTYPE_*
	int type;
	std::string name;
	S3DModelPiece* original;
	LocalModelPiece* parent;
	std::vector<LocalModelPiece*> childs;

	// initially always a clone
	// of the original->colvol
	CollisionVolume* colvol;

	unsigned int displist;
	std::vector<unsigned int> lodDispLists;


	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);
	void ApplyTransform() const;
	void GetPiecePosIter(CMatrix44f* mat) const;
	float3 GetPos() const;
	CMatrix44f GetMatrix() const;
	float3 GetDirection() const;
	bool GetEmitDirPos(float3& pos, float3& dir) const;
};

struct LocalModel
{
	LocalModel() : type(-1), lodCount(0) {};
	~LocalModel();

	int type;  //! MODELTYPE_*

	std::vector<LocalModelPiece*> pieces;
	unsigned int lodCount;

	inline void Draw() const { pieces[0]->Draw(); };
	inline void DrawLOD(unsigned int lod) const { if (lod <= lodCount) pieces[0]->DrawLOD(lod);};
	void SetLODCount(unsigned int count);

	//! raw forms, the piecenum must be valid
	void ApplyRawPieceTransform(int piecenum) const;
	float3 GetRawPiecePos(int piecenum) const;
	CMatrix44f GetRawPieceMatrix(int piecenum) const;
	float3 GetRawPieceDirection(int piecenum) const;
	void GetRawEmitDirPos(int piecenum, float3 &pos, float3 &dir) const;
};


#endif /* _3DMODEL_H */
