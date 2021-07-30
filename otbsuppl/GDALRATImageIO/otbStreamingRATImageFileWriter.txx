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
/*=========================================================================

  Program:   ORFEO Toolbox
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


  Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
  See OTBCopyright.txt for details.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __otbStreamingRATImageFileWriter_txx
#define __otbStreamingRATImageFileWriter_txx

#include "nmlog.h"
#define ctxSIFR "StreamingRATImageFileWriter"

#include "otbStreamingRATImageFileWriter.h"
#include "itkImageFileWriter.h"

#include "itkObjectFactoryBase.h"

#include "itkImageRegionMultidimensionalSplitter.h"
#include "otbImageIOFactory.h"
#include "otbGDALRATImageIO.h"
#include "nmNetCDFIO.h"
#include "itkMultiResolutionPyramidImageFilter.h"

#ifdef BUILD_RASSUPPORT
#include "otbRasdamanImageIO.h"
#endif

#include "itkMetaDataObject.h"
#include "otbImageKeywordlist.h"
#include "otbMetaDataKey.h"

#include "otbConfigure.h"

#include "otbNumberOfDivisionsStrippedStreamingManager.h"
#include "otbNumberOfDivisionsTiledStreamingManager.h"
#include "otbNumberOfLinesStrippedStreamingManager.h"
#include "otbRAMDrivenStrippedStreamingManager.h"
#include "otbTileDimensionTiledStreamingManager.h"
#include "otbRAMDrivenTiledStreamingManager.h"


namespace otb
{

/**
 *
 */
template <class TInputImage>
StreamingRATImageFileWriter<TInputImage>
::StreamingRATImageFileWriter()
    : m_WriteGeomFile(false), m_NumberOfInputs(1)//,
      //m_bUseCropRegion(false)
{
    m_UserSpecifiedIORegion = true;
    m_FactorySpecifiedImageIO = false;

    // By default, we use tiled streaming, with automatic tile size
    // We don't set any parameter, so the memory size is retrieved from
    // the OTB configuration options
    // this->SetAutomaticTiledStreaming();
    this->SetAutomaticStrippedStreaming(512);

    m_ResamplingType = "NEAREST";
    m_StreamingMethod = "STRIPPED";
    m_StreamingSize = 512;

    m_UseCompression = true;
    m_RATHaveBeenWritten = false;
    m_UseForcedLPR = false;
    m_UseUpdateRegion = false;
    m_UpdateMode = false;
    m_WriteImage = true;
#ifdef BUILD_RASSUPPORT
    this->mRasconn = 0;
#endif
}

/**
 *
 */
template <class TInputImage>
StreamingRATImageFileWriter<TInputImage>
::~StreamingRATImageFileWriter()
{
}

template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetNumberOfDivisionsStrippedStreaming(unsigned int nbDivisions)
{
    typedef NumberOfDivisionsStrippedStreamingManager<TInputImage> NumberOfDivisionsStrippedStreamingManagerType;
    typename NumberOfDivisionsStrippedStreamingManagerType::Pointer streamingManager = NumberOfDivisionsStrippedStreamingManagerType::New();
    streamingManager->SetNumberOfDivisions(nbDivisions);

    m_StreamingManager = streamingManager;
}

template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetNumberOfDivisionsTiledStreaming(unsigned int nbDivisions)
{
    typedef NumberOfDivisionsTiledStreamingManager<TInputImage> NumberOfDivisionsTiledStreamingManagerType;
    typename NumberOfDivisionsTiledStreamingManagerType::Pointer streamingManager = NumberOfDivisionsTiledStreamingManagerType::New();
    streamingManager->SetNumberOfDivisions(nbDivisions);

    m_StreamingManager = streamingManager;
}

template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetNumberOfLinesStrippedStreaming(unsigned int nbLinesPerStrip)
{
    typedef NumberOfLinesStrippedStreamingManager<TInputImage> NumberOfLinesStrippedStreamingManagerType;
    typename NumberOfLinesStrippedStreamingManagerType::Pointer streamingManager = NumberOfLinesStrippedStreamingManagerType::New();
    streamingManager->SetNumberOfLinesPerStrip(nbLinesPerStrip);

    m_StreamingManager = streamingManager;
}

template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetAutomaticStrippedStreaming(unsigned int availableRAM, double bias)
{
    typedef RAMDrivenStrippedStreamingManager<TInputImage> RAMDrivenStrippedStreamingManagerType;
    typename RAMDrivenStrippedStreamingManagerType::Pointer streamingManager = RAMDrivenStrippedStreamingManagerType::New();
    streamingManager->SetAvailableRAMInMB(availableRAM);
    streamingManager->SetBias(bias);

    m_StreamingManager = streamingManager;
}

