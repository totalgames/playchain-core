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
#include <boost/test/unit_test.hpp>

#include "test_applications.hpp"

#include <graphene/app/plugin.hpp>
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/witness/witness.hpp>
#include <graphene/market_history/market_history_plugin.hpp>
#include <graphene/egenesis/egenesis.hpp>
#include <graphene/wallet/wallet.hpp>

#include <boost/filesystem/path.hpp>

namespace cli
{
BOOST_AUTO_TEST_SUITE( base_tests )

////////////////
// Start a server and connect using the same calls as the CLI
////////////////
BOOST_AUTO_TEST_CASE( cli_connect )
{
   using namespace graphene::chain;
   using namespace graphene::app;
   std::shared_ptr<graphene::app::application> app1;
   try {
      fc::temp_directory app_dir ( graphene::utilities::temp_directory_path() );

       int server_port_number = 0;
       app1 = start_application(app_dir, server_port_number);

      // connect to the server
       client_connection con(app1, app_dir, server_port_number);

   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
   app1->shutdown();
}

///////////////////////
// Start a server and connect using the same calls as the CLI
// Vote for two witnesses, and make sure they both stay there
// after a maintenance block
///////////////////////
BOOST_AUTO_TEST_CASE( cli_vote_for_2_witnesses )
{
   using namespace graphene::chain;
   using namespace graphene::app;
   std::shared_ptr<graphene::app::application> app1;
   try {
      fc::temp_directory app_dir( graphene::utilities::temp_directory_path() );

      int server_port_number = 0;
      app1 = start_application(app_dir, server_port_number);

      // connect to the server
      client_connection con(app1, app_dir, server_port_number);

      BOOST_TEST_MESSAGE("Setting wallet password");
      con.api()->set_password("supersecret");
      con.api()->unlock("supersecret");

      // import Nathan and InitX accounts
      BOOST_TEST_MESSAGE("Importing keys");

      auto nathan_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("nathan")));
      std::vector<std::string> nathan_keys{key_to_wif(nathan_key)};
      BOOST_CHECK(con.api()->import_key("nathan", nathan_keys[0]));

      auto witness_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init")));
      BOOST_CHECK(con.api()->import_key("init1", key_to_wif(witness_key)));
      BOOST_CHECK(con.api()->import_key("init2", key_to_wif(witness_key)));

      BOOST_TEST_MESSAGE("Importing nathan's balance");
      std::vector<signed_transaction> import_txs = con.api()->import_balance("nathan", nathan_keys, true);
      account_object nathan_acct_before_upgrade = con.api()->get_account("nathan");

      // upgrade nathan
      BOOST_TEST_MESSAGE("Upgrading Nathan to LTM");
      signed_transaction upgrade_tx = con.api()->upgrade_account("nathan", true);
      account_object nathan_acct_after_upgrade = con.api()->get_account("nathan");

      // verify that the upgrade was successful
      BOOST_CHECK_PREDICATE( std::not_equal_to<uint32_t>(), (nathan_acct_before_upgrade.membership_expiration_date.sec_since_epoch())(nathan_acct_after_upgrade.membership_expiration_date.sec_since_epoch()) );
      BOOST_CHECK(nathan_acct_after_upgrade.is_lifetime_member());

      // create a new account
      graphene::wallet::brain_key_info bki = con.api()->suggest_brain_key();
      BOOST_CHECK(!bki.brain_priv_key.empty());
      signed_transaction create_acct_tx = con.api()->create_account_with_brain_key(bki.brain_priv_key, "jmjatlanta", "nathan", "nathan", true);
      // save the private key for this new account in the wallet file
      BOOST_CHECK(con.api()->import_key("jmjatlanta", bki.wif_priv_key));
      con.api()->save_wallet_file(con.wallet_filename());

      // attempt to give jmjatlanta some bitsahres
      BOOST_TEST_MESSAGE("Transferring bitshares from Nathan to jmjatlanta");
      signed_transaction transfer_tx = con.api()->transfer("nathan", "jmjatlanta", "10000", "1.3.0", "Here are some BTS for your new account", true);

      // get the details for init1
      witness_object init1_obj = con.api()->get_witness("init1");
      int init1_start_votes = init1_obj.total_votes;
      // Vote for a witness
      signed_transaction vote_witness1_tx = con.api()->vote_for_witness("jmjatlanta", "init1", true, true);

      // generate a block to get things started
      BOOST_CHECK(generate_block(app1));
      // wait for a maintenance interval
      BOOST_CHECK(generate_maintenance_block(app1));

      // Verify that the vote is there
      init1_obj = con.api()->get_witness("init1");
      witness_object init2_obj = con.api()->get_witness("init2");
      int init1_middle_votes = init1_obj.total_votes;
      BOOST_CHECK(init1_middle_votes > init1_start_votes);

      // Vote for a 2nd witness
      int init2_start_votes = init2_obj.total_votes;
      signed_transaction vote_witness2_tx = con.api()->vote_for_witness("jmjatlanta", "init2", true, true);

      // send another block to trigger maintenance interval
      BOOST_CHECK(generate_maintenance_block(app1));

      // Verify that both the first vote and the 2nd are there
      init2_obj = con.api()->get_witness("init2");
      init1_obj = con.api()->get_witness("init1");

      int init2_middle_votes = init2_obj.total_votes;
      BOOST_CHECK(init2_middle_votes > init2_start_votes);
      int init1_last_votes = init1_obj.total_votes;
      BOOST_CHECK(init1_last_votes > init1_start_votes);

      // wait for everything to finish up
      fc::usleep(fc::seconds(1));
   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
   app1->shutdown();
}

///////////////////
// Start a server and connect using the same calls as the CLI
// Set a voting proxy and be assured that it sticks
///////////////////
BOOST_AUTO_TEST_CASE( cli_set_voting_proxy )
{
   using namespace graphene::chain;
   using namespace graphene::app;
   std::shared_ptr<graphene::app::application> app1;
   try {

      fc::temp_directory app_dir( graphene::utilities::temp_directory_path() );

      int server_port_number;
      app1 = start_application(app_dir, server_port_number);

      // connect to the server
      client_connection con(app1, app_dir, server_port_number);

      BOOST_TEST_MESSAGE("Setting wallet password");
      con.api()->set_password("supersecret");
      con.api()->unlock("supersecret");

      // import Nathan account
      BOOST_TEST_MESSAGE("Importing nathan key");

      auto nathan_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("nathan")));
      std::vector<std::string> nathan_keys{key_to_wif(nathan_key)};
      BOOST_CHECK(con.api()->import_key("nathan", nathan_keys[0]));

