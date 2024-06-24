 /******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd
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
/*
 * Copyright (C) 2005-2020 Centre National d'Etudes Spatiales (CNES)
 *
 * This file is part of Orfeo Toolbox
 *
 *     https://www.orfeo-toolbox.org/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * NMStreamingImageFileWriterWrapper.cpp
 *
 *  Created on: 17/05/2012
 *      Author: alex
 *
 *  ... (many more updates in between)
 *
 *  Updated on: 10/01/2022
 *      Author: Alex Herzig
 *
 *  - added parallel IO
 *
 */



#include "NMMacros.h"
#include "NMStreamingImageFileWriterWrapper.h"

#include <QFileInfo>
#include <QDir>
#include <QFile>

#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkSmartPointer.h"
#include "itkProcessObject.h"
#include "otbAttributeTable.h"
#include "otbStreamingRATImageFileWriter.h"
//#include "itkNMImageRegionSplitterMaxSize.h"
//#include "otbImageRegionTileMapSplitter.h"
#include "itkImageRegionSplitter.h"
#include "NMModelController.h"
#include "NMMfwException.h"

#ifdef BUILD_RASSUPPORT
    #include "RasdamanConnector.hh"
    #include "otbRasdamanImageIO.h"
#endif

#ifndef _WIN32
#   include <mpi.h>
#endif


/** Helper Classes */
template <class PixelType, class PixelType2, unsigned int Dimension>
class NMStreamingImageFileWriterWrapper_Internal
{
public:

    typedef itk::RGBPixel< PixelType >                  RGBPixelType;
    typedef otb::Image< RGBPixelType, Dimension >       RGBImgType;

    typedef otb::Image<PixelType, Dimension> 			ImgType;
    typedef otb::VectorImage<PixelType, Dimension> 		VecImgType;

    typedef otb::StreamingRATImageFileWriter<RGBImgType>    RGBFilterType;
    typedef otb::StreamingRATImageFileWriter<ImgType> 		FilterType;
    typedef otb::StreamingRATImageFileWriter<VecImgType> 	VecFilterType;

    typedef typename RGBFilterType::Pointer             RGBFilterTypePointer;
    typedef typename FilterType::Pointer 				FilterTypePointer;
    typedef typename VecFilterType::Pointer 			VecFilterTypePointer;

    //using SplitterType = otb::ImageRegionTileMapSplitter<Dimension>;
    using SplitterType = itk::ImageRegionSplitter<Dimension>;

