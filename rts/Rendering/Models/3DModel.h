/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <vector>
#include <string>
#include <set>
#include <map>

#include "System/Matrix44f.h"


const int
	MODELTYPE_3DO   = 0,
	MODELTYPE_S3O   = 1,
	MODELTYPE_ASS	= 2, // Model loaded by Assimp library
	MODELTYPE_OTHER	= 3; // For future use. Still used in some parts of code.

struct CollisionVolume;
struct S3DModel;
struct S3DModelPiece;
struct LocalModel;
struct LocalModelPiece;
struct aiScene;

typedef std::map<std::string, S3DModelPiece*> PieceMap;

struct S3DModelPiece {
	S3DModelPiece(): type(-1) {
		parent = NULL;
		colvol = NULL;

		isEmpty = true;
		dispListID = 0;
	}

	virtual ~S3DModelPiece();
	virtual void DrawList() const = 0;
	virtual int GetVertexCount() const { return 0; }
	virtual int GetNormalCount() const { return 0; }
	virtual int GetTxCoorCount() const { return 0; }
	virtual void SetMinMaxExtends() {}
	virtual void SetVertexTangents() {}
	virtual const float3& GetVertexPos(int) const = 0;
	virtual void Shatter(float, int, int, const float3&, const float3&) const {}
	void DrawStatic() const;

    void SetCollisionVolume(CollisionVolume* cv) { colvol = cv; }
    const CollisionVolume* GetCollisionVolume() const { return colvol; }
    CollisionVolume* GetCollisionVolume() { return colvol; }

    unsigned int GetChildCount() const { return childs.size(); }
    S3DModelPiece* GetChild(unsigned int i) { return childs[i]; }

	std::string name;
	int type;               //! MODELTYPE_*
    S3DModel* model;
    std::string parentName;
	S3DModelPiece* parent;
    std::vector<S3DModelPiece*> childs;
	CollisionVolume* colvol;

	bool isEmpty;
	unsigned int dispListID;

	float3 mins;
	float3 maxs;
	float3 goffset;     //! offset from model root
	float3 offset;		//! offset from piece parent
	float3 rot;			//! relative rotation in radians
	float3 scale;		//! not used yet

};


struct S3DModel
{
	S3DModel(): id(-1), type(-1), textureType(-1) {
		numPieces = 0;

		radius = 0.0f;
		height = 0.0f;

		rootPiece = NULL;
	}
	~S3DModel();

	S3DModelPiece* GetRootPiece() { return rootPiece; }
	void SetRootPiece(S3DModelPiece* p) { rootPiece = p; }
	void DrawStatic() const { rootPiece->DrawStatic(); }
    S3DModelPiece* FindPiece( std::string name );

    std::string name;
	int id;                 //! unsynced ID, starting with 1
	int type;               //! MODELTYPE_*

	// TODO: Move next 5 fields into S3DModelPiece for per-piece texturing
	int textureType;        //! FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or Assimp)
	int flipTexY;			//! Turn both textures upside down before use
	int invertAlpha;		//! Invert teamcolor alpha channel in S3O texture 1
	std::string tex1;
	std::string tex2;

	float radius;
	float height;

	float3 mins;
	float3 maxs;
	float3 relMidPos;

	int numPieces;
	S3DModelPiece* rootPiece;  //! The piece at the base of the model hierarchy
	PieceMap pieces;   			//! Lookup table for pieces by name
	const aiScene* scene; 		//! For Assimp models. Contains imported data. NULL for s3o/3do.
};



struct LocalModelPiece
{
	LocalModelPiece() {
		parent     = NULL;
		colvol     = NULL;
		original   = NULL;
		dispListID = 0;
		visible    = false;
	}
	void Init(S3DModelPiece* piece) {
		original   =  piece;
		dispListID =  piece->dispListID;
		visible    = !piece->isEmpty;
		pos        =  piece->offset;

		childs.reserve(piece->childs.size());
	}

	void AddChild(LocalModelPiece* c) { childs.push_back(c); }
	void SetParent(LocalModelPiece* p) { parent = p; }

	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);
	void ApplyTransform() const;
	void GetPiecePosIter(CMatrix44f* mat) const;
	float3 GetPos() const;
	float3 GetDirection() const;
	bool GetEmitDirPos(float3& pos, float3& dir) const;
	CMatrix44f GetMatrix() const;

	void SetCollisionVolume(CollisionVolume* cv) { colvol = cv; }
	const CollisionVolume* GetCollisionVolume() const { return colvol; }

	float3 pos;
	float3 rot; 	//! in radian
	float3 scale;	//! not used yet

	// TODO: add (visibility) maxradius!
	bool visible;

	CollisionVolume* colvol;
	S3DModelPiece* original;

	LocalModelPiece* parent;
	std::vector<LocalModelPiece*> childs;

	unsigned int dispListID;
	std::vector<unsigned int> lodDispLists;
};

struct LocalModel
{
	LocalModel(const S3DModel* model) : type(-1), lodCount(0) {
		type = model->type;
		pieces.reserve(model->numPieces);

		assert(model->numPieces >= 1);

		for (unsigned int i = 0; i < model->numPieces; i++) {
			pieces.push_back(new LocalModelPiece());
		}
	}

	~LocalModel();
	void CreatePieces(S3DModelPiece*, unsigned int*);

	LocalModelPiece* GetPiece(unsigned int i) { return pieces[i]; }
	LocalModelPiece* GetRoot() { return GetPiece(0); }

	void Draw() const { pieces[0]->Draw(); }
	void DrawLOD(unsigned int lod) const { if (lod <= lodCount) pieces[0]->DrawLOD(lod); }
	void SetLODCount(unsigned int count);

	//! raw forms, the piecenum must be valid
	void ApplyRawPieceTransform(int piecenum) const;
	float3 GetRawPiecePos(int piecenum) const;
	CMatrix44f GetRawPieceMatrix(int piecenum) const;
	float3 GetRawPieceDirection(int piecenum) const;
	void GetRawEmitDirPos(int piecenum, float3& pos, float3& dir) const;


	int type;  //! MODELTYPE_*
	unsigned int lodCount;

	std::vector<LocalModelPiece*> pieces;
};


#endif /* _3DMODEL_H */
