 /* Copyright (c) 2018 Valik Grinko, and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/backup_snapshot/backup_snapshot.hpp>

#include <graphene/chain/database.hpp>

#include <fc/io/fstream.hpp>

using namespace graphene::chain;
using namespace graphene::backup_snapshot_plugin;
using std::string;
using std::vector;

namespace bpo = boost::program_options;

static const char* OPT_BLOCK_PERIOD  = "backup-snapshot-block-period";
static const char* OPT_DEST          = "backup-snapshot-dest";

void backup_snapshot_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   command_line_options.add_options()
         (OPT_BLOCK_PERIOD, bpo::value<uint32_t>(), "Block count perion, after which to do a snapshot")
         (OPT_DEST, bpo::value<string>(), "Pathname of JSON file where to store the snapshot")
         ;
   config_file_options.add(command_line_options);
}

std::string backup_snapshot_plugin::plugin_name()const
{
   return "backup_snapshot";
}

std::string backup_snapshot_plugin::plugin_description()const
{
   return "Create snapshots at a every choosen block number.";
}

void backup_snapshot_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{try {
    ilog("backup snapshot plugin: plugin_initialize() begin");

    if( options.count(OPT_BLOCK_PERIOD))
    {
        block_period = options[OPT_BLOCK_PERIOD].as<uint32_t>();
        if (block_period <= 0)
            block_period = 100;

        FC_ASSERT( options.count(OPT_DEST), "Must specify ${od}", ("od", OPT_DEST) );
        dest = options[OPT_DEST].as<std::string>();
        FC_ASSERT( exists(dest), "Destination file not exists ${path}", ("path", OPT_DEST) );
   }
   else
      FC_ASSERT( !options.count(OPT_DEST), "Must specify ${obp}", ("obp", OPT_BLOCK_PERIOD));

   ilog("backup snapshot plugin: plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void backup_snapshot_plugin::plugin_startup()
{
    ilog("backup snapshot plugin: startup");
    if (block_period) {
        database().applied_block.connect( [&]( const graphene::chain::signed_block& b ) {
           check_snapshot( b );
        });
    }
}

void backup_snapshot_plugin::plugin_shutdown() {}

static void create_snapshot( const graphene::chain::database& db, const fc::path& dest )
{
    ilog("backup snapshot plugin: start snapshot creating");

    std::string tmp_path = dest.string();
    tmp_path += ".tmp";
    const fc::path tmp_dest(tmp_path);

    fc::ofstream out;
    try
    {
       out.open( tmp_dest );
    }
    catch ( fc::exception e )
    {
       wlog( "Failed to open snapshot destination: ${ex}", ("ex",e) );
       return;
    }

   const std::vector< std::pair<uint8_t, uint8_t> > need_types = {{protocol_ids,account_object_type},
                                                             {implementation_ids,impl_account_balance_object_type}};

   for (const auto& type : need_types)
   {
       try
       {
           auto& index = db.get_index( type.first, type.second );
           index.inspect_all_objects( [&out]( const graphene::db::object& o ) {
              out << fc::json::to_string( o.to_variant() ) << '\n';
           });
       }
       catch (fc::assert_exception e)
       {
          continue;
       }
   }

   out.close();

   fc::rename( tmp_dest, dest );

   ilog("snapshot plugin: created snapshot");
}

void backup_snapshot_plugin::check_snapshot( const graphene::chain::signed_block& b )
{ try {

    uint32_t current_block = b.block_num();

    if(!(current_block%block_period))
    {
        create_snapshot( database(), dest );
    }
} FC_LOG_AND_RETHROW() }
