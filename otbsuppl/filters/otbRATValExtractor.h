/*=========================================================================

  Program:   ORFEO Toolbox
    Language:  C++
    Date:      $Date$
    Version:   $Revision$


    Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
    See OTBCopyright.txt for details.

    Some parts of this code are derived from ITK. See ITKCopyright.txt
    for details.


    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
        PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

/******************************************************************************
 * Adapted by Alexander Herzig
 * Copyright 2010-2015 Landcare Research New Zealand Ltd
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

#ifndef __otbRATValExtractor_h
#define __otbRATValExtractor_h

#include "itkInPlaceImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkArray.h"

#include "otbAttributeTable.h"

#include "otbsupplfilters_export.h"

namespace otb
{
/** \class RATValExtractor
  *
 * \ingroup Streamed
 * \ingroup Threaded
 */

template< class TImage >
class OTBSUPPLFILTERS_EXPORT RATValExtractor
  : public itk::InPlaceImageFilter< TImage >
{
public:
  /** Standard class typedefs. */
  typedef RATValExtractor< TImage >                 Self;
  typedef itk::InPlaceImageFilter< TImage >             Superclass;
  typedef itk::SmartPointer< Self >                     Pointer;
  typedef itk::SmartPointer< const Self >               ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(RATValExtractor, InPlaceImageFilter);

  /** Some convenient typedefs. */
  typedef TImage                                  ImageType;
  typedef typename ImageType::ConstPointer        ImageConstPointer;
  typedef typename ImageType::Pointer             ImagePointer;
  typedef typename ImageType::RegionType          ImageRegionType;
  typedef typename ImageRegionType::SizeType      SizeType;
  typedef typename ImageType::PixelType           PixelType;
  typedef typename ImageType::IndexType           IndexType;
  typedef typename ImageType::PointType           OriginType;
  typedef typename ImageType::SpacingType         SpacingType;

  typedef typename itk::ImageBase< ImageType::ImageDimension > ImageBaseType;

  typedef MultiParser                             ParserType;

  typedef typename AttributeTable::Pointer		   TablePointer;
  typedef typename AttributeTable::TableColumnType ColumnType;

  /** Set the nth filter input with or without a specified associated variable name */
  void SetNthInput( unsigned int idx, const ImageType * image);
  void SetNthInput( unsigned int idx, const ImageType * image, const std::string& varName);

  /** Set the attribute table of the nth filter input */
  void SetNthAttributeTable( unsigned int idx, const AttributeTable::Pointer,
          std::vector<std::string> vAttrNames);

  /** Change the nth filter input associated variable name */
  void SetNthInputName(unsigned int idx, const std::string& expression);

  /** Set the expression to be parsed */
  void SetExpression(const std::string& expression);

  /** Return the expression to be parsed */
  std::string GetExpression() const;

  /** Return the nth filter input associated variable name */
  std::string GetNthInputName(unsigned int idx) const;

  /** Return a pointer on the nth filter input */
  ImageType * GetNthInput(unsigned int idx);

  /** Return the nth attribute table */
  AttributeTable::Pointer GetNthAttributeTable(unsigned int idx);
  std::vector<std::string> GetNthTableAttributes(unsigned int idx);

  itkSetMacro(UseTableColumnCache, bool)
  itkGetMacro(UseTableColumnCache, bool)
  itkBooleanMacro(UseTableColumnCache)

  void ResetPipeline();

protected :
  RATValExtractor();
  virtual ~RATValExtractor();
  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;

  void ThreadedGenerateData(const ImageRegionType& outputRegionForThread, itk::ThreadIdType threadId );
  void CacheTableColumns(int idx);

private :
  RATValExtractor(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  /** Attribute Table support */

  std::string                           m_Expression;
  std::string                           m_ConcatChar;
  std::vector< std::vector<PixelType> > m_AImage;
  std::vector< std::string >            m_VVarName;
  std::vector< std::string >            m_UserNames;
  unsigned int                          m_NbVar;
  int					m_NbExpr;

  SpacingType                           m_Spacing;
  OriginType                            m_Origin;

  bool                                  m_UseTableColumnCache;
  std::vector<std::map<int, std::map<long long, double> > >  m_TableColumnDblCache;
  std::vector<std::map<int, std::map<long long, long long> > >  m_TableColumnLLongCache;
  long                                  m_UnderflowCount;
  long                                  m_OverflowCount;
  itk::Array<long>                      m_ThreadUnderflow;
  itk::Array<long>                      m_ThreadOverflow;

  /** Attribute Table support */
  std::vector<std::vector<TablePointer > > m_VRAT; //m_VThreadRAT;
  std::vector< std::vector<int> > 	m_VTabAttr;
  std::vector< std::vector< ColumnType > > m_VAttrTypes;
  std::vector< std::vector< std::vector<double> > >	m_VAttrValues;

};

}//end namespace otb

//#include "otbRATValExtractor_ExplicitInst.h"

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbRATValExtractor.txx"
#endif

#endif
