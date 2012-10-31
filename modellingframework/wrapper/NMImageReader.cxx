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

#include "NMImageReader.h"
#include "otbGDALRATImageIO.h"
#include "otbGDALRATImageFileReader.h"
#include "otbImageFileReader.h"
#include "otbImageIOFactory.h"
#include "itkIndent.h"
#include "otbAttributeTable.h"
#include "otbVectorImage.h"
#include "otbImage.h"
#include "itkImageBase.h"
#include "itkImageIOBase.h"
#include "vtkImageImport.h"
#include "itkVTKImageExport.h"

#ifdef BUILD_RASSUPPORT
  #include "otbRasdamanImageIO.h"
  #include "otbRasdamanImageReader.h"

	/**
	 *  Helper class which instantiates the first part of the (ITK) image
	 *  pipeline using the appropriate PixelType
	 */
	template <class PixelType, unsigned int ImageDimension>
	class RasdamanReader
	{
	public:
		typedef otb::Image< PixelType, ImageDimension > ImgType;
		typedef otb::RasdamanImageReader< ImgType > 	ReaderType;
		typedef typename ReaderType::Pointer			ReaderTypePointer;
		typedef typename ImgType::PointType				ImgOriginType;
		typedef typename ImgType::SpacingType			ImgSpacingType;

		typedef otb::VectorImage< PixelType, ImageDimension > 		VecImgType;
		typedef otb::RasdamanImageReader< VecImgType > 				VecReaderType;
		typedef typename VecReaderType::Pointer						VecReaderTypePointer;


		static otb::AttributeTable::Pointer
			fetchRAT(itk::ProcessObject* procObj, int band,
					unsigned int numBands)
		{
			if (numBands == 1)
			{
				ReaderType *r = dynamic_cast<ReaderType*>(procObj);
				return r->getRasterAttributeTable(band);
			}
			else
			{
				VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
				return r->getRasterAttributeTable(band);
			}
		}

		static itk::DataObject *getOutput(itk::ProcessObject::Pointer &readerProcObj,
				unsigned int numBands)
		{
			itk::DataObject *img = 0;
			if (numBands == 1)
			{
				ReaderType *r = dynamic_cast<ReaderType*>(readerProcObj.GetPointer());
				//r->Update();

//				r->GenerateOutputInformation();
//				ImgType* i = r->GetOutput();
//				const ImgOriginType& ori = i->GetOrigin();
//				const ImgSpacingType& spc = i->GetSpacing();
//
//				stringstream o;
//				stringstream s;
//				for (unsigned int d=0; d<ImageDimension; ++d)
//				{
//					o << ori[d] << " ";
//					s << spc[d] << " ";
//				}
//				NMDebugAI(<< "read origin & spacing ... " << endl);
//				NMDebugAI(<< o.str() << " | " << s.str() << endl);

				img = r->GetOutput();
			}
			else
			{
				VecReaderType *vr = dynamic_cast<VecReaderType*>(readerProcObj.GetPointer());
				//r->Update();
				img = vr->GetOutput();
			}
			return img;
		}

		static void initReader(itk::ProcessObject::Pointer &readerProcObj,
				itk::ImageIOBase *imgIOBase, QString &imgName,
				unsigned int numBands)
		{
			NMDebugCtx("RasdamanReader", << "...");

			if (numBands == 1)
			{
				ReaderTypePointer reader = ReaderType::New();
				otb::RasdamanImageIO* rio = dynamic_cast<otb::RasdamanImageIO*>(imgIOBase);
				reader->SetImageIO(rio);
				reader->SetFileName(imgName.toStdString().c_str());

				// keep reference to objects
				readerProcObj = reader;
			}
			else
			{
				VecReaderTypePointer reader = VecReaderType::New();
				otb::RasdamanImageIO* rio = dynamic_cast<otb::RasdamanImageIO*>(imgIOBase);
				reader->SetImageIO(rio);
				reader->SetFileName(imgName.toStdString().c_str());

				// keep reference to objects
				readerProcObj = reader;
			}

			NMDebugCtx("RasdamanReader", << "done!");
		}
	};
#endif // BUILD_RASSUPPORT 

