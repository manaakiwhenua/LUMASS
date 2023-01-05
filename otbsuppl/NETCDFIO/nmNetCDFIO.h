 /******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2020 Manaaki Whenua - Landcare Research New Zealand Ltd
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

#ifndef __nmNetCDFIO_h
#define __nmNetCDFIO_h

#include <netcdf>
#include "nmlog.h"
#include "otbImageIOBase.h"
#include "otbAttributeTable.h"
#include "netcdfio_export.h"

#ifndef _WIN32
#   include <mpi.h>
#endif

namespace otb
{

class NETCDFIO_EXPORT NetCDFIO : public otb::ImageIOBase
{
public:

    typedef NetCDFIO Self;
    typedef otb::ImageIOBase Superclass;
    typedef itk::SmartPointer<Self> Pointer;
    typedef Superclass::ByteOrder ByteOrder;

    itkNewMacro(Self)
    itkTypeMacro(NetCDFIO, Superclass)

    // overwrite the set file name method to parse the image specs
    virtual void SetFileName(const char*);

    virtual bool CanReadFile(const char*);

    virtual bool CanStreamRead() {return true;}

    virtual void ReadImageInformation();
    virtual void Read(void* buffer);

    virtual bool CanWriteFile(const char*);
    virtual bool CanStreamWrite() {return true;}

    virtual void WriteImageInformation();
    void InternalWriteImageInformation();

    virtual void Write(const void* buffer);

    /*!
     * \brief Set/GetParallelIO
     * \param pio true | false
     */
    bool InitParallelIO(MPI_Comm& comm, MPI_Info& info, bool write=true);

    bool GetParallelIO(void)
    {return this->m_bParallelIO;}

    void FinaliseParallelIO(void);

    /** Set/Get the band map to be read/written by this IO */
    void SetBandMap(std::vector<int> map)
      {m_BandMap = map;}
    std::vector<int> GetBandMap() {return m_BandMap;}

    /** Set/Get whether pixels are represented as RGB(A) pixels */
    itkSetMacro(RGBMode, bool)
    itkGetMacro(RGBMode, bool)

    /** Retrieve the name of the overview container file */
    itkGetStringMacro(OverviewContainerName)

    itkSetMacro(CompressionLevel, int)
    itkGetMacro(CompressionLevel, int)

    /** Get total number of components (bands) of source data set */
    int GetTotalNumberOfBands(void) {return m_NbBands;}

    /** Set/Get whether files should be opened in update mode
     *  rather than being overridden */
    itkSetMacro(ImageUpdateMode, bool)
    itkGetMacro(ImageUpdateMode, bool)

    itkGetMacro(NbOverviews, int)

    itkGetMacro(ZSliceIdx, int)
    void SetZSliceIdx(int slindex);

    itkGetMacro(OverviewIdx, int)
    void SetOverviewIdx(int idx);

    void BuildOverviews(const std::string& method);
    std::vector<unsigned int> GetOverviewSize(int ovv);

    /*! \brief Returns the index of dimensional variable dimVarName of varName
     *
     *  Dimensional coordinates are often stored in 1D variables of a given
     *  dimension. This getDimIndex returns the index of that dimension within
     *  the variable given by varName. Note: The index is return in ITK/OTB-order
     *  (x,y,z,...), i.e. the fastest moving dimension has the index 0 (x), the
     *  second fastest 1 (y), the third fastest 2 (t), etc.
     *
     *  \param varName The name of the variable who's dimension is represented
     *         by the variable dimVarName
     *  \param dimVarName The name of the variable who's dimension's position (index)
     *         in varName is to be returned
     *  \return The 0-based index of the varName's dimension represented by the variable dimVarName;
     *          if dimVarName's dimension is shared with varName, its position (index) in VarName
     *          is returned, othwerwise -1.
     */
    std::vector<size_t> getDimMap(std::string varName, std::string dimVarName);

    /*! \brief Reads variable vname
     *
     *  Reads the variable vname. vname is expected to be located in the
     *  same group as the main variable specified with \ref FileName.
     *
     *  \param vname Name of the variable to be read
     *  \param idx vector specifying the n-D start position from which values should be read from the array
     *  \param len vector specifying the n-D region from which values should be read from the array
     *  \param targetType the underlying type of the provided buffer buf; if type is NULL, the type of the
     *              'vname' array is to be used; note: this enables coversion of the original array type to 'type'
     *  \param buf The pre-allocated image buffer into which array values are going to be copied in
     *
     *  \return 'true', if the variable could be read successfully, otherwise 'false'
     */
    bool getVar(std::string vname, std::vector<size_t> idx, std::vector<size_t> len, netCDF::NcType targetType, void* buf);
    const netCDF::NcType getVarType(std::string vname);


    std::vector<double> getUpperLeftCorner(void)
    {return m_UpperLeftCorner;}

    std::vector<unsigned int> getLPR()
    {return m_LPRDimensions;}


    void SetForcedLPR(const itk::ImageIORegion& forcedLPR);

    otb::AttributeTable::Pointer getRasterAttributeTable(int band);
    void setRasterAttributeTable(otb::AttributeTable* rat, int band);
    void WriteRAT(otb::AttributeTable* tab, unsigned int band=1);

    /** define some stubs here for abstract methods inherited from base clase */
    virtual unsigned int GetOverviewsCount() {return (unsigned int)0;}
    virtual std::vector<std::string> GetOverviewsInfo() {return std::vector<std::string>();}
    virtual void SetOutputImagePixelType( bool isComplexInternalPixelType,
        bool isVectorImage) {}