    static void createInstance(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterTypePointer f = FilterType::New();
                otbFilter = f;
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterTypePointer r = RGBFilterType::New();
                otbFilter = r;
            }
            else
            {
                VecFilterTypePointer v = VecFilterType::New();
                otbFilter = v;
            }
        }

    static void setUpdateMode(itk::ProcessObject::Pointer& otbFilter,
                              unsigned int numBands, bool updateMode, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetUpdateMode(updateMode);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetUpdateMode(updateMode);
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetUpdateMode(updateMode);
            }
        }

    static void setParallelIO(itk::ProcessObject::Pointer& otbFilter,
                              unsigned int numBands, bool parallelIO, bool rgbMode, MPI_Comm comm)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetParallelIO(parallelIO);
                filter->SetMpiComm(comm);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetParallelIO(parallelIO);
                filter->SetMpiComm(comm);
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetParallelIO(parallelIO);
                filter->SetMpiComm(comm);
            }
        }

    static void ParallelIOUpdate(itk::ProcessObject::Pointer& otbFilter,
                                 unsigned int numBands, bool rgbMode,
                                 int nprocs, int rank, MPI_Comm comm)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->UpdateOutputInformation();

                ImgType* img = filter->GetOutput(0);
                typename ImgType::RegionType lpr = img->GetLargestPossibleRegion();
                itk::ImageIORegion fior(ImgType::ImageDimension);
                for (int d=0; d < ImgType::ImageDimension; ++d)
                {
                    fior.SetIndex(d, lpr.GetIndex()[d]);
                    fior.SetSize(d, lpr.GetSize()[d]);
                }
                filter->SetForcedLargestPossibleRegion(fior);

                typename SplitterType::Pointer splitter = SplitterType::New();

                itk::ImageIORegion pioRegion;
                itk::ImageIORegion nullRegion;

                typename ImgType::RegionType curSplitRegion;


                std::vector<itk::ImageIORegion> paraRegions;

                int numSplits = splitter->GetNumberOfSplits(lpr, nprocs);
                int subtract=1;
                while (numSplits > nprocs)
                {
                    numSplits = splitter->GetNumberOfSplits(lpr, (nprocs-subtract));
                    ++subtract;
                }


                unsigned long totalNumPix = lpr.GetNumberOfPixels();
                unsigned long pixPerRegion = totalNumPix / numSplits;

                if (rank == 0)
                {
                    NMDebugAI(<< std::endl << ">>>>>> ParallelIOUPdate <<<<<<<<<<<<" << std::endl
                              << "totalNumPix=" << totalNumPix << " pixPerRegion=" << pixPerRegion
                              << " numSplits=" << numSplits << std::endl);
                }

                NMDebugAI(<< "proc#" << rank << " arriving at barrier ... " << std::endl);
                MPI_Barrier(comm);
                NMDebugAI(<< "proc#" << rank << " stepped past the barrier!" << std::endl);

                int ri=0, sp=0;
                for (; sp < numSplits; ++sp, ++ri)
                {
                    if (ri >= nprocs)
                    {
                        ri = 0;
                    }

                    // init ImageIORegion and ImageRegion to largest possible region
                    pioRegion = fior;
                    curSplitRegion = lpr;

                    // populate ImageRegion with current image split
                    curSplitRegion = splitter->GetSplit(sp, numSplits, lpr);

                    // copy ImageRegion to ImageIORegion
                    // (required by Writer::SetUpdateRegion)
                    for (int d=0; d < Dimension; ++d)
                    {
                        pioRegion.SetIndex(d, curSplitRegion.GetIndex()[d]);
                        pioRegion.SetSize(d, curSplitRegion.GetSize()[d]);
                    }

                    paraRegions.push_back(pioRegion);

                    if (rank == 0)
                    {
                        NMDebugAI(<< "sp=" << sp << " ri=" << ri << " : "
                                  << pioRegion.GetIndex()[0] << "," << pioRegion.GetIndex()[1]
                                  << " " << pioRegion.GetSize()[0] << "," << pioRegion.GetSize()[1]
                                  << std::endl);
                    }
                }

                for (int round=0; round < paraRegions.size(); ++round)
                {
                    if (round == rank)
                    {
                        filter->SetUpdateRegion(paraRegions[round]);
                        filter->Update();
                    }
                }

            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                // not implemented for now
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                // not implemented for now
            }
        }

    static void setFileNames(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, std::vector<std::string> vecFileNames,

#ifdef BUILD_RASSUPPORT

            otb::RasdamanImageIO::Pointer& rio,
#endif
            bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetFileNames(vecFileNames);

#ifdef BUILD_RASSUPPORT
                if (rio.IsNotNull())
                    filter->SetImageIO(rio);
#endif
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetFileNames(vecFileNames);

#ifdef BUILD_RASSUPPORT
                if (rio.IsNotNull())
                    filter->SetImageIO(rio);
#endif
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetFileNames(vecFileNames);

#ifdef BUILD_RASSUPPORT
                if (rio.IsNotNull())
                    filter->SetImageIO(rio);
#endif
                }
        }

    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx, itk::DataObject* dataObj, bool rgbMode, bool writeImage)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                ImgType* img = dynamic_cast<ImgType*>(dataObj);
                filter->SetInput(idx, img);
                filter->SetWriteImage(writeImage);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                RGBImgType* img = dynamic_cast<RGBImgType*>(dataObj);
                filter->SetInput(idx, img);
                filter->SetWriteImage(writeImage);
            }
            else
            {
                VecImgType* img = dynamic_cast<VecImgType*>(dataObj);
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetInput(idx, img);
                filter->SetWriteImage(writeImage);
            }
        }

    static void setNthAttributeTable(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx, otb::AttributeTable::Pointer& tab, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetInputRAT(tab, idx);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetInputRAT(tab, idx);
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetInputRAT(tab, idx);
            }
        }

    static void setResamplingType(itk::ProcessObject::Pointer& otbFilter,
                                  unsigned int numBands, const QString& resamplingType, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetResamplingType(resamplingType.toStdString());
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetResamplingType(resamplingType.toStdString());
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetResamplingType(resamplingType.toStdString());
            }
        }

    static void setStreamingMethod(itk::ProcessObject::Pointer& otbFilter,
                                  unsigned int numBands, const QString& StreamingMethod, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetStreamingMethod(StreamingMethod.toStdString());
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetStreamingMethod(StreamingMethod.toStdString());
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetStreamingMethod(StreamingMethod.toStdString());
            }
        }

    static void setStreamingSize(itk::ProcessObject::Pointer& otbFilter,
                                  unsigned int numBands, const int streamSize, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetStreamingSize(streamSize);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetStreamingSize(streamSize);
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetStreamingSize(streamSize);
            }
        }

    static void setForcedLPR(itk::ProcessObject::Pointer& otbFilter,
                                  unsigned int numBands, itk::ImageIORegion& ior, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetForcedLargestPossibleRegion(ior);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetForcedLargestPossibleRegion(ior);
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetForcedLargestPossibleRegion(ior);
            }
        }

    static void setUpdateRegion(itk::ProcessObject::Pointer& otbFilter,
                                  unsigned int numBands, itk::ImageIORegion& ior, bool rgbMode)
        {
            if (numBands == 1)
            {
                FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                filter->SetUpdateRegion(ior);
            }
            else if (numBands == 3 && rgbMode)
            {
                RGBFilterType* filter = dynamic_cast<RGBFilterType*>(otbFilter.GetPointer());
                filter->SetUpdateRegion(ior);
            }
            else
            {
                VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
                filter->SetUpdateRegion(ior);
            }
        }
};

#ifdef BUILD_RASSUPPORT
    #define callSetFileName( PixelType, wrapName ) \
        if (this->mOutputNumDimensions == 1)                                    \
        {                                                                   \
            wrapName<PixelType, PixelType, 1>::setFileNames(this->mOtbProcess,   \
                    this->mOutputNumBands, vecFileNames,rio, mRGBMode);                  \
        }                                                                   \
        else if (this->mOutputNumDimensions == 2)                               \
        {                                                                   \
            wrapName<PixelType, PixelType, 2>::setFileNames(this->mOtbProcess,   \
                    this->mOutputNumBands, vecFileNames,rio, mRGBMode);                  \
        }                                                                   \
        else if (this->mOutputNumDimensions == 3)                               \
        {                                                                   \
            wrapName<PixelType, PixelType, 3>::setFileNames(this->mOtbProcess,   \
                    this->mOutputNumBands, vecFileNames,rio, mRGBMode);                  \
        }
