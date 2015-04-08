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

#include "itkMetaDataObject.h"

#include "otbMacro.h"
#include "otbSystem.h"
#include "otbImageIOFactory.h"
#include "otbImageKeywordlist.h"
#include "otbMetaDataKey.h"

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
      m_Curl(CurlHelper::New()),
      //m_FilenameHelper(FNameHelperType::New()),
      m_OverviewIdx(-1),
      m_UseUserLargestPossibleRegion(false),
      m_RAT(0),
      m_RGBMode(false),
      m_DatasetNumber(0)
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

//template <class TOutputImage>
//void
//NMImageReader<TOutputImage>
//::SetFileName(const char* filename)
//{
//    this->m_FileName = filename;

//#ifdef BUILD_RASSUPPORT
//    if (this->mRasconn != 0 && m_FileName.find('.') == std::string::npos)
//    {
//        if (this->mRasconn->getRasConnection() != 0)
//        {
//            this->mbRasMode = true;
//        }
//    }

//    if (mbRasMode)
//    {
//        otb::RasdamanImageIO::Pointer ario = dynamic_cast<otb::RasdamanImageIO*>(this->GetImageIO());
//        if (ario.IsNull())
//        {
//            if (!this->mRasconn)
//            {
//                this->Print(std::cerr);
//                ImageFileReaderException e(__FILE__, __LINE__);
//                std::ostringstream msg;
//                msg
//                        << " How inconvenient, someone forgot to set the RasdamanConnector object!"
//                        << std::endl;
//                msg << " We can't do anything without it!!" << std::endl;
//                e.SetDescription(msg.str().c_str());
//                throw e;
//                return;
//            }

//            ario = otb::RasdamanImageIO::New();
//            if (ario.IsNotNull())
//            {
//                ario->setRasdamanConnector(this->mRasconn);
//                this->SetImageIO(dynamic_cast<otb::ImageIOBase*>(ario.GetPointer()));
//            }
//        }
//    }
//    else
//#endif
//    {
//        // Find real image file name
//        // !!!!  Update FileName
//        std::string lFileName;
//        bool found = GetGdalReadImageFileName(m_FileName.c_str(), lFileName);
//        if (found == false)
//        {
//            otbMsgDebugMacro(
//                    << "Filename was NOT unknown. May be recognized by a Image factory ! ");
//        }
//        // Update FileName
//        this->m_FileName = lFileName;

//        // Test if the file exists and if it can be opened.
//        // An exception will be thrown otherwise.
//        // We catch the exception because some ImageIO's may not actually
//        // open a file. Still reports file error if no ImageIO is loaded.

//        try
//        {
//            m_ExceptionMessage = "";
//            this->TestFileExistanceAndReadability();
//        }
//        catch (itk::ExceptionObject & err)
//        {
//            m_ExceptionMessage = err.GetDescription();
//        }


//        otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(this->GetImageIO());
//        if (gio.IsNull())
//        {
//            gio = otb::GDALRATImageIO::New();
//            if (gio.IsNotNull())
//            {
//                this->SetImageIO(dynamic_cast<otb::ImageIOBase*>(gio.GetPointer()));
//                gio->SetFileName(m_FileName);
//                gio->SetOverviewIdx(this->m_OverviewIdx);
//                gio->SetRATSupport(this->m_RATSupport);
//                gio->SetRGBMode(m_RGBMode);
//                this->m_RAT = gio->ReadRAT(1);
//            }
//        }
//    }
//}

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

        this->DoConvertBuffer(loadBuffer, region.GetNumberOfPixels());

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

