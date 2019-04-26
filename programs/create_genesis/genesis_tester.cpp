#include "genesis_tester.hpp"

#include <fc/log/logger.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/genesis_state.hpp>
#include <graphene/utilities/tempdir.hpp>

namespace playchain {

namespace util {

using namespace graphene::chain;

void test_database(const genesis_state_type& genesis)
{
    chain::database db;
    try
    {
        fc::temp_directory data_dir = fc::temp_directory(graphene::utilities::temp_directory_path());
        db.open(data_dir.path(), [&genesis]{return genesis;}, "TEST");

        ilog("Test completed. Database is opened successfully.");
    }
    catch (const fc::exception& e)
    {
        elog("Test completed.");
        edump((e.to_detail_string()));
        throw;
    }
}
}
}
