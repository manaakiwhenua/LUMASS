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


#ifndef __otbRATBandMathImageFilter_txx
#define __otbRATBandMathImageFilter_txx
#include "otbRATBandMathImageFilter.h"

#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkNumericTraits.h"
#include "itkProgressReporter.h"
#include "otbMacro.h"
#include "itkMacro.h"
#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"

#include <iostream>
#include <string>

namespace otb
{

/** Constructor */
template <class TImage>
RATBandMathImageFilter<TImage>
::RATBandMathImageFilter()
{
  //This number will be incremented each time an image
  //is added over the one minimumrequired
  this->SetNumberOfRequiredInputs( 1 );
  this->InPlaceOff();

  m_NbExpr = 1;
  m_UnderflowCount = 0;
  m_OverflowCount = 0;
  m_ThreadUnderflow.SetSize(1);
  m_ThreadOverflow.SetSize(1);
  m_ConcatChar = "__";

  for (int t=0; t < this->GetNumberOfThreads(); ++t)
  {
      std::vector<TablePointer> vT;
      m_VRAT.push_back(vT);
  }
}

/** Destructor */
template <class TImage>
RATBandMathImageFilter<TImage>
::~RATBandMathImageFilter()
{
}

template <class TImage>
void RATBandMathImageFilter<TImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Expression: "      << m_Expression                  << std::endl;
  os << indent << "Computed values follow:"                            << std::endl;
  os << indent << "UnderflowCount: "  << m_UnderflowCount              << std::endl;
  os << indent << "OverflowCount: "   << m_OverflowCount               << std::endl;
  os << indent << "itk::NumericTraits<PixelType>::NonpositiveMin()  :  " 
               << itk::NumericTraits<PixelType>::NonpositiveMin()      << std::endl;
  os << indent << "itk::NumericTraits<PixelType>::max()  :             " 
         << itk::NumericTraits<PixelType>::max()                 << std::endl;
}

template<class TImage>
void RATBandMathImageFilter<TImage>
::SetNbExpr(int numExpr)
 {
	if (numExpr <= 1)
		return;

	int e = this->GetNumberOfOutputs()-1;
	for(; e < numExpr; ++e)
	{
		typename TImage::Pointer output =
				static_cast<TImage*>(this->MakeOutput(e).GetPointer());
		this->SetNthOutput(e, output.GetPointer());
	}
	this->Modified();
 }

