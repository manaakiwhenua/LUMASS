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
	name: otbRasdamanImageIO.cxx
	author:
	date:
	purpose: read and write rasdaman image files

*/

// TODO: to be removed from final version
#include "nmlog.h"
#define __rio "RImageIO"
// TODO: end removal

// postgresql
#include "libpq-fe.h"

#include <iostream>
#include <sstream>
#include <limits>
#include "otbRasdamanImageIO.h"
#include "itksys/SystemTools.hxx"
#include "itkExceptionObject.h"
#include "otbMacro.h"
#include "itkMacro.h"
#include "otbSystem.h"
#include "itkMetaDataObject.h"
#include "otbMetaDataKey.h"

// to account for the different table names
// of the rasdaman feature branch (as 2013-10)
#define PSPREFIX "ps"

namespace otb
{

/// ----------------------------------------- CONSTRUCTOR / DESTRUCTOR
RasdamanImageIO::RasdamanImageIO(void)
{
	// default number of dimensions is two
	this->SetNumberOfDimensions(2);
	// currently only scalars are supported
	m_PixelType = SCALAR;
	// reasonable default pixel type
	m_ComponentType = UCHAR;
	m_rtype = r_Type::CHAR;

	// default spacing (i.e. pixel size)
	m_Spacing[0] = 1.0;
	m_Spacing[1] = 1.0;

        m_UpperLeftCorner.resize(this->GetNumberOfDimensions());

        // default origin is (0.5,0.5)
        m_Origin[0] = 0.5;
        m_Origin[1] = 0.5;

	// we don't have any info about the image so far ...
	m_ImageSpec = "";
	m_prevImageSpec = "";
	m_collname = "";

	m_bCanRead = false;
	m_bCanWrite = false;
	m_bCollImageAvail = false;
	m_bWasWriteCalled = false;
	m_bImageInfoNeedsToBeWritten = false;
	m_bUpdateImage = true;
//	m_bCollectionsAsVector = false;
	this->m_CollectionTypeName = "";
	this->m_ImageTypeName = "";

	m_UseForcedLPR = false;
	m_ImageUpdateMode = false;

	// the only supported extension is none ''
	this->AddSupportedReadExtension("");
	this->AddSupportedWriteExtension("");

	this->m_Helper = 0;
	this->m_Rasconn = 0;
}

RasdamanImageIO::~RasdamanImageIO()
{
	if (this->m_Helper != 0)
		delete this->m_Helper;
	this->m_Helper = 0;
	this->m_Rasconn = 0;
}

// --------------------------- PUBLIC METHODS

void RasdamanImageIO::SetFileName(const char* filename)
{
	//NMDebugCtx(__rio, << "...");
	if (this->m_bCanRead || this->m_bCanWrite)
	{
		//NMDebugAI(<< "file name is set and we've checked accessibility already!"
		//		<< std::endl);
		//NMDebugCtx(__rio, << "done!");
		return;
	}

	if (this->parseImageSpec(filename))
	{
		this->m_FileName = filename;
		//NMDebugAI(<< "file name set to: " << filename << std::endl);
	}
	else
		this->m_FileName = "";
	//NMDebugCtx(__rio, << "done!");
}

void
RasdamanImageIO::SetForcedLPR(const itk::ImageIORegion& forcedLPR)
{
	this->m_ForcedLPR = forcedLPR;
	this->m_UseForcedLPR = true;
}

double
RasdamanImageIO::getOIDFromCollIndex(void)
{
	// in case we've specified a position/index within the collection rather than
	// an explicit oid, retrieve the corresponding oid and set it
	double theoid = -1;
	std::vector<double> availoids = this->m_Helper->getImageOIDs(this->m_collname);
	if ((!this->m_collstrindex.empty() || this->m_collnumindex >=0) &&
			availoids.size() > 0)
	{
		if (this->m_collstrindex == "first")
			theoid = availoids.at(0);
		else if (this->m_collstrindex == "last")
			theoid = availoids.at(availoids.size()-1);
		else if (this->m_collnumindex >=0 && this->m_collnumindex < availoids.size())
			theoid = availoids.at(this->m_collnumindex);
	}

	return theoid;
}

bool RasdamanImageIO::CanReadFile(const char* filename)
{
	//NMDebugCtx(__rio, << "...");

	// first we check, whether we can access petascope metadata
	if (!this->m_Rasconn->getPetaConnection())
	{
		NMErr(__rio, << "No connection to '" <<
				this->m_Rasconn->getPetaDbName() << "'!");
		this->m_bCanRead = false;
		return false;
	}

	// correct image specification string we are looking for:
	// collection_name:local_oid
	this->m_prevImageSpec = this->m_ImageSpec;
	this->m_ImageSpec = filename;

	// skip checking, if we've already checked the image
	// (only meaningful with stream reading, but in that case
	// it helps reducing db queries)
	if ((this->m_prevImageSpec == this->m_ImageSpec) &&
		 this->m_ImageSpec.find("_last_") == string::npos &&  this->m_bCanRead)
	{
		//NMDebugAI(<< "checked '" << filename << "' already and we believe "
		//		  << "we can read it!" << std::endl);
		//NMDebugCtx(__rio, << "done!");
		return true;
	}


	// prepare exception
	itk::ExceptionObject eo(__FILE__, __LINE__);


	if (this->m_oids.size() == 0)
	{
		// in case we've specified a position/index within the collection rather than
		// an explicit oid, retrieve the corresponding oid and set it
		double theoid = this->getOIDFromCollIndex();
		if (theoid >= 0)
			this->m_oids.push_back(theoid);
		else
			this->m_oids = this->m_Helper->getImageOIDs(this->m_collname);

		if (this->m_oids.size() == 1)
		{
			r_Minterval sdom = this->m_Helper->getImageSdom(this->m_collname,
					this->m_oids[0]);
			if (sdom.dimension() < 1 || sdom.dimension() > 3)
			{
				this->m_bCanRead = false;
				eo.SetDescription("Check image dimensions! RasdamanImageIO only supports 2D and 3D images!");
				throw eo;
			}
			else
				this->m_bCanRead = true;
		}
		else if (this->m_oids.size() > 1)
		{
			// check whether the individual images are flat
			// or composite: we only create vector images from
			// flat (i.e. single component) pixel types
			if (this->m_Helper->getBaseTypeElementCount(this->m_collname) == 1)
			{
				this->m_bCanRead = true;
			}
			else
			{
				this->m_bCanRead = false;
				eo.SetDescription("RasdamanImageIO does not support collapsing an "
						"image collection of images with composite pixel type!");
				throw eo;
			}
		}
		else
		{
			this->m_bCanRead = false;
			eo.SetDescription("RasdamanImageIO couldn't find any image in the "
					"given collection!");
			throw eo;
		}
	}
	// test for single band image by checking spatial domain
	else
	{
		r_Minterval sdom = this->m_Helper->getImageSdom(this->m_collname,
				this->m_oids[0]);
		if (sdom.dimension() < 1 || sdom.dimension() > 3)
		{
			this->m_bCanRead = false;
			eo.SetDescription("Check image dimensions! RasdamanImageIO only supports 2D and 3D images!");
			throw eo;
		}
		else
			this->m_bCanRead = true;
	}

	//NMDebugCtx(__rio, << "done!");
	return this->m_bCanRead;
}

void RasdamanImageIO::ReadImageInformation()
{
	//NMDebugCtx(__rio, << "...");

	if (!this->CanReadFile(this->GetFileName()))
		return;

	// in any case, we take all relevant parameters from the
	// first image we've got in the oids list

	// read spatial attributes:
	// spatial domain, cell size, pixel type
	m_sdom = this->m_Helper->getImageSdom(
			this->m_collname, this->m_oids[0]);
	this->SetNumberOfDimensions(m_sdom.dimension());

	std::vector<double> cellsize = this->m_Helper->getMetaCellSize(
			this->m_oids[0]);
	std::vector<double> geodom = this->m_Helper->getMetaGeoDomain(
			this->m_oids[0]);

	this->m_rtype = this->m_Helper->getBaseTypeId(this->m_collname);

        m_UpperLeftCorner.resize(m_sdom.dimension());

	int step=0;
	for (int d=0; d < m_sdom.dimension(); d++)
	{
		this->SetDimensions(d, m_sdom[d].get_extent()); // num columns and rows

		if (d == 1)
		{
                        m_UpperLeftCorner[d] = geodom[step+1];
                        this->SetOrigin(d, geodom[step+1] + 0.5 * cellsize[d]);  // real world origin
			this->SetSpacing(d, cellsize[d]);// * -1);
		}
		else
		{
			this->SetSpacing(d, cellsize[d]);			  // real world cellsize
                        this->SetOrigin(d, geodom[step] + 0.5 * cellsize[d]);
                        m_UpperLeftCorner[d] = geodom[step];
		}
		step += 2;
	}

	if (this->m_oids.size() > 1)
		this->SetNumberOfComponents(this->m_oids.size());
	else
		this->SetNumberOfComponents(this->m_Helper->getBaseTypeElementCount(this->m_collname));

	if (this->GetNumberOfComponents() > 1)
		this->SetPixelType(otb::ImageIOBase::VECTOR);

	this->SetFileTypeToBinary();
	this->SetComponentType(this->getOTBComponentType(this->m_rtype));

	//if (!this->m_Helper->isNMMetaAvailable())
	//{
	//	//NMDebugAI(<< "no geospatial meta data available!" << std::endl);
	//	//NMDebugCtx(__rio, << "done!");
	//}

	// --------------------------------------------------------------
	// set meta data
	itk::MetaDataDictionary& dict = this->GetMetaDataDictionary();

	// crs
	std::string crs_descr = this->m_Helper->getMetaCrsName(this->m_oids[0]);
	itk::EncapsulateMetaData< std::string > (dict, MetaDataKey::ProjectionRefKey, crs_descr);

	// LowerLeft
	MetaDataKey::VectorType corner;
	corner.push_back(geodom[0]); // xmin
	corner.push_back(geodom[2]); // ymin
	itk::EncapsulateMetaData< MetaDataKey::VectorType > (
			dict, MetaDataKey::LowerLeftCornerKey, corner);

	// UpperLeft
	corner.clear();
	corner.push_back(geodom[0]); // xmin
	corner.push_back(geodom[3]); // ymax
	itk::EncapsulateMetaData< MetaDataKey::VectorType > (
			dict, MetaDataKey::UpperLeftCornerKey, corner);

	// UpperRight
	corner.clear();
	corner.push_back(geodom[1]); // xmax
	corner.push_back(geodom[3]); // ymax
	itk::EncapsulateMetaData< MetaDataKey::VectorType > (
			dict, MetaDataKey::UpperRightCornerKey, corner);

	// LowerRight
	corner.clear();
	corner.push_back(geodom[1]); // xmax
	corner.push_back(geodom[2]); // ymin
	itk::EncapsulateMetaData< MetaDataKey::VectorType > (
			dict, MetaDataKey::LowerRightCornerKey, corner);

	// GeoTransform
	MetaDataKey::VectorType geotrans;
	geotrans.push_back(geodom[0]);    // 0: xmin
	geotrans.push_back(cellsize[0]);  // 1: cellsize x
	geotrans.push_back(0);            // 2: rotation x
	geotrans.push_back(geodom[3]);    // 3: ymax
	geotrans.push_back(0);			  // 4: rotation y
	geotrans.push_back(cellsize[1] * -1);  // 5: cellsize y
	itk::EncapsulateMetaData< MetaDataKey::VectorType > (
			dict, MetaDataKey::GeoTransformKey, geotrans);

	//NMDebugCtx(__rio, << "done!");
}

void RasdamanImageIO::Read(void* buffer)
{
	//NMDebugCtx(__rio, << "...");

	// since we can have negative pixel indices with rasdaman
	// images, we have to map the indices onto each other
	// to ensure we get the region we want
	// TODO: check whether this is really and issue!

	unsigned int ndim = this->GetNumberOfDimensions();
	r_Minterval readsdom = r_Minterval(ndim);

	long bandbufsize = 1;
	long numReadPix, firstPix, lastPix;
	unsigned int compsize = this->GetComponentSize();
	unsigned int numcomps = this->GetNumberOfComponents();
	unsigned int ndpixsize = compsize * numcomps;
	for (int d=0; d < ndim; d++)
	{
		numReadPix = this->GetIORegion().GetSize()[d];
		firstPix = m_sdom[d].low() + this->GetIORegion().GetIndex()[d];
		lastPix = firstPix + (numReadPix > 0 ? numReadPix - 1 : numReadPix);

		//NMDebugAI(<< "read dim #" << d << "\n");
		//NMDebugAI(<< "  numReadPix   = " << numReadPix << std::endl);
		//NMDebugAI(<< "  firstPix = " << firstPix << std::endl);
		//NMDebugAI(<< "  lastPix   = " << lastPix << std::endl);

		readsdom << r_Sinterval((r_Range)firstPix, (r_Range)lastPix);

		if (d == 0)
		{
			bandbufsize = ndpixsize * numReadPix;
		}
		else
			bandbufsize *= numReadPix;
	}
	//NMDebugAI(<< "size of band buffer:  " << bandbufsize << " (" << bandbufsize / (1024.0*1024.0) << " MiB)" << std::endl);
	//NMDebugAI(<< "size of image buffer: " << (bandbufsize * this->GetNumberOfComponents())
	//		<< " (" << (bandbufsize * this->GetNumberOfComponents()) / (1024.0*1024.0) << " MiB)" << std::endl);

	char* otbbuf = static_cast<char*>(buffer);
	char* bandbuf = new char[bandbufsize];

	unsigned int nlayers = ndim == 3 ? readsdom[2].get_extent() : 1;

	if (bandbuf > 0)
	{
		if (this->m_oids.size() == 1)
		{
			if ( numcomps == 1)
			{
				//NMDebugAI(<< "processing single band image ... " << endl);
			}
			else
			{
				//NMDebugAI(<< "processing multi-band band image ... " << endl);
			}

			this->m_Helper->getImageBuffer(this->m_collname, this->m_oids[0],
					bandbuf, readsdom);

			// until we switch to full nD support, we stick with the faster 3D restricted version
			//			this->m_Helper->colBuf2RowBuf(bandbuf, otbbuf, compsize, readsdom);
			this->m_Helper->colBuf2RowBuf(bandbuf, otbbuf, ndpixsize, numcomps,
					readsdom[0].get_extent(), readsdom[1].get_extent(), nlayers);
		}
		else
		{
			//NMDebugAI(<< "collapsing collection into multi-band image ... " << endl);
//			std::vector<int> rmidx;
//			std::vector<int> cmidx;
//			rmidx.resize(ndim);
//			cmidx.resize(ndim);

			// calc number of pixels and read dimensions
//			std::vector<int> cmdims;
//			cmdims.resize(ndim);
//			int npix = 1;
//			for (int d=0; d < ndim; ++d)
//			{
//				cmdims[d] = readsdom[d].get_extent();
//				npix *= readsdom[d].get_extent();
//			}
//			NMDebugAI(<< "number of pixel per band: " << npix <<  endl);

			// swap first two dimensions to swap from row-major to column-major
//			cmdims[0] = readsdom[1].get_extent();
//			cmdims[1] = readsdom[0].get_extent();
//
//			int rmpix, cmpixoffset, rmpixoffset;
			int ncols = readsdom[0].get_extent();
			int nrows = readsdom[1].get_extent();

			unsigned int compoff = 0;

			// iterate over bands and process them individually
			for (int b=0; b < this->m_oids.size(); b++)
			{
				this->m_Helper->getImageBuffer(this->m_collname, this->m_oids[b],
						bandbuf, readsdom);

				//NMDebugInd(nmlog::nmindent+1, << "processing image/band #" << b+1 << " oid: "
				//		<< this->m_oids[b] << " ... " << endl);

				// until we switch to full nD support, we stick with the faster 3D restricted version
				int row, col, layer;
				for (row = 0; row < nrows; ++row)
				{
					for(col = 0; col < ncols; ++col)
					{
						for (layer = 0; layer < nlayers; ++layer)
						{
							memcpy((void*)(otbbuf + (row * ncols + col + layer*ncols*nrows) * ndpixsize + compoff),
							       (const void*)(bandbuf + (col * nrows + row + layer*ncols*nrows) * compsize), compsize);
						}
					}
				}

//				// calc column-major and row-major buffer offsets and write
//				// band info to multi-band buffer
//				for (int pix=0; pix < npix; ++pix)
//				{
//					cmidx = this->m_Helper->offset2index(pix, cmdims);
//					rmidx = cmidx;
//					rmidx[0] = cmidx[1];
//					rmidx[1] = cmidx[0];
//					rmpix = this->m_Helper->index2offset(readsdom, rmidx);
//
//					rmpixoffset = rmpix * ndpixsize + compoff;
//					cmpixoffset = pix * compsize;
//
//					memcpy((void*)(otbbuf + rmpixoffset),
//						   (const void*)(bandbuf + cmpixoffset), compsize);
//
//				}

				//NMDebugInd(nmlog::nmindent+1,<< "done!" << endl);

				// increment the component offset by one componentsize unit
				compoff += compsize;
			}
		}
		// release memory
		delete bandbuf;
	}
	bandbuf = 0;


	//NMDebugCtx(__rio, << "done!");
}

bool RasdamanImageIO::CanWriteFile(const char* filename)
{
	//NMDebugCtx(__rio, << "...");

	// first we check, whether we can access petascope metadata
	if (!this->m_Rasconn->getPetaConnection())
	{
		NMErr(__rio, << "No connection to '" <<
				this->m_Rasconn->getPetaDbName() << "'!");
		return false;
	}


	if (this->m_FileName.empty())
	{
		//NMDebugAI(<< "no valid filename has been specified yet!" << endl);
		//NMDebugCtx(__rio, << "done!");
		return false;
	}

	//NMDebugAI(<< "checking for data set '" << filename << "' ..." << endl);

	// skip checking, if we've already checked the image
	// (only meaningful with stream writing
	if ((this->m_prevImageSpec == this->m_ImageSpec) && this->m_bCanWrite)
	{
		//NMDebugAI(<< "we know already we can!" << std::endl);
		return true;
	}

	if (!this->m_Helper->checkDbConnection())
	{
		this->m_bCanWrite = false;
		//NMDebugAI(<< "no data base connection!" << std::endl);
		return false;
	}

	// when we're in update mode, we have to check, whether the specified
	// image is available in the db at all!
	if (this->m_ImageUpdateMode)
	{
		double theoid = this->getOIDFromCollIndex();
		if (theoid == -1)
		{
			this->m_bCanWrite = false;
			//NMDebugAI(<< "specified local OID invalid!" << std::endl);
			return false;
		}
		this->m_oids.clear();
		this->m_oids.push_back(theoid);
		this->m_bUpdateImage = true;
	}

	//TODO: check whether we've got enough info to check for the dimensionality
	// of the image (-> restrict to 3D)

	this->m_bCanWrite = true;
	//NMDebugCtx(__rio, << "done!");
	return this->m_bCanWrite;
}

void RasdamanImageIO::setRasdamanTypeNames(string colltypename, string imagetypename)
{
	this->m_CollectionTypeName = colltypename;
	this->m_ImageTypeName = imagetypename;
}


double
RasdamanImageIO::insertForcedLPRDummyImage()//const std::string& collname, r_Minterval& sdom)
{
	//NMDebugCtx(__rio, << "...");

	double oid = -1;

	r_Minterval adom(2);
	r_Point ashift(2);

	r_Minterval fdom(2);
	r_Point fshift(2);
	for (int d=0; d < this->m_NumberOfDimensions; ++d)
	{
		adom << r_Sinterval((r_Range)0,(r_Range)0);
		ashift << (r_Long)0;
		fdom << r_Sinterval((r_Range)0, (r_Range)0);
		fshift << (r_Long)(this->m_ForcedLPR.GetSize(d)-1);
	}

	//NMDebugAI(<< "let's have a dummy image with a forced LPR ..." << endl);

	int pixelsize = this->GetComponentSize() * this->GetNumberOfComponents();
	r_Ref<r_GMarray> dimg;
	dimg = new (this->m_ImageTypeName.c_str()) r_GMarray(fdom, pixelsize);

	//NMDebugAI(<< "let's have the first initial pixel here ... " << endl);
	//NMDebugAI(<< "shift: " << ashift.get_string_representation() << endl);
	//NMDebugAI(<< "sdom:  " << adom.get_string_representation() << endl << endl);


	oid = this->m_Helper->insertImage(this->m_collname, 0,
			ashift, adom, true, this->m_ImageTypeName, "");

	dimg->r_deactivate();

	//NMDebugAI(<< "... the second pixel, to build-up the desired LPR, we put here ... " << endl);
	//NMDebugAI(<< "shift: " << fshift.get_string_representation() << endl);
	//NMDebugAI(<< "sdom:  " << fdom.get_string_representation() << endl << endl);

	this->m_Helper->updateImage(this->m_collname, oid, (char*)dimg.get_memory_ptr(),
			fshift, fdom, true, this->m_ImageTypeName);




//	double oid = -1;
//	r_Transaction ta;
//
//	// get type information about the collection
//	r_Type::r_Type_Id tid = this->m_Helper->getBaseTypeId(collname);
//	NMDebugAI(<< "collection's pixel base type: " << this->m_Helper->getDataTypeString(tid) << endl);
//
//	// get the number of elements (in case we've got a struct type)
//	unsigned int nelem = this->m_Helper->getBaseTypeElementCount(collname);
//
//	// get a list of oids available prior to inserting the new image
//	//std::vector<double> preoids = this->m_Helper->getImageOIDs(collname);
//
//	// format the spatial domain string
//	std::string qstr = "insert into $1 values marray x in ";
//	std::stringstream sdomstr;
//	sdomstr << sdom.get_string_representation();
//	//for (int d = 0; d < sdom.dimension(); d++)
//	//{
//	//	sdomstr << shift[d] << ":" << shift[d];
//	//	if (d == sdom.dimension()-1)
//	//		sdomstr << "]";
//	//	else
//	//		sdomstr << ",";
//	//}
//	qstr += sdomstr.str();
//
//	// format the values string depending on the nubmer of elements
//	// of the base type
//	string numconst = this->m_Helper->getNumConstChar(tid);
//	NMDebugAI(<< "numeric constant is: " << numconst << endl);
//	if (nelem == 1)
//	{
//		qstr += " values 0" + numconst;
//	}
//	else // struct type
//	{
//		qstr += " values {";
//		for (int e=0; e < nelem; ++e)
//		{
//			qstr += "0" + numconst;
//			if (e < nelem -1)
//				qstr += ", ";
//			else
//				qstr += "}";
//		}
//	}
//
//	NMDebugAI( << "dummy grid query: " << qstr << std::endl);
//
//	try
//	{
//		ta.begin(r_Transaction::read_write);
//		r_OQL_Query qins(qstr.c_str());
//		qins << collname.c_str();
//		r_oql_execute(qins);
//		ta.commit();
//
//		vector<double> oids = this->m_Helper->getImageOIDs(collname);
//		oid = oids.at(oids.size()-1);
//	}
//	catch(r_Error& re)
//	{
//		oid = -1;
//		ta.abort();
//		throw re;
//	}

	//NMDebugCtx(__rio, << "done!");
	return oid;
}

void RasdamanImageIO::WriteImageInformation()
{
	//NMDebugCtx(__rio, << "...");

	// we only write infos as soon we've got enough information
	// (which is effectively when write() is called for the fist time)
	if (!m_bImageInfoNeedsToBeWritten)
	{
		//NMDebugAI( << "apparently, we've done that already ..." << endl);
		//NMDebugCtx(__rio, << "done!");
		return;
	}


	// get some basic info about the image
	r_Type::r_Type_Id rtype = this->getRasdamanComponentType(this->GetComponentType());
	int nelem = this->GetNumberOfComponents();
	int ndim = this->GetNumberOfDimensions();

	if (ndim > 3)
	{
		this->m_bCanWrite = false;
		//NMDebugCtx(__rio, << "done!");
		return;
	}

	bool bAsCube = ndim == 3 ? true : false;

	// check whether target collection already exists
	if (this->m_Helper->doesCollectionExist(this->m_collname) == -1)
	{
		//NMDebugAI(<< "creating collection '" << this->m_collname << "' ... " << endl);

		// if we've got composite pixel type-based image, use the user specified collection
		// type
		if (!this->m_CollectionTypeName.empty())
		{
			this->m_Helper->insertUserCollection(this->m_collname, this->m_CollectionTypeName);
			//NMDebugAI(<< "collection '" << this->m_collname << "' created with type '" <<
			//		this->m_CollectionTypeName << "'" << endl);
		}
		else
		{
			this->m_Helper->insertCollection(this->m_collname, rtype, bAsCube);
			//NMDebugAI(<< "collection '" << this->m_collname << "' created with type '" <<
			//		this->m_Helper->getDataTypeString(rtype) << "'" << endl);
		}
	}
	else
	{
		//NMDebugAI(<< "found collection '" << this->m_collname << "' ... with "
		//		<< this->m_Helper->getBaseTypeElementCount(this->m_collname)
		//		<< " x " << this->m_Helper->getDataTypeString(rtype) << " pixels ... " << endl);
	}
	//NMDebug(<< endl);

	// -------------------------------
	// if we're inserting a new image into a collection,
	// create an initial image we're going to update in the
	// Write function
	if (this->m_oids.size() == 0)
	{
		this->m_bUpdateImage = false;
		r_Point shift = r_Point(this->m_NumberOfDimensions);
		r_Minterval sdom = r_Minterval(this->m_NumberOfDimensions);
		for (int d=0; d < this->m_NumberOfDimensions; ++d)
		{
			shift << this->GetIORegion().GetIndex()[d];
			sdom << r_Sinterval((r_Range)0,(r_Range)0);
		}

		//NMDebugAI(<< "creating image ... " << endl);
		double oid = -1;

		//NMDebugAI(<< "initial spatial image configuration ..." << endl);
		//NMDebugAI(<< "shift: " << shift.get_string_representation() << endl);
		if (this->m_UseForcedLPR)
		{
			oid = this->insertForcedLPRDummyImage();
		}
		else
		{
			//NMDebugAI(<< "sdom:  " << sdom.get_string_representation() << endl << endl);

			oid = this->m_Helper->insertImage(this->m_collname, 0, shift, sdom, true,
					this->m_ImageTypeName, "");
		}

		if (oid != -1)
		{
			//NMDebugAI(<< "image #" << oid << " inserted into '" << this->m_collname << "'" << endl);
			this->m_oids.push_back(oid);
		}
		else
		{
			NMErr(__rio, << "failed creating image(s) for collection '" << this->m_collname
					<< "'!");
			//NMDebugCtx(__rio, << "done!");
			this->m_bCanWrite = false;
			return;
		}


	}

	// ----------------------------------------------------------------------
	// determine the geo-shift, in case we update an already existing image
	// input geospatial domain parameters
        double minx = this->GetOrigin(0) - 0.5 * this->GetSpacing(0);
        double maxy = this->GetOrigin(1) - 0.5 * this->GetSpacing(1);
        double minz = ndim == 3 ? this->GetOrigin(2) * this->GetSpacing(2) : numeric_limits<double>::max() * -1;
	double csx = this->GetSpacing(0);
	double csy = this->GetSpacing(1);
	double csz = ndim == 3 ? this->GetSpacing(2) : 0;
	int xpix = this->m_Dimensions[0];
	int ypix = this->m_Dimensions[1];
	int zpix = ndim == 3 ? this->m_Dimensions[2] : 0;

	if (!this->m_bUpdateImage && this->m_UseForcedLPR)
	{
		xpix = this->m_ForcedLPR.GetSize(0);
		ypix = this->m_ForcedLPR.GetSize(1);
		if (ndim == 3)
			zpix = this->m_ForcedLPR.GetSize(2);
	}

	double maxx = minx + csx * xpix;
	double miny = maxy + csy * ypix;
	double maxz = ndim == 3 ? minz + csz * zpix : numeric_limits<double>::max();

	// get the current image domain geospatial info
	vector<double> curdom = this->m_Helper->getMetaGeoDomain(this->m_oids[0]);
	vector<double> cellsize = this->m_Helper->getMetaCellSize(this->m_oids[0]);

	// calc the new geospatial domain
	vector<double> newdom(6);

	// union the input domain and the available domain
	newdom[0] = minx < curdom[0] ? minx : curdom[0];
	newdom[1] = maxx > curdom[1] ? maxx : curdom[1];
	newdom[2] = miny < curdom[2] ? miny : curdom[2];
	newdom[3] = maxy > curdom[3] ? maxy : curdom[3];
	if (this->GetNumberOfDimensions() == 3)
	{
		newdom[4] = minz < curdom[4] ? minz : curdom[4];
		newdom[5] = maxz > curdom[5] ? maxz : curdom[5];
	}

	// calculate the shift vector for the input image
	r_Point shift = r_Point(this->GetNumberOfDimensions());
	if (this->m_bUpdateImage)
	{
		//NMDebugAI(<< "calculating shift vector for writing image piece into rasdaman ..." << endl);
		double xshift = (newdom[0] - curdom[0]) / csx;
		xshift = xshift > 0 ? xshift + 0.5 : xshift - 0.5;

		double yshift = (newdom[3] - curdom[3]) / csy;
		yshift = yshift > 0 ? yshift + 0.5 : yshift - 0.5;

		shift <<  (r_Long)xshift << (r_Long)yshift;
		if (this->GetNumberOfDimensions() == 3 && cellsize[2] > 0)
		{
			double zshift = (newdom[4] - curdom[4]) / csz;
			zshift = zshift > 0 ? zshift + 0.5 : zshift - 0.5;
			shift << (r_Long)zshift;
		}
	}
	else
	{
		//NMDebugAI(<< "no shift required, we're inserting a new image ..." << endl);
		shift << 0 << 0;
		if (this->GetNumberOfDimensions() == 3)
			shift << 0;
	}

	this->m_GeoUpdateShift = r_Point(this->GetNumberOfDimensions());
	this->m_GeoUpdateShift = shift;

	// -----------------------------------------------------------
	// write metadata

	itk::MetaDataDictionary& dict = this->GetMetaDataDictionary();
	string crsname;
	itk::ExposeMetaData<std::string>(dict, MetaDataKey::ProjectionRefKey,
			crsname);
	std::vector<std::string> crs;
	const int ndims = this->GetNumberOfDimensions();
	if (crsname.empty())
	{
		switch(ndims)
		{
		case 1:
			crsname = "%SECORE_URL%/crs/OGC/0/Unknown1D";
			break;
		case 3:
			crsname = "%SECORE_URL%/crs/OGC/0/Unknown3D";
			break;
		default:
			crsname = "%SECORE_URL%/crs/OGC/0/Unknown2D";
			break;
		}
	}

	crs.push_back(crsname);

	string collname = this->m_collname;
	long oid = this->m_oids[0];
	long epsgcode = -1;
	string pixeltype = this->m_ImageTypeName;
	if (this->m_ImageTypeName.empty())
		pixeltype = this->GetComponentTypeAsString(
				this->GetComponentType());


	// otb::ImageIOBase returns type strings concatenated with '_'
	// so we get rid of it ...
	string::size_type pos = 0;
	while( (pos = pixeltype.find("_", pos)) != string::npos)
	{
		pixeltype.replace(pos, 1, " ");
	}

	double stats_min = -1;
	double stats_max = -1;
	double stats_mean = -1;
	double stats_stddev = -1;
	string RATName = "";

	//this->m_Helper->writeNMMetadata(collname, oid, epsgcode, crsname,
	//		minx, maxx, miny, maxy, minz, maxz, csx, csy, csz, pixeltype,
	//		stats_min, stats_max, stats_mean, stats_stddev, RATName);

	std::string covname = "";

	// we do the standard stuff for now ...
	std::vector<bool> axisIndexed;
	std::vector<int> crs_order;
	for (int d=0; d < ndims; ++d)
	{
		axisIndexed.push_back(false);
		crs_order.push_back(d);
	}

	// note: RasdamanHelper2::writePSMetadata expects
	// cellsizes to be positive!
	csy = csy < 0 ? csy * -1 : csy;
	this->m_Helper->writePSMetadata(
			oid,
			collname,
			covname,
			crs,
			crs_order,
			pixeltype,
			minx,
			maxx,
			miny,
			maxy,
			minz,
			maxz,
			csx, csy, csz,
			true,
			-1,
			-1,
			axisIndexed);

	// write RAT
	// ToDo: this needs checking, whether oid = bands here?
	for (unsigned int ti = 0; ti < this->m_vecRAT.size(); ++ti)
	{
		otb::AttributeTable::Pointer tab = this->m_vecRAT.at(ti);
		if (tab.IsNull() || ti > this->m_oids.size()-1)
			continue;

		// for now, we assume ti=band
		this->writeRAT(tab, ti+1, this->m_oids[ti]);
	}

	//NMDebugCtx(__rio, << "done!");
}

void RasdamanImageIO::writeRAT(otb::AttributeTable* tab,
		unsigned int band, double _oid)
{
	//NMDebugCtx(__rio, << "...");

	// DEBUG DEBUG DEBUG
	// let's have a look how the table looks like ....
	//tab->Print(cout, itk::Indent(nmlog::nmindent), 100);

	long oid = _oid;

	const PGconn* conn = this->m_Rasconn->getRasConnection();
	if (conn == 0)
	{
		NMErr(__rio, << "connection with '"
				<< this->m_Rasconn->getRasDbName() << "' failed!");
		return;
	}

	// check, whether the nm_meta is installed at all
	//if (!this->m_Helper->isNMMetaAvailable())
	//	return;

	// -------------------------------------------------------------

	// build table name
	std::stringstream tablename;
	tablename << "rat" << band << "_" << oid;


	std::stringstream query;
	query.precision(14);
	PGresult* res;

	// check, whether the table already exists, if yes, delete it
	query << "select * from " << tablename.str();
	res = PQexec(const_cast<PGconn*>(conn), query.str().c_str());
	if (PQntuples(res) >= 1)
	{
		PQclear(res);
		query.str("");
		query << "drop table " << tablename.str();
		res = PQexec(const_cast<PGconn*>(conn), query.str().c_str());
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			NMErr(__rio, << "failed deleting present table "<< tablename.str()
					<< "! Abort!");
			PQclear(res);
			return;
		}
	}
	PQclear(res);
	query.str("");