#else
    #define callSetFileName( PixelType, wrapName ) \
        if (this->mOutputNumDimensions == 1)                                    \
        {                                                                   \
            wrapName<PixelType, PixelType, 1>::setFileNames(this->mOtbProcess,   \
                    this->mOutputNumBands, vecFileNames,mRGBMode);                  \
        }                                                                   \
        else if (this->mOutputNumDimensions == 2)                               \
        {                                                                   \
            wrapName<PixelType, PixelType, 2>::setFileNames(this->mOtbProcess,   \
                    this->mOutputNumBands, vecFileNames,mRGBMode);                  \
        }                                                                   \
        else if (this->mOutputNumDimensions == 3)                               \
        {                                                                   \
            wrapName<PixelType, PixelType, 3>::setFileNames(this->mOtbProcess,   \
                    this->mOutputNumBands, vecFileNames,mRGBMode);                  \
        }
#endif // BUILD_RASSUPPORT

#define callOutputTypeSetTab( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setNthAttributeTable( \
                this->mOtbProcess, this->mOutputNumBands, tabIdx, tab, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setNthAttributeTable( \
                this->mOtbProcess, this->mOutputNumBands, tabIdx, tab, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setNthAttributeTable( \
                this->mOtbProcess, this->mOutputNumBands, tabIdx, tab, mRGBMode); \
    }\
}

#define callFilterSetInput( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setNthInput( \
                this->mOtbProcess, this->mOutputNumBands, numInput, img, mRGBMode, mWriteImage); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setNthInput( \
                this->mOtbProcess, this->mOutputNumBands, numInput, img, mRGBMode, mWriteImage); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setNthInput( \
                this->mOtbProcess, this->mOutputNumBands, numInput, img, mRGBMode, mWriteImage); \
    }\
}

#define callSetUpdateMode( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setUpdateMode( \
                this->mOtbProcess, this->mOutputNumBands, this->mUpdateMode, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setUpdateMode( \
                this->mOtbProcess, this->mOutputNumBands, this->mUpdateMode, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setUpdateMode( \
                this->mOtbProcess, this->mOutputNumBands, this->mUpdateMode, mRGBMode); \
    }\
}

#define callSetParallelIO( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setParallelIO( \
                this->mOtbProcess, this->mOutputNumBands, this->mParallelIO, mRGBMode, comm); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setParallelIO( \
                this->mOtbProcess, this->mOutputNumBands, this->mParallelIO, mRGBMode, comm); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setParallelIO( \
                this->mOtbProcess, this->mOutputNumBands, this->mParallelIO, mRGBMode, comm); \
    }\
}


#define callParallelIOUpdate( imgType, wrapName ) \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::ParallelIOUpdate( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode, nprocs, rank, comm); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::ParallelIOUpdate( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode, nprocs, rank, comm); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::ParallelIOUpdate( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode, nprocs, rank, comm); \
    }\

#define callSetResamplingType( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setResamplingType( \
                this->mOtbProcess, this->mOutputNumBands, mPyramidResamplingType, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setResamplingType( \
                this->mOtbProcess, this->mOutputNumBands, mPyramidResamplingType, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setResamplingType( \
                this->mOtbProcess, this->mOutputNumBands, mPyramidResamplingType, mRGBMode); \
    }\
}

#define callSetStreamingMethod( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setStreamingMethod( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingMethodType, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setStreamingMethod( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingMethodType, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setStreamingMethod( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingMethodType, mRGBMode); \
    }\
}

#define callSetStreamingSize( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setStreamingSize( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingSize, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setStreamingSize( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingSize, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setStreamingSize( \
                this->mOtbProcess, this->mOutputNumBands, mStreamingSize, mRGBMode); \
    }\
}

#define callSetForcedLPR( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setForcedLPR( \
                this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setForcedLPR( \
                this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setForcedLPR( \
                this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
    }\
}

#define callSetUpdateRegion( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::setUpdateRegion( \
                this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::setUpdateRegion( \
                this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::setUpdateRegion( \
                this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
    }\
}

