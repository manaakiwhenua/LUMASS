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
#include "NMEditModelComponentDialog.h"
#include <QMetaObject>
#include <QMetaProperty>
#include <QList>
#include <QStringList>
#include <QScopedPointer>
#include <limits>


const std::string NMEditModelComponentDialog::ctx = "NMEditModelComponentDialog";

NMEditModelComponentDialog::NMEditModelComponentDialog(QWidget *parent)
    : QWidget(parent), mObj(0), comp(0), proc(0)
{
	ui.setupUi(this);

#ifdef BUILD_RASSUPPORT
	this->mRasConn = 0;
#endif
}

NMEditModelComponentDialog::~NMEditModelComponentDialog()
{
}

void
NMEditModelComponentDialog::closeEvent(QCloseEvent* event)
{
	emit finishedEditing(mObj);
	QWidget::closeEvent(event);
}

void
NMEditModelComponentDialog::update()
{
	NMDebugCtx(ctx, << "...");


	this->readComponentProperties(mObj, comp, proc);


	NMDebugCtx(ctx, << "done!");
}

void
//NMEditModelComponentDialog::setComponent(NMModelComponent* comp)
NMEditModelComponentDialog::setObject(QObject* obj)
{
	NMDebugCtx(ctx, << "...");

	// we only support NMModelComponent and
	// NMProcess objects at this stage
	this->comp = qobject_cast<NMModelComponent*>(obj);
	this->proc = qobject_cast<NMProcess*>(obj);

//	// make this dialog listen to any changes within the components themselves
//	if (comp != 0)
//	{
//		connect(comp, SIGNAL(NMModelComponentChanged()), this, SLOT(update()));
//	}
//	else if (proc != 0)
//	{
//		connect(proc, SIGNAL(NMProcessChanged()), this, SLOT(update()));
//	}
//	else
	if (comp == 0 && proc == 0)
	{
		NMDebugCtx(ctx, << "done!");
		return;
	}

	this->mObj = obj;
	this->setWindowTitle(obj->objectName());

	this->readComponentProperties(mObj, comp, proc);


	NMDebugCtx(ctx, << "done!");
}

void NMEditModelComponentDialog::readComponentProperties(QObject* obj, NMModelComponent* comp,
		NMProcess* proc)
{
	ui.propBrowser->clear();

	// let's start with the component's properties,
	// in case we've got a component
	if (comp != 0)// && comp->getProcess() == 0)
	{
		// do we have a process component?
		proc = comp->getProcess();

		const QMetaObject* meta = obj->metaObject();
		unsigned int nprop = meta->propertyCount();
		for (unsigned int p=0; p < nprop; ++p)
		{
			QMetaProperty property = meta->property(p);
			this->createPropertyEdit(property, obj);
		}

		if (comp->getProcess() == 0)
		{
			// now we add the subcomponents list for reference
			QStringList strCompChain;
			NMModelComponent* sc = comp->getInternalStartComponent();
			strCompChain << "{";
			while (sc != 0)
			{
				strCompChain << QString(tr("{%1}")).arg(sc->objectName());
				sc = comp->getNextInternalComponent();
			}
			strCompChain << "}";

			QtVariantEditorFactory* ed = new QtVariantEditorFactory();
			QtVariantPropertyManager* man = new QtVariantPropertyManager();
			QtVariantProperty* vprop;

			connect(man, SIGNAL(valueChanged(QtProperty*, const QVariant &)),
					this, SLOT(applySettings(QtProperty*, const QVariant &)));

			QVariant vval = QVariant::fromValue(strCompChain);
			vprop = man->addProperty(QVariant::StringList, "Subcomponents");
			vprop->setValue(vval);
			ui.propBrowser->setFactoryForManager(man, ed);
			ui.propBrowser->addProperty(vprop);
		}
	}

	// if this is a process component, we add the processes
	// properties to the dialog
	if (proc != 0)
	{
		const QMetaObject* procmeta = proc->metaObject();
		unsigned int nprop = procmeta->propertyCount();
		for (unsigned int p=0; p < nprop; ++p)
		{
			QMetaProperty property = procmeta->property(p);
			this->createPropertyEdit(property, proc);
		}
	}
}