template <class PixelType, unsigned int ImageDimension>
class FileReader
{
public:
	typedef otb::Image< PixelType, ImageDimension > 	ImgType;
	typedef otb::GDALRATImageFileReader< ImgType > 		ReaderType;
	typedef typename ReaderType::Pointer				ReaderTypePointer;

	typedef otb::VectorImage< PixelType, ImageDimension > VecImgType;
	typedef otb::GDALRATImageFileReader< VecImgType > 	  VecReaderType;
	typedef typename VecReaderType::Pointer				  VecReaderTypePointer;


	static otb::AttributeTable::Pointer
		fetchRAT(itk::ProcessObject* procObj, int band,
				unsigned int numBands)
	{
		if (numBands == 1)
		{
			ReaderType *r = dynamic_cast<ReaderType*>(procObj);
			return r->GetAttributeTable(band);
		}
		else
		{
			VecReaderType *r = dynamic_cast<VecReaderType*>(procObj);
			return r->GetAttributeTable(band);
		}
	}

	static itk::DataObject *getOutput(itk::ProcessObject::Pointer &readerProcObj,
			unsigned int numBands)
	{
		itk::DataObject *img = 0;
		if (numBands == 1)
		{
			ReaderType *r = dynamic_cast<ReaderType*>(readerProcObj.GetPointer());
			//r->Update();
			img = r->GetOutput();
		}
		else
		{
			VecReaderType *vr = dynamic_cast<VecReaderType*>(readerProcObj.GetPointer());
			//r->Update();
			img = vr->GetOutput();
		}
		return img;
	}

