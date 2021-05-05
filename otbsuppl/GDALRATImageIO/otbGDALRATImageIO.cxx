 /******************************************************************************
 * Created by Alexander Herzig;
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
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include "otbGDALRATImageIO.h"
//#include "otbMacro.h"
#include "otbSystem.h"
#include "otbImage.h"
#include "otbSQLiteTable.h"
#include "otbRAMTable.h"
#include "vcl_numeric.h"
#include "vcl_algorithm.h"
#include "itkVariableLengthVector.h"

#include "itkMetaDataObject.h"
#include "otbMetaDataKey.h"

#include "itkRGBPixel.h"
#include "itkRGBAPixel.h"
#include "itkTimeProbe.h"

#include "nmtypeinfo.h"

//#include "valgrind/callgrind.h"

const std::string otb::GDALRATImageIO::ctx = "GDALRATImageIO";

namespace otb
{

GDALRATImageIO::GDALRATImageIO()
{
  // By default set number of dimensions to two.
  this->SetNumberOfDimensions(2);

  // By default set pixel type to scalar.
  m_PixelType = SCALAR;

  // By default set component type to unsigned char
  m_ComponentType = UCHAR;
  m_UseCompression = true;
  m_CompressionLevel = 7; // default was 4: Range 0-9; 0 = no file compression, 9 = maximum file compression

  // Set default spacing to one
  m_Spacing[0] = 1.0;
  m_Spacing[1] = 1.0;

  // set the upper left corner coordinates
  m_UpperLeftCorner.resize(this->GetNumberOfDimensions());
  m_UpperLeftCorner[0] = 1.0;
  m_UpperLeftCorner[0] = 1.0;

  // Set default origin
  // upper left pixel centre
  m_Origin[0] = 0.5;
  m_Origin[1] = 0.5;

  m_IsIndexed   = false;
  m_DatasetNumber = 0;

  m_Dataset = 0;

  m_RATType = AttributeTable::ATTABLE_TYPE_RAM;
  m_RATSupport = false;
  m_DbRATReadOnly = false;
  m_ImageUpdateMode = false;
  m_UseForcedLPR = false;
  //m_UseUpdateRegion = false;
  m_NbBands = 0;
  m_FlagWriteImageInformation = true;

  m_CanStreamWrite = false;
  m_IsComplex = false;
  m_IsVectorImage = false;
  m_CreatedNotWritten = false;

  //this->AddSupportedReadExtension("kea");
  //this->AddSupportedWriteExtension("kea");

  m_NbOverviews = 0;
  m_OverviewIdx = -1;

  m_ImgInfoHasBeenRead = false;
  m_GDALComponentType = GDT_Byte;

  m_RGBMode = false;
  m_BandMap.clear();

  //ctx = "GDALRATImageIO";
}

GDALRATImageIO::~GDALRATImageIO()
{
    this->CloseDataset();
}

// Tell only if the file can be read with GDAL.
bool GDALRATImageIO::CanReadFile(const char* file)
{
  // First check the extension
  if (file == NULL)
    {
    NMLogDebug(<< "No filename specified.");
    return false;
    }

  // open data set and close right away, otherwise this
  // causes hawock in conjunction with the lumass pipeline
  // mechanics

  m_Dataset = (GDALDataset*)GDALOpen(file, GA_ReadOnly);

  bool bret = false;
  if (m_Dataset != 0)
  {
      bret = true;
      this->CloseDataset();
  }

  return bret;
}

// Used to print information about this object
void GDALRATImageIO::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Compression Level : " << m_CompressionLevel << "\n";
  os << indent << "IsComplex (otb side) : " << m_IsComplex << "\n";
  os << indent << "Byte per pixel : " << m_BytePerPixel << "\n";
}

// Read a 3D image (or event more bands)... not implemented yet
void GDALRATImageIO::ReadVolume(void*)
{
}

void GDALRATImageIO::CloseDataset(void)
{
    if (m_Dataset != 0)
    {
        GDALClose(m_Dataset);
    }
    m_Dataset = 0;
}

void
GDALRATImageIO::SetOverviewIdx(int idx)
{
    if (idx >= -1 && idx < this->m_NbOverviews)
    {
        this->m_OverviewIdx = idx;
        this->updateOverviewInfo();
    }
}

// Read image with GDAL
void GDALRATImageIO::Read(void* buffer)
{
  // Convert buffer from void * to unsigned char *
  unsigned char *p = static_cast<unsigned char *>(buffer);

  // Check if conversion succeed
  if (p == NULL)
    {
    NMLogError(<< "GDAL : Bad alloc");
    return;
    }

  // Get nb. of lines and columns of the region to read
  int lNbBufLines     = this->GetIORegion().GetSize()[1];
  int lNbBufColumns   = this->GetIORegion().GetSize()[0];
  int lFirstLine   = this->GetIORegion().GetIndex()[1]; // [1... ]
  int lFirstColumn = this->GetIORegion().GetIndex()[0]; // [1... ]

  // set the output number of cols and rows depending on the
  // resolution factor
  int lNbLines = lNbBufLines;
  int lNbColumns = lNbBufColumns;

  if (m_Dataset == 0)
  {
      m_Dataset = (GDALDataset*)GDALOpen(this->m_FileName.c_str(), GA_ReadOnly);
  }

  // calc reference (orig. image values) of requested region
  if (m_OverviewIdx >= 0 && m_OverviewIdx < this->m_NbOverviews)
  {
        //            std::cout << "ImgIO-read: ULC: " << lFirstColumn << "," << lFirstLine << std::endl;
        //            std::cout << "ImgIO-read: size: " << lNbBufColumns
        //                      << " x " << lNbBufLines << std::endl;

      double xstart = m_UpperLeftCorner[0] + lFirstColumn * m_Spacing[0];//m_LPRSpacing[0];
      double xend = xstart + lNbBufColumns * m_Spacing[0];
      lNbColumns = (xend - xstart) / m_LPRSpacing[0];

      double ystart = m_UpperLeftCorner[1] + lFirstLine * m_Spacing[1];//m_LPRSpacing[1];
      double yend = ystart + lNbBufLines * m_Spacing[1];
      lNbLines = (ystart - yend) / ::fabs(m_LPRSpacing[1]);


      lFirstColumn = (xstart - m_UpperLeftCorner[0]) / m_LPRSpacing[0];
      lFirstLine = (m_UpperLeftCorner[1] - ystart) / ::fabs(m_LPRSpacing[1]);

    //            std::cout << "ImgIO-read: orig. ULC: " << lFirstColumn << "," << lFirstLine << std::endl;
    //            std::cout << "ImgIO-read: orig. size: " << lNbColumns
    //                      << " x " << lNbLines << std::endl;
  }

  if (m_IsIndexed)
    {
    // TODO: This is a very special case and seems to be working only
    // for unsigned char pixels. There might be a gdal method to do
    // the work in a cleaner way
    std::streamoff lNbPixels = (static_cast<std::streamoff>(lNbBufColumns))
                             * (static_cast<std::streamoff>(lNbBufLines));
    std::streamoff lBufferSize = static_cast<std::streamoff>(m_BytePerPixel) * lNbPixels;
    itk::VariableLengthVector<unsigned char> value(lBufferSize);

   std::streamoff step = static_cast<std::streamoff>(this->GetNumberOfComponents())
                       * static_cast<std::streamoff>(m_BytePerPixel);

    CPLErr lCrGdal = m_Dataset->GetRasterBand(1)->RasterIO(GF_Read,
                                     lFirstColumn,
                                     lFirstLine,
                                     lNbColumns,
                                     lNbLines,
                                     const_cast<unsigned char*>(value.GetDataPointer()),
                                     lNbBufColumns,
                                     lNbBufLines,
                                     m_GDALComponentType,
                                     0,
                                     0);
    if (lCrGdal == CE_Failure)
      {
      NMLogError(<< "Error while reading image (GDAL format) " << m_FileName.c_str() << ".");
      }
    // Interpret index as color
    std::streamoff cpt(0);
    GDALColorTable* colorTable = m_Dataset->GetRasterBand(1)->GetColorTable();
    for (std::streamoff i = 0; i < lBufferSize; i = i + static_cast<std::streamoff>(m_BytePerPixel))
      {
      GDALColorEntry color;
      colorTable->GetColorEntryAsRGB(value[i], &color);
      p[cpt] = color.c1;
      p[cpt + 1] = color.c2;
      p[cpt + 2] = color.c3;
      p[cpt + 3] = color.c4;
      cpt += step;
      }
    }
  else
    {
    /********  Nominal case ***********/
    int nbBands     = m_NbBands;

    int* bandMap = 0;
    // make sure we've got a meaningful list of bands
    // to be read when we're in RGB mode
    if (this->GetPixelType() == RGB)
    {
        bandMap = new int[3];
        for (int b=0; b < 3; ++b)
        {
            if (m_BandMap.size() >= b+1)
            {
                bandMap[b] = m_BandMap[b];
            }
            else
            {
                bandMap[b] = m_NbBands >= b+1 ? b+1 : m_NbBands;
            }
        }
        nbBands = 3;
    }
    else if (m_BandMap.size() > 0)
    {
        nbBands = m_BandMap.size();
        bandMap = new int[nbBands];
        for (int b=0; b < nbBands; ++b)
        {
            bandMap[b] = m_BandMap[b];
        }
    }

    int pixelOffset = m_BytePerPixel * nbBands;
    int lineOffset  = m_BytePerPixel * nbBands * lNbBufColumns;
    int bandOffset  = m_BytePerPixel;

    // In some cases, we need to change some parameters for RasterIO
    if(!GDALDataTypeIsComplex(m_GDALComponentType) && m_IsComplex && m_IsVectorImage && (m_NbBands > 1))
      {
      pixelOffset = m_BytePerPixel * 2;
      lineOffset  = pixelOffset * lNbBufColumns;
      bandOffset  = m_BytePerPixel;
      }

    // keep it for the moment
    //NMLogDebug(<< "Number of bands inside input file: " << m_NbBands);
    NMLogDebug(<< "Parameters RasterIO : \n"
                   << " indX = " << lFirstColumn << "\n"
                   << " indY = " << lFirstLine << "\n"
                   << " sizeX = " << lNbBufColumns << "\n"
                   << " sizeY = " << lNbBufLines << "\n"
                   << " GDAL Data Type = " << GDALGetDataTypeName(m_GDALComponentType) << "\n"
                   << " pixelOffset = " << pixelOffset << "\n"
                   << " lineOffset = " << lineOffset << "\n"
                   << " bandOffset = " << bandOffset);

//    itk::TimeProbe chrono;
//    chrono.Start();
    CPLErr lCrGdal = m_Dataset->RasterIO(GF_Read,
                                       lFirstColumn,
                                       lFirstLine,
                                       lNbColumns,
                                       lNbLines,
                                       p,
                                       lNbBufColumns,
                                       lNbBufLines,
                                       m_GDALComponentType,
                                       nbBands,
                                       // read specified bands
                                       bandMap,
                                       pixelOffset,
                                       lineOffset,
                                       bandOffset);
//    chrono.Stop();
//    NMProcDebug(<< "RasterIO Read took " << chrono.GetTotal() << " sec")
//    //NMLogDebug(<< "RasterIO Read took " << chrono.GetTotal() << " sec")
//    NMDebugAI(<< "RasterIO Read took " << chrono.GetTotal() << " sec")

    if (bandMap) delete(bandMap);

    // Check if gdal call succeed
    if (lCrGdal == CE_Failure)
      {
      NMLogError(<< "Error while reading image (GDAL format) " << m_FileName.c_str() << ".");
      return;
      }
    //printDataBuffer(p, m_GDALComponentType, m_NbBands, lNbColumns*lNbLines);
    }

   // closes the underlying dataset
   this->CloseDataset();
}

//bool GDALRATImageIO::GetSubDatasetInfo(std::vector<std::string> &names, std::vector<std::string> &desc)
//{
//  // Note: we assume that the subdatasets are in order : SUBDATASET_ID_NAME, SUBDATASET_ID_DESC, SUBDATASET_ID+1_NAME, SUBDATASET_ID+1_DESC
//  char** papszMetadata;
//  papszMetadata = m_Dataset->GetDataSet()->GetMetadata("SUBDATASETS");

//  // Have we find some dataSet ?
//  // This feature is supported only for hdf4 and hdf5 file (regards to the bug 270)
//  if ( (CSLCount(papszMetadata) > 0) &&
//       ( (strcmp(m_Dataset->GetDataSet()->GetDriver()->GetDescription(),"HDF4") == 0) ||
//         (strcmp(m_Dataset->GetDataSet()->GetDriver()->GetDescription(),"HDF5") == 0) ) )
//    {
//    for (int cpt = 0; papszMetadata[cpt] != NULL; ++cpt)
//      {
//      std::string key, name;
//      if (System::ParseHdfSubsetName(papszMetadata[cpt], key, name))
//        {
//        NMLogDebug(<< "- key:  " << key);
//        NMLogDebug(<< "- name: " << name);
//        // check if this is a dataset name
//        if (key.find("_NAME") != std::string::npos) names.push_back(name);
//        // check if this is a dataset descriptor
//        if (key.find("_DESC") != std::string::npos) desc.push_back(name);
//        }
//      }
//    }
//  else
//    {
//    return false;
//    }
//  if (names.empty() || desc.empty()) return false;
//  if (names.size() != desc.size())
//    {
//    names.clear();
//    desc.clear();
//    return false;
//    }

//  return true;
//}

bool GDALRATImageIO::GDALPixelTypeIsComplex()
{
  return GDALDataTypeIsComplex(m_GDALComponentType);
}

void GDALRATImageIO::ReadImageInformation()
{
  //std::ifstream file;
  this->InternalReadImageInformation();
}

