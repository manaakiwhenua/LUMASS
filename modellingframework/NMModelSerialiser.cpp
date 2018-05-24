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
 * NMModelSerialiser.roocpp
 *
 *  Created on: 7/06/2012
 *      Author: alex
 */

#include <string>
#include <iostream>
#include <QFile>
#include <QMetaObject>
#include <QMetaProperty>
#include <QList>
#include <QStringList>

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif


#include "NMModelSerialiser.h"
#include "NMModelComponentFactory.h"
#include "NMProcessFactory.h"
#include "NMIterableComponent.h"
#include "NMDataComponent.h"

NMModelSerialiser::NMModelSerialiser(QObject* parent)
{
	this->setParent(parent);
	this->ctx = this->metaObject()->className();
#ifdef BUILD_RASSUPPORT
	this->mRasconn = 0;
#endif
}

NMModelSerialiser::~NMModelSerialiser()
{
}

QMap<QString, QString> NMModelSerialiser::parseComponent(const QString& fileName,
        NMIterableComponent* importHost, NMModelController* controller
#ifdef BUILD_RASSUPPORT		
		,
		NMRasdamanConnectorWrapper& rasWrapper
#endif
)

{
//	NMDebugCtx(ctx, << "...");

#ifdef BUILD_RASSUPPORT	
	this->mRasconn = &rasWrapper;
#endif

	// register for mapping parsed model component names to final model names
	// as registered with model controller
	QMap<QString, QString> nameRegister;

    QScopedPointer<QFile> file(new QFile(fileName));
	if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
	{
        NMLogError(<< ctx << ": unable to read input file '" << fileName.toStdString()
				<< "'!");
//		NMDebugCtx(ctx, << "done!");
		return nameRegister;
	}

	QDomDocument doc;
    if (!doc.setContent(file.data()))
	{
        NMLogError(<< ctx << ": unable to read input file '" << fileName.toStdString()
				<< "'!");
//		NMDebugCtx(ctx, << "done!");
		return nameRegister;
	}

    this->parseModelDocument(nameRegister, doc, importHost);

    file->close();

    return nameRegister;
}