#define callInitWriter( imgType, wrapName ) \
{ \
    if (this->mOutputNumDimensions == 1) \
    { \
        wrapName< imgType, imgType, 1 >::createInstance( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 2) \
    { \
        wrapName< imgType, imgType, 2 >::createInstance( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode); \
    } \
    else if (this->mOutputNumDimensions == 3) \
    { \
        wrapName< imgType, imgType, 3 >::createInstance( \
                this->mOtbProcess, this->mOutputNumBands, mRGBMode); \
    }\
}

//InstantiateObjectWrap( NMStreamingImageFileWriterWrapper, NMStreamingImageFileWriterWrapper_Internal )
//SetNthInputWrap( NMStreamingImageFileWriterWrapper, NMStreamingImageFileWriterWrapper_Internal )



NMStreamingImageFileWriterWrapper
::NMStreamingImageFileWriterWrapper(QObject* parent)
{
    this->setParent(parent);
    this->setObjectName(tr("NMStreamingImageFileWriterWrapper"));
    this->mbIsInitialised = false;
    this->mInputComponentType = otb::ImageIOBase::FLOAT;
    this->mOutputComponentType = otb::ImageIOBase::FLOAT;
    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;
    this->mInputNumDimensions = 2;
    this->mOutputNumDimensions = 2;
    this->mWriteTable = true;
    this->mWriteImage = true;
    this->mUpdateMode = false;
    this->mRGBMode = false;
    this->mParallelIO = false;

    this->mStreamingSize = 512;
    this->mWriteProcs = 1;

    this->mPyramidResamplingType = QString(tr("NEAREST"));
    mPyramidResamplingEnum.clear();
    mPyramidResamplingEnum
            << "NONE" << "NEAREST" << "GAUSS" << "CUBIC"
            << "AVERAGE" << "MODE";

    this->mStreamingMethodType = QString(tr("STRIPPED"));
    this->mStreamingMethodEnum.clear();
    this->mStreamingMethodEnum << "STRIPPED" << "TILED" << "NO_STREAMING";

    mbUseForcedLPR = false;
    mbUseUpdateRegion = false;

    mForcedLPR = {0,0,0,0,0,0};
    mUpdateRegion = {0,0,0,0,0,0};

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("PixelType"));
    mUserProperties.insert(QStringLiteral("OutputNumDimensions"), QStringLiteral("NumDimensions"));
    mUserProperties.insert(QStringLiteral("OutputNumBands"), QStringLiteral("NumBands"));
    mUserProperties.insert(QStringLiteral("FileNames"), QStringLiteral("FileNames"));
    mUserProperties.insert(QStringLiteral("InputTables"), QStringLiteral("InputTables"));
    mUserProperties.insert(QStringLiteral("RGBMode"), QStringLiteral("RGBMode"));
    mUserProperties.insert(QStringLiteral("WriteImage"), QStringLiteral("WriteImage"));
    mUserProperties.insert(QStringLiteral("WriteTable"), QStringLiteral("WriteTable"));
    mUserProperties.insert(QStringLiteral("StreamingMethodType"), QStringLiteral("StreamingMethod"));
    mUserProperties.insert(QStringLiteral("StreamingSize"), QStringLiteral("PipelineMemoryFootprint"));
    mUserProperties.insert(QStringLiteral("PyramidResamplingType"), QStringLiteral("PyramidResampling"));
    //mUserProperties.insert(QStringLiteral("ParallelIO"), QStringLiteral("ParallelIO"));
    mUserProperties.insert(QStringLiteral("WriteProcs"), QStringLiteral("WriteProcs"));

#ifdef BUILD_RASSUPPORT
    this->mRasConnector = 0;
    this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mParamPos = 0;
#endif
}

NMStreamingImageFileWriterWrapper
::NMStreamingImageFileWriterWrapper(QObject* parent,
            otb::ImageIOBase::IOComponentType componentType,
            unsigned int numDims, unsigned int numBands)
{
    this->setParent(parent);
    this->setObjectName(tr("NMStreamingImageFileWriterWrapper"));
    this->mbIsInitialised = false;
    this->mInputComponentType = componentType;
    this->mOutputComponentType = componentType;
    this->mInputNumBands = numBands;
    this->mOutputNumBands = numBands;
    this->mInputNumDimensions = numDims;
    this->mOutputNumDimensions = numDims;
    this->mWriteImage = true;
    this->mWriteTable = true;
    this->mUpdateMode = false;
    this->mRGBMode = false;
    this->mParallelIO = false;

    this->mStreamingSize = 512;
    this->mWriteProcs = 1;

    this->mPyramidResamplingType = QString(tr("NEAREST"));
    mPyramidResamplingEnum.clear();
    mPyramidResamplingEnum
            << "NONE" << "NEAREST" << "GAUSS" << "CUBIC"
            << "AVERAGE" << "MODE";

    this->mStreamingMethodType = QString(tr("STRIPPED"));
    this->mStreamingMethodEnum.clear();
    this->mStreamingMethodEnum << "STRIPPED" << "TILED" << "NO_STREAMING";

    mbUseForcedLPR = false;
    mbUseUpdateRegion = false;

    mForcedLPR = {0,0,0,0,0,0};
    mUpdateRegion = {0,0,0,0,0,0};

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("PixelType"));
    mUserProperties.insert(QStringLiteral("OutputNumDimensions"), QStringLiteral("NumDimensions"));
    mUserProperties.insert(QStringLiteral("OutputNumBands"), QStringLiteral("NumBands"));
    mUserProperties.insert(QStringLiteral("FileNames"), QStringLiteral("FileNames"));
    mUserProperties.insert(QStringLiteral("InputTables"), QStringLiteral("InputTables"));
    mUserProperties.insert(QStringLiteral("RGBMode"), QStringLiteral("RGBMode"));
    mUserProperties.insert(QStringLiteral("WriteImage"), QStringLiteral("WriteImage"));
    mUserProperties.insert(QStringLiteral("WriteTable"), QStringLiteral("WriteTable"));
    mUserProperties.insert(QStringLiteral("StreamingMethodType"), QStringLiteral("StreamingMethod"));
    mUserProperties.insert(QStringLiteral("StreamingSize"), QStringLiteral("PipelineMemoryFootprint"));
    mUserProperties.insert(QStringLiteral("PyramidResamplingType"), QStringLiteral("PyramidResampling"));
    //mUserProperties.insert(QStringLiteral("ParallelIO"), QStringLiteral("ParallelIO"));
    mUserProperties.insert(QStringLiteral("WriteProcs"), QStringLiteral("WriteProcs"));


#ifdef BUILD_RASSUPPORT
    this->mRasConnector = 0;
    this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mParamPos = 0;
#endif
}

NMStreamingImageFileWriterWrapper::~NMStreamingImageFileWriterWrapper()
{
}

void NMStreamingImageFileWriterWrapper
::setWriteProcs(int procs)
{
    if (procs >= 1)
    {
        this->mWriteProcs = procs;
    }
    else
    {
        this->mWriteProcs = 1;
    }

    emit nmChanged();
}

