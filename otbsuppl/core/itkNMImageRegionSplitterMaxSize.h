/******************************************************************************
* Created by Alexander Herzig
* Copyright 2015 Landcare Research New Zealand Ltd
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef __itkNMImageRegionSplitterMaxSize_h
#define __itkNMImageRegionSplitterMaxSize_h

#include "itkRegion.h"
#include "itkIndex.h"
#include "itkImageRegionSplitterBase.h"
#include "otbsupplfilters_export.h"

namespace itk
{
/** \class NMImageRegionSplitterMaxSize
 * \brief Splits a region into n subregions of a given size
 *
 * The class splits a given n-dimensional region into subregions
 * smaller or equal to a user specified size (i.e. number of pixel).
 * The split region is built up of chunks of rows and extends to
 * higher dimensions as long as the maximum number of pixels is
 * not exceeded.
 *
 * NMImageRegionSplitterMaxSize uses a similar API as the standard
 * ITK region splitter: The user calls \c GetNumberOfSplits() to
 * determine the number of splits created by this splitter. However,
 * in contrast to the standard implementation, \code requestedSize
 * is used to specify the requested number of pixel per region. If
 * \code requestedSize is smaller or equal to zero, \code MaxSize
 * pixel will be used. The nth split of the region is then retreived
 * with \code GetSplit, whereas the \code requestedSize parameter
 * also denotes the requested number of pixel per region.
 *
 * \sa SetMaxSize
 * \sa ImageRegionSplitterMultidimensional
 */

class NMImageRegionSplitterMaxSize
  : public ImageRegionSplitterBase
{
public:
  /** Standard class typedefs. */
  typedef NMImageRegionSplitterMaxSize Self;
  typedef ImageRegionSplitterBase             Superclass;
  typedef SmartPointer< Self >                Pointer;
  typedef SmartPointer< const Self >          ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(NMImageRegionSplitterMaxSize, ImageRegionSplitterBase);

  /** Get/Set the maximum number of pixel (size) per split region.
   *  Note: The minimum allowed size is the size of dimension 0
   *  (i.e. the fastest moving index) and the default split region
   *  size is 33554432 (= 128 MiB with 4 byte per pixel)
   */
  itkSetMacro(MaxSize, unsigned int)
  itkGetMacro(MaxSize, unsigned int)

protected:
  NMImageRegionSplitterMaxSize();


  virtual unsigned int GetNumberOfSplitsInternal(unsigned int dim,
                                                 const IndexValueType regionIndex[],
                                                 const SizeValueType regionSize[],
                                                 unsigned int requestedSize) const;

  virtual unsigned int GetSplitInternal(unsigned int dim,
                                        unsigned int i,
                                        unsigned int requestedSize,
                                        IndexValueType regionIndex[],
                                        SizeValueType regionSize[]) const;

  unsigned int ComputeSplits(unsigned int dim,
                             double maxsize,
                             const SizeValueType regionSize[],
                             std::vector<unsigned int>& splitRegion) const;

  void PrintSelf(std::ostream & os, Indent indent) const;

  unsigned int m_MaxSize;
//  std::vector<long> m_SplitRegion;
//  unsigned int m_NumSplitRegions;

private:
  NMImageRegionSplitterMaxSize(const Self &); //purposely not implemented
  void operator=(const Self &);                      //purposely not
                                                     //implemented
};
} // end namespace itk

#endif
