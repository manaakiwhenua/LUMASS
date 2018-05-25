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
 * NMImageReader.cxx
 *
 *  Created on: 25/11/2010
 *      Author: alex
 */
#include "nmlog.h"
#define ctxNMImageReader "NMImageReader"

#include <limits>
#include <QFileInfo>

#include "NMImageReader.h"
#include "otbGDALRATImageIO.h"
//#include "otbGDALRATImageFileReader.h"
#include "otbImageFileReader.h"
#include "otbImageIOFactory.h"
#include "itkIndent.h"
#include "otbAttributeTable.h"
#include "otbVectorImage.h"
#include "otbImage.h"
#include "itkImageBase.h"
#include "otbImageIOBase.h"
#include "NMMfwException.h"
#include "NMIterableComponent.h"
#include "NMModelController.h"
#include "itkRGBPixel.h"
#include "otbStreamingStatisticsImageFilter.h"
#include "otbStreamingImageVirtualWriter.h"

#include "otbNMImageReader.h"

#ifdef BUILD_RASSUPPORT
  #include "otbRasdamanImageIO.h"
  //#include "otbRasdamanImageReader.h"

	/**
	 *  Helper class which instantiates the first part of the (ITK) image
	 *  pipeline using the appropriate PixelType
	 */
	template <class PixelType, unsigned int ImageDimension>
	class RasdamanReader
	{
	public:
		typedef otb::Image< PixelType, ImageDimension > ImgType;
        typedef otb::NMImageReader< ImgType > 	ReaderType;
		typedef typename ReaderType::Pointer			ReaderTypePointer;
		typedef typename ImgType::PointType				ImgOriginType;
		typedef typename ImgType::SpacingType			ImgSpacingType;

        typedef itk::RGBPixel< PixelType >                  RGBPixelType;
        typedef otb::Image< RGBPixelType, ImageDimension >  RGBImageType;
        typedef otb::NMImageReader< RGBImageType >    RGBReaderType;
        typedef typename RGBReaderType::Pointer             RGBReaderTypePointer;
        typedef typename RGBReaderType::ImageRegionType     RGBReaderRegionType;


        typedef otb::VectorImage< PixelType, ImageDimension > 		VecImgType;
        typedef otb::NMImageReader< VecImgType > 				VecReaderType;
		typedef typename VecReaderType::Pointer						VecReaderTypePointer;

        typedef typename otb::StreamingStatisticsImageFilter<ImgType>    StatsFilterType;
        typedef typename otb::StreamingImageVirtualWriter<ImgType>       VirtWriterType;


        static void getImageStatistics(itk::ProcessObject* procObj, unsigned int numBands,
                                  std::vector<double>& stats, const int* index, const int* size)
        {
            stats.clear();

            if (numBands == 1)
            {
                ReaderType *r = dynamic_cast<ReaderType*>(procObj);

                if (index != nullptr && size != nullptr)
                {
                    typename ReaderType::ImageRegionType::IndexType idx;
                    typename ReaderType::ImageRegionType::SizeType sz;
                    typename ReaderType::ImageRegionType reg;

                    for (int d=0; d < ImgType::ImageDimension; ++d)
                    {
                        idx[d] = index[d];
                        sz[d] = size[d];
                    }
                    reg.SetIndex(idx);
                    reg.SetSize(sz);
                    r->SetUserLargestPossibleRegion(reg);
                    r->SetUseUserLargestPossibleRegion(true);
                }

                typename StatsFilterType::Pointer f = StatsFilterType::New();
                typename VirtWriterType::Pointer w = VirtWriterType::New();

                w->SetAutomaticStrippedStreaming(512);

                unsigned int nth = f->GetNumberOfThreads();
                nth = nth > 0 ? nth : 1;

                f->SetInput(r->GetOutput());
                f->SetNumberOfThreads(nth);
                w->SetInput(f->GetOutput());
                w->Update();

                stats.push_back(f->GetMinimum());
                stats.push_back(f->GetMaximum());
                stats.push_back(f->GetMean());
                stats.push_back(-9999);
                stats.push_back(f->GetSigma());
                stats.push_back(r->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels());
                stats.push_back(-9999);
            }
            else
            {
                stats.resize(7, -9999);
                return;
            }
        }

        static otb::AttributeTable::Pointer
            fetchRAT(itk::ProcessObject* procObj, int band,
                    unsigned int numBands, bool rgbMode)
        {
            if (numBands == 1)
            {
                ReaderType *r = dynamic_cast<ReaderType*>(procObj);
                //return r->getRasterAttributeTable(band);
                return r->GetAttributeTable(band);
            }
            else if (rgbMode && numBands == 3)
            {
                RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj);
                //return r->getRasterAttributeTable(band);
                return r->GetAttributeTable(band);
            }
            else
            {
                VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
                //return r->getRasterAttributeTable(band);
                return r->GetAttributeTable(band);
            }
        }

        //        static void setOverviewIdx(itk::ProcessObject::Pointer& procObj,
        //                                     unsigned int numBands, int ovvidx, int* userLPR,
        //                                   bool rgbMode)
        //        {
        //            if (numBands == 1)
        //            {
        //                ReaderType *r = dynamic_cast<ReaderType*>(procObj.GetPointer());
        //                r->SetOverviewIdx(ovvidx);
        //                if (userLPR != 0)
        //                {
        //                    ReaderRegionType lpr;
        //                    for (int d=0; d < ImageDimension; ++d)
        //                    {
        //                        lpr.SetIndex(d, userLPR[d*2]);
        //                        lpr.SetSize(d, userLPR[d*2+1]);
        //                    }
        //                    r->UseUserLargestPossibleRegionOn();
        //                    r->SetUserLargestPossibleRegion(lpr);
        //                }
        //                else
        //                {
        //                    r->UseUserLargestPossibleRegionOff();
        //                }
        //                r->UpdateOutputInformation();
        //            }
        //            else if (rgbMode && numBands == 3)
        //            {
        //                RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj.GetPointer());
        //                r->SetOverviewIdx(ovvidx);
        //                if (userLPR != 0)
        //                {
        //                    RGBReaderRegionType lpr;
        //                    for (int d=0; d < ImageDimension; ++d)
        //                    {
        //                        lpr.SetIndex(d, userLPR[d*2]);
        //                        lpr.SetSize(d, userLPR[d*2+1]);
        //                    }
        //                    r->UseUserLargestPossibleRegionOn();
        //                    r->SetUserLargestPossibleRegion(lpr);
        //                }
        //                else
        //                {
        //                    r->UseUserLargestPossibleRegionOff();
        //                }
        //                r->UpdateOutputInformation();
        //            }
        //            else
        //            {
        //                VecReaderType *r = dynamic_cast<VecReaderType*>(procObj.GetPointer());
        //                r->SetOverviewIdx(ovvidx);
        //                if (userLPR != 0)
        //                {
        //                    VecReaderRegionType lpr;
        //                    for (int d=0; d < ImageDimension; ++d)
        //                    {
        //                        lpr.SetIndex(d, userLPR[d*2]);
        //                        lpr.SetSize(d, userLPR[d*2+1]);
        //                    }
        //                    r->UseUserLargestPossibleRegionOn();
        //                    r->SetUserLargestPossibleRegion(lpr);
        //                }
        //                else
        //                {
        //                    r->UseUserLargestPossibleRegionOff();
        //                }
        //                r->UpdateOutputInformation();
        //            }
        //        }

        static itk::DataObject *getOutput(itk::ProcessObject::Pointer &readerProcObj,
                unsigned int numBands, unsigned int idx, bool rgbMode)
        {
            itk::DataObject *img = 0;
            if (numBands == 1)
            {
                ReaderType *r = dynamic_cast<ReaderType*>(readerProcObj.GetPointer());
                if (idx == 0)
                    img = r->GetOutput(idx);
                else if (idx == 1)
                    //img = dynamic_cast<itk::DataObject*>(r->getRasterAttributeTable(1).GetPointer());
                    img = dynamic_cast<itk::DataObject*>(r->GetAttributeTable(1).GetPointer());
            }
            else if (rgbMode && numBands == 3)
            {
                RGBReaderType *r = dynamic_cast<RGBReaderType*>(readerProcObj.GetPointer());
                if (idx == 0)
                    img = r->GetOutput(idx);
                else if (idx == 1)
                    //img = dynamic_cast<itk::DataObject*>(r->getRasterAttributeTable(1).GetPointer());
                    img = dynamic_cast<itk::DataObject*>(r->GetAttributeTable(1).GetPointer());
            }
            else
            {
                VecReaderType *vr = dynamic_cast<VecReaderType*>(readerProcObj.GetPointer());
                if (idx == 0)
                    img = vr->GetOutput(idx);
                else if (idx == 1)
                    //img = dynamic_cast<itk::DataObject*>(vr->getRasterAttributeTable(1).GetPointer());
                    img = dynamic_cast<itk::DataObject*>(vr->GetAttributeTable(1).GetPointer());
            }
            return img;
        }

        static void initReader(itk::ProcessObject::Pointer &readerProcObj,
                otb::ImageIOBase *imgIOBase, QString &imgName,
                unsigned int numBands, bool rgbMode)
        {
            if (numBands == 1)
            {
                ReaderTypePointer reader = ReaderType::New();
                otb::RasdamanImageIO* rio = dynamic_cast<otb::RasdamanImageIO*>(imgIOBase);
                reader->SetImageIO(rio);
                reader->SetFileName(imgName.toStdString().c_str());

                // keep references to the exporter and the reader
                // (we've already got a reference to the importer)
                readerProcObj = reader;
            }
            else if (rgbMode && numBands == 3)
            {
                RGBReaderTypePointer reader = RGBReaderType::New();
                otb::RasdamanImageIO* rio = dynamic_cast<otb::RasdamanImageIO*>(imgIOBase);
                reader->SetImageIO(rio);
                reader->SetFileName(imgName.toStdString().c_str());

                // keep references to the exporter and the reader
                // (we've already got a reference to the importer)
                readerProcObj = reader;
            }
            else
            {
                VecReaderTypePointer reader = VecReaderType::New();
                otb::RasdamanImageIO* rio = dynamic_cast<otb::RasdamanImageIO*>(imgIOBase);
                reader->SetImageIO(imgIOBase);
                reader->SetFileName(imgName.toStdString().c_str());

                // keep references to the exporter and the reader
                // (we've already got a reference to the importer)
                readerProcObj = reader;
            }
        }
    };