	// analyse the table structure
	int ncols = tab->GetNumCols();
	int nrows = tab->GetNumRows();

	//std::vector< std::string > colnames;
	std::vector< AttributeTable::TableColumnType > coltypes;

    // go and check the column names against the SQL standard, in case
    // they don't match it, we enclose in double quotes.
    //for (int c=0; c < ncols; ++c)
    //{
    //	bool quote = false;
    //	std::string name = tab->GetColumnName(c);
    //	if (name == "rowidx")
    //		continue;
    //	for (int l=0; l < name.size(); ++l)
    //	{
    //		if (l == 0)
    //    	{
    //    		if (!isalpha(name[l]) && name[l] != '_')
    //    		{
    //    			quote = true;
    //    			break;
    //    		}
    //    	}
    //    	else
    //    	{
    //    		if (!isalnum(name[l]) && name[l] != '_')
    //    		{
    //    			quote = true;
    //    			break;
    //    		}
    //    	}
    //	}
    //
    //	std::stringstream checkedstr;
    //	if (quote)
    //	{
    //		checkedstr << '\"' << name << '\"';
    //	}
    //	else
    //	{
    //		checkedstr << name;
    //	}
    //	colnames.push_back(checkedstr.str());
    //}



	// the first field is going to store the row index 'rowidx', which starts
	// at 0 and is incremented for each further row; this is necessary
	// to support indexed raster layers which refer to attributes by
	// their rowindex of the associated attribute table (e.g. ERDAS IMAGINE files *.img)

