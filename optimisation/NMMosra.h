/* NMMosra.h
 *
 * Copyright 2011 Alexander Herzig
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*!
 * \brief NMMosra implements multi-objective spatial resource allocation.
 *
 * NMMosra assumes resources are allocated to spatial units with an area > 0
 * (i.e. polygons, raster (not implemented yet)), which are provided by
 * an instance of NMLayer. Using this class involves the following recommended
 * sequence of member calls:
 *
 * 1. create the object: NMMosra* mosra = new NMMosra();
 * 2. (optional) set the time out : mosroa->setTimeOut(timeout);
 * 3. load the problem configuration: mosra->loadSettings(myLOSfile);
 * 4. set the specified NMLayer instance: mosra->setLayer(myNMLayer);
 * 5. solve the problem: mosra->solveLp();
 * 6. if 5. was successful, extract results and write them into the layer: mosra->mapLp()
 * 7. (optional) summarise the results: mosra->sumResults();
 * 8. (optional) write a report: mosra->writeReport();
 *
 * Additionally, you may want to export the formulated problem as an LP-file
 * (mosra->getLp()->WriteLp(filename)) or you may want to reflect the results
 * in your layer by calling layer->mapUniqueValues("OPT_STR").
 *
 *
 */

#ifndef NMMOSRA_H_
#define NMMOSRA_H_

#include "nmlog.h"
#define ctxNMMosra "NMMosra"

#include <QObject>
#include <QMap>
#include <QStringList>
//#include <QStandardItemModel>

#include "LpHelper.h"
//#include "NMLayer.h"

#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
//#include "vtkQtTableModelAdapter.h"

class NMMosra : public QObject
{
	Q_OBJECT
	Q_ENUMS(NMMosoDVType)
	Q_ENUMS(NMMOsoScalMeth)


public:
	NMMosra(QObject* parent=0);
	virtual ~NMMosra();

	// decision variable type
	enum NMMosoDVType {NM_MOSO_REAL, NM_MOSO_INT,
		NM_MOSO_BINARY};

	// scalarisation method
	enum NMMosoScalMeth {NM_MOSO_WSUM, NM_MOSO_INTERACTIVE};

	// area unit type
	enum AreaUnitType {NM_MOSO_MAP_UNITS, NM_MOSO_PERCENT_TOTAL,
		NM_MOSO_PERCENT_SELECTED, NM_MOSO_PERCENT_ZONE};

	// load settings from textfile
	// (from LUMASS Optimisation settings file)
	int loadSettings(QString fileName);

	QString getLayerName(void);

	void setDataSet(const vtkDataSet* dataset);
	const vtkDataSet* getDataSet()
		{return this->mDataSet;}
	vtkSmartPointer<vtkTable> getDataSetAsTable();

	void cancelSolving(void) {this->mbCanceled = true;};
	int solveLp(void);
	int mapLp(void);
	HLpHelper* getLp();
	vtkSmartPointer<vtkTable> sumResults(void);

	// enquire batch processing set up
	bool doBatch(void);
	QString getDataPath(void)
		{return this->msDataPath;}
	QStringList getPerturbationItems()
		{return this->mslPerturbItems;}
	QList<float> getUncertaintyLevels()
		{return this->mflUncertainties;}
	long getNumberOfPerturbations()
		{return this->mlReps;}
	QString getLosFileName(void)
		{return this->msLosFileName;}
	unsigned int getTimeOut(void)
		{return this->muiTimeOut;}

	void writeReport(QString fileName);
	QString getReport(void);

	/* set a solver timeout -> maximum availalbe time for solving
	 * the problem
	 */
	void setTimeOut(int secs)
		{this->muiTimeOut = secs >= 0 ? secs : 0;};

	/*	\brief add uncertainty to performance scores
	 *
	 *  This function varies the individual performance scores by
	 *  the uncertainty given as 'percent'. It takes the scores for
	 *  each individual land use of the criterion, and randomly
	 *  (uniform distribution) adds an uncertainty value
	 *  of +/- 0 to percent.
	 */
	void perturbCriterion(const QString& criterion, float percent);

	/* \brief varies a constraint by a given percent
	 *
	 *  This function varies the named criterion by the given
	 *  percentage.
	 */
	void varyConstraint(const QString& constraint, float percent);

	/* lp_solve callback function to check for user abortion ->
	 * i.e. interactive cancellation of solving process rather
	 * than a time one
	 */
	static int callbackIsSolveCanceled(lprec* lp, void* userhandle);



private:

	bool mbCanceled;

	HLpHelper* mLp;
	vtkDataSet* mDataSet;

	QString msReport;
	QString msSettingsReport;
	QString msLosFileName;

	QString msLandUseField;
	QString msAreaField;
	QString msLayerName;

	QString msDataPath;
	QStringList mslPerturbItems;
	QList<float> mflUncertainties;
	long mlReps;

	NMMosoDVType meDVType;
	NMMosoScalMeth meScalMeth;

	unsigned int muiTimeOut;
	int miNumOptions;
	QStringList mslOptions;

	// <objective> <min | max> <weight>
	QMap<QString, QStringList> mmslObjectives;

	// maps criterion performance measures onto criterion name
	// <criterion name>, <performance of option 1> [ <performance of option 2> ...]
	// option performance fields have to be given in the same order
	// as in the mlsOptions list
	QMap<QString, QStringList> mmslCriteria;

	// as above but this map contains per criterion the data fields which
	// are going to be used to evaluate status quo and optimised
	// performance
	QMap<QString, QStringList> mmslEvalFields;

	// maps areal constraints onto options
	// <constr label>, <option[:zonefield]> < >= | <= > < number > < percent_of_total | percent_of_selected | map_units >
	QMap<QString, QStringList> mmslAreaCons;
	QMap<QString, QStringList> mmslAreaZoneCons;
	QStringList mmslAreaConsLabel;

	/*! nested map; holds for each zone field the maximum available area for each specified
	 *  (combination of) land use options */
	QMap<QString, QMap<QString, double > > mmslZoneAreas;
	QMap<QString, QMap<QString, int > > mmslZoneLength;


	// maps criterion constraints onto criteria
	// nested maps:
	// outer (multi-)map:
	// 		key:   <criterion label>   value: <land use field map>
	// inner map (land use field map):
	// 		key: <land use label | "total">  value: <if 'land use': land use performance field>
	//				        		                <if 'total': list of all land use performance fields>
	//		 										 < >= | <= | = > < criterion cap >
	QMultiMap<QString, QMap<QString, QStringList > > mmslCriCons;

	// maps objective constraints onto objectives
	// <objective>, < >= | <= > < number >
	QMap<QString, QStringList> mmslObjCons;

	long mlNumDVar;
	long mlNumArealDVar;
	long mlNumOptFeat;
	long mlCriCons;
	long mlLpCols;
	double mdAreaTotal;
	double mdAreaSelected;

	// reset internal variables
	void reset(void);
	void createReport(void);

	int checkSettings(void);

	int makeLp(void);
	int addObjFn(void);
	int addObjCons(void);
	int addExplicitAreaCons(void);
	int addImplicitAreaCons(void);
	int addCriCons(void);

	int isSolveCanceled(void);

	/* converts the user specified area into area units
	  (same units as the specified area field) */
	double convertAreaUnits(double input, AreaUnitType otype);
	double convertAreaUnits(double input, const QString& otype, const QStringList& zoneSpec);

};

#endif /* NMMOSRA_H_ */