void
NMModelSerialiser::parseModelDocument(QMap<QString, QString>& nameRegister,
                                      QDomDocument& doc,
                                      NMIterableComponent* importHost)
{

	QDomElement modelElem = doc.documentElement();
//	NMDebugAI(<< "root element: '" << modelElem.attribute("name").toStdString() << "'" << endl);

    NMModelController* controller = this->getModelController();
#ifdef DEBUG
#ifndef _WIN32
    int ind = nmlog::nmindent;
#endif
#endif
	//==========================================================================================
	// parsing model components -- creating objects, assigning properties
	//==========================================================================================

	QDomNode compNode = modelElem.firstChild(); //Element("ModelComponent");
	for (; !compNode.isNull() && compNode.isElement(); compNode = compNode.nextSibling()) //Element("ModelComponent"))
	{
		QDomElement compElem = compNode.toElement();
		QString compType = compElem.tagName();
		// --------------------------------------------------------------------------------------
		// model component creation
		// --------------------------------------------------------------------------------------
		NMModelComponent* comp = 0;
		QString compName = compElem.attribute("name");
		if (compName.isEmpty())
		{
            NMLogError(<< ctx << ": detected unnamed component!");
            //			NMDebugCtx(ctx, << "done!");
            return;// nameRegister;
		}
		else if (compName.compare("root") == 0)
		{
            if (    importHost == 0
                ||  (importHost != 0 && importHost->objectName().compare("root", Qt::CaseInsensitive) == 0)
               )
            {
                // NOTE: we always overrite the root component here!
                comp = controller->getComponent("root");
                nameRegister.insert("root", "root");
            }
            else
            {
                //                NMDebugAI(<< "Ignored 'root' component! Cannot import 'root' "
                //                          << "component into higher level aggregate component!" << std::endl);
                continue;
            }
		}
		else
		{
			comp = NMModelComponentFactory::instance().createModelComponent(compType);


			// add new component to the model controller and make sure it has a unique name
            // note: we're sorting out the host component later (see below)
			comp->setObjectName(compName);
            QString finalName = controller->addComponent(comp, 0);
			if (finalName.compare(compName) != 0)
				comp->setObjectName(finalName);
			nameRegister.insert(compName, finalName);
		}

		// --------------------------------------------------------------------------------------
		// property assignment
		// --------------------------------------------------------------------------------------
        //		NMDebugInd(ind + 1, << "parsing '" << compName.toStdString() << "'" << endl);
		QDomElement propElem = compElem.firstChildElement("Property");
		for (; !propElem.isNull(); propElem = propElem.nextSiblingElement("Property"))
		{
			QVariant value;
			bool suc = false;
			value = this->extractPropertyValue(propElem);

			// get object name from name register to avoid doubly named components
			if (propElem.attribute("name").compare("objectName") == 0)
				value = QVariant(nameRegister.value(value.toString()));
			suc = comp->setProperty(propElem.attribute("name").toStdString().c_str(), value);

            //			NMDebugInd(ind+2, << "setting " << propElem.attribute("name").toStdString()
            //					<< "=" << value.toString().toStdString()
            //					<< " - " << suc << endl);
		}

		// --------------------------------------------------------------------------------------
		// process component creation
		// --------------------------------------------------------------------------------------
		QDomElement procElem = compElem.firstChildElement("Process");
		if (!procElem.isNull())
		{
			NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
			NMProcess* proc = 0;
			QString procName = procElem.attribute("name");
			if (!procName.isEmpty())
			{
                //				NMDebugInd(ind+2, << "setting process '" << procName.toStdString()
                //						<< "'" << endl);

				proc = NMProcessFactory::instance().createProcess(procName);
                if (proc == 0)
                {
                    // all right, obviously lumass couldn't produce the requested
                    // process componente - what do we do ...
                    NMWarn(ctx, << "Failed to create process component '"
                           << compName.toStdString()
                           << "(" << procName.toStdString() << ") - skipping it!");

                    // remove component from register and model controller and carry on ...
                    QMap<QString, QString>::Iterator it = nameRegister.find(compName);
                    nameRegister.erase(it);
                    if (!controller->removeComponent(compName))
                    {
                        NMWarn(ctx, << "Failed to remove NULL Process Component '"
                               << compName.toStdString() << "' from model controller!");
                    }
                    continue;
                }
				
				proc->setObjectName(procName);
				QDomElement procPropElem = procElem.firstChildElement("Property");
				for (; !procPropElem.isNull(); procPropElem = procPropElem.nextSiblingElement("Property"))
				{
					QVariant value = this->extractPropertyValue(procPropElem);
					bool suc = proc->setProperty(procPropElem.attribute("name").toStdString().c_str(), value);

                    //					NMDebugInd(ind+3, << "setting " << procPropElem.attribute("name").toStdString()
                    //							<< "=" << value.toString().toStdString()
                    //							<< " - " << suc << endl);
				}
			}
			ic->setProcess(proc);
		}
	}

	//==========================================================================================
	// harmonising input component settings and with possibly adjusted component names
	//==========================================================================================
	this->harmoniseInputComponentNames(nameRegister, controller);

	//==========================================================================================
    // parsing model components -- establish component relationships I: setting subcomponents
	//==========================================================================================
	compNode = modelElem.firstChild(); //Element("ModelComponent");
	for (; !compNode.isNull() && compNode.isElement(); compNode = compNode.nextSibling()) //Element("ModelComponent"))
	{
		QDomElement compElem = compNode.toElement();
		QString itCompName = compElem.attribute("name");
		if (itCompName.isEmpty())
			continue;

		NMModelComponent* itComp = 0;
        QMap<QString, QString>::Iterator it = nameRegister.find(itCompName);
        if (it == nameRegister.end())
            continue;

        QString finalName = it.value();
		itComp = controller->getComponent(finalName);
		if (itComp == 0)
			continue;

        //		NMDebugAI(<< "setting subcomponents for '" << itCompName.toStdString() << "'" << endl);
		QDomElement subcompElem = compElem.firstChildElement("Subcomponents");
		if (!subcompElem.isNull())
		{
			NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(itComp);
			QStringList lst = subcompElem.text().split(" ", QString::SkipEmptyParts);
			for (unsigned int i=0; i < lst.size(); ++i)
			{
				NMModelComponent* sub = 0;
				if (nameRegister.find(lst.at(i)) != nameRegister.end())
				{
					QString finalName = nameRegister[lst.at(i)];
					sub = controller->getComponent(finalName);
					if (sub != 0 && ic != 0)
					{
						ic->addModelComponent(sub);
					}
				}
			}
		}
	}

    //==========================================================================================
    // iterating over imported components -- establish component relationships II: setting host component
    //==========================================================================================
    NMIterableComponent* ic = importHost;
    if (ic == 0)
    {
        NMModelComponent* mc = this->getModelController()->getComponent(QString::fromUtf8("root"));
        ic = qobject_cast<NMIterableComponent*>(mc);
    }

    foreach(const QString& name, nameRegister.values())
    {
    //        NMDebugAI(<< "sorting host for '" << name.toStdString() << "' ..." << std::endl);
        NMModelComponent* c = this->getModelController()->getComponent(name);
        if (    c != 0
            &&  c->getHostComponent() == 0
            &&  c->objectName().compare("root") != 0
            &&  ic != 0
           )
        {
            ic->addModelComponent(c);
        }
    }

	// ------------------------------------------------------
	// a bit of debug code
    //	QMap<QString, NMModelComponent*>& repo =
    //			const_cast<QMap<QString, NMModelComponent*>& >(controller->getRepository());

    //	NMDebug(<< endl);
    //	NMDebugAI(<< "Model controller's contents after import ..." << endl);
    //	foreach(NMModelComponent* cmp, repo.values())
    //	{
    //		NMDebugAI(<< cmp->objectName().toStdString() << endl);
    //	}
        // ------------------------------------------------------


    //    NMDebugCtx(ctx, << "done!");
        //return nameRegister;
}