std::vector<unsigned int> GDALRATImageIO::GetOverviewSize(int ovv)
{
    std::vector<unsigned int> ret;
    if (ovv >=0 && ovv < this->m_NbOverviews)
    {
        ret = m_OvvSize[ovv];
    }

    return ret;
}

void GDALRATImageIO::updateOverviewInfo()
{
    // we just bail out if no img info is avail as yet
    // (i.e. the data set hasn't been initialised)
    if (!m_ImgInfoHasBeenRead)
    {
        this->ReadImageInformation();
        return;
    }

    if (m_OverviewIdx >= 0 && m_OverviewIdx < m_NbOverviews)
    {
        m_Dimensions[0] = m_OvvSize[m_OverviewIdx][0];
        m_Dimensions[1] = m_OvvSize[m_OverviewIdx][1];

        double xsize = (m_UpperLeftCorner[0] + (m_LPRSpacing[0] * m_LPRDimensions[0])) - m_UpperLeftCorner[0];
        double ysize = m_UpperLeftCorner[1] - (m_UpperLeftCorner[1] + (m_LPRSpacing[1] * m_LPRDimensions[1]));
        m_Spacing[0] = xsize / (double)m_Dimensions[0];
        m_Spacing[1] = -(ysize / (double)m_Dimensions[1]);

    }
    else if (m_OverviewIdx == -1)
    {
        m_Dimensions[0] = m_LPRDimensions[0];
        m_Dimensions[1] = m_LPRDimensions[1];
        m_Spacing[0] = m_LPRSpacing[0];
        m_Spacing[1] = m_LPRSpacing[1];
    }
}

void GDALRATImageIO::InternalReadImageInformation()
{
    if (m_ImgInfoHasBeenRead)
    {
        // do we have to look after the band map?
        return;
    }

  // do we have a valid dataset wrapper and a valid
  // GDALDataset ?
  if (m_Dataset == 0)
  {
    // we assume CanRead has been called on this data set already
    // as per design of the itk::ImageIO ...
    m_Dataset = (GDALDataset*)GDALOpen(this->m_FileName.c_str(), GA_ReadOnly);
  }

    //  // Detecting if we are in the case of an image with subdatasets
    //  // example: hdf Modis data
    //  // in this situation, we are going to change the filename to the
    //  // supported gdal format using the m_DatasetNumber value
    //  // HDF4_SDS:UNKNOWN:"myfile.hdf":2
    //  // and make m_Dataset point to it.
    //  if (m_Dataset->GetRasterCount() == 0)
    //    {
    //    // this happen in the case of a hdf file with SUBDATASETS
    //    // Note: we assume that the datasets are in order
    //    char** papszMetadata;
    //    papszMetadata = m_Dataset->GetMetadata("SUBDATASETS");
    //    //TODO: we might want to keep the list of names somewhere, at least the number of datasets
    //    std::vector<std::string> names;
    //    if( CSLCount(papszMetadata) > 0 )
    //      {
    //      for( int cpt = 0; papszMetadata[cpt] != NULL; ++cpt )
    //        {
    //        std::string key, name;
    //        if (System::ParseHdfSubsetName(papszMetadata[cpt], key, name))
    //          {
    //          NMLogDebug(<< "- key:  " << key);
    //          NMLogDebug(<< "- name: " << name);
    //          // check if this is a dataset name
    //          if (key.find("_NAME") != std::string::npos) names.push_back(name);
    //          }
    //        }
    //      }
    //    if (m_DatasetNumber < names.size())
    //      {
    //      NMLogDebug(<< "Reading: " << names[m_DatasetNumber]);
    //      m_Dataset = 0;
    //      m_Dataset = GDALDriverManagerWrapper::GetInstance().Open(names[m_DatasetNumber]);
    //      }
    //    else
    //      {
    //      NMLogError(<< "Dataset requested does not exist (" << names.size() << " datasets)");
    //      }
    //    }

  //GDALDataset* dataset = m_Dataset->GetDataSet();

  // Get image dimensions
  if ( m_Dataset->GetRasterXSize() == 0 || m_Dataset->GetRasterYSize() == 0 )
    {
    NMLogError(<< "Dimension is undefined.");
    }

  // Set image dimensions into IO
  m_LPRDimensions[0] = m_Dataset->GetRasterXSize();
  m_LPRDimensions[1] = m_Dataset->GetRasterYSize();
  m_Dimensions[0] = m_LPRDimensions[0];
  m_Dimensions[1] = m_LPRDimensions[1];

  // Get Number of Bands
  m_NbBands = m_Dataset->GetRasterCount();

  NMLogDebug(<< "Input file dimension: " << m_Dimensions[0] << ", " << m_Dimensions[1]);
  NMLogDebug(<< "Number of bands inside input file: " << m_NbBands);

  this->SetNumberOfComponents(m_NbBands);

  // Set the number of dimensions (verify for the dim )
  this->SetNumberOfDimensions(2);

  NMLogDebug(<< "Nb of Dimensions of the input file: " << m_NumberOfDimensions);

  // fetch overview information
  this->m_NbOverviews = m_Dataset->GetRasterBand(1)->GetOverviewCount();
  this->m_OvvSize.clear();
  for (int ov=0; ov < m_NbOverviews; ++ov)
  {
      std::vector<unsigned int> s;
      s.push_back(m_Dataset->GetRasterBand(1)->GetOverview(ov)->GetXSize());
      s.push_back(m_Dataset->GetRasterBand(1)->GetOverview(ov)->GetYSize());
      this->m_OvvSize.push_back(s);
  }

  if (m_OverviewIdx >= 0 && m_OverviewIdx < m_NbOverviews)
  {
      m_Dimensions[0] = m_OvvSize[m_OverviewIdx][0];
      m_Dimensions[1] = m_OvvSize[m_OverviewIdx][1];
  }
    //  std::cout << "ImgIO: ovv #" << m_OverviewIdx
    //            << ": size: " << m_Dimensions[0] << " x " << m_Dimensions[1] << std::endl;


  // Automatically set the Type to Binary for GDAL data
  this->SetFileTypeToBinary();

  // Get Data Type
  // Consider only the data type given by the first band
  // Maybe be could changed (to check)
  m_GDALComponentType = m_Dataset->GetRasterBand(1)->GetRasterDataType();
  NMLogDebug(<< "PixelType inside input file: "<< GDALGetDataTypeName(m_GDALComponentType) );
  if (m_GDALComponentType == GDT_Byte)
    {
    SetComponentType(UCHAR);

    // need to check for 'SIGNEDBYTE'!
    const char* meta = m_Dataset->GetMetadataItem("PIXELTYPE", "IMAGE_STRUCTURE");

    if (meta != 0)
    {
        if (strcmp(meta, "SIGNEDBYTE") == 0)
        {
            SetComponentType(CHAR);
            std::cout << "Got a hidden SIGNEDBYTE!" << std::endl;
        }
    }
    }
  else if (m_GDALComponentType == GDT_UInt16)
    {
    SetComponentType(USHORT);
    }
  else if (m_GDALComponentType == GDT_Int16)
    {
    SetComponentType(SHORT);
    }
  else if (m_GDALComponentType == GDT_UInt32)
    {
    SetComponentType(UINT);
    }
  else if (m_GDALComponentType == GDT_Int32)
    {
    SetComponentType(INT);
    }
  else if (m_GDALComponentType == GDT_Float32)
    {
    SetComponentType(FLOAT);
    }
  else if (m_GDALComponentType == GDT_Float64)
    {
    SetComponentType(DOUBLE);
    }
  else if (m_GDALComponentType == GDT_CInt16)
    {
    SetComponentType(CSHORT);
    }
  else if (m_GDALComponentType == GDT_CInt32)
    {
    SetComponentType(CINT);
    }
  else if (m_GDALComponentType == GDT_CFloat32)
    {
    SetComponentType(CFLOAT);
    }
  else if (m_GDALComponentType == GDT_CFloat64)
    {
    SetComponentType(CDOUBLE);
    }
  else
    {
    NMLogError(<< "Pixel type unknown");
    }

  if (this->GetComponentType() == CHAR)
    {
    m_BytePerPixel = sizeof(char);
    }
  else if (this->GetComponentType() == UCHAR)
    {
    m_BytePerPixel = sizeof(unsigned char);
    }
  else if (this->GetComponentType() == USHORT)
    {
    m_BytePerPixel = sizeof(unsigned short);
    }
  else if (this->GetComponentType() == SHORT)
    {
    m_BytePerPixel = sizeof(short);
    }
  else if (this->GetComponentType() == INT)
    {
    m_BytePerPixel = sizeof(int);
    }
  else if (this->GetComponentType() == UINT)
    {
    m_BytePerPixel = sizeof(unsigned int);
    }
  else if (this->GetComponentType() == LONG)
    {
    m_BytePerPixel = sizeof(long);
    }
  else if (this->GetComponentType() == ULONG)
    {
    m_BytePerPixel = sizeof(unsigned long);
    }
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
  else if (this->GetComponentType() == LONGLONG)
    {
      m_BytePerPixel = sizeof(long long);
    }
  else if (this->GetComponentType() == ULONGLONG)
    {
      m_BytePerPixel = sizeof(unsigned long long);
    }
#endif
  else if (this->GetComponentType() == FLOAT)
    {
    m_BytePerPixel = 4;
    }
  else if (this->GetComponentType() == DOUBLE)
    {
    m_BytePerPixel = 8;
    }
  else if (this->GetComponentType() == CSHORT)
    {
    m_BytePerPixel = sizeof(std::complex<short>);
    }
  else if (this->GetComponentType() == CINT)
    {
    m_BytePerPixel = sizeof(std::complex<int>);
    }
  else if (this->GetComponentType() == CFLOAT)
    {
    /*if (m_GDALComponentType == GDT_CInt16)
      m_BytePerPixel = sizeof(std::complex<short>);
    else if (m_GDALComponentType == GDT_CInt32)
      m_BytePerPixel = sizeof(std::complex<int>);
    else*/
      m_BytePerPixel = sizeof(std::complex<float>);
    }
  else if (this->GetComponentType() == CDOUBLE)
    {
      m_BytePerPixel = sizeof(std::complex<double>);
    }
  else
    {
    NMLogError(<< "Component type unknown");
    }

  /******************************************************************/
  // Set the pixel type with some special cases linked to the fact
  //  we read some data with complex type.
  if ( GDALDataTypeIsComplex(m_GDALComponentType) ) // Try to read data with complex type with GDAL
  {
      if ( !m_IsComplex && m_IsVectorImage )
      {
          // we are reading a complex data set into an image where the pixel
          // type is Vector<real>: we have to double the number of component
          // for that to work
          NMLogDebug( << "GDALtypeIO= Complex and IFReader::InternalPixelType= Scalar and IFReader::PixelType= Vector");
          this->SetNumberOfComponents(m_NbBands*2);
          this->SetPixelType(VECTOR);
      }
      else
      {
          this->SetPixelType(COMPLEX);
      }
  }
  else // Try to read data with scalar type with GDAL
  {
      if (this->m_RGBMode && m_NbBands > 1)
      {
          this->SetPixelType(RGB);
          this->SetNumberOfComponents(3);
      }
      else
      {
          if (m_NbBands > 1)
          {
              if (this->m_BandMap.size() > 0)
              {
                  this->SetNumberOfComponents(m_BandMap.size());
                  if (this->m_BandMap.size() > 1)
                  {
                        this->SetPixelType(VECTOR);
                  }
                  else
                  {
                        this->SetPixelType(SCALAR);
                  }
              }
              else
              {
                  this->SetNumberOfComponents(m_NbBands);
                  this->SetPixelType(VECTOR);
              }
          }
          else
          {
              this->SetNumberOfComponents(1);
              this->SetPixelType(SCALAR);
          }
      }
  }

  /*** Parameters set by Internal Read function ***/
  NMLogDebug( << "Pixel Type IFReader = " << GetPixelTypeAsString(this->GetPixelType()) )
  NMLogDebug( << "Number of component IFReader = " << this->GetNumberOfComponents() )
  NMLogDebug( << "Byte per pixel set = " << m_BytePerPixel )
  NMLogDebug( << "Component Type set = " << GetComponentTypeAsString(this->GetComponentType()) );

  /*----------------------------------------------------------------------*/
  /*-------------------------- METADATA ----------------------------------*/
  /*----------------------------------------------------------------------*/

  // Now initialize the itk dictionary
  itk::MetaDataDictionary& dict = this->GetMetaDataDictionary();

  /* -------------------------------------------------------------------- */
  /*  Get Spacing                */
  /* -------------------------------------------------------------------- */

  // Default Spacing
  m_Spacing[0] = 1;
  m_Spacing[1] = -1;
  if (m_NumberOfDimensions == 3) m_Spacing[2] = 1;

  // Default Spacing
  m_LPRSpacing[0] = 1;
  m_LPRSpacing[1] = -1;
  if (m_NumberOfDimensions == 3) m_LPRSpacing[2] = 1;


  char** papszMetadata;
  papszMetadata =  m_Dataset->GetMetadata(NULL);

  /* -------------------------------------------------------------------- */
  /*      Report general info.                                            */
  /* -------------------------------------------------------------------- */
  GDALDriver* pDriver = 0;

  pDriver = (GDALDriver*)m_Dataset->GetDriver();

  std::string driverShortName =  static_cast<std::string>(GDALGetDriverShortName(pDriver));
  std::string driverLongName  =  static_cast<std::string>(GDALGetDriverLongName(pDriver));

  itk::EncapsulateMetaData<std::string>(dict, MetaDataKey::DriverShortNameKey, driverShortName);
  itk::EncapsulateMetaData<std::string>(dict, MetaDataKey::DriverLongNameKey,  driverLongName);

  /* -------------------------------------------------------------------- */
  /* Get the projection coordinate system of the image : ProjectionRef  */
  /* -------------------------------------------------------------------- */
  if (m_Dataset->GetProjectionRef() != NULL && !std::string(m_Dataset->GetProjectionRef()).empty())
    {
    OGRSpatialReferenceH pSR = OSRNewSpatialReference(NULL);

    const char *         pszProjection = NULL;
    pszProjection =  m_Dataset->GetProjectionRef();

    if (OSRImportFromWkt(pSR, (char **) (&pszProjection)) == OGRERR_NONE)
      {
      char * pszPrettyWkt = NULL;
      OSRExportToPrettyWkt(pSR, &pszPrettyWkt, FALSE);

      itk::EncapsulateMetaData<std::string> (dict, MetaDataKey::ProjectionRefKey,
                                             static_cast<std::string>(pszPrettyWkt));

      CPLFree(pszPrettyWkt);
      }
    else
      {
      itk::EncapsulateMetaData<std::string>(dict, MetaDataKey::ProjectionRefKey,
                                            static_cast<std::string>(m_Dataset->GetProjectionRef()));
      }

    if (pSR != NULL)
      {
      OSRRelease(pSR);
      pSR = NULL;
      }
    }

  /* -------------------------------------------------------------------- */
  /* Get the GCP projection coordinates of the image : GCPProjection  */
  /* -------------------------------------------------------------------- */

  unsigned int gcpCount = 0;
  gcpCount = m_Dataset->GetGCPCount();
  if (gcpCount > 0)
    {
    std::string gcpProjectionKey = static_cast<std::string>(m_Dataset->GetGCPProjection());
    itk::EncapsulateMetaData<std::string>(dict, MetaDataKey::GCPProjectionKey, gcpProjectionKey);

    if (gcpProjectionKey.empty())
      {
      gcpCount = 0; //fix for uninitialized gcpCount in gdal (when
                    //reading Palsar image)
      }

    std::string key;

    itk::EncapsulateMetaData<unsigned int>(dict, MetaDataKey::GCPCountKey, gcpCount);

    for (unsigned int cpt = 0; cpt < gcpCount; ++cpt)
      {

      const GDAL_GCP *psGCP;
      psGCP = m_Dataset->GetGCPs() + cpt;

      GCP pOtbGCP;
      pOtbGCP.m_Id = std::string(psGCP->pszId);
      pOtbGCP.m_Info = std::string(psGCP->pszInfo);
      pOtbGCP.m_GCPRow = psGCP->dfGCPLine;
      pOtbGCP.m_GCPCol = psGCP->dfGCPPixel;
      pOtbGCP.m_GCPX = psGCP->dfGCPX;
      pOtbGCP.m_GCPY = psGCP->dfGCPY;
      pOtbGCP.m_GCPZ = psGCP->dfGCPZ;

      // Complete the key with the GCP number : GCP_i
      std::ostringstream lStream;
      lStream << MetaDataKey::GCPParametersKey << cpt;
      key = lStream.str();

      itk::EncapsulateMetaData<GCP>(dict, key, pOtbGCP);

      }

    }

  /* -------------------------------------------------------------------- */
  /*  Get the six coefficients of affine geoTtransform      */
  /* -------------------------------------------------------------------- */

  double     adfGeoTransform[6];
  MetaDataKey::VectorType VadfGeoTransform;

  if (m_Dataset->GetGeoTransform(adfGeoTransform) == CE_None)
  {
      for (int cpt = 0; cpt < 6; ++cpt)
          VadfGeoTransform.push_back(adfGeoTransform[cpt]);

      itk::EncapsulateMetaData<MetaDataKey::VectorType>(dict, MetaDataKey::GeoTransformKey, VadfGeoTransform);

      /// retrieve origin and spacing from the geo transform
      m_UpperLeftCorner[0] = VadfGeoTransform[0];
      m_UpperLeftCorner[1] = VadfGeoTransform[3];
      m_LPRSpacing[0] = VadfGeoTransform[1];
      m_LPRSpacing[1] = VadfGeoTransform[5];
      m_Spacing[0] = m_LPRSpacing[0];
      m_Spacing[1] = m_LPRSpacing[1];
      m_Origin[0] = m_UpperLeftCorner[0] + 0.5 * m_Spacing[0];
      m_Origin[1] = m_UpperLeftCorner[1] + 0.5 * m_Spacing[1];

      //    std::cout << "ImgIO: scaled spacing: x: " << m_Spacing[0] << " y: " << m_Spacing[1] << std::endl;
  }

  // want to adjust spacing even if we haven't got a GeoTransform given (e.g. for photos)
  if (m_OverviewIdx >= 0 && m_OverviewIdx < m_NbOverviews)
  {
      double xsize = (m_UpperLeftCorner[0] + (m_LPRSpacing[0] * m_LPRDimensions[0])) - m_UpperLeftCorner[0];
      double ysize = m_UpperLeftCorner[1] - (m_UpperLeftCorner[1] + (m_LPRSpacing[1] * m_LPRDimensions[1]));
      m_Spacing[0] = xsize / (double)m_Dimensions[0];
      m_Spacing[1] = -(ysize / (double)m_Dimensions[1]);
  }


  // m_Dataset info
  NMLogDebug(<< "**** ReadImageInformation() m_Dataset INFO: ****" );
  NMLogDebug(<< "Projection Ref: "<< m_Dataset->GetProjectionRef() );
  double GT[6];
  if (m_Dataset->GetGeoTransform(GT) == CE_None)
  {
      NMLogDebug( <<"Geo Transform: "<< GT[0] << ", " << GT[1] << ", "
                                                  << GT[2] << ", " << GT[3] << ", "
                                                  << GT[4] << ", " << GT[5] );
  }
  else
  {
      NMLogDebug( << "No Geo Transform: ");
  }
  NMLogDebug(<< "GCP Projection Ref: "<< m_Dataset->GetGCPProjection() );
  NMLogDebug(<< "GCP Count: " << m_Dataset->GetGCPCount() );

  /* -------------------------------------------------------------------- */
  /*      Report metadata.                                                */
  /* -------------------------------------------------------------------- */

  papszMetadata = m_Dataset->GetMetadata(NULL);
  if (CSLCount(papszMetadata) > 0)
    {
    std::string key;

    for (int cpt = 0; papszMetadata[cpt] != NULL; ++cpt)
      {
      std::ostringstream lStream;
      lStream << MetaDataKey::MetadataKey << cpt;
      key = lStream.str();

      itk::EncapsulateMetaData<std::string>(dict, key,
                                            static_cast<std::string>(papszMetadata[cpt]));
      }
    }

  /* -------------------------------------------------------------------- */
  /*      Report subm_Datasets.                                             */
  /* -------------------------------------------------------------------- */

  papszMetadata = m_Dataset->GetMetadata("SUBm_DatasetS");
  if (CSLCount(papszMetadata) > 0)
    {
    std::string key;

    for (int cpt = 0; papszMetadata[cpt] != NULL; ++cpt)
      {
      std::ostringstream lStream;
      lStream << MetaDataKey::SubMetadataKey << cpt;
      key = lStream.str();

      itk::EncapsulateMetaData<std::string>(dict, key,
                                            static_cast<std::string>(papszMetadata[cpt]));
      }
    }

  /* -------------------------------------------------------------------- */
  /* Report corners              */
  /* -------------------------------------------------------------------- */

  double     GeoX(0), GeoY(0);
  MetaDataKey::VectorType VGeo;

  GDALInfoReportCorner("Upper Left", 0.0, 0.0, GeoX, GeoY);
  VGeo.push_back(GeoX);
  VGeo.push_back(GeoY);

  itk::EncapsulateMetaData<MetaDataKey::VectorType>(dict, MetaDataKey::UpperLeftCornerKey, VGeo);

  VGeo.clear();

  GDALInfoReportCorner("Upper Right", m_Dimensions[0], 0.0, GeoX, GeoY);
  VGeo.push_back(GeoX);
  VGeo.push_back(GeoY);

  itk::EncapsulateMetaData<MetaDataKey::VectorType>(dict, MetaDataKey::UpperRightCornerKey, VGeo);

  VGeo.clear();

  GDALInfoReportCorner("Lower Left", 0.0, m_Dimensions[1], GeoX, GeoY);
  VGeo.push_back(GeoX);
  VGeo.push_back(GeoY);

  itk::EncapsulateMetaData<MetaDataKey::VectorType>(dict, MetaDataKey::LowerLeftCornerKey, VGeo);

  VGeo.clear();

  GDALInfoReportCorner("Lower Right", m_Dimensions[0], m_Dimensions[1], GeoX, GeoY);
  VGeo.push_back(GeoX);
  VGeo.push_back(GeoY);

  itk::EncapsulateMetaData<MetaDataKey::VectorType>(dict, MetaDataKey::LowerRightCornerKey, VGeo);

  VGeo.clear();

  /* -------------------------------------------------------------------- */
  /* Color Table                                                          */
  /* -------------------------------------------------------------------- */

  for (int iBand = 0; iBand < m_Dataset->GetRasterCount(); iBand++)
    {
    GDALRasterBandH hBand;
    hBand = GDALGetRasterBand(m_Dataset, iBand + 1);
    GDALColorTableH hTable = GDALGetRasterColorTable(hBand);
    if (!m_RATSupport)
        {
        if ((GDALGetRasterColorInterpretation(hBand) == GCI_PaletteIndex)
            && (hTable != NULL))
          {
          m_IsIndexed = true;

          unsigned int ColorEntryCount = GDALGetColorEntryCount(hTable);

          itk::EncapsulateMetaData<std::string>(dict, MetaDataKey::ColorTableNameKey,
                                                static_cast<std::string>(GDALGetPaletteInterpretationName(
                                                                           GDALGetPaletteInterpretation(hTable))));

          itk::EncapsulateMetaData<unsigned int>(dict, MetaDataKey::ColorEntryCountKey, ColorEntryCount);

          for (int i = 0; i < GDALGetColorEntryCount(hTable); ++i)
            {
            GDALColorEntry sEntry;
            MetaDataKey::VectorType VColorEntry;

            GDALGetColorEntryAsRGB(hTable, i, &sEntry);

            VColorEntry.push_back(sEntry.c1);
            VColorEntry.push_back(sEntry.c2);
            VColorEntry.push_back(sEntry.c3);
            VColorEntry.push_back(sEntry.c4);

            itk::EncapsulateMetaData<MetaDataKey::VectorType>(dict, MetaDataKey::ColorEntryAsRGBKey, VColorEntry);

            }
          }
        }
    else
        {
            m_IsIndexed = false;

            if (hTable != NULL)
            {
                unsigned int ColorEntryCount = GDALGetColorEntryCount(hTable);

                itk::EncapsulateMetaData<std::string>(dict,
                        MetaDataKey::ColorTableNameKey,
                        static_cast<std::string>(GDALGetPaletteInterpretationName(
                                GDALGetPaletteInterpretation(hTable))));

                itk::EncapsulateMetaData<unsigned int>(dict,
                        MetaDataKey::ColorEntryCountKey, ColorEntryCount);

                // NEW in the RAT version of this ImageIO:
                // the older version seemed to just have read one colour entry per band;
                // this version puts all colour entries (VColorEntry) into a vector
                // which can be fetched from the MetaDataDictionary using MetaDataKey::ColorEntryAsRGBKey key

                typedef std::vector<MetaDataKey::VectorType> VectorVectorType;
                VectorVectorType VVColorEntries;
                for (int i = 0; i < GDALGetColorEntryCount(hTable); ++i)
                {
                    GDALColorEntry sEntry;
                    MetaDataKey::VectorType VColorEntry;

                    GDALGetColorEntryAsRGB(hTable, i, &sEntry);

                    VColorEntry.push_back(sEntry.c1);
                    VColorEntry.push_back(sEntry.c2);
                    VColorEntry.push_back(sEntry.c3);
                    VColorEntry.push_back(sEntry.c4);

                    VVColorEntries.push_back(VColorEntry);
                }

                itk::EncapsulateMetaData<VectorVectorType>(dict,
                        MetaDataKey::ColorEntryAsRGBKey, VVColorEntries);
            }
        }
    }

  if (m_IsIndexed)
    {
    m_NbBands *= 4;
    this->SetNumberOfComponents(m_NbBands);
    this->SetPixelType(VECTOR);
    }

  // we close the dataset again to not
  // get into trouble!
  this->CloseDataset();

  m_ImgInfoHasBeenRead = true;
}