void NMEditModelComponentDialog::createPropertyEdit(const QMetaProperty& property,
		QObject* obj)
{
	NMModelComponent* comp = qobject_cast<NMModelComponent*>(obj);
	NMProcess* proc = qobject_cast<NMProcess*>(obj);

	QString propName = QString(property.name());
	if (QString("objectName").compare(property.name()) == 0)
	{
		if (proc != 0)
			propName = "ProcessName";
		else if (comp != 0)
			propName = "ComponentName";
	}
	int propType = property.userType();

	NMDebugAI(<< propName.toStdString() << " (" << property.typeName()
			<< "): " << obj->property(property.name()).toString().toStdString()
			<< " ...");

	QVariant value;
	bool bok;
	QStringList ctypes;
	ctypes << "uchar" << "char" << "ushort" << "short"
		   << "uint" << "int" << "ulong" << "long"
		   << "float" << "double" << "unknown";

	QStringList parammodes;
	parammodes << "NM_USE_UP" << "NM_CYCLE" << "NM_SYNC_WITH_HOST";

	QtVariantPropertyManager* manager = new QtVariantPropertyManager();
	QtVariantProperty* prop;

	// assign replacement types for the not supported variant
	// types
	if (QString("char").compare(property.typeName()) == 0 ||
		QString("int").compare(property.typeName()) == 0 ||
		QString("short").compare(property.typeName()) == 0 ||
		QString("long").compare(property.typeName()) == 0
	   )
	{
		propType = QVariant::Int;
		value = obj->property(property.name()).toInt(&bok);
		prop = manager->addProperty(propType, propName);
	}
	else if (QString("uchar").compare(property.typeName()) == 0 ||
			QString("uint").compare(property.typeName()) == 0 ||
			QString("ushort").compare(property.typeName()) == 0 ||
			QString("ulong").compare(property.typeName()) == 0
		   )
	{
		value = obj->property(property.name()).toInt(&bok);
		propType = value.type();
		prop = manager->addProperty(propType, propName);
		prop->setAttribute("minimum", 1);
		prop->setAttribute("maximum", QVariant::fromValue(INT_MAX));
	}
	else if (QString("float").compare(property.typeName()) == 0)
	{
		value = obj->property(property.name()).toDouble(&bok);
		propType = value.type();
		prop = manager->addProperty(propType, propName);
		prop->setAttribute("minimum", QVariant::fromValue(
				(std::numeric_limits<float>::max()-1) * -1));
		prop->setAttribute("maximum", QVariant::fromValue(
				std::numeric_limits<float>::max()));
	}
	else if (QString("NMItkDataObjectWrapper::NMComponentType")
			.compare(property.typeName()) == 0)
	{
		propType = QtVariantPropertyManager::enumTypeId();
		prop = manager->addProperty(propType, propName);
		prop->setAttribute("enumNames", ctypes);
		QString curPropValStr = NMItkDataObjectWrapper::getComponentTypeString(
				obj->property(property.name())
				.value<NMItkDataObjectWrapper::NMComponentType>());
		NMDebug(<< "current value = " << curPropValStr.toStdString() << std::endl);
		for (unsigned int p=0; p < ctypes.size(); ++p)
		{
			if (ctypes.at(p).compare(curPropValStr) == 0)
				value = QVariant(p);
		}
	}
#ifdef BUILD_RASSUPPORT
	else if (QString("NMRasdamanConnectorWrapper*")
			.compare(property.typeName()) == 0)
	{
		NMRasdamanConnectorWrapper* wrap = obj->property(property.name())
				.value<NMRasdamanConnectorWrapper*>();

		propType = QVariant::Bool;
		if (wrap != 0)
			value = QVariant(true);
		else
			value = QVariant(false);
		prop = manager->addProperty(propType, propName);
	}
#endif	
	else if (QString("QStringList")
			.compare(property.typeName()) == 0)
	{
		QStringList nakedList = obj->property(property.name()).toStringList();
		QStringList bracedList;
		bracedList << QString("{");
		foreach(QString s, nakedList)
		{
			bracedList << QString(tr("{%1}").arg(s));
		}
		bracedList << QString(tr("}"));

		value = QVariant::fromValue(bracedList);
		prop = manager->addProperty(QVariant::StringList, propName);
	}
	else if (QString("QList<QStringList>").compare(property.typeName()) == 0)
	{
		QList<QStringList> lst = obj->property(property.name()).value<QList<QStringList> >();

		QStringList bracedList;
		bracedList << QString("{");
		foreach(QStringList l, lst)
		{
			bracedList << QString("{");
			foreach(QString s, l)
			{
				bracedList << QString(tr("{%1}").arg(s));
			}
			bracedList << QString(tr("}"));
		}
		bracedList << QString("}");

		value = QVariant::fromValue(bracedList);
		prop = manager->addProperty(QVariant::StringList, propName);
	}
	else if (QString("QList<QList<QStringList> >").compare(property.typeName()) == 0)
	{
		QList<QList<QStringList> > llsl = obj->property(property.name()).value<QList<QList<QStringList> > >();
		QStringList bracedList;

		bracedList << QString("{");
		foreach(QList<QStringList> lsl, llsl)
		{
			bracedList << QString("{");
			foreach(QStringList sl, lsl)
			{
				bracedList << QString("{");
				foreach(QString s, sl)
				{
					bracedList << QString(tr("{%1}").arg(s));
				}
				bracedList << QString(tr("}"));
			}
			bracedList << QString("}");
		}
		bracedList << QString("}");

		value = QVariant::fromValue(bracedList);
		prop = manager->addProperty(QVariant::StringList, propName);
	}
	else if (QString("AdvanceParameter").compare(property.typeName()) == 0)
	{
		propType = QtVariantPropertyManager::enumTypeId();
		//propName = QString("Input Parameter Handling");
		prop = manager->addProperty(propType, propName);
		prop->setAttribute("enumNames", parammodes);
		value = QVariant(obj->property(property.name()).toInt(&bok));
		NMDebugAI(<< "prop2ctrl: ParameterHandling = "
				<< value.toInt(&bok) << endl);
	}
	else
	{
		NMDebug(<< "standard ");
		value = obj->property(property.name());
		prop = manager->addProperty(propType, propName);
	}

	// add property to browser and set value
	if (prop != 0)
	{
		QtVariantEditorFactory* editor = new QtVariantEditorFactory();

		if (!value.isNull())
			prop->setValue(value);

		if (prop->propertyName().compare("ProcessName") == 0 ||
			prop->propertyName().compare("ComponentName") == 0)
			prop->setEnabled(false);

		prop->setToolTip(QString(tr("type: %1")).arg(value.typeName()));

		ui.propBrowser->setFactoryForManager(manager, editor);
		ui.propBrowser->addProperty(prop);

		connect(manager, SIGNAL(valueChanged(QtProperty*, const QVariant &)),
				this, SLOT(applySettings(QtProperty*, const QVariant &)));

		NMDebug(<< " - processed!" << std::endl);
	}
	else
	{
		NMDebug(<< " - failed!" << std::endl);
		delete manager;
	}
}