	string k = ", ";
	string s = " ";

	query << "create table " << tablename.str() << " ("
			<< "rowidx integer unique NOT NULL,";

	unsigned int bSkipIdx = -1;
	int c_orig, c_target, r;
	for (c_target=0, c_orig=0; c_orig < ncols; ++c_orig)
	{
		// TODO: check whether that's always a smart thing to do
		// if, for whatever reason, we've got already a row-index,
		// we drop this one, and use instead the new one
		if (tab->GetColumnName(c_orig) == "rowidx")
		{
			bSkipIdx = c_orig;
			continue;
		}
    	std::string tabcolname = tab->GetColumnName(c_orig);
    	char* colname = PQescapeIdentifier(const_cast<PGconn*>(conn),
    			tabcolname.c_str(), tabcolname.size());

		coltypes.push_back(tab->GetColumnType(c_orig));

		string typestr = "";
		switch (coltypes[c_target])
		{
			case AttributeTable::ATTYPE_INT:
				typestr = "integer";
				break;
			case AttributeTable::ATTYPE_DOUBLE:
				typestr = "double precision";
				break;
			case AttributeTable::ATTYPE_STRING:
				typestr = "text";
				break;
		}

		query << colname << s << typestr << k;
		++c_target;

		PQfreemem((void*)colname);
	}