protected:
  NetCDFIO();
  virtual ~NetCDFIO();

  void updateOverviewInfo();

  void PrintSelf(std::ostream& os, itk::Indent indent) const;


    bool parseImageSpec(const std::string imagespec);
    otb::ImageIOBase::IOComponentType getOTBComponentType(
            netCDF::NcType::ncType nctype);

    netCDF::NcType::ncType getNetCDFComponentType(otb::ImageIOBase::IOComponentType otbtype);

    bool m_bCanRead;
    bool m_bCanWrite;
    bool m_bParallelIO;

    // this var is for internal use and indicates the
    // second stage of the creation of a new image
    // file in the collection
    bool m_bUpdateImage;

    // this var indicates whether an already available image
    // should be update in which case the user has to provide
    // and oid or _first_ | _<index>_ | _last_ to specify
    // which of the images in the collection should be updated
    bool m_ImageUpdateMode;

    itk::ImageIORegion m_ForcedLPR;
    bool m_UseForcedLPR;

    bool m_bWasWriteCalled;
    bool m_bImageInfoNeedsToBeWritten;

    otb::ImageIOBase::ByteOrder m_FileByteOrder;
    std::vector<otb::AttributeTable::Pointer> m_vecRAT;

    std::vector<double> m_UpperLeftCorner;

    std::string m_FileContainerName;
    std::vector<std::string> m_GroupNames;
    std::vector<int> m_GroupIDs;
    std::string m_NcVarName;

    std::string m_OverviewContainerName;

    netCDF::NcType::ncType m_ncType;
    std::string m_ImageTypeName;

    int m_CompressionLevel;
    int m_NbBands;

    std::vector<int> m_BandMap;
    bool m_RGBMode;

    int m_ZSliceIdx;
    int m_NbOverviews;
    int m_OverviewIdx;
    std::vector<std::vector<unsigned int > > m_OvvSize;



    std::vector<std::vector<double> > m_CoordMinMax;

    std::vector<unsigned int> m_LPRDimensions;
    std::vector<std::string>  m_DimensionNames;
    std::vector<double> m_LPRSpacing;

    bool m_bImageSpecParsed;

    bool m_ImgInfoHasBeenRead;

private:
    NetCDFIO(const Self&); //purposely not implemented
    void operator=(const Self&); //purposely not implemented

    /** Nombre d'octets par pixel */
    int m_BytePerPixel;

    netCDF::NcFile mFile;

    MPI_Comm m_MPIComm;
    MPI_Info m_MPIInfo;

    bool m_CreatedNotWritten;
    bool m_FlagWriteImageInformation;
    bool m_CanStreamWrite;

    bool m_IsComplex;


    /** whether the pixel type is
     *  vector (otb side)
     */
    bool m_IsVectorImage;

};


} 		 // namespace otb

#endif // __otbNetCDFIO_h