void NMEditModelComponentDialog::applySettings(QtProperty* prop,
		const QVariant& val)
{
	NMDebugCtx(ctx, << "...");

	NMModelComponent* comp = qobject_cast<NMModelComponent*>(mObj);
	NMProcess* proc = qobject_cast<NMProcess*>(mObj);
	if (comp == 0 && proc == 0)
	{
		NMDebugCtx(ctx, << "done!");
		return;
	}

	if (mObj->property(prop->propertyName().toStdString().c_str()).isValid() ||
		(prop->propertyName().compare("ComponentName")== 0))
	{
		this->setComponentProperty(prop, mObj);
	}
	else if (comp != 0 && comp->getProcess() != 0)
	{
		proc = comp->getProcess();
		if (proc->property(prop->propertyName().toStdString().c_str()).isValid() ||
			(prop->propertyName().compare("ProcessName") == 0))
		{
			this->setComponentProperty(prop, proc);
		}
	}
	else
	{
		this->setComponentProperty(prop, mObj);
	}

	NMDebugCtx(ctx, << "done!");
}

void NMEditModelComponentDialog::setComponentProperty(const QtProperty* prop,
		QObject* obj)
{
	bool bok;
	QStringList ctypes;
	ctypes << "uchar" << "char" << "ushort" << "short"
		   << "uint" << "int" << "ulong" << "long"
		   << "float" << "double" << "unknown";

	QStringList parammodes;
	parammodes << "NM_USE_UP" << "NM_CYCLE" << "NM_SYNC_WITH_HOST";

	QtVariantPropertyManager* manager =
			qobject_cast<QtVariantPropertyManager*>(prop->propertyManager());
	if (manager == 0)
	{
		NMErr(ctx, << "couldn't get the property manager for "
				<< prop->propertyName().toStdString() << "!");
		return;
	}
	QVariant value = manager->value(prop);

	NMDebugAI(<< "setting " << prop->propertyName().toStdString()
			<< " (" << value.typeName() << " = " << value.toString().toStdString()
			<< ") ... ");

	QVariant updatedValue;
	QString propName = prop->propertyName();

	if (QString("ComponentName").compare(prop->propertyName()) == 0 ||
		QString("ProcessName").compare(prop->propertyName()) == 0)
	{
		propName = "objectName";
		updatedValue = value.toString();
	}
//	else if (QString("Subcomponents").compare(prop->propertyName()) == 0)
//	{
//		QStringList bracedList = value.value<QStringList>();
//		QStringList nakedList;
//		for (unsigned int q=0; q < bracedList.size(); ++q)
//		{
//			if (bracedList.at(q).compare("{") ||
//				bracedList.at(q).compare("}"))
//			{
//				continue;
//			}
//			else
//			{
//				QString s = bracedList.at(q);
//				nakedList.push_back(s.mid(1, s.size()-2));
//			}
//		}
//		this->updateSubComponents(nakedList);
//		return;
//	}
#ifdef BUILD_RASSUPPORT
	else if (QString("RasConnector").compare(prop->propertyName()) == 0)
	{
		if (value.toBool())
		{
			if (this->mRasConn == 0)
			{
				NMErr(ctx, << "rasdaman connector requested, but non available!");
				updatedValue.setValue<NMRasdamanConnectorWrapper*>(0);
			}
			else if (this->mRasConn->getConnector() == 0)
			{
				NMErr(ctx, << "rasdaman connector requested, but non available!");
				updatedValue.setValue<NMRasdamanConnectorWrapper*>(0);
			}
			else
				updatedValue.setValue<NMRasdamanConnectorWrapper*>(this->mRasConn);
		}
		else
			updatedValue.setValue<NMRasdamanConnectorWrapper*>(0);
	}
#endif	
	else if (QString("int").compare(value.typeName()) == 0 &&
			prop->propertyName().contains("ComponentType"))
	{
		NMItkDataObjectWrapper::NMComponentType type =
				NMItkDataObjectWrapper::getComponentTypeFromString(ctypes.at(value.toInt(&bok)));
		updatedValue = QVariant::fromValue(type);
	}
	else if (QString("ParameterHandling").compare(prop->propertyName()) == 0)
	{
		int v = value.toInt(&bok);
		NMProcess::AdvanceParameter ap;
		switch(v)
		{
			case 0: ap = NMProcess::NM_USE_UP; break;
			case 1: ap = NMProcess::NM_CYCLE; break;
			case 2: ap = NMProcess::NM_SYNC_WITH_HOST; break;
		}

		//NMDebugAI(<< "ParameterHandling: dlg value = " << v << " | casted value: " << ap << endl);
		updatedValue = QVariant::fromValue(ap);
	}
	else if (QString("QStringList").compare(value.typeName()) == 0)
	{
		NMDebugAI(<< "incoming stringlist: " << value.toStringList().join(" | ").toStdString() << std::endl);
		QStringList bracedList = value.toStringList();
		if (!bracedList.isEmpty())
		{
			if (!bracedList.at(0).startsWith("invalid"))
			{
				updatedValue = this->nestedListFromStringList(bracedList);
			}
		}
	}
	else
	{
		updatedValue = value;
	}

	// do some value checking and set the new value
	if (!updatedValue.isNull() && updatedValue.isValid())
	{
		if (QString("Subcomponents").compare(prop->propertyName()) == 0 &&
			QString("QStringList").compare(updatedValue.typeName()) == 0)
		{
			this->updateSubComponents(updatedValue.value<QStringList>());
		}
		else
		{
			NMDebug(<< "updated type: " << updatedValue.typeName());
			obj->setProperty(propName.toStdString().c_str(), updatedValue);
			NMDebug(<< " done!" << std::endl);
		}
	}
	else
	{
		NMDebug(<< " failed! Invalid value!" << std::endl);
	}
}