#ifdef BUILD_RASSUPPORT
    otb::RasdamanImageIO* rio = static_cast<otb::RasdamanImageIO*>(this->GetImageIO());
    otb::GDALRATImageIO* gio = static_cast<otb::GDALRATImageIO*>(this->GetImageIO());
    if (rio == 0 && gio == 0)
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
    otb::GDALRATImageIO* gio = static_cast<otb::GDALRATImageIO*>(this->GetImageIO());
    if (gio == 0)
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
            if (gio.IsNull())
            {
                gio = otb::GDALRATImageIO::New();
                if (gio.IsNotNull())
                {
                    this->SetImageIO(dynamic_cast<otb::ImageIOBase*>(gio.GetPointer()));
                    gio->SetFileName(this->GetFileName());
                    gio->SetOverviewIdx(this->m_OverviewIdx);
                    gio->SetRATSupport(this->m_RATSupport);
                    gio->SetRGBMode(m_RGBMode);
                    this->m_RAT = gio->ReadRAT(1);
                }
            }
        }

    }

    if ( this->GetImageIO() == 0 )
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


    SizeType dimSize;
    double spacing[ TOutputImage::ImageDimension ];
    double origin[ TOutputImage::ImageDimension ];
    typename TOutputImage::DirectionType direction;
    std::vector<double> axis;

    for (unsigned int i=0; i<TOutputImage::ImageDimension; ++i)
    {
        if ( i < this->GetImageIO()->GetNumberOfDimensions() )
        {
            dimSize[i] = this->GetImageIO()->GetDimensions(i);
            spacing[i] = this->GetImageIO()->GetSpacing(i);
            origin[i]  = this->GetImageIO()->GetOrigin(i);
            // Please note: direction cosines are stored as columns of the
            // direction matrix
            axis = this->GetImageIO()->GetDirection(i);
            for (unsigned j=0; j<TOutputImage::ImageDimension; ++j)
            {
                if (j < this->GetImageIO()->GetNumberOfDimensions())
                {
                    direction[j][i] = axis[j];
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
            origin[i] = 0.0;
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

    // Update otb Keywordlist
    ImageKeywordlist otb_kwl = ReadGeometryFromImage(lFileNameOssimKeywordlist);

    // Update itk MetaData Dictionary
    itk::MetaDataDictionary& dict = this->GetImageIO()->GetMetaDataDictionary();

    // Don't add an empty ossim keyword list
    if( otb_kwl.GetSize() != 0 )
    {
        itk::EncapsulateMetaData<ImageKeywordlist>(dict,
                                                   MetaDataKey::OSSIMKeywordlistKey, otb_kwl);
    }

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
            otb::GDALRATImageIO* gio = static_cast<otb::GDALRATImageIO*>(
                        this->GetImageIO());
            if (gio)
            {
                gio->SetOverviewIdx(ovvidx);
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
  // Test if the file a server name
  std::string tmpFN = this->GetFileName();
  if  (tmpFN.find("http") == 0)
        //      [0] == 'h'
        //       && this->GetFileName()[1] == 't'
        //       && this->GetFileName()[2] == 't'
        //       && this->GetFileName()[3] == 'p')
    {
    // If the url is not available
    if (!m_Curl->TestUrlAvailability(this->GetFileName()))
      {
      otb::ImageFileReaderException e(__FILE__, __LINE__);
      std::ostringstream msg;
      msg << "File name is an http address, but curl fails to connect to it "
          << std::endl << "Filename = " << this->GetFileName()
          << std::endl;
      e.SetDescription(msg.str().c_str());
      throw e;
      }
    return;
    }

  // Test if we have an hdf file with dataset spec
  std::string realfile(this->GetFileName());
  unsigned int datasetNum;
  if (System::ParseHdfFileName(this->GetFileName(), realfile, datasetNum))
  {
      otbMsgDevMacro(<< "HDF name with dataset specification detected"); otbMsgDevMacro(<< " - " << realfile); otbMsgDevMacro(<< " - " << datasetNum);
      this->SetFileName(realfile);
      m_DatasetNumber = datasetNum;
  }

  // Test if the file exists.
  if (!itksys::SystemTools::FileExists(this->GetFileName()))
    {
    otb::ImageFileReaderException e(__FILE__, __LINE__);
    std::ostringstream msg;
    msg << "The file doesn't exist. "
        << std::endl << "Filename = " << this->GetFileName()
        << std::endl;
    e.SetDescription(msg.str().c_str());
    throw e;
    return;
    }

  // Test if the file can be open for reading access.
  //Only if m_FileName specify a filename (not a dirname)
  if (itksys::SystemTools::FileExists(this->GetFileName(), true) == true)
    {
    std::ifstream readTester;
    readTester.open(this->GetFileName());
    if (readTester.fail())
      {
      readTester.close();
      std::ostringstream msg;
      msg << "The file couldn't be opened for reading. "
          << std::endl << "Filename: " << this->GetFileName()
          << std::endl;
      otb::ImageFileReaderException e(__FILE__, __LINE__, msg.str().c_str(), ITK_LOCATION);
      throw e;
      return;

      }
    readTester.close();
    }
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


} //namespace otb

#endif
