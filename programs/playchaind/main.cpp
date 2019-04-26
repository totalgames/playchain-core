/*
 * Copyright (c) 2018 Total Games LLC and contributors.
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

#include <graphene/app/application.hpp>

#include <graphene/witness/witness.hpp>
#include <graphene/debug_witness/debug_witness.hpp>
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/elasticsearch/elasticsearch_plugin.hpp>
#include <graphene/market_history/market_history_plugin.hpp>
#include <graphene/delayed_node/delayed_node_plugin.hpp>
#include <graphene/snapshot/snapshot.hpp>
#include <graphene/es_objects/es_objects.hpp>
#include <graphene/grouped_orders/grouped_orders_plugin.hpp>
#include <graphene/backup_snapshot/backup_snapshot.hpp>

#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>

#include <boost/filesystem.hpp>
#include <fc/asio.hpp>

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#ifdef WIN32
# include <signal.h>
#else
# include <csignal>
#include <sys/prctl.h>
#endif

#define PLAYCHAIN_SET_SIGNAL_HANDLER(SIG)                                                                              \
    fc::set_signal_handler(                                                                                            \
        [&handle_promise, &return_signal](int signal) {                                                                \
            return_signal = signal;                                                                                    \
            handle_promise->set_value(signal);                                                                         \
        },                                                                                                             \
        SIG);

using namespace graphene;

void wait_signals(app::application* /*node*/)
{
    for (;;)
    {
        fc::promise<int>::ptr handle_promise = new fc::promise<int>("UNIX Signal Handler");

        int return_signal = 0;
        PLAYCHAIN_SET_SIGNAL_HANDLER(SIGINT)
        PLAYCHAIN_SET_SIGNAL_HANDLER(SIGTERM)
        PLAYCHAIN_SET_SIGNAL_HANDLER(SIGHUP)
        PLAYCHAIN_SET_SIGNAL_HANDLER(SIGUSR1)
        PLAYCHAIN_SET_SIGNAL_HANDLER(SIGUSR2)

        std::cout << std::flush;
        std::cerr << std::flush;

        handle_promise->wait(); // wait signal

        bool exit = true;
        switch (return_signal)
        {
        case SIGINT:
            elog("Caught SIGINT attempting to exit cleanly");
            break;
        case SIGTERM:
            elog("Caught SIGTERM attempting to exit cleanly");
            break;
        case SIGHUP:
            elog("Caught SIGHUP attempting to exit cleanly");
            break;
        case SIGUSR1:
            elog("Caught SIGUSR1");
            //TODO: node->sigusr1();
            exit = false;
            break;
        case SIGUSR2:
            elog("Caught SIGUSR2");
            //TODO: node->sigusr2();
            exit = false;
            break;
        default:
            elog("Unexpected interruption. Signal #${s}", ("s", return_signal));
        }

        if (exit)
            break;
    }
}

namespace bpo = boost::program_options;

int main(int argc, char** argv) {

   //ignore SIGPIPE in daemon mode
   std::shared_ptr<boost::asio::signal_set> sig_set(new boost::asio::signal_set(fc::asio::default_io_service(), SIGPIPE));
   sig_set->async_wait([sig_set](const boost::system::error_code&, int) {});

   app::application* node = new app::application();
   fc::oexception unhandled_exception;
   try {
      static const char options_title[] = PLAYCHAIN " Node";
      bpo::options_description app_options(options_title);
      bpo::options_description cfg_options(options_title);
      app_options.add_options()
            ("help,h", "Print this help message and exit")
            ("sticked", "Make this process sticked to parent. Process receives SIGHUP from any parent process when parent die")
            ("config-file,x", bpo::value<boost::filesystem::path>(),
               (std::string("Path to config file. Defaults to data_dir/") + app::options_helper::get_config_file_name()).c_str());

      bpo::variables_map options;

      auto witness_plug = node->register_plugin<witness_plugin::witness_plugin>();
      auto debug_witness_plug = node->register_plugin<debug_witness_plugin::debug_witness_plugin>();
      auto history_plug = node->register_plugin<account_history::account_history_plugin>();
      auto elasticsearch_plug = node->register_plugin<elasticsearch::elasticsearch_plugin>();
      auto market_history_plug = node->register_plugin<market_history::market_history_plugin>();
      auto delayed_plug = node->register_plugin<delayed_node::delayed_node_plugin>();
      auto snapshot_plug = node->register_plugin<snapshot_plugin::snapshot_plugin>();
      auto es_objects_plug = node->register_plugin<es_objects::es_objects_plugin>();
      auto grouped_orders_plug = node->register_plugin<grouped_orders::grouped_orders_plugin>();
      auto backup_snapshot_plug = node->register_plugin<backup_snapshot_plugin::backup_snapshot_plugin>();

      try
      {
         bpo::options_description cli, cfg;
         node->set_program_options(cli, cfg);
         app_options.add(cli);
         cfg_options.add(cfg);
         bpo::store(bpo::parse_command_line(argc, argv, app_options), options);
      }
      catch (const boost::program_options::error& e)
      {
          std::cerr << "Error parsing command line: " << e.what() << "\n";
          return 1;
      }

      if( options.count("help") )
      {
          app_options.print(std::cout);
          return 0;
      }
      if( options.count("version") )
      {
         app::options_helper::print_application_version(std::cout);
         return 0;
      }

      if ( options.count("sticked") )
      {
#ifdef WIN32
          std::cerr << "Option 'sticked' is implemented for Linux only" << std::endl;
          return 1;
#else
          prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif
      }

      const fc::path config_file_path = app::options_helper::get_config_file_path(options);

      app::options_helper::create_config_file_if_not_exist(config_file_path, cfg_options);

      app::options_helper::load_config_file(config_file_path, cfg_options, options);

      const fc::path data_dir = app::options_helper::get_data_dir_path(options);

      std::cout << "Data directory is " << data_dir.preferred_string() << std::endl;

      bpo::notify(options);
      node->initialize(data_dir, options);
      node->initialize_plugins( options );

      node->startup();
      node->startup_plugins();

      std::cout << PLAYCHAIN << " network started." << std::endl;

      wait_signals(node);

      node->shutdown_plugins();
      node->shutdown();
      delete node;
      return 0;
   } catch( const fc::exception& e ) {
      // deleting the node can yield, so do this outside the exception handler
      unhandled_exception = e;
   }

   if (unhandled_exception)
   {
      elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
      node->shutdown();
      delete node;
      return 1;
   }
   ilog("done");
   return 0;
}