template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetTileDimensionTiledStreaming(unsigned int tileDimension)
{
    typedef TileDimensionTiledStreamingManager<TInputImage> TileDimensionTiledStreamingManagerType;
    typename TileDimensionTiledStreamingManagerType::Pointer streamingManager = TileDimensionTiledStreamingManagerType::New();
    streamingManager->SetTileDimension(tileDimension);

    m_StreamingManager = streamingManager;
}

template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetAutomaticTiledStreaming(unsigned int availableRAM, double bias)
{
    typedef RAMDrivenTiledStreamingManager<TInputImage> RAMDrivenTiledStreamingManagerType;
    typename RAMDrivenTiledStreamingManagerType::Pointer streamingManager = RAMDrivenTiledStreamingManagerType::New();
    streamingManager->SetAvailableRAMInMB(availableRAM);
    streamingManager->SetBias(bias);
    m_StreamingManager = streamingManager;
}

/**
 *
 */
template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetBufferMemorySize(unsigned long memory_size_divisions)
{
    //  itkWarningMacro("SetNumberOfDivisionsStrippedStreaming is DEPRECATED. "
    //                  "Use one of SetNumberOfLinesStrippedStreaming, "
    //                  "SetAutomaticStrippedStreaming, SetTileDimensionTiledStreaming, "
    //                  "or SetAutomaticTiledStreaming." );

    unsigned int ram = static_cast<unsigned int>(memory_size_divisions / 1024 / 1024);
    this->SetAutomaticStrippedStreaming(ram);
}

/**
 *
 */
template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetBufferNumberOfLinesDivisions(unsigned long nb_lines_divisions)
{
    //  itkWarningMacro("SetBufferNumberOfLinesDivisions is DEPRECATED. "
    //                  "Use one of SetNumberOfLinesStrippedStreaming, "
    //                  "SetAutomaticStrippedStreaming, SetTileDimensionTiledStreaming, "
    //                  "or SetAutomaticTiledStreaming." );

    unsigned int nb_lines = static_cast<unsigned int>(nb_lines_divisions);
    this->SetNumberOfLinesStrippedStreaming(nb_lines);
}

/**
 *
 */
template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetNumberOfStreamDivisions(unsigned long nb_divisions)
{
    //  itkWarningMacro("SetNumberOfStreamDivisions is DEPRECATED. "
    //                  "Use one of SetNumberOfLinesStrippedStreaming, "
    //                  "SetAutomaticStrippedStreaming, SetTileDimensionTiledStreaming, "
    //                  "or SetAutomaticTiledStreaming." );
    unsigned int nb_div = static_cast<unsigned int>(nb_divisions);
    this->SetNumberOfDivisionsStrippedStreaming(nb_div);
}

/**
 *
 */
template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetAutomaticNumberOfStreamDivisions(void)
{
    itkWarningMacro("SetAutomaticNumberOfStreamDivisions is DEPRECATED. "
                    "Use one of SetNumberOfLinesStrippedStreaming, "
                    "SetAutomaticStrippedStreaming, SetTileDimensionTiledStreaming, "
                    "or SetAutomaticTiledStreaming." );
    this->SetAutomaticStrippedStreaming(0);
}

/**
 *
 */
template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetTilingStreamDivisions(void)
{
    itkWarningMacro("SetTilingStreamDivisions is DEPRECATED. "
                    "Use one of SetNumberOfLinesStrippedStreaming, "
                    "SetAutomaticStrippedStreaming, SetTileDimensionTiledStreaming, "
                    "or SetAutomaticTiledStreaming." );
    this->SetAutomaticTiledStreaming(0);
}

template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetTilingStreamDivisions(unsigned long nb_divisions)
{
    itkWarningMacro("SetTilingStreamDivisions is DEPRECATED. "
                    "Use one of SetNumberOfLinesStrippedStreaming, "
                    "SetAutomaticStrippedStreaming, SetTileDimensionTiledStreaming, "
                    "or SetAutomaticTiledStreaming." );
    unsigned int tileDim = static_cast<unsigned int>(nb_divisions);
    this->SetNumberOfDivisionsTiledStreaming(nb_divisions);
}

/**
 *
 */
template <class TInputImage>
unsigned long
StreamingRATImageFileWriter<TInputImage>
::GetNumberOfStreamDivisions(void)
{
    return m_StreamingManager->GetNumberOfSplits();
}

/**
 *
 */
template <class TInputImage>
std::string
StreamingRATImageFileWriter<TInputImage>
::GetMethodUseToCalculateNumberOfStreamDivisions(void)
{
    itkWarningMacro("GetMethodUseToCalculateNumberOfStreamDivisions is DEPRECATED");
    return "NOT-IMPLEMENTED";
}