void
NMStreamingImageFileWriterWrapper
::setInternalFileNames(QStringList fileNames)
{
    if (!this->mbIsInitialised)
        return;

#ifdef BUILD_RASSUPPORT
    otb::RasdamanImageIO::Pointer rio;
    if (mRasConnector != 0 && this->mRasConnector->getConnector() != 0)
    {
        rio = otb::RasdamanImageIO::New();
        rio->setRasdamanConnector(const_cast<RasdamanConnector*>(
                this->mRasConnector->getConnector()));
    }
#endif

    std::vector<std::string> vecFileNames;
    foreach(const QString& fn, fileNames)
    {
        vecFileNames.push_back(fn.toStdString());
    }

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetFileName, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::instantiateObject(void)
{
    if (this->mbIsInitialised)
        return;

    bool init = true;
    switch(this->mOutputComponentType)
    {
    MacroPerType( callInitWriter, NMStreamingImageFileWriterWrapper_Internal )
    default:
        init = false;
        break;
    }
    this->mbIsInitialised = init;
    NMDebugAI( << "... " << this->objectName().toStdString() << " - " << init << std::endl);
}

void
NMStreamingImageFileWriterWrapper
::setInternalResamplingType()
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetResamplingType, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::setInternalStreamingMethod()
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetStreamingMethod, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::setInternalStreamingSize()
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetStreamingSize, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::setInternalForcedLargestPossibleRegion(itk::ImageIORegion &ior)
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetForcedLPR, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::setInternalUpdateRegion(itk::ImageIORegion &ior)
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetUpdateRegion, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

bool
NMStreamingImageFileWriterWrapper
::isOutputFileNameWriteable(const QString &fn)
{
    bool ret = false;

    // need to 'cut off any special
    // filename add-ons, e.g. as for
    // netcdf variables;
    //int didx = fn.lastIndexOf(QStringLiteral("."));
    int cidx = fn.lastIndexOf(QStringLiteral(":"));

    QString justFilePath = fn;
    if (cidx > 5)// && cidx > didx)
    {
        const int len = fn.length();
        justFilePath = fn.left(len-(len-cidx));
    }

    // create a test file
    QFile file(justFilePath);
    bool bexists = file.exists();

    if (file.open(QIODevice::ReadWrite))
    {
        ret = true;

        // clean up again if file did exist
        if (!bexists)
        {
            file.remove();
        }
    }

    return ret;
}


void
NMStreamingImageFileWriterWrapper
::linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    //NMDebugCtx(ctxNMStreamWriter, << "...");
    NMDebugCtx(this->parent()->objectName().toStdString(), << "...");
    //if (step > this->mFileNames.size()-1)
    //	step = 0;
//    step = this->mapHostIndexToPolicyIndex(step, mFileNames.size());

//	if (step < this->mFileNames.size())
//	{
//		this->setInternalFileName(this->mFileNames.at(step));
//	}

    std::stringstream errtxt;

    QString fnProvNAttr;
    QString tabProvNAttr;
    QVariant inputsVar = this->getParameter("InputComponents");
    int nbInputs = 1;
    if (inputsVar.isValid())
    {
        if (inputsVar.type() == QVariant::StringList)
        {
            nbInputs = inputsVar.toStringList().size();
        }
    }

    // enable 'just streaming` or `VirtualWriter` mode
    // if no file- or tablename has been specified and
    // WriteImage and WriteTable are not selected but
    // we have inputs defined for this component
    //
    // This provides a 'virtual sink' for the pipeline,
    // e.g. for streaming over the Image2Table component
    int numFns = this->getFileNames().size();
    int numTns = this->getInputTables().size();
    bool bJustStreaming = false;
    if (nbInputs > 0 && numFns == 0 && numTns == 0 && !mWriteImage && !mWriteTable)
    {
        bJustStreaming = true;
    }

    int rank = mController->getRank(this->parent()->objectName());
    int procs = mController->getNumProcs(this->parent()->objectName());
    MPI_Comm comm = mController->getNextUpstrMPIComm(this->parent()->objectName());

    bool bParallel = false;
    if (this->getWriteProcs() > 1 && procs > 1)
    {
        bParallel = true;
    }
    else
    {
        rank = 0;
    }

    bool bWriteable = false;

    if (nbInputs == 1)
    {
        QVariant param = this->getParameter("FileNames");
        if (param.isValid())
        {
            if (!bJustStreaming)
            {
                bWriteable = false;
                if (rank == 0)
                {
                   NMDebugAI(<< this->parent()->objectName().toStdString()
                             << " tests whether " << param.toString().toStdString()
                             << " is writable." << endl);
                   bWriteable = this->isOutputFileNameWriteable(param.toString());
                }

                if (bParallel && comm != MPI_COMM_NULL)
                {
                    NMDebugAI(<< "broadcasting writeability to fellow ranks ... " << endl);

                    int errc = MPI_Bcast(&bWriteable, 1, MPI_CXX_BOOL, 0, comm);
                    MPI_Barrier(comm);
                    NMDebugAI(<< "Bcast: " << (errc == 0 ? "successful" : "failed") << endl);
                }

                NMDebugAI(<< "lr" << rank << ": " << param.toString().toStdString()
                          << "'s writable? " << (bWriteable ? "yes" : "no") << endl);

                if (!bWriteable)
                {
                    errtxt.str("");
                    errtxt << "The filename '" << param.toString().toStdString()
                           << "' is not writable!";
                    if (bParallel)
                    {
                        if (rank == 0)
                        {
                            NMErr("NMStreamingImageFileWriterWrapper", << errtxt.str());
                        }
                        NMDebugCtx(ctxNMStreamWriter, << "done!");
                        MPI_Finalize();
                        exit(1);
                    }
                    else
                    {
                        NMDebugCtx(ctxNMStreamWriter, << "done!");
                        NMErr("NMStreamingImageFileWriterWrapper", << errtxt.str());
                        NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                        e.setDescription(errtxt.str());
                        throw e;
                    }
                }
                this->setInternalFileNames(QStringList(param.toString()));
                this->mProcessedFileNames = QStringList(param.toString());
            }
            fnProvNAttr = QString("nm:FileNames=\"%1\"")
                                  .arg(param.toString());
        }

        QVariant param2 = this->getParameter("InputTables");
        if (param2.isValid())
        {
            this->setInternalInputTables(QStringList(param2.toString()), repo);
            tabProvNAttr = QString("nm:TableNames=\"%1\"")
                                  .arg(param2.toString());
        }
    }
    else
    {
        QStringList procFNs;
        foreach(const QString& fn, this->mFileNames)
        {
            QString pstr = this->mController->processStringParameter(this, fn);

            bWriteable = false;
            if (rank == 0)
            {
                NMDebugAI(<< this->parent()->objectName().toStdString()
                          << " tests whether " << pstr.toStdString()
                          << " is writable." << endl);
                bWriteable = this->isOutputFileNameWriteable(pstr);
            }

            if (bParallel && comm != MPI_COMM_NULL)
            {
                NMDebugAI(<< "broadcasting writeability (" << bWriteable
                          << ") to fellow ranks ... " << endl);

                int errc = MPI_Bcast(&bWriteable, 1, MPI_CXX_BOOL, 0, comm);
                MPI_Barrier(comm);
                NMDebugAI(<< "Bcast: " << (errc == 0 ? "successful" : "failed") << endl);
            }

            NMDebugAI(<< "lr" << rank << ": " << pstr.toStdString()
                      << "'s writable? " << (bWriteable ? "yes" : "no") << endl);

            //if (!this->isOutputFileNameWriteable(pstr))
            if (!bWriteable)
            {
                errtxt.str("");
                errtxt << "The filename '" << pstr.toStdString()
                       << "' is not writable!";
                if (bParallel)
                {
                    if (rank == 0)
                    {
                        NMErr("NMStreamingImageFileWriterWrapper", << errtxt.str());
                    }
                    NMDebugCtx(ctxNMStreamWriter, << "done!");
                    MPI_Finalize();
                    exit(1);
                }
                else
                {
                    NMDebugCtx(ctxNMStreamWriter, << "done!");
                    errtxt.str("");
                    errtxt << "The filename '" << pstr.toStdString()
                               << "' is not writable!";
                    NMErr("NMStreamingImageFileWriterWrapper", << errtxt.str());
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription(errtxt.str());
                    throw e;
                }
            }

            procFNs << pstr;
        }
        this->setInternalFileNames(procFNs);
        this->mProcessedFileNames = procFNs;
        fnProvNAttr = QString("nm:FileNames=\"%1\"")
                              .arg(procFNs.join(" "));

        QStringList procTabNs;
        foreach (const QString& tn, this->mInputTables)
        {
            QString tstr = this->mController->processStringParameter(this, tn);
            procTabNs << tstr;
        }
        this->setInternalInputTables(procTabNs, repo);
        tabProvNAttr = QString("nm:TableNames=\"%1\"")
                              .arg(procTabNs.join(" "));
    }

    this->addRunTimeParaProvN(fnProvNAttr);
    this->addRunTimeParaProvN(tabProvNAttr);

    this->setInternalUpdateMode();

    const int nDim = this->getOutputNumDimensions();
    if (mbUseForcedLPR)
    {
        std::stringstream flprstr;
        flprstr << "nm:ForcedLPR=\"";
        itk::ImageIORegion flpr(nDim);
        for (int d=0; d < nDim; ++d)
        {
            flpr.SetIndex(d, mForcedLPR[d]);
            flprstr << mForcedLPR[d];
            if (d < nDim-1)
            {
                flprstr << " ";
            }
        }
        for (int d=0; d < nDim; ++d)
        {
            flpr.SetSize(d, mForcedLPR[d+nDim]);
            flprstr << mForcedLPR[d+nDim];
            if (d < nDim-1)
            {
                flprstr << " ";
            }
        }
        flprstr << "\"";
        this->addRunTimeParaProvN(flprstr.str().c_str());

        this->setInternalForcedLargestPossibleRegion(flpr);
    }

    if (mbUseUpdateRegion)
    {
        std::stringstream urstr;
        urstr << "nm:UpdateRegion=\"";
        itk::ImageIORegion ur(nDim);
        for (int d=0; d < nDim; ++d)
        {
            ur.SetIndex(d, mUpdateRegion[d]);
            urstr << mUpdateRegion[d];
            if (d < nDim-1)
            {
                urstr << " ";
            }
        }
        for (int d=0; d < nDim; ++d)
        {
            ur.SetSize(d, mUpdateRegion[d+nDim]);
            urstr << mUpdateRegion[d+nDim];
            if (d < nDim-1)
            {
                urstr << " ";
            }
        }
        urstr << "\"";
        this->addRunTimeParaProvN(urstr.str().c_str());

        this->setInternalUpdateRegion(ur);
    }

    QString updateProvNAttr = QString("nm:UpdateMode=\"%1\"")
                          .arg(mUpdateMode ? "true" : "false");
    this->addRunTimeParaProvN(updateProvNAttr);
    QString writeImgProvNAttr = QString("nm:WriteImage=\"%1\"")
                          .arg(mWriteImage ? "true" : "false");
    this->addRunTimeParaProvN(writeImgProvNAttr);
    QString writeTabProvNAttr = QString("nm:WriteTable=\"%1\"")
                          .arg(mWriteTable ? "true" : "false");
    this->addRunTimeParaProvN(writeTabProvNAttr);
    QString rgbModeProvNAttr = QString("nm:RGBMode=\"%1\"")
                          .arg(mRGBMode ? "true" : "false");
    this->addRunTimeParaProvN(rgbModeProvNAttr);

    this->setInternalResamplingType();
    QString resTypeProvNAttr = QString("nm:ResamplingType=\"%1\"")
                              .arg(mPyramidResamplingType);
    this->addRunTimeParaProvN(resTypeProvNAttr);

    this->setInternalStreamingMethod();
    QString streamMethodProvNAttr = QString("nm:StreamingMethod=\"%1\"")
                          .arg(mStreamingMethodType);
    this->addRunTimeParaProvN(streamMethodProvNAttr);

    this->setInternalStreamingSize();
    QString streamSizeProvNAttr = QString("nm:StreamingSize=\"%1\"")
                          .arg(mStreamingSize);
    this->addRunTimeParaProvN(streamSizeProvNAttr);

    this->setInternalParallelIO();

    NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");

}

