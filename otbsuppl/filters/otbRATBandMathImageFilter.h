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

#ifndef __otbRATBandMathImageFilter_h
#define __otbRATBandMathImageFilter_h

#include "itkInPlaceImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkArray.h"

#include "otbMultiParser.h"
#include "otbAttributeTable.h"

#include "otbsupplfilters_export.h"

namespace otb
{
/** \class RATBandMathImageFilter
 * \brief Extension to the BandMathImageFilter which performs a mathematical
 * operation on the input images according to the formula specified by the user.
 * This extension allows to integrate raster attributes into those
 * mathematical expressions. Attribute Values within an expression are
 * referenced by combining the band name with the band's attributes in the
 * following manner:
 * <band name>__<column name>		e.g.: dist__SBH - 3 * dist__CCOL + lu
 *  (double underscore)									  b1__SBH - 3 * b1__CCOL + b2
 *
 * This filter is based on the mathematical parser library muParser.
 * The built in functions and operators list is available at:
 * http://muparser.sourceforge.net/mup_features.html#idDef2
 *
 * OTB additional functions:
 * ndvi(r, niri)
 * 
 * OTB additional constants:
 * e - log2e - log10e - ln2 - ln10 - pi - euler
 *
 * In order to use this filter, at least one input image is to be
 * set. An associated variable name can be specified or not by using
 * the corresponding SetNthInput method. For the nth input image, if
 * no associated variable name has been spefified, a default variable
 * name is given by concatenating the letter "b" (for band) and the
 * corresponding input index.
 * Next step is to set the expression according to the variable
 * names. For example, in the default case with three input images the
 * following expression is valid :
 * "ndvi(b1,b2)*b3"
 * 
 * As an additional functionality, the filter also granted access to
 * indexes information under special virtual bands named idxX, idxY
 * for the images indexes and idxPhyX,idxPhyY for the physical
 * indexes.
 * It allows the user to perform, for example a spatial processing
 * aiming to suppress a determined area :
 * "if(sqrt((idxPhyX-105.3)*(idxPhyX-105.3)+
 *          (idxPhyY-207.1)*(idxPhyY-207.1))>100, b1, 0)"
 * This expression replace the physical zone around the point of
 * physical index (105.3;207.1) by a black area
 * This functionality assumes that all the band involved have the same
 * spacing and origin.
 * 
 * NOTE:
 * The inputs to this filter, if more than one, need to be specified in
 * sequential order, i.e. starting with index 0: 0,1,...,n
 * the reason is that the filter may be used as part of a LUMASS model
 * and may be subsequnently executed with a varying number of inputs.
 * When ever an indexed input is specified, all potentially previously
 * set input images are removed from the input list.
 * 
 * \sa Parser
 * 
 * \ingroup Streamed
 * \ingroup Threaded
 */

template< class TImage >
class OTBSUPPLFILTERS_EXPORT RATBandMathImageFilter
  : public itk::InPlaceImageFilter< TImage >
{
public:
  /** Standard class typedefs. */
  typedef RATBandMathImageFilter< TImage >                 Self;
  typedef itk::InPlaceImageFilter< TImage >             Superclass;
  typedef itk::SmartPointer< Self >                     Pointer;
  typedef itk::SmartPointer< const Self >               ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  
  /** Run-time type information (and related methods). */
  itkTypeMacro(RATBandMathImageFilter, InPlaceImageFilter);

  /** Some convenient typedefs. */
  typedef TImage                                  ImageType;
  typedef typename ImageType::ConstPointer        ImagePointer;
  typedef typename ImageType::RegionType          ImageRegionType; 
  typedef typename ImageType::PixelType           PixelType;
  typedef typename ImageType::IndexType           IndexType;
  typedef typename ImageType::PointType           OrigineType;
  typedef typename ImageType::SpacingType         SpacingType;
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

  /** Indicates the number of expressions provided to the filter
   *  and thus the number of outputs generated by this filter;
   *  this parameter is mandatory for more than one output*/
  itkGetMacro(NbExpr, int);
  void SetNbExpr(int numExpr);

  itkSetMacro(UseTableColumnCache, bool)
  itkGetMacro(UseTableColumnCache, bool)
  itkBooleanMacro(UseTableColumnCache)

  void ResetPipeline();

protected :
  RATBandMathImageFilter();
  virtual ~RATBandMathImageFilter();
  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;
 
  void BeforeThreadedGenerateData();
  void ThreadedGenerateData(const ImageRegionType& outputRegionForThread, itk::ThreadIdType threadId );
  void AfterThreadedGenerateData();

  void CacheTableColumns(int idx);

private :
  RATBandMathImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  /** Attribute Table support */
  //bool DefineAttrVars();

  std::string                           m_Expression; 
  std::string                           m_ConcatChar;
  std::vector<ParserType::Pointer>      m_VParser;
  std::vector< std::vector<double> >    m_AImage;
  std::vector< std::string >            m_VVarName;
  std::vector< std::string >            m_UserNames;
  unsigned int                          m_NbVar;
  int									m_NbExpr;

  SpacingType                           m_Spacing;
  OrigineType                           m_Origin;

  bool                                  m_UseTableColumnCache;
  std::vector<std::map<int, std::map<long, double> > >  m_TableColumnCache;
  long                                  m_UnderflowCount;
  long                                  m_OverflowCount;
  itk::Array<long>                      m_ThreadUnderflow;
  itk::Array<long>                      m_ThreadOverflow;

  /** Attribute Table support */
  //std::vector<TablePointer> m_VRAT;
  std::vector<std::vector<TablePointer > > m_VRAT; //m_VThreadRAT;
  std::vector< std::vector<int> > 	m_VTabAttr;
  std::vector< std::vector< ColumnType > > m_VAttrTypes;
  std::vector< std::vector< std::vector<double> > >	m_VAttrValues;
};

}//end namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbRATBandMathImageFilter.txx"
#endif

#endif
