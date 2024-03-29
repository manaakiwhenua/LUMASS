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
 * NMImageLayer.h
 *
 *  Created on: 18/01/2012
 *      Author: alex
 */

#ifndef NMIMAGELAYER_H_
#define NMIMAGELAYER_H_
#define ctxNMImageLayer "NMImageLayer"

#include <NMLayer.h>
#include <NMImageReader.h>
#include <NMItk2VtkConnector.h>
#include <NMDataComponent.h>

#include "itkDataObject.h"
#include "otbImage.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "otbImageIOBase.h"
#include "vtkImageHistogram.h"
#include "NMVtkOpenGLImageSliceMapper.h"
#include "vtkImageSlice.h"
#include "vtkImageProperty.h"
#include "vtkAlgorithmOutput.h"


#ifdef BUILD_RASSUPPORT
  #include "RasdamanConnector.hh"
#endif

class vtkIdTypeArray;
class NMChartView;

class NMImageLayer: public NMLayer
{
    Q_OBJECT

public:
    NMImageLayer(vtkRenderWindow* renWin,
            vtkRenderer* renderer=0,
            QObject* parent=0);
    virtual ~NMImageLayer();

    otb::ImageIOBase::IOComponentType getITKComponentType(void);
    unsigned int getNumDimensions(void)
        {return this->mNumDimensions;}
    unsigned int getNumBands(void)
            {return this->mNumBands;}
    unsigned int getTotalNumBands(void)
            {return this->mTotalNumBands;}


    bool setFileName(QString filename);

#ifdef BUILD_RASSUPPORT
    void setRasdamanConnector(RasdamanConnector* rasconn)
        {this->mpRasconn = rasconn;}
#endif

    const vtkDataSet* getDataSet(void);

    /*!
     * \brief Calculates pixel indices for a given world coordinate on the
     *        displayed image. bOnLPR=true ensures that pixel indices of
     *        the largest possible region of the image are returned rather
     *        than pixel indices of an associated overview image (pyramid layer).
     * \param world
     * \param pixel
     * \param bOnLPR
     */
    void world2pixel(double world[3], int pixel[3],
        bool bOnLPR, bool bImgConstrained);

    void getBBox(double bbox[6]);
    const double* getBBox(){return (double*)&mBBox[0];}
    const double* getSignedSpacing(){return (double*)&mSignedSpacing[0];}
    const double* getOrigin(){return (double*)&mOrigin[0];}
    const int*    getVisibleRegion(){return (int*)&mVisibleRegion[0];}

    void getSignedOverviewSpacing(int ovidx, double spac[3]);

    otb::AttributeTable::Pointer getRasterAttributeTable(int band);
    long long getNumTableRecords();

    void setImage(QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    void setUseOverviews(bool useOvv)
    {this->mbUseOverviews = useOvv;}
    bool getUseOverviews(void)
    {return this->mbUseOverviews;}

    int getOverviewIndex(void) {return mOverviewIdx;}

    itk::DataObject *getITKImage(void);
    vtkImageData* getVTKImage(void);
    vtkAlgorithmOutput* getItkVtkPipelineOutput();
    QSharedPointer<NMItkDataObjectWrapper> getImage(void);
    NMProcess* getProcess(void)
        {return this->mReader;}

    bool isRasLayer(void) {return this->mReader->isRasMode();}

    void setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg);
    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);

    std::vector<double> getWindowStatistics(void);
    std::vector<double> getWholeImageStatistics(void);

    const vtkImageProperty* getImageProperty(void)
        {return this->mImgProp;}

    double getDefaultNodata(void);

    void setBandMap(const std::vector<int> map);
    std::vector<int> getBandMap(void) {return this->mBandMap;}
    std::vector<std::vector<int> > getOverviewSizes(void);

    void setScalarBand(const int& band)
        {this->mScalarBand = band >= 1 && band <= (int)this->mTotalNumBands ? band : 1;}
    int getScalarBand(void) {return this->mScalarBand;}

    int getZSliceIndex();
    void setZSliceIndex(int slindex);

