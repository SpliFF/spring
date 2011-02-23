/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file aiTypes.h
 *  Basic data types and primitives, such as vectors or colors. 
 */
#ifndef AI_TYPES_H_INC
#define AI_TYPES_H_INC

// Some runtime headers
#include <sys/types.h>
#include <memory.h>
#include <math.h>
#include <stddef.h>

// Our compile configuration
#include "aiDefines.h"

// Some types moved to separate header due to size of operators
#include "aiVector3D.h"
#include "aiVector2D.h"
#include "aiColor4D.h"
#include "aiMatrix3x3.h"
#include "aiMatrix4x4.h"
#include "aiQuaternion.h"

#ifdef __cplusplus
#include <new>		// for std::nothrow_t
#include <string>	// for aiString::Set(const std::string&)

namespace Assimp	{
	//! @cond never
namespace Intern		{
	// --------------------------------------------------------------------
	/** @brief Internal helper class to utilize our internal new/delete 
	 *    routines for allocating object of this and derived classes.
	 *
	 * By doing this you can safely share class objects between Assimp
	 * and the application - it works even over DLL boundaries. A good
	 * example is the #IOSystem where the application allocates its custom
	 * #IOSystem, then calls #Importer::SetIOSystem(). When the Importer
	 * destructs, Assimp calls operator delete on the stored #IOSystem.
	 * If it lies on a different heap than Assimp is working with,
	 * the application is determined to crash.
	 */
	// --------------------------------------------------------------------
#ifndef SWIG
	struct ASSIMP_API AllocateFromAssimpHeap	{
		// http://www.gotw.ca/publications/mill15.htm

		// new/delete overload
		void *operator new    ( size_t num_bytes) /* throw( std::bad_alloc ) */;
		void *operator new    ( size_t num_bytes, const std::nothrow_t& ) throw();
		void  operator delete ( void* data);

		// array new/delete overload
		void *operator new[]    ( size_t num_bytes) /* throw( std::bad_alloc ) */;
		void *operator new[]    ( size_t num_bytes, const std::nothrow_t& )  throw();
		void  operator delete[] ( void* data);

	}; // struct AllocateFromAssimpHeap
#endif
} // namespace Intern
	//! @endcond
} // namespace Assimp

