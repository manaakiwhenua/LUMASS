/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2015 Landcare Research New Zealand Ltd
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
 *  NMSQLiteProcessorWrapper.cpp
 *
 *  Created on: 2015-11-08
 *      Author: Alexander Herzig
 */

#include "NMSQLiteProcessorWrapper.h"
#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
#include "NMModelController.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "otbSQLiteProcessor.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMSQLiteProcessorWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::SQLiteProcessor<InImgType, OutImgType>      FilterType;
    typedef typename FilterType::Pointer        FilterTypePointer;

    // more typedefs
    typedef typename InImgType::PixelType  InImgPixelType;
    typedef typename OutImgType::PixelType OutImgPixelType;

    typedef typename OutImgType::SpacingType      OutSpacingType;
    typedef typename OutImgType::SpacingValueType OutSpacingValueType;
    typedef typename OutImgType::PointType        OutPointType;
    typedef typename OutImgType::PointValueType   OutPointValueType;
    typedef typename OutImgType::SizeValueType    SizeValueType;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
	{
		FilterTypePointer f = FilterType::New();
		otbFilter = f;
	}

    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
                    unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
    {
        InImgType* img = dynamic_cast<InImgType*>(dataObj);
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        filter->SetInput(idx, img);
    }


	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		return dynamic_cast<OutImgType*>(filter->GetOutput(idx));
	}

    static otb::AttributeTable::Pointer getRAT(
        itk::ProcessObject::Pointer& procObj, 
        unsigned int numBands, unsigned int idx)
    {
        FilterType *f = dynamic_cast<FilterType*>(procObj.GetPointer());
        return f->getRAT(idx);
    }


    static void setRAT(
        itk::ProcessObject::Pointer& procObj, 
        unsigned int numBands, unsigned int idx,
        otb::AttributeTable::Pointer& rat)
    {
        FilterType *f = dynamic_cast<FilterType*>(procObj.GetPointer());
        return f->setRAT(idx, rat);
    }



    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc,
			unsigned int step, const QMap<QString, NMModelComponent*>& repo)
	{
		NMDebugCtx("NMSQLiteProcessorWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMSQLiteProcessorWrapper* p =
				dynamic_cast<NMSQLiteProcessorWrapper*>(proc);

		// make sure we've got a valid filter object
		if (f == 0)
		{
			NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
			e.setMsg("We're trying to link, but the filter doesn't seem to be initialised properly!");
			throw e;
			return;
		}

		/* do something reasonable here */
		bool bok;
		int givenStep = step;

		
        step = p->mapHostIndexToPolicyIndex(givenStep, p->mSQLStatement.size());
        std::string curSQLStatement;
        if (step < p->mSQLStatement.size())
        {
            curSQLStatement = p->mSQLStatement.at(step).toStdString().c_str();
            f->SetSQLStatement(curSQLStatement);
        }


                
	    step = p->mapHostIndexToPolicyIndex(givenStep, p->mInputComponents.size());				
	    std::vector<std::string> userIDs;                                                                       
	    QStringList currentInputs;                                                                              
	    if (step < p->mInputComponents.size())                                                                  
	    {                                                                                                       
		    currentInputs = p->mInputComponents.at(step);                                                   
		    int cnt=0;                                                                                      
		    foreach (const QString& input, currentInputs)                                                   
		    {                                                                                               
		        std::stringstream uid;                                                                      
		        uid << "L" << cnt;                                                                          
		        QString inputCompName = NMModelController::getComponentNameFromInputSpec(input);            
		        NMModelComponent* comp = NMModelController::getInstance()->getComponent(inputCompName);     
		        if (comp != 0)                                                                              
		        {                                                                                           
			        if (comp->getUserID().isEmpty())                                                        
			        {                                                                                       
				        userIDs.push_back(uid.str());                                                   
			        }                                                                                       
			        else                                                                                    
			        {                                                                                       
				        userIDs.push_back(comp->getUserID().toStdString());                             
			        }                                                                                       
		        }                                                                                           
		        else                                                                                        
		        {                                                                                           
			        userIDs.push_back(uid.str());                                                           
		        }                                                                                           
		        ++cnt;                                                                                      
		    }                                                                                               
	    }                                                                                                       
	    f->SetImageNames(userIDs);


		NMDebugCtx("NMSQLiteProcessorWrapper_Internal", << "done!");
	}
};

template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<char, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<short, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<int, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<long, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<float, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned char, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, char, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned short, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, short, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned int, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, int, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned long, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, long, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, float, 1>;
template class NMSQLiteProcessorWrapper_Internal<double, double, 1>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<char, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<short, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<int, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<long, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<float, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned char, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, char, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned short, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, short, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned int, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, int, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned long, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, long, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, float, 2>;
template class NMSQLiteProcessorWrapper_Internal<double, double, 2>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned char, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<char, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned short, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<short, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned int, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<int, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<unsigned long, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<long, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<float, double, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned char, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, char, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned short, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, short, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned int, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, int, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, unsigned long, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, long, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, float, 3>;
template class NMSQLiteProcessorWrapper_Internal<double, double, 3>;


InstantiateObjectWrap( NMSQLiteProcessorWrapper, NMSQLiteProcessorWrapper_Internal )
SetNthInputWrap( NMSQLiteProcessorWrapper, NMSQLiteProcessorWrapper_Internal )
GetOutputRATWrap( NMSQLiteProcessorWrapper, NMSQLiteProcessorWrapper_Internal )
LinkInternalParametersWrap( NMSQLiteProcessorWrapper, NMSQLiteProcessorWrapper_Internal )
GetRATWrap( NMSQLiteProcessorWrapper, NMSQLiteProcessorWrapper_Internal )

SetRATWrap( NMSQLiteProcessorWrapper, NMSQLiteProcessorWrapper_Internal )


NMSQLiteProcessorWrapper
::NMSQLiteProcessorWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMSQLiteProcessorWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMSQLiteProcessorWrapper
::~NMSQLiteProcessorWrapper()
{
}