#endif // BUILD_RASSUPPORT 

template <class PixelType, unsigned int ImageDimension>
class FileReader
{
public:
	typedef otb::Image< PixelType, ImageDimension > 	ImgType;
    //typedef otb::GDALRATImageFileReader< ImgType > 		ReaderType;
    typedef otb::NMImageReader< ImgType > 		ReaderType;
	typedef typename ReaderType::Pointer				ReaderTypePointer;
    typedef typename ReaderType::ImageRegionType        ReaderRegionType;

    typedef itk::RGBPixel< PixelType >                  RGBPixelType;
    typedef otb::Image< RGBPixelType, ImageDimension >  RGBImageType;
    //typedef otb::GDALRATImageFileReader< RGBImageType > RGBReaderType;
    typedef otb::NMImageReader< RGBImageType > RGBReaderType;
    typedef typename RGBReaderType::Pointer             RGBReaderTypePointer;
    typedef typename RGBReaderType::ImageRegionType     RGBReaderRegionType;

	typedef otb::VectorImage< PixelType, ImageDimension > VecImgType;
    //typedef otb::GDALRATImageFileReader< VecImgType > 	  VecReaderType;
    typedef otb::NMImageReader< VecImgType > 	  VecReaderType;
	typedef typename VecReaderType::Pointer				  VecReaderTypePointer;
    typedef typename VecReaderType::ImageRegionType       VecReaderRegionType;

    typedef typename otb::PersistentStatisticsImageFilter<ImgType>    StatsFilterType;
    typedef typename otb::StreamingImageVirtualWriter<ImgType>       VirtWriterType;


    static void getImageStatistics(itk::ProcessObject* procObj, unsigned int numBands,
                              std::vector<double>& stats, const int* index, const int* size)
    {
        stats.clear();

        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj);
            if (index != nullptr && size != nullptr)
            {
                typename ReaderType::ImageRegionType::IndexType idx;
                typename ReaderType::ImageRegionType::SizeType sz;
                typename ReaderType::ImageRegionType reg;

                for (int d=0; d < ImgType::ImageDimension; ++d)
                {
                    idx[d] = index[d];
                    sz[d] = size[d];
                }
                reg.SetIndex(idx);
                reg.SetSize(sz);
                r->UseUserLargestPossibleRegionOn();
                r->SetUserLargestPossibleRegion(reg);
            }

            typename StatsFilterType::Pointer f = StatsFilterType::New();
            typename VirtWriterType::Pointer w = VirtWriterType::New();

            w->SetAutomaticStrippedStreaming(512);

            unsigned int nth = f->GetNumberOfThreads();
            nth = nth > 0 ? nth : 1;

            f->SetInput(r->GetOutput());
            f->SetNumberOfThreads(nth);
            w->SetInput(f->GetOutput());

            f->Reset();
            w->Update();
            f->Synthetize();

