/*                    
	This file is part of the rimageio library for
	the orfeo tool box
	
*/

#ifndef __TEMPLATE_INST_RIMAGEIO_
#define __TEMPLATE_INST_RIMAGEIO_

//// RASLIB INCLUDES ------------------------------------------------
#include <rasodmg/ref.hh>
#include <raslib/primitivetype.hh>
#include <raslib/pointtype.hh>


//// RASDL INCLUDES ------------------------------------------------
//#include <qlparser/symtab.hh>
//  
//#include <raslib/itertype.hh>
//#include <raslib/attribute.hh>
//#include <raslib/dlist.hh>
//
//#include <indexmgr/keyobject.hh>
//
//#include <reladminif/dbref.hh>
//#include <reladminif/dbobjectiterator.hh>
//#include <reladminif/dbobjectiditerator.hh>
//#include <reladminif/dbobject.hh>
//
//#include <relblobif/dbtile.hh>
//#include <relblobif/blobtile.hh>
//#include <relblobif/inlinetile.hh>
//
//#include <relcatalogif/structtype.hh>
//#include <relcatalogif/settype.hh>
//#include <relcatalogif/mddtype.hh>
//#include <relcatalogif/dbminterval.hh>
//
//#include <relindexif/hierindex.hh>
//#include <relindexif/dbrcindexds.hh>
//#include <relindexif/dbtcindex.hh>
//
//#include <relmddif/dbmddobj.hh>
//#include <relmddif/dbmddset.hh>
//
//#include <relstorageif/dbstoragelayout.hh>


//// RASLIB INSTANTIATION ------------------------------------
template class r_Ref< r_Primitive_Type >;
template class r_Ref< r_Primitive >;
template class r_Ref< r_Point >;
template class r_Ref< r_Type >;
template class r_Ref< r_Char >;
//template class r_Ref< r_Set< r_GMarray > >;
template class r_Set< r_Ref < r_Marray<r_Char> > >;
//template class r_Set< r_Ref < r_Marray<r_Boolean> > >; 
template class r_Set< r_Ref < r_Marray<r_ULong> > >;
template class r_Set< r_Ref < r_Marray<r_UShort> > >;
template class r_Set< r_Ref < r_Marray<r_Long> > >;
template class r_Set< r_Ref < r_Marray<r_Short> > >;
template class r_Set< r_Ref < r_Marray<r_Octet> > >;
template class r_Set< r_Ref < r_Marray<r_Double> > >;
template class r_Set< r_Ref < r_Marray<r_Float> > >;


//template class TypeIterator< MDDType >;
//template class DBRef< MDDType >;

template class r_Marray< char>;
template class r_Ref< r_Marray < char > > ;

template class r_Marray<r_UShort>;
template class r_Marray<r_Long>;
template class r_Marray<r_Octet>;
template class r_Marray<r_Double>;
template class r_Marray<r_Float>;
template class r_Marray<r_Short>;


/// RASDL INSTANTIATON -----------------------------------------
//template class DBObjectIterator<SetType>;
//template class DBObjectIterator<StructType>;
//template class DBObjectIterator<MDDType>;
//template class DBObjectIterator<DBMDDObj>;
//template class DBObjectIdIterator<DBMDDObj>;
//template bool operator< (const DBRef<DBMDDObj>&, const DBRef<DBMDDObj>&);
//
//template class DBRef<DBTile>;
//template class DBRef<InlineTile>;
//template class DBRef<BLOBTile>;
//template class DBRef<DBObject>;     
//template class DBRef<DBMDDObj>;
//template class DBRef<DBMDDSet>;
//template class DBRef<DBHierIndex>;
//template class DBRef<DBTCIndex>;
//template class DBRef<DBRCIndexDS>;
//template class DBRef<DBMinterval>;
//template class DBRef<StructType>;
//template class DBRef<MDDType>;
//template class DBRef<SetType>;
//template class DBRef<DBStorageLayout>;

// double instantiation
//template class r_IterType<r_Attribute>;

//template class SymbolTable<int>;
//
//template std::ostream& operator<< (const std::vector<KeyObject>&, std::ostream&);
//template std::ostream& operator<< (std::ostream &, const std::vector<KeyObject>&);
	// double instantiation
	//template std::ostream& operator<< (std::ostream &, const std::vector<double>&);


#endif