void
NMStreamingImageFileWriterWrapper
::setInternalParallelIO(void)
{
    NMDebugCtx(this->parent()->objectName().toStdString(), << "...");

    if (!this->mbIsInitialised)
        return;

    //MPI_Comm comm = MPI_COMM_WORLD;
    //int rank, procs;
    //MPI_Comm_size(comm,&procs);
    //MPI_Comm_rank(comm,&rank);


    if (this->mWriteProcs > 1 && mController->getNumProcs(this->parent()->objectName()) > 1)
    {
        this->mParallelIO = true;
        NMDebugAI(<< ctxNMStreamWriter << ": mWriteProcs=" << mWriteProcs << " -> ParallelIO=true!" << std::endl);
        NMDebugAI(<< ctxNMStreamWriter << ": #procs=" << mController->getNumProcs(this->parent()->objectName()) << std::endl);
    }
    else
    {
        this->mParallelIO = false;
        NMDebugAI(<< ctxNMStreamWriter << ": mWriteProcs=" << mWriteProcs << " -> ParallelIO=false!" << std::endl);
        NMDebugAI(<< ctxNMStreamWriter << ": #procs=" << mController->getNumProcs(this->parent()->objectName()) << std::endl);
    }

    MPI_Comm comm = mController->getNextUpstrMPIComm(this->parent()->objectName());
    if (comm == MPI_COMM_NULL)
    {
        NMDebugAI(<< ctxNMStreamWriter << ": No valid MPI_Comm communicator registered for this component!"
                  << " So will do good old squential processing instead!" << std::endl);
        this->mParallelIO = false;
        return;
    }


    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetParallelIO, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
    NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
}

