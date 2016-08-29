/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

/******************************************************************************
 * Adapted by Alexander Herzig
 * Copyright 2016 Landcare Research New Zealand Ltd
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


#ifndef itkNMConstShapedNeighborhoodIterator_h
#define itkNMConstShapedNeighborhoodIterator_h

#include "itkConstShapedNeighborhoodIterator.h"

namespace itk
{
/*! \class NMConstShapedNeighborhoodIterator
 *
 *  \brief Specialised version of the original ITK class; it assumes that
 *         ::ActivateIndex() is called in order, and therefore avoids expensive
 *         re-ordering
 */

template< typename TImage,  typename TBoundaryCondition =
            ZeroFluxNeumannBoundaryCondition< TImage > >
class NMConstShapedNeighborhoodIterator:
  private NeighborhoodIterator< TImage, TBoundaryCondition >
{
public:

  /** Extract image type information. */
  typedef typename TImage::InternalPixelType InternalPixelType;
  typedef typename TImage::PixelType         PixelType;

  /** Save the image dimension. */
  itkStaticConstMacro(Dimension, unsigned int, TImage::ImageDimension);

  /** Standard class typedefs. */
  typedef NMConstShapedNeighborhoodIterator                    Self;
  typedef NeighborhoodIterator< TImage, TBoundaryCondition > Superclass;

  /** Inherit typedefs from superclass */
  typedef typename Superclass::OffsetType      OffsetType;
  typedef typename OffsetType::OffsetValueType OffsetValueType;
  typedef typename Superclass::RadiusType      RadiusType;
  typedef typename Superclass::SizeType        SizeType;
  typedef typename SizeType::SizeValueType     SizeValueType;

  /** Typedef support for common objects */
  typedef TImage                                     ImageType;
  typedef typename TImage::RegionType                RegionType;
  typedef Index< itkGetStaticConstMacro(Dimension) > IndexType;
  typedef typename IndexType::IndexValueType         IndexValueType;
  typedef Neighborhood< PixelType, itkGetStaticConstMacro(Dimension) >
  NeighborhoodType;

  typedef typename NeighborhoodType::NeighborIndexType  NeighborIndexType;

  /** In this specialised version of the class, we use a vector for
   *  storage and assume, that ::ActivateIndex is called in order
   */
  typedef std::vector< NeighborIndexType >             IndexListType;

  typedef typename IndexListType::iterator        IndexListIterator;
  typedef typename IndexListType::const_iterator  IndexListConstIterator;

  /** Typedef for boundary condition type. */
  typedef TBoundaryCondition BoundaryConditionType;

  /** Typedef for generic boundary condition pointer */
  typedef ImageBoundaryCondition< ImageType > *ImageBoundaryConditionPointerType;

  /** Const Interator */
  struct ConstIterator {
    ConstIterator() { m_NeighborhoodIterator = ITK_NULLPTR; }
    ConstIterator(Self *s)
    {
      m_NeighborhoodIterator = s;
      this->GoToBegin();
    }

    virtual ~ConstIterator() {}

    ConstIterator & operator=(const ConstIterator & o)
    {
      m_NeighborhoodIterator = o.m_NeighborhoodIterator;
      m_ListIterator = o.m_ListIterator;
      return *this;
    }

    ConstIterator(const ConstIterator & o)
    {
      m_NeighborhoodIterator = o.m_NeighborhoodIterator;
      m_ListIterator = o.m_ListIterator;
    }

    void operator++(int)
    { m_ListIterator++; }

    void operator--(int)
    { m_ListIterator--; }

    const ConstIterator & operator++()
    {
      m_ListIterator++;
      return *this;
    }

    const ConstIterator & operator--()
    {
      m_ListIterator--;
      return *this;
    }

    bool operator!=(const ConstIterator & o) const
    { return m_ListIterator != o.m_ListIterator; }
    bool operator==(const ConstIterator & o) const
    { return m_ListIterator == o.m_ListIterator; }

    bool IsAtEnd() const
    {
      if ( m_ListIterator == m_NeighborhoodIterator->GetActiveIndexList().end() )
        {
        return true;
        }
      else
        {
        return false;
        }
    }

    void GoToBegin()
    {
      m_ListIterator = m_NeighborhoodIterator->GetActiveIndexList().begin();
    }

    void GoToEnd()
    {
      m_ListIterator = m_NeighborhoodIterator->GetActiveIndexList().end();
    }

    PixelType Get() const
    { return m_NeighborhoodIterator->GetPixel(*m_ListIterator); }

    OffsetType GetNeighborhoodOffset() const
    { return m_NeighborhoodIterator->GetOffset(*m_ListIterator); }

    typename IndexListType::value_type GetNeighborhoodIndex() const
    { return *m_ListIterator; }

protected:

    Self *m_NeighborhoodIterator;

    typename IndexListType::const_iterator m_ListIterator;

    void ProtectedSet(const PixelType & v) const
    { m_NeighborhoodIterator->SetPixel(*m_ListIterator, v); }
  };

  /** Returns a const iterator for the neighborhood which points to the first
   * pixel in the neighborhood. */
  const ConstIterator & Begin() const
  { return m_ConstBeginIterator; }

  /** Returns a const iterator for the neighborhood which points to the last
   * pixel in the neighborhood. */
  const ConstIterator & End() const
  { return m_ConstEndIterator; }

  /** Default constructor */
  NMConstShapedNeighborhoodIterator()
  {
    InitializeNMConstShapedNeighborhoodIterator();
  }

  /** Initialize the iterator. */
  void InitializeNMConstShapedNeighborhoodIterator()
  {
    m_ConstBeginIterator = ConstIterator(this);
    m_ConstEndIterator = ConstIterator(this);
    m_ConstEndIterator.GoToEnd();
    m_CenterIsActive = false;
  }

  /** Virtual destructor */
  virtual ~NMConstShapedNeighborhoodIterator()  {}

  /** Constructor which establishes the region size, neighborhood, and image
   * over which to walk. */
  NMConstShapedNeighborhoodIterator(const SizeType & radius,
                                  const ImageType *ptr,
                                  const RegionType & region):
    Superclass (radius, const_cast< ImageType * >( ptr ), region)
  {
    InitializeNMConstShapedNeighborhoodIterator();
  }

  // Expose the following methods from the superclass.  This is a
  // restricted subset of the methods available for
  // ConstNeighborhoodIterator.
  using Superclass::GetImagePointer;
  using Superclass::GetRadius;
  using Superclass::GetIndex;
  using Superclass::GetNeighborhoodIndex;
  using Superclass::GetCenterNeighborhoodIndex;
  using Superclass::GetRegion;
  using Superclass::GetBeginIndex;
  using Superclass::GoToBegin;
  using Superclass::GoToEnd;
  using Superclass::IsAtBegin;
  using Superclass::IsAtEnd;
  using Superclass::GetOffset;
  using Superclass::operator==;
  using Superclass::operator!=;
  using Superclass::operator<;
  using Superclass::operator>;
  using Superclass::operator>=;
  using Superclass::operator<=;
  using Superclass::operator[];
  using Superclass::GetElement;
  using Superclass::SetLocation;
  using Superclass::GetCenterPointer;
  using Superclass::GetCenterPixel;
  using Superclass::OverrideBoundaryCondition;
  using Superclass::ResetBoundaryCondition;
  using Superclass::GetBoundaryCondition;
  using Superclass::GetNeedToUseBoundaryCondition;
  using Superclass::SetNeedToUseBoundaryCondition;
  using Superclass::NeedToUseBoundaryConditionOn;
  using Superclass::NeedToUseBoundaryConditionOff;
  using Superclass::Print;
  using Superclass::operator-;
  using Superclass::GetPixel;
  using Superclass::SetRegion;

  /** Assignment operator */
  Self & operator=(const Self & orig)
  {
    if(this != &orig)
      {
      Superclass::operator=(orig);
      m_ActiveIndexList = orig.m_ActiveIndexList;
      m_CenterIsActive = orig.m_CenterIsActive;

      // Reset begin and end pointers
      m_ConstBeginIterator.GoToBegin();
      m_ConstEndIterator.GoToBegin();
      }
    return *this;
  }

  /** Standard itk print method */
  virtual void PrintSelf(std::ostream &, Indent) const;

  /** Add/Remove a neighborhood offset (from the center of the neighborhood)
   *  to/from the active list.  Active list offsets are the only locations
   *  updated and accessible through the iterator.  */
  virtual void ActivateOffset(const OffsetType & off)
  { this->ActivateIndex( Superclass::GetNeighborhoodIndex(off) ); }
  virtual void DeactivateOffset(const OffsetType & off)
  { this->DeactivateIndex( Superclass::GetNeighborhoodIndex(off) ); }

  /** Removes all active pixels from this neighborhood. */
  virtual void ClearActiveList()
  {
    m_ActiveIndexList.clear();
    m_ConstBeginIterator.GoToBegin();
    m_ConstEndIterator.GoToEnd();
    m_CenterIsActive = false;
  }

  /** Returns the list of active indices in the neighborhood */
  const IndexListType & GetActiveIndexList() const
  { return m_ActiveIndexList; }

  void SetActiveIndexList(const IndexListType& activeList);

  /** Returns the size of the list of active neighborhood indices. */
  typename IndexListType::size_type GetActiveIndexListSize() const
  { return m_ActiveIndexList.size(); }

  /** Add non-zero neighborhood offsets to the active list. The
    * radius of the neighborhood must match the radius of the shaped
    * iterator */
  void CreateActiveListFromNeighborhood(const NeighborhoodType &);

  /** Reimplements the operator++ method so that only active pixel locations
   * are updated. */
  Self & operator++();

  /** Reimplements the operator-- method so that only active pixel locations
   * are updated. */
  Self & operator--();

  /** Addition of an itk::Offset.  Note that this method does not do any bounds
   * checking.  Adding an offset that moves the iterator out of its assigned
   * region will produce undefined results. */
  Self & operator+=(const OffsetType &);

  /** Subtraction of an itk::Offset. Note that this method does not do
   *  any bounds checking.  Subtracting an offset that moves the iterator
   *  out of its assigned region will produce undefined results. */
  Self & operator-=(const OffsetType &);

protected:
  using Superclass::SetPixel;
  using Superclass::SetCenterPixel;
  /** Copy constructor */
  NMConstShapedNeighborhoodIterator(const NMConstShapedNeighborhoodIterator &);
  // purposely not implemented

  friend struct ConstIterator;

  /** Class is protected here so that it is not publicly accessible, but can be
   * accessed by subclasses.. */
  //  Superclass::SetPixel;
  //  Superclass::SetCenterPixel;

  /** Add/Remove a neighborhood index to/from the active.  Locations in the
      active list are the only accessible elements in the neighborhood. The
      argument is an index location calculated as an offset into a linear
      array which represents the image region defined by the radius of this
      iterator, with the smallest dimension as the fastest increasing index. */
  virtual void ActivateIndex( NeighborIndexType );

  virtual void DeactivateIndex( NeighborIndexType );

  bool          m_CenterIsActive;
  IndexListType m_ActiveIndexList;
  ConstIterator m_ConstEndIterator;
  ConstIterator m_ConstBeginIterator;
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkNMConstShapedNeighborhoodIterator.hxx"
#endif

#endif
