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