bool GDALRATImageIO::CanWriteFile(const char* name)
{
  m_FileName = name;

  // First check the filename
  if (name == NULL)
    {
    NMLogDebug(<< "No filename specified.");
    return false;
    }

  // Get the GDAL format ID from the name
  std::string gdalDriverShortName = FilenameToGdalDriverShortName(name);
  if (gdalDriverShortName == "NOT-FOUND")
    {
    NMLogDebug(<< "Couldn't find suitable driver for file type.");
    return false;
    }

  // Check the driver for support of Create or at least CreateCopy
  GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(gdalDriverShortName.c_str());
  if ( GDALGetMetadataItem( driver, GDAL_DCAP_CREATE, NULL ) == NULL
       && GDALGetMetadataItem( driver, GDAL_DCAP_CREATECOPY, NULL ) == NULL )
    {
    NMLogDebug(<< "The driver " << GDALGetDriverShortName(driver) << " does not support writing");
    return false;
    }
  return true;
}

bool GDALRATImageIO::CanStreamWrite()
{
  // Get the GDAL format ID from the name
  std::string gdalDriverShortName = FilenameToGdalDriverShortName(m_FileName);
  GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(gdalDriverShortName.c_str());

  if (driver == NULL)
    {
    NMLogDebug(<< "Unable to instantiate driver " << gdalDriverShortName);
    m_CanStreamWrite = false;
    }

  if ( GDALGetMetadataItem( driver, GDAL_DCAP_CREATE, NULL ) != NULL )
    {
    m_CanStreamWrite = true;
    }
  else
    {
    m_CanStreamWrite = false;
    }
  return m_CanStreamWrite;
}