	query << "constraint " << tablename.str() << "_pkey primary key (rowidx))";

	NMDebugInd(1, << "'" << query.str() << "' ... ");
	res = PQexec(const_cast<PGconn*>(conn), query.str().c_str());
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		NMErr(__rio, "creating raster attribute table for image '" <<
					oid << "' failed: " << endl << PQresultErrorMessage(res));

		PQclear(res);
		return;
	}
	//NMDebug(<< "done!" << endl);
	query.str("");
	PQclear(res);

	//NMDebugInd(1, << "copying table content ... ");
	// copy the table body into the postgres table
	for (r=0; r < nrows; r++)
	{
		query << "insert into " << tablename.str() << " values (" << r << k;
		for (c_orig=0, c_target=0; c_orig < ncols; ++c_orig)
		{
			// skip the index column
			if (bSkipIdx == c_orig)
				continue;

			switch (coltypes[c_target])
			{
			case AttributeTable::ATTYPE_INT:
				query << tab->GetIntValue(c_orig, r);
				break;
			case AttributeTable::ATTYPE_DOUBLE:
				{
					double val = tab->GetDblValue(c_orig, r);
					// NaN test complying with IEEE floating-point arithmetic
					if (val != val)
					{
						query << "\'NaN\'::numeric";
					}
					else
					{
						query << val;
					}
				}
				break;

			case AttributeTable::ATTYPE_STRING:
				string val = tab->GetStrValue(c_orig, r);
				//if (val.empty())
				//	query << "NULL";
				//else
				{
					char* litstr = PQescapeLiteral(const_cast<PGconn*>(conn),
								val.c_str(), val.size());
					query << litstr;
					PQfreemem((void*)litstr);
				}

				break;
			}

			if (c_orig < ncols -1)
				query << k;
			else
				query << ")";

			++c_target;
		}

		res = PQexec(const_cast<PGconn*>(conn), query.str().c_str());
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			//NMDebug(<< endl);
			NMErr(__rio, << "failed copying row " << r << "/" << nrows << " for table "
					     << tablename.str() << "': " << endl << PQresultErrorMessage(res));
		}

		query.str("");
		PQclear(res);
	}

	NMDebug(<< std::endl);
	// --------------------------------------------------------------

	// write a reference to the table into the ps database
	std::string metadata = "attrtable_name=" + tablename.str();
	if (!this->m_Helper->writeExtraMetadata(oid, metadata))
	{
		NMWarn(__rio, << "failed writing RAT name '" << tablename.str()
				<< "' into '" << this->m_Rasconn->getRasDbName() << "'!");
	}

	//// write the name of this table into the nm_meta table
	//query << "update nm_meta set attrtable_name = 'nmrat_" << oid << "' " <<
	//		 "where img_id = " << oid;
	//NMDebugInd(1, << "'" << query.str() << "' ... ");
	//res = PQexec(const_cast<PGconn*>(conn), query.str().c_str());
	//if (PQresultStatus(res) != PGRES_COMMAND_OK)
	//{
	//	//NMDebug(<< std::endl);
	//	NMErr(__rio, << "failed writing RAT name 'nmrat_" << oid
	//			<< "' into nm_meta table: " << endl << PQresultErrorMessage(res));
	//	PQclear(res);
	//	return;
	//}
	//NMDebug(<< "done!" << endl);

	// free result structure
	//PQclear(res);

	//NMDebugCtx(__rio, << "done!");
}

