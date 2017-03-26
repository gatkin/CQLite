/** @file */ 

#ifndef _CQLITE_H
#define _CQLITE_H

#include <sqlite3.h>

#define CQLITE_INVALID_ROW_ID ( -1 )

typedef enum
    {
    CQLITE_SUCCESS,
    CQLITE_ERROR,
    } cqlite_rcode_t;

/**
* Add model to result list function type.
*
* Prototype of functions to add a model to a list of models being
* read from the results of a single query. The provided model_list
* will already be allocated to hold all of the results of the query.
* The next_model_list_idx indicates the index into the model list into
* which the row result from the query parameter is to be read as a model.
* These functions should NEVER alter the provided query (e.g. by calling
* sqlite3_step()) and should only ever read column results from it.
*
* These types of function should return 1 on success, 0 on error.
*
* Typically, all these functions should do is cast the model_list
* parameter to the appropriate type and call the corresponding
* cqlite_model_from_row_result_func_t defined for a particular model
* type. For instance, if we have a model type named my_model_t, and
* a cqlite_model_from_row_result_func_t function named my_model_from_query,
* then we would implement the my_model_t's cqlite_model_add_to_list_func_t
* function as
*
*       int my_model_add_to_list( sqlite3_stmt * query, void * model_list, int next_model_list_idx )
*       {
*       my_model_t * my_models;
*
*       my_models = (my_model_t*)model_list;
*
*       return my_model_from_query( query, &my_models[next_model_list_idx] );
*       }
*/
typedef int (*cqlite_model_add_to_list_func_t)
    (
    sqlite3_stmt *  query,              //!< Query pointing at row result                                            
    void *          model_list,         //!< List of all models returned so far                                      
    int             next_model_list_idx //!< Index into the model list into which the query row result should be read
    );

/**
* Model from row result function type.
*
* Prototype of functions to read the provided query parameter
* pointing at a retrived row result as a model of a particular
* type into the model_out pointer, which should be large enough
* to hold all model data. These functions should NEVER alter
* the provided query (e.g. by calling sqlite3_step()) and should
* only read column results from it.
*
* These types of function should return 1 on success, 0 on error.
*/
typedef int (*cqlite_model_from_row_result_func_t)
    (
    sqlite3_stmt *  query,      //!< Query pointing at row result         
    void *          model_out   //!< (out) Model populated from row result
    );

/**
* Execute count query.
* 
* Executes the provided COUNT query string that takes no parameters
* on the provided database.
*/
cqlite_rcode_t cqlite_count_query_execute
    (
    sqlite3 *           db,                 //!< Database on which to execute the query
    char const * const  count_query_str,    //!< Parameter-less COUNT query string     
    int *               count_out           //!< (out) Returned count                  
    );

/**
* Execute prepared count query.
* 
* Executes the provided prepared COUNT query.
*/
cqlite_rcode_t cqlite_count_query_execute_prepared
    (
    sqlite3_stmt *  count_query,    //!< Prepared COUNT query
    int *           count_out       //!< (out) Returned count
    );

/**
* Read dynamically-allocated string from query.
*
* Reads the specified column from the provided query row result as
* a dynamically allocated string into string_out. The caller must
* call free() on string_out. If the column result is NULL or not
* a string, string_out will be set to NULL.
*/
cqlite_rcode_t cqlite_dynamic_string_read
    (
    sqlite3_stmt *  query,      //!< Query result
    int             column,     //!< Column of string to read from query result
    char **         string_out  //!< (out) Dynamically-allocated string read from query, caller must free
    );

/**
* Read fixed-length string from query.
*
* Reads the specified column from the provided query row result as
* a fixed length string into the provided string buffer. Returns an
* error if the column result either is not a string or if its length
* exceeds the size of the string buffer. If the column result is NULL,
* then the string buffer will be set to an empty string.
*/
cqlite_rcode_t cqlite_fixed_length_string_read
    (
    sqlite3_stmt *  query,          //!< Query result
    int             column,         //!< Column of string to read from query result
    char *          string,         //!< String buffer, must be allocated by caller to be of size string_size
    size_t          string_size     //!< Size of the string buffer
    );

/**
* Execute insert query.
* 
* Executes the provided prepared insert query. If new_row_id_out
* is not NULL, then this will also output the newly saved record's
* row id.
*/
cqlite_rcode_t cqlite_insert_query_execute
    (
    sqlite3 *       db,             //!< Database on which to execute the query
    sqlite3_stmt *  insert_query,   //!< Prepared insert query                 
    sqlite_int64 *  new_row_id_out  //!< (out) Generated row id of new record  
    );

/**
* Execute SELECT query.
* 
* Executes the SELECT query on the given database handle and reads the
* results into model_list_out. The model_list_out output parameter is
* allocated using calloc() so the caller is responsible for calling
* free() on model_list_out as well as cleaning up any memory owned by
* elements of the model list. Since model_list_out is allocated and
* initialized to all zeros, it should be safe to free all data in
* the model list even if an error occurs part way through reading
* the query results. The caller is always responsible for freeing
* the memory in model_list_out regardless of whether the function
* executes successfully.
* 
* The provided COUNT query string should retrieve the number of results
* that will be retrieved by the SELECT query string. For instance, if
* the select query string is:
* 
*         "SELECT * FROM my_table WHERE my_column = 7;"
* 
* Then the COUNT query string should be:
* 
*         "SELECT COUNT(*) FROM my_table WHERE my_column = 7;"
*/
cqlite_rcode_t cqlite_select_query_execute
    (
    sqlite3 *                       db,                 //!< Database on which to execute the query                
    char const * const              select_query_str,   //!< Parameter-less SELECT query string                    
    char const * const              count_query_str,    //!< Parameter-less COUNT query string                     
    cqlite_model_add_to_list_func_t add_to_list_func,   //!< Add model to list function pointer                    
    size_t                          model_size,         //!< Size of the model type                                
    void **                         model_list_out,     //!< (out) List of models read from query, caller must free
    int *                           model_list_cnt_out  //!< (out) Number of models read from query                
    );

/**
* Execute prepared SELECT query.

* @see cqlite_select_query_execute()
*/
cqlite_rcode_t cqlite_select_query_execute_prepared
    (
    sqlite3_stmt *                  select_query,       //!< Prepared SELECT query                                 
    sqlite3_stmt *                  count_query,        //!< Prepared COUNT query                                  
    cqlite_model_add_to_list_func_t add_to_list_func,   //!< Add model to list function pointer                    
    size_t                          model_size,         //!< Size of the model type                                
    void **                         model_list_out,     //!< (out) List of models read from query, caller must free
    int *                           model_list_cnt_out  //!< (out) Number of models read from query                
    );

#endif