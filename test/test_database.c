#include <stdlib.h>
#include <string.h>

#include "cqlite.h"
#include "test_database.h"

#define NO_CALLBACK         ( NULL )
#define NO_CALLBACK_PARAM   ( NULL )
#define NO_ERROR_MESSAGE    ( NULL )
#define READ_TO_END         ( -1 )
#define NO_TAIL             ( NULL )


/**********************************************
Database definitions
**********************************************/
typedef enum
    {
    INSERT_MODE_NEW_RECORD,
    INSERT_MODE_EXISTING_RECORD,
    } insert_mode_t;


static char const * const TEST_TABLE_CREATE = 
    "CREATE TABLE IF NOT EXISTS test"
    "("
    "id INTEGER PRIMARY KEY"
    ", real_field REAL"
    ", int_field INTEGER"
    ", dynamic_string_field TEXT"
    ", fixed_string_field TEXT"
    ");";

typedef enum
    {
    TEST_TABLE_ID_COL = 0,
    TEST_TABLE_REAL_FIELD_COL = 1,
    TEST_TABLE_INT_FIELD_COL = 2,
    TEST_TABLE_DYNAMIC_STRING_FIELD_COL = 3,
    TEST_TABLE_FIXED_STRING_FIELD_COL = 4,
    } test_table_columns_t;


static char const * const TEST_TABLE_DELETE_ALL     = "DELETE FROM test;";
static char const * const TEST_TABLE_INSERT         = "INSERT OR REPLACE INTO test VALUES (?, ?, ?, ?, ?);";
static char const * const TEST_TABLE_SELECT_BY_ID   = "SELECT * FROM test WHERE id = ?;";


/**********************************************
Functions
**********************************************/
static int test_model_add_to_list
    (
    sqlite3_stmt *  query,             
    void *          model_list,        
    int             next_model_list_idx
    );

static int test_model_from_row_result
    (
    sqlite3_stmt *  query,   
    void *          model_out
    );

static int test_model_insert_query_prepare
    (
    sqlite3 *               db,
    test_model_t const *    model,
    insert_mode_t           insert_mode,
    sqlite3_stmt **         query_out
    );



/**
* Free test model.
*/
void test_model_free
    (
    test_model_t * model
    )
{
free( model->dynamic_string_field );

test_model_init( model );
}    

/**
* Initialize test model.
*/
void test_model_init
    (
    test_model_t * model
    )
{
memset( model, 0, sizeof( *model ) );
}    


/**
* Are models equal?
*
* Returns 1 if the two models are equal, 0 otherwise
*/
int test_models_are_equal
    (
    test_model_t const * expected,
    test_model_t const * actual
    )
{
int are_equal;
int are_strings_equal;

if( ( NULL == expected->dynamic_string_field ) || ( NULL == actual->dynamic_string_field ) )
    {
    are_strings_equal = ( expected->dynamic_string_field == actual->dynamic_string_field );
    }
else
    {
    are_strings_equal = ( 0 == strcmp( expected->dynamic_string_field, actual->dynamic_string_field ) );
    }

are_equal = ( are_strings_equal ) &&
            ( expected->id == actual->id ) &&
            ( expected->real_field == actual->real_field ) &&
            ( expected->int_field == actual->int_field ) &&
            ( 0 == strcmp( expected->fixed_string_field, actual->fixed_string_field ) );

return are_equal;
}    


/**
* Free test model list.
*/
void test_model_list_free
    (
    test_model_list_t * models
    )
{
int i;

for( i = 0; i < models->cnt; i++ )
    {
    test_model_free( &models->list[i] );
    }

free( models->list );

test_model_list_init( models );
}    

/**
* Initialize test model list.
*/
void test_model_list_init
    (
    test_model_list_t * models
    )
{
memset( models, 0, sizeof( *models ) );
}


/**
* Are model lists equal?
*
* Returns 1 if the two test model lists are equal, 0 otherwise.
*/
int test_model_lists_are_equal
    (
    test_model_list_t const * expected,
    test_model_list_t const * actual
    )
{
int are_equal;
int i;

are_equal = ( expected->cnt == actual->cnt );

for( i = 0; ( are_equal ) && ( i < expected->cnt ); i++ )
    {
    are_equal = test_models_are_equal( &expected->list[i], &actual->list[i] );
    }

return are_equal;
}    


/**
* Delete all data from test database.
*/
int test_database_delete_all_data
    (
    sqlite3 * db
    )
{
return ( SQLITE_OK == sqlite3_exec( db, TEST_TABLE_DELETE_ALL, NO_CALLBACK, NO_CALLBACK_PARAM, NO_ERROR_MESSAGE ) );
}    