            stats.push_back(f->GetMinimum());
            stats.push_back(f->GetMaximum());
            stats.push_back(f->GetMean());
            stats.push_back(-9999);
            stats.push_back(f->GetSigma());
            stats.push_back(r->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels());
            stats.push_back(-9999);
        }
        else
        {
            stats.resize(7, -9999);
            return;
        }
    }


	static otb::AttributeTable::Pointer
		fetchRAT(itk::ProcessObject* procObj, int band,
                unsigned int numBands, bool rgbMode)
	{
		if (numBands == 1)
		{
			ReaderType *r = dynamic_cast<ReaderType*>(procObj);
			return r->GetAttributeTable(band);
		}
        else if (rgbMode && numBands == 3)
        {
            RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj);
            return r->GetAttributeTable(band);
        }
		else
		{
			VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
			return r->GetAttributeTable(band);
		}
	}

    static void
        setRATType(itk::ProcessObject* procObj, unsigned int numBands,
                   otb::AttributeTable::TableType ttype, bool rgbMode)
    {
        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj);
            r->SetRATType(ttype);
        }
        else if (rgbMode && numBands == 3)
        {
            RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj);
            r->SetRATType(ttype);
        }
        else
        {
            VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
            r->SetRATType(ttype);
        }
    }

    static void
        buildOverviews(itk::ProcessObject* procObj, unsigned int numBands,
                   const std::string& resamplingType, bool rgbMode)
    {
        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj);
            r->BuildOverviews(resamplingType);
        }
        else if (rgbMode && numBands == 3)
        {
            RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj);
            r->BuildOverviews(resamplingType);
        }
        else
        {
            VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
            r->BuildOverviews(resamplingType);
        }
    }


    static void setOverviewIdx(itk::ProcessObject::Pointer& procObj,
                                 unsigned int numBands, int ovvidx, int* userLPR,
                               bool rgbMode)
    {
        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj.GetPointer());
            r->SetOverviewIdx(ovvidx);
            if (userLPR != 0)
            {
                ReaderRegionType lpr;
                for (int d=0; d < ImageDimension; ++d)
                {
                    lpr.SetIndex(d, userLPR[d*2]);
                    lpr.SetSize(d, userLPR[d*2+1]);
                }
                r->UseUserLargestPossibleRegionOn();
                r->SetUserLargestPossibleRegion(lpr);
            }
            else
            {
                r->UseUserLargestPossibleRegionOff();
            }
            r->UpdateOutputInformation();
        }
        else if (rgbMode && numBands == 3)
        {
            RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj.GetPointer());
            r->SetOverviewIdx(ovvidx);
            if (userLPR != 0)
            {
                RGBReaderRegionType lpr;
                for (int d=0; d < ImageDimension; ++d)
                {
                    lpr.SetIndex(d, userLPR[d*2]);
                    lpr.SetSize(d, userLPR[d*2+1]);
                }
                r->UseUserLargestPossibleRegionOn();
                r->SetUserLargestPossibleRegion(lpr);
            }
            else
            {
                r->UseUserLargestPossibleRegionOff();
            }
            r->UpdateOutputInformation();
        }
        else
        {
            VecReaderType *r = dynamic_cast<VecReaderType*>(procObj.GetPointer());
            r->SetOverviewIdx(ovvidx);
            if (userLPR != 0)
            {
                VecReaderRegionType lpr;
                for (int d=0; d < ImageDimension; ++d)
                {
                    lpr.SetIndex(d, userLPR[d*2]);
                    lpr.SetSize(d, userLPR[d*2+1]);
                }
                r->UseUserLargestPossibleRegionOn();
                r->SetUserLargestPossibleRegion(lpr);
            }
            else
            {
                r->UseUserLargestPossibleRegionOff();
            }
            r->UpdateOutputInformation();
        }
    }

	static itk::DataObject *getOutput(itk::ProcessObject::Pointer &readerProcObj,
            unsigned int numBands, unsigned int idx, bool rgbMode)
	{
		itk::DataObject *img = 0;
		if (numBands == 1)
		{
			ReaderType *r = dynamic_cast<ReaderType*>(readerProcObj.GetPointer());
			if (idx == 0)
				img = r->GetOutput(idx);
			else if (idx == 1)
				img = dynamic_cast<itk::DataObject*>(r->GetAttributeTable(1).GetPointer());
		}
        else if (rgbMode && numBands == 3)
        {
            RGBReaderType *r = dynamic_cast<RGBReaderType*>(readerProcObj.GetPointer());
            if (idx == 0)
                img = r->GetOutput(idx);
            else if (idx == 1)
                img = dynamic_cast<itk::DataObject*>(r->GetAttributeTable(1).GetPointer());
        }
		else
		{
			VecReaderType *vr = dynamic_cast<VecReaderType*>(readerProcObj.GetPointer());
			if (idx == 0)
				img = vr->GetOutput(idx);
			else if (idx == 1)
				img = dynamic_cast<itk::DataObject*>(vr->GetAttributeTable(1).GetPointer());
		}
		return img;
	}

	static void initReader(itk::ProcessObject::Pointer &readerProcObj,
			otb::ImageIOBase *imgIOBase, QString &imgName,
            unsigned int numBands, bool rgbMode)
	{
		NMDebugCtx("FileReader", << "...");

		if (numBands == 1)
		{
			ReaderTypePointer reader = ReaderType::New();
			otb::GDALRATImageIO *gio = dynamic_cast<otb::GDALRATImageIO*>(imgIOBase);
			reader->SetImageIO(imgIOBase);
			reader->SetFileName(imgName.toStdString().c_str());

			// keep references to the exporter and the reader
			// (we've already got a reference to the importer)
			readerProcObj = reader;
		}
        else if (rgbMode && numBands == 3)
        {
            RGBReaderTypePointer reader = RGBReaderType::New();
            otb::GDALRATImageIO *gio = dynamic_cast<otb::GDALRATImageIO*>(imgIOBase);
            reader->SetImageIO(imgIOBase);
            reader->SetFileName(imgName.toStdString().c_str());

            // keep references to the exporter and the reader
            // (we've already got a reference to the importer)
            readerProcObj = reader;
        }
		else
		{
			VecReaderTypePointer reader = VecReaderType::New();
			otb::GDALRATImageIO *gio = dynamic_cast<otb::GDALRATImageIO*>(imgIOBase);
			reader->SetImageIO(imgIOBase);
			reader->SetFileName(imgName.toStdString().c_str());

			// keep references to the exporter and the reader
			// (we've already got a reference to the importer)
			readerProcObj = reader;
		}

		NMDebugCtx("FileReader", << "done!");
	}
};

