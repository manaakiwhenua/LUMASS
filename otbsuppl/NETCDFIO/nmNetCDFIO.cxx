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

#include <string>
#include <iostream>
#include <sstream>
#include <limits>
#include <algorithm>

#include "NMMacros.h"
#include "nmlog.h"
#include "nmtypeinfo.h"
#include "nmNetCDFIO.h"
#include "otbSystem.h"
#include "otbImage.h"

#include "itkMetaDataObject.h"
#include "otbMetaDataKey.h"

#include "itkRGBPixel.h"
#include "itkRGBAPixel.h"
#include "itkArray2D.h"

#include "otbNMImageReader.h"
#include "otbNMGridResampleImageFilter.h"
#include "otbStreamingRATImageFileWriter.h"

#define NcGenerateOverviews( comptype ) \
{                           \
    switch(numDimensions)   \
    { \
    case 1: \
        PyramidHelper<comptype, 1>::GeneratePyramids( \
            imgIO, m_OvvSize, resampleType, imgVarName, inImgSpec, imgVarGrpName); \
        break; \
    case 2: \
        PyramidHelper<comptype, 2>::GeneratePyramids( \
            imgIO, m_OvvSize, resampleType, imgVarName, inImgSpec, imgVarGrpName); \
        break; \
    case 3: \
        PyramidHelper<comptype, 3>::GeneratePyramids( \
            imgIO, m_OvvSize, resampleType, imgVarName, inImgSpec, imgVarGrpName); \
        break; \
    }\
}


using namespace netCDF;

template<class PixelType, unsigned int ImageDimension>
class PyramidHelper
{
public:
    using ImgType                   = typename otb::Image<PixelType, ImageDimension>;
    using PointType                 = typename ImgType::PointType;
    using DirType                   = typename ImgType::DirectionType;
    using SpacingType               = typename ImgType::SpacingType;
    using SizeType                  = typename ImgType::SizeType;
    using RegionType                = typename ImgType::RegionType;

    using ReaderType                = typename otb::NMImageReader< ImgType >;
    using ReaderTypePointer         = typename ReaderType::Pointer;

    using PyramidFilterType         = typename otb::NMGridResampleImageFilter<ImgType, ImgType>;
    using PyramidFilterTypePointer  = typename PyramidFilterType::Pointer;

    using WriterType                = typename otb::StreamingRATImageFileWriter<ImgType>;
    using WriterTypePointer         = typename WriterType::Pointer;


    static void GeneratePyramids(otb::ImageIOBase* imgIO,
         std::vector<std::vector<unsigned int> >& m_OvvSize,
         std::string& resampleType,
         std::string& imgVarName,
         std::string& inImgSpec,
         std::string& imgVarGrpName
    )
    {
        otb::NetCDFIO* nio = dynamic_cast<otb::NetCDFIO*>(imgIO);
        if (nio == nullptr)
        {
            NMDebug(<< "Failed creating overviews!");
            return;
        }
        const unsigned int numOvv = m_OvvSize.size();
        std::string containerName = nio->GetOverviewContainerName();
        std::string inputVarSpec = inImgSpec;
        std::stringstream ovvn;
        std::vector<std::string> ovvnames;

        for (int level=1; level <= numOvv; ++level)
        {
            ovvn << containerName << ":"
                 << imgVarGrpName << "/OVERVIEWS_"
                 << imgVarName << "/"
                 << imgVarName << "_ovv_" << level;

            ovvnames.push_back(ovvn.str());
            ovvn.str("");
        }

        for (unsigned int out=0; out < numOvv; ++out)
        {
            ReaderTypePointer reader = ReaderType::New();
            if (out == 0)
            {
                reader->SetFileName(inputVarSpec);
            }
            else
            {
                reader->SetFileName(ovvnames.at(out-1));
            }

            PointType outOrigin;
            SpacingType outSpacing;
            SizeType outSize;

            for (int d = ImgType::ImageDimension - 1; d >=0; --d)
            {
                const double dirmult = nio->GetDirection(d)[d];
                const double ulc = nio->getUpperLeftCorner()[d];
                const double origSpacing = nio->GetSpacing(d);
                const unsigned int origSize = nio->GetDimensions(d);
                const double lrc = ulc + origSpacing * (double)origSize * dirmult;

                outSize[d] = m_OvvSize[out][d];
                outSpacing[d] = ((lrc - ulc) / outSize[d]) * dirmult;
                outOrigin[d] = ulc + 0.5 * outSpacing[d] * dirmult;
            }

            PyramidFilterTypePointer pygen = PyramidFilterType::New();
            pygen->SetInput(reader->GetOutput());
            pygen->SetInterpolationMethod(resampleType);
            pygen->SetOutputOrigin(outOrigin);
            pygen->SetOutputSpacing(outSpacing);
            pygen->SetOutputSize(outSize);
            pygen->SetDefaultPixelValue(0);

            const int mxth = pygen->GetNumberOfThreads();
            pygen->SetNumberOfThreads(std::max(mxth-2, 1));

            WriterTypePointer writer = WriterType::New();
            writer->SetInput(pygen->GetOutput());
            writer->SetAutomaticTiledStreaming(512);
            writer->SetResamplingType("NONE");
            writer->SetFileName(ovvnames.at(out));
            writer->Update();
        }
    }
};

