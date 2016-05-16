 /****************************************************************************** 
 * Created by Alexander Herzig 
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
/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkNMScriptableKernelFilter.txx,v $
  Language:  C++
  Date:      $Date: 2008-10-16 18:05:25 $
  Version:   $Revision: 1.16 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __otbNMScriptableKernelFilter_txx
#define __otbNMScriptableKernelFilter_txx


// First make sure that the configuration is available.
// This line can be removed once the optimized versions
// gets integrated into the main directories.
//#include "itkConfigure.h"
//
//#ifdef ITK_USE_CONSOLIDATED_MORPHOLOGY
//#include "itkOptNMScriptableKernelFilter.txx"
//#else

#include "otbNMScriptableKernelFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"
#include "itkOffset.h"
#include "itkProgressReporter.h"

#include "otbAttributeTable.h"

namespace otb
{

template <class TInputImage, class TOutputImage>
NMScriptableKernelFilter<TInputImage, TOutputImage>
::NMScriptableKernelFilter()
{
    m_Radius.Fill(1);
    m_KernelShape = "Square";

    m_PixelCounter = 0;
}

template <class TInputImage, class TOutputImage>
NMScriptableKernelFilter<TInputImage, TOutputImage>
::~NMScriptableKernelFilter()
{
    this->Reset();
}

template <class TInputImage, class TOutputImage>
NMScriptableKernelFilter<TInputImage, TOutputImage>
::Reset()
{
    m_PixelCounter = 0;
    for (int v=0; v < m_vecParsers.size(); ++v)
    {
        for (int p=0; p < m_vecParsers.at(v).size(); ++p)
        {
            delete m_vecParsers.at(v).at(p);
        }
        m_vecParsers.at(v).clear();
    }
    m_vecParsers.clear();

    m_mapParserName.clear();
    m_mapNameImgValue.clear();
    m_mapNameAuxValue.clear();
    m_vecBlockLen.clear();
}


template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter<TInputImage, TOutputImage>
::SetInputNames(const std::vector<std::string> &inputNames)
{
    m_DataNames.clear();
    for (int n=0; n < inputNames.size(); ++n)
    {
        m_DataNames.push_back(inputNames.at(n));
    }
}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter<TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{
    // update input data
    this->CacheInputData();

    // parse the script


}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter<TInputImage, TOutputImage>
::CacheInputData()
{
    for (int n=0; n < m_DataNames.size(); ++n)
    {
        const std::string& name = m_DataNames.at(n);

        if (n < this->GetNumberOfIndexedInputs())
        {
            itk::DataObject* dataObject = this->GetIndexedInputs().at(n).GetPointer();
            AttributeTable* tab = static_cast<otb::AttributeTable*>(dataObject);
            TInputImage* img = static_cast<TInputImage*>(dataObject);

            // if we've got a table here, we haven't dealt with, we
            // just copy the content into a matrix type mup::Value
            if (tab != 0 && m_mapNameAuxValue.find(name) == m_mapNameAuxValue.end())
            {
                int ncols = tab->GetNumCols();
                int nrows = tab->GetNumRows();

                // note: access is rows, columns
                mup::Value tabValue(nrows, ncols);

                for (int row = 0; row < nrows; ++row)
                {
                    for (int col=0; col < ncols; ++coll)
                    {
                        switch(tab->GetColumnType(col))
                        {
                        case AttributeTable::ATTYPE_INT:
                            tabValue.At(row, col) = tab->GetIntValue(col, row);
                            break;
                        case AttributeTable::ATTYPE_DOUBLE:
                            tabValue.At(row, col) = tab->GetDblValue(col, row);
                            break;
                        case AttributeTable::ATTYPE_STRING:
                            tabValue.At(row, col) = tab->GetStrValue(col, row);
                            break;
                        }
                    }
                }

                m_mapNameAuxValue.insert(std::pair<std::string, mup::Value>(name, tabValue));
            }

            // for images, we just prepare the map and put some placeholders in
            // we'll redefine once we know the actual values
            else if (img != 0)
            {
                if (m_mapNameImgValue.size() > 0)
                {
                    if (m_mapNameImgValue.at(0).find(name) == m_mapNameImgValue.at(0).end())
                    {
                        for (int th=0; th < m_NumberOfThreads; ++th)
                        {
                            m_mapNameImgValue.at(th).insert(
                                        std::pair<std::string, mup::Value>(name, mup::Value()));
                        }
                    }
                }
                else
                {
                    for (int th=0; th < m_NumberOfThreads; ++th)
                    {
                        std::map<std::string, mup::Value> thMap;
                        thMap.insert(std::pair<std::string, mup::Value>(name, mup::Value()));
                        m_mapNameImgValue.push_back(thMap);
                    }
                }
            }
        }
    }
}

template <class TInputImage, class TOutputImage>
void 
NMScriptableKernelFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion() throw (itk::InvalidRequestedRegionError)
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();
  
  // get pointers to the input and output
  typename Superclass::InputImagePointer inputPtr = 
    const_cast< TInputImage * >( this->GetInput() );
  typename Superclass::OutputImagePointer outputPtr = this->GetOutput();
  
  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  // get a copy of the input requested region (should equal the output
  // requested region)
  typename TInputImage::RegionType inputRequestedRegion;
  inputRequestedRegion = inputPtr->GetRequestedRegion();

  // pad the input requested region by the operator radius
  inputRequestedRegion.PadByRadius( m_Radius );

  // crop the input requested region at the input's largest possible region
  if ( inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()) )
    {
    inputPtr->SetRequestedRegion( inputRequestedRegion );
    return;
    }
  else
    {
    // Couldn't crop the region (requested region is outside the largest
    // possible region).  Throw an exception.

    // store what we tried to request (prior to trying to crop)
    inputPtr->SetRequestedRegion( inputRequestedRegion );
    
    // build an exception
    itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
    e.SetLocation(ITK_LOCATION);
    e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
    e.SetDataObject(inputPtr);
    throw e;
    }
}


template< class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter< TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId)
{
  unsigned int i;
  itk::ZeroFluxNeumannBoundaryCondition<InputImageType> nbc;

  itk::ConstNeighborhoodIterator<InputImageType> bit;
  itk::ImageRegionIterator<OutputImageType> it;
  
  // Allocate output
  typename OutputImageType::Pointer output = this->GetOutput();
  typename  InputImageType::ConstPointer input  = this->GetInput();
  
  // Find the data-set boundary "faces"
  typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>::FaceListType faceList;
  itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType> bC;
  faceList = bC(input, outputRegionForThread, m_Radius);

  typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>::FaceListType::iterator fit;

  // support progress methods/callbacks
  itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());
  
  int count;
  int val;

  // Process each of the boundary faces.  These are N-d regions which border
  // the edge of the buffer.
  for (fit=faceList.begin(); fit != faceList.end(); ++fit)
    { 
    bit = itk::ConstNeighborhoodIterator<InputImageType>(m_Radius,
                                                    input, *fit);
    unsigned int neighborhoodSize = bit.Size();
    it = itk::ImageRegionIterator<OutputImageType>(output, *fit);
    bit.OverrideBoundaryCondition(&nbc);
    bit.GoToBegin();

    while ( ! bit.IsAtEnd() && !this->GetAbortGenerateData())
      {
      
      ++bit;
      ++it;
      progress.CompletedPixel();
      }
    }
}

/**
 * Standard "PrintSelf" method
 */
template <class TInputImage, class TOutput>
void
NMScriptableKernelFilter<TInputImage, TOutput>
::PrintSelf(
  std::ostream& os, 
  itk::Indent indent) const
{
  Superclass::PrintSelf( os, indent );
  os << indent << "Radius:    " << m_Radius << std::endl;
}

} // end namespace otb

//#endif

#endif