template class FileReader<unsigned char, 1>;
template class FileReader<char, 1>;
template class FileReader<unsigned short, 1>;
template class FileReader<short, 1>;
template class FileReader<unsigned int, 1>;
template class FileReader<int, 1>;
template class FileReader<unsigned long, 1>;
template class FileReader<long, 1>;
template class FileReader<float, 1>;
template class FileReader<double, 1>;
template class FileReader<unsigned char, 2>;
template class FileReader<char, 2>;
template class FileReader<unsigned short, 2>;
template class FileReader<short, 2>;
template class FileReader<unsigned int, 2>;
template class FileReader<int, 2>;
template class FileReader<unsigned long, 2>;
template class FileReader<long, 2>;
template class FileReader<float, 2>;
template class FileReader<double, 2>;
template class FileReader<unsigned char, 3>;
template class FileReader<char, 3>;
template class FileReader<unsigned short, 3>;
template class FileReader<short, 3>;
template class FileReader<unsigned int, 3>;
template class FileReader<int, 3>;
template class FileReader<unsigned long, 3>;
template class FileReader<long, 3>;
template class FileReader<float, 3>;
template class FileReader<double, 3>;

#ifdef BUILD_RASSUPPORT
template class RasdamanReader<unsigned char, 1>;
template class RasdamanReader<char, 1>;
template class RasdamanReader<unsigned short, 1>;
template class RasdamanReader<short, 1>;
template class RasdamanReader<unsigned int, 1>;
template class RasdamanReader<int, 1>;
template class RasdamanReader<unsigned long, 1>;
template class RasdamanReader<long, 1>;
template class RasdamanReader<float, 1>;
template class RasdamanReader<double, 1>;
template class RasdamanReader<unsigned char, 2>;
template class RasdamanReader<char, 2>;
template class RasdamanReader<unsigned short, 2>;
template class RasdamanReader<short, 2>;
template class RasdamanReader<unsigned int, 2>;
template class RasdamanReader<int, 2>;
template class RasdamanReader<unsigned long, 2>;
template class RasdamanReader<long, 2>;
template class RasdamanReader<float, 2>;
template class RasdamanReader<double, 2>;
template class RasdamanReader<unsigned char, 3>;
template class RasdamanReader<char, 3>;
template class RasdamanReader<unsigned short, 3>;
template class RasdamanReader<short, 3>;
template class RasdamanReader<unsigned int, 3>;
template class RasdamanReader<int, 3>;
template class RasdamanReader<unsigned long, 3>;
template class RasdamanReader<long, 3>;
template class RasdamanReader<float, 3>;
template class RasdamanReader<double, 3>;
#endif


#ifdef BUILD_RASSUPPORT
  /** Helper Macro to call either the Rasdaman or File Reader methods */

    #define CallGetImgStatsMacro( PixelType ) \
    { \
      if (mbRasMode) \
      { \
          switch (this->mOutputNumDimensions) \
          { \
          case 1: \
              RasdamanReader< PixelType, 1 >::getImageStatistics( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, stats, index, size); \
              break; \
          case 3: \
              RasdamanReader< PixelType, 3 >::getImageStatistics( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, stats, index, size); \
              break; \
          default: \
              RasdamanReader< PixelType, 2 >::getImageStatistics( \
                  this->mOtbProcess, \
                  this->mOutputNumBands, stats, index, size); \
          }\
      } \
      else \
      { \
          switch (this->mOutputNumDimensions) \
          { \
          case 1: \
              FileReader< PixelType, 1 >::getImageStatistics( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, stats, index, size); \
              break; \
          case 3: \
              FileReader< PixelType, 3 >::getImageStatistics( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, stats, index, size); \
              break; \
          default: \
              FileReader< PixelType, 2 >::getImageStatistics( \
                  this->mOtbProcess, \
                  this->mOutputNumBands, stats, index, size); \
          }\
      } \
    }


  #define CallReaderMacro( PixelType ) \
  { \
  	if (mbRasMode) \
  	{ \
  		switch (this->mOutputNumDimensions) \
  		{ \
  		case 1: \
  			RasdamanReader< PixelType, 1 >::initReader( \
  					this->mOtbProcess, \
  					this->mItkImgIOBase, \
  					this->mFileName, \
                    this->mOutputNumBands, mRGBMode); \
  			break; \
  		case 3: \
  			RasdamanReader< PixelType, 3 >::initReader( \
  					this->mOtbProcess, \
  					this->mItkImgIOBase, \
  					this->mFileName, \
                    this->mOutputNumBands, mRGBMode); \
  			break; \
  		default: \
  			RasdamanReader< PixelType, 2 >::initReader( \
  				this->mOtbProcess, \
  				this->mItkImgIOBase, \
  				this->mFileName, \
                this->mOutputNumBands, mRGBMode); \
  		}\
  	} \
  	else \
  	{ \
  		switch (this->mOutputNumDimensions) \
  		{ \
  		case 1: \
  			FileReader< PixelType, 1 >::initReader( \
  					this->mOtbProcess, \
  					this->mItkImgIOBase, \
  					this->mFileName, \
                    this->mOutputNumBands, mRGBMode); \
  			break; \
  		case 3: \
  			FileReader< PixelType, 3 >::initReader( \
  					this->mOtbProcess, \
  					this->mItkImgIOBase, \
  					this->mFileName, \
                    this->mOutputNumBands, mRGBMode); \
  			break; \
  		default: \
  			FileReader< PixelType, 2 >::initReader( \
  				this->mOtbProcess, \
  				this->mItkImgIOBase, \
  				this->mFileName, \
                this->mOutputNumBands, mRGBMode); \
  		}\
  	} \
  }  



	/** Macro for getting the reader's output */
	#define RequestReaderOutput( PixelType ) \
	{ \
		if (this->mbRasMode) \
		{ \
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				img = RasdamanReader<PixelType, 1 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
				break; \
			case 3: \
				img = RasdamanReader<PixelType, 3 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
				break; \
			default: \
				img = RasdamanReader<PixelType, 2 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
			}\
		} \
		else \
		{ \
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				img = FileReader<PixelType, 1 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
				break; \
			case 3: \
				img = FileReader<PixelType, 3 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
				break; \
			default: \
				img = FileReader<PixelType, 2 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
			}\
		} \
	}

    #define SetReaderOverviewIdx( PixelType ) \
    { \
        if (this->mbRasMode) \
        { \
            ;\
        } \
        else \
        { \
            switch (this->mOutputNumDimensions) \
            { \
            case 1: \
                FileReader<PixelType, 1 >::setOverviewIdx( \
                        this->mOtbProcess, this->mOutputNumBands, ovvidx, userLPR, mRGBMode); \
                break; \
            case 3: \
                FileReader<PixelType, 3 >::setOverviewIdx( \
                        this->mOtbProcess, this->mOutputNumBands, ovvidx, userLPR, mRGBMode); \
                break; \
            default: \
                FileReader<PixelType, 2 >::setOverviewIdx( \
                        this->mOtbProcess, this->mOutputNumBands, ovvidx, userLPR, mRGBMode); \
            }\
        } \
    }

	/* Helper macro for calling the right class to fetch the attribute table
	 * from the reader
	 */
	#define CallFetchRATClassMacro( PixelType ) \
	{ \
		if (mbRasMode) \
		{ \
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				rat = RasdamanReader< PixelType, 1 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
				break; \
			case 3: \
				rat = RasdamanReader< PixelType, 3 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
				break; \
			default: \
				rat = RasdamanReader< PixelType, 2 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
			}\
		}\
		else \
			{ \
    © imago

    vorheriges Bild
    näc
    © imago

    vorheriges Bild
    näc
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				rat = FileReader< PixelType, 1 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
				break; \
			case 3: \
				rat = FileReader< PixelType, 3 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
				break; \
			default: \
				rat = FileReader< PixelType, 2 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
			}\
		} \
	}