void GDALRATImageIO::Write(const void* buffer)
{
    //NMDebugCtx(ctx, << "...");

    // Check if conversion succeed
    if (buffer == NULL)
    {
        NMLogError(<< "GDAL : Bad alloc");
        //NMDebugCtx(ctx, << "done!");
        return;
    }

    // Check if we have to write the image information
    if (m_FlagWriteImageInformation == true && m_Dataset == 0)
    {
        this->InternalWriteImageInformation(buffer);
        m_FlagWriteImageInformation = false;
    }


    if (m_ImageUpdateMode &&  m_CanStreamWrite)
    {
        if (!m_CreatedNotWritten)
        {
            // make sure we open the dataset in update mode before we proceed
            if (m_Dataset != 0)
            {
                this->CloseDataset();
            }
            m_Dataset = (GDALDataset*) GDALOpen(m_FileName.c_str(), GA_Update);
        }

        if (m_Dataset != 0)
        {
            NMLogDebug(<< "opened '" << m_FileName << "' in update mode.");

            // being in update mode may mean that we're not aware of some essential
            // metadata we need later on for the RasterIO call, so we read from the
            // just read image ...
            m_NbBands = m_Dataset->GetRasterCount();
            m_GDALComponentType = m_Dataset->GetRasterBand(1)->GetRasterDataType();
            m_BytePerPixel = GDALGetDataTypeSize(m_GDALComponentType) / 8;
            m_Dimensions[0] = m_Dataset->GetRasterXSize();
            m_Dimensions[1] = m_Dataset->GetRasterYSize();
        }
        else
        {
            if (m_CreatedNotWritten)
            {
                NMLogError(
                        << "GDAL: couldn't write to freshly created dataset '"
                        << m_FileName << "'!");
            }
            else
            {
                NMLogError(
                        << "GDAL: couldn't open '" << m_FileName << "' in update mode."
                        << " Double check whether the file format supports update!");
            }
        }
    }
    else if (m_ImageUpdateMode && !m_CanStreamWrite)
    {
        NMLogError(
                << "GDAL: file (format) cannot be used in update mode!")
    }

    // Compute offset and size
    unsigned int lNbLines = this->GetIORegion().GetSize()[1];
    unsigned int lNbColumns = this->GetIORegion().GetSize()[0];
    int lFirstLine = this->GetIORegion().GetIndex()[1]; // [1... ]
    int lFirstColumn = this->GetIORegion().GetIndex()[0]; // [1... ]

    // adjust overall image dimensions (ie largest possible region) when we've set
    // the forced largest possible region
    if (m_UseForcedLPR)
    {
        for (unsigned int d=0; d < this->GetNumberOfDimensions(); ++d)
        {
            m_Dimensions[d] = m_ForcedLPR.GetSize()[d];
        }
    }

    // Particular case: checking that the written region is the same size
    // of the entire image
    // starting at offset 0 (when no streaming)
    if ((lNbLines == m_Dimensions[1]) && (lNbColumns == m_Dimensions[0]) && !m_ImageUpdateMode)
    {
        lFirstLine = 0;
        lFirstColumn = 0;
    }

    // Convert buffer from void * to unsigned char *
    //unsigned char *p = static_cast<unsigned char*>( const_cast<void *>(buffer));
    //printDataBuffer(p,  m_GDALComponentType, m_NbBands, 10*2); // Buffer incorrect

    // If driver supports streaming
    if (m_CanStreamWrite)
    {

        NMLogDebug(<< "RasterIO Write requested region : " << this->GetIORegion() <<
                "\n, lNbColumns =" << lNbColumns <<
                "\n, lNbLines =" << lNbLines <<
                "\n, m_GDALComponentType =" << GDALGetDataTypeName(m_GDALComponentType) <<
                "\n, m_NbBands =" << m_NbBands <<
                "\n, m_BytePerPixel ="<< m_BytePerPixel <<
                "\n, Pixel offset =" << m_BytePerPixel * m_NbBands << // is nbComp * BytePerPixel
                "\n, Line offset =" << m_BytePerPixel * m_NbBands * lNbColumns <<// is pixelOffset * nbColumns
                "\n, Band offset =" << m_BytePerPixel)//  is BytePerPixel

        itk::TimeProbe chrono;

        //NMDebugAI(<< "PARAMETERS for GDAL RasterIO ... " << std::endl);
        //NMDebugAI(<< "IO start col: "<< lFirstColumn << " row: " << lFirstLine << std::endl);
        //NMDebugAI(<< "IO width:" << lNbColumns << " height: " << lNbLines << std::endl);
        //NMDebugAI(<< "image width: " << m_Dimensions[0] << " image height: " << m_Dimensions[1] << std::endl);
        //NMDebugAI(<< "no bands: " << m_NbBands << " of type: " << GDALGetDataTypeName(m_GDALComponentType)
        //		<< " of " << m_BytePerPixel << " bytes" << std::endl);

        // set up band map parameter
        int* bandMap = 0;
        int numWriteBands = m_NbBands;
        if (m_BandMap.size() > 0 && m_BandMap.size() <= m_NbBands)
        {
            bandMap = &m_BandMap[0];
            numWriteBands = m_BandMap.size();
        }

        chrono.Start();
        CPLErr lCrGdal = m_Dataset->RasterIO(GF_Write,
                            lFirstColumn,
                            lFirstLine,
                            lNbColumns,
                            lNbLines,
                            const_cast<void *>(buffer),
                            lNbColumns,
                            lNbLines,
                            m_GDALComponentType,
                            numWriteBands,
                            // We want to write all bands
                            bandMap,
                            // Pixel offset
                            // is nbComp * BytePerPixel
                            m_BytePerPixel * m_NbBands,
                            // Line offset
                            // is pixelOffset * nbColumns
                            m_BytePerPixel * m_NbBands * lNbColumns,
                            // Band offset is BytePerPixel
                            m_BytePerPixel);
        chrono.Stop();
        NMLogDebug(<< "RasterIO Write took " << chrono.GetTotal() << " sec")

        m_CreatedNotWritten = false;

        // Check if writing succeed
        if (lCrGdal == CE_Failure)
        {
            NMLogError(
                    << "Error while writing image (GDAL format) " << m_FileName.c_str() << ".");
        }

        // In update mode, we better close the GDAL data set (and with it the wrapper)
        // to avoid unpredictable behaviour of some drivers; in stream write mode, we
        // just flush the dataset cache though
        if (m_ImageUpdateMode)
        {
            // also update RAT's
            NMLogDebug( << "Writing RATs ...");
            for (unsigned int ti = 0; ti < this->m_vecRAT.size(); ++ti)
            {
                otb::AttributeTable::Pointer tab = this->m_vecRAT.at(ti);
                if (tab.IsNull())
                    continue;

                // for now, we assume ti=band
                this->WriteRAT(tab, ti + 1);
            }

            this->CloseDataset();
        }
        else
        {
            m_Dataset->FlushCache();
        }
    }
    else
    {
        // We only wrote data to the memory dataset
        // Now write it to the real file with CreateCopy()
        //		std::string gdalDriverShortName = FilenameToGdalDriverShortName(
        //				m_FileName);
        //		std::string realFileName = GetGdalWriteImageFileName(
        //				gdalDriverShortName, m_FileName);

        //		GDALDriver* driver =
        //				GDALDriverManagerWrapper::GetInstance().GetDriverByName(
        //						gdalDriverShortName);
        //		if (driver == NULL)
        //		{
        //			NMLogError(
        //                    << "Unable to instantiate driver " << gdalDriverShortName
        //                        << " to write " << m_FileName);
        //		}

        //		// handle creation options
        //		char ** option = 0;
        //		if (gdalDriverShortName.compare("JPEG") == 0)
        //		{
        //			// If JPEG, set the JPEG compression quality to 95.
        //			option = CSLAddNameValue(option, "QUALITY", "95");

        //		}
        //		else if (gdalDriverShortName.compare("HFA") == 0)
        //		{
        //			option = CSLAddNameValue(option, "COMPRESSED", "YES");
        //			option = CSLAddNameValue(option, "IGNOREUTM", "YES");
        //		}
        //		else if (gdalDriverShortName.compare("GTiff") == 0)
        //		{
        //			option = CSLAddNameValue(option, "COMPRESS", "LZW");
        //		}

        GDALDataset* hOutputDS = CreateCopy();
        //                driver->CreateCopy(realFileName.c_str(),
        //				m_Dataset->GetDataSet(), FALSE, option, NULL, NULL);
        std::cout << "raw DS::copy" << std::endl;

        //        if (gdalDriverShortName.compare("HFA") == 0)
        //        {
        //            for (int t=0; t < this->m_vecRAT.size(); ++t)
        //            {
        //                hOutputDS->GetRasterBand(t+1)->SetMetadataItem("LAYER_TYPE",
        //                                                                             "thematic");
        //                hOutputDS->GetRasterBand(t+1)->SetMetadataItem("STATISTICS_HISTOBINFUNCTION",
        //                                                                             "direct");
        //            }
        //        }


        GDALClose(hOutputDS);
        std::cout << "raw DS::close" << std::endl;
    }

    //NMDebugCtx(ctx, << "done!");
}

void
GDALRATImageIO::BuildOverviews(const std::string& resamplingType)
{
    if (resamplingType.compare("NONE") == 0)
    {
        return;
    }

    // don't know whether the DS was opened in update mode or not
    // so to be sure we can write to it, we just close it and
    // re-open it read-write
    if (m_Dataset != 0)
    {
        this->CloseDataset();
    }

    m_Dataset = (GDALDataset*)GDALOpen(this->GetFileName(), GA_Update);
    if (m_Dataset == 0)
    {
        NMProcWarn(<< "Failed opening dataset for overview generation!");
        // in case the data set was open before we tried building overivews,
        // we try to re-opening it again in ReadOnly mode (note: many drivers don't
        // support writing)
        if (m_ImgInfoHasBeenRead)
        {
            m_Dataset = (GDALDataset*)GDALOpen(this->GetFileName(), GA_ReadOnly);
        }
        return;
    }

    if (m_Dataset->GetRasterCount() == 0)
    {
        this->CloseDataset();
        return;
    }

    int pix = vcl_min(m_Dataset->GetRasterXSize(), m_Dataset->GetRasterYSize());

    std::vector<int> factorList;
    double exp = 1;
    double factor = static_cast<int>(vcl_pow(2,exp));
    while ((pix / factor) >= 256 )
    {
        factorList.push_back(factor);
        ++exp;
        factor = static_cast<int>(vcl_pow(2,exp));
    }

    m_Dataset->BuildOverviews(resamplingType.c_str(),
                       factorList.size(),
                       (int*)(&factorList[0]),
                       0,
                       0,
                       GDALDummyProgress,
                       0);


    this->CloseDataset();
    // if this ImageIO is in reading mode, we re-open the data set
    // in readonly mode to leave everything as we found it; and
    // we also update the overview information
    if (m_ImgInfoHasBeenRead)
    {
        m_Dataset = (GDALDataset*)GDALOpen(this->GetFileName(), GA_ReadOnly);

        this->m_NbOverviews = m_Dataset->GetRasterBand(1)->GetOverviewCount();
        this->m_OvvSize.clear();
        for (int ov=0; ov < m_NbOverviews; ++ov)
        {
            std::vector<unsigned int> s;
            s.push_back(m_Dataset->GetRasterBand(1)->GetOverview(ov)->GetXSize());
            s.push_back(m_Dataset->GetRasterBand(1)->GetOverview(ov)->GetYSize());
            this->m_OvvSize.push_back(s);
        }
    }
}

/** TODO : Methode WriteImageInformation non implementee */
void GDALRATImageIO::WriteImageInformation()
{
}

void GDALRATImageIO::SetForcedLPR(const itk::ImageIORegion& forcedLPR)
{
    this->m_ForcedLPR = forcedLPR;
    this->m_UseForcedLPR = true;
    //this->m_FlagWriteImageInformation = true;
}

//void GDALRATImageIO::SetUpdateRegion(const itk::ImageIORegion& updateRegion)
//{
//	//NMDebugCtx(ctx, << "...");
//
//	//NMDebugAI(<< "forced LPR .... " << std::endl);
//	//m_ForcedLPR.Print(std::cout, itk::Indent(nmlog::nmindent+4));
//	//NMDebugAI(<< "update region .... " << std::endl);
//	//updateRegion.Print(std::cout, itk::Indent(nmlog::nmindent+4));
//
//	// check, whether the update region is contained within the
//	// forced lpr
//	if ((updateRegion.GetIndex()[0] + updateRegion.GetSize()[0]) <= m_ForcedLPR.GetSize()[0] &&
//		(updateRegion.GetIndex()[1] + updateRegion.GetSize()[1]) <= m_ForcedLPR.GetSize()[1]     )
//	{
//		this->m_UpdateRegion = updateRegion;
//		this->m_UseUpdateRegion = true;
//	}
//	else
//	{
//		NMProcWarn(<< "The provided update region does not fit into the set forced largest possible region!");
//		this->m_UseUpdateRegion = false;
//	}
//
//	//NMDebugCtx(ctx, << "done!");
//}