template<class TImage>
void RATBandMathImageFilter<TImage>
::SetNthAttributeTable(unsigned int idx, AttributeTable::Pointer tab,
		std::vector<std::string> vAttrNames	)
{
	// exclude invalid settings
	if (idx < 0)// || tab.IsNull() || tab.GetPointer() == 0)
	{
		std::cout << __FILE__ << ", line " << __LINE__ << ": WARNING:"
				<< " Invalid index for ::SetNthAttributeTable()!" << std::endl;
		return;
	}

    //    if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE)
    //    {
    //        itkExceptionMacro(<< "SQLiteTables are currently not supported! "
    //                          << "Please use ATTABLE_TYPE_RAM-based tables!");
    //        return;
    //    }

	std::vector<int> columns;
	std::vector<ColumnType> types;
	if (tab.IsNotNull())
	{
		int ncols = tab->GetNumCols();
		for (int n=0; n < vAttrNames.size(); ++n)
		{
			int c = -1;
			if ((c = tab->ColumnExists(vAttrNames.at(n))) >= 0)
			{
				if (tab->GetColumnType(c) == AttributeTable::ATTYPE_INT
					|| tab->GetColumnType(c) == AttributeTable::ATTYPE_DOUBLE)
				{
					columns.push_back(c);
					types.push_back(tab->GetColumnType(c));
				}
			}
		}
	}

    int nt = this->GetNumberOfThreads();

    for (int i=this->m_VRAT[0].size(); i < idx; ++i)
    {
        for (int t=0; t < nt; ++t)
        {
            this->m_VRAT[t].push_back(0);
        }

        std::vector<int> columns;
        this->m_VTabAttr.push_back(columns);
        std::vector<ColumnType> types;
        this->m_VAttrTypes.push_back(types);
    }

    if (this->m_VAttrTypes.size() == idx)
    {
        this->m_VRAT[0].push_back(tab);
        for (int t=1; t < nt; ++t)
        {
            if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_RAM)
            {
                this->m_VRAT[t].push_back(tab);
            }
            else
            {
                otb::SQLiteTable::Pointer stab = static_cast<otb::SQLiteTable*>(tab.GetPointer());
                otb::SQLiteTable::Pointer thtab = otb::SQLiteTable::New();
                thtab->SetUseSharedCache(false);
                if (thtab->CreateTable(stab->GetDbFileName(), "1") != otb::SQLiteTable::ATCREATE_READ)
                {
                    itkExceptionMacro(<< "Failed creating table connection for thread!");
                    return;
                }
                otb::AttributeTable::Pointer ctp = static_cast<otb::AttributeTable*>(thtab.GetPointer());
                this->m_VRAT[t].push_back(ctp);
            }
        }

        this->m_VAttrTypes.push_back(types);
        this->m_VTabAttr.push_back(columns);
    }
    else
    {
        this->m_VRAT[0][idx] = tab;
        for (int t=1; t < nt; ++t)
        {
            if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_RAM)
            {
                this->m_VRAT[t][idx] = tab;
            }
            else
            {
                otb::SQLiteTable::Pointer stab = static_cast<otb::SQLiteTable*>(tab.GetPointer());
                otb::SQLiteTable::Pointer thtab = otb::SQLiteTable::New();
                thtab->SetUseSharedCache(false);
                if (thtab->CreateTable(stab->GetDbFileName(), "1") != otb::SQLiteTable::ATCREATE_READ)
                {
                    itkExceptionMacro(<< "Failed creating table connection for thread!");
                    return;
                }
                this->m_VRAT[t][idx] = thtab;
            }
        }

        this->m_VAttrTypes[idx] = types;
        this->m_VTabAttr[idx] = columns;
    }

    if (m_UseTableColumnCache)
    {
        CacheTableColumns(idx);
    }

	this->Modified();
}

template<class TImage>
void RATBandMathImageFilter<TImage>
::ResetPipeline()
{
    m_TableColumnCache.clear();
}


template<class TImage>
void RATBandMathImageFilter<TImage>
::CacheTableColumns(int idx)
{
    if (idx < m_VRAT[0].size())
    {
        int t = idx;
        otb::AttributeTable::Pointer tab = m_VRAT[0].at(t);

        // there's no point in using a cache for an already cached
        // RAM-based table, really
        if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_RAM)
        {
            m_UseTableColumnCache = false;
            m_TableColumnCache.clear();
            return;
        }


        otb::SQLiteTable::Pointer dbtab = static_cast<otb::SQLiteTable*>(tab.GetPointer());

        std::map<int, std::map<long, double> > tab_map;
        std::vector<std::string> colnames;
        colnames.push_back(dbtab->GetPrimaryKey());

        for (int c=0; c < m_VTabAttr.at(t).size(); ++c)
        {
            std::string col_name = tab->GetColumnName(m_VTabAttr.at(t).at(c));
            if (col_name.compare(colnames[0]) != 0)
            {
                colnames.push_back(col_name);
            }

            std::map<long, double> col_map;
            tab_map.insert(std::pair<int, std::map<long, double> >(
                                                m_VTabAttr.at(t).at(c), col_map));
        }


        if (!dbtab->GreedyNumericFetch(colnames, tab_map))
        {
            m_UseTableColumnCache = false;
            m_TableColumnCache.clear();
            return;
        }


        for (int i=m_TableColumnCache.size(); i < t; ++i)
        {
            std::map<int, std::map<long, double> > tm;
            m_TableColumnCache.push_back(tm);
        }

        if (t < m_TableColumnCache.size())
        {
            m_TableColumnCache[t] = tab_map;
        }
        else if (t == m_TableColumnCache.size())
        {
            m_TableColumnCache.push_back(tab_map);
        }
    }
}

