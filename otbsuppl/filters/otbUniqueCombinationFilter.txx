/******************************************************************************
 * Created by Alexander Herzig
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

#ifndef __otbUniqueCombinationFilter_txx
#define __otbUniqueCombinationFilter_txx

#include "nmlog.h"
#include "otbUniqueCombinationFilter.h"
#include "otbCombineTwoFilter.h"
#include "otbNMImageReader.h"
#include "otbStreamingImageFileWriter.h"
#include "otbRATBandMathImageFilter.h"
#include "otbSQLiteTable.h"


#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkMacro.h"

//// TOKYO CABINET
////#include "tcutil.h"
//#include "tchdb.h"
//#include <stdlib.h>
//#include <stdbool.h>
//#include <stdint.h>

namespace otb
{

template< class TInputImage, class TOutputImage >
UniqueCombinationFilter< TInputImage, TOutputImage >
::UniqueCombinationFilter()
    : m_StreamingProc(false),
      m_DropTmpTables(true),
      m_UVTable(0),
      m_UVTableIndex(0),
      m_UVTableName(""),
      m_OutIdx(0)
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);

    m_UVTable = otb::SQLiteTable::New();
}

template< class TInputImage, class TOutputImage >
UniqueCombinationFilter< TInputImage, TOutputImage >
::~UniqueCombinationFilter()
{
    this->ResetPipeline();
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::SetNthInput(unsigned int idx, const TInputImage* image)
{
    this->SetInput(idx, const_cast<TInputImage *>( image ));
}

template< class TInputImage, class TOutputImage >
AttributeTable::Pointer
UniqueCombinationFilter< TInputImage, TOutputImage >
::getRAT(unsigned int idx)
{
    AttributeTable::Pointer tab = m_UVTable.GetPointer();
    return tab;
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::setRAT(unsigned int idx, AttributeTable::Pointer table)
{
    // the user is always right
    for (int i=this->m_vInRAT.size(); i <= idx; ++i)
    {
        this->m_vInRAT.push_back(0);
    }

    this->m_vInRAT[idx] = table;
}

//template< class TInputImage, class TOutputImage >
//AttributeTable::Pointer
//UniqueCombinationFilter< TInputImage, TOutputImage >
//::getRAT(unsigned int idx)
//{
//    if (idx < this->m_vOutRAT.size())
//    {
//        return this->m_vOutRAT.at(idx);
//    }
//    return 0;
//}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::SetUVTableName(const std::string& name)
{
    if (name.empty())
    {
        m_DropTmpTables = true;
    }
    else
    {
        m_DropTmpTables = false;
    }
    this->m_UVTableName = name;
    this->ResetPipeline();
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::InternalAllocateOutput()
{
    typename TInputImage::ConstPointer inImg = this->GetInput();
    typename TOutputImage::Pointer outImg = this->GetOutput();
    outImg->SetLargestPossibleRegion(inImg->GetLargestPossibleRegion());
    outImg->SetBufferedRegion(inImg->GetBufferedRegion());
    outImg->SetRequestedRegion(inImg->GetRequestedRegion());
    outImg->Allocate();

}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
//::BeforeThreadedGenerateData()
::GenerateData()
{
    //NMDebugCtx(ctx, << "...");

    unsigned int nbInputs = this->GetNumberOfIndexedInputs();
    unsigned int nbRAT = this->m_vInRAT.size();
    if (nbInputs < 2 || nbRAT < 2)
    {
        itkExceptionMacro(<< "Need at least two input layers to work!");
        return;
    }

    // ======================================================================
    // IMAGE AXES DIMENSION CHECK
    // ======================================================================

    // check, whether all inputs have the same dimension
    // (... and hopefully also the same projection - but
    // we don't check that ...)
    IndexType inputSize[2];
    inputSize[0] = this->GetInput(0)->GetLargestPossibleRegion().GetSize(0);
    inputSize[1] = this->GetInput(0)->GetLargestPossibleRegion().GetSize(1);

    // NMDebugAI(<< "nbInputs = " << nbInputs << std::endl);
    for (unsigned int p=0; p < nbInputs; ++p)
    {
        if((inputSize[0] != this->GetInput(p)->GetLargestPossibleRegion().GetSize(0))
           || (inputSize[1] != this->GetInput(p)->GetLargestPossibleRegion().GetSize(1)))
          {
          itkExceptionMacro(<< "Input images must have the same dimensions." << std::endl
                            << "image #1 is [" << inputSize[0] << ";" << inputSize[1] << "]" << std::endl
                            << "image #" << p+1 << " is ["
                            << this->GetInput(p)->GetLargestPossibleRegion().GetSize(0) << ";"
                            << this->GetInput(p)->GetLargestPossibleRegion().GetSize(1) << "]");

          itk::ExceptionObject e(__FILE__, __LINE__);
          e.SetLocation(ITK_LOCATION);
          e.SetDescription("Input regions don't match in size!");
          throw e;
          }
    }

    // ======================================================================
    // ALLOCATE THE OUTPUT IMAGE
    // ======================================================================
    typename TInputImage::ConstPointer inImg = this->GetInput();
    typename TOutputImage::Pointer outImg = this->GetOutput();
    this->InternalAllocateOutput();

    IndexType lprPixNum = inImg->GetLargestPossibleRegion().GetNumberOfPixels();
    typename TOutputImage::RegionType outRegion = outImg->GetBufferedRegion();


    /// here's what we do:
    /// - make a list of all intputs
    /// - make a list of combination iterations
    /// - according to the above iterate over internal pipeline
    ///   which does the following:
    ///   - take the first n images and combine them using otbCombineTwo
    ///     - set a name for the resulting table
    ///   - normalise the resulting image of the above:
    ///     use a another internal pipeline: reader->BandMath->writer
    ///     - use b_1__UvId as the output image values
    ///   - join the CombineTwo result table onto the new BandMath output
    ///
    ///   - take the next n additional images from the list and repeat the
    ///     iteration

    // set up objects for the first pipline
    typedef typename otb::RATBandMathImageFilter<TInputImage, TOutputImage> MathFilterType;
    typedef typename otb::CombineTwoFilter<TInputImage, TOutputImage> CombFilterType;
    typedef typename otb::NMImageReader<TInputImage> ReaderType;
    typedef typename otb::StreamingImageFileWriter<TOutputImage> WriterType;


    IndexType maxIdx = itk::NumericTraits<IndexType>::max();
    IndexType accIdx = static_cast<IndexType>(this->getRAT(0)->GetNumRows());
    unsigned int cnt = 0;
    while (   cnt+1 < nbRAT
           && (accIdx > maxIdx / (this->getRAT(cnt+1) > 0 ? this->getRAT(cnt+1) : 1))
          )
    {
        accIdx *= this->getRAT(cnt);
        ++cnt;
    }

    int fstImg = 0;
    int lastImg = cnt;
    while (lastImg+1 < nbRAT)
    {
        CombFilterType::Pointer ctFilter = CombFilterType::New();
        for (int i=fstImg; i <= lastImg; ++i)
        {
            ctFilter->SetInput(i, this->GetInput(i));
            ctFilter->setRAT(i, this->getRAT(i));
        }

    }


    //NMDebugCtx(ctx, << "done!");
}

template< class TInputImage, class TOutputImage >
std::string
UniqueCombinationFilter< TInputImage, TOutputImage >
::getRandomString(int length)
{
    int len = length;
    if (len < 8)
        len = 8;

    std::srand(std::time(0));
    char nam[len];
    for (int i=0; i < len; ++i)
    {
        if (i == 0)
        {
            if (::rand() % 2 == 0)
            {
                nam[i] = ::rand() % 26 + 65;
            }
            else
            {
                nam[i] = ::rand() % 26 + 97;
            }
        }
        else
        {
            if (::rand() % 7 == 0)
            {
                nam[i] = '_';
            }
            else if (::rand() % 5 == 0)
            {
                nam[i] = ::rand() % 26 + 65;
            }
            else if (::rand() % 3 == 0)
            {
                nam[i] = ::rand() % 26 + 97;
            }
            else
            {
                nam[i] = ::rand() % 10 + 48;
            }
        }
    }

    std::string ret = nam;

    return ret;
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
    NMDebugCtx(ctx, << "...");

    m_StreamingProc = false;

    if (this->m_UVTable.IsNotNull())
    {
        if (m_DropTmpTables)
        {
            this->m_UVTable->CloseTable(true);
        }
        else
        {
            this->m_UVTable->CloseTable(false);
        }
    }
    m_UVTable = 0;
    m_UVTable = SQLiteTable::New();
    m_OutIdx = 0;
    m_UVTableIndex = 0;

    Superclass::ResetPipeline();
    NMDebugCtx(ctx, << "done!")
}


}   // end namespace otb

#endif /* __otbUniqueCombinationFilter_txx */