/**
 *
 */
template <class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
    Superclass::PrintSelf(os, indent);

    os << indent << "FileNames: ";
    for (int f=0; f < m_FileNames.size(); ++f)
    {
        os << m_FileNames[f].data() << " ";
    }
    os << std::endl;

    for (int io=0; io < m_ImageIOs.size(); ++io)
    {
        if (m_ImageIOs[io].IsNull())
        {
            os << "(none) ";
        }
        else
        {
            os << m_ImageIOs[io] << " ";
        }
    }
    os << std::endl;

    os << indent << "IO Region: " << m_IORegion << "\n";

    if (m_UseCompression)
    {
        os << indent << "Compression: On\n";
    }
    else
    {
        os << indent << "Compression: Off\n";
    }

    if (m_UseInputMetaDataDictionary)
    {
        os << indent << "UseInputMetaDataDictionary: On\n";
    }
    else
    {
        os << indent << "UseInputMetaDataDictionary: Off\n";
    }

    if (m_FactorySpecifiedImageIO)
    {
        os << indent << "FactorySpecifiedmageIO: On\n";
    }
    else
    {
        os << indent << "FactorySpecifiedmageIO: Off\n";
    }
}

//---------------------------------------------------------




template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetImageIO(ImageIOBase *imgIO)
{
    if (m_ImageIOs.size() == 0)
    {
        m_ImageIOs.push_back(imgIO);
    }
    else
    {
        m_ImageIOs[0] = imgIO;
    }
    this->Modified();
}

template<class TInputImage>
otb::ImageIOBase*
StreamingRATImageFileWriter<TInputImage>
::GetImageIO(void)
{
    otb::ImageIOBase* iob = nullptr;
    if (m_ImageIOs.size() > 0)
    {
        iob = m_ImageIOs[0].GetPointer();
    }

    return iob;
}

template<class TInputImage>
const otb::ImageIOBase*
StreamingRATImageFileWriter<TInputImage>
::GetImageIO(void) const
{
    otb::ImageIOBase* iob = nullptr;
    if (m_ImageIOs.size() > 0)
    {
        iob = m_ImageIOs[0].GetPointer();
    }

    return iob;
}


template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetIORegion(const itk::ImageIORegion& region)
{
    itkDebugMacro("setting IORegion to " << region);
    if (m_IORegion != region)
    {
        m_IORegion = region;
        this->Modified();
        m_UserSpecifiedIORegion = true;
    }
}

//template<class TInputImage>
//void
//StreamingRATImageFileWriter<TInputImage>
//::EnlargeOutputRequestedRegion(itk::DataObject* output)
//{
//    Superclass::EnlargeOutputRequestedRegion(output);
//    OutputImageType* out = static_cast<OutputImageType*>(output);

//    if (m_UpdateMode && m_UseForcedLPR)
//    {
//        itk::ImageRegion<TInputImage::ImageDimension> outr;
//        for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
//        {
//            outr.SetIndex(d, m_ForcedLargestPossibleRegion.GetIndex()[d]);
//            outr.SetSize(d, m_ForcedLargestPossibleRegion.GetSize()[d]);
//        }
//        out->SetLargestPossibleRegion(outr);
//    }
//}

//template<class TInputImage>
//void StreamingRATImageFileWriter<TInputImage>
//::GenerateInputRequestedRegion()
// {
//    //NMDebugCtx(ctxSIFR, << "...");
//    Superclass::GenerateInputRequestedRegion();

//    InputImageType * inputPtr = const_cast<InputImageType*>(this->GetInput());

//    if(!inputPtr)
//    {
//        return;
//    }
//    typename InputImageType::RegionType lregion = inputPtr->GetLargestPossibleRegion();

//    typename InputImageType::SizeType rsize;
//    rsize.Fill(0);
//    lregion.SetSize(rsize);

//    inputPtr->SetRequestedRegion(lregion);
//}

template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetForcedLargestPossibleRegion(const itk::ImageIORegion& forcedReg)
{
    if (m_ForcedLargestPossibleRegion != forcedReg)
    {
        m_ForcedLargestPossibleRegion = forcedReg;
        this->m_UseForcedLPR = true;
        this->Modified();
    }
}

template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::BuildOverviews(void)
{
    for (int i=0; i < m_ImageIOs.size(); ++i)
    {
        GDALRATImageIO* gio = dynamic_cast<GDALRATImageIO*>(m_ImageIOs[i].GetPointer());
        NetCDFIO* nio = dynamic_cast<NetCDFIO*>(m_ImageIOs[i].GetPointer());

        if (nio != nullptr)
        {
            nio->BuildOverviews(m_ResamplingType);
        }
        else if (gio != nullptr)
        {
            gio->BuildOverviews(m_ResamplingType);
        }
    }
}

