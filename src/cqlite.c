#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cqlite.h"

#define READ_TO_END ( -1 )
#define NO_TAIL     ( NULL )


// Execute count query.
cqlite_rcode_t cqlite_count_query_execute
    (
    sqlite3 *           db,                 //!< Database on which to execute the query
    char const * const  count_query_str,    //!< Parameter-less COUNT query string     
    int *               count_out           //!< (out) Returned count                  
    )
{
cqlite_rcode_t  rcode = CQLITE_ERROR;
int             success;
sqlite3_stmt *  count_query = NULL;

*count_out = 0;

success = ( SQLITE_OK == sqlite3_prepare_v2( db, count_query_str, READ_TO_END, &count_query, NO_TAIL ) );

if( success )
    {
    rcode = cqlite_count_query_execute_prepared( count_query, count_out );
    }

// Clean up
sqlite3_finalize( count_query );

return rcode;
}    


// Execute prepared count query.
cqlite_rcode_t cqlite_count_query_execute_prepared
    (
    sqlite3_stmt *  count_query,    //!< Prepared COUNT query
    int *           count_out       //!< (out) Returned count
    )
{
cqlite_rcode_t rcode = CQLITE_ERROR;

*count_out = 0;

if( SQLITE_ROW == sqlite3_step( count_query ) )
    {
    rcode = CQLITE_SUCCESS;
    *count_out = sqlite3_column_int( count_query, 0 );
    }

return rcode;
}    


// Read dynamically-allocated string from query.
cqlite_rcode_t cqlite_dynamic_string_read
    (
    sqlite3_stmt *  query,      //!< Query result
    int             column,     //!< Column of string to read from query result
    char **         string_out  //!< (out) Dynamically-allocated string read from query, caller must free
    )
{
cqlite_rcode_t  rcode = CQLITE_ERROR;
int             success;
int             column_type;

*string_out = NULL;

column_type = sqlite3_column_type( query, column );
success = ( SQLITE_TEXT == column_type ) || ( SQLITE_NULL == column_type );

if( SQLITE_TEXT == column_type )
    {
    *string_out = strdup( sqlite3_column_text( query, column ) );
    success = ( NULL != *string_out );
    }

if( success )
    {
    rcode = CQLITE_SUCCESS;
    }

return rcode;
}  


// Find model.
cqlite_rcode_t cqlite_find
    (
    sqlite3_stmt *                      query,                  //!< Prepared SELECT query to return one result
    cqlite_model_from_row_result_func_t model_from_result_func, //!< Function to read the result into the model
    int *                               found_out,              //!< (out) Was a record found?
    void *                              model_out               //!< (out) Found model
    )
{
cqlite_rcode_t  rcode = CQLITE_ERROR;
int             success;
int             sqlite_rcode;

*found_out = 0;

sqlite_rcode = sqlite3_step( query );
success = ( SQLITE_ROW == sqlite_rcode ) || ( SQLITE_DONE == sqlite_rcode );

if( SQLITE_ROW == sqlite_rcode )
    {
    *found_out = 1;
    success = model_from_result_func( query, model_out );
    }

if( success )
    {
    rcode = CQLITE_SUCCESS;
    }

return rcode;
}    


// Find model by id.
cqlite_rcode_t cqlite_find_by_id
    (
    sqlite3 *                           db,                     //!< Database on which to execute the query
    char const * const                  find_by_id_query,       //!< SELECT query string taking a single id parameter
    sqlite_int64                        id,                     //!< Id to search for
    cqlite_model_from_row_result_func_t model_from_result_func, //!< Function to read the result into the model
    int *                               found_out,              //!< (out) Was a record found?
    void *                              model_out               //!< (out) Found model
    )
{
cqlite_rcode_t  rcode = CQLITE_ERROR;
int             success;
sqlite3_stmt *  select_query = NULL;

*found_out = 0;

success = ( SQLITE_OK == sqlite3_prepare_v2( db, find_by_id_query, READ_TO_END, &select_query, NO_TAIL ) ) &&
          ( SQLITE_OK == sqlite3_bind_int64( select_query, 1, id ) );

if( success )
    {
    rcode = cqlite_find( select_query, model_from_result_func, found_out, model_out );
    }

// Clean up
sqlite3_finalize( select_query );

return rcode;
}    