template<class TImage>
AttributeTable::Pointer RATBandMathImageFilter<TImage>
::GetNthAttributeTable(unsigned int idx)
{
    if (idx < 0 || idx >= m_VRAT[0].size())
		return 0;

    return m_VRAT[0][idx];
}

template<class TImage>
std::vector<std::string> RATBandMathImageFilter<TImage>
::GetNthTableAttributes(unsigned int idx)
{
	std::vector<std::string> ret;
	if (idx < 0 || idx >= m_VTabAttr.size())
	{
		return ret;
	}

	for (int i=0; i < m_VTabAttr[idx].size(); ++i)
        ret.push_back(m_VRAT[0][idx]->GetColumnName(m_VTabAttr[idx][i]));

	return ret;
}


template <class TImage>
void RATBandMathImageFilter<TImage>
::SetNthInput(unsigned int idx, const ImageType * image)
{  
    this->SetInput(idx, const_cast<TImage *>( image ));
    unsigned int nbInput = idx+1;
    while (nbInput < this->GetNumberOfInputs())
    {
        this->RemoveInput(this->GetNumberOfInputs()-1);
    }

    m_VVarName.resize(nbInput+4);
    std::ostringstream varName;
    varName << "b" << nbInput;
    m_VVarName[idx] = varName.str();
    m_VVarName[idx+1] = "idxX";
    m_VVarName[idx+2] = "idxY";
    m_VVarName[idx+3] = "idxPhyX";
    m_VVarName[idx+4] = "idxPhyY";


    // increase the RAT related vectors according to the
    // given idx, if necessary
    for (int i=this->m_VRAT[0].size(); i <= idx; ++i)
    {
        for (int t=0; t < this->GetNumberOfThreads(); ++t)
        {
            this->m_VRAT[t].push_back(0);
        }

        std::vector<int> columns;
        this->m_VTabAttr.push_back(columns);
        std::vector<ColumnType> types;
        this->m_VAttrTypes.push_back(types);
    }

    this->Modified();

}

template <class TImage>
void RATBandMathImageFilter<TImage>
::SetNthInput(unsigned int idx, const ImageType * image, const std::string& varName)
{
    this->SetInput(idx, const_cast<TImage *>( image ));
    unsigned int nbInput = idx+1;
    while (nbInput < this->GetNumberOfInputs())
    {
        this->RemoveInput(this->GetNumberOfInputs()-1);
    }

    m_VVarName.resize(nbInput);
    m_VVarName[idx] = varName;
    m_VVarName[idx+1] = "idxX";
    m_VVarName[idx+2] = "idxY";
    m_VVarName[idx+3] = "idxPhyX";
    m_VVarName[idx+4] = "idxPhyY";

    // increase the RAT related vectors according to the
    // given idx, if necessary
    for (int i=this->m_VRAT[0].size(); i <= idx; ++i)
    {
        for (int t=0; t < this->GetNumberOfThreads(); ++t)
        {
            this->m_VRAT[t].push_back(0);
        }
        std::vector<int> columns;
        this->m_VTabAttr.push_back(columns);
        std::vector<ColumnType> types;
        this->m_VAttrTypes.push_back(types);
    }

    this->Modified();
}

template <class TImage>
void RATBandMathImageFilter<TImage>
::SetNthInputName(unsigned int idx, const std::string& varName)
{
    m_UserNames.resize(idx+1);
    m_UserNames[idx] = varName;
    this->Modified();
}

template <typename TImage>
TImage * RATBandMathImageFilter<TImage>
::GetNthInput(unsigned int idx)
{
  return const_cast<TImage *>(this->GetInput(idx));
}

template< typename TImage >
void RATBandMathImageFilter<TImage>
::SetExpression(const std::string& expression)
{
  if (m_Expression != expression)
    m_Expression = expression;
  this->Modified();
}

template< typename TImage >
std::string RATBandMathImageFilter<TImage>
::GetExpression() const
{
  return m_Expression;
}

template< typename TImage >
std::string RATBandMathImageFilter<TImage>
::GetNthInputName(unsigned int idx) const
{
  return m_VVarName.at(idx);
}