#else

    #define CallGetImgStatsMacro( PixelType ) \
    { \
      { \
          switch (this->mOutputNumDimensions) \
          { \
          case 1: \
              FileReader< PixelType, 1 >::getImageStatistics( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, stats, index, size); \
              break; \
          case 3: \
              FileReader< PixelType, 3 >::getImageStatistics( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, stats, index, size); \
              break; \
          default: \
              FileReader< PixelType, 2 >::getImageStatistics( \
                  this->mOtbProcess, \
                  this->mOutputNumBands, stats, index, size); \
          }\
      } \
    }


    #define CallReaderMacro( PixelType ) \
	{ \
		{ \
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				FileReader< PixelType, 1 >::initReader( \
						this->mOtbProcess, \
						this->mItkImgIOBase, \
						this->mFileName, \
                        this->mOutputNumBands, mRGBMode); \
				break; \
			case 3: \
				FileReader< PixelType, 3 >::initReader( \
						this->mOtbProcess, \
						this->mItkImgIOBase, \
						this->mFileName, \
                        this->mOutputNumBands, mRGBMode); \
				break; \
			default: \
				FileReader< PixelType, 2 >::initReader( \
					this->mOtbProcess, \
					this->mItkImgIOBase, \
					this->mFileName, \
                    this->mOutputNumBands, mRGBMode); \
			}\
		} \
	}

	/** Macro for getting the reader's output */
	#define RequestReaderOutput( PixelType ) \
	{ \
		{ \
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				img = FileReader<PixelType, 1 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
				break; \
			case 3: \
				img = FileReader<PixelType, 3 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
				break; \
			default: \
				img = FileReader<PixelType, 2 >::getOutput( \
                        this->mOtbProcess, this->mOutputNumBands, idx, mRGBMode); \
			}\
		} \
	}

    /*! Macro for setting the OverviewIdx factor of the GDALRATImageFileReader
     */
    #define SetReaderOverviewIdx( PixelType ) \
    {\
        switch (this->mOutputNumDimensions) \
        { \
        case 1: \
            FileReader<PixelType, 1 >::setOverviewIdx( \
                    this->mOtbProcess, this->mOutputNumBands, ovvidx, userLPR, mRGBMode); \
            break; \
        case 3: \
            FileReader<PixelType, 3 >::setOverviewIdx( \
                    this->mOtbProcess, this->mOutputNumBands, ovvidx, userLPR, mRGBMode); \
            break; \
        default: \
            FileReader<PixelType, 2 >::setOverviewIdx( \
                    this->mOtbProcess, this->mOutputNumBands, ovvidx, userLPR, mRGBMode); \
        }\
     }

    /*! Macro for setting the RAT type to be read upon fetch
     */
    #define SetHelperRATType( PixelType ) \
    {\
        switch (this->mOutputNumDimensions) \
        { \
        case 1: \
            FileReader<PixelType, 1 >::setRATType( \
                    this->mOtbProcess, this->mOutputNumBands, ttype, mRGBMode); \
            break; \
        case 3: \
            FileReader<PixelType, 3 >::setRATType( \
                    this->mOtbProcess, this->mOutputNumBands, ttype, mRGBMode); \
            break; \
        default: \
            FileReader<PixelType, 2 >::setRATType( \
                    this->mOtbProcess, this->mOutputNumBands, ttype, mRGBMode); \
        }\
     }


    /*! Macro for setting the RAT type to be read upon fetch
     */
    #define CallBuildOverviews( PixelType ) \
    {\
        switch (this->mOutputNumDimensions) \
        { \
        case 1: \
            FileReader<PixelType, 1 >::buildOverviews( \
                    this->mOtbProcess, this->mOutputNumBands, resamplingType, mRGBMode); \
            break; \
        case 3: \
            FileReader<PixelType, 3 >::buildOverviews( \
                    this->mOtbProcess, this->mOutputNumBands, resamplingType, mRGBMode); \
            break; \
        default: \
            FileReader<PixelType, 2 >::buildOverviews( \
                    this->mOtbProcess, this->mOutputNumBands, resamplingType, mRGBMode); \
        }\
     }


	/* Helper macro for calling the right class to fetch the attribute table
	 * from the reader
	 */
	#define CallFetchRATClassMacro( PixelType ) \
	{ \
		{ \
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				rat = FileReader< PixelType, 1 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
				break; \
			case 3: \
				rat = FileReader< PixelType, 3 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
				break; \
			default: \
				rat = FileReader< PixelType, 2 >::fetchRAT( \
                        this->mOtbProcess.GetPointer(), band, this->mOutputNumBands, mRGBMode ); \
			}\
		} \
	}
#endif // BUILD_RASSUPPORT

/* ================================================================================== */

NMImageReader::NMImageReader(QObject * parent)
{
	this->setParent(parent);
	this->setObjectName(tr("NMImageReader"));
	this->mbIsInitialised = false;
	this->mOutputNumBands = 1;
	this->mInputNumBands = 1;
	this->mOutputNumDimensions = 2;
	this->mInputNumDimensions = 2;
	this->mInputComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mOutputComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mFileName = "";
    this->mBandList.clear();
	this->mbRasMode = false;
    this->mRGBMode = false;
	this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mRATType = QString("ATTABLE_TYPE_RAM");
    this->mRATEnum << "ATTABLE_TYPE_RAM" << "ATTABLE_TYPE_SQLITE";
#ifdef BUILD_RASSUPPORT	
	this->mRasconn = 0;
	this->mRasConnector = 0;
	this->mbLinked = false;
#endif	
	
}

NMImageReader::~NMImageReader()
{
}

#ifdef BUILD_RASSUPPORT
void NMImageReader::setRasdamanConnector(RasdamanConnector * rasconn)
{
	this->mRasconn = rasconn;
	if (rasconn != 0)
		this->mbRasMode = true;
	else
		this->mbRasMode = false;
	NMDebugAI(<< "rasmode is: " << this->mbRasMode << endl);
}
#endif