/**
* Initialize test database.
*
* Initializes the test database by creating the schema for the
* test table if it does not already exist.
*/
int test_database_init
    (
    sqlite3 * db
    )
{
return ( SQLITE_OK == sqlite3_exec( db, TEST_TABLE_CREATE, NO_CALLBACK, NO_CALLBACK_PARAM, NO_ERROR_MESSAGE ) );
}


/**
* Find model by id.
*
* Caller must call test_model_free() on model_out.
*/
int test_model_find_by_id
    (
    sqlite3 *       db,
    sqlite3_int64   id,
    int *           found_out,
    test_model_t *  model_out
    )
{
cqlite_rcode_t rcode;

*found_out = 0;
test_model_init( model_out );

rcode = cqlite_find_by_id( db, TEST_TABLE_SELECT_BY_ID, id, test_model_from_row_result, found_out, model_out );

return ( CQLITE_SUCCESS == rcode );
}    


/**
* Insert new model.
*
* Inserts the provided model as a new record into the database.
* This will modify the provided model with the generated insert
* id.
*/
int test_model_insert_new
    (
    sqlite3 *       db,
    test_model_t *  model
    )
{
int             success;
sqlite3_stmt *  insert_query = NULL;

success = test_model_insert_query_prepare( db, model, INSERT_MODE_NEW_RECORD, &insert_query );

if( success )
    {
    success = ( CQLITE_SUCCESS == cqlite_insert_query_execute( db, insert_query, &model->id ) );
    }

// Clean up.
sqlite3_finalize( insert_query );

return success;
}    


/**
* Add test model to result list.
*/
static int test_model_add_to_list
    (
    sqlite3_stmt *  query,             
    void *          model_list,        
    int             next_model_list_idx
    )
{
test_model_t * test_models;

test_models = (test_model_t*)model_list;

return test_model_from_row_result( query, &test_models[next_model_list_idx] );
}    


/**
* Read test model from query row result.
*/
static int test_model_from_row_result
    (
    sqlite3_stmt *  query,   
    void *          model_out
    )
{
int             success;
test_model_t *  test_model;

test_model = (test_model_t*)model_out;

test_model_init( test_model );

test_model->id = sqlite3_column_int64( query, TEST_TABLE_ID_COL );
test_model->real_field = sqlite3_column_double( query, TEST_TABLE_REAL_FIELD_COL );
test_model->int_field = sqlite3_column_int( query, TEST_TABLE_INT_FIELD_COL );

success = ( CQLITE_SUCCESS == cqlite_dynamic_string_read( query, TEST_TABLE_DYNAMIC_STRING_FIELD_COL, &test_model->dynamic_string_field ) );

if( success )
    {
    success = ( CQLITE_SUCCESS == cqlite_fixed_length_string_read( query, TEST_TABLE_FIXED_STRING_FIELD_COL, test_model->fixed_string_field, sizeof( test_model->fixed_string_field ) ) );
    }

return success;
}


/**
* Prepare insert query for test model.
*
* The caller must call sqlite3_finalize on query_out.
*/
static int test_model_insert_query_prepare
    (
    sqlite3 *               db,
    test_model_t const *    model,
    insert_mode_t           insert_mode,
    sqlite3_stmt **         query_out
    )
{
int             success;
sqlite3_stmt *  insert_query = NULL;

*query_out = NULL;

success = ( SQLITE_OK == sqlite3_prepare_v2( db, TEST_TABLE_INSERT, READ_TO_END, &insert_query, NO_TAIL ) );

if( success )
    {
    // Why are query parameters 1-indexed and query results 0-indexed?
    success = ( SQLITE_OK == sqlite3_bind_double( insert_query, (TEST_TABLE_REAL_FIELD_COL + 1), model->real_field ) ) &&
              ( SQLITE_OK == sqlite3_bind_int( insert_query, (TEST_TABLE_INT_FIELD_COL + 1), model->int_field ) ) &&
              ( SQLITE_OK == sqlite3_bind_text( insert_query, (TEST_TABLE_DYNAMIC_STRING_FIELD_COL + 1), model->dynamic_string_field, READ_TO_END, SQLITE_TRANSIENT ) ) &&
              ( SQLITE_OK == sqlite3_bind_text( insert_query, (TEST_TABLE_FIXED_STRING_FIELD_COL + 1), model->fixed_string_field, READ_TO_END, SQLITE_TRANSIENT ) );
    }

if( success && ( INSERT_MODE_EXISTING_RECORD == insert_mode ) )
    {
    success = ( SQLITE_OK == sqlite3_bind_int64( insert_query, (TEST_TABLE_ID_COL + 1), model->id ) );
    }

// Set the output or clean up on error
if( success )
    {
    *query_out = insert_query;
    }
else
    {
    sqlite3_finalize( insert_query );
    }

return success;
}    