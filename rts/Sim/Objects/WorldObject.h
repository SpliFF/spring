/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORLD_OBJECT_H
#define WORLD_OBJECT_H

#include "Object.h"
#include "float3.h"

struct S3DModel;
class CWorldObject: public CObject
{
public:
	CR_DECLARE(CWorldObject);

	CWorldObject()
		: id(0)
		, pos(ZeroVector)
		, radius(0)
		, sqRadius(0)
		, drawRadius(0)
		, useAirLos(false)
		, alwaysVisible(false)
		, rootpiece(NULL)
	{}
	CWorldObject(const float3& pos)
		: id(0)
		, pos(pos)
		, radius(0)
		, sqRadius(0)
		, drawRadius(0)
		, useAirLos(false)
		, alwaysVisible(false)
		, rootpiece(NULL)
	{}

	void SetRadius(float r);
	virtual ~CWorldObject();

	int id;

	float3 pos;
	float radius;       ///< used for collisions
	float sqRadius;

	float drawRadius;   ///< used to see if in los
	bool useAirLos;     ///< if true, object's visibility is checked against airLosMap[allyteam]
	bool alwaysVisible;

	CWorldPiece* rootpiece;  /// First link in the chain of model pieces used for skeleton and rendering
};

#endif /* WORLD_OBJECT_H */