otb::AttributeTable::Pointer
NMImageReader::getRasterAttributeTable(int band)
{
	if (!this->mbIsInitialised || band < 1)
		return 0;

	otb::AttributeTable::Pointer rat;

	switch (this->mOutputComponentType)
	{
	case otb::ImageIOBase::UCHAR:
		CallFetchRATClassMacro( unsigned char );
		break;
	case otb::ImageIOBase::CHAR:
		CallFetchRATClassMacro( char );
		break;
	case otb::ImageIOBase::USHORT:
		CallFetchRATClassMacro( unsigned short );
		break;
	case otb::ImageIOBase::SHORT:
		CallFetchRATClassMacro( short );
		break;
	case otb::ImageIOBase::UINT:
		CallFetchRATClassMacro( unsigned int );
		break;
	case otb::ImageIOBase::INT:
		CallFetchRATClassMacro( int );
		break;
	case otb::ImageIOBase::ULONG:
		CallFetchRATClassMacro( unsigned long );
		break;
	case otb::ImageIOBase::LONG:
		CallFetchRATClassMacro( long );
		break;
	case otb::ImageIOBase::FLOAT:
		CallFetchRATClassMacro( float );
		break;
	case otb::ImageIOBase::DOUBLE:
		CallFetchRATClassMacro( double );
		break;
	default:
        NMLogError(<<  ctxNMImageReader << ": UNKNOWN DATA TYPE, couldn't fetch RAT!");
		break;
	}

	return rat;
}

void NMImageReader::setFileName(QString filename)
{
	this->mFileName = filename;
}

QString NMImageReader::getFileName(void)
{
	return this->mFileName;
}

void
NMImageReader::setBandMap(std::vector<int> map)
{
    this->mBandMap = map;
    if (this->isInitialised())
    {
        if (!mbRasMode)
        {
            otb::GDALRATImageIO::Pointer gio = static_cast<otb::GDALRATImageIO*>(
                        this->mItkImgIOBase.GetPointer());
            gio->SetBandMap(mBandMap);
            this->mOtbProcess->Modified();
        }
    }
}

bool NMImageReader::initialise()
{
	NMDebugCtx(ctxNMImageReader, << "...");

	// refuse to work if we don't know yet the filename
	if (this->mFileName.isEmpty())
	{
		NMDebugCtx(ctxNMImageReader, << "done!");
		return false;
	}

	if (!mbRasMode)
	{
		NMDebugAI(<< "we're not in ras mode ..." << endl);
		otb::GDALRATImageIO::Pointer gio = otb::GDALRATImageIO::New();
		gio->SetRATSupport(true);
        if (mRATType.compare(QString("ATTABLE_TYPE_RAM")) == 0)
        {
            gio->SetRATType(otb::AttributeTable::ATTABLE_TYPE_RAM);
        }
        else
        {
            gio->SetRATType(otb::AttributeTable::ATTABLE_TYPE_SQLITE);
        }

        gio->SetRGBMode(mRGBMode);
        gio->SetBandMap(mBandMap);
		this->mItkImgIOBase = gio;
	}
#ifdef BUILD_RASSUPPORT
	else
	{
		NMDebugAI(<< "rasmode!" << endl);
		otb::RasdamanImageIO::Pointer rio = otb::RasdamanImageIO::New();

		try
		{
			rio->setRasdamanConnector(this->mRasconn);
			this->mItkImgIOBase = rio;
		}
		catch (r_Error& re)
		{
			NMDebugCtx(ctxNMImageReader, << "done!");
			throw(re);
		}
	}
#endif	

	if (!this->mItkImgIOBase)
	{
        NMLogError(<<  ctxNMImageReader << ": NO IMAGEIO WAS FOUND!");
		NMDebugCtx(ctxNMImageReader, << "done!");
		return false;
	}

	// Now that we found the appropriate ImageIO class, ask it to
	// read the meta data from the image file.
	this->mItkImgIOBase->SetFileName(this->mFileName.toStdString().c_str());
	if (!this->mItkImgIOBase->CanReadFile(this->mFileName.toStdString().c_str()))
	{
        NMLogError(<<  ctxNMImageReader << ": Failed reading '" << this->mFileName.toStdString() << "'!");
		NMDebugCtx(ctxNMImageReader, << "done!");
		return false;
	}

	NMDebugAI(<< "reading image information ..." << endl);
	this->mItkImgIOBase->ReadImageInformation();

    this->mOutputNumBands = this->mItkImgIOBase->GetNumberOfComponents();

	if (this->mOutputComponentType == otb::ImageIOBase::UNKNOWNCOMPONENTTYPE)
		this->mOutputComponentType = this->mItkImgIOBase->GetComponentType();

	this->mOutputNumDimensions = this->mItkImgIOBase->GetNumberOfDimensions();
    //this->mInputNumBands = this->mItkImgIOBase->GetNumberOfComponents();

	if (this->mInputComponentType == otb::ImageIOBase::UNKNOWNCOMPONENTTYPE)
		this->mInputComponentType = this->mItkImgIOBase->GetComponentType();

	this->mInputNumDimensions = this->mItkImgIOBase->GetNumberOfDimensions();

    if (!mbRasMode)
    {
        otb::GDALRATImageIO::Pointer gio = static_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());

        this->mInputNumBands = gio->GetTotalNumberOfBands();

        if (mBandMap.size() > 0 && mOutputNumBands > 1)
        {
            mOutputNumBands = mBandMap.size();
            gio->SetBandMap(mBandMap);
        }

        NMDebugAI(<< "img size: " << gio->GetDimensions(0)
                  << " x " << gio->GetDimensions(1) << std::endl);
        uint numOvv = gio->GetNbOverviews();
        for (uint i=0; i < numOvv; ++i)
        {
            std::vector<unsigned int> s = gio->GetOverviewSize(i);
            NMDebugAI(<< "#" << i+1 << " overview: " << s[0] << " x " << s[1] << std::endl);
        }
    }


    NMDebugAI(<< "... numBands in/out: " << this->mInputNumBands << "/"
              << this->mOutputNumBands << endl);
	NMDebugAI(<< "... comp type: " << this->mOutputComponentType << endl);

	bool ret = true;
	switch (this->mOutputComponentType)
	{
	case otb::ImageIOBase::UCHAR:
		CallReaderMacro( unsigned char );
		break;
	case otb::ImageIOBase::CHAR:
		CallReaderMacro( char );
		break;
	case otb::ImageIOBase::USHORT:
		CallReaderMacro( unsigned short );
		break;
	case otb::ImageIOBase::SHORT:
		CallReaderMacro( short );
		break;
	case otb::ImageIOBase::UINT:
		CallReaderMacro( unsigned int );
		break;
	case otb::ImageIOBase::INT:
		CallReaderMacro( int );
		break;
	case otb::ImageIOBase::ULONG:
		CallReaderMacro( unsigned long );
		break;
	case otb::ImageIOBase::LONG:
		CallReaderMacro( long );
		break;
	case otb::ImageIOBase::FLOAT:
		CallReaderMacro( float );
		break;
	case otb::ImageIOBase::DOUBLE:
		CallReaderMacro( double );
		break;
	default:
        NMLogError(<<  ctxNMImageReader << ": UNKNOWN DATA TYPE, couldn't create Pipeline!");
		ret = false;
		break;
	}
	this->mbIsInitialised = ret;

    this->setInternalRATType();

	// set the observer
	ReaderObserverType::Pointer observer = ReaderObserverType::New();
	observer->SetCallbackFunction(this,
			&NMImageReader::UpdateProgressInfo);
	//this->mOtbProcess->AddObserver(itk::ProgressEvent(), observer);
	this->mOtbProcess->AddObserver(itk::StartEvent(), observer);
	this->mOtbProcess->AddObserver(itk::EndEvent(), observer);
	this->mOtbProcess->AddObserver(itk::AbortEvent(), observer);
    this->mOtbProcess->AddObserver(itk::NMLogEvent(), observer);

	NMDebugCtx(ctxNMImageReader, << "done!");

	return ret;
}