extern "C" {
#endif

/** Maximum dimension for strings, ASSIMP strings are zero terminated. */
#ifdef __cplusplus
const size_t MAXLEN = 1024;
#else
#	define MAXLEN 1024
#endif

#include "./Compiler/pushpack1.h"

// ----------------------------------------------------------------------------------
/** Represents a plane in a three-dimensional, euclidean space
*/
struct aiPlane
{
#ifdef __cplusplus
	aiPlane () : a(0.f), b(0.f), c(0.f), d(0.f) {}
	aiPlane (float _a, float _b, float _c, float _d) 
		: a(_a), b(_b), c(_c), d(_d) {}

	aiPlane (const aiPlane& o) : a(o.a), b(o.b), c(o.c), d(o.d) {}

#endif // !__cplusplus

	//! Plane equation
	float a,b,c,d;
} PACK_STRUCT; // !struct aiPlane

// ----------------------------------------------------------------------------------
/** Represents a ray
*/
struct aiRay
{
#ifdef __cplusplus
	aiRay () {}
	aiRay (const aiVector3D& _pos, const aiVector3D& _dir)
		: pos(_pos), dir(_dir) {}

	aiRay (const aiRay& o) : pos (o.pos), dir (o.dir) {}

#endif // !__cplusplus

	//! Position and direction of the ray
	C_STRUCT aiVector3D pos, dir;
} PACK_STRUCT; // !struct aiRay

// ----------------------------------------------------------------------------------
/** Represents a color in Red-Green-Blue space. 
*/
struct aiColor3D
{
#ifdef __cplusplus
	aiColor3D () : r(0.0f), g(0.0f), b(0.0f) {}
	aiColor3D (float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
	aiColor3D (float _r) : r(_r), g(_r), b(_r) {}
	aiColor3D (const aiColor3D& o) : r(o.r), g(o.g), b(o.b) {}
	
	/** Component-wise comparison */
	// TODO: add epsilon?
	bool operator == (const aiColor3D& other) const
		{return r == other.r && g == other.g && b == other.b;}

	/** Component-wise inverse comparison */
	// TODO: add epsilon?
	bool operator != (const aiColor3D& other) const
		{return r != other.r || g != other.g || b != other.b;}

	/** Component-wise addition */
	aiColor3D operator+(const aiColor3D& c) const {
		return aiColor3D(r+c.r,g+c.g,b+c.b);
	}

	/** Component-wise subtraction */
	aiColor3D operator-(const aiColor3D& c) const {
		return aiColor3D(r+c.r,g+c.g,b+c.b);
	}

	/** Component-wise multiplication */
	aiColor3D operator*(const aiColor3D& c) const {
		return aiColor3D(r*c.r,g*c.g,b*c.b);
	}
	
	/** Multiply with a scalar */
	aiColor3D operator*(float f) const {
		return aiColor3D(r*f,g*f,b*f);
	}

	/** Access a specific color component */
	float operator[](unsigned int i) const {
		return *(&r + i);
	}

	/** Access a specific color component */
	float& operator[](unsigned int i) {
		return *(&r + i);
	}

	/** Check whether a color is black */
	bool IsBlack() const {
		static const float epsilon = 10e-3f;
		return fabs( r ) < epsilon && fabs( g ) < epsilon && fabs( b ) < epsilon;
	}

#endif // !__cplusplus

	//! Red, green and blue color values
	float r, g, b;
} PACK_STRUCT;  // !struct aiColor3D
#include "./Compiler/poppack1.h"

// ----------------------------------------------------------------------------------
/** Represents an UTF-8 string, zero byte terminated.
 *
 *  The character set of an aiString is explicitly defined to be UTF-8. This Unicode
 *  transformation was chosen in the belief that most strings in 3d files are limited
 *  to the ASCII characters, thus the character set needed to be ASCII compatible.
 *  
 *  Most text file loaders provide proper Unicode input file handling, special unicode
 *  characters are correctly transcoded to UTF8 and are kept throughout the libraries'
 *  import pipeline. 
 *
 *  For most applications, it will be absolutely sufficient to interpret the
 *  aiString as ASCII data and work with it as one would work with a plain char*. 
 *  Windows users in need of proper support for i.e asian characters can use the
 *  #MultiByteToWideChar(), #WideCharToMultiByte() WinAPI functionality to convert the
 *  UTF-8 strings to their working character set (i.e. MBCS, WideChar).
 *
 *  We use this representation instead of std::string to be C-compatible. The 
 *  (binary) length of such a string is limited to MAXLEN characters (including the
 *  the terminating zero).
*/
struct aiString
{
#ifdef __cplusplus
	/** Default constructor, the string is set to have zero length */
	aiString() :
		length(0) 
	{
		data[0] = '\0';

#ifdef _DEBUG
		// Debug build: overwrite the string on its full length with ESC (27)
		memset(data+1,27,MAXLEN-1);
#endif
	}

	/** Copy constructor */
	aiString(const aiString& rOther) : 
		length(rOther.length) 
	{
		// Crop the string to the maximum length
		length = length>=MAXLEN?MAXLEN-1:length;
		memcpy( data, rOther.data, length);
		data[length] = '\0';
	}

	/** Constructor from std::string */
	explicit aiString(const std::string& pString) : 
		length(pString.length()) 
	{
		length = length>=MAXLEN?MAXLEN-1:length;
		memcpy( data, pString.c_str(), length);
		data[length] = '\0';
	}

	/** Copy a std::string to the aiString */
	void Set( const std::string& pString) {
		if( pString.length() > MAXLEN - 1) {
			return;
		}
		length = pString.length();
		::memcpy( data, pString.c_str(), length);
		data[length] = 0;
	}

	/** Copy a const char* to the aiString */
	void Set( const char* sz) {
		const size_t len = ::strlen(sz);
		if( len > MAXLEN - 1) {
			return;
		}
		length = len;
		::memcpy( data, sz, len);
		data[len] = 0;
	}

	/** Assign a const char* to the string */
	aiString& operator = (const char* sz) {
		Set(sz);
		return *this;
	}

	/** Assign a cstd::string to the string */
	aiString& operator = ( const std::string& pString) {
		Set(pString);
		return *this;
	}

	/** Comparison operator */
	bool operator==(const aiString& other) const {
		return  (length == other.length && 0 == strcmp(this->data,other.data));
	}

	/** Inverse comparison operator */
	bool operator!=(const aiString& other) const {
		return  (length != other.length || 0 != ::strcmp(this->data,other.data));
	}

	/** Append a string to the string */
	void Append (const char* app)	{
		const size_t len = ::strlen(app);
		if (!len) {
			return;
		}
		if (length + len >= MAXLEN) {
			return;
		}

		memcpy(&data[length],app,len+1);
		length += len;
	}

	/** Clear the string - reset its length to zero */
	void Clear ()	{
		length  = 0;
		data[0] = '\0';

#ifdef _DEBUG
		// Debug build: overwrite the string on its full length with ESC (27)
		memset(data+1,27,MAXLEN-1);
#endif
	}

#endif // !__cplusplus

	/** Binary length of the string excluding the terminal 0. This is NOT the 
	 *  logical length of strings containing UTF-8 multibyte sequences! It's
	 *  the number of bytes from the beginning of the string to its end.*/
	size_t length;

	/** String buffer. Size limit is MAXLEN */
	char data[MAXLEN];
} ;  // !struct aiString


// ----------------------------------------------------------------------------------
/**	Standard return type for some library functions.
 * Rarely used, and if, mostly in the C API.
 */
enum aiReturn
{
	/** Indicates that a function was successful */
	aiReturn_SUCCESS = 0x0,

	/** Indicates that a function failed */
	aiReturn_FAILURE = -0x1,

	/** Indicates that not enough memory was available
	 * to perform the requested operation 
	 */
	aiReturn_OUTOFMEMORY = -0x3,

	/** @cond never 
	 *  Force 32-bit size enum
	 */
	_AI_ENFORCE_ENUM_SIZE = 0x7fffffff 
};  // !enum aiReturn

// just for backwards compatibility, don't use these constants anymore
#define AI_SUCCESS     aiReturn_SUCCESS
#define AI_FAILURE     aiReturn_FAILURE
#define AI_OUTOFMEMORY aiReturn_OUTOFMEMORY

// ----------------------------------------------------------------------------------
/** Seek origins (for the virtual file system API).
 *  Much cooler than using SEEK_SET, SEEK_CUR or SEEK_END.
 */
enum aiOrigin
{
	/** Beginning of the file */
	aiOrigin_SET = 0x0,	

	/** Current position of the file pointer */
	aiOrigin_CUR = 0x1,		

	/** End of the file, offsets must be negative */
	aiOrigin_END = 0x2,

	/**  @cond never 
	 *   Force 32-bit size enum 
	 */
	_AI_ORIGIN_ENFORCE_ENUM_SIZE = 0x7fffffff 
}; // !enum aiOrigin

// ----------------------------------------------------------------------------------
/** @brief Enumerates predefined log streaming destinations. 
 *  Logging to these streams can be enabled with a single call to 
 *   #LogStream::createDefaultStream or #aiAttachPredefinedLogStream(),
 *   respectively.
 */
enum aiDefaultLogStream	
{
	/** Stream the log to a file */
	aiDefaultLogStream_FILE = 0x1,

	/** Stream the log to std::cout */
	aiDefaultLogStream_STDOUT = 0x2,

	/** Stream the log to std::cerr */
	aiDefaultLogStream_STDERR = 0x4,

	/** MSVC only: Stream the log the the debugger
	 * (this relies on OutputDebugString from the Win32 SDK)
	 */
	aiDefaultLogStream_DEBUGGER = 0x8,

	/** @cond never 
	 *  Force 32-bit size enum 
	 */
	_AI_DLS_ENFORCE_ENUM_SIZE = 0x7fffffff 
}; // !enum aiDefaultLogStream

// just for backwards compatibility, don't use these constants anymore
#define DLS_FILE     aiDefaultLogStream_FILE
#define DLS_STDOUT   aiDefaultLogStream_STDOUT
#define DLS_STDERR   aiDefaultLogStream_STDERR
#define DLS_DEBUGGER aiDefaultLogStream_DEBUGGER

// ----------------------------------------------------------------------------------
/** Stores the memory requirements for different components (e.g. meshes, materials,
 *  animations) of an import. All sizes are in bytes.
 *  @see Importer::GetMemoryRequirements()
*/
struct aiMemoryInfo
{
#ifdef __cplusplus

	/** Default constructor */
	aiMemoryInfo()
		: textures   (0)
		, materials  (0)
		, meshes     (0)
		, nodes      (0)
		, animations (0)
		, cameras	 (0)
		, lights	 (0)
		, total      (0)
	{}

#endif

	/** Storage allocated for texture data */
	unsigned int textures;

	/** Storage allocated for material data  */
	unsigned int materials;

	/** Storage allocated for mesh data */
	unsigned int meshes;

	/** Storage allocated for node data */
	unsigned int nodes;

	/** Storage allocated for animation data */
	unsigned int animations;

	/** Storage allocated for camera data */
	unsigned int cameras;

	/** Storage allocated for light data */
	unsigned int lights;

	/** Total storage allocated for the full import. */
	unsigned int total;
}; // !struct aiMemoryInfo 

#ifdef __cplusplus
}
#endif //!  __cplusplus

// Include implementations
#include "aiVector3D.inl"
#include "aiColor4D.inl"
#include "aiMatrix3x3.inl"
#include "aiMatrix4x4.inl"
#endif 
