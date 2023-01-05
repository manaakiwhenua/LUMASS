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
/*=========================================================================

    NOTE: this is an adjusted version of the ImageFileReader class
          to allow for reading and writing images GDAL-/rasdaman-based images
          supporting raster attribute tables


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
#ifndef __otbNMImageReader_txx
#define __otbNMImageReader_txx
#include "otbNMImageReader.h"

#include "nmlog.h"

#include "itkMetaDataObject.h"

#include "otbMacro.h"
#include "otbSystem.h"
#include "otbImageIOFactory.h"
//#include "otbImageKeywordlist.h"
#include "otbMetaDataKey.h"
#include "otbCurlHelper.h"

#include "nmNetCDFIO.h"
#include "otbGDALRATImageIO.h"
#include "otbAttributeTable.h"
#ifdef BUILD_RASSUPPORT
#include "otbRasdamanImageIO.h"
#endif

#include "itkDefaultConvertPixelTraits.h"

#include <itksys/SystemTools.hxx>
#include <fstream>

namespace otb
{

template <class TOutputImage>
NMImageReader<TOutputImage>
::NMImageReader()
    : otb::ImageFileReader<TOutputImage>(),
      mbRasMode(false),
      m_RATSupport(true),
      m_RATType(otb::AttributeTable::ATTABLE_TYPE_RAM),
      m_Curl(CurlHelper::New()),
      //m_FilenameHelper(FNameHelperType::New()),
      m_OverviewIdx(-1),
      m_ZSliceIdx(0),
      m_UseUserLargestPossibleRegion(false),
      m_RAT(0),
      m_RGBMode(false),
      m_DatasetNumber(0),
      m_DbRATReadOnly(false)
#ifdef BUILD_RASSUPPORT
    , mRasconn(0)
#endif

{
}


template <class TOutputImage>
NMImageReader<TOutputImage>
::~NMImageReader()
{
}

template <class TOutputImage>
void NMImageReader<TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
    Superclass::PrintSelf(os, indent);
    otb::NMImageReader<TOutputImage>* myself =
            const_cast<otb::NMImageReader<TOutputImage>* >(this);
    otb::ImageIOBase* iob = myself->GetImageIO();
    //const_cast<otb::ImageIOBase*>(myself->GetImageIO());

    if (iob != 0)
    {
        os << indent << "ImageIO: \n";
        iob->Print(os, indent.GetNextIndent());
    }
    else
    {
        os << indent << "ImageIO: (null)" << "\n";
    }

    //os << indent << "UserSpecifiedImageIO flag: " << this->m_UserSpecifiedImageIO << "\n";
    os << indent << "m_FileName: " << this->GetFileName() << "\n";

    if (this->m_RAT.IsNotNull())
        this->m_RAT->PrintStructure(os, indent);
}

template <class TOutputImage>
void
NMImageReader<TOutputImage>
::BuildOverviews(const std::string& method)
{
    GDALRATImageIO* gio = dynamic_cast<GDALRATImageIO*>(this->GetImageIO());
    NetCDFIO* nio = dynamic_cast<NetCDFIO*>(this->GetImageIO());
    if (gio != nullptr)
    {
        gio->BuildOverviews(method);
    }
    else if (nio != nullptr)
    {
        nio->BuildOverviews(method);
    }
}

template <class TOutputImage>
void
NMImageReader<TOutputImage>
::GenerateData()
{

    typename TOutputImage::Pointer output = this->GetOutput();

    // allocate the output buffer
    output->SetBufferedRegion( output->GetRequestedRegion() );
    output->Allocate();

    //otbMsgDebugMacro( <<"NMImageReader<TOutputImage>::GenerateData : ");
    //otbMsgDebugMacro( <<" output->GetRequestedRegion() : "<<output->GetRequestedRegion());

    // Test if the file exist and if it can be open.
    // and exception will be thrown otherwise.
    if (!this->mbRasMode)
    {
        this->TestFileExistanceAndReadability();
    }

    // Tell the ImageIO to read the file
    //
    OutputImagePixelType *buffer =
            output->GetPixelContainer()->GetBufferPointer();
    this->GetImageIO()->SetFileName(this->GetFileName());

    itk::ImageIORegion ioRegion(TOutputImage::ImageDimension);

    itk::ImageIORegion::SizeType ioSize = ioRegion.GetSize();
    itk::ImageIORegion::IndexType ioStart = ioRegion.GetIndex();

    /* Init IORegion with size or streaming size */
    SizeType dimSize;
    for (unsigned int i=0; i<TOutputImage::ImageDimension; ++i)
    {
        if (i < this->GetImageIO()->GetNumberOfDimensions())
        {
            if ( !this->GetImageIO()->CanStreamRead() )
                dimSize[i] = this->GetImageIO()->GetDimensions(i);
            else
                dimSize[i] = output->GetRequestedRegion().GetSize()[i];
        }
        else
        {
            // Number of dimensions in the output is more than number of dimensions
            // in the ImageIO object (the file).  Use default values for the size,
            // spacing, and origin for the final (degenerate) dimensions.
            dimSize[i] = 1;
        }
    }

    for (unsigned int i = 0; i < dimSize.GetSizeDimension(); ++i)
    {
        ioSize[i] = dimSize[i];
    }

    typedef typename TOutputImage::IndexType   IndexType;
    IndexType start;
    if ( !this->GetImageIO()->CanStreamRead() )  start.Fill(0);
    else start = output->GetRequestedRegion().GetIndex();
    for (unsigned int i = 0; i < start.GetIndexDimension(); ++i)
    {
        ioStart[i] = start[i];
    }

    ioRegion.SetSize(ioSize);
    ioRegion.SetIndex(ioStart);


    this->GetImageIO()->SetIORegion(ioRegion);

    typedef itk::DefaultConvertPixelTraits<typename TOutputImage::IOPixelType> ConvertIOPixelTraits;
    typedef itk::DefaultConvertPixelTraits<typename TOutputImage::PixelType> ConvertPixelTraits;

    if (this->GetImageIO()->GetComponentTypeInfo()
            == typeid(typename ConvertPixelTraits::ComponentType)
            && (this->GetImageIO()->GetNumberOfComponents()
                == ConvertIOPixelTraits::GetNumberOfComponents()))
    {
        // Have the ImageIO read directly into the allocated buffer
        this->GetImageIO()->Read(buffer);
        return;
    }
    else // a type conversion is necessary
    {
        // note: char is used here because the buffer is read in bytes
        // regardless of the actual type of the pixels.
        ImageRegionType region = output->GetBufferedRegion();

        // Adapt the image size with the region
        std::streamoff nbBytes = (this->GetImageIO()->GetComponentSize() * this->GetImageIO()->GetNumberOfComponents())
                * static_cast<std::streamoff>(region.GetNumberOfPixels());

        char * loadBuffer = new char[nbBytes];

        otbMsgDevMacro(<< "size of Buffer to RasdamanImageIO::read = " << nbBytes << " = \n"
                       << "ComponentSize ("<< this->GetImageIO()->GetComponentSize() << ") x " \
                       << "Nb of Component (" << this->GetImageIO()->GetNumberOfComponents() << ") x " \
                       << "Nb of Pixel to read (" << region.GetNumberOfPixels() << ")" );

        this->GetImageIO()->Read(loadBuffer);

#if defined(OTB_VERSION_SIX)
        this->DoNMConvertBuffer(loadBuffer, region.GetNumberOfPixels());
#else
        this->DoConvertBuffer(loadBuffer, region.GetNumberOfPixels());
#endif
        delete[] loadBuffer;
    }
}