void
NMImageReader::buildOverviews(const std::string& resamplingType)
{
    // CallBuildOverivews
    if (!mbRasMode)
    {
        otb::GDALRATImageIO::Pointer gio = static_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());
        if (gio)
        {
            switch(this->mOutputComponentType)
            {
            LocalMacroPerSingleType( CallBuildOverviews )
            default:
                break;
            }
        }
    }
}


std::vector<double> NMImageReader::getImageStatistics(const int *index, const int *size)
{
    std::vector<double> stats;

    switch(this->mOutputComponentType)
    {
    LocalMacroPerSingleType( CallGetImgStatsMacro )
    default:
        break;
    }


    return stats;
}

void
NMImageReader::setInternalRATType()
{
    if (!mbRasMode)
    {
        otb::GDALRATImageIO::Pointer gio = static_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());
        if (gio)
        {
            otb::AttributeTable::TableType ttype;
            if(mRATType.compare(QString("ATTABLE_TYPE_RAM")) == 0)
            {
                ttype = otb::AttributeTable::ATTABLE_TYPE_RAM;
            }
            else
            {
                ttype = otb::AttributeTable::ATTABLE_TYPE_SQLITE;
            }


            switch(this->mOutputComponentType)
            {
            LocalMacroPerSingleType( SetHelperRATType )
            default:
                break;
            }
        }
    }
}

void
NMImageReader::setOverviewIdx(int ovvidx, int* userLPR)
{
    if (!mbRasMode)
    {
        otb::GDALRATImageIO::Pointer gio = static_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());
        if (gio)
        {
            switch(this->mOutputComponentType)
            {
            LocalMacroPerSingleType( SetReaderOverviewIdx )
            default:
                break;
            }
        }
    }
}

int
NMImageReader::getNumberOfOverviews()
{
    int ret = 0;
    if (!mbRasMode)
    {
        otb::GDALRATImageIO::Pointer gio = static_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());

        if (gio)
        {
            ret = gio->GetNbOverviews();
        }
    }
    return ret;
}

std::vector<unsigned int>
NMImageReader::getOverviewSize(int ovvidx)
{
    std::vector<unsigned int> ret;
    if (!mbRasMode)
    {
        otb::GDALRATImageIO::Pointer gio = static_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());

        if (    gio
            &&  (   ovvidx >= 0
                 && ovvidx < gio->GetNbOverviews()
                )
           )
        {
            ret = gio->GetOverviewSize(ovvidx);
        }
    }
    return ret;
}

void
NMImageReader::UpdateProgressInfo(itk::Object* obj, const itk::EventObject& event)
{
	// just call base class implementation here
	NMProcess::UpdateProgressInfo(obj, event);
}

QSharedPointer<NMItkDataObjectWrapper> NMImageReader::getOutput(unsigned int idx)
{
	if (!this->mbIsInitialised)
	{
		NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
        e.setSource(this->objectName().toStdString());
        QString hostName = "";
        if (this->parent() != 0)
            hostName = this->parent()->objectName();
        QString msg = QString::fromLatin1("%1: NMImageReader::getOutput(%2) failed - Object not initialised!")
                .arg(hostName).arg(idx);
        e.setDescription(msg.toStdString());
		throw e;
	}

	itk::DataObject *img = 0;
	switch(this->mOutputComponentType)
	{
	case otb::ImageIOBase::UCHAR:
		RequestReaderOutput( unsigned char );
		break;
	case otb::ImageIOBase::CHAR:
		RequestReaderOutput( char );
		break;
	case otb::ImageIOBase::USHORT:
		RequestReaderOutput( unsigned short );
		break;
	case otb::ImageIOBase::SHORT:
		RequestReaderOutput( short );
		break;
	case otb::ImageIOBase::UINT:
		RequestReaderOutput( unsigned int );
		break;
	case otb::ImageIOBase::INT:
		RequestReaderOutput( int );
		break;
	case otb::ImageIOBase::ULONG:
		RequestReaderOutput( unsigned long );
		break;
	case otb::ImageIOBase::LONG:
		RequestReaderOutput( long );
		break;
	case otb::ImageIOBase::FLOAT:
		RequestReaderOutput( float );
		break;
	case otb::ImageIOBase::DOUBLE:
		RequestReaderOutput( double );
		break;
	default:
        NMLogError(<<  ctxNMImageReader << ": UNKNOWN DATA TYPE, couldn't get output!");
		break;
	}

    QSharedPointer<NMItkDataObjectWrapper> dw(new NMItkDataObjectWrapper(this, img, this->mOutputComponentType,
            this->mOutputNumDimensions, this->mOutputNumBands));

	// in case we've got an attribute table, we fetch it
	dw->setOTBTab(this->getRasterAttributeTable(1));

	return dw;
}

itk::DataObject* NMImageReader::getItkImage(void)
{
	itk::DataObject *img = 0;
	unsigned int idx = 0;
	switch(this->mOutputComponentType)
	{
	case otb::ImageIOBase::UCHAR:
		RequestReaderOutput( unsigned char );
		break;
	case otb::ImageIOBase::CHAR:
		RequestReaderOutput( char );
		break;
	case otb::ImageIOBase::USHORT:
		RequestReaderOutput( unsigned short );
		break;
	case otb::ImageIOBase::SHORT:
		RequestReaderOutput( short );
		break;
	case otb::ImageIOBase::UINT:
		RequestReaderOutput( unsigned int );
		break;
	case otb::ImageIOBase::INT:
		RequestReaderOutput( int );
		break;
	case otb::ImageIOBase::ULONG:
		RequestReaderOutput( unsigned long );
		break;
	case otb::ImageIOBase::LONG:
		RequestReaderOutput( long );
		break;
	case otb::ImageIOBase::FLOAT:
		RequestReaderOutput( float );
		break;
	case otb::ImageIOBase::DOUBLE:
		RequestReaderOutput( double );
		break;
	default:
        NMLogError(<<  ctxNMImageReader << ": UNKNOWN DATA TYPE, couldn't get output!");
		break;
	}
	return img;
}