// Read fixed-length string from query.
cqlite_rcode_t cqlite_fixed_length_string_read
    (
    sqlite3_stmt *  query,          //!< Query result
    int             column,         //!< Column of string to read from query result
    char *          string,         //!< String buffer, must be allocated by caller to be of size string_size
    size_t          string_size     //!< Size of the string buffer
    )
{
cqlite_rcode_t  rcode = CQLITE_ERROR;
int             success;
int             column_type;
int             bytes_printed;

column_type = sqlite3_column_type( query, column );
success = ( SQLITE_TEXT == column_type ) || ( SQLITE_NULL == column_type );

if( SQLITE_TEXT == column_type )
    {
    bytes_printed = snprintf( string, string_size, "%s", sqlite3_column_text( query, column ) );
    success = ( 0 <= bytes_printed ) && ( bytes_printed < string_size );
    }
else if( SQLITE_NULL == column_type )
    {
    memset( string, 0, string_size );
    }

if( success )
    {
    rcode = CQLITE_SUCCESS;
    }

return rcode;
}    


// Execute insert query.
cqlite_rcode_t cqlite_insert_query_execute
    (
    sqlite3 *       db,             //!< Database on which to execute the query
    sqlite3_stmt *  insert_query,   //!< Prepared insert query                 
    sqlite_int64 *  new_row_id_out  //!< (out) Generated row id of new record  
    )
{
cqlite_rcode_t  rcode = CQLITE_ERROR;
int             success;

success = ( SQLITE_DONE == sqlite3_step( insert_query ) );

if( success )
    {
    rcode = CQLITE_SUCCESS;

    if( NULL != new_row_id_out )
        {
        *new_row_id_out = sqlite3_last_insert_rowid( db );
        }
    }

return rcode;
}


// Execute select query.
cqlite_rcode_t cqlite_select_query_execute
    (
    sqlite3 *                       db,                 //!< Database on which to execute the query                
    char const * const              select_query_str,   //!< Parameter-less SELECT query string                    
    char const * const              count_query_str,    //!< Parameter-less COUNT query string                     
    cqlite_model_add_to_list_func_t add_to_list_func,   //!< Add model to list function pointer                    
    size_t                          model_size,         //!< Size of the model type                                
    void **                         model_list_out,     //!< (out) List of models read from query, caller must free
    int *                           model_list_cnt_out  //!< (out) Number of models read from query                
    )
{
cqlite_rcode_t  rcode = CQLITE_ERROR;
int             success;
sqlite3_stmt *  select_query = NULL;
sqlite3_stmt *  count_query = NULL;

*model_list_out = NULL;
*model_list_cnt_out = 0;

success = ( SQLITE_OK == sqlite3_prepare_v2( db, select_query_str, READ_TO_END, &select_query, NO_TAIL ) ) &&
          ( SQLITE_OK == sqlite3_prepare_v2( db, count_query_str, READ_TO_END, &count_query, NO_TAIL ) );

if( success )
    {
    rcode = cqlite_select_query_execute_prepared( select_query, count_query, add_to_list_func, model_size, model_list_out, model_list_cnt_out );
    }

// Clean up
sqlite3_finalize( select_query );
sqlite3_finalize( count_query );

return rcode;
}    


// Execute prepared select query.
cqlite_rcode_t cqlite_select_query_execute_prepared
    (
    sqlite3_stmt *                  select_query,       //!< Prepared SELECT query                                 
    sqlite3_stmt *                  count_query,        //!< Prepared COUNT query                                  
    cqlite_model_add_to_list_func_t add_to_list_func,   //!< Add model to list function pointer                    
    size_t                          model_size,         //!< Size of the model type                                
    void **                         model_list_out,     //!< (out) List of models read from query, caller must free
    int *                           model_list_cnt_out  //!< (out) Number of models read from query                
    )
{
cqlite_rcode_t  rcode = CQLITE_ERROR;
int             success;
int             model_list_cnt = 0;
void *          model_list = NULL;
int             sqlite_rcode = SQLITE_ERROR;
int             model_idx = 0;

// Get the number of expected results
success = ( CQLITE_SUCCESS == cqlite_count_query_execute_prepared( count_query, &model_list_cnt ) );

// Allocate the output list to hold all expected results.
if( success && ( model_list_cnt > 0 ) )
    {
    model_list = calloc( model_list_cnt, model_size );
    success = ( NULL != model_list );
    }

if( success )
    {
    sqlite_rcode = sqlite3_step( select_query );
    }

// Read each result into the output list
while( success && ( SQLITE_ROW == sqlite_rcode ) )
    {
    if( model_idx >= model_list_cnt )
        {
        // More results returned than expected.
        success = 0;
        }
    else
        {
        success = add_to_list_func( select_query, model_list, model_idx );

        // Move to the next result
        sqlite_rcode = sqlite3_step( select_query );
        model_idx++;
        }
    }

// Ensure all results were successfully read
if( success )
    {
    success = ( SQLITE_DONE == sqlite_rcode );
    }

// Set the output
*model_list_out     = model_list;
*model_list_cnt_out = model_list_cnt;

if( success )
    {
    rcode = CQLITE_SUCCESS;
    }

return rcode;
}    