void RasdamanImageIO::Write(const void* buffer)
{
	//NMDebugCtx(__rio, << "...");

	// call "WriteImageInformation" when this function is called the first time
	// (necessary for stream writing)
	// this dirty hack is needed because the right component type is only
	// known when "ImageIO::Write" is called and not when
	// "ImageIO::WriteImageInformation" is called
	if (!m_bWasWriteCalled)
	{
		this->CanWriteFile(this->m_collname.c_str());
		m_bImageInfoNeedsToBeWritten = true;
		this->WriteImageInformation();
		m_bImageInfoNeedsToBeWritten = false;
		m_bWasWriteCalled = true;
	}

	// we have to check this again because the call to
	// 'WriteImageInformation' could yield  that we
	// actually can't write the image (e.g. because of
	// the image's number of dimensions
	if (!this->m_bCanWrite)
	{
		//NMDebugAI(<< "we've probably had a problem writing the image's information, " <<
		//		"so we abort here!" << endl);
		//NMDebugCtx(__rio, << "done!");
		return;
	}

	//NMDebugAI(<< "IORegion ..." << std::endl);
	//itk::ImageIORegion ioRegion = this->GetIORegion();
	//ioRegion.Print(std::cout, itk::Indent(4));

	r_Minterval sdom = r_Minterval(this->GetNumberOfDimensions());
	r_Point seqShift = r_Point(this->GetNumberOfDimensions());
	for (int d=0; d < this->GetNumberOfDimensions(); ++d)
	{
		sdom << r_Sinterval((r_Range)0, (r_Range)this->GetIORegion().GetSize()[d]-1);
		seqShift << this->GetIORegion().GetIndex()[d];
	}

	r_Point totalShift = this->m_GeoUpdateShift + seqShift;


	//NMDebugAI(<< "spatial image update configuration ..." << endl);
	//NMDebugAI(<< "shift: " << totalShift.get_string_representation() << endl);
	//NMDebugAI(<< "sdom:  " << sdom.get_string_representation() << endl << endl);

	// write otb buffer into rasdaman db
	void* otbtmp = const_cast<void*>(buffer);
	char* otbbuf = (char*)otbtmp;

	this->m_Helper->updateImage(this->m_collname, this->m_oids[0], otbbuf,
			totalShift, sdom, true, this->m_ImageTypeName);


	//NMDebugCtx(__rio, << "done!");
}

