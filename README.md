# CQLite
The goal of CQLite is to provide a slightly simplified C interface for processing with SQLite query results in order to reduce the amount of boiler-plate code developers need to write to interact with a database. CQLite identifies a few common operations performed on SQLite databases and provides a way to abstract away the common bits of code meeded to perform these actions which include:
      
- Reading through all results of a SELECT query
- Reading the result of a COUNT query
- Inserting a new record into the database and retrieving its generated row id

Application developers define their queries and their models as well as how to convert a query result into an appropriate model and then call into CQLite to perform all the work to actually execute the query.

Suppose we wanted to track running race results in a SQLite database. We might define our races table to look something like:
```
CREATE TABLE IF NOT EXISTS races
    (
    id INTEGER PRIMARY KEY,
    distance REAL,
    name TEXT,
    city TEXT,
    state TEXT
    );
````
We might also have a model type defined to hold data for races
```C
typedef struct
    {
    sqlite_int64    id;
    double          distance;
    char *          name;
    char *          city;
    char            state[3];
    } race_t;
```
Suppose we wanted to retrieve a list of all marathons from the database. Rather than manully writing the code to loop through and read all results, we could use CQLite and only need to define a function to read a query row result into a race model as well as a function to add a race model to a list of models.
```C
static int race_add_to_list
    (
    sqlite3_stmt *  query,              //!< Query pointing at row result                                            
    void *          model_list,         //!< List of all models returned so far                                      
    int             next_model_list_idx //!< Index into the model list into which the query row result should be read
    )
{
race_t * race_list;

race_list = (race_t*)model_list;

return race_from_row_result( query, &race_list[next_model_list_idx] );
}    

static int race_from_row_result
    (
    sqlite3_stmt *  query,      //!< Query pointing at row result         
    void *          model_out   //!< (out) Model populated from row result
    )
{
int         success;
race_t *    race;

race = (race_t*)model_out;
race_init( race );

race->id = sqlite3_column_int64( query, 0 );
race->distance = sqlite3_column_double( query, 1 );

success = ( CQLITE_SUCCESS == cqlite_dynamic_string_read( query, 2, &race->name ) );

if( success )
    {
    success = ( CQLITE_SUCCESS == cqlite_dynamic_string_read( query, 3, &race->city ) );
    }

if( success )
    {
    success = ( CQLITE_SUCCESS == cqlite_fixed_string_read( query, 4, race->state, sizeof( race->state ) ) );
    }

return success;
}
```
Finally, our function to execute the actual query to get the list of marathons becomes simple and declarative.
```C
int get_marathons
    (
    sqlite3 *   db,
    race_t **   races_out,
    int *       races_cnt_out
    )
{
cqlite_rcode_t rcode;

rcode = cqlite_select_query_execute
    (
    db,
    "SELECT * FROM races WHERE distance = 26.2;",
    "SELECT COUNT(*) FROM races WHERE distance = 26.2;",
    race_add_to_list,
    sizeof( race_t ),
    races_out,
    races_cnt_out
    );

return ( CQLITE_SUCCESS == rcode );
}  
```
The benefit is that the same `race_add_to_list()` and `race_from_query()` functions can be re-used to perform other queries--such as selecting only races that occur in Boston or selecting only 10Ks that occur in Boston--with a minimal amount of additional code. Adding additional fields to the race model and table then only requires updating the single function where a race model is read from a query row result.