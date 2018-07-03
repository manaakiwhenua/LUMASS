/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2017 Landcare Research New Zealand Ltd
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
 *  NMMosraFilterWrapper.cpp
 *
 *  Created on: 2017-03-03
 *      Author: Alexander Herzig
 */

#include "NMMosraFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include <QTextStream>
#include <QFile>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
#include "NMModelController.h"

#include "otbNMMosraFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMMosraFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::NMMosraFilter<InImgType, OutImgType>      FilterType;
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
		NMDebugCtx("NMMosraFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMMosraFilterWrapper* p =
				dynamic_cast<NMMosraFilterWrapper*>(proc);

		// make sure we've got a valid filter object
		if (f == 0)
		{
			NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
            e.setDescription("We're trying to link, but the filter doesn't seem to be initialised properly!");
			throw e;
			return;
		}

		/* do something reasonable here */
		bool bok;
		int givenStep = step;

        QString workspace;
        QVariant varWS = p->getModelController()->getSetting("Workspace");
        if (varWS.isValid())
        {
            workspace = varWS.toString();
            if (!workspace.isEmpty())
            {
                f->SetWorkspace(workspace.toStdString());
            }
        }

        QObject* parent = p->parent();
        QString compName = "MOSRA";
        if (parent != nullptr)
        {
            compName = parent->objectName();
            if (!compName.isEmpty())
            {
                f->SetCompName(compName.toStdString());
            }
        }


        QVariant curLosFileNameVar = p->getParameter("LosFileName");
        QString curLosFileName;
        if (curLosFileNameVar.isValid())
        {
            curLosFileName = curLosFileNameVar.toString();
            QFile losFile(curLosFileName);
            if (!losFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                NMErr("NMMosraFilterWrapper_Internal", << "Invlid value for 'LosFileName'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setDescription("Ivalid value for 'LosFileName'!");
                throw e;
            }

            QTextStream losStream(&losFile);
            QString losInString = losStream.readAll();
            losFile.close();
            QString losOutString = p->getModelController()->processStringParameter(p, losInString);
            //f->SetLosFileName(curLosFileName);
            f->SetLosSettings(losOutString.toStdString());
        }

        QVariant curTimeOutVar = p->getParameter("TimeOut");
        int curTimeOut;
        if (curTimeOutVar.isValid())
        {
            curTimeOut = curTimeOutVar.toInt(&bok);
            if (bok)
            {
                f->SetTimeOut((curTimeOut));
            }
            else
            {
                NMErr("NMMosraFilterWrapper_Internal", << "Invalid value for 'TimeOut'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setDescription("Invalid value for 'TimeOut'!");
                throw e;
            }
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
                QString inputCompName = p->getModelController()->getComponentNameFromInputSpec(input);
                NMModelComponent* comp = p->getModelController()->getComponent(inputCompName);
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


		NMDebugCtx("NMMosraFilterWrapper_Internal", << "done!");
	}
};

/*$<HelperClassInstantiation>$*/

InstantiateObjectWrap( NMMosraFilterWrapper, NMMosraFilterWrapper_Internal )
SetNthInputWrap( NMMosraFilterWrapper, NMMosraFilterWrapper_Internal )
GetOutputRATWrap( NMMosraFilterWrapper, NMMosraFilterWrapper_Internal )
LinkInternalParametersWrap( NMMosraFilterWrapper, NMMosraFilterWrapper_Internal )
GetRATWrap( NMMosraFilterWrapper, NMMosraFilterWrapper_Internal )

SetRATWrap( NMMosraFilterWrapper, NMMosraFilterWrapper_Internal )


NMMosraFilterWrapper
::NMMosraFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMMosraFilterWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMMosraFilterWrapper
::~NMMosraFilterWrapper()
{
}