template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetUpdateRegion(const itk::ImageIORegion& updateRegion)
{
    if ((updateRegion.GetIndex()[0] + updateRegion.GetSize()[0]) <= m_ForcedLargestPossibleRegion.GetSize()[0] &&
            (updateRegion.GetIndex()[1] + updateRegion.GetSize()[1]) <= m_ForcedLargestPossibleRegion.GetSize()[1]     )
    {
        this->m_UpdateRegion = updateRegion;
        this->m_UseUpdateRegion = true;
        this->Modified();
    }
    else
    {
        NMProcWarn(<< "The provided update region does not fit into the configured 'forced largest possible region'!");
        this->m_UseUpdateRegion = false;
    }
}

template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::SetInputRAT(AttributeTable *rat, unsigned int idx)
{
    otb::AttributeTable::Pointer tab = rat;
    if (idx < m_InputRATs.size())
    {
        m_InputRATs[idx] = rat;
    }
    else
    {
        m_InputRATs.resize(idx+1);
        m_InputRATs[idx] = rat;
    }
}

#ifdef BUILD_RASSUPPORT
template<class TInputImage>
void StreamingRATImageFileWriter<TInputImage>
::SetRasdamanConnector(RasdamanConnector* rascon)
{
    if (rascon)
    {
        this->mRasconn = rascon;
        otb::RasdamanImageIO::Pointer rio = otb::RasdamanImageIO::New();
        rio->setRasdamanConnector(rascon);
        this->m_ImageIO = rio;
    }
    else
    {
        this->mRasconn = 0;
        this->m_ImageIO = NULL;
    }
}
#endif

/**
 *
 */