void GDALRATImageIO::InternalWriteImageInformation(const void* buffer)
{
  char **     papszOptions = NULL;
  std::string driverShortName;
  m_NbBands = this->GetNumberOfComponents();
  //this->Get

  if ((m_Dimensions[0] == 0) && (m_Dimensions[1] == 0))
    {
    NMLogError(<< "Dimensions are not defined.");
    }

  if ((this->GetPixelType() == COMPLEX) /*&& (m_NbBands / 2 > 0)*/)
    {
    //m_NbBands /= 2;

    if (this->GetComponentType() == CSHORT)
      {
      m_BytePerPixel = 4;
      m_GDALComponentType = GDT_CInt16;
      }
    else if (this->GetComponentType() == CINT)
      {
      m_BytePerPixel = 8;
      m_GDALComponentType = GDT_CInt32;
      }
    else if (this->GetComponentType() == CFLOAT)
      {
      m_BytePerPixel = 8;
      m_GDALComponentType = GDT_CFloat32;
      }
    else if (this->GetComponentType() == CDOUBLE)
      {
      m_BytePerPixel = 16;
      m_GDALComponentType = GDT_CFloat64;
      }
    else
      {
      NMLogError(<< "This complex type is not defined :" << this->GetPixelTypeAsString(this->GetPixelType()) );
      }
    }
  else
    {
    if (this->GetComponentType() == CHAR)
      {
      m_BytePerPixel = 1;
      m_GDALComponentType = GDT_Byte;
      }
    else if (this->GetComponentType() == UCHAR)
      {
      m_BytePerPixel = 1;
      m_GDALComponentType = GDT_Byte;
      }
    else if (this->GetComponentType() == USHORT)
      {
      m_BytePerPixel = 2;
      m_GDALComponentType = GDT_UInt16;
      }
    else if (this->GetComponentType() == SHORT)
      {
      m_BytePerPixel = 2;
      m_GDALComponentType = GDT_Int16;
      }
    else if (this->GetComponentType() == INT)
      {
      m_BytePerPixel = 4;
      m_GDALComponentType = GDT_Int32;
      }
    else if (this->GetComponentType() == UINT)
      {
      m_BytePerPixel = 4;
      m_GDALComponentType = GDT_UInt32;
      }
    else if (this->GetComponentType() == LONG)
      {
        long tmp;
        m_BytePerPixel = sizeof(tmp);
        if( m_BytePerPixel == 8 )
          {
            NMProcWarn(<< "Cast a long (64 bits) image into an int (32 bits) one.")
          }
        m_GDALComponentType = GDT_Int32;
      }
    else if (this->GetComponentType() == ULONG)
      {
        unsigned long tmp;
        m_BytePerPixel = sizeof(tmp);
        if( m_BytePerPixel == 8 )
          {
            NMProcWarn(<< "Cast an unsigned long (64 bits) image into an unsigned int (32 bits) one.")
              }
        m_GDALComponentType = GDT_UInt32;
      }
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
    else if (this->GetComponentType() == LONGLONG)
    {
        m_BytePerPixel = sizeof(long long);
        if (m_BytePerPixel == 8)
        {
            NMProcWarn(<< "Cast a long long (64 bits) image into an int (32 bits) one.")
        }
        m_GDALComponentType = GDT_Int32;
    }
    else if (this->GetComponentType() == ULONGLONG)
    {
        m_BytePerPixel = sizeof(unsigned long long);
        if (m_BytePerPixel == 8)
        {
            NMProcWarn(<< "Cast an unsigned long long (64 bits) image into an unsigned int (32 bits) one.")
        }
        m_GDALComponentType = GDT_UInt32;
    }
#endif
    else if (this->GetComponentType() == FLOAT)
      {
      m_BytePerPixel = 4;
      m_GDALComponentType = GDT_Float32;
      }
    else if (this->GetComponentType() == DOUBLE)
      {
      m_BytePerPixel = 8;
      m_GDALComponentType = GDT_Float64;
      }
    else
      {
      m_BytePerPixel = 1;
      m_GDALComponentType = GDT_Byte;
      }
    }

  // Automatically set the Type to Binary for GDAL data
  this->SetFileTypeToBinary();

  driverShortName = FilenameToGdalDriverShortName(m_FileName);
  if (driverShortName == "NOT-FOUND")
    {
    NMLogError(
      << "GDAL Writing failed : the image file name '" << m_FileName.c_str() << "' is not recognized by GDAL.");
    }


  unsigned int totalRegionCols = m_Dimensions[0];
  unsigned int totalRegionLines = m_Dimensions[1];
  if (m_UseForcedLPR)
  {
      totalRegionCols = m_ForcedLPR.GetSize()[0];
      totalRegionLines = m_ForcedLPR.GetSize()[1];
  }


  if (m_CanStreamWrite)
    {

    // Force tile mode for TIFF format. Tile mode is a lot more
    // efficient when writing huge tiffs
    if( driverShortName.compare("GTiff") == 0 )
      {
      NMLogDebug(<< "Enabling TIFF Tiled mode")
      papszOptions = CSLAddNameValue( papszOptions, "TILED", "YES" );

      // Use a fixed tile size
      // Take as reference is a 256*256 short int 4 bands tile
      const unsigned int ReferenceTileSizeInBytes = 256 * 256 * 4 * 2;

      unsigned int nbPixelPerTile = ReferenceTileSizeInBytes / m_BytePerPixel / m_NbBands;
      unsigned int tileDimension = static_cast<unsigned int>( vcl_sqrt(static_cast<float>(nbPixelPerTile)) );

      // align the tile dimension to the next multiple of 16 (needed by TIFF spec)
      tileDimension = ( tileDimension + 15 ) / 16 * 16;

      NMLogDebug(<< "Tile dimension : " << tileDimension << " * " << tileDimension)

      std::ostringstream oss;
      oss << tileDimension;
      papszOptions = CSLAddNameValue( papszOptions, "BLOCKXSIZE", oss.str().c_str() );
      papszOptions = CSLAddNameValue( papszOptions, "BLOCKYSIZE", oss.str().c_str() );

      if (this->m_UseCompression)
        papszOptions = CSLAddNameValue( papszOptions, "COMPRESS", "LZW");


      }
    else if (driverShortName.compare("HFA") == 0)
     {
        //if (this->m_UseCompression)
        papszOptions = CSLAddNameValue( papszOptions, "COMPRESSED", "TRUE");
        papszOptions = CSLAddNameValue( papszOptions, "IGNOREUTM", "TRUE");
     }
    else if (driverShortName.compare("KEA") == 0)
    {
        std::stringstream cl;
        cl << this->m_CompressionLevel;
        papszOptions = CSLAddNameValue(papszOptions, "DEFLATE", cl.str().c_str());
    }

    GDALDriver* drv = GetGDALDriverManager()->GetDriverByName(driverShortName.c_str());
    if (drv != 0)
    {
        m_Dataset = (GDALDataset*)drv->Create(
                         GetGdalWriteImageFileName(driverShortName, m_FileName).c_str(),
                         totalRegionCols, totalRegionLines,
                         m_NbBands, m_GDALComponentType,
                         papszOptions);
        m_CreatedNotWritten = true;
    }
  }
  else
  {
    // buffer casted in unsigned long cause under Win32 the adress
    // don't begin with 0x, the adress in not interpreted as
    // hexadecimal but alpha numeric value, then the conversion to
    // integer make us pointing to an non allowed memory block => Crash.
    std::ostringstream stream;
    stream << "MEM:::"
           <<  "DATAPOINTER=" << (unsigned long)(buffer) << ","
           <<  "PIXELS=" << totalRegionCols << ","
           <<  "LINES=" << totalRegionLines << ","
           <<  "BANDS=" << m_NbBands << ","
           <<  "DATATYPE=" << GDALGetDataTypeName(m_GDALComponentType) << ","
           <<  "PIXELOFFSET=" << m_BytePerPixel * m_NbBands << ","
           <<  "LINEOFFSET=" << m_BytePerPixel * m_NbBands * m_Dimensions[0] << ","
           <<  "BANDOFFSET=" << m_BytePerPixel;

    m_Dataset = (GDALDataset*)GDALOpen(stream.str().c_str(), GA_ReadOnly);
    }

  if (m_Dataset == 0)
    {
    NMLogError(
      << "GDAL Writing failed : Impossible to create the image file name '" << m_FileName << "'.");
    }

  /*----------------------------------------------------------------------*/
  /*-------------------------- METADATA ----------------------------------*/
  /*----------------------------------------------------------------------*/

  /* -------------------------------------------------------------------- */
  /* Set the LAYER_TYPE for HFA-based raster bands                        */
  /* -------------------------------------------------------------------- */
  if (driverShortName.compare("HFA") == 0)
  {
      for (int t=0; t < this->m_vecRAT.size(); ++t)
      {
          m_Dataset->GetRasterBand(t+1)->SetMetadataItem("LAYER_TYPE", "thematic");
          m_Dataset->GetRasterBand(t+1)->SetMetadataItem("STATISTICS_HISTOBINFUNCTION",
                                                                       "direct");
      }
  }


  // Now initialize the itk dictionary
  itk::MetaDataDictionary& dict = this->GetMetaDataDictionary();
  std::ostringstream oss;
  //GDALDataset* dataset = m_Dataset->GetDataSet();

  std::string projectionRef;
  itk::ExposeMetaData<std::string>(dict, MetaDataKey::ProjectionRefKey, projectionRef);

  /* -------------------------------------------------------------------- */
  /* Set the GCPs                                                          */
  /* -------------------------------------------------------------------- */
  const double Epsilon = 1E-10;
  if (projectionRef.empty()
      &&  (vcl_abs(m_Origin[0]) > Epsilon
           || vcl_abs(m_Origin[1]) > Epsilon
           || vcl_abs(m_Spacing[0] - 1) > Epsilon
           || vcl_abs(m_Spacing[1] - 1) > Epsilon) )
    {
    // See issue #303 :
    // If there is no ProjectionRef, and the GeoTransform is not the identity,
    // then saving also GCPs is undefined behavior for GDAL, and a WGS84 projection crs
    // is assigned arbitrarily
    //NMProcWarn(<< "Skipping GCPs saving to prevent GDAL from assigning a WGS84 projection ref to the file")
    }
  else
    {
    unsigned int gcpCount = 0;
    itk::ExposeMetaData<unsigned int>(dict, MetaDataKey::GCPCountKey, gcpCount);

    if (gcpCount > 0)
      {

      GDAL_GCP * gdalGcps = new GDAL_GCP[gcpCount];

      for (unsigned int gcpIndex = 0; gcpIndex < gcpCount; ++gcpIndex)
        {
        //Build the GCP string in the form of GCP_n
        std::ostringstream lStream;
        lStream << MetaDataKey::GCPParametersKey << gcpIndex;
        std::string key = lStream.str();

        GCP gcp;
        itk::ExposeMetaData<GCP>(dict, key, gcp);

        gdalGcps[gcpIndex].pszId = const_cast<char *>(gcp.m_Id.c_str());
        gdalGcps[gcpIndex].pszInfo = const_cast<char *>(gcp.m_Info.c_str());
        gdalGcps[gcpIndex].dfGCPPixel = gcp.m_GCPCol;
        gdalGcps[gcpIndex].dfGCPLine = gcp.m_GCPRow;
        gdalGcps[gcpIndex].dfGCPX = gcp.m_GCPX;
        gdalGcps[gcpIndex].dfGCPY = gcp.m_GCPY;
        gdalGcps[gcpIndex].dfGCPZ = gcp.m_GCPZ;
        }

      std::string gcpProjectionRef;
      itk::ExposeMetaData<std::string>(dict, MetaDataKey::GCPProjectionKey, gcpProjectionRef);

      m_Dataset->SetGCPs(gcpCount, gdalGcps, gcpProjectionRef.c_str());

      delete[] gdalGcps;
      }
    }

  /* -------------------------------------------------------------------- */
  /* Set the projection coordinate system of the image : ProjectionRef    */
  /* -------------------------------------------------------------------- */
  if (!projectionRef.empty())
    {
    m_Dataset->SetProjection(projectionRef.c_str());
    }

  /* -------------------------------------------------------------------- */
  /*  Set the six coefficients of affine geoTransform                     */
  /* -------------------------------------------------------------------- */
  itk::VariableLengthVector<double> geoTransform(6);

  MetaDataKey::VectorType upperLeft;
  itk::ExposeMetaData<MetaDataKey::VectorType>(dict, MetaDataKey::UpperLeftCornerKey, upperLeft);

  // Setting the GeoTransform
  geoTransform[0] = m_Origin[0] - 0.5 * m_Spacing[0];
  geoTransform[3] = m_Origin[1] - 0.5 * m_Spacing[1];
  geoTransform[1] = m_Spacing[0];
  geoTransform[5] = m_Spacing[1];

  // FIXME: Here component 1 and 4 should be replaced by the orientation parameters
  geoTransform[2] = 0.;
  geoTransform[4] = 0.;
  m_Dataset->SetGeoTransform(const_cast<double*>(geoTransform.GetDataPointer()));

  /* -------------------------------------------------------------------- */
  /*      Report metadata.                                                */
  /* -------------------------------------------------------------------- */

  std::string              svalue = "";
  std::vector<std::string> keys = dict.GetKeys();
  std::string const metadataKey = MetaDataKey::MetadataKey;

  for (unsigned int itkey = 0; itkey < keys.size(); ++itkey)
    {
    if (keys[itkey].compare(0, metadataKey.length(), metadataKey) == 0)
      {
      itk::ExposeMetaData<std::string>(dict, keys[itkey], svalue);
      unsigned int equalityPos = svalue.find_first_of('=');
      std::string  tag = svalue.substr(0, equalityPos);
      std::string  value = svalue.substr(equalityPos + 1);
      NMLogDebug(<< "Metadata: " << tag << "=" << value);
      m_Dataset->SetMetadataItem(tag.c_str(), value.c_str(), NULL);
      }
    }
  // END

  // Dataset info
  NMLogDebug( << "**** WriteImageInformation() DATASET INFO: ****" );
  NMLogDebug( << "Projection Ref: "<<m_Dataset->GetProjectionRef() );
  double GT[6];
  if (m_Dataset->GetGeoTransform(GT) == CE_None)
    {
    NMLogDebug( <<"Geo Transform: "<< GT[0] << ", " << GT[1] << ", "
                                 << GT[2] << ", " << GT[3] << ", "
                                 << GT[4] << ", " << GT[5] );
    }
  else
    {
    NMLogDebug( << "No Geo Transform: ");
    }

  NMLogDebug( << "GCP Projection Ref: "<< m_Dataset->GetGCPProjection() );
  NMLogDebug( << "GCP Count: " << m_Dataset->GetGCPCount() );


  // we now write the RAT's
  NMLogDebug( << "Writing RATs ...");
  for (unsigned int ti = 0; ti < this->m_vecRAT.size(); ++ti)
  {
    otb::AttributeTable::Pointer tab = this->m_vecRAT.at(ti);
    if (tab.IsNull())
        continue;

    // for now, we assume ti=band
    this->WriteRAT(tab, ti+1);
  }

}