      BOOST_TEST_MESSAGE("Importing nathan's balance");
      std::vector<signed_transaction> import_txs = con.api()->import_balance("nathan", nathan_keys, true);
      account_object nathan_acct_before_upgrade = con.api()->get_account("nathan");

      // upgrade nathan
      BOOST_TEST_MESSAGE("Upgrading Nathan to LTM");
      signed_transaction upgrade_tx = con.api()->upgrade_account("nathan", true);
      account_object nathan_acct_after_upgrade = con.api()->get_account("nathan");

      // verify that the upgrade was successful
      BOOST_CHECK_PREDICATE( std::not_equal_to<uint32_t>(), (nathan_acct_before_upgrade.membership_expiration_date.sec_since_epoch())(nathan_acct_after_upgrade.membership_expiration_date.sec_since_epoch()) );
      BOOST_CHECK(nathan_acct_after_upgrade.is_lifetime_member());

      // create a new account
      graphene::wallet::brain_key_info bki = con.api()->suggest_brain_key();
      BOOST_CHECK(!bki.brain_priv_key.empty());
      signed_transaction create_acct_tx = con.api()->create_account_with_brain_key(bki.brain_priv_key, "jmjatlanta", "nathan", "nathan", true);
      // save the private key for this new account in the wallet file
      BOOST_CHECK(con.api()->import_key("jmjatlanta", bki.wif_priv_key));
      con.api()->save_wallet_file(con.wallet_filename());

      // attempt to give jmjatlanta some bitsahres
      BOOST_TEST_MESSAGE("Transferring bitshares from Nathan to jmjatlanta");
      signed_transaction transfer_tx = con.api()->transfer("nathan", "jmjatlanta", "10000", "1.3.0", "Here are some BTS for your new account", true);

      // grab account for comparison
      account_object prior_voting_account = con.api()->get_account("jmjatlanta");
      // set the voting proxy to nathan
      BOOST_TEST_MESSAGE("About to set voting proxy.");
      signed_transaction voting_tx = con.api()->set_voting_proxy("jmjatlanta", "nathan", true);
      account_object after_voting_account = con.api()->get_account("jmjatlanta");
      // see if it changed
      BOOST_CHECK(prior_voting_account.options.voting_account != after_voting_account.options.voting_account);

      // wait for everything to finish up
      fc::usleep(fc::seconds(1));
   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
   app1->shutdown();
}

BOOST_AUTO_TEST_SUITE_END()
}