QString
NMModelSerialiser::removeSurplusCR(const QString& inStr)
{
    QList<int> pos;
    QString ret;
    QTextStream text(&ret);
    for (int i=0; i < inStr.size(); ++i)
    {
        if (inStr[i] == '\r')
        {
            pos << i;
        }
        else if (inStr[i] == '\n')
        {
            if (pos.size() > 0)
            {
                pos.clear();
            }
            text << inStr[i];
        }
        else
        {
            if (pos.size() > 0)
            {
                text << inStr.mid(pos.first(), pos.last() - pos.first() + 1);
                pos.clear();
            }
            text << inStr[i];
        }
    }

    // any CRs left we haven't written yet
    if (pos.size() > 0)
    {
        text << inStr.mid(pos.first(), pos.last() - pos.first() + 1);
        pos.clear();
    }

    return ret;
}


QVariant
NMModelSerialiser::extractPropertyValue(QDomElement& propElem)
{
	QVariant value;
	bool bok;

	QDomNode valueNode = propElem.firstChild();
	if (valueNode.isNull() || !valueNode.isElement())
		return value;

	if (valueNode.nodeName() == "string")
	{
        value = QVariant::fromValue(removeSurplusCR(valueNode.toElement().text()));
	}
	else if (valueNode.nodeName() == "stringlist")
	{
		QStringList lst;
		QDomNodeList nodeList = valueNode.childNodes();
		for (unsigned int i = 0; i < nodeList.count(); ++i)
            lst << removeSurplusCR(nodeList.at(i).toElement().text());
		value = QVariant::fromValue(lst);
	}
	else if (valueNode.nodeName() == "list_stringlist")
	{
		QList<QStringList> lsl;
		QDomNodeList nodeList = valueNode.childNodes();
		for (unsigned int i = 0; i < nodeList.count(); ++i)
		{
			QStringList lst;
			QDomNodeList innerList = nodeList.at(i).childNodes();
			for (unsigned int ii = 0; ii < innerList.count(); ++ii)
                lst << removeSurplusCR(innerList.at(ii).toElement().text());
			lsl.push_back(lst);
		}
		value = QVariant::fromValue(lsl);
	}
	else if (valueNode.nodeName() == "list_list_stringlist")
	{
		QList<QList<QStringList> > llsl;
		QDomNodeList outerList = valueNode.childNodes();
		for (unsigned int i = 0; i < outerList.count(); ++i)
		{
			QList<QStringList> lsl;
			QDomNodeList innerList = outerList.at(i).childNodes();
			for (unsigned int ii = 0; ii < outerList.count(); ++ii)
			{
				QStringList lst;
				QDomNodeList stringList = innerList.at(ii).childNodes();
				for (unsigned int iii = 0; iii < stringList.count(); ++iii)
                    lst << removeSurplusCR(stringList.at(iii).toElement().text());
				lsl.push_back(lst);
			}
			llsl.push_back(lsl);
		}
		value = QVariant::fromValue(llsl);
	}
	else if (valueNode.nodeName() == "component_type")
	{
		NMItkDataObjectWrapper::NMComponentType type =
				NMItkDataObjectWrapper::getComponentTypeFromString(valueNode.toElement().text());
		value = QVariant::fromValue(type);
	}
	else if (valueNode.nodeName() == "advance_parameter")
	{
		NMProcess::AdvanceParameter ap;
		QString t = valueNode.toElement().text();
		if (t.compare("NM_USE_UP") == 0)
			ap = NMProcess::NM_USE_UP;
		else if (t.compare("NM_CYCLE") == 0)
			ap = NMProcess::NM_CYCLE;
		else if (t.compare("NM_SYNC_WITH_HOST") == 0)
			ap = NMProcess::NM_SYNC_WITH_HOST;
		value = QVariant::fromValue(ap);
	}
#ifdef BUILD_RASSUPPORT	
	else if (valueNode.nodeName() == "rasdaman_connection")
	{
		QString val = valueNode.toElement().text();
		if (val.compare("yes", Qt::CaseInsensitive) == 0 ||
			val.compare("1") == 0)
		{
			if (this->mRasconn->getConnector() != 0)
			{
				value = QVariant::fromValue(this->mRasconn);
			}
			else
			{
				value.setValue<NMRasdamanConnectorWrapper*>(0);
                NMLogError(<< ctx << ": rasdaman connector requested, but non available!");
			}
		}
		else
		{
			value.setValue<NMRasdamanConnectorWrapper*>(0);
		}
	}
#endif	
	else if (valueNode.nodeName() == "uchar" ||
			 valueNode.nodeName() == "ushort"
			)
	{
		unsigned short v = valueNode.toElement().text().toUShort(&bok);
		if (bok)
			value = QVariant::fromValue(v);
	}
	else if (valueNode.nodeName() == "char" ||
			 valueNode.nodeName() == "short"
			)
	{
		short v = valueNode.toElement().text().toShort(&bok);
		if (bok)
			value = QVariant::fromValue(v);
	}
	else if (valueNode.nodeName() == "uint")
	{
		unsigned int v = valueNode.toElement().text().toUInt(&bok);
		if (bok)
			value = QVariant::fromValue(v);
	}
	else if (valueNode.nodeName() == "int")
	{
		int v = valueNode.toElement().text().toInt(&bok);
		if (bok)
			value = QVariant::fromValue(v);
	}
	else if (valueNode.nodeName() == "ulong")
	{
		unsigned long v = valueNode.toElement().text().toULong(&bok);
		if (bok)
			value = QVariant::fromValue(v);
	}
	else if (valueNode.nodeName() == "long")
	{
		long v = valueNode.toElement().text().toLong(&bok);
		if (bok)
			value = QVariant::fromValue(v);
	}
	else if (valueNode.nodeName() == "float")
	{
		float v = valueNode.toElement().text().toFloat(&bok);
		if (bok)
			value = QVariant::fromValue(v);
	}
	else if (valueNode.nodeName() == "double")
	{
		double v = valueNode.toElement().text().toDouble(&bok);
		if (bok)
			value = QVariant::fromValue(v);
	}
	else if (valueNode.nodeName() == "bool")
	{
		QString vstr = valueNode.toElement().text();
		bool v;
		if (vstr.compare("true") == 0)
			v = true;
		else
			v = false;
		value = QVariant::fromValue(v);
	}

	return value;
}

