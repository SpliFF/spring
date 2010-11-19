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
		, model(NULL)
	{}
	CWorldObject(const float3& pos)
		: id(0)
		, pos(pos)
		, radius(0)
		, sqRadius(0)
		, drawRadius(0)
		, useAirLos(false)
		, alwaysVisible(false)
		, model(NULL)
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

/** Synced joints

Synced joints define parts of a simulated skeleton for linkages in world objects, typically unit models. They contain transform data and hierarchy.
**/

class CWorldPiece:
{
public:
	CR_DECLARE(CWorldPiece);

	CWorldObject()
		: id(0)
		, mat(ZeroMatrix)
		, radius(0)
		, sqRadius(0)
		, drawRadius(0)
		, useAirLos(false)
		, alwaysVisible(false)
		, model(NULL)
	{}

    int model_id;                                   //! ID of model that should be rendered on this joint
    WorldObject* object;                            //! World object this piece is attached to
	CMatrix44f mat;                                 //! Current transformation relative to parent
	CSimJoint* parent;                              //! Joint this joint is attached to. Can be NULL
	std::vector<CSimJoint*> childs;                 //! Joints attached to this joint

	// Transform helpers
	CMatrix44f GetMatrix() const;                   //! Get/Set transformation matrix
	CMatrix44f GetAbsMatrix() const;
	void SetMatrix( CMatrix44f mat ) const;
	void GetTransform( float3& pos, float3& rot, float3& sca ) const;   //! Get/Set transformation as discrete vectors
	void GetAbsTransform( float3& pos, float3& rot, float3& sca ) const;
	void SetTransform( float3& pos, float3& rot, float3& sca ) const;
	float3 GetPosition() const;                     //! Returns only position data as 3 floats
	float3 GetAbsPosition(float3& pos) const;
	float3 GetRotation() const;                     //! Returns only rotation data as 3 floats
	float3 GetAbsRotation() const;
	float3 GetScale() const;                        //! Returns only scale data as 3 floats
	float3 GetAbsScale() const;
	void SetPosition() const;                       //! Reset relative position
	void SetRotation() const;                       //! Reset relative rotation
	void SetScale() const;                          //! Reset relative scale
	float3 AddPosition() const;                     //! Accumulate relative offset and return it
	float3 AddRotation() const;                     //! Accumulate relative internal rotation and return it
	float3 AddScale() const;                        //! Accumulate relative internal scale and return it
	void SetAxisRotation( int axis, float radians ) const;      //! Reset rotation around the specified axis in radians
	float GetAxisRotation( int axis, float radians ) const;     //! Returns rotation around the specified axis in radians
	float AddAxisRotation( int axis, float radians ) const;     //! Accumulate rotation around the specified axis in radians and return it
	float3 GetDirection() const;                    //! Gets the "direction" the object is pointing. This is the vector along the X axis.
	float3 GetAbsDirection() const;
}

#endif /* WORLD_OBJECT_H */
