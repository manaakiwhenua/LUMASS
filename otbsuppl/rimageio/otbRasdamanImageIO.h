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
	name: otbRasdamanImageIO.h
	author:
	date: 
	version: 0.5.0
	
	purpose: read and write rasdaman image files
	
		infos: - default connection parameters are set at construction time
						 !! important: currently user and password are set only here !!
					 - specific connection details are provided via the filename 
					   parameter when calling the "CanRead" method
*/

#ifndef __otbRasdamanImageIO_h
#define __otbRasdamanImageIO_h

/// RASDAMAN includes
#ifdef EARLY_TEMPLATE
#define __EXECUTABLE__
#ifdef __GNUG__
#include "raslib/template_inst.hh"
#include "template_rimageio_inst.hh"
#endif
#endif

#include "otbAttributeTable.h"
#include "otbImageIOBase.h"
#include "RasdamanHelper2.hh"
#include "RasdamanConnector.hh"
#include "rasdaman.hh"


namespace otb
{

class ITK_EXPORT RasdamanImageIO : public ImageIOBase
{
public:

	typedef RasdamanImageIO Self;
	typedef otb::ImageIOBase Superclass;
	typedef itk::SmartPointer<Self> Pointer;
	typedef Superclass::ByteOrder ByteOrder;
	
	itkNewMacro(Self);
	itkTypeMacro(RasdamanImageIO, Superclass);
	
	// overwrite the set file name method to parse the image specs
	virtual void SetFileName(const char*);
	
	virtual bool CanReadFile(const char*);
	
	virtual bool CanStreamRead() {return true;};
	
	virtual void ReadImageInformation();
	virtual void Read(void* buffer);
	
	virtual bool CanWriteFile(const char*);
	virtual bool CanStreamWrite() {return true;};
	
	virtual void WriteImageInformation();
	
	virtual void Write(const void* buffer);
	
	std::string getCollectionName(void)
		{return this->m_collname;}

	std::vector<double> getOIDs(void)
		{return this->m_oids;}

    std::vector<double> getUpperLeftCorner(void)
    {return m_UpperLeftCorner;}

	/*! Sets a valid RasdamanConnector instance */
	void setRasdamanConnector(RasdamanConnector* rasconn);
	RasdamanConnector* getRasdamanConnector(void)
	{
		return this->m_Rasconn;
	}

	/* the collection and image type names have to be specified
	 * to write composite pixel type images into the data base;
	 * note that no type checking is taking place, so make sure
	 * you get it right!
	 * however, this is not required for writing 'flat' pixels
 	 */
	void setRasdamanTypeNames(string colltypename, string imagetypename);

	itkSetMacro(ImageUpdateMode, bool);
	itkGetMacro(ImageUpdateMode, bool);

	void SetForcedLPR(const itk::ImageIORegion& forcedLPR);

	/* set image id of image to be updated*/
	//void setImageID(double oid) {this->m_oids.push_back(oid);};

	otb::AttributeTable::Pointer getRasterAttributeTable(int band);
	void setRasterAttributeTable(otb::AttributeTable* rat, int band);
	void writeRAT(otb::AttributeTable* tab, unsigned int band, double _oid);

protected:	
  RasdamanImageIO();
  virtual ~RasdamanImageIO();
  
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

private:
  	RasdamanImageIO(const Self&); //purposely not implemented
  	void operator=(const Self&); //purposely not implemented
	
	bool parseImageSpec(const std::string imagespec);
	otb::ImageIOBase::IOComponentType getOTBComponentType(
			r_Type::r_Type_Id rtype);
	double getOIDFromCollIndex(void);
	r_Type::r_Type_Id getRasdamanComponentType(otb::ImageIOBase::IOComponentType otbtype);
	double insertForcedLPRDummyImage();
			//const std::string& collname,
			//r_Minterval& sdom);
  
	bool m_bCanRead;
	bool m_bCanWrite;
	bool m_bCollImageAvail;
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
	RasdamanHelper2* m_Helper;
	RasdamanConnector* m_Rasconn;
	std::vector<otb::AttributeTable::Pointer> m_vecRAT;

    std::vector<double> m_UpperLeftCorner;
	std::string m_ImageSpec;
	std::string m_prevImageSpec;
	std::string m_collname;
	std::vector<double> m_oids;
	std::string m_collstrindex;
	long m_collnumindex;
	r_Minterval m_sdom;
	r_Point m_GeoUpdateShift;

	r_Type::r_Type_Id m_rtype;

	std::string m_CollectionTypeName;
	std::string m_ImageTypeName;
};


} 		 // namespace otb 

#endif // __otbRasdamanImageIO_h