otb::AttributeTable::Pointer RasdamanImageIO::getRasterAttributeTable(int band)
{
	//NMDebugCtx(__rio, << "...");
	// check band parameter
	if (band < 1 || band > this->GetNumberOfComponents() ||
			band > this->m_oids.size())
	{
		//NMDebugCtx(__rio, << "done!");
		return 0;
	}

	// check, whether we've got rasgeo support (a nm_meta table) at all
	//if (!this->m_Helper->isNMMetaAvailable())
	//{
	//	//NMDebugAI(<< "there doesn't seem to be nm_meta support "
	//	//		  << "for this rasdaman data base!" << endl);
	//	//NMDebugCtx(__rio, << "done!");
	//	return 0;
	//}


	// get and check connection to the data base
	const PGconn* conn = this->m_Rasconn->getPetaConnection();
	if (conn == 0)
	{
		NMErr(__rio, << "No connection to petascope data base!");
		//NMDebugCtx(__rio, << "done!");
		return 0;
	}

	// ------------------- query table name ---------------------------
	std::stringstream query;
	PGresult* res;

	query << "select value from " << PSPREFIX << "_extra_metadata as em "
	             "inner join " << PSPREFIX << "_extra_metadata_type as mt "
	             "on mt.id = em.metadata_type_id "
			        "where mt.type = 'attrtable_name' "
			        "and em.coverage_id = "
			"(select coverage_id from " << PSPREFIX << "_range_set as rs "
			     "inner join " << PSPREFIX << "_rasdaman_collection as rc "
			     "on rs.storage_id = rc.id "
			         "where rc.oid = " << this->m_oids[band-1] << ")";
	NMDebug( << query.str() << std::endl);
	res = PQexec(const_cast<PGconn*>(conn), query.str().c_str());
	if (PQntuples(res) < 1)
	{
		NMDebugAI(<< "this band hasn't got an associated attribute table!" << endl);
		//NMDebugCtx(__rio, << "done!");
		return 0;
	}
	std::string ratName = PQgetvalue(res, 0, 0);
	PQclear(res);


	//-------------------- check, whether table exists  ----------------------
	// before, we connect to the rasdaman database since the table is physically stored there!
	conn = this->m_Rasconn->getRasConnection();
	if (conn == 0)
	{
		NMErr(__rio, << "No connection to rasdaman data base!");
		//NMDebugCtx(__rio, << "done!");
		return 0;
	}

	// CREDITS TO michaelb on 'http://bytes.com/topic/postgresql/answers/692471-how-get-column-names-table'
	query.str("");
	query << "SELECT a.attname as \"Column\", " <<
			 "  pg_catalog.format_type(a.atttypid, a.atttypmod) as \"Datatype\" " <<
			 "FROM pg_catalog.pg_attribute a " <<
			 "WHERE a.attnum > 0 AND NOT a.attisdropped AND a.attrelid = (" <<
	         "   SELECT c.oid " <<
	         "   FROM pg_catalog.pg_class c " <<
	         "        LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace" <<
	         "   WHERE c.relname ~ '^(" << ratName << ")$' " <<
	         "         AND pg_catalog.pg_table_is_visible(c.oid))";

	res = PQexec(const_cast<PGconn*>(conn), query.str().c_str());
	int ntuples = PQntuples(res);
	if (ntuples < 1)
	{
		//NMDebugAI(<< "There's no table for band #"
		//		<< band << " !" << std::endl);
		//NMDebugCtx(__rio, << "done!");
		return 0;
	}

	//NMDebugAI(<< "reading table structure info ..." << endl);
	std::vector<std::string> names(ntuples);
	std::vector<std::string> types(ntuples);
	for (int r=0; r < ntuples; ++r)
	{
		names[r] = PQgetvalue(res, r, 0);
		types[r] = PQgetvalue(res, r, 1);
		//NMDebugAI(<< " ... field '" << names[r] << "' - type: '" << types[r] << "'" << endl);
	}
	PQclear(res);

	// create local table and init structure
	otb::AttributeTable::Pointer rat = otb::AttributeTable::New();
	rat->SetBandNumber(band);
	// well, we just set the name of the attribute table here,
	// since that's we need later for updating the table in
	// the data base, rather than the coll_name:oid in
	// case of rasdaman images, also we can easily retrieve
	// the coverage/image name this table belongs to
	rat->SetImgFileName(ratName);

	//NMDebugAI(<< "cloning table structure ..." << endl);
	for (int c=0; c < names.size(); ++c)
	{
		if (types[c] == "integer")
		{
			rat->AddColumn(names[c], otb::AttributeTable::ATTYPE_INT);
			//NMDebugAI(<< " ... added '" << names[c] << "' (integer)" << endl);
		}
		else if (types[c] == "double precision")
		{
			rat->AddColumn(names[c], otb::AttributeTable::ATTYPE_DOUBLE);
			//NMDebugAI(<< " ... added '" << names[c] << "' (double)" << endl);
		}
		else // "text"
		{
			rat->AddColumn(names[c], otb::AttributeTable::ATTYPE_STRING);
			//NMDebugAI(<< " ... added '" << names[c] << "' (string)" << endl);
		}
	}

	// ------------------ query and copy table ----------------------------------------
	//NMDebugAI(<< "querying full table ... " << endl);
	query.str("");
	query << "SELECT * from " << ratName;
	res = PQexec(const_cast<PGconn*>(conn), query.str().c_str());
	ntuples = PQntuples(res);

	//NMDebugAI(<< "cloning table contents ..." << endl);
	rat->AddRows(ntuples);
	char tmpstr[256];
	for (int r=0; r < ntuples; ++r)
	{
		for (int c=0; c < names.size(); ++c)
		{
			switch (rat->GetColumnType(c))
			{
			case otb::AttributeTable::ATTYPE_INT:
				rat->SetValue(c, r, ::strtol(PQgetvalue(res, r, c),0,10));
				break;
			case otb::AttributeTable::ATTYPE_DOUBLE:
				rat->SetValue(c, r, ::strtod(PQgetvalue(res, r, c),0));
				break;
			case otb::AttributeTable::ATTYPE_STRING:
			default:
				rat->SetValue(c, r, PQgetvalue(res, r, c));
				break;
			}
		}
	}
	PQclear(res);

	//NMDebugCtx(__rio, << "done!");
	return rat;
}