std::string GDALRATImageIO::FilenameToGdalDriverShortName(const std::string& name) const
{
  std::string extension;
  std::string gdalDriverShortName;

  // Get extension in lowercase
  extension = itksys::SystemTools::LowerCase( itksys::SystemTools::GetFilenameLastExtension(name) );
  extension = extension.substr(1, extension.size()-1);

  if      ( extension == "tif" || extension == "tiff" )
    gdalDriverShortName = "GTiff";
  else if ( extension == "hdr" )
    gdalDriverShortName = "ENVI";
  else if ( extension == "img" )
    gdalDriverShortName = "HFA";
  else if ( extension == "ntf" )
    gdalDriverShortName = "NITF";
  else if ( extension == "png" )
    gdalDriverShortName="PNG";
  else if ( extension == "jpg" || extension== "jpeg" )
    gdalDriverShortName="JPEG";
  else if ( extension == "pix" )
    gdalDriverShortName="PCIDSK";
  else if ( extension == "lbl" || extension == "pix" )
    gdalDriverShortName="ISIS2";
  else if ( extension == "kea")
    gdalDriverShortName="KEA";
  else
    gdalDriverShortName = "NOT-FOUND";

  return gdalDriverShortName;
}

std::string GDALRATImageIO::GetGdalWriteImageFileName(const std::string& gdalDriverShortName, const std::string& filename) const
{
  std::string gdalFileName;

  gdalFileName = filename;
  // Suppress hdr extension for ENVI format
  if (gdalDriverShortName == "ENVI")
    {
    gdalFileName = System::GetRootName(filename);
    }
  return gdalFileName;
}

bool GDALRATImageIO::GDALInfoReportCorner(const char * /*corner_name*/, double x, double y, double& GeoX, double& GeoY) const
{
  const char *pszProjection;
  double      adfGeoTransform[6];

  bool IsTrue;

  if (m_Dataset == 0)
  {
      return false;
  }

  /* -------------------------------------------------------------------- */
  /*      Transform the point into georeferenced coordinates.             */
  /* -------------------------------------------------------------------- */
  if (m_Dataset->GetGeoTransform(adfGeoTransform) == CE_None)
    {
    pszProjection = m_Dataset->GetProjectionRef();

    GeoX = adfGeoTransform[0] + adfGeoTransform[1] * x
           + adfGeoTransform[2] * y;
    GeoY = adfGeoTransform[3] + adfGeoTransform[4] * x
           + adfGeoTransform[5] * y;
    IsTrue = true;
    }

  else
    {
    GeoX = x;
    GeoY = y;
    IsTrue = false;
    }

  return IsTrue;
}

AttributeTable::Pointer GDALRATImageIO::ReadRAT(unsigned int iBand)
{
    AttributeTable::Pointer tab = 0;
    RAMTable::Pointer rtab = 0;
    SQLiteTable::Pointer stab = 0;
    switch(m_RATType)
    {
    case AttributeTable::ATTABLE_TYPE_RAM:
        rtab = InternalReadRAMRAT(iBand);
        tab = rtab.GetPointer();//static_cast<AttributeTable*>(rtab.GetPointer());
        break;
    case AttributeTable::ATTABLE_TYPE_SQLITE:
        stab = InternalReadSQLiteRAT(iBand);
        tab = stab.GetPointer();//static_cast<AttributetTable*>(stab.GetPointer());
        break;
    default:
        return 0;
    }

    return tab;
}


RAMTable::Pointer GDALRATImageIO::InternalReadRAMRAT(unsigned int iBand)
{
    // if m_Dataset hasn't been instantiated before, we do it here, because
    // we might want to fetch the attribute table before the pipeline has
    // been executed
    GDALDataset* img;
    bool bClose = false;
    if (m_Dataset == 0)
    {
        m_Dataset = (GDALDataset*)GDALOpen(this->GetFileName(), GA_ReadOnly);
        if (m_Dataset == 0)
            return 0;
        bClose = true;
    }

    // data set already available?
    //img = m_Dataset->GetDataSet();
//    if (m_Dataset == 0)
//    {
//        //NMProcWarn(<< "ReadRAT: unable to access data set!");
//        //NMLogError(<< "ReadRAT: unable to access data set!");
//        return 0;
//    }

    // how many bands? (band index is 1-based)
    if (m_Dataset->GetRasterCount() < iBand)
    {
        if (bClose) this->CloseDataset();
        return 0;
    }

    // get the RAT for the specified band
#ifdef GDAL_NEWRATAPI
    GDALRasterAttributeTable* rat = m_Dataset->GetRasterBand(iBand)->GetDefaultRAT();
#else
    const GDALRasterAttributeTable* rat = m_Dataset->GetRasterBand(iBand)->GetDefaultRAT();
#endif
    if (rat == 0)
    {
        if (bClose) this->CloseDataset();
        return 0;
    }

    // double check, whether the table actually contains some data
    int nrows = rat->GetRowCount();
    int ncols = rat->GetColumnCount();
    if (nrows == 0 || ncols == 0)
    {
        if (bClose) this->CloseDataset();
        return 0;
    }

    // copy gdal tab into otbAttributeTable
    RAMTable::Pointer otbTab = RAMTable::New();

    // set filename and band number
    otbTab->SetBandNumber(iBand);

    //NMDebugAI(<< "filename we want to set: '" << this->GetFileName() << "'" << std::endl);
    otbTab->SetImgFileName(this->GetFileName());

    //otbTab->AddColumn("rowidx", AttributeTable::ATTYPE_INT);
    std::vector< std::string > colnames;

    // go and check the column names against the SQL standard, in case
    // they don't match it, we enclose in double quotes.
    std::string colIdName;
    int fstIntIdx = -1;
    for (int c=0; c < ncols; ++c)
    {
        if (rat->GetTypeOfCol(c) == GFT_Integer)
        {
            if (    strcmp(rat->GetNameOfCol(c), "rowid") == 0
                ||  strcmp(rat->GetNameOfCol(c), "rowidx") == 0
               )
            {
                    colIdName = rat->GetNameOfCol(c);
            }

            if (fstIntIdx == -1)
            {
                fstIntIdx = c;
            }
        }

        colnames.push_back(rat->GetNameOfCol(c));
    }

    if (colIdName.empty())
    {
        fstIntIdx = fstIntIdx < 0 ? 0 : fstIntIdx;
        colIdName = rat->GetNameOfCol(fstIntIdx);
    }

    //otbTab->AddColumn(colIdName, AttributeTable::ATTYPE_INT);


    // copy table
    ::GDALRATFieldType gdaltype;
    AttributeTable::TableColumnType otbtabtype;
    for (int c=0; c < ncols; ++c)
    {
        gdaltype = rat->GetTypeOfCol(c);
        std::string colname = colnames[c];

        switch(gdaltype)
        {
        case GFT_Integer:
            otbtabtype = AttributeTable::ATTYPE_INT;
            break;
        case GFT_Real:
            otbtabtype = AttributeTable::ATTYPE_DOUBLE;
            break;
        case GFT_String:
            otbtabtype = AttributeTable::ATTYPE_STRING;
            break;
        default:
            return 0;
        }
        otbTab->AddColumn(colname, otbtabtype);
    }
    otbTab->AddRows(nrows);

#ifdef GDAL_NEWRATAPI
    // the new way - chunk of rows by chunk of rows
    // might want to turn that into a user-specified variable
    int chunksize = nrows < 5000 ? nrows : 5000;
    int rest=0;
    int numChunks=0;
    if (nrows <= chunksize)
    {
        numChunks = 1;
    }
    else
    {
        numChunks =  nrows / chunksize;
        rest = nrows - (numChunks * chunksize);
        if (rest == 0)
        {
            ++numChunks;
        }
    }

    int procrows = 0;
    int s=0;
    int e=0;
    for (int chunk = 0; chunk < numChunks+1; ++chunk)
    {
        s = procrows;
        e = s + chunksize;// - 1;

        for (int col=0; col < ncols; ++col)
        {
            gdaltype = rat->GetTypeOfCol(col);
            void* colPtr = 0;

            //            if (col == 0)
            //            {
            //                colPtr = otbTab->GetColumnPointer(col);
            //                long* valPtr = static_cast<long*>(colPtr);
            //                for (int r=s; r < e; ++r)
            //                {
            //                    valPtr[r] = r;
            //                }
            //            }
            //colPtr = otbTab->GetColumnPointer(col+1);
            colPtr = otbTab->GetColumnPointer(col);

            switch(gdaltype)
            {
            case GFT_Integer:
            {
                long long* valPtr = static_cast<long long*>(colPtr);
                int* tptr = (int*)CPLCalloc(sizeof(int), chunksize);
                rat->ValuesIO(GF_Read, col, s, chunksize, tptr);
                for (int k=0; k < chunksize; ++k)
                {
                    valPtr[s+k] = static_cast<long long>(tptr[k]);
                }
                CPLFree(tptr);
            }
                break;
            case GFT_Real:
            {
                double* valPtr = static_cast<double*>(colPtr);
                rat->ValuesIO(GF_Read, col, s, chunksize, (valPtr+s));
            }
                break;
            case GFT_String:
            {
                char** valPtr = (char**)CPLCalloc(sizeof(char*), chunksize);
                if (valPtr == 0)
                {
                    NMLogError(<< "Not enough memory to allocate chunk of string records!");
                    break;
                }
                rat->ValuesIO(GF_Read, col, s, chunksize, valPtr);
                for (int k=0; k < chunksize; ++k)
                {
                    const char* ccv = valPtr[k] != 0 ? valPtr[k] : "NULL";
                    //otbTab->SetValue(col+1, s+k, ccv);
                    otbTab->SetValue(col, s+k, ccv);
                    CPLFree(valPtr[k]);
                }
                CPLFree(valPtr);
            }
                break;
            default:
                continue;
            }
        }

        procrows += chunksize;

        if (chunk == numChunks-1)
        {
            if (rest > 0)
            {
                chunksize = rest;
            }
            else
            {
                chunk = numChunks;
            }
        }
    }
#else
    // the old way - row by row
    for (int c=0; c < ncols; ++c)
    {
        gdaltype = rat->GetTypeOfCol(c);
        for (int r=0; r < nrows; ++r)
        {
            //            if (c==0)
            //            {
            //                otbTab->SetValue(c, r, (long)r);
            //            }

            switch (gdaltype)
            {
            case GFT_Integer:
                //otbTab->SetValue(c+1, r, (long)rat->GetValueAsInt(r, c));
                otbTab->SetValue(c, r, (long long)rat->GetValueAsInt(r, c));
                break;
            case GFT_Real:
                //otbTab->SetValue(c+1, r, rat->GetValueAsDouble(r, c));
                otbTab->SetValue(c, r, rat->GetValueAsDouble(r, c));
                break;
            case GFT_String:
                //otbTab->SetValue(c+1, r, rat->GetValueAsString(r, c));
                otbTab->SetValue(c, r, rat->GetValueAsString(r, c));
                break;
            default:
                continue;
            }
        }
    }
#endif

    //rat = 0;
    //img = 0;

    //NMDebug(<< std::endl);
    //NMDebugAI(<< "now pritn' the actual Table ...");
    //otbTab->Print(std::cout, itk::Indent(0), 100);
    if (bClose) this->CloseDataset();

    return otbTab;
}


