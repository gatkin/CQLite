#include <sqlite3.h>
#include <unistd.h>

#include "cqlite.h"
#include "test_database.h"
#include "unity.h"

#define TEST_DATABASE_FILE  ( "test.db" )

// Database handle shared by all tests. We assume that the
// tests are never run in parallel so it is safe for them to
// share a single database handle.
sqlite3 * g_db = NULL;


/*************************************
Test functions
*************************************/
static void test_insert_new
    (
    void
    );

/*************************************
Helper functions
*************************************/
static void after_all_tests
    (
    void
    );

static void assert_model_in_database
    (
    sqlite3 *               db,
    test_model_t const *    model
    );

static int before_all_tests
    (
    void
    );

static int before_each_test
    (
    void
    );


/**
* Tests inserting a new record into the database
*/
static void test_insert_new
    (
    void
    )
{
test_model_t new_model = 
    {/* id,                     real_field,     int_field,  dynamic_string, fixed_string    */
        CQLITE_INVALID_ROW_ID,  1.0,            1,          "Hello",        "ABC" 
    };

int success;

before_each_test();

// Attempt to insert a new model into the database.
success = test_model_insert_new( g_db, &new_model );

TEST_ASSERT_TRUE( success );
TEST_ASSERT( CQLITE_INVALID_ROW_ID != new_model.id );

// Ensure the model was actually saved to the database
assert_model_in_database( g_db, &new_model );
}


/**
* Executes clean up logic after all tests have finished.
*/
static void after_all_tests
    (
    void
    )
{
unlink( TEST_DATABASE_FILE );
}    


/**
* Assert model in database.
*
* Asserts that the given model is in the database.
*/
static void assert_model_in_database
    (
    sqlite3 *               db,
    test_model_t const *    model
    )
{
int             success;
int             model_found;
test_model_t    actual_model;

test_model_init( &actual_model );

success = test_model_find_by_id( db, model->id, &model_found, &actual_model );

TEST_ASSERT_TRUE( success );
TEST_ASSERT_TRUE( model_found );
TEST_ASSERT_TRUE( test_models_are_equal( model, &actual_model ) );

// Clean up
test_model_free( &actual_model );
}    


/**
* Run set up logic before all tests.
*/
static int before_all_tests
    (
    void
    )
{
int success;

success = ( SQLITE_OK == sqlite3_open( TEST_DATABASE_FILE, &g_db ) );

if( success )
    {
    success = test_database_init( g_db );
    }

return success;
}

/**
* Executes set-up logic to run before each test runs
*/
static int before_each_test
    (
    void
    )
{
// Start each test with a clean database.
return test_database_delete_all_data( g_db );
}    


/**
* Top-level entry-point into the test suite
*/
int main
    (
    void
    )
{
UNITY_BEGIN();

before_all_tests();

RUN_TEST(test_insert_new);

after_all_tests();

return UNITY_END();
}