template <class TOutputImage>
void
NMImageReader<TOutputImage>
::EnlargeOutputRequestedRegion(itk::DataObject *output)
{
    typename TOutputImage::Pointer out = dynamic_cast<TOutputImage*>(output);

    // the ImageIO object cannot stream, then set the RequestedRegion to the
    // LargestPossibleRegion
    if (!this->GetImageIO()->CanStreamRead())
    {
        if (out)
        {
            out->SetRequestedRegion( out->GetLargestPossibleRegion() );
        }
        else
        {
            throw ImageFileReaderException(__FILE__, __LINE__,
                                           "Invalid output object type");
        }
    }
}

#ifdef BUILD_RASSUPPORT
template <class TOutputImage>
void
NMImageReader<TOutputImage>
::SetRasdamanConnector(RasdamanConnector* rasconn)
{
    this->mRasconn = rasconn;
}
#endif

template <class TOutputImage>
void
NMImageReader<TOutputImage>
::GenerateOutputInformation(void)
{

    typename TOutputImage::Pointer output = this->GetOutput();

    itkDebugMacro(<<"Reading file for GenerateOutputInformation()" << this->GetFileName());

    // Check to see if we can read the file given the name or prefix
    //
    if ( this->GetFileName() == "" )
    {
        throw ImageFileReaderException(__FILE__, __LINE__, "FileName must be specified");
        return;
    }

    std::string lFileNameOssimKeywordlist = this->GetFileName();

    // set either GDALRATImageIO or RasdamanImageIO for this reader
    // which marks the ImageIO as 'UserSpecified' to prevent any
    // 'automatic' change further down the track

    otb::NetCDFIO* nio = dynamic_cast<otb::NetCDFIO*>(this->GetImageIO());
    if (nio == 0)
    {
        std::string fn = this->GetFileName();
        if (fn.find(".nc") != std::string::npos)
        {
            otb::NetCDFIO::Pointer pnio = otb::NetCDFIO::New();
            pnio->SetFileName(fn.c_str());
            this->SetImageIO(pnio);

        }
    }


    nio = dynamic_cast<otb::NetCDFIO*>(this->GetImageIO());
    otb::GDALRATImageIO* gio = dynamic_cast<otb::GDALRATImageIO*>(this->GetImageIO());
#ifdef BUILD_RASSUPPORT
    otb::RasdamanImageIO* rio = dynamic_cast<otb::RasdamanImageIO*>(this->GetImageIO());
    if (rio == nullptr && gio == nullptr && nio == nullptr)
    {
        std::string tmpFN = this->GetFileName();
        if (this->mRasconn != 0 && tmpFN.find('.') == std::string::npos)
        {
            if (this->mRasconn->getRasConnection() != 0)
            {
                this->mbRasMode = true;
            }
        }

        if (mbRasMode)
        {
            otb::RasdamanImageIO::Pointer ario = dynamic_cast<otb::RasdamanImageIO*>(this->GetImageIO());
            if (ario.IsNull())
            {
                if (!this->mRasconn)
                {
                    this->Print(std::cerr);
                    ImageFileReaderException e(__FILE__, __LINE__);
                    std::ostringstream msg;
                    msg
                            << " How inconvenient, someone forgot to set the RasdamanConnector object!"
                            << std::endl;
                    msg << " We can't do anything without it!!" << std::endl;
                    e.SetDescription(msg.str().c_str());
                    throw e;
                    return;
                }

                ario = otb::RasdamanImageIO::New();
                if (ario.IsNotNull())
                {
                    ario->setRasdamanConnector(this->mRasconn);
                    ario->SetFileName(this->GetFileName());
                    this->SetImageIO(dynamic_cast<otb::ImageIOBase*>(ario.GetPointer()));
                }
            }
        }
        else
#else
    if (nio == nullptr && gio == nullptr)
    {
#endif
        {
            // Find real image file name
            // !!!!  Update FileName
            std::string lFileName;
            bool found = GetGdalReadImageFileName(this->GetFileName(), lFileName);
            if (found == false)
            {
                otbMsgDebugMacro(
                        << "Filename was NOT unknown. May be recognized by a Image factory ! ");
            }
            // Update FileName
            this->SetFileName(lFileName);

            // Test if the file exists and if it can be opened.
            // An exception will be thrown otherwise.
            // We catch the exception because some ImageIO's may not actually
            // open a file. Still reports file error if no ImageIO is loaded.

            try
            {
                m_ExceptionMessage = "";
                this->TestFileExistanceAndReadability();
            }
            catch (itk::ExceptionObject & err)
            {
                m_ExceptionMessage = err.GetDescription();
            }


            otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(this->GetImageIO());
            if (gio.IsNull() && nio == nullptr)
            {
                gio = otb::GDALRATImageIO::New();
                if (gio.IsNotNull())
                {
                    this->SetImageIO(dynamic_cast<otb::ImageIOBase*>(gio.GetPointer()));
                    gio->SetFileName(this->GetFileName());
                    gio->SetOverviewIdx(this->m_OverviewIdx);
                    gio->SetRATSupport(this->m_RATSupport);
                    gio->SetRATType(this->m_RATType);
                    gio->SetRGBMode(m_RGBMode);
                    gio->SetDbRATReadOnly(this->m_DbRATReadOnly);
                    this->m_RAT = gio->ReadRAT(1);
                }
            }
        }

    }
//    else
//    {
//        gio->SetRATType(m_RATType);
//        gio->SetDbRATReadOnly(m_DbRATReadOnly);
//    }

    if ( this->GetImageIO() == nullptr )
    {
        this->Print( std::cerr );
        ImageFileReaderException e(__FILE__, __LINE__);
        std::ostringstream msg;
        msg << " Mmmh ... failed to set an appropriate ImageIO - I'm sory!"
            << std::endl;
        e.SetDescription(msg.str().c_str());
        throw e;
        return;
    }


    // Special actions for the gdal image IO
    if (strcmp(this->GetImageIO()->GetNameOfClass(), "GDALRATImageIO") == 0)
    {
        typename GDALRATImageIO::Pointer imageIO =
                dynamic_cast<GDALRATImageIO*>(this->GetImageIO());

        imageIO->SetRATType(m_RATType);
        imageIO->SetDbRATReadOnly(m_DbRATReadOnly);


        // Hint the IO whether the OTB image type takes complex pixels
        // this will determine the strategy to fill up a vector image
        OutputImagePixelType dummy;
        imageIO->SetIsComplex(PixelIsComplex(dummy));

        // VectorImage ??
        if (strcmp(output->GetNameOfClass(), "VectorImage") == 0)
        {
            imageIO->SetIsVectorImage(true);
        }
        else
        {
            imageIO->SetIsVectorImage(false);
        }

        imageIO->SetDatasetNumber(m_DatasetNumber);
        //        // Pass the dataset number (used for hdf files for example)
        //        if (m_FilenameHelper->SubDatasetIndexIsSet())
        //        {
        //            imageIO->SetDatasetNumber(m_FilenameHelper->GetSubDatasetIndex());
        //        }
        //        else
        //        {
        //            imageIO->SetDatasetNumber(m_AdditionalNumber);
        //        }

    }

    // Got to allocate space for the image. Determine the characteristics of
    // the image.
    //
    //this->GetImageIO()->SetFileName(this->GetFileName());
    this->GetImageIO()->ReadImageInformation();
    if (gio != nullptr)
    {
        m_UpperLeftCorner = gio->GetUpperLeftCorner();
    }

#ifdef BUILD_RASSUPPORT
    if (rio != nullptr)
    {
        m_UpperLeftCorner = rio->getUpperLeftCorner();
    }
#endif

    if (nio != nullptr)
    {
        m_UpperLeftCorner = nio->getUpperLeftCorner();
    }

    SizeType dimSize;
    double spacing[ TOutputImage::ImageDimension ];
    double origin[ TOutputImage::ImageDimension ];
    typename TOutputImage::DirectionType direction;
    std::vector<double> axis;
    double spacing_sign (0);

    for (unsigned int i=0; i<TOutputImage::ImageDimension; ++i)
    {
        if ( i < this->GetImageIO()->GetNumberOfDimensions() )
        {
            dimSize[i] = this->GetImageIO()->GetDimensions(i);
            if (this->GetImageIO()->GetSpacing(i) < 0)
            {
                spacing_sign = -1;
            }
            else
            {
                spacing_sign = 1;
            }
            spacing[i] = this->GetImageIO()->GetSpacing(i) * spacing_sign;
            origin[i] = this->GetImageIO()->GetOrigin(i);
            // Please note: direction cosines are stored as columns of the
            // direction matrix
            axis = this->GetImageIO()->GetDirection(i);
            for (unsigned j=0; j<TOutputImage::ImageDimension; ++j)
            {
                if (j < this->GetImageIO()->GetNumberOfDimensions())
                {
                    direction[j][i] = axis[j] * spacing_sign;
                }
                else
                {
                    direction[j][i] = 0.0;
                }
            }
        }
        else
        {
            // Number of dimensions in the output is more than number of dimensions
            // in the ImageIO object (the file).  Use default values for the size,
            // spacing, origin and direction for the final (degenerate) dimensions.
            dimSize[i] = 1;
            spacing[i] = 1.0;
            origin[i] = 0.5;
            for (unsigned j = 0; j < TOutputImage::ImageDimension; ++j)
            {
                if (i == j)
                {
                    direction[j][i] = 1.0;
                }
                else
                {
                    direction[j][i] = 0.0;
                }
            }
        }
    }

    output->SetSpacing( spacing );     // Set the image spacing
    output->SetOrigin( origin );       // Set the image origin
    output->SetDirection( direction ); // Set the image direction cosines


    //Copy MetaDataDictionary from instantiated reader to output image.
    output->SetMetaDataDictionary(this->GetImageIO()->GetMetaDataDictionary());
    this->SetMetaDataDictionary(this->GetImageIO()->GetMetaDataDictionary());

    typedef typename TOutputImage::IndexType   IndexType;

    IndexType start;
    start.Fill(0);

    ImageRegionType region;
    if (m_UseUserLargestPossibleRegion)
    {
        // NOTE: the user is responsible for specifying a
        // region which is a sub region of the actual LPR!!
        region.SetSize(m_UserLargestPossibleRegion.GetSize());
        region.SetIndex(m_UserLargestPossibleRegion.GetIndex());
    }
    else
    {
        region.SetSize(dimSize);
        region.SetIndex(start);
    }

    // THOMAS : ajout
    // If a VectorImage, this requires us to set the
    // VectorLength before allocate
    if ( strcmp( output->GetNameOfClass(), "VectorImage" ) == 0 )
    {
        typedef typename TOutputImage::AccessorFunctorType AccessorFunctorType;
        AccessorFunctorType::SetVectorLength( output, this->GetImageIO()->GetNumberOfComponents() );
    }

    output->SetLargestPossibleRegion(region);

    // ==================================================================
    // log provenance information
    // provenance information
    std::stringstream sstr;

    // create table entity
    std::vector<std::string> args;
    std::vector<std::string> attrs;

    std::string fn = this->GetFileName();
    std::size_t pos1 = fn.find_last_of("/\\");
    std::string basename = fn.substr(pos1+1);
    std::size_t pos2 = basename.find_last_of('.');
    std::string ext = basename.substr(pos2+1);

    sstr << "img:" << basename;
    args.push_back(sstr.str());

    sstr.str("");
    sstr << "nm:FileName=\"" << this->GetFileName() << "\"";
    attrs.push_back(sstr.str());

    sstr.str("");
    sstr << "nm:Format=\"" << ext << "\"";
    attrs.push_back(sstr.str());

    NMProcProvN(itk::NMLogEvent::NM_PROV_ENTITY, args, attrs);

    // provn used by
    args.clear();
    attrs.clear();

    sstr.str("");
    sstr << "img:" << basename;
    args.push_back(sstr.str());

    NMProcProvN(itk::NMLogEvent::NM_PROV_USAGE, args, attrs);

}

template<class TOutputImage>
void
NMImageReader<TOutputImage>::SetOverviewIdx(int ovvidx)
{
    if (!mbRasMode)
    {
        if (ovvidx != this->m_OverviewIdx)
        {
            this->m_OverviewIdx = ovvidx;
            otb::NetCDFIO* nio = dynamic_cast<otb::NetCDFIO*>(this->GetImageIO());
            otb::GDALRATImageIO* gio = dynamic_cast<otb::GDALRATImageIO*>(
                        this->GetImageIO());
            if (gio != nullptr)
            {
                gio->SetOverviewIdx(ovvidx);
                this->Modified();
            }
            else if (nio != nullptr)
            {
                nio->SetOverviewIdx(ovvidx);
                this->Modified();
            }
        }
    }
}

template<class TOutputImage>
void
NMImageReader<TOutputImage>::SetZSliceIdx(int slindex)
{
    if (!mbRasMode)
    {
        if (slindex != this->m_ZSliceIdx)
        {
            this->m_ZSliceIdx = slindex;
            otb::NetCDFIO* nio = dynamic_cast<otb::NetCDFIO*>(this->GetImageIO());
            if (nio != nullptr)
            {
                nio->SetZSliceIdx(slindex);
                this->Modified();
            }
        }
    }
}


template <class TOutputImage>
void
NMImageReader<TOutputImage>
::TestFileExistanceAndReadability()
{

  // check first and foremost for a netcdf file
  std::string tmpFN = this->GetFileName();
  if (tmpFN.find(".nc") != std::string::npos)
  {
      otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(this->GetImageIO());
      if (nio.IsNotNull())
      {
          if (nio->CanReadFile(tmpFN.c_str()))
          {
              return;
          }
      }
  }

  otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(this->GetImageIO());
  if (gio.IsNotNull())
  {
      if (gio->CanReadFile(tmpFN.c_str()))
      {
          return;
      }
  }

  otb::ImageFileReaderException e(__FILE__, __LINE__);
  std::ostringstream msg;
  if (tmpFN.find(".nc") != std::string::npos)
  {
      msg << "otbNMImageReader: ERROR - LUMASS failed to read the NetCDF image file "
          << "'" << tmpFN << "'! Please make sure "
          << "the filename points to valid and readable NetCDF variable within the given file!";
  }
  else if (tmpFN.find(".kea") != std::string::npos)
  {
      msg << "otbNMImageReader: ERROR - LUMASS failed to read the KEA image file "
          << "'" << tmpFN << "'! Please make sure the KEA GDAL plugin is installed, "
          << " and the filename points to valid and readable file!";
  }
  else
  {
      msg << "otbNMImageReader: ERROR - LUMASS failed to read the image file "
          << "'" << tmpFN << "'! Please make sure the filename points to valid, "
          << "readable, and supported image file!";
  }
  e.SetDescription(msg.str().c_str());
  throw e;

  //// Test if the file a server name
  //if  (tmpFN.find("http") == 0)
  //      //      [0] == 'h'
  //      //       && this->GetFileName()[1] == 't'
  //      //       && this->GetFileName()[2] == 't'
  //      //       && this->GetFileName()[3] == 'p')
  //  {
  //  // If the url is not available
  //  if (!m_Curl->TestUrlAvailability(this->GetFileName()))
  //    {
  //    otb::ImageFileReaderException e(__FILE__, __LINE__);
  //    std::ostringstream msg;
  //    msg << "File name is an http address, but curl fails to connect to it "
  //        << std::endl << "Filename = " << this->GetFileName()
  //        << std::endl;
  //    e.SetDescription(msg.str().c_str());
  //    throw e;
  //    }
  //  return;
  //  }

  //// Test if we have an hdf file with dataset spec
  //std::string realfile(this->GetFileName());
  //unsigned int datasetNum;
  //if (System::ParseHdfFileName(this->GetFileName(), realfile, datasetNum))
  //{
  //    otbMsgDevMacro(<< "HDF name with dataset specification detected"); otbMsgDevMacro(<< " - " << realfile); otbMsgDevMacro(<< " - " << datasetNum);
  //    this->SetFileName(realfile);
  //    m_DatasetNumber = datasetNum;
  //}

  //// Test if the file exists.
  //if (!itksys::SystemTools::FileExists(this->GetFileName()))
  //  {
  //  otb::ImageFileReaderException e(__FILE__, __LINE__);
  //  std::ostringstream msg;
  //  msg << "The file doesn't exist. "
  //      << std::endl << "Filename = " << this->GetFileName()
  //      << std::endl;
  //  e.SetDescription(msg.str().c_str());
  //  throw e;
  //  return;
  //  }
  //
  //// Test if the file can be open for reading access.
  ////Only if m_FileName specify a filename (not a dirname)
  //if (itksys::SystemTools::FileExists(this->GetFileName(), true) == true)
  //  {
  //  std::ifstream readTester;
  //  readTester.open(this->GetFileName());
  //  if (readTester.fail())
  //    {
  //    readTester.close();
  //    std::ostringstream msg;
  //    msg << "The file couldn't be opened for reading. "
  //        << std::endl << "Filename: " << this->GetFileName()
  //        << std::endl;
  //    otb::ImageFileReaderException e(__FILE__, __LINE__, msg.str().c_str(), ITK_LOCATION);
  //    throw e;
  //    return;
  //
  //    }
  //  readTester.close();
  //  }
}

template<class TOutputImage>
otb::AttributeTable::Pointer
NMImageReader< TOutputImage >
::GetAttributeTable(int band)
{

    if (this->GetImageIO() == 0)
        this->GenerateOutputInformation();

    // let's see if we've got already the right table
    if (this->m_RAT.IsNotNull())
    {
        if (this->m_RAT->GetBandNumber() == band)
            return this->m_RAT;
    }
    else
    {
#ifdef BUILD_RASSUPPORT
        if (mbRasMode)
        {
            otb::RasdamanImageIO* rio = dynamic_cast<otb::RasdamanImageIO*>(
                        this->GetImageIO());

            if (rio != 0)
            {
                this->m_RAT = rio->getRasterAttributeTable(band);
                return this->m_RAT;
            }
        }
        else
    #endif
        {
            otb::GDALRATImageIO* gio = dynamic_cast<otb::GDALRATImageIO*>(
                        this->GetImageIO());

            if (gio != 0)
            {
                gio->SetRATSupport(m_RATSupport);
                gio->SetRATType(m_RATType);
                gio->SetDbRATReadOnly(m_DbRATReadOnly);
                this->m_RAT = gio->ReadRAT(band);
                return this->m_RAT;
            }
        }
    }

    return 0;
}

template <class TOutputImage>
bool
NMImageReader<TOutputImage>
::GetGdalReadImageFileName( const std::string & filename, std::string & GdalFileName )
{
    std::vector<std::string> listFileSearch;
    listFileSearch.push_back("DAT_01.001");
    listFileSearch.push_back("dat_01.001");// RADARSAT or SAR_ERS2
    listFileSearch.push_back("IMAGERY.TIF");
    listFileSearch.push_back("imagery.tif");//For format SPOT5TIF
    // Not recognised as a supported file format by GDAL.
    //        listFileSearch.push_back("IMAGERY.BIL");listFileSearch.push_back("imagery.bil");//For format SPOT5BIL
    listFileSearch.push_back("IMAG_01.DAT");
    listFileSearch.push_back("imag_01.dat");//For format SPOT4

    std::string str_FileName;
    bool fic_trouve(false);

    // Si c'est un repertoire, on regarde le contenu pour voir si c'est pas du RADARSAT, ERS
    std::vector<std::string> listFileFind;
    listFileFind = System::Readdir(filename);
    if ( listFileFind.empty() == false )
    {
        unsigned int cpt(0);
        while ( (cpt < listFileFind.size()) && (fic_trouve==false) )
        {
            str_FileName = std::string(listFileFind[cpt]);
            for (unsigned int i = 0; i < listFileSearch.size(); ++i)
            {
                if (str_FileName.compare(listFileSearch[i]) == 0)
                {
                    GdalFileName = std::string(filename)+str_FileName;//listFileSearch[i];
                    fic_trouve=true;
                }
            }
            cpt++;
        }
    }
    else
    {
        std::string strFileName(filename);

        std::string extension = itksys::SystemTools::GetFilenameLastExtension(strFileName);
        if ( (extension=="HDR") || (extension=="hdr") )
        {
            //Supprime l'extension
            GdalFileName = System::GetRootName(strFileName);
        }

        else
        {
            // Sinon le filename est le nom du fichier a ouvrir
            GdalFileName = std::string(filename);
        }
        fic_trouve=true;
    }
    otbMsgDevMacro(<<"lFileNameGdal : "<<GdalFileName.c_str());
    otbMsgDevMacro(<<"fic_trouve : "<<fic_trouve);
    return( fic_trouve );
}

#ifdef OTB_VERSION_SIX
template <class TOutputImage >
void
NMImageReader< TOutputImage >
::DoNMConvertBuffer(void* inputData,
                  size_t numberOfPixels)
{
  // get the pointer to the destination buffer
  OutputImagePixelType *outputData =
    this->GetOutput()->GetPixelContainer()->GetBufferPointer();

  // TODO:
  // Pass down the PixelType (RGB, VECTOR, etc.) so that any vector to
  // scalar conversion be type specific. i.e. RGB to scalar would use
  // a formula to convert to luminance, VECTOR to scalar would use
  // vector magnitude.


  // Create a macro as this code is a bit lengthy and repetitive
  // if the ImageIO pixel type is typeid(type) then use the ConvertPixelBuffer
  // class to convert the data block to TOutputImage's pixel type
  // see DefaultConvertPixelTraits and ConvertPixelBuffer

  // The first else if block applies only to images of type itk::VectorImage
  // VectorImage needs to copy out the buffer differently.. The buffer is of
  // type InternalPixelType, but each pixel is really 'k' consecutive pixels.

  typedef typename otb::DefaultConvertPixelTraits<OutputImagePixelType> ConvertPixelTraits;

#define OTB_CONVERT_BUFFER_IF_BLOCK(type)               \
 else if( this->GetImageIO()->GetComponentTypeInfo() == typeid(type) )   \
   {   \
   if( strcmp( this->GetOutput()->GetNameOfClass(), "VectorImage" ) == 0 ) \
     { \
     ConvertPixelBuffer<                                 \
      type,                                             \
      OutputImagePixelType,                             \
      ConvertPixelTraits                                \
      >                                                 \
      ::ConvertVectorImage(                             \
       static_cast<type*>(inputData),                  \
       this->GetImageIO()->GetNumberOfComponents(),             \
       outputData,                                     \
       numberOfPixels);              \
     } \
   else \
     { \
     ConvertPixelBuffer<                                 \
      type,                                             \
      OutputImagePixelType,                             \
      ConvertPixelTraits                                \
      >                                                 \
      ::Convert(                                        \
        static_cast<type*>(inputData),                  \
        this->GetImageIO()->GetNumberOfComponents(),             \
        outputData,                                     \
        numberOfPixels);              \
      } \
    }
#define OTB_CONVERT_CBUFFER_IF_BLOCK(type)               \
 else if( this->GetImageIO()->GetComponentTypeInfo() == typeid(type) )   \
   {  \
   if( strcmp( this->GetOutput()->GetNameOfClass(), "VectorImage" ) == 0 ) \
     { \
     if( (typeid(OutputImagePixelType) == typeid(std::complex<double>))     \
         || (typeid(OutputImagePixelType) == typeid(std::complex<float>))   \
         || (typeid(OutputImagePixelType) == typeid(std::complex<int>))     \
         || (typeid(OutputImagePixelType) == typeid(std::complex<short>)) ) \
       {\
       ConvertPixelBuffer<                                 \
        type::value_type,        \
        OutputImagePixelType,                             \
        ConvertPixelTraits                                \
        >                                                 \
        ::ConvertComplexVectorImageToVectorImageComplex(                             \
         static_cast<type*>(inputData),                \
         this->GetImageIO()->GetNumberOfComponents(),             \
         outputData,                                     \
         numberOfPixels); \
       }\
     else\
       {\
       ConvertPixelBuffer<                                 \
        type::value_type,        \
        OutputImagePixelType,                             \
        ConvertPixelTraits                                \
        >                                                  \
        ::ConvertComplexVectorImageToVectorImage(                             \
         static_cast<type*>(inputData),                \
         this->GetImageIO()->GetNumberOfComponents(),             \
         outputData,                                     \
         numberOfPixels);              \
       }\
     } \
   else \
     { \
     ConvertPixelBuffer<                                 \
      type::value_type,        \
      OutputImagePixelType,                             \
      ConvertPixelTraits                                \
      >                                                 \
      ::ConvertComplexToGray(                                        \
       static_cast<type*>(inputData),                  \
       this->GetImageIO()->GetNumberOfComponents(),             \
       outputData,                                     \
       numberOfPixels);              \
     } \
   }

  if(0)
    {
    }
  OTB_CONVERT_BUFFER_IF_BLOCK(unsigned char)
  OTB_CONVERT_BUFFER_IF_BLOCK(char)
  OTB_CONVERT_BUFFER_IF_BLOCK(unsigned short)
  OTB_CONVERT_BUFFER_IF_BLOCK(short)
  OTB_CONVERT_BUFFER_IF_BLOCK(unsigned int)
  OTB_CONVERT_BUFFER_IF_BLOCK(int)
  OTB_CONVERT_BUFFER_IF_BLOCK(unsigned long)
  OTB_CONVERT_BUFFER_IF_BLOCK(long)
  OTB_CONVERT_BUFFER_IF_BLOCK(unsigned long long)
  OTB_CONVERT_BUFFER_IF_BLOCK(long long)
  OTB_CONVERT_BUFFER_IF_BLOCK(float)
  OTB_CONVERT_BUFFER_IF_BLOCK(double)
  OTB_CONVERT_CBUFFER_IF_BLOCK(std::complex<short>)
  OTB_CONVERT_CBUFFER_IF_BLOCK(std::complex<int>)
  OTB_CONVERT_CBUFFER_IF_BLOCK(std::complex<float>)
  OTB_CONVERT_CBUFFER_IF_BLOCK(std::complex<double>)
  else
    {
    otb::ImageFileReaderException e(__FILE__, __LINE__);
    std::ostringstream msg;
    msg <<"Couldn't convert component type: "
        << std::endl << "    "
        << this->GetImageIO()->GetComponentTypeAsString(this->GetImageIO()->GetComponentType())
        << std::endl << "to one of: "
        << std::endl << "    " << typeid(unsigned char).name()
        << std::endl << "    " << typeid(char).name()
        << std::endl << "    " << typeid(unsigned short).name()
        << std::endl << "    " << typeid(short).name()
        << std::endl << "    " << typeid(unsigned int).name()
        << std::endl << "    " << typeid(int).name()
        << std::endl << "    " << typeid(unsigned long).name()
        << std::endl << "    " << typeid(long).name()
        << std::endl << "    " << typeid(unsigned long long).name()
        << std::endl << "    " << typeid(long long).name()
        << std::endl << "    " << typeid(float).name()
        << std::endl << "    " << typeid(double).name()
        << std::endl;
    e.SetDescription(msg.str().c_str());
    e.SetLocation(ITK_LOCATION);
    throw e;
    return;
    }
#undef OTB_CONVERT_BUFFER_IF_BLOCK
#undef OTB_CONVERT_CBUFFER_IF_BLOCK
}
#endif // OTB_VERSION_6

} //namespace otb

#endif