	static void initReader(itk::ProcessObject::Pointer &readerProcObj,
			itk::ImageIOBase *imgIOBase, QString &imgName,
			unsigned int numBands)
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

#ifdef BUILD_RASSUPPORT
  /** Helper Macro to call either the Rasdaman or File Reader methods */
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
  					this->mOutputNumBands); \
  			break; \
  		case 3: \
  			RasdamanReader< PixelType, 3 >::initReader( \
  					this->mOtbProcess, \
  					this->mItkImgIOBase, \
  					this->mFileName, \
  					this->mOutputNumBands); \
  			break; \
  		default: \
  			RasdamanReader< PixelType, 2 >::initReader( \
  				this->mOtbProcess, \
  				this->mItkImgIOBase, \
  				this->mFileName, \
  				this->mOutputNumBands); \
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
  					this->mOutputNumBands); \
  			break; \
  		case 3: \
  			FileReader< PixelType, 3 >::initReader( \
  					this->mOtbProcess, \
  					this->mItkImgIOBase, \
  					this->mFileName, \
  					this->mOutputNumBands); \
  			break; \
  		default: \
  			FileReader< PixelType, 2 >::initReader( \
  				this->mOtbProcess, \
  				this->mItkImgIOBase, \
  				this->mFileName, \
  				this->mOutputNumBands); \
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
						this->mOtbProcess, this->mOutputNumBands); \
				break; \
			case 3: \
				img = RasdamanReader<PixelType, 3 >::getOutput( \
						this->mOtbProcess, this->mOutputNumBands); \
				break; \
			default: \
				img = RasdamanReader<PixelType, 2 >::getOutput( \
						this->mOtbProcess, this->mOutputNumBands); \
			}\
		} \
		else \
		{ \
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				img = FileReader<PixelType, 1 >::getOutput( \
						this->mOtbProcess, this->mOutputNumBands); \
				break; \
			case 3: \
				img = FileReader<PixelType, 3 >::getOutput( \
						this->mOtbProcess, this->mOutputNumBands); \
				break; \
			default: \
				img = FileReader<PixelType, 2 >::getOutput( \
						this->mOtbProcess, this->mOutputNumBands); \
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
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
				break; \
			case 3: \
				rat = RasdamanReader< PixelType, 3 >::fetchRAT( \
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
				break; \
			default: \
				rat = RasdamanReader< PixelType, 2 >::fetchRAT( \
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
			}\
		}\
		else \
			{ \
			switch (this->mOutputNumDimensions) \
			{ \
			case 1: \
				rat = FileReader< PixelType, 1 >::fetchRAT( \
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
				break; \
			case 3: \
				rat = FileReader< PixelType, 3 >::fetchRAT( \
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
				break; \
			default: \
				rat = FileReader< PixelType, 2 >::fetchRAT( \
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
			}\
		} \
	}
#else
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
						this->mOutputNumBands); \
				break; \
			case 3: \
				FileReader< PixelType, 3 >::initReader( \
						this->mOtbProcess, \
						this->mItkImgIOBase, \
						this->mFileName, \
						this->mOutputNumBands); \
				break; \
			default: \
				FileReader< PixelType, 2 >::initReader( \
					this->mOtbProcess, \
					this->mItkImgIOBase, \
					this->mFileName, \
					this->mOutputNumBands); \
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
						this->mOtbProcess, this->mOutputNumBands); \
				break; \
			case 3: \
				img = FileReader<PixelType, 3 >::getOutput( \
						this->mOtbProcess, this->mOutputNumBands); \
				break; \
			default: \
				img = FileReader<PixelType, 2 >::getOutput( \
						this->mOtbProcess, this->mOutputNumBands); \
			}\
		} \
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
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
				break; \
			case 3: \
				rat = FileReader< PixelType, 3 >::fetchRAT( \
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
				break; \
			default: \
				rat = FileReader< PixelType, 2 >::fetchRAT( \
						this->mOtbProcess.GetPointer(), band, this->mOutputNumBands ); \
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
	this->mInputComponentType = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mOutputComponentType = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mFileName = "";
	this->mbRasMode = false;
	this->mFilePos = 0;
#ifdef BUILD_RASSUPPORT	
	this->mRasconn = 0;
	this->mRasConnector = 0;
	this->mParameterHandling = NMProcess::NM_USE_UP;
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

otb::AttributeTable::Pointer NMImageReader::getRasterAttributeTable(int band)
{
	if (!this->mbIsInitialised || band < 1)
		return 0;

	otb::AttributeTable::Pointer rat;

	switch (this->mOutputComponentType)
	{
	case itk::ImageIOBase::UCHAR:
		CallFetchRATClassMacro( unsigned char );
		break;
	case itk::ImageIOBase::CHAR:
		CallFetchRATClassMacro( char );
		break;
	case itk::ImageIOBase::USHORT:
		CallFetchRATClassMacro( unsigned short );
		break;
	case itk::ImageIOBase::SHORT:
		CallFetchRATClassMacro( short );
		break;
	case itk::ImageIOBase::UINT:
		CallFetchRATClassMacro( unsigned int );
		break;
	case itk::ImageIOBase::INT:
		CallFetchRATClassMacro( int );
		break;
	case itk::ImageIOBase::ULONG:
		CallFetchRATClassMacro( unsigned long );
		break;
	case itk::ImageIOBase::LONG:
		CallFetchRATClassMacro( long );
		break;
	case itk::ImageIOBase::FLOAT:
		CallFetchRATClassMacro( float );
		break;
	case itk::ImageIOBase::DOUBLE:
		CallFetchRATClassMacro( double );
		break;
	default:
		NMErr(ctxNMImageReader, << "UNKNOWN DATA TYPE, couldn't fetch RAT!");
		break;
	}

	return rat;
}

//itk::ImageIOBase::IOComponentType NMImageReader::getITKComponentType()
//{
//	return this->mOutputComponentType;
//}

void NMImageReader::setFileName(QString filename)
{
	this->mFileName = filename;
//	if (!filename.contains("."))
//		this->mbRasMode = true;
//
//	NMDebugAI(<< "rasmode is: " << this->mbRasMode << endl);
}

QString NMImageReader::getFileName(void)
{
	return this->mFileName;
}

bool NMImageReader::initialise() throw (r_Error)
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
		NMErr(ctxNMImageReader, << "NO IMAGEIO WAS FOUND!");
		return false;
	}

	// Now that we found the appropriate ImageIO class, ask it to
	// read the meta data from the image file.
	this->mItkImgIOBase->SetFileName(this->mFileName.toStdString().c_str());
	if (!this->mItkImgIOBase->CanReadFile(this->mFileName.toStdString().c_str()))
	{
		NMErr(ctxNMImageReader, << "Failed reading '" << this->mFileName.toStdString() << "'!");
		NMDebugCtx(ctxNMImageReader, << "done!");
		return false;
	}

	NMDebugAI(<< "reading image information ..." << endl);
	this->mItkImgIOBase->ReadImageInformation();
	this->mOutputNumBands = this->mItkImgIOBase->GetNumberOfComponents();

	if (this->mOutputComponentType == itk::ImageIOBase::UNKNOWNCOMPONENTTYPE)
		this->mOutputComponentType = this->mItkImgIOBase->GetComponentType();

	this->mOutputNumDimensions = this->mItkImgIOBase->GetNumberOfDimensions();
	this->mInputNumBands = this->mItkImgIOBase->GetNumberOfComponents();

	if (this->mInputComponentType == itk::ImageIOBase::UNKNOWNCOMPONENTTYPE)
		this->mInputComponentType = this->mItkImgIOBase->GetComponentType();

	this->mInputNumDimensions = this->mItkImgIOBase->GetNumberOfDimensions();

	NMDebugAI(<< "... numBands: " << this->mOutputNumBands << endl);
	NMDebugAI(<< "... comp type: " << this->mOutputComponentType << endl);

	bool ret = true;
	switch (this->mOutputComponentType)
	{
	case itk::ImageIOBase::UCHAR:
		CallReaderMacro( unsigned char );
		break;
	case itk::ImageIOBase::CHAR:
		CallReaderMacro( char );
		break;
	case itk::ImageIOBase::USHORT:
		CallReaderMacro( unsigned short );
		break;
	case itk::ImageIOBase::SHORT:
		CallReaderMacro( short );
		break;
	case itk::ImageIOBase::UINT:
		CallReaderMacro( unsigned int );
		break;
	case itk::ImageIOBase::INT:
		CallReaderMacro( int );
		break;
	case itk::ImageIOBase::ULONG:
		CallReaderMacro( unsigned long );
		break;
	case itk::ImageIOBase::LONG:
		CallReaderMacro( long );
		break;
	case itk::ImageIOBase::FLOAT:
		CallReaderMacro( float );
		break;
	case itk::ImageIOBase::DOUBLE:
		CallReaderMacro( double );
		break;
	default:
		NMErr(ctxNMImageReader, << "UNKNOWN DATA TYPE, couldn't create Pipeline!");
		ret = false;
		break;
	}

	NMDebugCtx(ctxNMImageReader, << "done!");

	this->mbIsInitialised = ret;
	return ret;
}

