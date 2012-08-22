/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkPixelTraits.h,v $
  Language:  C++
  Date:      $Date: 2009-03-03 15:07:54 $
  Version:   $Revision: 1.30 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  Portions of this code are covered under the VTK copyright.
  See VTKCopyright.txt or http://www.kitware.com/VTKCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkNMPixelTraits_h
#define __itkNMPixelTraits_h

#include "itkMacro.h"

namespace itk
{

/** \class NMPixelTraits
 * \brief Traits for a pixel that define the dimension and component type.
 *
 * NMPixelTraits determines the dimension and the component type
 * of a pixel.  The default implementation is suitable for all subclasses
 * of itk::Array. This (will) include RGBPixel and RGBAPixel. Specialized
 * versions of NMPixelTraits are defined for the standard scalar types.
 */
template<class TPixelType>
class NMPixelTraits
{
public:
  /** Dimension of the pixel (range). */
  itkStaticConstMacro(Dimension, unsigned int, TPixelType::Length);
  
  /** Type of a single component of a pixel. */
  typedef typename TPixelType::ValueType ValueType;
};


/** \class NMPixelTraits<bool>
 * Specialization of NMPixelTraits for scalar images. */
template <> class NMPixelTraits<bool>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef bool ValueType;
};

template <> class NMPixelTraits<char>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef char ValueType;
};

template <> class NMPixelTraits<signed char>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef char ValueType;
};

template <> class NMPixelTraits<unsigned char>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef unsigned char ValueType;
};

template <> class NMPixelTraits<short>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef short ValueType;
};

template <> class NMPixelTraits<unsigned short>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef unsigned short ValueType;
};

template <> class NMPixelTraits<int>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef int ValueType;
};

template <> class NMPixelTraits<unsigned int>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef unsigned int ValueType;
};

template <> class NMPixelTraits<long>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef long ValueType;
};

template <> class NMPixelTraits<unsigned long>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef unsigned long ValueType;
};

template <> class NMPixelTraits<float>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef float ValueType;
};

template <> class NMPixelTraits<double>
{
public:
  itkStaticConstMacro(Dimension, unsigned int, 1);
  typedef double ValueType;
};

/** \class NMJoinTraits
 * \brief Trait to determine what datatype is needed if the specified 
 * pixel types are "joined" into a single vector.
 *
 * NMJoinTraits defines the value type needed to combine the specified
 * pixel types into a single vector.  The data type selected is the
 * smallest data type that can represent the dynamic range of both
 * input pixel types.  For example, if a char and unsigned short are
 * "joined", the resulting data type must be a vector of int.  In
 * some cases, like joining a unsigned int and a char, the join
 * value type is promoted all the way to a float.  This provides
 * consistent behavior on both 32 and 64 bit systems (on 64 bit
 * systems, we could have promoted to a long which is distinct from
 * an int but this is not the case for 32 bit systems, so we promote
 * to float). There are several combinations similar to this.  Most
 * of the NMJoinTraits are specializations of the base template.
 */
template <class TValueType1, class TValueType2>
class NMJoinTraits
{
public:
  typedef TValueType1 ValueType;
};

/** \class NMJoinTraits
 * Specializations for bool. */
template<>
class NMJoinTraits<bool, bool>
{
public:
  typedef bool ValueType;
};

template<>
class NMJoinTraits<bool, char>
{
public:
  typedef char ValueType;
};

template<>
class NMJoinTraits<bool, unsigned char>
{
public:
  typedef unsigned char ValueType;
};

template<>
class NMJoinTraits<bool, short>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<bool, unsigned short>
{
public:
  typedef unsigned short ValueType;
};

template<>
class NMJoinTraits<bool, int>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<bool, unsigned int>
{
public:
  typedef unsigned int ValueType;
};

template<>
class NMJoinTraits<bool, long>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<bool, unsigned long>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<bool, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<bool, double>
{
public:
  typedef double ValueType;
};

/**  \class NMPixelTraits<char>
 * Specializations for char. */
template<>
class NMJoinTraits<char, bool>
{
public:
  typedef char ValueType;
};

template<>
class NMJoinTraits<char, char>
{
public:
  typedef char ValueType;
};

template<>
class NMJoinTraits<char, unsigned char>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<char, short>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<char, unsigned short>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<char, int>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<char, unsigned int>
{
public:
  // unsigned int & unsigned long may be the same size, so promote to float
  typedef float ValueType;
};

template<>
class NMJoinTraits<char, long>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<char, unsigned long>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<char, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<char, double>
{
public:
  typedef double ValueType;
};

/**  \class NMPixelTraits<unsigned char>
 * Specializations for unsigned char. */
template<>
class NMJoinTraits<unsigned char, bool>
{
public:
  typedef unsigned char ValueType;
};

template<>
class NMJoinTraits<unsigned char, char>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<unsigned char, unsigned char>
{
public:
  typedef unsigned char ValueType;
};

template<>
class NMJoinTraits<unsigned char, short>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<unsigned char, unsigned short>
{
public:
  typedef unsigned short ValueType;
};