void
NMModelSerialiser::serialiseComponent(NMModelComponent* comp,
        const QString& fileName, unsigned int indent, bool appendmode)
{
    NMDebugCtx(ctx, << "...");

	QFile file(fileName);
	if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
	{
        NMLogError(<< ctx << ": unable to create file '" << fileName.toStdString()
				<< "'!");
        NMDebugCtx(ctx, << "done!");
		return;
	}

	// if the file is empty, then we're obviously not in appendmode!
	if (file.size() == 0)
		appendmode = false;

	// if we're appending a model component, we read the file's content
	// into the doc object and erase the file's contents; if we're not
	// overwriting the entire file, we're not reading its content but
	// only erasing it
	QDomDocument doc;
	if (appendmode)
	{
		if (!doc.setContent(&file))
		{
            NMLogError(<< ctx << ": failed reading document structure from '"
					<< fileName.toStdString() << "'!");
            NMDebugCtx(ctx, << "done!");
			return;
		}
	}
	else
	{
		// we've got to have exactly one root tag element enclosing all sub
		// model components, that's what we call 'Model' here
		QDomElement modElem = doc.createElement("Model");
		modElem.setAttribute("description", "the one and only model element");
		doc.appendChild(modElem);
	}
	file.write("");
	file.close();
	file.open(QIODevice::WriteOnly | QIODevice::Text);

	this->serialiseComponent(comp, doc);

	QTextStream out(&file);
	out << doc.toString(indent);
	file.close();

    NMDebugCtx(ctx, << "done!");
}

