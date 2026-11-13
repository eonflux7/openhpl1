// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_VECTOR2_H
#define HPL_VECTOR2_H

#include <math.h>
#include <stdio.h>
#include "system/SystemTypes.h"

namespace hpl {

	template <class T> 	class cVector2 {
		public:
			union{
				struct {
					T x,y;
				};
				T v[2];
			};

			cVector2()
			{
				x=0;y=0;
			}
			cVector2(T aVal)
			{
				x=aVal;y=aVal;
			}
			cVector2(T aX, T aY)
			{
				x=aX;y=aY;
			}

			cVector2(cVector2<T> const &aVec)
			{
				x = aVec.x; y = aVec.y;
			}

			// Copy

			inline cVector2<T>& operator=(const cVector2<T> &aVec)
			{
				x = aVec.x;
				y = aVec.y;
				return *this;
			}

			inline cVector2<T>& operator=(const T aVal)
			{
				x = aVal;
				y = aVal;
				return *this;
			}

			// Boolean

			inline bool operator==(const cVector2<T> &aVec) const
			{
				if(x == aVec.x && y==aVec.y)return true;
				else return false;
			}

			inline bool operator!=(const cVector2<T> &aVec) const
			{
				if(x == aVec.x && y==aVec.y)return false;
				else return true;
			}

			// Vector Arithmetic

			inline cVector2<T> operator+(const cVector2<T> &aVec) const
			{
				cVector2<T> vec;
				vec.x = x + aVec.x;
				vec.y = y + aVec.y;
				return vec;
			}

			inline cVector2<T> operator-(const cVector2<T> &aVec) const
			{
				cVector2<T> vec;
				vec.x = x - aVec.x;
				vec.y = y - aVec.y;
				return vec;
			}

			inline cVector2<T> operator*(const cVector2<T> &aVec) const
			{
				cVector2<T> vec;
				vec.x = x * aVec.x;
				vec.y = y * aVec.y;
				return vec;
			}

			inline cVector2<T> operator/(const cVector2<T> &aVec) const
			{
				cVector2<T> vec;
				vec.x = x / aVec.x;
				vec.y = y / aVec.y;
				return vec;
			}

			inline cVector2<T>& operator-=(const cVector2<T>  &aVec)
			{
				x-=aVec.x;
				y-=aVec.y;
				return *this;
			}

			inline cVector2<T>& operator+=(const cVector2<T> &aVec)
			{
				x+=aVec.x;
				y+=aVec.y;
				return *this;
			}

			inline cVector2<T>& operator*=(const cVector2<T>  &aVec)
			{
				x*=aVec.x;
				y*=aVec.y;
				return *this;
			}

			inline cVector2<T>& operator/=(const cVector2<T> &aVec)
			{
				x/=aVec.x;
				y/=aVec.y;
				return *this;
			}

			// Real Arithmetic

			inline cVector2<T> operator/(const T &aVal) const
			{
				cVector2<T> vec;
				vec.x = x / aVal;
				vec.y = y / aVal;
				return vec;
			}

			inline cVector2<T> operator*(const T &aVal) const
			{
				cVector2<T> vec;
				vec.x = x * aVal;
				vec.y = y * aVal;
				return vec;
			}

			inline cVector2<T> operator+(const T &aVal) const
			{
				cVector2<T> vec;
				vec.x = x + aVal;
				vec.y = y + aVal;
				return vec;
			}

			cVector2<T> operator-(const T &aVal) const
			{
				cVector2<T> vec;
				vec.x = x - aVal;
				vec.y = y - aVal;
				return vec;
			}

			inline void FromVec(const T *apVec)
			{
				x = apVec[0];
				y = apVec[1];
			}

			/**
			 * Only use this on double or float vectors
			 * \return Length of Vector
			 */
			T Normalise()
			{
				T length  = sqrt( x * x + y * y);

				if ( length > 1e-08 )
				{
					T InvLength = 1.0f / length;
					x *= InvLength;
					y *= InvLength;
				}

				return length;
			}

			// Printing

			tString ToString() const{
				char buf[512];
				sprintf(buf,"%f : %f",x,y);
				tString str = buf;
				return str;
			}

			tString ToFileString() const{
				char buf[512];
				sprintf(buf,"%g %g",x,y);
				tString str = buf;
				return str;
			}
	};

};
#endif // HPL_VECTOR2_H