SQLiteTable::Pointer GDALRATImageIO::InternalReadSQLiteRAT(unsigned int iBand)
{
    //CALLGRIND_START_INSTRUMENTATION;

    // if m_Dataset hasn't been instantiated before, we do it here, because
    // we might want to fetch the attribute table before the pipeline has
    // been executed
    GDALDataset* img;
    bool bClose = false;
    if (m_Dataset == 0)
    {
        m_Dataset = (GDALDataset*)GDALOpen(this->GetFileName(), GA_ReadOnly);
        if (m_Dataset == 0)
            return 0;
        bClose = true;
    }

    // data set already available?
    //img = m_Dataset->GetDataSet();
//	if (img == 0)
//	{
//		//NMProcWarn(<< "ReadRAT: unable to access data set!");
//		//NMLogError(<< "ReadRAT: unable to access data set!");
//		return 0;
//	}

    // how many bands? (band index is 1-based)
    if (m_Dataset->GetRasterCount() < iBand)
    {
        if (bClose) this->CloseDataset();
        return 0;
    }

    // format the database filename for the external lumass db file
    // copy gdal tab into otbAttributeTable
    std::string imgFN = this->m_FileName;
    std::string dbFN = imgFN;
    size_t pos = dbFN.find_last_of('.');
    if (pos > 0)
    {
        dbFN = dbFN.substr(0, pos);
    }
    dbFN += ".ldb";

    // get the RAT for the specified band
#ifdef GDAL_NEWRATAPI
    GDALRasterAttributeTable* rat = m_Dataset->GetRasterBand(iBand)->GetDefaultRAT();
#else
    const GDALRasterAttributeTable* rat = m_Dataset->GetRasterBand(iBand)->GetDefaultRAT();
#endif
    if (rat == 0)
    {
        if (bClose) this->CloseDataset();
        return 0;
    }

    // double check, whether the table actually contains some data
    int nrows = rat->GetRowCount();
    int ncols = rat->GetColumnCount();
    if (nrows == 0 || ncols == 0)
    {
        // just test whether we've got an external ldb file here before
        // we attempt to actually create a proper SQLiteTable
        std::ifstream filestr(dbFN.c_str());
        if (filestr)
        {
            filestr.close();
        }
        else
        {
            if (bClose) this->CloseDataset();
            return 0;
        }

        std::stringstream ssband;
        ssband << iBand;
        // double check, whether there's an external lumass db file out there,
        // which we could use instead
        SQLiteTable::Pointer ldbTab = SQLiteTable::New();
        ldbTab->SetOpenReadOnly(m_DbRATReadOnly);
        if (ldbTab->CreateTable(dbFN, ssband.str()) == SQLiteTable::ATCREATE_READ)
        {
            return ldbTab;
        }
        ldbTab->DeleteDatabase();
        ldbTab = 0;
        return 0;
    }

    // establish, whether we need an extra rowidx or whether
    // it is already included
    bool bRowIdx = false;
    std::string idColName = "";
    for (int c=0; c < ncols; ++c)
    {
        std::string colName = rat->GetNameOfCol(c);
        std::transform(colName.begin(), colName.end(), colName.begin(), tolower);
        if (rat->GetTypeOfCol(c) == GFT_Integer)
        {
            if (    strcmp(colName.c_str(), "rowidx") == 0
                ||  strcmp(colName.c_str(), "rowid") == 0
                ||  strcmp(colName.c_str(), "rowno") == 0
                ||  strcmp(colName.c_str(), "row") == 0
                //||  strcmp(colName.c_str(), "value") == 0
               )
            {
                bRowIdx = true;
                idColName = rat->GetNameOfCol(c);
                break;
            }
        }
    }

    std::stringstream tag;
    tag << iBand;

    SQLiteTable::Pointer otbTab = SQLiteTable::New();
    otbTab->SetRowIDColName(idColName);
    otbTab->SetOpenReadOnly(m_DbRATReadOnly);
    switch(otbTab->CreateTable(dbFN, tag.str()))
    {
    case SQLiteTable::ATCREATE_ERROR:
        NMProcWarn(<< "Couldn't create attribute table '"
                        << dbFN << "'!")
        if (bClose) this->CloseDataset();
        return 0;
        break;
    case SQLiteTable::ATCREATE_READ:
        if (bClose) this->CloseDataset();
        return otbTab;
        break;
    }


    // set filename and band number
    otbTab->SetBandNumber(iBand);

    //NMDebugAI(<< "filename we want to set: '" << this->GetFileName() << "'" << std::endl);
    otbTab->SetImgFileName(this->GetFileName());

    // the rowidx field is handled internally in otb::AttributeTable's slqite-based
    // implementation
        //otbTab->AddColumn("rowidx", AttributeTable::ATTYPE_INT);

    std::vector< std::string > colnames;
    std::vector< otb::AttributeTable::TableColumnType > coltypes;

    idColName = otbTab->GetPrimaryKey();

#ifdef GDAL_NEWRATAPI
    std::vector< int > colpos;
    //std::vector< AttributeTable::ColumnValue > colValues;
    //colValues.resize(ncols+1);

    // pointer to column value stores
    std::vector< int* > int_cols;
    std::vector< double* > dbl_cols;
    std::vector< char** > chr_cols;


    // prepare iteration over the RAT when employing the
    // new GDAL RAT API

    // the new way - chunk of rows by chunk of rows
    // might want to turn that into a user-specified variable
    int chunksize = nrows < 5000 ? nrows : 5000;
    int _chunksize = chunksize;
    int rest=0;
    int numChunks=0;
    if (nrows <= chunksize)
    {
        numChunks = 1;
    }
    else
    {
        numChunks =  nrows / chunksize;
        rest = nrows - (numChunks * chunksize);
        if (rest == 0)
        {
            ++numChunks;
        }
    }

    // store admin info about the cols to be read
    // check, whether we've got a 'rowidx' to be read
    // or whether we let the db popluate it for us
    if (!bRowIdx)
    {
        colnames.push_back(idColName);
        coltypes.push_back(otb::AttributeTable::ATTYPE_INT);
        int* iptr = (int*)CPLCalloc(sizeof(int), chunksize);
        int_cols.push_back(iptr);
        colpos.push_back(0);
    }

#else
    int chunksize = nrows;
    if (!bRowIdx)
    {
        //colnames.push_back("rowidx");
        colnames.push_back(idColName);
    }
#endif

    otbTab->BeginTransaction();
    int idxCorr = 0;
    if (!bRowIdx)
    {
        //otbTab->AddColumn(colnames[0], coltypes[0]);
        idxCorr = 1;
    }

    for (int c=0; c < ncols; ++c)
    {
        colnames.push_back(rat->GetNameOfCol(c));
#ifdef GDAL_NEWRATAPI
        switch(rat->GetTypeOfCol(c))
        {
            case GFT_Integer:
            {
                coltypes.push_back(otb::AttributeTable::ATTYPE_INT);
                int* iptr = (int*)CPLCalloc(sizeof(int), chunksize);
                int_cols.push_back(iptr);
                colpos.push_back(int_cols.size()-1);
            }
            break;

            case GFT_Real:
            {
                coltypes.push_back(otb::AttributeTable::ATTYPE_DOUBLE);
                double* dptr = (double*)CPLCalloc(sizeof(double), chunksize);
                dbl_cols.push_back(dptr);
                colpos.push_back(dbl_cols.size()-1);
            }
            break;

            case GFT_String:
            {
                coltypes.push_back(otb::AttributeTable::ATTYPE_STRING);
                char** cptr = (char**)CPLCalloc(sizeof(char*), chunksize);
                chr_cols.push_back(cptr);
                colpos.push_back(chr_cols.size()-1);
            }
            break;
        }
#else
        switch(rat->GetTypeOfCol(c))
        {
        case GFT_Integer: coltypes.push_back(otb::AttributeTable::ATTYPE_INT);
            break;
        case GFT_Real: coltypes.push_back(otb::AttributeTable::ATTYPE_DOUBLE);
            break;
        case GFT_String: coltypes.push_back(otb::AttributeTable::ATTYPE_STRING);
            break;
        }
#endif
        // NOTE: with 'rowidx' we've got one addtional columnn
        // over the actual columns in the table
        //if (colnames[c+idxCorr].compare(idColName) != 0)
        {
            otbTab->AddColumn(colnames[c+idxCorr], coltypes[c+idxCorr]);
        }
    }
    otbTab->EndTransaction();
    otbTab->PrepareBulkSet(colnames, true);
    otbTab->BeginTransaction();

#ifdef GDAL_NEWRATAPI
    int procrows = 0;
    int s=0;
    int e=0;

    ::GDALRATFieldType gdaltype;
    for (int chunk = 0; chunk < numChunks+1; ++chunk)
    {
        s = procrows;
        e = s + chunksize;

        // NOTE: because of 'rowidx' we've got one
        // additional col over the columns
        // in the actual table, hence, we've got tcol
        // for accessing the correct column index stored in
        // colpos[]
        int tcol = idxCorr;
        for (int col=0; col < ncols; ++col, ++tcol)
        {
            gdaltype = rat->GetTypeOfCol(col);

            // populate the 'rowidx' column with
            // zero-based index
            if (col == 0 && !bRowIdx)
            {
                for (int r=s, ch=0; r < e; ++r, ++ch)
                {
                    int_cols[0][ch] = r;
                }
            }

            switch(gdaltype)
            {
                case GFT_Integer:
                {
                    rat->ValuesIO(GF_Read, col, s, chunksize,
                                  int_cols[colpos[tcol]]);
                }
                break;

                case GFT_Real:
                {
                    rat->ValuesIO(GF_Read, col, s, chunksize,
                                  dbl_cols[colpos[tcol]]);
                }
                break;

                case GFT_String:
                {
                    rat->ValuesIO(GF_Read, col, s, chunksize,
                                  chr_cols[colpos[tcol]]);
                }
                break;

            default:
                continue;
            }
        }

        for (int k=0; k < chunksize; ++k)
        {
            otbTab->DoPtrBulkSet(int_cols, dbl_cols, chr_cols,
                              colpos, k);

            // clear strings allocated by GDAL
            for (int g=0; g < chr_cols.size(); ++g)
            {
                CPLFree(chr_cols[g][k]);
            }
        }

        procrows += chunksize;

        if (chunk == numChunks-1)
        {
            if (rest > 0)
            {
                chunksize = rest;
            }
            else
            {
                chunk = numChunks;
            }
        }
    }


    // release memory
    for (int c=0; c < coltypes.size(); ++c)
    {
        switch(coltypes[c])
        {
        case otb::AttributeTable::ATTYPE_INT:
            CPLFree(int_cols[colpos[c]]);
            break;

        case otb::AttributeTable::ATTYPE_STRING:
            CPLFree(chr_cols[colpos[c]]);
            break;

        case otb::AttributeTable::ATTYPE_DOUBLE:
            CPLFree(dbl_cols[colpos[c]]);
            break;
        }
    }

#else
    // the old way - row by row
    std::vector< otb::AttributeTable::ColumnValue > colValues;
    if (!bRowIdx)
    {
        colValues.resize(ncols+1);
    }
    else
    {
        colValues.resize(ncols);
    }

    for (int r=0; r < nrows; ++r)
    {
        if (!bRowIdx)
        {
            colValues[0].type = AttributeTable::ATTYPE_INT;
            colValues[0].ival = r;
        }
        for (int c=0; c < ncols; ++c)
        {
            gdaltype = rat->GetTypeOfCol(c);
            switch (gdaltype)
            {
            case GFT_Integer:
                colValues[c+idxCorr].type = AttributeTable::ATTYPE_INT;
                colValues[c+idxCorr].ival = rat->GetValueAsInt(r, c);
                break;
            case GFT_Real:
                colValues[c+idxCorr].type = AttributeTable::ATTYPE_DOUBLE;
                colValues[c+idxCorr].dval = rat->GetValueAsDouble(r, c);
                break;
            case GFT_String:
                colValues[c+idxCorr].type = AttributeTable::ATTYPE_STRING;
                colValues[c+idxCorr].tval = const_cast<char*>(rat->GetValueAsString(r, c));
                break;
            default:
                continue;
            }
        }
        otbTab->DoBulkSet(colValues, -1);
    }
#endif

    otbTab->EndTransaction();

    //CALLGRIND_STOP_INSTRUMENTATION;
    //CALLGRIND_DUMP_STATS;
    if (bClose) this->CloseDataset();
    return otbTab;
}

void
GDALRATImageIO::setRasterAttributeTable(AttributeTable* rat, int band)
{
    if (band < 1)
        return;

    if (this->m_vecRAT.capacity() < band)
        this->m_vecRAT.resize(band);

    this->m_vecRAT[band-1] = rat;
}


bool
GDALRATImageIO::TableStructureChanged(AttributeTable::Pointer tab, unsigned int iBand)
{
    bool bClose = false;
    if (m_Dataset == 0)
    {
        m_Dataset = (GDALDataset*)GDALOpen(this->GetFileName(), GA_ReadOnly);
        if (m_Dataset == 0)
            return false;
        bClose = true;
    }

#ifdef GDAL_NEWRATAPI
    GDALRasterAttributeTable* gdaltab = m_Dataset->GetRasterBand(iBand)->GetDefaultRAT();
#else
    const GDALRasterAttributeTable* gdaltab = m_Dataset->GetRasterBand(iBand)->GetDefaultRAT();
#endif

    if (gdaltab == 0)
    {
        if (bClose) this->CloseDataset();
        return false;
    }

    int curNumCols = gdaltab->GetColumnCount();
    int newNumCols = tab->GetNumCols();
    if (curNumCols != newNumCols)
    {
        if (bClose) this->CloseDataset();
        return true;
    }

    for (int c=0; c < curNumCols; ++c)
    {
        if (tab->GetColumnName(c).compare(gdaltab->GetNameOfCol(c)) != 0)
        {
            if (bClose) this->CloseDataset();
            return true;
        }
    }

    return false;
}

GDALDataset* GDALRATImageIO::CreateCopy()
{
    // We only wrote data to the memory dataset
    // Now write it to the real file with CreateCopy()
    std::string gdalDriverShortName = FilenameToGdalDriverShortName(
            m_FileName);
    std::string realFileName = GetGdalWriteImageFileName(
            gdalDriverShortName, m_FileName);

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(
                    gdalDriverShortName.c_str());
    if (driver == NULL)
    {
        NMLogError(
                << "Unable to instantiate driver " << gdalDriverShortName
                    << " to write " << m_FileName);
    }

    // handle creation options
    char ** option = 0;
    if (gdalDriverShortName.compare("JPEG") == 0)
    {
        // If JPEG, set the JPEG compression quality to 95.
        option = CSLAddNameValue(option, "QUALITY", "95");

    }
    else if (gdalDriverShortName.compare("HFA") == 0)
    {
        option = CSLAddNameValue(option, "COMPRESSED", "YES");
        option = CSLAddNameValue(option, "IGNOREUTM", "YES");
    }
    else if (gdalDriverShortName.compare("KEA") == 0)
    {
        std::stringstream cl;
        cl << this->m_CompressionLevel;
        option = CSLAddNameValue(option, "DEFLATE", cl.str().c_str());
    }
    else if (gdalDriverShortName.compare("GTiff") == 0)
    {
        option = CSLAddNameValue(option, "COMPRESS", "LZW");
    }

    GDALDataset* hOutputDS = driver->CreateCopy(realFileName.c_str(),
            m_Dataset, FALSE, option, NULL, NULL);

    if (gdalDriverShortName.compare("HFA") == 0 && hOutputDS != 0)
    {
        for (int t=0; t < this->m_vecRAT.size(); ++t)
        {
            hOutputDS->GetRasterBand(t+1)->SetMetadataItem("LAYER_TYPE",
                                                                         "thematic");
            hOutputDS->GetRasterBand(t+1)->SetMetadataItem("STATISTICS_HISTOBINFUNCTION",
                                                                         "direct");
        }
    }

    return hOutputDS;
}