template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::UpdateOutputData(itk::DataObject *itkNotUsed(output))
{
    //    NMDebugCtx(ctxSIFR, << "...");

    unsigned int idx;
    /**
   * prevent chasing our tail
   */
    if (this->m_Updating)
    {
        return;
    }

    /**
   * Make sure we have the necessary inputs
   */
    //unsigned int ninputs = this->GetNumberOfValidRequiredInputs();
    //if (ninputs < this->GetNumberOfRequiredInputs())
    //{
    //    itkExceptionMacro(<< "At least " << static_cast<unsigned int>(
    //                          this->GetNumberOfRequiredInputs()) << " inputs are required but only " << ninputs <<
    //                      " are specified.");
    //    return;
    //}
    this->SetAbortGenerateData(0);
    this->SetProgress(0.0);
    this->m_Updating = true;
    /**
   * Tell all Observers that the filter is starting
   */
    this->InvokeEvent(itk::StartEvent());

    /** Prepare ImageIO  : create ImageFactory */

    m_NumberOfInputs = this->GetNumberOfInputs();
    if (m_FileNames.size() == 0)
    {
        itkExceptionMacro(<< "No filename provided! Please specify a filename for each input "
                          << "image you want to write! Abort.");
        return;
    }
    else if (m_FileNames.size() < m_NumberOfInputs)
    {
        // Make sure that we can write the file given the name
        NMProcWarn(<< "Only " << m_FileNames.size() << " of " << m_NumberOfInputs
                   << " images are going to written! Provide a filename for "
                   << " each input image to write all inputs!");
        m_NumberOfInputs = m_FileNames.size();
    }

    this->SetNumberOfIndexedOutputs(m_NumberOfInputs);
    this->PrepareOutputs();

    // make sure we've got enough outputs
    for (int r=1; r < m_NumberOfInputs; ++r)
    {
        const InputImageType* inImg = this->GetInput(r);
        typename OutputImageType::Pointer outImg = dynamic_cast<OutputImageType*>(this->MakeOutput(r).GetPointer());
        outImg->CopyInformation(inImg);
        this->SetNthOutput(r, outImg.GetPointer());
    }

    // make sure we've got the same number of objects in the lists
    m_ImageIOs.resize(m_NumberOfInputs);
    m_InputRATs.resize(m_NumberOfInputs);

    for (int io=0; io < this->m_FileNames.size(); ++io)
    {
        if (m_ImageIOs[io].IsNotNull())
        {
            m_ImageIOs[io]->SetFileName(m_FileNames[io]);
        }
    }


    /**
     *  instantiate an imageIO for each image to be written
     */
    bool bCanStreamWriteAll = true;
    for (int io=0; io < this->m_FileNames.size(); ++io)
    {
        if (m_ImageIOs[io].IsNull())
        {
            if (m_FileNames[io].find(".nc") != std::string::npos)
            {
                NetCDFIO::Pointer nioPtr = NetCDFIO::New();
                if (nioPtr.IsNotNull() && nioPtr->CanWriteFile(m_FileNames[io].c_str()))
                {
                    nioPtr->SetFileName(m_FileNames[io].c_str());

                    if (!nioPtr->CanStreamWrite())
                    {
                        bCanStreamWriteAll = false;
                    }

                    this->m_ImageIOs[io] = nioPtr;
                }
                else
                {
                    itkExceptionMacro(<< "Failed to create image file '"
                                      << m_FileNames[io].c_str() << "'!");
                }
            }
            // in multi-input scenarios, a 'NO_WRITE' filename
            // indicates that this input shall not be written;
            // we use this esp. for the multi-res pyramid inputs
            // where we need to account for input[0] for RAM footprint
            // calculation, but actually don't want to write that
            // image
            else if (m_FileNames[io].compare("DO_NOT_WRITE") != 0)
            {
                // if the image io hasn't been set, we're using the GDALRATImageIO by default;
                GDALRATImageIO::Pointer gioPtr = GDALRATImageIO::New();
                if (gioPtr.IsNull())
                {
                    itkExceptionMacro(<< "Failed to create instance of GDALRATImageIO");
                }

                gioPtr->SetFileName(this->m_FileNames[io]);

                if (!gioPtr->CanStreamWrite())
                {
                    bCanStreamWriteAll = false;
                }

                this->m_ImageIOs[io] = gioPtr;

            }
        }

        /** End of Prepare ImageIO  : create ImageFactory */


        /** set writer and imageIO output information */
        NetCDFIO* nio = dynamic_cast<NetCDFIO*>(m_ImageIOs[io].GetPointer());
        GDALRATImageIO* gio = dynamic_cast<otb::GDALRATImageIO*>(m_ImageIOs[io].GetPointer());
#ifdef BUILD_RASSUPPORT
        RasdamanImageIO* rio = dynamic_cast<otb::RasdamanImageIO*>(m_ImageIOs[io].GetPointer());
#endif

        if (m_InputRATs[io].IsNotNull())
        {
            if (nio != nullptr)
            {
                nio->setRasterAttributeTable(m_InputRATs[io], 1);
            }
            else if (gio != nullptr)
            {
                gio->setRasterAttributeTable(m_InputRATs[io], 1);
            }
#ifdef BUILD_RASSUPPORT
            else if (rio != nullptr)
            {
                rio->setRasterAttributeTable(m_InputRATs[io], 1);
            }
#endif
        }



        // update and forcedLPR mode only considered for single
        // image writing
        if (m_NumberOfInputs == 1 && m_UpdateMode)
        {   if (nio != nullptr)
            {
                nio->SetImageUpdateMode(true);
            }
            else if (gio != nullptr)
            {
                gio->SetImageUpdateMode(true);
            }
#ifdef BUILD_RASSUPPORT
            else if (rio != nullptr)
            {
                rio->SetImageUpdateMode(true);
            }
#endif
        }

        /* in case we want to make the image bigger than the we've currently data for
         * (e.g. for externally driven sequential writing with intertwined reading),
         */
        if (m_NumberOfInputs == 1 && m_UseForcedLPR)
        {   if (nio != nullptr)
            {
                nio->SetForcedLPR(m_ForcedLargestPossibleRegion);
            }
            else if (gio != nullptr)
            {
                gio->SetForcedLPR(m_ForcedLargestPossibleRegion);
            }
#ifdef BUILD_RASSUPPORT
            else if (rio != nullptr)
            {
                rio->SetForcedLPR(m_ForcedLargestPossibleRegion);
            }
#endif
        }

    }

    // in case the user specified an explicit update region for externally controlled
    // streaming, we set this as the outputRegion to allow for streaming over this region
    OutputImageRegionType outputRegion = this->GetOutput(0)->GetLargestPossibleRegion();

    if (m_NumberOfInputs == 1 && m_UpdateMode && m_UseUpdateRegion)
    {
        for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
        {
            outputRegion.SetIndex(d, m_UpdateRegion.GetIndex()[d]);
            outputRegion.SetSize(d, m_UpdateRegion.GetSize()[d]);
        }
    }

   /**
    * Grab the input
    */
    InputImagePointer inputPtr =
            const_cast<InputImageType *>(this->GetInput(0));

   /**
    * Set the user's streaming preferences; for multi-input output
    * components it is assumed (for simplicity) that all outputs
    * are of the same size and if not that output[0] will be
    * the largest of all therefore m_StreamingSize could be
    * seen as the max RAM required
    *
    * the streaming size
    */
    if (m_StreamingMethod.compare("STRIPPED") == 0)
    {
        this->SetAutomaticStrippedStreaming(m_StreamingSize / m_NumberOfInputs);
    }
    else
    {
        this->SetAutomaticTiledStreaming(m_StreamingSize / m_NumberOfInputs);
    }


    /**
   * Determine the of number of pieces to divide the input.  This will be the
   * minimum of what the user specified via SetNumberOfDivisionsStrippedStreaming()
   * and what the Splitter thinks is a reasonable value.
   */

    /** Control if the ImageIO is CanStreamWrite*/
    if (!bCanStreamWriteAll || InputImageDimension == 1)
    {
        this->SetNumberOfDivisionsStrippedStreaming(1);
    }
    else if (m_NumberOfInputs == 1 && inputPtr->GetBufferedRegion() == inputPtr->GetLargestPossibleRegion())
    {
        this->SetNumberOfDivisionsStrippedStreaming(1);
    }
    else if (m_StreamingMethod.compare("NO_STREAMING") == 0)
    {
        this->SetNumberOfDivisionsStrippedStreaming(1);
    }
    m_StreamingManager->PrepareStreaming(inputPtr, outputRegion);
    m_NumberOfDivisions = m_StreamingManager->GetNumberOfSplits();

    // below code is copied from otbMultiImageFileWriter.cxx
//    if (m_NumberOfDivisions > 1)
//    {
//        // Check this number of division is compatible with all inputs
//        bool nbDivValid = false;
//        while ((!nbDivValid) && 1 < m_NumberOfDivisions)
//        {
//            unsigned int smallestNbDiv = m_NumberOfDivisions;
//            for (unsigned int i = 0; i < m_NumberOfInputs; ++i)
//            {
//                InputImageRegionType inLPR = this->GetInput(i)->GetLargestPossibleRegion();
//                unsigned int div = m_StreamingManager->GetSplitter()->GetNumberOfSplits(inLPR, m_NumberOfDivisions);
//                smallestNbDiv    = std::min(div, smallestNbDiv);
//            }
//            if (smallestNbDiv == m_NumberOfDivisions)
//            {
//                nbDivValid = true;
//            }
//            else
//            {
//                m_NumberOfDivisions = smallestNbDiv;
//            }
//        }
//        if (m_NumberOfDivisions == 1)
//        {
//            otbWarningMacro("Can't find a common split scheme between all inputs, streaming disabled\n");
//        }
//    }

    // no point in chopping up the image, if we're not
    // intrested in it (and only want to write the table)
    if (!m_WriteImage)
    {
        m_NumberOfDivisions = 1;
    }
    //otbMsgDebugMacro(<< "Number Of Stream Divisions : " << m_NumberOfDivisions);

   /**
   * Loop over the number of pieces of each image, execute the upstream pipeline on each
   * piece (and image), and copy the results into the output image(s).
   */
    //InputImageRegionType streamRegion;

    //
    // Setup the ImageIOs with information from output images
    //
    for (int ni=0; ni < m_NumberOfInputs; ++ni)
    {
        // skip 'DO_NOT_WRITE's
        if (m_ImageIOs[ni].IsNull())
        {
            continue;
        }

        InputImageType* inImg = const_cast<InputImageType*>(this->GetInput(ni));
        OutputImagePointer outImg = this->GetOutput(ni);
        OutputImageRegionType outRegion = outImg->GetLargestPossibleRegion();

        m_ImageIOs[ni]->SetNumberOfDimensions(TInputImage::ImageDimension);
        const typename TInputImage::SpacingType&   spacing = outImg->GetSpacing();
        const typename TInputImage::PointType&     origin = outImg->GetOrigin();
        const typename TInputImage::DirectionType& direction = outImg->GetDirection();
        int direction_sign(0);
        for (unsigned int i = 0; i < TInputImage::ImageDimension; ++i)
        {
            if (direction[i][i] < 0)
            {
                direction_sign = -1;
            }
            else
            {
                direction_sign = 1;
            }
            // Final image size
            m_ImageIOs[ni]->SetDimensions(i, outRegion.GetSize(i));
            m_ImageIOs[ni]->SetSpacing(i, spacing[i] * direction_sign);
            m_ImageIOs[ni]->SetOrigin(i, origin[i]);

            vnl_vector<double> axisDirection(TInputImage::ImageDimension);
            // Please note: direction cosines are stored as columns of the
            // direction matrix
            for (unsigned int j = 0; j < TInputImage::ImageDimension; ++j)
            {
                axisDirection[j] = direction[j][i] * direction_sign;
            }
            m_ImageIOs[ni]->SetDirection(i, axisDirection);
        }

        m_ImageIOs[ni]->SetUseCompression(m_UseCompression);
        m_ImageIOs[ni]->SetMetaDataDictionary(inImg->GetMetaDataDictionary());

        /** Create Image file */
        // Setup the image IO for writing.
        //
        //NMDebugAI(<< "Image information for ImageIO ..." << endl);
        //NMDebugAI(<< "  origin: " << origin[0] << ", " << origin[1] << endl);
        //NMDebugAI(<< "  spacing: " << spacing[0] << ", " << spacing[1] << endl);
        //NMDebugAI(<< "  region ... " << endl);
        //outputRegion.Print(std::cout, itk::Indent(nmlog::nmindent + 4));

        m_ImageIOs[ni]->WriteImageInformation();

    }

    // Notify START event observers
    this->InvokeEvent(itk::StartEvent());

    this->UpdateProgress(0);

    /*  Pulling a piece of the input image(s) through the pipeline
     *  and then writing it out; if the input component happens to
     *  produce additional images, i.e. output[1..n-1], we grab them
     *  and write them out too. Therebye, we're assuming that the
     *  input component prepares all of its outpus based on the
     *  requested region for streaming (streamRegion);
     *  NOTE: this does not work for
     */
    for (m_CurrentDivision = 0;
         m_CurrentDivision < m_NumberOfDivisions && !this->GetAbortGenerateData();
         m_CurrentDivision++, m_DivisionProgress = 0, this->UpdateFilterProgress())
    {
        InputImageType* inImg = const_cast<InputImageType*>(this->GetInput(0));
        InputImageRegionType streamRegion = inImg->GetLargestPossibleRegion();
        InputImageRegionType inLPR = streamRegion;
        m_StreamingManager->GetSplitter()->GetSplit(m_CurrentDivision, m_NumberOfDivisions, streamRegion);

        inImg->SetRequestedRegion(streamRegion);
        inImg->PropagateRequestedRegion();
        inImg->UpdateOutputData();

        for (int ni=0; ni < m_NumberOfInputs; ++ni)
        {

            if (m_ImageIOs[ni].IsNull())
            {
                continue;
            }

            m_CurrentWriteImage = ni;
            if (ni > 0)
            {
                inImg = const_cast<InputImageType*>(this->GetInput(ni));
                streamRegion = inImg->GetRequestedRegion();
            }

            // Write the whole image
            itk::ImageIORegion ioRegion(TInputImage::ImageDimension);
            for (unsigned int i = 0; i < TInputImage::ImageDimension; ++i)
            {
                ioRegion.SetSize(i, streamRegion.GetSize(i));
                ioRegion.SetIndex(i, streamRegion.GetIndex(i));
            }
            this->SetIORegion(ioRegion);
            m_ImageIOs[ni]->SetIORegion(m_IORegion);

            // Start writing stream region in the image file
            if (m_WriteImage)
            {
                this->GenerateData();
            }
        }
    }


    /** build overviews */
    if (m_ResamplingType.compare("NONE") != 0 && m_WriteImage)
    {
        this->BuildOverviews();
    }

    /**
   * If we ended due to aborting, push the progress up to 1.0 (since
   * it probably didn't end there)
   */
    // ONLY WHEN RAT AVAILABLE AND HAS NOT BEEN WRITTEN WITH IMAGE (above)
    for (int inimg=0; inimg < m_NumberOfInputs; ++inimg)
    {
        if (!m_WriteImage && m_InputRATs[inimg].IsNotNull())
        {
            otb::GDALRATImageIO* gio = dynamic_cast<otb::GDALRATImageIO*>(m_ImageIOs[inimg].GetPointer());
            otb::NetCDFIO* nio = dynamic_cast<otb::NetCDFIO*>(m_ImageIOs[inimg].GetPointer());
            if (nio != nullptr)
            {
                nio->WriteRAT(m_InputRATs[inimg]);
            }
            else if (gio)
            {
                NMProcDebug(<< "writing ONLY RAT!");
                gio->WriteRAT(m_InputRATs[inimg]);
            }
            if (m_InputRATs[inimg]->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE)
            {
                otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(m_InputRATs[inimg].GetPointer());
                if (sqltab)
                {
                    sqltab->CloseTable();
                }
            }
        }
    }

    if (!this->GetAbortGenerateData())
    {
        this->UpdateProgress(1.0);
    }

    // Notify end event observers
    this->InvokeEvent(itk::EndEvent());

    /**
   * Now we have to mark the data as up to data.
   */
    for (idx = 0; idx < this->GetNumberOfOutputs(); ++idx)
    {
        if (this->GetOutput(idx))
        {
            this->GetOutput(idx)->DataHasBeenGenerated();
        }
    }

    // Write the image keyword list if any
    // ossimKeywordlist geom_kwl;
    // ImageKeywordlist otb_kwl;

    // itk::MetaDataDictionary dict = this->GetInput()->GetMetaDataDictionary();
    // itk::ExposeMetaData<ImageKeywordlist>(dict, MetaDataKey::OSSIMKeywordlistKey, otb_kwl);
    // otb_kwl.convertToOSSIMKeywordlist(geom_kwl);
    //FIXME: why nothing is done with otb_kwl in that case???
    // If required, put a call to WriteGeometry() here

    /**
   * Release any inputs if marked for release
   */

    this->ReleaseInputs();

    // Mark that we are no longer updating the data in this filter
    this->m_Updating = false;

    // close the GDALDataset
    for (int num=0; num < m_NumberOfInputs; ++num)
    {
        //otb::GDALRATImageIO* agio = static_cast<otb::GDALRATImageIO*>(m_ImageIOs[num].GetPointer());
        //if (agio != nullptr)
        //{
        //    agio->CloseDataset();
        //}

        // provenance information
        std::stringstream sstr;

        // create table entity
        std::vector<std::string> args;
        std::vector<std::string> attrs;

        std::string fn = this->m_FileNames[num];//GetFileName();
        std::size_t pos1 = fn.find_last_of("/\\");
        std::string basename = fn.substr(pos1+1);
        std::size_t pos2 = basename.find_last_of('.');
        std::string ext = basename.substr(pos2+1);

        // provn used by
        sstr.str("");
        sstr << "img:" << basename;
        args.push_back(sstr.str());

        //  sstr.str("");
        //  sstr << "nm:FileName=\"" << this->GetFileName();
        //  attrs.push_back(sstr.str());

        //  sstr.str("");
        //  sstr << "nm:ImageFormat=\"" << ext;
        //  attrs.push_back(sstr.str());

        NMProcProvN(itk::NMLogEvent::NM_PROV_GENERATION, args, attrs);

        args.clear();
        attrs.clear();
        sstr.str("");
        sstr << "img:" << basename;
        args.push_back(sstr.str());

        sstr.str("");
        //sstr << "nm:FileName=\"" << this->GetFileName() << "\"";
        sstr << "nm:FileName=\"" << this->m_FileNames[num] << "\"";
        attrs.push_back(sstr.str());

        sstr.str("");
        sstr << "nm:Format=\"" << ext << "\"";
        attrs.push_back(sstr.str());

        NMProcProvN(itk::NMLogEvent::NM_PROV_ENTITY, args, attrs);
    }
}