bool RasdamanImageIO::parseImageSpec(const std::string imagespec)
{
	//NMDebugCtx(__rio, << "...");

	//NMDebugAI(<< "user specified imagespec: " << imagespec << std::endl);

	// correct image specification string we are looking for:
	// collection_name[:local_OID]
	// note: local_OID is optional; if it is omitted it is assumed
	// that we're dealing with a multiband image
	this->m_ImageSpec = imagespec;
	if (imagespec.empty())
	{
		itkExceptionMacro(<< "empty imagespec!");
		//NMDebugAI(<< "imagespec empty: " << imagespec << std::endl);
		//NMDebugCtx(__rio, << "done!");
		return false;
	}

	this->m_collstrindex = "";
	this->m_collnumindex = -1;
	this->m_oids.clear();
	std::string::size_type pos = 0;
	pos = imagespec.find(":", 0);

	if (imagespec.find("=", 0) != std::string::npos)				// in case we got a metadata-based query
	{
		std::string thename;
		std::vector<double> _oids = this->m_Helper->queryImageOIDs(imagespec);
		if (_oids.size() > 0)
			thename = this->m_Helper->getCollectionNameFromOID(_oids[0]);

		if (!thename.empty())
			this->m_collname = thename;

		this->m_oids.push_back(_oids[0]);
	}
	else if (pos == string::npos)			// in case we got just the collection name
	{
		this->m_collname = imagespec;
	}
	else									// in case we got a collection name and oid/index spec
	{
		this->m_collname = imagespec.substr(0, pos);

		// get the local oid
		std::string oidstr = imagespec.substr(pos+1,
				imagespec.size()-pos+1);
		if (oidstr.at(0) == '_' && oidstr.at(oidstr.size()-1) == '_')
		{
			std::string stridx = oidstr.substr(1, oidstr.size()-2);
			if (stridx == "first" || stridx == "last")
				this->m_collstrindex = stridx;
			else
				this->m_collnumindex = ::atol(stridx.c_str());

			//NMDebugAI(<< "image #" << oidstr << " of collection '"
			//		<< this->m_collname << "' specified as input!" << std::endl);
		}
		else
		{
			this->m_oids.push_back(::atol(oidstr.c_str()));
			//NMDebugAI(<< "image #" << this->m_oids[0] << " of collection '"
			//		<< this->m_collname << "' specified as input!" << std::endl);
		}
	}

	//NMDebugCtx(__rio, << "done!");
	return true;
}

