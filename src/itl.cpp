//---------------------------------------------------------------------------
//
// itl wrappers, callable from C, C++, and Fortran
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
// Copyright Notice
// + 2010 University of Chicago
//
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// preprocessors

	//! The maximal dimension of blocks. 4 should be sufficient for most scientific data
	#define MAX_BLOCK_DIM	4

//--------------------------------------------------------------------------
// ITL-relarted headers
	#include "ITL_header.h"
	#include "ITL_base.h"
	#include "ITL_ioutil.h"
	#include "ITL_vectormatrix.h"
	#include "ITL_localentropy.h"
	#include "ITL_globalentropy.h"
	#include "ITL_localjointentropy.h"
	#include "ITL_field_regular.h"

	// ADD-BY-LEETEN 07/22/2011-BEGIN
	#include "ITL_random_field.h"
	// ADD-BY-LEETEN 07/22/2011-END

	// header for the wrapper
	#include "itl.h"

	// headers from my own libraries (mylib)
	#include "liblog.h"
	#include "libbuf3d.h"

	//! The rank of the current process
	static int iRank;

	#include "ITL_random_field.h"

	char szGlobalEntropyLogPathFilename[1024];

	ITLRandomField *pcBoundRandomField;

	// ADD-BY-LEETEN 08/05/2011-BEGIN
	const char DEFAULT_DUMP_PATH[] = "dump";
	static int iTimeStamp = 0;
	static char szDumpPath[1024];
	static char szCommand[1024];	// buffer to hold the command for the func. system()
	// ADD-BY-LEETEN 08/05/2011-END
	
	// ADD-BY-LEETEN 08/06/2011-BEGIN
	//! Name of the current test, which is specified via ITL_begin/itl_begin_
	static char szName[1024];
	// ADD-BY-LEETEN 08/06/2011-END

//--------------------------------------------------------------------------
// functions