namespace otb
{

/// ----------------------------------------- CONSTRUCTOR / DESTRUCTOR
NetCDFIO::NetCDFIO(void)
{
    // default number of dimensions is two
    this->SetNumberOfDimensions(2);

    // currently only scalars are supported
    m_PixelType = SCALAR;
    m_IsVectorImage = false;
    m_IsComplex = false;

    // reasonable default pixel type
    m_ComponentType = FLOAT;
    m_ncType = netCDF::NcType::nc_FLOAT;
    m_CompressionLevel = 5;

    // we don't have any info about the image so far ...
    m_bCanRead = false;
    m_bCanWrite = false;

    m_bImageSpecParsed = false;
    m_bWasWriteCalled = false;
    m_ImgInfoHasBeenRead = false;
    m_bImageInfoNeedsToBeWritten = false;
    m_bUpdateImage = true;

    this->m_ImageTypeName = "";

    m_UseForcedLPR = false;
    m_ImageUpdateMode = false;

    // the only supported extension is none ''
    this->AddSupportedReadExtension("");
    this->AddSupportedWriteExtension("");

    this->m_NbBands = 1;
    this->m_NbOverviews = 0;
    this->m_OverviewIdx = - 1;
    this->m_ZSliceIdx = -1;
}

NetCDFIO::~NetCDFIO()
{
}

// --------------------------- PUBLIC METHODS

void NetCDFIO::SetFileName(const char* filename)
{
    if (    this->m_FileName.compare(filename) == 0
         && (this->m_bCanRead || this->m_bCanWrite)
        )
    {
        NMLogDebug(<< "file name is set and we've checked accessibility already!");
        return;
    }

    if (this->parseImageSpec(filename))
    {
        this->m_FileName = filename;
    }
    else
    {
        this->m_FileName = "";
    }
}

void
NetCDFIO::SetForcedLPR(const itk::ImageIORegion& forcedLPR)
{
    this->m_ForcedLPR = forcedLPR;
    this->m_UseForcedLPR = true;
}


bool NetCDFIO::CanReadFile(const char* filename)
{
    if (!this->parseImageSpec(filename))
    {
        this->m_bCanRead = false;
        return this->m_bCanRead;
    }

    try
    {
        NcFile nc(this->m_FileContainerName, NcFile::read);
        if (nc.isNull())
        {
            this->m_bCanRead = false;
            return this->m_bCanRead;
        }

        NcGroup curGrp;
        for (int n=0; n < this->m_GroupNames.size(); ++n)
        {
            if (n == 0)
            {
                // this may include the root group name '/'
                curGrp = nc.getGroup(m_GroupNames.at(n), netCDF::NcGroup::AllGrps);
            }
            else
            {
                curGrp = curGrp.getGroup(m_GroupNames.at(n));
            }
            if (curGrp.isNull())
            {
                NMProcErr(<< "ERROR reading image '"
                           << filename << "': Invalid group '"
                           << m_GroupNames.at(n) << "'!");
                this->m_bCanRead = false;
                return m_bCanRead;
            }
            m_GroupIDs.push_back(curGrp.getId());
        }

        // in case we havent' got any child groups in the file
        if (m_GroupIDs.empty())
        {
            //m_GroupNames.push_back("/");
            m_GroupIDs.push_back(nc.getId());
        }

        NcGroup theGroup(m_GroupIDs.back());
        this->m_bCanRead = true;

        // if there's no variable name given, we assume
        // at least one, but possibly more than one
        // variable (i.e. bands) sharing the same dimension
        if (m_NcVarName.empty())
        {
            if (theGroup.getVarCount() == 0)
            {
                NMProcErr(<< "Failed reading image! Couldn't find"
                           << " any variable in this group!");
            }
            else
            {
                NMProcErr(<< "Sorry, we're not reading any multi-band "
                           << "images at this stage ..., but it'll come!");
            }
            this->m_bCanRead = false;

        }
        else
        {
            NcVar var = theGroup.getVar(m_NcVarName);
            if (var.isNull())
            {
                NMProcErr(<< "Failed reading image '"
                           << filename << "': Invalid variable '"
                           << m_NcVarName << "'!");
                this->m_bCanRead = false;
            }
            m_ncType = var.getType().getTypeClass();
        }


        // check whether we've got the right info we need ...
        // TBD:
        // - check for dimension variables
        return this->m_bCanRead;
    }
    catch(exceptions::NcException& e)
    {
        NMProcErr(<< "ERROR reading file '"
                   << filename << "': " << e.what());
        this->m_bCanRead = false;
    }

    return this->m_bCanRead;
}

void NetCDFIO::ReadImageInformation()
{
    if (m_ImgInfoHasBeenRead)
    {
        return;
    }

    if (!this->CanReadFile(this->GetFileName()))
    {
        return;
    }


    // what we need to do here
    // cellsize
    // image size
    // spacing
    // number of components
    // pixel type
    // crs description
    // LowerLeftCorner
    // LowerRightCorner
    // UpperLeftCorner
    // UpperRightCorner
    // GeoTransform
    // num bands

    //-----------
    // - get dimensions for the named variable
    //      - look up size of the dimensions

    try
    {
        NcFile nc(m_FileContainerName, NcFile::read);
        if (nc.isNull())
        {
            NMProcErr(<< "Failed opening file '"
                       << m_FileContainerName
                       << "' for reading!");
            return;
        }

        const int grpId = m_GroupIDs.size() > 0 ? m_GroupIDs.back() : nc.getId();
        NcGroup grp(grpId);
        NcVar var = grp.getVar(m_NcVarName);
        if (var.isNull())
        {
            NMProcErr(<< "Sorry, not dealing in possible multi-valued data at this stage!");
            return;
        }


        if (var.getEndianness() == netCDF::NcVar::nc_ENDIAN_BIG)
        {
            this->m_ByteOrder = otb::ImageIOBase::BigEndian;
        }
        else
        {
            this->m_ByteOrder = otb::ImageIOBase::LittleEndian;
        }

        // get dimensions
        m_Origin.clear();
        m_UpperLeftCorner.clear();
        m_LPRDimensions.clear();
        m_Dimensions.clear();
        m_DimensionNames.clear();
        m_CoordMinMax.clear();
        m_Spacing.clear();
        m_Direction.clear();
        m_LPRSpacing.clear();

        //this->SetNumberOfDimensions(var.getDimCount());
        const unsigned int nDim = var.getDimCount();
        this->m_NumberOfDimensions = nDim;

        this->m_NbBands = 1;
        //this->SetNumberOfComponents(m_NbBands);
        this->m_NumberOfComponents = m_NbBands;

        // we go 'backwards' here since netcdf has the
        // dimensions ordered slowest to fastest changing,
        // whereas otb/itk uses fastest, usually x, to
        // slowest, e.g. time for space-time cubes
        for (int d = var.getDimCount()-1, a=0; d >= 0; --d, ++a)
        {
            //set some defaults
            m_Origin.push_back(0.5);
            m_UpperLeftCorner.push_back(0);
            m_Spacing.push_back(1);
            m_LPRSpacing.push_back(1);
            std::vector<double> axis;
            for (int aa=0; aa < nDim; ++aa)
            {
                if (aa == a)
                {
                    axis.push_back(1);
                }
                else
                {
                    axis.push_back(0);
                }
            }
            m_Direction.push_back(axis);

            m_LPRDimensions.push_back(var.getDim(d).getSize());
            m_Dimensions.push_back(var.getDim(d).getSize());
            m_DimensionNames.push_back(var.getDim(d).getName());

            // the coordinate domain for each dimension
            std::vector<double> vMinMaxVal(2, 0);
            NcVar coorVar = grp.getVar(m_DimensionNames.back());
            if (coorVar.isNull())
            {
                NMLogWarn(<< "Couldn't get the coordinate variable for dimension '"
                           << m_DimensionNames.back() << "', "
                           << "so we have to use the pixel domain instead!");

                vMinMaxVal[0] = 0;
                vMinMaxVal[1] = m_LPRDimensions.back()-1;
            }
            else
            {
                const std::vector<size_t> idx0 = {0};
                const std::vector<size_t> idx1 = {1};
                const std::vector<size_t> idxMax = {m_LPRDimensions.back()-1};
                const std::vector<size_t> len = {1};

                double vidx1 = 0.0;
                coorVar.getVar(idx0, len, &vMinMaxVal[0]);
                coorVar.getVar(idx1, len, &vidx1);
                coorVar.getVar(idxMax, len, &vMinMaxVal[1]);

                m_Spacing[a] = std::abs(vidx1 - vMinMaxVal[0]);
                m_LPRSpacing[a] = m_Spacing[a];
            }

            // account for potential (y) dimension (dis-)order
            if (vMinMaxVal[0] > vMinMaxVal[1])
            {
                const double max = vMinMaxVal[0];
                vMinMaxVal[0] = vMinMaxVal[1];
                vMinMaxVal[1] = max;
            }
            m_CoordMinMax.push_back(vMinMaxVal);
        }


        // ====================================================
        //              READ OVERVIEW INFORMATION
        // ====================================================
        // got any Os?
        // --> seriously funny: The Two Ronnies
        // https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&uact=8&ved=2ahUKEwiy8pDt2f7nAhVaXn0KHcAEDVMQyCkwAHoECAsQBA&url=https%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3DpV1IP4N9ajg&usg=AOvVaw3L24FqM2zvemx_EQmgeMYB

        this->m_NbOverviews = 0;
        this->m_OvvSize.clear();

        std::string ovvGrpName = "OVERVIEWS_" + m_NcVarName;
        NcGroup ovvGrp = grp.getGroup(ovvGrpName);
        if (!ovvGrp.isNull())
        {
            // we expect at least getVarCount overviews in this group;
            // but since there could be more, we look for the explict
            // overview names used by this IO, i.e.
            //      <varname>_ovv_<scaling level>       (
            // scaling levels are 1, 2, ..., <number of overviews>-1

            std::stringstream ovvname;
            for (int v=0; v < ovvGrp.getVarCount(); ++v)
            {
                ovvname << m_NcVarName << "_ovv_" << v+1;
                NcVar ovvar = ovvGrp.getVar(ovvname.str());
                ovvname.str("");
                if (ovvar.isNull())
                {
                    continue;
                }

                // get overview size
                std::vector<unsigned int> ovvsize;
                for (int d = ovvar.getDimCount() - 1; d >= 0; --d)
                {
                    ovvsize.push_back(ovvar.getDim(d).getSize());
                }
                m_OvvSize.push_back(ovvsize);
            }
        }
        this->m_NbOverviews = m_OvvSize.size();

        // ====================================================
        //              IMAGE TYPE INFORMATION
        // ====================================================

        // get type
        NcType ncType = var.getType();
        this->SetComponentType(this->getOTBComponentType(ncType.getTypeClass()));
        this->m_BytePerPixel = ncType.getSize();

        this->SetFileTypeToBinary();

    }
    catch(exceptions::NcException& e)
    {
        NMProcErr(<< "Whoopsie - something went wrong: " << e.what());
        return;
    }

    // vector image?
    this->m_IsVectorImage = false;
    this->m_IsComplex = false;
    this->m_RGBMode = false;
    this->m_BandMap.clear();

    // calculate spacing, origin, upper left corner
    // m_CoordMinMax
    // m_Dimensions
    // m_LPRDimensions

    // iterate forward here since we're operating on
    // otb/itk-ordered dimensions ...
    for (int d=0; d < this->m_NumberOfDimensions; ++d)
    {
        // since we've got the origin at the
        // centre of the top left pixel (of
        // face down lying image cube)
        // for the y dimension that is usually
        // the second most quickly changing
        // index in row-major representation
        if (d == 1)
        {
            m_LPRSpacing[d] = m_LPRSpacing[d] * -1;
            m_Spacing[d] = m_Spacing[d] * -1;

            m_Origin[d] = m_CoordMinMax[d][1];
        }
        else
        {
            m_Origin[d] = m_CoordMinMax[d][0];
        }
        m_UpperLeftCorner[d] = m_Origin[d] - 0.5 * m_LPRSpacing[d];
    }

    // DONE
    this->m_ImgInfoHasBeenRead = true;
}


std::vector<size_t>
NetCDFIO::getDimMap(std::string varName, std::string dimVarName)
{
    std::vector<size_t> imap;

    if (!m_bImageSpecParsed)
    {
        return imap;
    }

    try
    {
        NcFile nc(this->m_FileContainerName, NcFile::read);
        const int imgGrpId = m_GroupIDs.size() > 0 ? m_GroupIDs.back() : nc.getId();
        NcGroup imgGrp(imgGrpId);

        NcVar dimVar = imgGrp.getVar(dimVarName);
        if (dimVar.isNull())
        {
            NMProcErr(<< "Couldn't find '" << dimVarName << "'!");
            return imap;
        }

        NcVar imgVar = imgGrp.getVar(varName);
        if (imgVar.isNull())
        {
            NMProcErr(<< "Couldn't find '" << varName << "'!");
            return imap;
        }

        std::vector<NcDim> imgDims = imgVar.getDims();
        std::vector<NcDim> dimDims = dimVar.getDims();
        if (dimDims.size() > 1)
        {
            NMProcWarn(<< "'" << dimVarName << "' has more than 1 dimension!");
        }

        for (size_t d=0; d < dimDims.size(); ++d)
        {
            for (size_t dd=0; dd < imgDims.size(); ++dd)
            {
                if (dimDims.at(d).getName().compare(imgDims.at(dd).getName()) == 0)
                {
                    imap.push_back(imgDims.size()-1-dd);
                }
            }
        }
    }
    catch(const exceptions::NcException& nce)
    {
        NMLogError(<< "Reading var '" << vname << "' failed: "
                   << nce.what());
        return imap;
    }

    return imap;
}

const NcType
NetCDFIO::getVarType(std::string vname)
{
    netCDF::NcType rtype;

    if (!m_bImageSpecParsed)
    {
        return rtype;
    }

    try
    {
        NcFile nc(this->m_FileContainerName, NcFile::read);
        const int imgGrpId = m_GroupIDs.size() > 0 ? m_GroupIDs.back() : nc.getId();
        NcGroup imgGrp(imgGrpId);

        NcVar imgVar = imgGrp.getVar(vname);
        if (!imgVar.isNull())
        {
            return imgVar.getType();
        }
    }
    catch(const exceptions::NcException& nce)
    {
        NMProcErr(<< "Reading var '" << vname << "' failed: "
                   << nce.what());
        return rtype;
    }

    return rtype;
}

bool
NetCDFIO::getVar(std::string vname,
                 std::vector<size_t> idx,
                 std::vector<size_t> len, NcType targetType, void* buf)
{
    bool ret = false;

    if (!m_bImageSpecParsed)
    {
        return ret;
    }

    try
    {
        NcFile nc(this->m_FileContainerName, NcFile::read);
        const int imgGrpId = m_GroupIDs.size() > 0 ? m_GroupIDs.back() : nc.getId();
        NcGroup imgGrp(imgGrpId);

        NcVar imgVar = imgGrp.getVar(vname);
        if (imgVar.isNull())
        {
            return ret;
        }

        NcType::ncType type = targetType.isNull()
                                ? imgVar.getType().getTypeClass()
                                : targetType.getTypeClass();
        switch(type)
        {
        case NcType::nc_BYTE:
            imgVar.getVar(idx, len, static_cast<signed char*>(buf));
            break;
        case NcType::nc_UBYTE:
        case NcType::nc_CHAR:
            imgVar.getVar(idx, len, static_cast<unsigned char*>(buf));
            break;
        case NcType::nc_SHORT:
            imgVar.getVar(idx, len, static_cast<short*>(buf));
            break;
        case NcType::nc_USHORT:
            imgVar.getVar(idx, len, static_cast<unsigned short*>(buf));
            break;
        case NcType::nc_INT:
            imgVar.getVar(idx, len, static_cast<int*>(buf));
            break;
        case NcType::nc_UINT:
            imgVar.getVar(idx, len, static_cast<unsigned int*>(buf));
            break;
        case NcType::nc_FLOAT:
            imgVar.getVar(idx, len, static_cast<float*>(buf));
            break;
        case NcType::nc_DOUBLE:
            imgVar.getVar(idx, len, static_cast<double*>(buf));
            break;
        case NcType::nc_INT64:
            imgVar.getVar(idx, len, static_cast<long long*>(buf));
            break;
        case NcType::nc_UINT64:
            imgVar.getVar(idx, len, static_cast<unsigned long long*>(buf));
            break;
        default:
            imgVar.getVar(idx, len, static_cast<double*>(buf));
            break;
        }

        ret = true;
    }
    catch(const exceptions::NcException& nce)
    {
        NMProcErr(<< "Reading var '" << vname << "' failed: "
                   << nce.what());
        return ret;
    }

    return ret;

}

void NetCDFIO::Read(void* buffer)
{
    // read the specified IORegion
    std::vector<size_t> start(this->m_NumberOfDimensions, 0);
    std::vector<size_t> len(this->m_NumberOfDimensions, 1);

    size_t numReadPix = 1;
    unsigned int id_netcdf = 0;
    for (int id_itk=m_NumberOfDimensions-1; id_itk >=0; --id_itk, ++id_netcdf)
    {
        start[id_netcdf] = this->GetIORegion().GetIndex()[id_itk];
        len[id_netcdf] = this->GetIORegion().GetSize()[id_itk];

        numReadPix *= len[id_netcdf];
    }

    if (m_NumberOfDimensions == 3 && m_ZSliceIdx >= 0)
    {
        start[0] = m_ZSliceIdx;
        len[0] = 1;
    }

    try
    {
        NcFile nc(this->m_FileContainerName, NcFile::read);
        const int imgGrpId = m_GroupIDs.size() > 0 ? m_GroupIDs.back() : nc.getId();
        NcGroup imgGrp(imgGrpId);

        std::stringstream readImgName;
        NcGroup readGrp;

        // reading overview image
        if (this->m_OverviewIdx >= 0 && this->m_OverviewIdx < this->m_OvvSize.size())
        {
            readImgName << m_NcVarName << "_ovv_" << m_OverviewIdx + 1;
            std::string ovvGrpName = "OVERVIEWS_" + m_NcVarName;
            readGrp = imgGrp.getGroup(ovvGrpName);

            // clamp z-slice start index to available overview-z indices
            if (m_NumberOfDimensions == 3 && m_ZSliceIdx >= 0)
            {
                start[0] = std::min(m_OvvSize[m_OverviewIdx][2]-1,
                        static_cast<unsigned int>(m_ZSliceIdx));
            }
        }
        else
        {
            readImgName << m_NcVarName;
            readGrp = imgGrp;
        }

        NcVar var = readGrp.getVar(readImgName.str());
        if (var.isNull())
        {
            NMProcErr(<< "Failed accessing the variable '"
                       << readImgName.str() << "' in group '"
                       << readGrp.getName() << "' of file '"
                       << m_FileContainerName << "'!");
            return;
        }

        var.getVar(start, len, buffer);
    }
    catch(exceptions::NcException& e)
    {
        NMProcErr(<< e.what());
    }
}

bool NetCDFIO::CanWriteFile(const char* filename)
{
    if (!this->parseImageSpec(filename))
    {
        this->m_bCanWrite = false;
        return this->m_bCanWrite;
    }

    NcFile nc;
    try
    {
        // assue file exists and try opening it for writing ...
        nc.open(this->m_FileContainerName, NcFile::write);
        this->m_bCanWrite = true;
    }
    catch(...)
    {
        try
        {
            // file does not exists, or couldn't be openend for writing,
            // so try creating a new file with the given name ...
            // ... this will fail if the file already exists
            nc.open(this->m_FileContainerName, NcFile::newFile);
            this->m_bCanWrite = true;
        }
        catch (...)
        {
            this->m_bCanWrite = false;
        }
    }

    return this->m_bCanWrite;
}

void NetCDFIO::BuildOverviews(const std::string &method)
{
    if (!m_bImageSpecParsed)
    {
        return;
    }

    if (!m_ImgInfoHasBeenRead)
    {
        this->ReadImageInformation();
    }

    // =======================================================================
    //              DETERMINE PYRAMID SCHEDULE
    // ========================================================================

    std::vector<bool> dimdone;
    std::vector<unsigned int> lastsize;
    for (int d=0; d < this->GetNumberOfDimensions(); ++d)
    {
        if (m_LPRDimensions[d] > 256 || d >= 2)
        {
            dimdone.push_back(false);
        }
        else
        {
            dimdone.push_back(true);
        }
        lastsize.push_back(m_LPRDimensions[d]);
    }

    m_OvvSize.clear();
    int exp = 1;
    double factor = static_cast<int>(std::pow(2, exp));
    bool balldone = false;
    while(!balldone)
    {
        balldone = true;
        std::vector<unsigned int> ovvsize;
        for (int d=0; d < this->GetNumberOfDimensions(); ++d)
        {
            unsigned int osize = m_LPRDimensions[d]/(int)factor;
            if (dimdone[d] || (d < 2 && osize < 256) || (d >= 2 && osize < 2) )
            {
                osize = lastsize[d];
                dimdone[d] = true;
            }
            else
            {
                lastsize[d] = osize;
                balldone = false;
            }
            ovvsize.push_back(osize);
        }

        // avoid adding the same
        // overview schedule twice!
        if (balldone)
        {
            break;
        }
        m_OvvSize.push_back(ovvsize);

        ++exp;
        factor = static_cast<int>(std::pow(2, exp));
    }

    // =======================================================================
    //              CALL HELPER CLASS FOR PYRAMID GENERATION
    // ========================================================================

    // define interpolation method
    std::string resampleType;
    if (method.compare("NEAREST") == 0)
    {
        resampleType = "NearestNeighbour";
    }
    else if (method.compare("AVERAGE") == 0)
    {
        resampleType = "Mean";
    }
    else if (method.compare("MODE") == 0)
    {
        resampleType = "Median";
    }
    else
    {
        NMDebug(<< "Requested resampling type '" <<
                method << "' is not available for netCDF!"
                << " Gonna use 'NEAREST' instead!");
        resampleType = "NearestNeighbour";
    }

    // create base name for ovv file
    m_OverviewContainerName = m_FileContainerName;

    //unsigned int numBands = this->GetNumberOfComponents();
    unsigned int numDimensions = this->GetNumberOfDimensions();

    NcGroup imgVarGrp = m_GroupIDs.back();

    std::string imgVarGrpName = "";
    for (int gn=0; gn < m_GroupNames.size(); ++gn)
    {
        imgVarGrpName += "/" + m_GroupNames.at(gn);
    }

    std::string imgVarName = m_NcVarName;
    std::string inImgSpec = this->GetFileName();

    netCDF::NcType ncType = m_ncType;

    otb::ImageIOBase* imgIO = this;

    switch (this->getOTBComponentType(ncType.getTypeClass()))
    {
        LocalMacroPerSingleType( NcGenerateOverviews )
        default:
            break;
    }
    m_NbOverviews = m_OvvSize.size();
}

std::vector<unsigned int>
NetCDFIO::GetOverviewSize(int ovv)
{
    std::vector<unsigned int> ret;
    if (ovv >=0 && ovv < this->m_NbOverviews)
    {
        ret = m_OvvSize[ovv];
    }

    return ret;
}

void NetCDFIO::SetZSliceIdx(int slindex)
{
    const int dim = static_cast<int>(this->GetDimensions(2));
    if (slindex >= 0 && slindex < dim)
    {
        this->m_ZSliceIdx = slindex;
    }
}

void NetCDFIO::SetOverviewIdx(int idx)
{
    if (idx >= -1 && idx < this->m_NbOverviews)
    {
        this->m_OverviewIdx = idx;
        this->updateOverviewInfo();
    }

}

void NetCDFIO::updateOverviewInfo()
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



void NetCDFIO::WriteImageInformation()
{
    this->InternalWriteImageInformation();
}

void NetCDFIO::InternalWriteImageInformation()
{
    if (!this->m_bCanWrite || !m_bImageInfoNeedsToBeWritten)
    {
        return;
    }

    // double check whether we've got all the data we
    // need to write proper image metadata ...
    const unsigned int ndims = this->GetNumberOfDimensions();
    if (     m_Origin.size() < ndims
         ||  m_Spacing.size() < ndims
       )
    {
        NMProcErr(<< "Failed accessing Origin and Spacing "
                   << "information for the image to be written!");
        return;
    }

    try
    {
        NcFile nc(this->m_FileContainerName, NcFile::write, NcFile::nc4);
        if (nc.isNull())
        {
            NMProcErr(<< "Failed writing image information to '"
                       << this->m_FileContainerName << "'!");
            return;
        }

        // creating the group structure
        NcGroup grp = nc.getId();
        for (int g=0; g < this->m_GroupNames.size(); ++g)
        {
            NcGroup curGrp = grp.getGroup(m_GroupNames.at(g));
            if (curGrp.isNull())
            {
                curGrp = grp.addGroup(m_GroupNames.at(g));
                if (!curGrp.isNull())
                {
                    this->m_GroupIDs.push_back(curGrp.getId());
                }
                else
                {
                    NMProcErr(<< "Failed creating group '" << m_GroupNames.at(g)
                               << "' in '" << this->m_FileContainerName << "'!");
                    return;
                }
            }
            else
            {
                this->m_GroupIDs.push_back(curGrp.getId());
            }
            grp = curGrp;
        }

        if (m_GroupIDs.empty())
        {
            m_GroupNames.clear();
            m_GroupIDs.push_back(nc.getId());
        }

        const NcType::ncType vtype = this->getNetCDFComponentType(
                    this->GetComponentType());


        // ========================================================
        //                 CHECK VARIABLE EXISTENCE
        // ========================================================
        NcVar valVar = grp.getVar(this->m_NcVarName);

        // check whether variable is an overview
        bool bIsOverview = false;
        size_t pos = m_NcVarName.find("_ovv_");
        int ovvLevel = -1;
        if (pos != std::string::npos)
        {
            bIsOverview = true;
            std::string strLevel = m_NcVarName.substr(pos+5, std::string::npos);
            try
            {
                ovvLevel = std::stoi(strLevel);
            }
            catch(const std::invalid_argument& iva)
            {
                NMWarn("nmNetCDFIO", << iva.what() << "! "
                       << "Failed converting '" << strLevel
                       << "' to overview level! Carry on as if this was "
                       << " a normal image!");
                bIsOverview = false;
            }
        }

        // ========================================================
        //                 CREATE VARIABLE + DIMENSIONS
        // ========================================================


        if (valVar.isNull())
        {
            // add the dimension
            std::vector<std::string> dimtemplates = {"x", "y", "z"};
            std::vector<NcDim> dims;
            for (int d = ndims-1; d >=0; --d)
            {
                unsigned long dsize = this->GetDimensions(d);
                if (m_UseForcedLPR)
                {
                    dsize = m_ForcedLPR.GetSize(d);
                }

                std::stringstream dname;
                if (bIsOverview)
                {
                    dname << "ovv" << ovvLevel << "d" << (d+1);
                }
                else
                {
                    if (d <= 2)
                    {
                        dname << dimtemplates.at(d);
                    }
                    else
                    {
                        dname << "d" << (d+1);
                    }
                }

                NcDim aDim = grp.getDim(dname.str());
                if (aDim.isNull())
                {
                    if (dsize > 1)
                    {
                        // add fixed dimension
                        aDim = grp.addDim(dname.str(), dsize);
                    }
                    else
                    {
                        // add unlimited dimension
                        aDim = grp.addDim(dname.str());
                    }
                }
                else
                {
                    if (    !aDim.isUnlimited()
                        &&  dsize != aDim.getSize()
                       )
                    {
                        NMProcErr(<< m_NcVarName << ": " << dname.str()
                                  << "-axis size incompatible with existing " << dname.str()
                                  << "-axis!");
                        return;
                    }
                 }

                dims.push_back(aDim);


                NcVar dimVar = grp.getVar(dname.str());
                if (dimVar.isNull())
                {
                    dimVar = grp.addVar(dname.str(), NcType::nc_DOUBLE, aDim);
                    dimVar.setCompression(true, true, m_CompressionLevel);

                    std::vector<double> dimVals(dsize, 0.0);
                    for (unsigned int dimIdx=0; dimIdx < dsize; ++dimIdx)
                    {
                        dimVals[dimIdx] = this->m_Origin[d] + dimIdx * this->m_Spacing[d];
                    }

                    std::vector<size_t> startp = {0};
                    std::vector<size_t> countp = {dsize};
                    dimVar.putVar(startp, countp, &dimVals[0]);
                }
                else
                {
                    NMProcWarn(<< m_NcVarName << " is re-using dimension variable '"
                               << dname.str() << "'!");
                }
             }

            // now add the actual variable we want to write
            valVar = grp.addVar(this->m_NcVarName, vtype, dims);
            valVar.setCompression(true, true, m_CompressionLevel);
            valVar.setFill(true, 0);
        }
        // ========================================================
        //                 ADJUST VAR's DIMENSION
        // ========================================================
        else
        {
            itk::ImageIORegion ioReg = this->GetIORegion();
            std::vector<NcDim> dims = valVar.getDims();
            for (int d=0; d < dims.size(); ++d)
            {
                if (dims[d].isUnlimited())
                {
                    // maps the netcdf dimension order : [..., [d4,]] z, y, x
                    // to the itk/otb dimension order:   x, y, z [, d4 [, ...]]
                    const unsigned int otbDimIdx = ndims - d - 1;

                    // determine the number of new dimension ids (and
                    // coordinate values) that need to be added for the
                    // chunk of data to be added to this variable
                    std::vector<size_t> ioidx = {static_cast<size_t>(ioReg.GetIndex(otbDimIdx))};
                    std::vector<size_t> iolen = {ioReg.GetSize(otbDimIdx)};
                    std::vector<size_t> lastRecIdx = {dims[d].getSize()-1};

                    size_t numNewIds = 0;
                    if (ioidx[0]+iolen[0]-1 > lastRecIdx[0])
                    {
                        numNewIds = (ioidx[0]+iolen[0]-1) - lastRecIdx[0];
                    }

                    if (numNewIds > 0)
                    {
                        // fetch the coordinate value corresponsing with the
                        // start point of the incoming chunk of data
                        std::vector<size_t> lastNumRec = {1};
                        std::vector<double> lastRec    = {0};

                        NcVar dvar = grp.getVar(dims[d].getName());
                        dvar.getVar(lastRecIdx, lastNumRec, &lastRec[0]);

                        // add new records' coordinates
                        //std::vector<size_t> startp = {ioReg.GetIndex(otbDimIdx)};
                        //std::vector<size_t> cnt    = {ioReg.GetSize(otbDimIdx)};
                        //std::vector<double> dvals(ioReg.GetSize(otbDimIdx), 0.0);
                        //std::vector<size_t> startp = {dims[d].getSize()};
                        //std::vector<size_t> cnt    = {this->GetDimensions(otbDimIdx)};
                        std::vector<double> dvals(numNewIds, 0.0);
                        for (unsigned int dIdx=0; dIdx < numNewIds; ++dIdx)
                        {
                            dvals[dIdx] = lastRec[0] + (dIdx + 1) * this->m_Spacing[otbDimIdx];
                        }
                        dvar.putVar(ioidx, iolen, &dvals[0]);
                    }
                }
            }
        }
    }
    catch(exceptions::NcException& e)
    {
        NMProcErr(<< e.what());
    }
}

void NetCDFIO::WriteRAT(otb::AttributeTable* tab,
        unsigned int band)
{
}

void NetCDFIO::Write(const void* buffer)
{
    // call "WriteImageInformation" when this function is called the first time
    // (necessary for stream writing)
    // this dirty hack is needed because the right component type is only
    // known when "ImageIO::Write" is called and not when
    // "ImageIO::WriteImageInformation" is called
    if (!m_bWasWriteCalled)
    {
        this->CanWriteFile(this->m_FileName.c_str());

        m_bImageInfoNeedsToBeWritten = true;
        this->InternalWriteImageInformation();
        m_bImageInfoNeedsToBeWritten = false;
        m_bWasWriteCalled = true;
    }

    // we have to check this again because the call to
    // 'WriteImageInformation' could yield  that we
    // actually can't write the image (e.g. because of
    // the image's number of dimensions
    if (!this->m_bCanWrite)
    {
        return;
    }

    try
    {
        NcFile nc(this->m_FileContainerName, NcFile::write);
        if (nc.isNull())
        {
            NMProcErr(<< "Failed opening file '"
                       << m_FileContainerName << "' for writing!");
            return;
        }

        const int grpId = m_GroupIDs.size() > 0 ? m_GroupIDs.back() : nc.getId();
        NcGroup grp(grpId);
        if (grp.isNull())
        {
            NMProcErr(<< "Failed accessing group '"
                       << grp.getName(true) << "' in file '"
                       << m_FileContainerName << "'!");
            return;
        }

        // format the start (index) and count (size) vectors
        itk::ImageIORegion ioReg = this->GetIORegion();

        std::vector<size_t> vStart;
        std::vector<size_t> vCount;
        for (int d = this->GetNumberOfDimensions()-1; d >= 0; --d)
        {
            vStart.push_back(ioReg.GetIndex()[d]);
            vCount.push_back(ioReg.GetSize()[d]);
        }

        NcVar valVar = grp.getVar(m_NcVarName);
        if (valVar.isNull())
        {
            NMProcErr(<< "Failed accessing the variable '"
                       << m_NcVarName << "'!");
            return;
        }

        const NcType::ncType vtype = this->getNetCDFComponentType(
            this->GetComponentType());
        void* buf = const_cast<void*>(buffer);

        switch (vtype)
        {
        case NcType::nc_BYTE:
            valVar.putVar(vStart, vCount, (char*)(buf));
            break;

        case NcType::nc_UBYTE:
            valVar.putVar(vStart, vCount, (unsigned char*)(buf));
            break;

        case NcType::nc_SHORT:
            valVar.putVar(vStart, vCount, (short*)(buf));
            break;

        case NcType::nc_USHORT:
            valVar.putVar(vStart, vCount, (unsigned short*)(buf));
            break;

        case NcType::nc_INT:
            valVar.putVar(vStart, vCount, (int*)(buf));
            break;

        case NcType::nc_UINT:
            valVar.putVar(vStart, vCount, (unsigned int*)(buf));
            break;

        case NcType::nc_UINT64:
            valVar.putVar(vStart, vCount, (unsigned long long*)(buf));
            break;

        case NcType::nc_INT64:
            valVar.putVar(vStart, vCount, (long long*)(buf));
            break;

        case NcType::nc_DOUBLE:
            valVar.putVar(vStart, vCount, (double*)(buf));
            break;

        case NcType::nc_FLOAT:
        default:
            valVar.putVar(vStart, vCount, static_cast<float*>(buf));
            break;
        }

        nc.sync();
    }
    catch(exceptions::NcException& e)
    {
        itkExceptionMacro(<< e.what());
    }
}

otb::AttributeTable::Pointer NetCDFIO::getRasterAttributeTable(int band)
{
    otb::AttributeTable::Pointer rat;
    return rat;
}

bool NetCDFIO::parseImageSpec(const std::string imagespec)
{
    if (m_bImageSpecParsed && imagespec.compare(m_FileName) == 0)
    {
        NMLogInfo(<< "We know already that we can read '" << imagespec << "'!");
        return true;
    }

    NMLogDebug(<< "user specified imagespec: " << imagespec << std::endl);

    // correct image specification string we are looking for:
    //      $ FilePathName:/[GrpName1/[GrpName2/[...]]VarName
    // The GrpNameX is optional and only required if the
    // image contains multiple groups; if ommitted, it is
    // assumed that the relevant image variable VarName
    // is defined in the root group
    //
    // note: ':' colons are not otherwise allowed in a filename spec
    //       regardless whether they're quoted or not

    if (imagespec.empty())
    {
        itkExceptionMacro(<< "empty imagespec!");
        return false;
    }

    m_GroupNames.clear();
    m_GroupIDs.clear();
    m_NcVarName = "";

    std::string partspec;
    std::string::size_type lastpos = std::string::npos;
    std::string::size_type pos = imagespec.rfind(":", lastpos);

#ifdef _WIN32
    if (pos >= 3)
#else
    if (pos != std::string::npos)
#endif
    {
        m_FileContainerName = imagespec.substr(0, pos);
        lastpos = pos;
        pos = imagespec.find("/", lastpos+1);

        // we've got the '/' directly following the ':', so
        // rather look for the next one, if we've actually
        // found one...
        if (pos != std::string::npos)
        {
            lastpos = pos;
            pos = imagespec.find("/", lastpos+1);
        }

        while (pos != std::string::npos)
        {
            partspec = imagespec.substr(lastpos+1, pos-lastpos-1);
            m_GroupNames.push_back(partspec);
            lastpos = pos;
            pos = imagespec.find("/", lastpos+1);
        }
        partspec = imagespec.substr(lastpos+1, imagespec.size()-lastpos-1);
        m_NcVarName = partspec;
    }
    else
    {
        m_FileContainerName = imagespec;
        std::string::size_type seppos = imagespec.find_last_of('/');
        std::string::size_type suffpos = imagespec.find(".nc");
        if (seppos != std::string::npos && suffpos != std::string::npos)
        {
            m_NcVarName = imagespec.substr(seppos + 1, suffpos - seppos - 1);
        }
    }

    m_bImageSpecParsed = true;

    return true;
}

void NetCDFIO::setRasterAttributeTable(AttributeTable* rat, int band)
{
    if (band < 1)
        return;

    if (this->m_vecRAT.capacity() < band)
        this->m_vecRAT.resize(band);

    this->m_vecRAT[band-1] = rat;
}

void NetCDFIO::PrintSelf(std::ostream& os, itk::Indent indent) const
{
    Superclass::PrintSelf(os, indent);
}

otb::ImageIOBase::IOComponentType NetCDFIO::getOTBComponentType(
        NcType::ncType nctype)
{
    otb::ImageIOBase::IOComponentType otbtype;
    switch (nctype)
    {
        case NcType::nc_UBYTE:
            otbtype = otb::ImageIOBase::UCHAR;
            break;
        case NcType::nc_BYTE:
            otbtype = otb::ImageIOBase::CHAR;
            break;
        case NcType::nc_UINT:
            otbtype = otb::ImageIOBase::UINT;
            break;
        case NcType::nc_INT:
            otbtype = otb::ImageIOBase::INT;
            break;
        case NcType::nc_USHORT:
            otbtype = otb::ImageIOBase::USHORT;
            break;
        case NcType::nc_SHORT:
            otbtype = otb::ImageIOBase::SHORT;
            break;
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
         case NcType::nc_UINT64:
            otbtype = otb::ImageIOBase::ULONGLONG;
            break;
        case NcType::nc_INT64:
            otbtype = otb::ImageIOBase::LONGLONG;
            break;
#else
        case NcType::nc_UINT64:
            otbtype = otb::ImageIOBase::ULONG;
            break;
        case NcType::nc_INT64:
            otbtype = otb::ImageIOBase::LONG;
            break;
#endif

        case NcType::nc_DOUBLE:
            otbtype = otb::ImageIOBase::DOUBLE;
            break;
        case NcType::nc_FLOAT:
            otbtype = otb::ImageIOBase::FLOAT;
            break;
        default:
            otbtype = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
            break;
    }
    return otbtype;
}

NcType::ncType
NetCDFIO::getNetCDFComponentType(otb::ImageIOBase::IOComponentType otbtype)
{
    NcType::ncType nctype;
    switch (otbtype)
    {
        case otb::ImageIOBase::UCHAR:
            nctype = NcType::nc_UBYTE;
            break;
        case otb::ImageIOBase::CHAR:
            nctype = NcType::nc_BYTE;
            break;
#ifdef _WIN32
        case otb::ImageIOBase::ULONG:
#endif
        case otb::ImageIOBase::UINT:
            nctype = NcType::nc_UINT;
            break;
#ifdef _WIN32
        case otb::ImageIOBase::LONG:
#endif
        case otb::ImageIOBase::INT:
            nctype = NcType::nc_INT;
            break;
        case otb::ImageIOBase::USHORT:
            nctype = NcType::nc_USHORT;
            break;
        case otb::ImageIOBase::SHORT:
            nctype = NcType::nc_SHORT;
            break;
#ifdef __linux__
        case otb::ImageIOBase::ULONG:
#endif
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
        case otb::ImageIOBase::ULONGLONG:
#endif
            nctype = NcType::nc_UINT64;
            break;
#ifdef __linux__
        case otb::ImageIOBase::LONG:
#endif
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
        case otb::ImageIOBase::LONGLONG:
#endif
            nctype = NcType::nc_INT64;
            break;
        case otb::ImageIOBase::DOUBLE:
            nctype = NcType::nc_DOUBLE;
            break;
        case otb::ImageIOBase::FLOAT:
            nctype = NcType::nc_FLOAT;
            break;
        default:
            nctype = NcType::nc_FLOAT;
            break;
    }
    return nctype;
}


}		// end of namespace otb