void
NMModelSerialiser::serialiseComponent(NMModelComponent* comp,
		QDomDocument& doc)
{
//	NMDebugCtx(ctx, << "...");

	if (comp == 0)
	{
        NMLogError(<< ctx << ": cannot serialise NULL component!");
		return;
	}
//	NMDebugAI(<< "serialising '" << comp->objectName().toStdString() << endl);

	// ----------------------------------------------------------------------
	// CREATE THE MODEL COMPONENT ELEMENT
	// what kind of component do we've got here?
	const QMetaObject* meta = comp->metaObject();
	QDomElement rootElem = doc.documentElement();
	QDomElement compElem = doc.createElement(meta->className());
	compElem.setAttribute("name", comp->objectName());
	rootElem.appendChild(compElem);

	// ---------------------------------------------------------------------
	// NON-ITERABLE COMPONENTS (ie non-group or non-process components)
	// serialise component's properties first
	unsigned int nprop = meta->propertyCount();
	for (unsigned int p=0; p < nprop; ++p)
	{
		QMetaProperty property = meta->property(p);
		QDomElement propElem = doc.createElement("Property");
		propElem.setAttribute("name", property.name());
		compElem.appendChild(propElem);

		QVariant v = comp->property(property.name());
		QDomElement valueElem = this->createValueElement(doc, v);
		propElem.appendChild(valueElem);
	}

	// -----------------------------------------------------------------------
	// ITERABLE COMPONENTS
	// serialise either a reference to subcomponents or
	// the one and only process object
	NMIterableComponent* ic =
			qobject_cast<NMIterableComponent*>(comp);

	if (ic != 0)
	{
		// go through the component chain and serialise each component
		if (ic->getProcess() == 0)
		{
			QStringList strCompChain;
            NMModelComponentIterator cit = ic->getComponentIterator();
            while (*cit != 0)
			{
                strCompChain << cit->objectName();
                ++cit;
			}

			QDomElement subComps = doc.createElement("Subcomponents");
			compElem.appendChild(subComps);

			QDomNode subList = doc.createTextNode(strCompChain.join(" "));
			subComps.appendChild(subList);
		}
		// serialise NMProcess
		else
		{
			NMProcess* proc = ic->getProcess();
			const QMetaObject* procMeta = proc->metaObject();

			QDomElement procElem = doc.createElement("Process");
			procElem.setAttribute("name", procMeta->className());
			compElem.appendChild(procElem);

			unsigned int nprocprops = procMeta->propertyCount();
			for (unsigned int pp=0; pp < nprocprops; ++pp)
			{
				QMetaProperty procProp = procMeta->property(pp);
				QDomElement procPropElem = doc.createElement("Property");
				procPropElem.setAttribute("name", procProp.name());
				procElem.appendChild(procPropElem);

				QVariant v = proc->property(procProp.name());
				QDomElement valueElem = this->createValueElement(doc, v);
				procPropElem.appendChild(valueElem);
			}
		}
	}

//	NMDebugCtx(ctx, << "done!");
}