void NMEditModelComponentDialog::updateSubComponents(const QStringList& compList)
{
	NMModelComponent* comp = qobject_cast<NMModelComponent*>(mObj);
	if (comp == 0)
	{
		NMDebugCtx(ctx, << "comp is NULL!");
		return;
	}

	QStringList oldCompNames;
	QList<NMModelComponent*> oldComps;

	NMModelComponent* sub = comp->getInternalStartComponent();
	while(sub != 0)
	{
		oldCompNames.push_back(sub->objectName());
		sub = comp->getNextInternalComponent();
	}

	// remove all components
	// and add store them in a list
	foreach(QString ds, oldCompNames)
	{
		oldComps.append(comp->removeModelComponent(ds));
	}

	// add components in new order, make sure we don't add any new
	// components
	foreach(QString us, compList)
	{
		int idx = oldCompNames.indexOf(us);
		if(idx >=0)
		{
			comp->addModelComponent(oldComps.value(idx));
		}
	}
}

QVariant NMEditModelComponentDialog::nestedListFromStringList(const QStringList& strList)
{
	QVariant val;

	// determine the max depth (level) of the stringlist
	int allowedlevel = 3;
	int maxlevel = 0;
	int levelcounter = 0;
	foreach(QString ts, strList)
	{
		if (ts.compare("{") == 0)// || ts.startsWith("{"))
		{
			++levelcounter;
			maxlevel = levelcounter > maxlevel && levelcounter <= allowedlevel ? levelcounter : maxlevel;
		}
		else if (ts.compare("}") == 0)// || ts.endsWith("}"))
		{
			--levelcounter;
		}
	}
//	NMDebugAI( << "detected maxlevel: " << maxlevel << std::endl);
//	NMDebugAI( << "parsing list ..." << std::endl << std::endl);

	QList<QList<QStringList> >* llsl;
	QList<QStringList>* lsl;
	QStringList* sl;

	levelcounter = maxlevel;
	foreach(QString ts, strList)
	{
//		NMDebugAI(<< "'" << ts.toStdString() << "' on level " << levelcounter);
		if (ts.compare("{") == 0)
		{
			switch (levelcounter)
			{
			case 3:
//				NMDebug(<< " -> new llsl" << std::endl);
				llsl = new QList<QList<QStringList> >();
				break;
			case 2:
//				NMDebug(<< " -> new lsl" << std::endl);
				lsl = new QList<QStringList>();
				break;
			case 1:
//				NMDebug(<< " -> new sl" << std::endl);
				sl = new QStringList();
				break;
			default:
//				NMDebug(<< " - > can't handle this one!" << std::endl);
				continue;
				break;
			}
			--levelcounter;
		}
		else if (ts.compare("}") == 0)
		{
			switch (levelcounter)
			{
			case 0:
				if (maxlevel > 1)
				{
//					NMDebug(<< " -> close ")
					lsl->push_back(*sl);
					sl->clear();
					delete sl;
				}
				break;
			case 1:
				if (maxlevel > 2)
				{
					llsl->push_back(*lsl);
					lsl->clear();
					delete lsl;
				}
				break;
			default:
				break;
			}
			++levelcounter;
		}
		else if (ts.startsWith("{")
				 && ts.endsWith("}")
				   && levelcounter == 0)
		{
			QString ns = ts.mid(1,ts.size()-2); //ts.remove(0,1);
			sl->push_back(ns);
//			NMDebugAI( << ">>> pushed '" << ns.toStdString() << "'" << std::endl);
		}
	}

	switch(maxlevel)
	{
	case 3:
		val = QVariant::fromValue(*llsl);
		llsl->clear();
		delete llsl;
		break;
	case 2:
		val = QVariant::fromValue(*lsl);
		lsl->clear();
		delete lsl;
		break;
	case 1:
		val = QVariant::fromValue(*sl);
		sl->clear();
		delete sl;
		break;
	default:
		break;
	}

	return val;
}

//void NMEditModelComponentDialog::updateSubComponents(const QStringList& compList)
//{
//
//}