template< typename TImage >
void RATBandMathImageFilter<TImage>
::BeforeThreadedGenerateData()
{
  typename std::vector<ParserType::Pointer>::iterator        itParser;
  typename std::vector< std::vector<PixelType> >::iterator   itVImage;
  //this->SetNumberOfThreads(1);
  unsigned int nbThreads = this->GetNumberOfThreads();
  //unsigned int nbInputImages = this->GetNumberOfInputs();

  unsigned int nbAccessIndex = 4; //to give access to image and physical index
  unsigned int nbInputImages = m_VVarName.size() - nbAccessIndex;
  if (nbInputImages > this->GetNumberOfInputs())
  {
      itkExceptionMacro(<< "The number of referenced images (" << nbInputImages << ")"
                        << "exceeds the number of actual input images (" << this->GetNumberOfInputs()
                        << ")!" << std::endl);
      itk::ExceptionObject e(__FILE__, __LINE__);
      e.SetLocation(ITK_LOCATION);
      e.SetDescription("Expression doesn't fit actual filter configuration!");
      throw e;
  }

  unsigned int i, j;
  unsigned int inputSize[2];
  std::vector< std::string > tmpIdxVarNames;

  tmpIdxVarNames.resize(nbAccessIndex);
  tmpIdxVarNames.at(0) = "idxX";
  tmpIdxVarNames.at(1) = "idxY";
  tmpIdxVarNames.at(2) = "idxPhyX";
  tmpIdxVarNames.at(3) = "idxPhyY";

  // Check if input image dimensions matches
  inputSize[0] = this->GetNthInput(0)->GetLargestPossibleRegion().GetSize(0);
  inputSize[1] = this->GetNthInput(0)->GetLargestPossibleRegion().GetSize(1);

  for(unsigned int p = 1; p < nbInputImages; p++)
    {
    if((inputSize[0] != this->GetNthInput(p)->GetLargestPossibleRegion().GetSize(0))
       || (inputSize[1] != this->GetNthInput(p)->GetLargestPossibleRegion().GetSize(1)))
      {
      itkExceptionMacro(<< "Input images must have the same dimensions." << std::endl
                        << "band #1 is [" << inputSize[0] << ";" << inputSize[1] << "]" << std::endl
                        << "band #" << p+1 << " is [" 
                        << this->GetNthInput(p)->GetLargestPossibleRegion().GetSize(0) << ";" 
                        << this->GetNthInput(p)->GetLargestPossibleRegion().GetSize(1) << "]");

      itk::ExceptionObject e(__FILE__, __LINE__);
      e.SetLocation(ITK_LOCATION);
      e.SetDescription("Input regions don't match in size!");
      throw e;
      }
    }

  // Store images specs
  m_Spacing = this->GetNthInput(0)->GetSpacing();
  m_Origin = this->GetNthInput(0)->GetOrigin();

  // Allocate and initialize the thread temporaries
  m_ThreadUnderflow.SetSize(nbThreads);
  m_ThreadUnderflow.Fill(0);
  m_ThreadOverflow.SetSize(nbThreads);
  m_ThreadOverflow.Fill(0);
  m_VParser.resize(nbThreads);
  m_AImage.resize(nbThreads);
  m_NbVar = nbInputImages+nbAccessIndex;
  //m_VVarName.resize(m_NbVar);

  // replace default varnames with user names, if applicable
  for (int n=0; n < m_UserNames.size(); ++n)
  {
      if (!m_UserNames.at(n).empty())
      {
          if (n < m_VVarName.size())
          {
              m_VVarName[n] = m_UserNames[n];
          }
      }
  }
  
  // attribute table support
  m_VAttrValues.resize(nbThreads);

  for(itParser = m_VParser.begin(); itParser < m_VParser.end(); ++itParser)
  {
    *itParser = ParserType::New();
  }

  for(i = 0; i < nbThreads; i++)
  {
    m_AImage.at(i).resize(m_NbVar);
    m_VAttrValues.at(i).resize(nbInputImages);
    m_VParser.at(i)->SetExpr(m_Expression);

//    // debug
//    if (i==0)
//    {
//    //std::cout << "Thread loop ----------------------------------------------" << std::endl << std::endl;
//    std::cout << "\t>>> no. of vars: " << m_NbVar << std::endl;
//    std::cout << "\t>>> parser expression: " << m_VParser[i]->GetExpr() << std::endl;
//    }
  
    //std::cout << "image loop ----------------------------------------------" << std::endl << std::endl;
    for(j=0; j < nbInputImages; j++)
    {
      m_VParser.at(i)->DefineVar(m_VVarName.at(j), &(m_AImage.at(i).at(j)));
//      if (i==0) std::cout << "img-name #" << j << ": " << m_VVarName[j] << std::endl;

      //attribute table support
      if (m_VRAT[0].size() > 0)
      {
          if (&m_VRAT[0][j] != 0)
    	  {
			  m_VAttrValues[i][j].resize(m_VTabAttr[j].size());
			  std::string bname = this->GetNthInputName(j) + m_ConcatChar;
			  for (int c=0; c < m_VTabAttr[j].size(); ++c)
			  {
                  std::string vname = bname + m_VRAT[0][j]->GetColumnName(m_VTabAttr[j][c]);
				  m_VParser[i]->DefineVar(vname, &(m_VAttrValues[i][j][c]));
			  }
    	  }
      }

    }
    //std::cout << std::endl;

    //std::cout << "image idx loop ----------------------------------------------" << std::endl << std::endl;
    for(j=nbInputImages; j < nbInputImages+nbAccessIndex; j++)
    {
      m_VVarName.at(j) = tmpIdxVarNames.at(j-nbInputImages);
      //std::cout << "set varname for img#" << j << " to " << m_VVarName[j] << std::endl;
      m_VParser.at(i)->DefineVar(m_VVarName.at(j), &(m_AImage.at(i).at(j)));
      //std::cout << "Parser #" << i << ": define var: " << m_VVarName[j] << " for img-idx #" << j << std::endl;
    }
    //std::cout << std::endl;
  }
}