NMItkDataObjectWrapper* NMImageReader::getOutput(void)
{
	itk::DataObject *img = 0;
	switch(this->mOutputComponentType)
	{
	case itk::ImageIOBase::UCHAR:
		RequestReaderOutput( unsigned char );
		break;
	case itk::ImageIOBase::CHAR:
		RequestReaderOutput( char );
		break;
	case itk::ImageIOBase::USHORT:
		RequestReaderOutput( unsigned short );
		break;
	case itk::ImageIOBase::SHORT:
		RequestReaderOutput( short );
		break;
	case itk::ImageIOBase::UINT:
		RequestReaderOutput( unsigned int );
		break;
	case itk::ImageIOBase::INT:
		RequestReaderOutput( int );
		break;
	case itk::ImageIOBase::ULONG:
		RequestReaderOutput( unsigned long );
		break;
	case itk::ImageIOBase::LONG:
		RequestReaderOutput( long );
		break;
	case itk::ImageIOBase::FLOAT:
		RequestReaderOutput( float );
		break;
	case itk::ImageIOBase::DOUBLE:
		RequestReaderOutput( double );
		break;
	default:
		NMErr(ctxNMImageReader, << "UNKNOWN DATA TYPE, couldn't get output!");
		break;
	}

	NMItkDataObjectWrapper* dw = new NMItkDataObjectWrapper(this, img, this->mOutputComponentType,
			this->mOutputNumDimensions, this->mOutputNumBands);

	return dw;
}

