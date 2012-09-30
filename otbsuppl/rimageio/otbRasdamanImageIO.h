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
#include "itkImageIOBase.h"
#include "RasdamanHelper2.hh"
#include "RasdamanConnector.hh"
#include "rasdaman.hh"



namespace otb
{

class ITK_EXPORT RasdamanImageIO : public itk::ImageIOBase
{
public:

	typedef RasdamanImageIO Self;
	typedef itk::ImageIOBase Superclass;
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
	
	/*! Sets a valid RasdamanConnector instance */
	void setRasdamanConnector(RasdamanConnector* rasconn);

	/* the collection and image type names have to be specified
	 * to write composite pixel type images into the data base;
	 * note that no type checking is taking place, so make sure
	 * you get it right!
	 * however, this is not required for writing 'flat' pixels
 	 */
	void setRasdamanTypeNames(string colltypename, string imagetypename);

	/* set image id of image to be updated*/
	//void setImageID(double oid) {this->m_oids.push_back(oid);};

	otb::AttributeTable::Pointer getRasterAttributeTable(int band);
	void setRasterAttributeTable(otb::AttributeTable* rat, int band);

protected:	
  RasdamanImageIO();
  virtual ~RasdamanImageIO();
  
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

private:
  RasdamanImageIO(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented	
	
	bool parseImageSpec(const std::string imagespec);
	itk::ImageIOBase::IOComponentType getOTBComponentType(
			r_Type::r_Type_Id rtype);

	r_Type::r_Type_Id getRasdamanComponentType(itk::ImageIOBase::IOComponentType otbtype);

	void WriteRAT(otb::AttributeTable* tab, double _oid);
  
	bool m_bCanRead;
	bool m_bCanWrite;
	bool m_bCollImageAvail;
	bool m_bUpdateImage;
	
	bool m_bWasWriteCalled;
	bool m_bImageInfoNeedsToBeWritten;

	itk::ImageIOBase::ByteOrder m_FileByteOrder;
	RasdamanHelper2* m_Helper;
	RasdamanConnector* m_Rasconn;
	std::vector<otb::AttributeTable::Pointer> m_vecRAT;

	std::string m_ImageSpec;
	std::string m_prevImageSpec;
	std::string m_collname;
	std::vector<double> m_oids;
	std::string m_collstrindex;
	long m_collnumindex;
	r_Minterval m_sdom;
	r_Point m_GeoUpdateShift;

	r_Type::r_Type_Id m_rtype;

	string m_CollectionTypeName;
	string m_ImageTypeName;
};


} 		 // namespace otb 

#endif // __otbRasdamanImageIO_h