void
GDALRATImageIO::WriteRAT(AttributeTable::Pointer intab, unsigned int iBand)
{
    if (intab.IsNull())
    {
        return;
    }

    switch(intab->GetTableType())
    {
    case AttributeTable::ATTABLE_TYPE_RAM:
        return InternalWriteRAMRAT(intab, iBand);
        break;
    case AttributeTable::ATTABLE_TYPE_SQLITE:
        return InternalWriteSQLiteRAT(intab, iBand);
        break;
    default:
        return;
    }
}

void
GDALRATImageIO::InternalWriteRAMRAT(AttributeTable::Pointer intab, unsigned int iBand)
{
    // if m_Dataset hasn't been instantiated before, we do it here, because
    // we just do an independent write of the RAT into the data set
    // (i.e. outside any pipeline activities ...)
    bool bClose = false;
    if (m_Dataset == 0)
    {
        m_Dataset = (GDALDataset*)GDALOpen(this->GetFileName(), GA_Update);
        if (m_Dataset == 0)
        {
            std::cout << "Sorry, couldn't open raster layer for RAT update!" << std::endl;
            return;
        }
        bClose = true;
    }

    // data set already available?
//    ds = m_Dataset->GetDataSet();
//    if (ds == 0)
//    {
//        //std::cout << "Sorry, couldn't open raster layer for RAT update!" << std::endl;
//        NMProcWarn(<< "ReadRAT: unable to access data set!");
//        //NMLogError(<< "ReadRAT: unable to access data set!");
//        return;
//    }

    // how many bands? (band index is 1-based)
    if (m_Dataset->GetRasterCount() < iBand)
    {
        if (bClose) this->CloseDataset();
        return;
    }

    // fetch the band
    GDALRasterBand* band = m_Dataset->GetRasterBand(iBand);

    // create a new raster attribute table
    // note: we just create a whole new table and replace the
    // current one; we don't bother with just writing changed
    // values (too lazy for doing the required housekeeping
    // beforehand) ...
#ifdef GDAL_NEWRATAPI
    GDALDefaultRasterAttributeTable* gdaltab = new GDALDefaultRasterAttributeTable();
#else
    GDALRasterAttributeTable* gdaltab = new GDALRasterAttributeTable();
#endif

    RAMTable::Pointer tab = static_cast<otb::RAMTable*>(intab.GetPointer());
    int rowcount = tab->GetNumRows();
    // if we've got an IMAGINE file with UCHAR data type, we create
    // a table with at least 256 rows, otherwise ERDAS Imagine wouldn't like it
    GDALDriverH driver = GDALGetDatasetDriver(m_Dataset);
    std::string dsn = GDALGetDriverShortName(driver);
    if (dsn.compare("HFA") == 0 && this->m_ComponentType == otb::ImageIOBase::UCHAR)
    {
        rowcount = rowcount < 256 ? 256 : rowcount;
    }

    //gdaltab->SetRowCount(tab->GetNumRows());
    gdaltab->SetRowCount(rowcount);

    // add the category field "Value"
    //gdaltab->CreateColumn("Value", GFT_Integer, GFU_MinMax);
    CPLErr err;


    // write all columns to the output table
    for (int col = 0; col < tab->GetNumCols(); ++col)
    {
        GDALRATFieldType type;
        switch(tab->GetColumnType(col))
        {
        case otb::AttributeTable::ATTYPE_INT:
            type = GFT_Integer;
            break;
        case otb::AttributeTable::ATTYPE_DOUBLE:
            type = GFT_Real;
            break;
        case otb::AttributeTable::ATTYPE_STRING:
            type = GFT_String;
            break;
        }

        GDALRATFieldUsage usage = GFU_Generic;
        err = gdaltab->CreateColumn(tab->GetColumnName(col).c_str(),
                            type, usage);
        if (err == CE_Failure)
        {
            NMLogError(<< "Failed creating column #" << col
                    << " '" << tab->GetColumnName(col).c_str() << "!");
        }
        NMLogDebug(<< "Created column #" << col << " '"
                << tab->GetColumnName(col).c_str() << "'");
    }

    // copy values row by row
    // ... and got to go with what GDAL offers ...
    int nrows = static_cast<int>(tab->GetNumRows());
    for (int row=0; row < nrows; ++row)
    {
        for (int col=0; col < tab->GetNumCols(); ++col)
        {
            //            NMLogDebug(<< "Setting value: col=" << col
            //                    << " row=" << row << " value=" << tab->GetStrValue(col, row).c_str());
            switch(tab->GetColumnType(col))
            {
            case otb::AttributeTable::ATTYPE_INT:
                gdaltab->SetValue(row, col, (int)tab->GetIntValue(col, row));
                break;
            case otb::AttributeTable::ATTYPE_DOUBLE:
                gdaltab->SetValue(row, col, tab->GetDblValue(col, row));
                break;
            case otb::AttributeTable::ATTYPE_STRING:
                gdaltab->SetValue(row, col, tab->GetStrValue(col, row).c_str());
                break;
            default:
                delete gdaltab;
                NMProcWarn(<< "Unrecognised field type! Couldn't set value col=" << col
                        << " row=" << row << " value=" << tab->GetStrValue(col, row).c_str());
                break;
            }
        }
    }

    // associate the table with the band
    // (only if there is actually a record in the table yet,
    // it could be we're in update mode and the table is only
    // being made available once all pixels have been processed
    // (e.g. SumZones))
    if (rowcount && gdaltab->GetColumnCount())
    {
        err = band->SetDefaultRAT(gdaltab);
        if (err == CE_Failure)
        {
            if (bClose) this->CloseDataset();
            delete gdaltab;
            NMLogError(<< "Failed writing table to band!");
        }
        m_Dataset->FlushCache();
    }

    // if we don't close the data set here, the RAT is not written properly to disk
    // (not quite sure why that's not done when m_Dataset runs out of scope(?)
    if (bClose) this->CloseDataset();

    // need an open data set for writing the actual image later on;
    // when we're only updating the RAT, the data sets gets closed as soon as
    // the data set run's out of scope
    //m_Dataset = GDALDriverManagerWrapper::GetInstance().Update(this->GetFileName());

    delete gdaltab;
}

void
GDALRATImageIO::InternalWriteSQLiteRAT(AttributeTable::Pointer intab, unsigned int iBand)
{
    // if m_Dataset hasn't been instantiated before, we do it here, because
    // we just do an independent write of the RAT into the data set
    // (i.e. outside any pipeline activities ...)
    bool bCloseDataSet = false;
    if (m_Dataset == 0)
    {
        m_Dataset = (GDALDataset*)GDALOpen(this->GetFileName(), GA_Update);
        if (m_Dataset == 0)
        {
            std::cout << "Sorry, couldn't open raster layer for RAT update!" << std::endl;
            return;
        }
        bCloseDataSet = true;
    }

    // how many bands? (band index is 1-based)
    if (m_Dataset->GetRasterCount() < iBand)
    {
        if (bCloseDataSet) this->CloseDataset();
        return;
    }

    // fetch the band
    GDALRasterBand* band = m_Dataset->GetRasterBand(iBand);

    // create a new raster attribute table
    // note: we just create a whole new table and replace the
    // current one; we don't bother with just writing changed
    // values (too lazy for doing the required housekeeping
    // beforehand) ...
#ifdef GDAL_NEWRATAPI
    GDALDefaultRasterAttributeTable* gdaltab = new GDALDefaultRasterAttributeTable();
#else
    GDALRasterAttributeTable* gdaltab = new GDALRasterAttributeTable();
#endif


    otb::SQLiteTable::Pointer tab = static_cast<otb::SQLiteTable*>(intab.GetPointer());
    int rowcount = tab->GetNumRows();
    bool bCloseConnection = false;
    bool bOpenReadOnlyFlag = tab->GetOpenReadOnlyFlag();
    bool bOpenSharedCache = tab->GetUseSharedCache();
    if (rowcount == 0)
    {
        // it could be that the RAT connection has been closed and just
        // needs reopening ...
        if (    tab->GetDbConnection() == 0
            &&  !tab->GetTableName().empty()
            &&  !tab->GetDbFileName().empty())
        {
            tab->SetOpenReadOnly(true);
            tab->SetUseSharedCache(false);
            if (tab->openConnection())
            {
                bCloseConnection = true;
                if (!tab->PopulateTableAdmin())
                {
                    tab->SetOpenReadOnly(bOpenReadOnlyFlag);
                    tab->SetUseSharedCache(bOpenSharedCache);
                    tab->CloseTable();
                    if (bCloseDataSet) this->CloseDataset();
                    delete gdaltab;
                    gdaltab = 0;
                    return;
                }
            }
            else
            {
                tab->SetOpenReadOnly(bOpenReadOnlyFlag);
                tab->SetUseSharedCache(bOpenSharedCache);
                if (bCloseDataSet) this->CloseDataset();
                delete gdaltab;
                gdaltab = 0;
                return;
            }
            rowcount = tab->GetNumRows();
        }
    }

    // if we've got an IMAGINE file with UCHAR data type, we create
    // a table with at least 256 rows, otherwise ERDAS Imagine wouldn't like it
    GDALDriverH driver = GDALGetDatasetDriver(m_Dataset);
    std::string dsn = GDALGetDriverShortName(driver);
    if (dsn.compare("HFA") == 0 && this->m_ComponentType == otb::ImageIOBase::UCHAR)
    {
        rowcount = rowcount < 256 ? 256 : rowcount;
    }

    gdaltab->SetRowCount(rowcount);
    std::vector<std::string> colnames;

    CPLErr err;

    // add all columns to the table
    for (int col = 0; col < tab->GetNumCols(); ++col)
    {
        GDALRATFieldType type;
        switch(tab->GetColumnType(col))
        {
        case otb::AttributeTable::ATTYPE_INT:
            type = GFT_Integer;
            break;
        case otb::AttributeTable::ATTYPE_DOUBLE:
            type = GFT_Real;
            break;
        case otb::AttributeTable::ATTYPE_STRING:
            type = GFT_String;
            break;
        default:
            {
                NMLogError(<< "Type of column #" << col
                    << " '" << tab->GetColumnName(col).c_str() << "' is undefined!");
            }
        }

        GDALRATFieldUsage usage = GFU_Generic;
        err = gdaltab->CreateColumn(tab->GetColumnName(col).c_str(),
                            type, usage);
        if (err == CE_Failure)
        {
            NMLogError(<< "Failed creating column #" << col
                    << " '" << tab->GetColumnName(col).c_str() << "!");
        }

        colnames.push_back(tab->GetColumnName(col));

        NMLogDebug(<< "Created column #" << col << " '"
                << tab->GetColumnName(col).c_str() << "'");
    }

    tab->BeginTransaction();
    tab->PrepareBulkGet(colnames, "");

    std::vector< otb::AttributeTable::ColumnValue > values;
    values.resize(tab->GetNumCols());

    // copy values row by row
    for (long long row=0; row < tab->GetNumRows(); ++row)
    {
        if (!tab->DoBulkGet(values))
        {
            NMProcWarn(<< "Copying records failed at row idx=" << row
                            << " of 0-" << tab->GetNumRows()-1 << "!");
            break;
        }
        for (int col=0; col < tab->GetNumCols(); ++col)
        {
            switch(tab->GetColumnType(col))
            {
            case otb::AttributeTable::ATTYPE_INT:
                gdaltab->SetValue(row, col, static_cast<int>(values[col].ival));
                break;
            case otb::AttributeTable::ATTYPE_DOUBLE:
                gdaltab->SetValue(row, col, values[col].dval);
                break;
            case otb::AttributeTable::ATTYPE_STRING:
                if (values[col].tval == 0)
                {
                    const char nullval = '\0';
                    gdaltab->SetValue(row, col, &nullval);
                }
                else
                {
                    gdaltab->SetValue(row, col, values[col].tval);
                }
                break;
            default:
                delete gdaltab;
                gdaltab = nullptr;
                NMLogError(<< "Unrecognised field type! Couldn't set value col=" << col
                        << " row=" << row << " value=" << tab->GetStrValue(col, row).c_str());
                break;
            }
        }
    }
    tab->EndTransaction();

    // if we've just established the connection for writing the RAT
    // we leave if as we found it again ...
    tab->SetOpenReadOnly(bOpenReadOnlyFlag);
    tab->SetUseSharedCache(bOpenSharedCache);
    if (bCloseConnection) tab->CloseTable();

    // associate the table with the band
    // (only if there is actually a record in the table yet,
    // it could be we're in update mode and the table is only
    // being made available once all pixels have been processed
    // (e.g. SumZones))
    if (rowcount && gdaltab->GetColumnCount())
    {
        err = band->SetDefaultRAT(gdaltab);
        if (err == CE_Failure)
        {
            if (bCloseDataSet) this->CloseDataset();
            delete gdaltab;
            gdaltab = nullptr;
            NMLogError(<< "Failed writing table to band!");
        }
        else
        {
            m_Dataset->FlushCache();
        }
    }

    // if we don't close the data set here, the RAT is not written properly to disk
    // (not quite sure why that's not done when m_Dataset runs out of scope(?)
    //m_Dataset->CloseDataSet();
    if (bCloseDataSet) this->CloseDataset();

    // need an open data set for writing the actual image later on;
    // when we're only updating the RAT, the data sets gets closed as soon as
    // the data set run's out of scope
    //m_Dataset = GDALDriverManagerWrapper::GetInstance().Update(this->GetFileName());

    delete gdaltab;
}


} // end namespace otb