itk::DataObject* NMImageReader::getItkImage(void)
{
	itk::DataObject *img = 0;
	switch(this->mOutputComponentType)
	{
	case itk::ImageIOBase::UCHAR:
		RequestReaderOutput( unsigned char );
		break;
	case itk::ImageIOBase::CHAR:
		RequestReaderOutput( char );
		break;
	case itk::ImageIOBase::USHORT:
		RequestReaderOutput( unsigned short );
		break;
	case itk::ImageIOBase::SHORT:
		RequestReaderOutput( short );
		break;
	case itk::ImageIOBase::UINT:
		RequestReaderOutput( unsigned int );
		break;
	case itk::ImageIOBase::INT:
		RequestReaderOutput( int );
		break;
	case itk::ImageIOBase::ULONG:
		RequestReaderOutput( unsigned long );
		break;
	case itk::ImageIOBase::LONG:
		RequestReaderOutput( long );
		break;
	case itk::ImageIOBase::FLOAT:
		RequestReaderOutput( float );
		break;
	case itk::ImageIOBase::DOUBLE:
		RequestReaderOutput( double );
		break;
	default:
		NMErr(ctxNMImageReader, << "UNKNOWN DATA TYPE, couldn't get output!");
		break;
	}
	return img;
}

const itk::ImageIOBase* NMImageReader::getImageIOBase(void)
{
	if (this->mbIsInitialised)
		return this->mItkImgIOBase;
	else
		return 0;
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

//void NMImageReader::linkInPipeline(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
//{
//	// we deliberately don't call the base class implementation here
//	// 'cauz this reader does not have any image type input
//
//	if (this->mParameterPos < this->mFileNames.size())
//	{
//		if (this->mFileName.compare(this->mFileNames.at(this->mParameterPos)) != 0)
//		{
//			this->setFileName(this->mFileNames.at(this->mParameterPos));
//			if (this->mRasConnector != 0)
//			{
//				this->setRasdamanConnector(
//						const_cast<RasdamanConnector*>(
//								this->mRasConnector->getConnector()));
//			}
//			this->initialise();
//		}
//	}
//
//	this->linkParameters(step, repo);
//}

void
NMImageReader::linkParameters(unsigned int step,
		const QMap<QString, NMModelComponent*>& repo)
{
	// set the step parameter according to the ParameterHandling mode set for this process
	switch(this->mParameterHandling)
	{
	case NM_USE_UP:
		if (this->mFilePos < this->mFileNames.size())
		{
			step = this->mFilePos;
			++this->mFilePos;
		}
		else if (this->mFilePos >= this->mFileNames.size())
		{
			this->mFilePos = this->mFileNames.size()-1;
			step = this->mFilePos;
		}
		break;
	case NM_CYCLE:
		if (this->mFilePos < this->mFileNames.size())
		{
			step = this->mFilePos;
			++this->mFilePos;
		}
		else if (this->mFilePos >= this->mFileNames.size())
		{
			step = 0;
			this->mFilePos = 1;
		}
		break;
	case NM_SYNC_WITH_HOST:
		if (step < this->mInputComponents.size())
		{
			this->mFilePos = step;
		}
		else
		{
			step = 0;
			this->mFilePos = 0;
			NMErr(ctxNMProcess, << "mFilePos and host's step out of sync!! Set mFilePos = 0");
		}
		break;
	}


	if (this->mFileNames.size() > 0)
	{
		if (this->mFileName.compare(this->mFileNames.at(step)) != 0)
		{
			this->setFileName(this->mFileNames.at(step));
			this->initialise();
		}
	}
}

void NMImageReader::setNthInput(unsigned int numInput,
		NMItkDataObjectWrapper* img)
{
	// don't need file input for the reader
}

void NMImageReader::instantiateObject(void)
{
	// grab properties and feed member vars for successful initialisation
#ifdef BUILD_RASSUPPORT
	if (this->mRasConnector != 0)
	{
		RasdamanConnector* rasconn = const_cast<RasdamanConnector*>(
				this->mRasConnector->getConnector());
		if (rasconn != 0)
			this->setRasdamanConnector(rasconn);
	}
#endif	

	if (this->getFileNames().size() > 0)
		this->setFileName(this->getFileNames().at(0));
	this->initialise();
}

