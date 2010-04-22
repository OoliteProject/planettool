/*
 *  SphericalPixelSource.h
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-02.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#ifndef INCLUDED_SphericalPixelSource_h
#define INCLUDED_SphericalPixelSource_h

#include "FloatPixMap.h"
#include <math.h>
#include "OOMaths.h"

FPM_BEGIN_EXTERN_C


#define kPiF			(3.14159265358979323846264338327950288f)
#define kDegToRad		(kPiF / 180.0f)
#define kRadToDeg		(180.0f / kPiF)


// In Mac OS X on Intel systems, fmodf() calls fmodl() (externally), but fmodl() is inlined and thus faster.
#if FMODF_JUST_WRAPS_FMODL
#undef fmodf
#define fmodf fmodl
#endif


/*	Coordinate convention: looking at a planet with the north pole upwards
	and the geographical coordinate origin in the middle, the vector coordinate
	space forms a right-handed basis with Y pointing north, X pointing east and
	Z pointing outwards. In other words:
	(0, 0, 1) is 0° N, 0° E.
	(0, 1, 0) is 90° N, <undefined>° E. (VectorToCoords will say 0° E.)
	(1, 0, 0) is 0° N, 90 °E.
*/

Vector VectorFromCoordsRad(float latitude, float longitude) FPM_PURE;
FPM_INLINE Vector VectorFromCoordsDeg(float latitude, float longitude) FPM_PURE;
FPM_INLINE Vector VectorFromCoordsDeg(float latitude, float longitude)
{
	return VectorFromCoordsRad(latitude * kDegToRad, longitude * kDegToRad);
}

void VectorToCoordsRad(Vector v, float *latitude, float *longitude);
void VectorToCoordsDeg(Vector v, float *latitude, float *longitude);


/*	Coordinate set, which can carry either lat/long or vector coordinates.
	This way, transformations are not made until it's necessary.
	
	Always manipulate Coordinates using the functions below.
*/
typedef struct Coordinates
{
	union
	{
		Vector			v;
		struct
		{
			float		lat;
			float		lon;
		}				l;
	}					d;
	enum
	{
		kCoordsVector, kCoordsLLRad, kCoordsLLDeg
	}					type;
} Coordinates;

FPM_INLINE Coordinates MakeCoordsVector(Vector v) FPM_PURE;
FPM_INLINE Coordinates MakeCoordsVector(Vector v)
{
	Coordinates result;
	result.d.v = v;
	result.type = kCoordsVector;
	return result;
}

FPM_INLINE Coordinates MakeCoordsLatLongRad(float lat, float lon)
{
	Coordinates result;
	result.d.l.lat = lat;
	result.d.l.lon = lon;
	result.type = kCoordsLLRad;
	return result;
}

FPM_INLINE Coordinates MakeCoordsLatLongDeg(float lat, float lon)
{
	Coordinates result;
	result.d.l.lat = lat;
	result.d.l.lon = lon;
	result.type = kCoordsLLDeg;
	return result;
}

FPM_INLINE Vector CoordsGetVector(Coordinates c) FPM_PURE;
FPM_INLINE Vector CoordsGetVector(Coordinates c)
{
	switch (c.type)
	{
		case kCoordsVector:  return c.d.v;
		case kCoordsLLRad:  return VectorFromCoordsRad(c.d.l.lat, c.d.l.lon);
	//	case kCoordsLLDeg:
		default: return VectorFromCoordsDeg(c.d.l.lat, c.d.l.lon);
	}
}

FPM_INLINE void CoordsGetLatLongRad(Coordinates c, float *lat, float *lon) FPM_NON_NULL_ALL;
FPM_INLINE void CoordsGetLatLongRad(Coordinates c, float *lat, float *lon)
{
	switch (c.type)
	{
		case kCoordsLLRad:  *lat = c.d.l.lat; *lon = c.d.l.lon; break;
		case kCoordsLLDeg:  *lat = c.d.l.lat * kDegToRad; *lon = c.d.l.lon * kDegToRad; break;
		case kCoordsVector:  VectorToCoordsRad(c.d.v, lat, lon); break;
	}
}

FPM_INLINE void CoordsGetLatLongDeg(Coordinates c, float *lat, float *lon) FPM_NON_NULL_ALL;
FPM_INLINE void CoordsGetLatLongDeg(Coordinates c, float *lat, float *lon)
{
	switch (c.type)
	{
		case kCoordsLLDeg:  *lat = c.d.l.lat; *lon = c.d.l.lon; break;
		case kCoordsLLRad:  *lat = c.d.l.lat * kRadToDeg; *lon = c.d.l.lon * kRadToDeg; break;
		case kCoordsVector:  VectorToCoordsDeg(c.d.v, lat, lon); break;
	}
}


enum
{
	kRenderFast		= 0x00000001,
	kRenderJitter	= 0x00000002
};
typedef uint32_t RenderFlags;


/*	Function which takes a (unit) vector and returns a colour, used to sample
	any generator or input format.
*/
typedef FPMColor (*SphericalPixelSourceFunction)(Coordinates where, RenderFlags flags, void *context);

typedef bool (*SphericalPixelSourceConstructorFunction)(FloatPixMapRef sourceImage, RenderFlags flags, void **context);
typedef void (*SphericalPixelSourceDestructorFunction)(void *context);

typedef void (*ProgressCallbackFunction)(size_t numerator, size_t denominator, void *context);

typedef FloatPixMapRef (*SphericalPixelSinkFunction)(size_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, void *progressContext);


//	Build a lookup table of Gauss distribution numbers.
void BuildGaussTable(unsigned size, float *table);

//	Look up a value in a pregenerated Gauss table.
float GaussTableLookup(float value, float mid, float halfWidth, unsigned tblSize, float *table);
float GaussTableLookup2D(float x, float xmid, float y, float ymid, float halfWidth, unsigned tblSize, float *table);


void DummyProgressCallback(size_t numerator, size_t denominator, void *context);


// [0..1]
FPM_INLINE float RandF(void)
{
	return (float)random() / (float)0x7FFFFFFF;
}


// [-1..1]
FPM_INLINE float RandF2(void)
{
	return RandF() * 2.0f - 1.0f;
}

FPM_END_EXTERN_C
#endif	/* INCLUDED_SphericalPixelSource_h */