// ADD-BY-LEETEN 08/06/2011-BEGIN
static
const char* SZConvert2CStr
(
	const char *szStr
)
{
	static char szTemp[1024];
	strncpy(szTemp, szStr, sizeof(szTemp));
	for(int i = 0; i < (int)sizeof(szTemp); i++)
		if(! ( ('0' <= szTemp[i] && szTemp[i] <= '9') ||
			('a' <= szTemp[i] && szTemp[i] <= 'z') ||
			('A' <= szTemp[i] && szTemp[i] <= 'Z') ||
			'.' == szTemp[i] ||
			'-' == szTemp[i] ||
			'_' == szTemp[i] ) )
		{
			szTemp[i] = '\0';
			break;
		}
	return szTemp;
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to create the NetCDF file
/*!
*/
void
ITL_nc_create
(
)
{
	pcBoundRandomField->_CreateNetCdf
	(
		::szDumpPath,
		::szName
	);
}

//! The Fortran API to create the NetCDF file
/*!
\sa ITL_nc_create
*/
extern "C"
void
itl_nc_create_
(
)
{
	ITL_nc_create();
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to dump the geometry of the bound block to the NetCDF file
/*!
\param	szGeomPathFilename path/filename of the file
*/
void
ITL_nc_wr_geom
(
)
{
	for(int b = 0; b < ::pcBoundRandomField->IGetNrOfBlocks(); b++)
	{
		::pcBoundRandomField->_DumpBlockGeometry2NetCdf(b);
	}
}

//! The Fortran API to dump the geometry of the bound block to a default file
/*!
 * \sa ITL_nc_dump_bblk_geom
*/
extern "C"
void
itl_nc_wr_geom_
(
)
{
	ITL_nc_wr_geom();
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to dump the data of all blocks 
/*!
*/
void
ITL_nc_wr_data
(
)
{
  ::pcBoundRandomField->_DumpData2NetCdf();
}

//! The Fortran API to dump the geometry of the bound block to a default file
/*!
 * \sa ITL_nc_wr_data
*/
extern "C"
void
itl_nc_wr_data_
(
)
{
	ITL_nc_wr_data();
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to dump the data of all blocks 
/*!
*/
void
ITL_nc_wr_rv
(
 const int iRvId
)
{
  ::pcBoundRandomField->_DumpRandomSamples2NetCdf
    (
     iRvId
     );
}

//! The Fortran API to dump the geometry of the bound block to a default file
/*!
 * \sa ITL_nc_wr_rv
*/
extern "C"
void
itl_nc_wr_rv_
(
 int *piRvId
)
{
	ITL_nc_wr_rv(
		     *piRvId-1 // from 1-based to 0-based ID
		     );
}
// ADD-BY-LEETEN 08/06/2011-END

//! The C/C++ API to initialize ITL
/*!
 *
 *\sa iNameLength	Length of the given string szName
 *\sa szName		Name of the current random field
 *\sa
*/
void
// MOD-BY-LEETEN 08/05/2011-FROM:
	// ITL_begin()
// TO:
  ITL_begin
  (
   const int iNameLength,
   const char* szName
  ) 
// MOD-BY-LEETEN 08/05/2011-END
{
	// Get the rank of the current processors
	MPI_Comm_rank(MPI_COMM_WORLD, &::iRank);

	// Initialize ITL
	ITL_base::ITL_init();

	#if	0	// DEL-BY-LEETEN 12/02/2011-BEGIN
		// Initialize histogram
		// "!" means the default patch file
		ITL_histogram::ITL_init_histogram( "!" );
	#endif	// DEL-BY-LEETEN 12/02/2011-END

	// create a folder to hold the tmp. dumpped result
	strcpy(::szName, SZConvert2CStr(szName));

	if( iNameLength <= 0 )
		sprintf(::szDumpPath, "%s", DEFAULT_DUMP_PATH);
	else
		sprintf(::szDumpPath, "%s/%s", DEFAULT_DUMP_PATH, SZConvert2CStr(szName));

	if( 0 == iRank )
	{
	        // MOD-BY-LEETEN 08/29/2011-FROM:
	  // system("mkdir dump");
	        // TO:
		sprintf(::szCommand, "mkdir %s", DEFAULT_DUMP_PATH);
		system(::szCommand);
	        // MOD-BY-LEETEN 08/29/2011-FROM:

		sprintf(::szCommand, "rm -r %s", ::szDumpPath);
		system(::szCommand);

		sprintf(::szCommand, "mkdir %s", ::szDumpPath);
		system(::szCommand);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	// create the path/filename of the log for global entropy
	// MOD-BY-LEETEN 08/05/2011-FROM:
		// sprintf(szGlobalEntropyLogPathFilename, "dump/ge.rank_%d.log", ::iRank);
	// TO:
	sprintf(szGlobalEntropyLogPathFilename, "%s/ge.rank_%d.log", ::szDumpPath, ::iRank);
	// MOD-BY-LEETEN 08/05/2011-END

	// register ITL_end to be called at the end of execution (in case the application does not call it)
	atexit(ITL_end);
}

//! The Fortran API to initialize ITL
/*!
 * \sa ITL_begin
*/
extern "C"
void
#if 0 // MOD-BY-LEETEN 08/05/2011-FROM:
	itl_begin_()
	{
		ITL_begin();
	}
#else	// MOD-BY-LEETEN 08/05/2011-TO:
itl_begin_
(
 int *piNameLength,
 char *szName
 )
{
  ITL_begin
    ( 
     *piNameLength,
     szName
      );
}
#endif	// MOD-BY-LEETEN 08/05/2011-END

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to terminate ITL
/*!
 *
*/
void
ITL_end
(
)
{
  if( pcBoundRandomField ) // ADD-By-LEETEN 08/29/2011
	// ADD-BY-LEETEN 08/06/2011-BEGIN
	pcBoundRandomField->_CloseNetCdf();
	// ADD-BY-LEETEN 08/06/2011-END
}

//! The Fortran API to free ITL
/*!
 * \sa ITL_end
*/
extern "C"
void itl_end_
(
)
{
	ITL_end();
}

// ADD-BY-LEETEN 08/12/2011-BEGIN
/////////////////////////////////////////////////////////////////////
//! The C/C++ API to specify the mapping between global and local block IDs
/*!
\sa	int iNrOfGlobalBlocks,
\sa	const int piLocal2GlobalMapping[],
 *
*/
void
ITL_set_local2global_mapping
(
	const int piLocal2GlobalMapping[],
	const bool bIs1Based
)
{
	::pcBoundRandomField->_MapBlock2GlobalId
	(
			piLocal2GlobalMapping,
			bIs1Based
			);
}

//! The Fortran API to free ITL
/*!
 * \sa ITL_set_local2global_mapping
*/
extern "C"
void itl_set_local2global_mapping_
(
	int *piLocal2GlobalMapping,
	int *is1Based
)
{
	ITL_set_local2global_mapping
	(
			piLocal2GlobalMapping,
			(1 == *is1Based)?true:false
			);
}
// ADD-BY-LEETEN 08/12/2011-END

// ADD-BY-LEETEN 08/05/2011-BEGIN
/////////////////////////////////////////////////////////////////////
//! The C/C++ API to specify the time stamp
/*!
 * \sa iTimeStamp	the specified time stamp
*/
void
ITL_set_time_stamp
(
	const int iTimeStamp
)
{
  ::iTimeStamp = iTimeStamp;
	// ADD-BY-LEETEN 08/06/2011-BEGIN
  ::pcBoundRandomField->_AddTimeStamp(iTimeStamp);
	// ADD-BY-LEETEN 08/06/2011-END
}

//! The Fortran API to automatically decide the random variable's range
/*!
 * \param	piRandomVariable	Pointer to the index (1-based) of the random variable
 * \sa ITL_set_time_stamp 
*/
extern "C"
void
itl_set_time_stamp_
(
	int *piTimeStamp
)
{
	ITL_set_time_stamp 
	(
		*piTimeStamp
	);
}
// ADD-BY-LEETEN 08/05/2011-END

// ADD-BY-LEETEN 07/22/2011-BEGIN
/////////////////////////////////////////////////////////////////////
//! The C/C++ API to automatically decide a random variable's range
/*!
 * \sa iRandomVariable	ID (0-based) of the random variable
*/
void
ITL_use_domain_range
(
	const int iRandomVariable
)
{
	pcBoundRandomField->_UseDomainRange(iRandomVariable);
}

//! The Fortran API to automatically decide the random variable's range
/*!
 * \param	piRandomVariable	Pointer to the ID (1-based) of the random variable
 * \sa ITL_use_domain_range
*/
extern "C"
void
itl_use_domain_range_
(
	int *piRandomVariable
)
{
	ITL_use_domain_range
	(
		*piRandomVariable - 1
	);
}
// ADD-BY-LEETEN 07/22/2011-END

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to add a random field
/*!
\param	iNrOfBlocks				#blocks of the current process
\param	iNrOfDataComponents		#data components to be used for entropy computation
\param	piRfId					Pointer to the ID (0-based) of the created random field
*/
void
ITL_add_random_field
(
	const int iNrOfBlocks,
	const int iNrOfDataComponents,
	int *piRfId
)
{
	pcBoundRandomField = new ITLRandomField;
	pcBoundRandomField->_Create(iNrOfBlocks, iNrOfDataComponents);
	*piRfId = 0;
}

//! The Fortran API to free ITL
/*!
 * \sa ITL_add_random_field
*/
extern "C"
void
itl_add_random_field_
(
	int *piNrOfBlocks,
	int *piNrOfDataComponents,
	int *piRfId
)
{
	int iRfId;
	ITL_add_random_field
	(
		*piNrOfBlocks,
		*piNrOfDataComponents,
		&iRfId
	);
	*piRfId = iRfId + 1;
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to bind a random field
/*!
\param	iRfId	ID (0-based) of the random field
*/
void
ITL_bind_random_field
(
	const int iRfId
)
{
}

//! The Fortran API to bind a random field
/*!
\param	piRfId	Pointer to the ID (1-based) of the random field
 * \sa ITL_bind_random_field
*/
extern "C"
void
itl_bind_random_field_
(
	int *piRfId
)
{
	ITL_bind_random_field
	(
		*piRfId - 1	// 1-based to 0-based
	);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to bind a block.
/*!
\param iBlockId 	ID (0-based) of the block
*/
void
ITL_bind_block
(
	const int iBlockId
)
{
	pcBoundRandomField->_BindBlock(iBlockId);
}

//! The Fortran API to bind the block.
/*!
\param piBlockId 	Pointer to the ID (1-based) of the block
\sa ITL_bind_block
*/
extern "C"
void
itl_bind_block_
(
	int *piBlockId
)
{
	ITL_bind_block(
		*piBlockId - 1	// convert the block Id from 1-based (Fortran) to C (0-based)
	);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to specify the block size
/*!
\param iDim 		Dimension of the block
\param pikDimLengths
					The length of each dimension. The size should be
					equal to iDim. If length is large than CBlock::MAX_DIM,
					those elements exceed CBlock::MAX_DIM will be ignored.
\sa   CBlock::MAX_DIM
*/
void
ITL_block_size
(
	const int iDim,
	const int piDimLengths[]
)
{
	pcBoundRandomField->CGetBoundBlock()._SetBlockSize(iDim, piDimLengths);
}

//! The Fortran API to specify the block size
/*!
\sa   ITL_block_size
*/
extern "C"
void
itl_block_size_
(
	int *piDim,
	int *piDimLengths
)
{
	ITL_block_size
	(
		*piDim,
		piDimLengths
	);
}

//! The Fortran API to specify the size for 2D block
/*!
\sa   ITL_block_size
*/
extern "C"
void
itl_block_size2_
(
	int *piXLength,
	int *piYLength
)
{
	int piDimLengths[2];
	piDimLengths[0] = *piXLength;
	piDimLengths[1] = *piYLength;

	ITL_block_size
	(
		2,
		piDimLengths
	);
}

//! The Fortran API to specify the size for 3D block
/*!
\sa   ITL_block_size
*/
extern "C"
void
itl_block_size3_
(
	int *piXLength,
	int *piYLength,
	int *piZLength
)
{
	int piDimLengths[3];
	piDimLengths[0] = *piXLength;
	piDimLengths[1] = *piYLength;
	piDimLengths[2] = *piZLength;

	ITL_block_size
	(
		3,
		piDimLengths
	);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to bind a data component
/*!
\param iBlockId 	ID (0-based) of the data component
*/
void
ITL_bind_data_component
(
	const int iDataComponent
)
{
	pcBoundRandomField->_BindDataComponent(iDataComponent);
}

//! The Fortran API to bind the block.
/*!
\param piBlockId 	Pointer to the ID (1-based) of the data component
\sa ITL_bind_data_component
*/
extern "C"
void
itl_bind_data_component_
(
	int *piDataComponent
)
{
	ITL_bind_data_component(
		*piDataComponent - 1	// convert the Id from 1-based (Fortran) to C (0-based)
	);
}

// ADD-BY-LEETEN 08/06/2011-BEGIN
/////////////////////////////////////////////////////////////////////
//! The C/C++ API to specify the name of the bound data component
void
ITL_data_name
(
		const char *szName
)
{
  char szDataName[1024];
  sprintf(szDataName, "data_%s", szName);
  pcBoundRandomField->CGetBoundDataComponent()._SetName(szDataName);
}

//! The Fortran API to specify the name of the bound data component
extern "C"
void
itl_data_name_
(
		char *szName
)
{
	ITL_data_name(
			SZConvert2CStr(szName)
			);
}
// ADD-BY-LEETEN 08/06/2011-END

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to specify the range of the bound data component
/*!
\param dMin
\param dMax
*/
void
ITL_data_range
(
	const double dMin,
	const double dMax
)
{
	pcBoundRandomField->CGetBoundDataComponent().cRange._Set(dMin, dMax);
}

//! The Fortran API to specify the range of the bound data component
/*!
\sa ITL_data_range
*/
extern "C"
void
itl_data_range_
(
	double *pdMin,
	double *pdMax
)
{
	ITL_data_range
	(
		*pdMin,
		*pdMax
	);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to specify the data source of the bound data component
/*!
\param	pdData		The array of the data
\param	iBase		The ID (0-based) of the 1st element
\param	iStep		The distance between consecutive elements
*/
void
ITL_data_source
(
	const double pdData[],
	const int iBase,
	const int iStep
)
{
	pcBoundRandomField->_SetBoundArray
	(
		pdData,
		iBase,
		iStep
	);
}

//! The Fortran API to specify the data source of the bound data component
/*!
\param	piBase		Pointer to the ID (1-based) of the 1st element
\sa ITL_data_source
*/
extern "C"
void
itl_data_source_
(
	double *pdData,
	int *piBase,
	int *piStep
)
{
	ITL_data_source
	(
		pdData,
		*piBase - 1,	// 1-based to 0-based
		*piStep
	);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to specify the coordinates along one dim. for regular grid
/*!
 * \param iDimId	ID (0-based) of the block dimension
 * \param pdCoord	the array of the coordinates along the specified block dim
 * \param iBase		the 1st element (0-based) in the pool
 * \param iStep		the difference between the consecutive elements
*/
void
ITL_geom_rect_dim_coord
(
	const int iDimId,
	double *pdCoord,
	const int iBase,
	const int iStep
)
{
	::pcBoundRandomField->CGetBoundBlock().cGeometry._SetDimCoords(iDimId, pdCoord, iBase, iStep);
}

//! The Fortran API to specify whether the vector orientation is used as the random variable
/*!
 * \param *piDimId	Pointer to the ID (1-based) of the block dimension
 * \param *piBase	Pointer to the 1st element (1-based) in the array
 *
 * \sa	ITL_geom_rect_dim_coord
*/
extern "C"
void
itl_geom_rect_dim_coord_
(
	int *piDimId,
	double *pdCoord,
	int *piBase,
	int *piStep
)
{
	ITL_geom_rect_dim_coord
	(
		*piDimId - 1,	// from 1-based to 0-based index
		pdCoord,
		*piBase - 1,	// from 1-based to 0-based index
		*piStep
	);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to add a random variable
/*!
 * \param	piRvId		ID (0-based) of the new created random variable
*/
void
ITL_add_random_variable
(
	int *piRvId
)
{
	pcBoundRandomField->_AddRandomVariable(piRvId);
}

//! The Fortran API to add a random variable
/*!
 * \param	piRvId		Pointer to the ID (1-based) of the new created random variable
\sa ITL_add_random_variable
*/
extern "C"
void
itl_add_random_variable_
(
	int *piRvId
)
{
	int iRvId;
	ITL_add_random_variable
	(
		&iRvId
	);
	*piRvId = iRvId + 1;	// from 0-based to 1-based
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to bind a random variable
/*!
 * \param	iRvId		ID (0-based) of the random variable
*/
void
ITL_bind_random_variable
(
	const int iRvId
)
{
	pcBoundRandomField->_BindRandomVariable(iRvId);
}

//! The Fortran API to add a random variable
/*!
 * \param	piRvId		Pointer to the ID (1-based) of the random variable
 * \sa 		ITL_bind_random_variable
*/
extern "C"
void
itl_bind_random_variable_
(
	int *piRvId
)
{
	ITL_bind_random_variable
	(
		*piRvId - 1	// from 1-based to 0-based
	);
}

// ADD-BY-LEETEN 08/06/2011-BEGIN
/////////////////////////////////////////////////////////////////////
//! The C/C++ API to set the name of the bound random variable
/*!
 * \param	piRvId		ID (0-based) of the new created random variable
*/
void
ITL_rv_name
(
	const char* szName
)
{
	char szRvName[1024];
	sprintf(szRvName, "rv_%s", szName);
	pcBoundRandomField->CGetBoundRandomVariable()._SetName(szRvName);
}

//! The Fortran API to add a random variable
/*!
 * \param	piRvId		Pointer to the ID (1-based) of the new created random variable
\sa ITL_rv_name
*/
extern "C"
void
itl_rv_name_
(
	char* szName
)
{
	ITL_rv_name
	(
		SZConvert2CStr(szName)
	);
}
// ADD-BY-LEETEN 08/06/2011-END

// ADD-BY-LEETEN 07/22/2011-BEGIN
/////////////////////////////////////////////////////////////////////
//! The C/C++ API to bind a random variable
/*!
 * \param	iRvId		ID (0-based) of the random variable
*/
void
ITL_set_random_variable_range
(
	const double dMin,
	const double dMax
)
{
	pcBoundRandomField->CGetBoundRandomVariable().cRange._Set(dMin, dMax);
}

//! The Fortran API to add a random variable
/*!
 * \param	piRvId		Pointer to the ID (1-based) of the random variable
 * \sa 		ITL_bind_random_variable
*/
extern "C"
void
itl_set_random_variable_range_
(
	double *pdMin,
	double *pdMax
)
{
	ITL_set_random_variable_range
	(
			*pdMin,
			*pdMax
	);
}
// ADD-BY-LEETEN 07/22/2011-END

// ADD-BY-LEETEN 07/31/2011-BEGIN
/////////////////////////////////////////////////////////////////////
//! The C/C++ API to specify the #bins of the bound random variable
/*!
 * \param	iNrOfBins	#bins
*/
void
ITL_set_n_bins
(
	const int iNrOfBins
)
{
  pcBoundRandomField->CGetBoundRandomVariable()._SetNrOfBins(iNrOfBins);
}

//! The Fortran API to specify the #bins of the bound random variable
/*!
 * \sa 		ITL_set_n_bins
*/
extern "C"
void
itl_set_n_bins_
(
	int *piNrOfBins
)
{
	ITL_set_n_bins
	(
	 *piNrOfBins
	);
}
// ADD-BY-LEETEN 07/31/2011-END

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to set up the feature vector of the current random variable
/*!
\param	iFeatureLength			Length of the feature vector
\param	piFeatureVector			The array of indices (0-based) to the data components
\param	bIsUsingOrientation		A flag whether the vector orientation is used as the random variable (true) or the magnitude (false)
*/
void
ITL_random_varable_set_feature_vector
(
	const int iFeatureLength,
	const int piFeatureVector[],
	const int iFeatureMapping
)
{
	pcBoundRandomField->_SetFeatureVector
	(
		iFeatureLength,
		piFeatureVector,
		iFeatureMapping
	);
}

//! The Fortran API to set up the feature vector of the current random variable
/*!
\param	piFeatureVector			The array of indices (1-based) to the data components
\sa 	ITL_random_varable_set_feature_vector
*/
extern "C"
void
itl_random_varable_set_feature_vector_
(
	int *piFeatureLength,
	int *piFeatureVector,
	int *piIsUsingOrientation
)
{
	// convert the indices from 1-based to 0-based
	int iFeatureLength = *piFeatureLength;
	TBuffer<int> piFeatureVector_0based;
	piFeatureVector_0based.alloc(iFeatureLength);
	for(int f = 0; f < iFeatureLength; f++)
		piFeatureVector_0based[f] = piFeatureVector[f] - 1;

	ITL_random_varable_set_feature_vector
	(
		iFeatureLength,
		&piFeatureVector_0based[0],
		(*piIsUsingOrientation)?
				ITLRandomField::CRandomVariable::FEATURE_ORIENTATION:
				ITLRandomField::CRandomVariable::FEATURE_MAGNITUDE
	);
}

//! The Fortran API to set up a scalar as the feature vector of the current random variable
/*!
 * \param	piScalar	Pointer to the ID (1-based) of the data component
 * \sa 		ITL_random_varable_set_feature_vector
*/
extern "C"
void
itl_random_varable_as_scalar_
(
	int *piScalar,
	char* szFeatureMapping
)
{
	int piFeatureVector[1];
	piFeatureVector[0] = *piScalar - 1;		// from 1-based to 0-based
	ITL_random_varable_set_feature_vector
	(
		sizeof(piFeatureVector)/sizeof(piFeatureVector[0]),
		piFeatureVector,
		ITLRandomField::CRandomVariable::IConvertStringToFeatureMapping(szFeatureMapping)
	);
}

//! The Fortran API to set up a 3D vector as the feature vector of the current random variable
/*!
 * \param	piU		Pointer to the ID (1-based) of U vector component
 * \param	piV		Pointer to the ID (1-based) of V vector component
 * \param	piW		Pointer to the ID (1-based) of W vector component
 * \sa 		ITL_random_varable_set_feature_vector
*/
extern "C"
void
itl_random_varable_as_vector3_
(
	int *piU,
	int *piV,
	int *piW,
	char *szFeatureMapping
)
{
	int piFeatureVector[3];
	piFeatureVector[0] = *piU - 1;	// from 1-based to 0-based
	piFeatureVector[1] = *piV - 1;	// from 1-based to 0-based
	piFeatureVector[2] = *piW - 1;	// from 1-based to 0-based
	ITL_random_varable_set_feature_vector
	(
		sizeof(piFeatureVector)/sizeof(piFeatureVector[0]),
		piFeatureVector,
		ITLRandomField::CRandomVariable::IConvertStringToFeatureMapping(szFeatureMapping)
	);
}

// ADD-BY-LEETEN 07/22/2011-BEGIN
//! The Fortran API to set up a 2D vector as the feature vector of the current random variable
/*!
 * \param	piU		Pointer to the ID (1-based) of U vector component
 * \param	piV		Pointer to the ID (1-based) of V vector component
 * \sa 		ITL_random_varable_set_feature_vector
*/
extern "C"
void
itl_random_varable_as_vector2_
(
	int *piU,
	int *piV,
	char* szFeatureMapping
)
{
	int piFeatureVector[2];
	piFeatureVector[0] = *piU - 1;	// from 1-based to 0-based
	piFeatureVector[1] = *piV - 1;	// from 1-based to 0-based
	ITL_random_varable_set_feature_vector
	(
		sizeof(piFeatureVector)/sizeof(piFeatureVector[0]),
		piFeatureVector,
		ITLRandomField::CRandomVariable::IConvertStringToFeatureMapping(szFeatureMapping)
	);
}
// ADD-BY-LEETEN 07/22/2011-END

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to dump the geometry of the bound block to a file
/*!
\param	szGeomPathFilename path/filename of the file
*/
void
ITL_dump_bound_block_geom
(
	const char* szGeomPathFilename
)
{
	::pcBoundRandomField->CGetBoundBlock()._DumpGeometry(szGeomPathFilename);
}

//! The Fortran API to dump the geometry of the bound block to a default file
/*!
*/
extern "C"
void
itl_dump_bound_block_geom_2tmp_
(
)
{
	char szGeomPathFilename[1024];
	// MOD-BY-LEETEN 08/05/2011-FROM:
		// sprintf(szGeomPathFilename, "dump/geom.rank_%d.block_%d.txt", ::iRank, ::pcBoundRandomField->IGetBoundBlock());
	// TO:
	sprintf(szGeomPathFilename, "%s/geom.rank_%d.blk_%d.t_%d.txt", ::szDumpPath, ::iRank, ::pcBoundRandomField->IGetBoundBlock(), ::iTimeStamp);
	// MOD-BY-LEETEN 08/05/2011-END
	ITL_dump_bound_block_geom(szGeomPathFilename);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to dump the feature vector of a specified random variable to a file
/*!
\param	iRvId	ID (0-based) of the random variable
\param	szFeatureVectorPathFilename	Path/filename of the file
*/
void
ITL_dump_bound_block_feature_vector_
(
	const int iRvId,
	const char* szFeatureVectorPathFilename
)
{
	::pcBoundRandomField->_DumpBoundBlockFeatureVector
	 (
		 iRvId,
		 szFeatureVectorPathFilename
	 );
}

//! The Fortran API to dump the feature vector of a specified random variable to a default file
/*!
\param	iRvId	ID (1-based) of the random variable
\sa		ITL_dump_bound_block_feature_vector
*/
extern "C"
void
itl_dump_bound_block_feature_vector_2tmp_
(
	int *piRvId
)
{
	const int iRvId = *piRvId - 1;
	char szFeatureVectorPathFilename[1024];
	// MOD-BY-LEETEN 08/05/2011-FROM:
		// sprintf(szFeatureVectorPathFilename, "dump/feature_vector.rank_%d.block_%d.rv_%d", ::iRank, ::pcBoundRandomField->IGetBoundBlock(), iRvId);
	// TO:
	sprintf(szFeatureVectorPathFilename, "%s/feature_vector.rank_%d.blk_%d.t_%d.rv_%d", ::szDumpPath, ::iRank, ::pcBoundRandomField->IGetBoundBlock(), ::iTimeStamp, iRvId);
	// MOD-BY-LEETEN 08/05/2011-END
	ITL_dump_bound_block_feature_vector_
	(
		iRvId,
		szFeatureVectorPathFilename
	);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to compute and dump the global entropy of the bound block to a file
/*!
\param	iRvId							ID (0-based) of the random variable
\param	szGlobalEntropyLogPathFilename	Path/filename of the file
*/
void
ITL_dump_bound_block_global_entropy
(
	const int iRvId,
	const char* szGlobalEntropyLogPathFilename
)
{
	pcBoundRandomField->_ComputeEntorpyInBoundBlock
	(
		iRvId,
		 szGlobalEntropyLogPathFilename
	);
}

//! The Fortran API to dump the feature vector to a default path/filename
/*!
\param	iRvId	Pointer to the ID (1-based) of the random variable
\sa		ITL_dump_bound_block_global_entropy
*/
extern "C"
void
itl_dump_bound_block_global_entropy_2tmp_
(
	int *piRvId
)
{
	ITL_dump_bound_block_global_entropy
	(
		*piRvId - 1,
		szGlobalEntropyLogPathFilename
	);
}

/////////////////////////////////////////////////////////////////////
//! The C/C++ API to compute and dump the local entropy of the bound block to a file
/*!
\param	iRvId							ID (0-based) of the random variable
\param	szGlobalEntropyLogPathFilename	Path/filename of the file
*/
void
ITL_dump_bound_block_local_entropy
(
	const int iRvId,
	const int iDim,
	const double pdNeighborhood[],
	const char* szLocalEntropyLogPathFilename
)
{
	::pcBoundRandomField->_ComputeEntorpyFieldInBoundBlock
	(
		iRvId,
		iDim,
		pdNeighborhood,
		szLocalEntropyLogPathFilename
	);
}

//! The Fortran API to dump the feature vector to a default path/filename
/*!
\param	iRvId	Pointer to the ID (1-based) of the random variable
\sa		ITL_dump_bound_block_global_entropy
*/
extern "C"
void
itl_dump_bound_block_local_entropy3_2tmp_
(
	int *piRvId,
	double *pdXNeighborhood,
	double *pdYNeighborhood,
	double *pdZNeighborhood
)
{
	double pdNeighborhood[3];
	pdNeighborhood[0] = *pdXNeighborhood;
	pdNeighborhood[1] = *pdYNeighborhood;
	pdNeighborhood[2] = *pdZNeighborhood;
	char szLocalEntropyLogPathFilename[1024];
	// MOD-BY-LEETEN 08/05/2011-FROM:
		// sprintf(szLocalEntropyLogPathFilename, "dump/le.rank_%d.block_%d", ::iRank, ::pcBoundRandomField->IGetBoundBlock());
	// TO:
	sprintf(szLocalEntropyLogPathFilename, "%s/le.rank_%d.blk_%d.t_%d", ::szDumpPath, ::iRank, ::pcBoundRandomField->IGetBoundBlock(), ::iTimeStamp);
	// MOD-BY-LEETEN 08/05/2011-END

	ITL_dump_bound_block_local_entropy
	(
		*piRvId - 1,
		sizeof(pdNeighborhood)/sizeof(pdNeighborhood[0]),
		pdNeighborhood,
		szLocalEntropyLogPathFilename
	);
}

// ADD-BY-LEETEN 07/31/2011-BEGIN
/////////////////////////////////////////////////////////////////////
//! The C/C++ API to compute and dump the global joint entropy of the bound block to a file
/*!
\param	iRvId1	ID (0-based) of the 1st random variable
\param	iRvId2	ID (0-based) of the 2nd random variable
\param	szGlobalEntropyLogPathFilename	Path/filename of the file
*/
void
ITL_dump_bound_block_global_jentropy
(
	const int iRvId1,
	const int iRvId2,
	const char* szGlobalEntropyLogPathFilename
)
{
	pcBoundRandomField->_ComputeJointEntorpyInBoundBlock
	(
		iRvId1,
		iRvId2,
		 szGlobalEntropyLogPathFilename
	);
}

//! The Fortran API to dump the feature vector to a default path/filename
/*!
\param	iRvId	Pointer to the ID (1-based) of the random variable
\sa		ITL_dump_bound_block_global_joint_entropy
*/
extern "C"
void
itl_dump_bblk_jentropy_2tmp_
(
 int *piRvId1,
 int *piRvId2,
 int *piIsLocal
)
{
  if( 0 == *piIsLocal )
	ITL_dump_bound_block_global_jentropy
	(
		*piRvId1 - 1,
		*piRvId2 - 1,
		szGlobalEntropyLogPathFilename
	);
  else
    {
      ASSERT_OR_LOG(false, fprintf(stderr, "Not implemented yet."));
    }

}
// ADD-BY-LEETEN 07/31/2011-END

/*
 *
 * $Log$
 *
 */