template< typename TImage >
void RATBandMathImageFilter<TImage>
::AfterThreadedGenerateData()
{
  unsigned int nbThreads = this->GetNumberOfThreads();
  unsigned int i;
  
  m_UnderflowCount = 0;
  m_OverflowCount = 0;

  // Accumulate counts for each thread
  for(i = 0; i < nbThreads; i++)
    {
    m_UnderflowCount += m_ThreadUnderflow[i];
    m_OverflowCount += m_ThreadOverflow[i];
    }

  if((m_UnderflowCount != 0) || (m_OverflowCount!=0))
    otbWarningMacro(<< std::endl 
        << "The Following Parsed Expression  :  "
        << this->GetExpression()                                 << std::endl
        << "Generated " << m_UnderflowCount << " Underflow(s) " 
        << "And " << m_OverflowCount        << " Overflow(s) "   << std::endl
        << "The Parsed Expression, The Inputs And The Output "  
        << "Type May Be Incompatible !");        
}

template< typename TImage >
void RATBandMathImageFilter<TImage>
::ThreadedGenerateData(const ImageRegionType& outputRegionForThread,
           itk::ThreadIdType threadId)
{
	double* value;
	unsigned int j, r;
	unsigned int nbInputImages = this->GetNumberOfInputs();
	unsigned int nbOutputImages = this->GetNumberOfOutputs();

	typedef itk::ImageRegionConstIterator<TImage> ImageRegionConstIteratorType;
	typedef itk::ImageRegionIterator<TImage> ImageRegionIteratorType;

	/** Attribute table support */
	// create a vector indicating valid attribute table pointers
	std::vector<bool> vTabAvail;
	vTabAvail.resize(nbInputImages);
	for (unsigned int ii = 0; ii < nbInputImages; ii++)
	{
        if (m_VRAT[threadId].size() > 0)
		{
            if (&m_VRAT[threadId][ii] != 0)
			{
                if (m_VRAT[threadId][ii].IsNotNull())
					vTabAvail[ii] = true;
				else
					vTabAvail[ii] = false;
			}
		}
		else
			vTabAvail[ii] = false;
	}

	std::vector<ImageRegionConstIteratorType> Vit;
	Vit.resize(nbInputImages);
	for (j = 0; j < nbInputImages; j++)
	{
		Vit[j] = ImageRegionConstIteratorType(this->GetNthInput(j),
				outputRegionForThread);
	}

	std::vector<ImageRegionIteratorType> Vot;
	Vot.resize(nbOutputImages);
	for (r = 0; r < nbOutputImages; ++r)
	{
		Vot[r] = ImageRegionIteratorType(this->GetOutput(r),
				outputRegionForThread);
	}

	// support progress methods/callbacks
	itk::ProgressReporter progress(this, threadId,
			outputRegionForThread.GetNumberOfPixels());

	while (!Vit.at(0).IsAtEnd())
	{
		for (j = 0; j < nbInputImages; j++)
		{
			m_AImage.at(threadId).at(j) = static_cast<double>(Vit.at(j).Get());

			// raster attribute table support ......................................................................
			if (vTabAvail[j])
			{
                if (m_UseTableColumnCache)
                {
                    for (unsigned int c = 0; c < m_VTabAttr[j].size(); c++)
                    {
                        m_VAttrValues[threadId][j][c] =
                                m_TableColumnCache[j][m_VTabAttr[j][c]][static_cast<long>(Vit[j].Get())];
                    }
                }
                else
                {
                    for (unsigned int c = 0; c < m_VTabAttr[j].size(); c++)
                    {
                        switch (m_VAttrTypes[j][c])
                        {
                        case AttributeTable::ATTYPE_DOUBLE://2:
                            m_VAttrValues[threadId][j][c] =
                                    static_cast<double>(m_VRAT[threadId][j]->GetDblValue(
                                            m_VTabAttr[j][c], Vit[j].Get()));
                            break;
                        case AttributeTable::ATTYPE_INT:
                            m_VAttrValues[threadId][j][c] =
                                    static_cast<double>(m_VRAT[threadId][j]->GetIntValue(
                                            m_VTabAttr[j][c], Vit[j].Get()));
                            break;
                        }
                    }
                }
			}
			// .......................................................................................................

		}

		//// Image Indexes
        for (j = 0; j < 2; j++)
        {
            m_AImage.at(threadId).at(nbInputImages + j) =
                    static_cast<double>(Vit.at(0).GetIndex()[j]);
        }
        for (j = 0; j < 2; j++)
        {
            m_AImage.at(threadId).at(nbInputImages + 2 + j) =
                    static_cast<double>(m_Origin[j])
                            + static_cast<double>(Vit.at(0).GetIndex()[j])
                                    * static_cast<double>(m_Spacing[j]);
        }

		try
		{
			value = m_VParser.at(threadId)->Eval(this->m_NbExpr);
		}
		catch (itk::ExceptionObject& err)
		{
			throw err;
			itkExceptionMacro(<< err);
		}

		// Case value is equal to -inf or inferior to the minimum value
		// allowed by the pixelType cast
		for (r = 0; r < nbOutputImages; ++r)
		{
			if (value[r] < double(itk::NumericTraits<PixelType>::NonpositiveMin()))
			{
				Vot.at(r).Set(itk::NumericTraits<PixelType>::NonpositiveMin());
				m_ThreadUnderflow[threadId]++;
			}
			// Case value is equal to inf or superior to the maximum value
			// allowed by the pixelType cast
			else if (value[r] > double(itk::NumericTraits<PixelType>::max()))
			{
				Vot.at(r).Set(itk::NumericTraits<PixelType>::max());
				m_ThreadOverflow[threadId]++;
			}
			else
			{
				Vot.at(r).Set(static_cast<PixelType>(value[r]));
			}

			++(Vot.at(r));
		}

		for (j = 0; j < nbInputImages; j++)
		{
			++(Vit.at(j));
		}

		progress.CompletedPixel();
	}
}

}// end namespace otb

#endif