//---------------------------------------------------------

/**
 *
 */
template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::GenerateData(void)
{
    const InputImageType * input = this->GetInput(m_CurrentWriteImage);

    // Make sure that the image is the right type and no more than
    // four components.
    typedef typename InputImageType::PixelType ImagePixelType;

    if (strcmp(input->GetNameOfClass(), "VectorImage") == 0)
    {
        typedef typename InputImageType::InternalPixelType VectorImagePixelType;
        m_ImageIOs[m_CurrentWriteImage]->SetPixelTypeInfo(typeid(VectorImagePixelType));

        typedef typename InputImageType::AccessorFunctorType AccessorFunctorType;
        m_ImageIOs[m_CurrentWriteImage]->SetNumberOfComponents(AccessorFunctorType::GetVectorLength(input));
    }
    else
    {
        // Set the pixel and component type; the number of components.
        m_ImageIOs[m_CurrentWriteImage]->SetPixelTypeInfo(typeid(ImagePixelType));
    }

    // Setup the image IO for writing.
    //
    //okay, now extract the data as a raw buffer pointer
    const void* dataPtr = (const void*) input->GetBufferPointer();
    m_ImageIOs[m_CurrentWriteImage]->Write(dataPtr);

    if (m_WriteGeomFile)
    {
        ImageKeywordlist otb_kwl;
        itk::MetaDataDictionary dict = this->GetInput(m_CurrentWriteImage)->GetMetaDataDictionary();
        itk::ExposeMetaData<ImageKeywordlist>(dict, MetaDataKey::OSSIMKeywordlistKey, otb_kwl);
        WriteGeometry(otb_kwl, this->m_FileNames[m_CurrentWriteImage]);
    }

    //this->ReleaseInputs();
}

template<class TInputImage>
void
StreamingRATImageFileWriter<TInputImage>
::VerifyInputInformation()
{
    // not doing anything here
}

} // end namespace otb

#endif