void RasdamanImageIO::setRasterAttributeTable(AttributeTable* rat, int band)
{
	if (band < 1)
		return;

	if (this->m_vecRAT.capacity() < band)
		this->m_vecRAT.resize(band);

	this->m_vecRAT[band-1] = rat;
}

void RasdamanImageIO::setRasdamanConnector(RasdamanConnector* rasconn)
{
	if (this->m_Helper != 0 )
		delete this->m_Helper;

	this->m_Helper = new RasdamanHelper2(rasconn);
	this->m_Rasconn = rasconn;
}

void RasdamanImageIO::PrintSelf(std::ostream& os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
}

otb::ImageIOBase::IOComponentType RasdamanImageIO::getOTBComponentType(
		r_Type::r_Type_Id rtype)
{
	otb::ImageIOBase::IOComponentType otbtype;
	switch (rtype)
	{
		case r_Type::CHAR:
		case r_Type::BOOL:
			otbtype = otb::ImageIOBase::UCHAR;
			break;
		case r_Type::ULONG:
			//otbtype = otb::ImageIOBase::ULONG;
			otbtype = otb::ImageIOBase::UINT;
			break;
	  case r_Type::USHORT:
			otbtype = otb::ImageIOBase::USHORT;
			break;
		case r_Type::LONG:
			//otbtype = otb::ImageIOBase::LONG;
			otbtype = otb::ImageIOBase::INT;
			break;
	  case r_Type::SHORT:
			otbtype = otb::ImageIOBase::SHORT;
			break;
	  case r_Type::OCTET:
		  otbtype = otb::ImageIOBase::CHAR;
			break;
	  case r_Type::DOUBLE:
		  otbtype = otb::ImageIOBase::DOUBLE;
			break;
	  case r_Type::FLOAT:
			otbtype = otb::ImageIOBase::FLOAT;
			break;
		default:
			otbtype = otbtype = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
			break;
	}
	return otbtype;
}

r_Type::r_Type_Id
RasdamanImageIO::getRasdamanComponentType(otb::ImageIOBase::IOComponentType otbtype)
{
	r_Type::r_Type_Id rtype;

	switch (otbtype)
	{
	case otb::ImageIOBase::UCHAR:
		rtype = r_Type::CHAR;
		break;
	case otb::ImageIOBase::UINT:
	case otb::ImageIOBase::ULONG:
		rtype = r_Type::ULONG;
		break;
	case otb::ImageIOBase::USHORT:
		rtype = r_Type::USHORT;
		break;
	case otb::ImageIOBase::INT:
	case otb::ImageIOBase::LONG:
		rtype = r_Type::LONG;
		break;
	case otb::ImageIOBase::SHORT:
		rtype = r_Type::SHORT;
		break;
	case otb::ImageIOBase::CHAR:
		rtype = r_Type::OCTET;
		break;
	case otb::ImageIOBase::DOUBLE:
		rtype = r_Type::DOUBLE;
		break;
	case otb::ImageIOBase::FLOAT:
		rtype = r_Type::FLOAT;
		break;
	default:
		rtype = r_Type::UNKNOWNTYPE;
		break;
	}

	return rtype;
}


}		// end of namespace otb
