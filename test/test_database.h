#ifndef _TEST_DATABASE_H
#define _TEST_DATABASE_H

#include <sqlite3.h>

typedef struct
    {
    sqlite3_int64   id;
    double          real_field;
    int             int_field;
    char *          dynamic_string_field;
    char            fixed_string_field[4];
    } test_model_t;

typedef struct
    {
    test_model_t *  list;
    int             cnt;
    } test_model_list_t;


/**********************************************
Data type functions
**********************************************/
void test_model_free
    (
    test_model_t * model
    );

void test_model_init
    (
    test_model_t * model
    );

int test_models_are_equal
    (
    test_model_t const * expected,
    test_model_t const * actual
    );

void test_model_list_free
    (
    test_model_list_t * model
    );

void test_model_list_init
    (
    test_model_list_t * model
    );

int test_model_lists_are_equal
    (
    test_model_list_t const * expected,
    test_model_list_t const * actual
    );

/**********************************************
Database functions
**********************************************/
int test_database_delete_all_data
    (
    sqlite3 * db
    );

int test_database_init
    (
    sqlite3 * db
    );

int test_model_find_by_id
    (
    sqlite3 *       db,
    sqlite3_int64   id,
    int *           found_out,
    test_model_t *  model_out
    );

int test_model_insert_new
    (
    sqlite3 *       db,
    test_model_t *  model
    );

#endif