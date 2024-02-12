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
#include "nmtypeinfo.h"
#define ctxNMImageReader "NMImageReader"

#include <limits>
#include <QFileInfo>

#include "NMImageReader.h"
#include "otbGDALRATImageIO.h"
#include "nmNetCDFIO.h"
#include "otbAttributeTable.h"
#include "otbVectorImage.h"
#include "itkImageBase.h"
#include "NMMfwException.h"
#include "NMIterableComponent.h"
#include "NMModelController.h"
#include "itkRGBPixel.h"
#include "otbStreamingStatisticsImageFilter.h"
#include "otbStreamingRATImageFileWriter.h"
#include "itkImageToHistogramFilter.h"
#include "itkExtractImageFilter.h"
#include "otbNMGridResampleImageFilter.h"

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

        static void setRequestedRegion(itk::ProcessObject::Pointer& procObj,
                                        unsigned int numBands, itk::ImageIORegion& ior, bool rgbMode)
        {
            if (numBands == 1)
            {
                ReaderType *r = dynamic_cast<ReaderType*>(procObj);
                r->SetRequestedRegion(ior);
            }
            else if (rgbMode && numBands == 3)
            {
                RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj);
                r->SetRequestedRegion(ior);
            }
            else
            {
                VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
                r->SetRequestedRegion(ior);
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
    typedef otb::Image< PixelType, 2>                   Img2DType;
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


    typedef typename otb::PersistentStatisticsImageFilter<ImgType>          StatsFilterType;

    typedef typename itk::ExtractImageFilter<ImgType, Img2DType>            ExtractFilterType;
    typedef typename otb::PersistentStatisticsImageFilter<Img2DType>        Stats2DFilterType;
    typedef typename otb::NMGridResampleImageFilter<Img2DType, Img2DType>   ResampleImage2DFilterType;

    typedef typename itk::Statistics::ImageToHistogramFilter<ImgType>       HistogramFilterType;
    typedef typename itk::Statistics::ImageToHistogramFilter<Img2DType>     Histogram2DFilterType;

    typedef typename otb::StreamingImageVirtualWriter<ImgType>              VirtWriterType;
    //typedef typename otb::StreamingImageVirtualWriter<Img2DType>            Virt2DWriterType;

    typedef typename otb::StreamingRATImageFileWriter<Img2DType>            Virt2DWriterType;


    static void getImageStatistics(itk::ProcessObject* procObj, unsigned int numBands,
                              std::vector<double>& stats, const int* index, const int* size,
                                   const int zSliceIdx)
    {
        stats.clear();

        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj);
            r->UpdateOutputInformation();

            typename ImgType::PointType inOrigin = r->GetOutput()->GetOrigin();
            typename ImgType::SpacingType inSpacing = r->GetOutput()->GetSpacing();

            const int curOvvIdx = r->GetOverviewIdx();
            if (r->GetOverviewsCount() > 0)
            {
                r->SetOverviewIdx(r->GetOverviewsCount()-1);
            }

            typename ReaderType::ImageRegionType reg;
            if (index != nullptr && size != nullptr)
            {
                typename ReaderType::ImageRegionType::IndexType idx;
                typename ReaderType::ImageRegionType::SizeType sz;


                for (int d=0; d < ImgType::ImageDimension; ++d)
                {
                    idx[d] = d == 2 ? zSliceIdx : index[d];
                    sz[d] = d == 2 ? 0 : size[d];
                }
                reg.SetIndex(idx);
                reg.SetSize(sz);
                r->UseUserLargestPossibleRegionOn();
                r->SetUserLargestPossibleRegion(reg);
            }
            else
            {
                reg = r->GetOutput()->GetLargestPossibleRegion();
                if (ImgType::ImageDimension == 3)
                {
                    reg.SetIndex(2, zSliceIdx);
                    reg.SetSize(2, 0);
                }
            }

            // we reduce to an easily managable size to speed up stats
            // processing as much as possible
            double div = div = std::max(reg.GetSize()[0],reg.GetSize()[1]) / 1024.0;

            typename ResampleImage2DFilterType::Pointer res = ResampleImage2DFilterType::New();
            typename ExtractFilterType::Pointer ex = ExtractFilterType::New();
            typename Stats2DFilterType::Pointer f = Stats2DFilterType::New();
            typename Virt2DWriterType::Pointer w = Virt2DWriterType::New();

            w->SetAutomaticStrippedStreaming(512);
            w->SetWriteImage(false);

            unsigned int nth = f->GetNumberOfThreads();
            nth = nth > 0 ? nth : 1;

            //if (ImgType::ImageDimension == Img2DType::ImageDimension)
            {
                ex->SetInPlace(true);
            }
            ex->SetDirectionCollapseToSubmatrix();
            ex->SetInput(r->GetOutput());
            ex->SetExtractionRegion(reg);

            if (div > 1)
            {
                typename Img2DType::IndexType startIndex;
                typename Img2DType::SizeType outputSize;
                typename Img2DType::SpacingType outputSpacing;
                typename Img2DType::PointType outputOrigin;

                for (int d=0; d < 2; ++d)
                {
                    outputOrigin[d] = inOrigin[d];
                    startIndex[d] = reg.GetIndex(d) / div;
                    outputSize[d] = reg.GetSize(d) / div;
                    outputSpacing[d] = (inSpacing[d] * reg.GetSize(d)) / outputSize[d];
                }

                res->SetInput(ex->GetOutput());
                res->SetOutputOrigin(outputOrigin);
                res->SetOutputSize(outputSize);
                res->SetOutputStartIndex(startIndex);
                res->SetOutputSpacing(outputSpacing);

                f->SetInput(res->GetOutput());
                //f->SetInput(ex->GetOutput());
            }
            else
            {
                f->SetInput(ex->GetOutput());
            }

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

            r->SetOverviewIdx(curOvvIdx);
        }
        else
        {
            stats.resize(7, -9999);
            return;
        }
    }

    static void getImageHistogram(itk::ProcessObject* procObj, unsigned int numBands,
                              std::vector<double>& bins, std::vector<int>& freqs,
                              const int numBins, double binMin, double binMax,
                              const int* index, const int* size)
    {
        bins.clear();
        freqs.clear();

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

            using SizeType = typename HistogramFilterType::HistogramType::SizeType;
            SizeType histSize(1);
            histSize.Fill(numBins);

            using HistMeasureType = typename HistogramFilterType::HistogramMeasurementVectorType;
            HistMeasureType msMin(numBins);
            HistMeasureType msMax(numBins);
            msMin.Fill(binMin);
            msMax.Fill(binMax);

            typename HistogramFilterType::Pointer f = HistogramFilterType::New();
            //typename VirtWriterType::Pointer w = VirtWriterType::New();
            //w->SetAutomaticStrippedStreaming(512);

            unsigned int nth = f->GetNumberOfThreads();
            nth = nth > 0 ? nth : 1;

            f->SetInput(r->GetOutput());
            f->SetNumberOfThreads(nth);
            f->SetAutoMinimumMaximum(false);
            f->SetHistogramSize(histSize);
            f->SetHistogramBinMinimum(msMin);
            f->SetHistogramBinMaximum(msMax);
            f->Update();

            //w->SetInput(f->GetOutput());
            //w->Update();

            using HistogramType = typename HistogramFilterType::HistogramType;
            HistogramType* hist = f->GetOutput();
            int hsize = hist->GetSize()[0];
            freqs.resize(hsize);
            bins.resize(hsize);
            for (unsigned int i=0; i < hist->GetSize()[0]; ++i)
            {
                freqs[i] = hist->GetFrequency(i);
                bins[i] = hist->GetMeasurement(i, 0);
            }
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
        setDbRATReadOnly(itk::ProcessObject* procObj, unsigned int numBands,
                   bool bDbRATReadOnly, bool rgbMode)
    {
        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj);
            r->SetDbRATReadOnly(bDbRATReadOnly);
        }
        else if (rgbMode && numBands == 3)
        {
            RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj);
            r->SetDbRATReadOnly(bDbRATReadOnly);
        }
        else
        {
            VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
            r->SetDbRATReadOnly(bDbRATReadOnly);
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


    static void setRequestedRegion(itk::ProcessObject* procObj,
                                    unsigned int numBands, itk::ImageIORegion& ior, bool rgbMode)
    {
        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj);
            ReaderRegionType rreg;
            for (int d=0; d < ImageDimension; ++d)
            {
                rreg.SetIndex(d, ior.GetIndex()[d]);
                rreg.SetSize(d, ior.GetSize()[d]);
            }

            r->GetOutput()->SetRequestedRegion(rreg);
        }
        else if (rgbMode && numBands == 3)
        {
            RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj);
            RGBReaderRegionType rreg;
            for (int d=0; d < ImageDimension; ++d)
            {
                rreg.SetIndex(d, ior.GetIndex()[d]);
                rreg.SetSize(d, ior.GetSize()[d]);
            }
            r->GetOutput()->SetRequestedRegion(rreg);
        }
        else
        {
            VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
            VecReaderRegionType rreg;
            for (int d=0; d < ImageDimension; ++d)
            {
                rreg.SetIndex(d, ior.GetIndex()[d]);
                rreg.SetSize(d, ior.GetSize()[d]);
            }
            r->GetOutput()->SetRequestedRegion(rreg);
        }
    }

    static void setOverviewIdx(itk::ProcessObject::Pointer& procObj,
                                 unsigned int numBands, int ovvidx, const int* userLPR,
                               bool rgbMode)
    {
        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj.GetPointer());
            if (r == nullptr)
            {
                return;
            }
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
            if (r == nullptr)
            {
                return;
            }
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
            if (r == nullptr)
            {
                return;
            }
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

    static void setZSliceIdx(itk::ProcessObject::Pointer& procObj,
                                 unsigned int numBands, int slindex, bool rgbMode)
    {
        if (numBands == 1)
        {
            ReaderType *r = dynamic_cast<ReaderType*>(procObj.GetPointer());
            r->SetZSliceIdx(slindex);
            r->UpdateOutputInformation();
        }
        else if (rgbMode && numBands == 3)
        {
            RGBReaderType *r = dynamic_cast<RGBReaderType*>(procObj.GetPointer());
            r->SetZSliceIdx(slindex);
            r->UpdateOutputInformation();
        }
        else
        {
            VecReaderType *r = dynamic_cast<VecReaderType*>(procObj.GetPointer());
            r->SetZSliceIdx(slindex);
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
            reader->SetImageIO(imgIOBase);
            reader->SetFileName(imgName.toStdString().c_str());

            // keep references to the exporter and the reader
            // (we've already got a reference to the importer)
            readerProcObj = reader;
        }
        else if (rgbMode && numBands == 3)
        {
            RGBReaderTypePointer reader = RGBReaderType::New();
            reader->SetImageIO(imgIOBase);
            reader->SetFileName(imgName.toStdString().c_str());

            // keep references to the exporter and the reader
            // (we've already got a reference to the importer)
            readerProcObj = reader;
        }
        else
        {
            VecReaderTypePointer reader = VecReaderType::New();
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

    #define CallGetImageHistogram( PixelType ) \
    { \
      { \
          switch (this->mOutputNumDimensions) \
          { \
          case 1: \
              FileReader< PixelType, 1 >::getImageHistogram( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, bins, freqs, numBins, binMin, binMax, index, size); \
              break; \
          case 3: \
              FileReader< PixelType, 3 >::getImageHistogram( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, bins, freqs, numBins, binMin, binMax, index, size); \
              break; \
          default: \
              FileReader< PixelType, 2 >::getImageHistogram( \
                  this->mOtbProcess, \
                  this->mOutputNumBands, bins, freqs, numBins, binMin, binMax, index, size); \
          }\
      } \
    }



    #define CallGetImgStatsMacro( PixelType ) \
    { \
      { \
          switch (this->mOutputNumDimensions) \
          { \
          case 1: \
              FileReader< PixelType, 1 >::getImageStatistics( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, stats, index, size, mZSliceIdx); \
              break; \
          case 3: \
              FileReader< PixelType, 3 >::getImageStatistics( \
                      this->mOtbProcess, \
                      this->mOutputNumBands, stats, index, size, mZSliceIdx); \
              break; \
          default: \
              FileReader< PixelType, 2 >::getImageStatistics( \
                  this->mOtbProcess, \
                  this->mOutputNumBands, stats, index, size, mZSliceIdx); \
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

    #define SetReaderZSliceIdx( PixelType ) \
    {\
        switch (this->mOutputNumDimensions) \
        { \
        case 1: \
            FileReader<PixelType, 1 >::setZSliceIdx( \
                    this->mOtbProcess, this->mOutputNumBands, slindex, mRGBMode); \
            break; \
        case 3: \
            FileReader<PixelType, 3 >::setZSliceIdx( \
                    this->mOtbProcess, this->mOutputNumBands, slindex, mRGBMode); \
            break; \
        default: \
            FileReader<PixelType, 2 >::setZSliceIdx( \
                    this->mOtbProcess, this->mOutputNumBands, slindex, mRGBMode); \
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

    /*! Macro for setting whether Db-RAT should be opened readonly
     */
    #define SetHelperDbRATReadOnly( PixelType ) \
    {\
        switch (this->mOutputNumDimensions) \
        { \
        case 1: \
            FileReader<PixelType, 1 >::setDbRATReadOnly( \
                    this->mOtbProcess, this->mOutputNumBands, mDbRATReadOnly, mRGBMode); \
            break; \
        case 3: \
            FileReader<PixelType, 3 >::setDbRATReadOnly( \
                    this->mOtbProcess, this->mOutputNumBands, mDbRATReadOnly, mRGBMode); \
            break; \
        default: \
            FileReader<PixelType, 2 >::setDbRATReadOnly( \
                    this->mOtbProcess, this->mOutputNumBands, mDbRATReadOnly, mRGBMode); \
        }\
     }


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

    #define CallSetRequestedRegion( PixelType ) \
    {\
        switch (this->mOutputNumDimensions) \
        { \
        case 1: \
            FileReader<PixelType, 1 >::setRequestedRegion( \
                    this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
            break; \
        case 3: \
            FileReader<PixelType, 3 >::setRequestedRegion( \
                    this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
            break; \
        default: \
            FileReader<PixelType, 2 >::setRequestedRegion( \
                    this->mOtbProcess, this->mOutputNumBands, ior, mRGBMode); \
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
    this->mDbRATReadOnly = false;
#ifdef BUILD_RASSUPPORT
    this->mRasconn = 0;
    this->mRasConnector = 0;
    this->mbLinked = false;
#endif
    this->mZSliceIdx = -1;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("PixelType"));
    mUserProperties.insert(QStringLiteral("OutputNumDimensions"), QStringLiteral("NumDimensions"));
    mUserProperties.insert(QStringLiteral("OutputNumBands"), QStringLiteral("NumBands"));
    mUserProperties.insert(QStringLiteral("BandList"), QStringLiteral("BandList"));
    mUserProperties.insert(QStringLiteral("FileNames"), QStringLiteral("FileNames"));
    mUserProperties.insert(QStringLiteral("RATType"), QStringLiteral("RATType"));
    mUserProperties.insert(QStringLiteral("DbRATReadOnly"), QStringLiteral("DbRATReadOnly"));
    mUserProperties.insert(QStringLiteral("RGBMode"), QStringLiteral("RGBMode"));
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
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
    case otb::ImageIOBase::ULONGLONG:
        CallFetchRATClassMacro(unsigned long long);
        break;
    case otb::ImageIOBase::LONGLONG:
        CallFetchRATClassMacro(long long);
        break;
#endif
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
            otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(
                        this->mItkImgIOBase.GetPointer());
            otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(
                        this->mItkImgIOBase.GetPointer());

            if (gio.IsNotNull())
            {
                gio->SetBandMap(mBandMap);
                this->mOtbProcess->Modified();
            }
            else if (nio.IsNotNull())
            {
                nio->SetBandMap(mBandMap);
            }
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
        this->mbIsInitialised = false;
        return false;
    }

#ifdef BUILD_RASSUPPORT
    otb::RasdamanImageIO::Pointer rio
#endif
    otb::NetCDFIO::Pointer nio;
    otb::GDALRATImageIO::Pointer gio;

    if (!mbRasMode)
    {
        // think about a better way
        if (this->mFileName.contains(".nc"))
        {
            NMDebugAI( << "we're now in netCDF mode ..." << endl);
            nio = otb::NetCDFIO::New();
            this->mItkImgIOBase = nio;
        }
        else
        {
            NMDebugAI(<< "we're not in ras mode ..." << endl);
            gio = otb::GDALRATImageIO::New();
            gio->SetRATSupport(true);
            if (mRATType.compare(QString("ATTABLE_TYPE_RAM")) == 0)
            {
                gio->SetRATType(otb::AttributeTable::ATTABLE_TYPE_RAM);
            }
            else
            {
                gio->SetRATType(otb::AttributeTable::ATTABLE_TYPE_SQLITE);
            }
            gio->SetDbRATReadOnly(mDbRATReadOnly);
            gio->SetRGBMode(mRGBMode);
            gio->SetBandMap(mBandMap);
            this->mItkImgIOBase = gio;
        }
    }
#ifdef BUILD_RASSUPPORT
    else
    {
        NMDebugAI(<< "rasmode!" << endl);
        rio = otb::RasdamanImageIO::New();

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
        this->mbIsInitialised = false;
        return false;
    }

    // Now that we found the appropriate ImageIO class, ask it to
    // read the meta data from the image file.
    this->mItkImgIOBase->SetFileName(this->mFileName.toStdString().c_str());
    if (!this->mItkImgIOBase->CanReadFile(this->mFileName.toStdString().c_str()))
    {
        NMLogError(<<  ctxNMImageReader << ": Failed reading '" << this->mFileName.toStdString() << "'!");
        NMDebugCtx(ctxNMImageReader, << "done!");
        this->mbIsInitialised = false;
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
        if (gio != nullptr)
        {
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
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
    case otb::ImageIOBase::ULONGLONG:
        CallReaderMacro(unsigned long long);
        break;
    case otb::ImageIOBase::LONGLONG:
        CallReaderMacro(long long);
        break;
#endif
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
    this->setInternalDbRATReadOnly();

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
        otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(this->mItkImgIOBase.GetPointer());
        otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());
        if (gio.IsNotNull() || nio.IsNotNull())
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

void NMImageReader::getImageHistogram(std::vector<double>& bins,
                                      std::vector<int>& freqs,
                                      int numBins, double binMin, double binMax,
                                      const int* index, const int* size)
{
    switch(this->mOutputComponentType)
    {
    LocalMacroPerSingleType( CallGetImageHistogram )
    default:
        break;
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
        otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());
        if (gio.IsNotNull())
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
NMImageReader::setInternalDbRATReadOnly()
{
    if (!mbRasMode)
    {
        otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());
        if (gio.IsNotNull())
        {
            switch(this->mOutputComponentType)
            {
            LocalMacroPerSingleType( SetHelperDbRATReadOnly )
            default:
                break;
            }
        }
    }
}



void
NMImageReader::setOverviewIdx(int ovvidx, const int *userLPR)
{
    if (!mbRasMode)
    {
        otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(this->mItkImgIOBase.GetPointer());
        otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());
        if (gio.IsNotNull() || nio.IsNotNull())
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

void
NMImageReader::setZSliceIdx(int slindex)
{
    if (!mbRasMode)
    {
        otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(this->mItkImgIOBase.GetPointer());
        if (nio.IsNotNull() && this->getOutputNumDimensions() == 3)
        {
            this->mZSliceIdx = slindex;
            switch(this->mOutputComponentType)
            {
            LocalMacroPerSingleType( SetReaderZSliceIdx )
            default:
                break;
            }
        }
    }
}


void
NMImageReader::setRequestedRegion(itk::ImageIORegion &ior)
{
    switch(this->mOutputComponentType)
    {
    LocalMacroPerSingleType( CallSetRequestedRegion )
    default:
        break;
    }
}

int
NMImageReader::getNumberOfOverviews()
{
    int ret = 0;
    if (!mbRasMode)
    {


        otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(this->mItkImgIOBase.GetPointer());
        otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());

        if (gio.IsNotNull())
        {
            ret = gio->GetNbOverviews();
        }
        else if (nio.IsNotNull())
        {
            ret = nio->GetNbOverviews();
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
        otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(this->mItkImgIOBase.GetPointer());
        otb::GDALRATImageIO::Pointer gio = dynamic_cast<otb::GDALRATImageIO*>(
                    this->mItkImgIOBase.GetPointer());
        if (gio.IsNotNull())
        {
            if (    ovvidx >= 0
                &&  ovvidx < gio->GetNbOverviews()
               )
            {
                ret = gio->GetOverviewSize(ovvidx);
            }
            else
            {
                for (unsigned int d=0; d < gio->GetNumberOfDimensions(); ++d)
                {
                    ret.push_back(gio->getLPR()[d]);
                }
            }
        }
        else if (nio.IsNotNull())
        {
            if (    ovvidx >= 0
                &&  ovvidx < nio->GetNbOverviews()
               )
            {
                ret = nio->GetOverviewSize(ovvidx);
            }
            else
            {
                for (unsigned int d=0; d < nio->GetNumberOfDimensions(); ++d)
                {
                    ret.push_back(nio->getLPR()[d]);
                }
            }
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
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
    case otb::ImageIOBase::ULONGLONG:
        RequestReaderOutput(unsigned long long);
        break;
    case otb::ImageIOBase::LONGLONG:
        RequestReaderOutput(long long);
        break;
#endif
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
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
    case otb::ImageIOBase::ULONGLONG:
        RequestReaderOutput(unsigned long long);
        break;
    case otb::ImageIOBase::LONGLONG:
        RequestReaderOutput(long long);
        break;
#endif
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
NMImageReader::getUpperLeftCorner(double ulcorner[3])
{
    double origin[3];
    this->getOrigin(origin);

    ulcorner[0] = origin[0] - 0.5 * this->mItkImgIOBase->GetSpacing(0);
    // note: we expect y-axis spacing from the ImageIO to be negative
    ulcorner[1] = origin[1] - (0.5 * this->mItkImgIOBase->GetSpacing(1));
    if (this->mItkImgIOBase->GetNumberOfDimensions() == 3)
    {
        // note: this assumes zmax (top) pixel is origin (?)
        ulcorner[2] = origin[2] + 0.5 * this->mItkImgIOBase->GetSpacing(2);
    }
    else
    {
        ulcorner[2] = origin[2];
    }
}

void
NMImageReader::getSignedSpacing(double signedspacing[3])
{
    signedspacing[0] = this->mItkImgIOBase->GetSpacing(0);
    signedspacing[1] = this->mItkImgIOBase->GetSpacing(1);
    signedspacing[2] = this->mItkImgIOBase->GetNumberOfDimensions() == 3
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

    // turn pixel centre-based origin coordinates into
    // upper left corner coordinates
    xmin -= 0.5 * csx;
    ymax += 0.5 * csy;
    zmin -= 0.5 * csz;

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
        QSharedPointer<NMItkDataObjectWrapper> img, const QString& name)
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
        if (!fn.contains(".nc:"))
        {
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
        else
        {
            this->setFileName(param.toString());
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