const otb::ImageIOBase* NMImageReader::getImageIOBase(void)
{
	if (this->mbIsInitialised)
		return this->mItkImgIOBase;
	else
		return 0;
}

void
NMImageReader::getOrigin(double origin[3])
{
    origin[0] = this->mItkImgIOBase->GetOrigin(0);
    origin[1] = this->mItkImgIOBase->GetOrigin(1);
    if (this->mItkImgIOBase->GetNumberOfDimensions() == 3)
    {
        origin[2] = this->mItkImgIOBase->GetOrigin(2);
    }
    else
    {
        origin[2] = 0;
    }
}

void
NMImageReader::getSpacing(double spacing[3])
{
    spacing[0] = this->mItkImgIOBase->GetSpacing(0);
    spacing[1] = this->mItkImgIOBase->GetSpacing(1);
    spacing[2] = this->mItkImgIOBase->GetNumberOfDimensions() == 3
                    ? this->mItkImgIOBase->GetSpacing(2)
                    : 0;
}

void NMImageReader::getBBox(double bbox[6])
{
	bool ddd = this->mItkImgIOBase->GetNumberOfDimensions() == 3 ? true : false;

	double xmin = this->mItkImgIOBase->GetOrigin(0);
	double ymax = this->mItkImgIOBase->GetOrigin(1);
	double zmin = 0; //numeric_limits<double>::max() * -1;
	if (ddd) zmin = this->mItkImgIOBase->GetOrigin(2);

	//itk::ImageIORegion region = this->mItkImgIOBase->GetIORegion();
	int ncols = this->mItkImgIOBase->GetDimensions(0);
	int nrows = this->mItkImgIOBase->GetDimensions(1);
	int nlayers = 1;
	if (ddd) nlayers = this->mItkImgIOBase->GetDimensions(2);

	double csx = this->mItkImgIOBase->GetSpacing(0);
	double csy = this->mItkImgIOBase->GetSpacing(1);
	if (csy < 0) csy *= -1;
	double csz = 0;
	if (ddd) csz = this->mItkImgIOBase->GetSpacing(2);

	double xmax = xmin + (ncols * csx);
	double ymin = ymax - (nrows * csy);
	double zmax = 0; //numeric_limits<double>::max();
	if (ddd) zmax = zmin + (nlayers * csz);

	bbox[0] = xmin;
	bbox[1] = xmax;
	bbox[2] = ymin;
	bbox[3] = ymax;
	bbox[4] = zmin;
	bbox[5] = zmax;
}


void
NMImageReader::linkParameters(unsigned int step,
		const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(ctxNMImageReader, << "...");
	// set the step parameter according to the ParameterHandling mode set for this process

    QVariant param = this->getParameter("FileNames");
    if (param.isValid() && !param.toString().isEmpty())
    {
        this->setFileName(param.toString());
        QString paramProvN = QString("nm:FileNames=\"%1\"").arg(param.toString());
        this->addRunTimeParaProvN(paramProvN);
    }

    param = this->getParameter("BandList");
    if (param.isValid() && param.type() == QVariant::StringList)
    {
        QStringList bandlist = param.toStringList();
        this->mBandMap.clear();
        bool bok;
        foreach(const QString& band, bandlist)
        {
            int b = band.toInt(&bok);
            if (!bok)
            {
                NMWarn(ctxNMImageReader, "Not all bands of the band map were "
                       "identified correctly!");
                continue;
            }
            this->mBandMap.push_back(b);
        }

        QString bandListProvN = QString("nm:BandList=\"%1\"").arg(bandlist.join(" "));
        this->addRunTimeParaProvN(bandListProvN);
    }

    this->setInternalRATType();

    NMDebugAI(<< "FileName set to '" << this->mFileName.toStdString() << "'" << endl);
    this->initialise();

	NMDebugCtx(ctxNMImageReader, << "done!");
}


void NMImageReader::setNthInput(unsigned int numInput,
        QSharedPointer<NMItkDataObjectWrapper> img)
{
	// don't need file input for the reader
}

void NMImageReader::instantiateObject(void)
{
    NMDebugCtx(this->objectName().toStdString(), << "...");
	// grab properties and feed member vars for successful initialisation
#ifdef BUILD_RASSUPPORT
	if (this->mRasConnector != 0)
	{
		RasdamanConnector* rasconn = const_cast<RasdamanConnector*>(
				this->mRasConnector->getConnector());
		this->setRasdamanConnector(rasconn);
	}
	else
	{
		this->setRasdamanConnector(0);
	}
#endif	

    // put the name of the hosting iterable component
    // because that's the 'visible' feature in the
    // GUI rather than the internal NMProcess-derived class
    QString source = this->objectName();
    if (this->parent())
    {
        source = this->parent()->objectName();
    }

    QVariant param = this->getParameter("FileNames");
    if (param.isValid() && !param.toString().isEmpty())
    {
        QString fn = param.toString();
        QFileInfo fifo(fn);
        if (fifo.isFile() && fifo.isReadable())
        {
            this->setFileName(param.toString());
        }
        else
        {
            NMMfwException ex(NMMfwException::NMProcess_InvalidParameter);
            ex.setSource(source.toStdString());
            QString msg = QString(tr("File %1 does not exist or is not readable!"))
                    .arg(fn);
            ex.setDescription(msg.toStdString());
            NMDebugCtx(this->objectName().toStdString(), << "done!");
            throw ex;
        }
    }

    param = this->getParameter("BandList");
    if (param.isValid() && param.type() == QVariant::StringList)
    {
        QStringList bandlist = param.toStringList();
        this->mBandMap.clear();
        bool bok;
        foreach(const QString& band, bandlist)
        {
            int b = band.toInt(&bok);
            if (!bok)
            {
                NMLogWarn(<< source.toStdString() ": Not all bands of the band map were "
                       "identified correctly!");
                continue;
            }
            this->mBandMap.push_back(b);
        }
    }


    NMDebugAI(<< "FileName set to '" << this->mFileName.toStdString() << "'" << endl);
    this->initialise();


    NMDebugCtx(this->objectName().toStdString(), << "done!");
}

#ifdef BUILD_RASSUPPORT
void NMImageReader::setRasConnector(NMRasdamanConnectorWrapper* rw)
{
    if (rw != this->mRasConnector)
    {
        this->mRasConnector = rw;
        emit nmChanged();
    }
}

NMRasdamanConnectorWrapper* NMImageReader::getRasConnector(void)
{
    return this->mRasConnector;
}
#endif