QDomElement NMModelSerialiser::createValueElement(QDomDocument& doc,
		QVariant& dataValue)
{
	QDomElement valueElement;
	QDomNode valueElementChild;

	if (QString(dataValue.typeName()).compare("QString") == 0)
	{
		valueElement = doc.createElement("string");
		valueElementChild = doc.createTextNode(dataValue.toString());
		valueElement.appendChild(valueElementChild);
	}
	else if (QString(dataValue.typeName()).compare("QStringList") == 0)
	{
		valueElement = doc.createElement("stringlist");
		QStringList sl = dataValue.value<QStringList>();
		for (unsigned int s=0; s < sl.size(); ++s)
		{
			QVariant str = sl.at(s);
			QDomElement childElem = this->createValueElement(doc, str);
			valueElement.appendChild(childElem);
		}
	}
	else if (QString(dataValue.typeName()).compare("QList<QStringList>") == 0)
	{
		valueElement = doc.createElement("list_stringlist");
		QList<QStringList> lst = dataValue.value<QList<QStringList> >();
		for (unsigned int l=0; l < lst.size(); ++l)
		{
			QVariant sl = QVariant::fromValue(lst.at(l));
			QDomElement childElem = this->createValueElement(doc, sl);
			valueElement.appendChild(childElem);
		}
	}
	else if (QString(dataValue.typeName()).compare("QList<QList<QStringList> >") == 0)
	{
		valueElement = doc.createElement("list_list_stringlist");
		QList<QList<QStringList> > llsl = dataValue.value<QList<QList<QStringList> > >();
		for (unsigned int l=0; l < llsl.size(); ++l)
		{
			QVariant lsl = QVariant::fromValue(llsl.at(l));
			QDomElement childElement = this->createValueElement(doc, lsl);
			valueElement.appendChild(childElement);
		}
	}
	else if (QString(dataValue.typeName()).compare("NMItkDataObjectWrapper::NMComponentType") == 0)
	{
		valueElement = doc.createElement("component_type");
		QString stype = NMItkDataObjectWrapper::getComponentTypeString(
				dataValue.value<NMItkDataObjectWrapper::NMComponentType>());
		valueElementChild = doc.createTextNode(stype);
		valueElement.appendChild(valueElementChild);
	}
	else if (QString(dataValue.typeName()).compare("NMProcess::AdvanceParameter") == 0)
	{
		valueElement = doc.createElement("advance_parameter");

		QString stype;
		bool bok;
		int v = dataValue.toInt(&bok);
		switch(v)
		{
		case 0: stype = "NM_USE_UP"; break;
		case 1: stype = "NM_CYCLE"; break;
		case 2: stype = "NM_SYNC_WITH_HOST"; break;
		}
		valueElementChild = doc.createTextNode(stype);
		valueElement.appendChild(valueElementChild);
	}
#ifdef BUILD_RASSUPPORT	
	else if (QString(dataValue.typeName()).compare("NMRasdamanConnectorWrapper*") == 0)
	{
		valueElement = doc.createElement("rasdaman_connection");
		NMRasdamanConnectorWrapper* rr = dataValue.value<NMRasdamanConnectorWrapper*>();
		if (rr != 0)
		{
			if (rr->getConnector() == 0)
				valueElementChild = doc.createTextNode("no");
			else
				valueElementChild = doc.createTextNode("yes");
		}
		else
			valueElementChild = doc.createTextNode("no");
		valueElement.appendChild(valueElementChild);
	}
#endif	
	//else if (QString(dataValue.typeName()).compare("NMModelComponent*") == 0)
	//{
	//	valueElement = doc.createElement("component_name");
	//	NMModelComponent* comp = dataValue.value<NMModelComponent*>();
	//	if (comp != 0)
	//		valueElementChild = doc.createTextNode(comp->objectName());
	//	else
	//		valueElementChild = doc.createTextNode("");
	//	valueElement.appendChild(valueElementChild);
	//}
	else if (QString(dataValue.typeName()).compare("bool") == 0)
	{
		valueElement = doc.createElement("bool");
		if (dataValue.toBool() == true)
			valueElementChild = doc.createTextNode("true");
		else
			valueElementChild = doc.createTextNode("false");
		valueElement.appendChild(valueElementChild);
	}
	else if (QString(dataValue.typeName()).compare("uchar") == 0 ||
			 QString(dataValue.typeName()).compare("char") == 0 ||
			 QString(dataValue.typeName()).compare("uint") == 0 ||
			 QString(dataValue.typeName()).compare("int") == 0 ||
			 QString(dataValue.typeName()).compare("ushort") == 0 ||
			 QString(dataValue.typeName()).compare("short") == 0 ||
			 QString(dataValue.typeName()).compare("ulong") == 0 ||
			 QString(dataValue.typeName()).compare("long") == 0 ||
			 QString(dataValue.typeName()).compare("float") == 0 ||
			 QString(dataValue.typeName()).compare("double") == 0
			)
	{
		valueElement = doc.createElement(dataValue.typeName());
		valueElementChild = doc.createTextNode(dataValue.toString());
		valueElement.appendChild(valueElementChild);
	}
	return valueElement;
}