public slots:
    void updateSourceBuffer(void);
    void writeDataSet(void);
    void selectionChanged(const QItemSelection& newSel,
            const QItemSelection& oldSel);

    void test(void);

    void mapExtentChanged(void);
    void setUpdateScalars()
        {this->mbUpdateScalars = true;}
    void setScalars(vtkImageData* img);
    void mapRGBImageScalars(vtkImageData* img);
    void mapRGBImage(void);
    void showHistogram(void);
    void updateHistogram(vtkObject*);

    void loadLegend(const QString &filename);

protected:

    void createTableView(void);
    void createImgSelData(void);
    void updateScalarBuffer(void);
    void updateSelectionColor(void);

    void sendData(QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    template<class T>
    void setLongScalars(T* buf, long long* out, long long numPix, long long nodata);

    template<class T>
    void setLongDBScalars(T* buf, long long* out, long long start, long long end, long long nodata);

    template<class T>
    void setDoubleScalars(T* buf, double* out, long long numPix, double nodata);

    template<class T>
    void setDoubleDBScalars(T* buf, double* out, long long start, long long end, double nodata);

    template<class T>
    void mapScalarsToRGB(T* in, unsigned char* out, int numPix, int numComp,
                         const std::vector<double>& minmax);

    template<class T>
    void setComponentScalars(T* in, T* out, int numPix);


    NMDataComponent* mSourceBuffer;
    NMImageReader* mReader;
    NMItk2VtkConnector* mPipeconn;

#ifdef BUILD_RASSUPPORT
    RasdamanConnector* mpRasconn;
#endif

    otb::AttributeTable::Pointer mOtbRAT;
    otb::ImageIOBase::IOComponentType mComponentType;

    itk::DataObject::Pointer mImage;
    vtkSmartPointer<vtkImageProperty> mImgProp;

    NMChartView* mHistogramView;

    struct LayerHistType
    {
        int numBins;
        double binMin;
        double binMax;
        std::vector<double> bins;
        std::vector<int>    freqs;

    } mHistogram;

    // selection mapping & vis
    vtkSmartPointer<vtkImageSliceMapper> mImgSelMapper;
    vtkSmartPointer<vtkImageSlice> mImgSelSlice;
    vtkSmartPointer<vtkImageProperty> mImgSelProperty;

    unsigned int mNumRecords;
    unsigned int mNumDimensions;
    unsigned int mNumBands;
    unsigned int mTotalNumBands;
    std::vector<int> mBandMap;
    std::vector<double> mBandMinMax;
    int mScalarBand;

    NMLayer::NMLegendType mLastLegendType;

    double mSignedSpacing[3];
    double mOrigin[3];
    double mUpperLeftCorner[3];

    int mVisibleRegion[6];

    int mZSliceIdx;
    int mOverviewIdx;
    bool mbUpdateScalars;

    // this keeps track of the visible extent
    // (in the map view) to decide whether new
    // data needs to be loaded or not
    double mWTLx;
    int mPrevZIdx;

    /*!
     * \brief The buffered (current) image region
     *        (real world coordinates)
     *
     * \sa mLargestPossibleRegion
     *
     */
    double mBufferedBox[6];

    bool mbStatsAvailable;
    bool mbLayerLoaded;

    /*!
     * \brief Image stats
     *
     * 0: min
     * 1: max
     * 2: mean
     * 3: median
     * 4: standard deviation
     * 5: number of pixels
     *
     */
    double mImgStats[6];

    bool mbUseOverviews;
    std::vector< std::vector<int> > mOverviewSize;

    int mScalarColIdx;
    static const int mMaxLayerDimensions;
    FILE* mScalarBufferFile;

    std::map<long long, long long> mScalarLongLongMap;
    std::map<long long, double> mScalarDoubleMap;

protected slots:
    int updateAttributeTable(void);
};

#endif // ifndef NMIMAGELAYER_H_