void
NMStreamingImageFileWriterWrapper
::setInternalUpdateMode()
{
    if (!this->mbIsInitialised)
        return;

    switch(this->mOutputComponentType)
    {
    MacroPerType( callSetUpdateMode, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}

void
NMStreamingImageFileWriterWrapper
::internalParallelIO_Update()
{
    NMDebugCtx(ctxNMStreamWriter, << "...");
    if (!this->mbIsInitialised)
        return;

    const int nprocs = std::min(mController->getNumProcs(this->parent()->objectName()), this->mWriteProcs);
    const int rank =   mController->getRank(this->parent()->objectName());
    MPI_Comm comm =    mController->getNextUpstrMPIComm(this->parent()->objectName());

    //MPI_Comm comm = MPI_COMM_WORLD;
    //int nprocs, rank;
    //MPI_Comm_rank(comm, &rank);
    //MPI_Comm_size(comm, &nprocs);

    if (comm == MPI_COMM_NULL)
    {
        NMLogError(<< " assigned MPI_Comm is NULL!");
        return;
    }

    NMDebugAI("MPI rank= " << rank << " | MPI procs=" << nprocs )

    switch(this->mOutputComponentType)
    {
    MacroPerType( callParallelIOUpdate, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
    NMDebugCtx(ctxNMStreamWriter, << "done!");
}

void
NMStreamingImageFileWriterWrapper
::update()
{
    NMDebugCtx(this->parent()->objectName().toStdString(), << "...");

    // remove debug below
    int rank = mController->getRank(this->parent()->objectName());
    std::stringstream msg;
    msg << "lr" << rank << ": writes " << this->mProcessedFileNames.join(" ").toStdString();
    NMDebugCtx(this->parent()->objectName().toStdString(), << msg.str());
    // remove debug above

    QDateTime startTime;

    if (this->mbIsInitialised && this->mOtbProcess.IsNotNull())
    {
        try
        {
            startTime = QDateTime::currentDateTime();
            QString startString = startTime.toString("dd.MM.yyyy hh:mm:ss.zzz");
            NMDebugAI(<< this->parent()->objectName().toStdString()
                      << ": started at: " << startString.toStdString() << std::endl);

            if (mParallelIO)
            {
                bool bPio_cnt = true;
                foreach(const QString& fn, mProcessedFileNames)
                {
                    //QFileInfo pifo(fn);
                    //if (pifo.suffix().compare("nc", Qt::CaseInsensitive) != 0)
                    if (!fn.contains(".nc"))
                    {
                        bPio_cnt = false;
                        exit;
                    }
                }

                if (mInputNumBands > 1)
                {
                    bPio_cnt = false;
                }

                if (bPio_cnt)
                {
                    //MPI_Barrier(MPI_COMM_WORLD);
                    internalParallelIO_Update();
                }
                else
                {
                    this->mOtbProcess->Update();
                }
            }
            else
            {
                this->mOtbProcess->Update();
            }
        }
        catch (...)
        {
            emit signalProgress(0);

            throw;
        }

        this->mMTime = QDateTime::currentDateTime();
        QString tstring = mMTime.toString("dd.MM.yyyy hh:mm:ss.zzz");
        int msec = startTime.msecsTo(this->mMTime);
        int min = msec / 60000;
        double sec = (msec % 60000) / 1000.0;
        QString elapsedTime = QString("%1:%2").arg((int)min).arg(sec,0,'g',3);
        NMDebugAI(<< this->parent()->objectName().toStdString()
                  << ": modified at " << tstring.toStdString() << std::endl);
        NMDebugAI(<< this->parent()->objectName().toStdString()
                  << ": ::Update() took (min:sec) " << elapsedTime.toStdString() << std::endl);
    }
    else
    {
        NMLogError(<< this->parent()->objectName().toStdString()
                << ": Update failed! Either the process object is not initialised or"
                << " itk::ProcessObject is NULL and ::update() is not re-implemented!");
    }
    this->mbLinked = false;

    //NMDebugAI(<< "update value for: mParamPos=" << this->mParamPos << endl);
    NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
}

void
NMStreamingImageFileWriterWrapper
::setRAT(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> imgWrapper)
{
    // we only set the RAT from the input component, if
    // - we're supposed to write a table at all
    // - we're not writing a table provided by a non-input component
    // - and the wrapper we've received is actually not null
    QVariant varInputTables = this->getParameter("InputTables");
    QString inputtable;
    if (varInputTables.isValid() && !varInputTables.isNull())
    {
        inputtable = varInputTables.toString();
    }

    if (    mWriteTable
        &&  inputtable.isEmpty()
        &&  !imgWrapper.isNull()
       )
    {
        otb::AttributeTable::Pointer tab = imgWrapper->getOTBTab();

        unsigned int tabIdx = idx;
        if (tab.IsNotNull())
        {
            switch(this->mOutputComponentType)
            {
            MacroPerType( callOutputTypeSetTab, NMStreamingImageFileWriterWrapper_Internal )
            default:
                break;
            }
        }
    }
}

void
NMStreamingImageFileWriterWrapper
::setInternalInputTables(const QStringList tabelSpecList,
                        const QMap<QString, NMModelComponent*>& repo)
{
    if (!this->mbIsInitialised)
        return;

    for (unsigned int tabIdx=0; tabIdx < tabelSpecList.size(); ++tabIdx)
    {
        QString tabelSpec = tabelSpecList.at(tabIdx);
        QStringList speclst = tabelSpec.split(":");
        QString compName = speclst.at(0);
        if (repo.contains(compName))
        {
            int idx = 0;
            if (speclst.size() == 2)
            {
                bool bok;
                idx = speclst.at(1).toInt(&bok);
            }
            NMModelComponent* comp = repo.value(compName);
            QSharedPointer<NMItkDataObjectWrapper> dw = comp->getOutput(idx);

            if (!dw.isNull())
            {
                otb::AttributeTable::Pointer tab = dw->getOTBTab();

                if (tab.IsNotNull())
                {
                    switch(this->mOutputComponentType)
                    {
                    MacroPerType( callOutputTypeSetTab, NMStreamingImageFileWriterWrapper_Internal )
                    default:
                        break;
                    }
                }
            }
        }
    }
}


void
NMStreamingImageFileWriterWrapper::setForcedLargestPossibleRegion(std::array<int, 6> forcedLPR)
{
    mbUseForcedLPR = false;

    if (forcedLPR.size() < 6)
    {
        return;
    }

    if (forcedLPR[3] == 0 && forcedLPR[4] == 0)
    {
        return;
    }

    mbUseForcedLPR = true;
    mForcedLPR = forcedLPR;
}

void
NMStreamingImageFileWriterWrapper::setUpdateRegion(std::array<int, 6> updateRegion)
{
    mbUseUpdateRegion = false;

    if (updateRegion.size() < 6)
    {
        return;
    }

    if (updateRegion[3] == 0 && updateRegion[4] == 0)
    {
        return;
    }

    mbUseUpdateRegion = true;
    mUpdateRegion = updateRegion;
}

void
NMStreamingImageFileWriterWrapper
::setNthInput(unsigned int numInput,
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name)
{
    if (!this->mbIsInitialised)
        return;

    itk::DataObject* img = imgWrapper->getDataObject();
    switch(this->mOutputComponentType)
    {
    MacroPerType( callFilterSetInput, NMStreamingImageFileWriterWrapper_Internal )
    default:
        break;
    }
}


QSharedPointer<NMItkDataObjectWrapper>
NMStreamingImageFileWriterWrapper::getOutput(unsigned int idx)
{
    QSharedPointer<NMItkDataObjectWrapper> dw;
    dw.clear();
    return dw;
}



#ifdef BUILD_RASSUPPORT
void NMStreamingImageFileWriterWrapper::setRasConnector(NMRasdamanConnectorWrapper* rw)
{
    if (rw != this->mRasConnector)
    {
        this->mRasConnector = rw;
        emit nmChanged();
    }
}

NMRasdamanConnectorWrapper* NMStreamingImageFileWriterWrapper::getRasConnector(void)
{
    return this->mRasConnector;
}
#endif