void
NMModelSerialiser::harmoniseInputComponentNames(QMap<QString, QString>& nameRegister,
		NMModelController* controller)
{
	/*  We go through the list of imported components,
     *  and ensure for all process and data components that
	 *  the names of any input components are consistent
	 *  with the actual component names as result of
     *  the import process.
     *  We also make sure that we remove any input registered
     *  with the component, but not available amongst the currently
     *  loaded model components (this is the case when a single
     *  component embedded in a certain context, is saved individually
     *  and re-used (imported) to another modelling context).
     */
	QStringList newnames = nameRegister.values();
	foreach(const QString& nn, newnames)
	{
        NMModelComponent* comp = controller->getComponent(nn);
        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
        NMDataComponent* dc = qobject_cast<NMDataComponent*>(comp);
        if ((ic != 0 && ic->getProcess() != 0)
             || dc != 0)
		{
            QList<QStringList> revisedList;
            QList<QStringList> inputslist = comp->getInputs();
			for(int i=0; i < inputslist.size(); ++i)
			{
                QStringList newInputsList;
				QStringList oldinputs = inputslist.at(i);
				for(int oi=0; oi < oldinputs.size(); ++oi)
				{
					QString oldinputSrc = oldinputs.at(oi);
					QString newinput;
					if (oldinputSrc.contains(":"))
					{
						QStringList oldinputSrcParams = oldinputSrc.split(":", QString::SkipEmptyParts);
						if (oldinputSrcParams.size() == 2)
						{
							newinput = QString("%1:%2").
								arg(nameRegister.value(oldinputSrcParams.at(0))).
								arg(oldinputSrcParams.at(1));
						}
						else
						{
                            NMLogWarn(<< ctx << ": " << comp->objectName().toStdString() << " seems to contain an "
									<< "ill formatted component input string: '"
									<< oldinputSrc.toStdString() << "' - we just go for the name then!");
							newinput = nameRegister.value(oldinputSrcParams.at(0));
						}
					}
					else
					{
						newinput = nameRegister.value(oldinputSrc);
					}
                    if (!newinput.isEmpty())
                    {
                        newInputsList.push_back(newinput);
                    }
				}
                if (newInputsList.size() > 0)
                {
                    revisedList.push_back(newInputsList);
                }
			}
            comp->setInputs(revisedList);
		}
	}
}