template<>
class NMJoinTraits<unsigned char, int>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<unsigned char, unsigned int>
{
public:
  typedef unsigned int ValueType;
};

template<>
class NMJoinTraits<unsigned char, long>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<unsigned char, unsigned long>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<unsigned char, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned char, double>
{
public:
  typedef double ValueType;
};
  
/**  \class NMPixelTraits<short>
 * Specializations for short. */
template<>
class NMJoinTraits<short, bool>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<short, char>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<short, unsigned char>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<short, short>
{
public:
  typedef short ValueType;
};

template<>
class NMJoinTraits<short, unsigned short>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<short, int>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<short, unsigned int>
{
public:
  // unsigned int & unsigned long may be the same size, so promote to float
  typedef float ValueType;
};

template<>
class NMJoinTraits<short, long>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<short, unsigned long>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<short, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<short, double>
{
public:
  typedef double ValueType;
};
  
/**  \class NMPixelTraits<unsigned short>
 * Specializations for unsigned short. */
template<>
class NMJoinTraits<unsigned short, bool>
{
public:
  typedef unsigned short ValueType;
};

template<>
class NMJoinTraits<unsigned short, char>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<unsigned short, unsigned char>
{
public:
  typedef unsigned short ValueType;
};

template<>
class NMJoinTraits<unsigned short, short>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<unsigned short, unsigned short>
{
public:
  typedef unsigned short ValueType;
};

template<>
class NMJoinTraits<unsigned short, int>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<unsigned short, unsigned int>
{
public:
  typedef unsigned int ValueType;
};

template<>
class NMJoinTraits<unsigned short, long>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<unsigned short, unsigned long>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<unsigned short, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned short, double>
{
public:
  typedef double ValueType;
};
  
/**  \class NMPixelTraits<int>
 * Specializations for int. */
template<>
class NMJoinTraits<int, bool>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<int, char>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<int, unsigned char>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<int, short>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<int, unsigned short>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<int, int>
{
public:
  typedef int ValueType;
};

template<>
class NMJoinTraits<int, unsigned int>
{
public:
  // unsigned int & unsigned long may be the same size, so promote to float
  typedef float ValueType;
};

template<>
class NMJoinTraits<int, long>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<int, unsigned long>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<int, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<int, double>
{
public:
  typedef double ValueType;
};
  
/**  \class NMPixelTraits<unsigned int>
 * Specializations for unsigned int. */
template<>
class NMJoinTraits<unsigned int, bool>
{
public:
  typedef unsigned int ValueType;
};

template<>
class NMJoinTraits<unsigned int, char>
{
public:
  // unsigned int & unsigned long may be the same size, so promote to float
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned int, unsigned char>
{
public:
  typedef unsigned int ValueType;
};

template<>
class NMJoinTraits<unsigned int, short>
{
public:
  // unsigned int & unsigned long may be the same size, so promote to float
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned int, unsigned short>
{
public:
  typedef unsigned int ValueType;
};

template<>
class NMJoinTraits<unsigned int, int>
{
public:
  // unsigned int & unsigned long may be the same size, so promote to float
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned int, unsigned int>
{
public:
  typedef unsigned int ValueType;
};

template<>
class NMJoinTraits<unsigned int, long>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned int, unsigned long>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<unsigned int, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned int, double>
{
public:
  typedef double ValueType;
};
  
/** \class NMPixelTraits<long>
 * Specializations for long. */
template<>
class NMJoinTraits<long, bool>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<long, char>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<long, unsigned char>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<long, short>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<long, unsigned short>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<long, int>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<long, unsigned int>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<long, long>
{
public:
  typedef long ValueType;
};

template<>
class NMJoinTraits<long, unsigned long>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<long, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<long, double>
{
public:
  typedef double ValueType;
};
  
/** \class NMPixelTraits<unsigned long>
 * Specializations for unsigned long. */
template<>
class NMJoinTraits<unsigned long, bool>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<unsigned long, char>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned long, unsigned char>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<unsigned long, short>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned long, unsigned short>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<unsigned long, int>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned long, unsigned int>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<unsigned long, long>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned long, unsigned long>
{
public:
  typedef unsigned long ValueType;
};

template<>
class NMJoinTraits<unsigned long, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<unsigned long, double>
{
public:
  typedef double ValueType;
};
  
/**  \class NMPixelTraits<float>
 * Specializations for float. */
template<>
class NMJoinTraits<float, bool>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, char>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, unsigned char>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, short>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, unsigned short>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, int>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, unsigned int>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, long>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, unsigned long>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, float>
{
public:
  typedef float ValueType;
};

template<>
class NMJoinTraits<float, double>
{
public:
  typedef double ValueType;
};
  
/** \class NMPixelTraits<double>
 * Specializations for double. */
template<>
class NMJoinTraits<double, bool>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, char>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, unsigned char>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, short>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, unsigned short>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, int>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, unsigned int>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, long>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, unsigned long>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, float>
{
public:
  typedef double ValueType;
};

template<>
class NMJoinTraits<double, double>
{
public:
  typedef double ValueType;
};
  
} // end namespace itk

#endif // __itkPixelTraits_h
