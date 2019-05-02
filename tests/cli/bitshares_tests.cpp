#include <boost/test/unit_test.hpp>

#include "cli_tests_fixture.hpp"

#include <fc/crypto/hex.hpp>

namespace account_create_tests
{
using namespace cli;

BOOST_FIXTURE_TEST_SUITE( bitshares_tests, cli_tests_fixture )

#if 0

///////////////////
// Test blind transactions and mantissa length of range proofs.
///////////////////
BOOST_AUTO_TEST_CASE( cli_confidential_tx_test )
{
   using namespace graphene::wallet;
   try {
      // we need to increase the default max transaction size to run this test.
      cli_app->chain_database()->modify(
         cli_app->chain_database()->get_global_properties(),
         []( global_property_object& p) {
            p.parameters.maximum_transaction_size = 8192;
      });
      std::vector<signed_transaction> import_txs;

      auto nathan_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("nathan")));
      std::vector<std::string> nathan_keys{key_to_wif(nathan_key)};

      BOOST_TEST_MESSAGE("Importing nathan's balance");
      import_txs = connection->api()->import_balance("nathan", nathan_keys, true);

      unsigned int head_block = 0;
      auto & W = *connection->api(); // Wallet alias

      BOOST_TEST_MESSAGE("Creating blind accounts");
      graphene::wallet::brain_key_info bki_nathan = W.suggest_brain_key();
      graphene::wallet::brain_key_info bki_alice = W.suggest_brain_key();
      graphene::wallet::brain_key_info bki_bob = W.suggest_brain_key();
      W.create_blind_account("nathan", bki_nathan.brain_priv_key);
      W.create_blind_account("alice", bki_alice.brain_priv_key);
      W.create_blind_account("bob", bki_bob.brain_priv_key);
      BOOST_CHECK(W.get_blind_accounts().size() == 3);

      // ** Block 1: Import Nathan account:
      BOOST_TEST_MESSAGE("Importing nathan key and balance");
      nathan_keys = std::vector<std::string>{"5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"};
      W.import_key("nathan", nathan_keys[0]);
      W.import_balance("nathan", nathan_keys, true);
      generate_block(cli_app); head_block++;

      // ** Block 2: Nathan will blind 100M CORE token:
      BOOST_TEST_MESSAGE("Blinding a large balance");
      W.transfer_to_blind("nathan", GRAPHENE_SYMBOL, {{"nathan","100000000"}}, true);
      BOOST_CHECK( W.get_blind_balances("nathan")[0].amount == 10000000000000 );
      generate_block(cli_app); head_block++;

      // ** Block 3: Nathan will send 1M CORE token to alice and 10K CORE token to bob. We
      // then confirm that balances are received, and then analyze the range
      // prooofs to make sure the mantissa length does not reveal approximate
      // balance (issue #480).
      std::map<std::string, share_type> to_list = {{"alice",100000000000},
                                                   {"bob",    1000000000}};
      vector<blind_confirmation> bconfs;
      asset_object core_asset = W.get_asset("1.3.0");
      BOOST_TEST_MESSAGE("Sending blind transactions to alice and bob");
      for (auto to : to_list) {
         string amount = core_asset.amount_to_string(to.second);
         bconfs.push_back(W.blind_transfer("nathan",to.first,amount,core_asset.symbol,true));
         BOOST_CHECK( W.get_blind_balances(to.first)[0].amount == to.second );
      }
      BOOST_TEST_MESSAGE("Inspecting range proof mantissa lengths");
      vector<int> rp_mantissabits;
      for (auto conf : bconfs) {
         for (auto out : conf.trx.operations[0].get<blind_transfer_operation>().outputs) {
            rp_mantissabits.push_back(1+out.range_proof[1]); // 2nd byte encodes mantissa length
         }
      }
      // We are checking the mantissa length of the range proofs for several Pedersen
      // commitments of varying magnitude.  We don't want the mantissa lengths to give
      // away magnitude.  Deprecated wallet behavior was to use "just enough" mantissa
      // bits to prove range, but this gives away value to within a factor of two. As a
      // naive test, we assume that if all mantissa lengths are equal, then they are not
      // revealing magnitude.  However, future more-sophisticated wallet behavior
      // *might* randomize mantissa length to achieve some space savings in the range
      // proof.  The following test will fail in that case and a more sophisticated test
      // will be needed.
      auto adjacent_unequal = std::adjacent_find(rp_mantissabits.begin(),
                                                 rp_mantissabits.end(),         // find unequal adjacent values
                                                 std::not_equal_to<int>());
      BOOST_CHECK(adjacent_unequal == rp_mantissabits.end());
      generate_block(cli_app); head_block++;

      // ** Check head block:
      BOOST_TEST_MESSAGE("Check that all expected blocks have processed");
      dynamic_global_property_object dgp = W.get_dynamic_global_properties();
      BOOST_CHECK(dgp.head_block_number == head_block);
   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
}

/******
 * Check account history pagination (see bitshares-core/issue/1176)
 */
BOOST_AUTO_TEST_CASE( account_history_pagination )
{
   try
   {
      create_new_account();

      // attempt to give jmjatlanta some bitsahres
      BOOST_TEST_MESSAGE("Transferring bitshares from Nathan to jmjatlanta");
      for(int i = 1; i <= 199; i++)
      {
         signed_transaction transfer_tx = connection->api()->transfer("nathan", "jmjatlanta", std::to_string(i),
                                                "1.3.0", "Here are some CORE token for your new account", true);
      }

      BOOST_CHECK(generate_block(cli_app));

      // now get account history and make sure everything is there (and no duplicates)
      std::vector<graphene::wallet::operation_detail> history = connection->api()->get_account_history("jmjatlanta", 300);
      BOOST_CHECK_EQUAL(201u, history.size() );

      std::set<object_id_type> operation_ids;

      for(auto& op : history)
      {
         if( operation_ids.find(op.op.id) != operation_ids.end() )
         {
            BOOST_FAIL("Duplicate found");
         }
         operation_ids.insert(op.op.id);
      }
   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
}

///////////////////////
// Start a server and connect using the same calls as the CLI
// Create an HTLC
///////////////////////
BOOST_AUTO_TEST_CASE( cli_create_htlc )
{
   using namespace graphene::chain;
   using namespace graphene::app;
   std::shared_ptr<graphene::app::application> app1;
   try {
      // set committee parameters
      cli_app->chain_database()->modify(app1->chain_database()->get_global_properties(), [](global_property_object& p) {
         graphene::chain::htlc_options params;
         params.max_preimage_size = 1024;
         params.max_timeout_secs = 60 * 60 * 24 * 28;
         p.parameters.extensions.value.updatable_htlc_options = params;
      });

      BOOST_TEST_MESSAGE("Setting wallet password");
      connection->api()->set_password("supersecret");
      connection->api()->unlock("supersecret");

      // import Nathan account
      BOOST_TEST_MESSAGE("Importing nathan key");
      std::vector<std::string> nathan_keys{"5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"};
      BOOST_CHECK_EQUAL(nathan_keys[0], "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3");
      BOOST_CHECK(connection->api()->import_key("nathan", nathan_keys[0]));

      BOOST_TEST_MESSAGE("Importing nathan's balance");
      std::vector<signed_transaction> import_txs = connection->api()->import_balance("nathan", nathan_keys, true);
      account_object nathan_acct_before_upgrade = connection->api()->get_account("nathan");

      // upgrade nathan
      BOOST_TEST_MESSAGE("Upgrading Nathan to LTM");
      signed_transaction upgrade_tx = connection->api()->upgrade_account("nathan", true);
      account_object nathan_acct_after_upgrade = connection->api()->get_account("nathan");

      // verify that the upgrade was successful
      BOOST_CHECK_PREDICATE( std::not_equal_to<uint32_t>(), (nathan_acct_before_upgrade.membership_expiration_date.sec_since_epoch())
            (nathan_acct_after_upgrade.membership_expiration_date.sec_since_epoch()) );
      BOOST_CHECK(nathan_acct_after_upgrade.is_lifetime_member());

      // Create new asset called BOBCOIN
      try
      {
         graphene::chain::asset_options asset_ops;
         asset_ops.max_supply = 1000000;
         asset_ops.core_exchange_rate = price(asset(2),asset(1,asset_id_type(1)));
         fc::optional<graphene::chain::bitasset_options> bit_opts;
         connection->api()->create_asset("nathan", "BOBCOIN", 5, asset_ops, bit_opts, true);
      }
      catch(exception& e)
      {
         BOOST_FAIL(e.what());
      }
      catch(...)
      {
         BOOST_FAIL("Unknown exception creating BOBCOIN");
      }

      // create a new account for Alice
      {
         graphene::wallet::brain_key_info bki = connection->api()->suggest_brain_key();
         BOOST_CHECK(!bki.brain_priv_key.empty());
         signed_transaction create_acct_tx = connection->api()->create_account_with_brain_key(bki.brain_priv_key, "alice",
               "nathan", "nathan", true);
         // save the private key for this new account in the wallet file
         BOOST_CHECK(connection->api()->import_key("alice", bki.wif_priv_key));
         connection->api()->save_wallet_file(connection->wallet_filename());
         // attempt to give alice some bitsahres
         BOOST_TEST_MESSAGE("Transferring bitshares from Nathan to alice");
         signed_transaction transfer_tx = connection->api()->transfer("nathan", "alice", "10000", "1.3.0",
               "Here are some CORE token for your new account", true);
      }

      // create a new account for Bob
      {
         graphene::wallet::brain_key_info bki = connection->api()->suggest_brain_key();
         BOOST_CHECK(!bki.brain_priv_key.empty());
         signed_transaction create_acct_tx = connection->api()->create_account_with_brain_key(bki.brain_priv_key, "bob",
               "nathan", "nathan", true);
         // save the private key for this new account in the wallet file
         BOOST_CHECK(connection->api()->import_key("bob", bki.wif_priv_key));
         connection->api()->save_wallet_file(connection->wallet_filename());
         // attempt to give bob some bitsahres
         BOOST_TEST_MESSAGE("Transferring bitshares from Nathan to Bob");
         signed_transaction transfer_tx = connection->api()->transfer("nathan", "bob", "10000", "1.3.0",
               "Here are some CORE token for your new account", true);
         connection->api()->issue_asset("bob", "5", "BOBCOIN", "Here are your BOBCOINs", true);
      }


      BOOST_TEST_MESSAGE("Alice has agreed to buy 3 BOBCOIN from Bob for 3 BTS. Alice creates an HTLC");
      // create an HTLC
      std::string preimage_string = "My Secret";
      fc::sha256 preimage_md = fc::sha256::hash(preimage_string);
      std::stringstream ss;
      for(size_t i = 0; i < preimage_md.data_size(); i++)
      {
         char d = preimage_md.data()[i];
         unsigned char uc = static_cast<unsigned char>(d);
         ss << std::setfill('0') << std::setw(2) << std::hex << (int)uc;
      }
      std::string hash_str = ss.str();
      BOOST_TEST_MESSAGE("Secret is " + preimage_string + " and hash is " + hash_str);
      uint32_t timelock = fc::days(1).to_seconds();
      graphene::chain::signed_transaction result_tx
            = connection->api()->htlc_create("alice", "bob",
            "3", "1.3.0", "SHA256", hash_str, preimage_string.size(), timelock, true);

      // normally, a wallet would watch block production, and find the transaction. Here, we can cheat:
      std::string alice_htlc_id_as_string;
      {
         BOOST_TEST_MESSAGE("The system is generating a block");
         graphene::chain::signed_block result_block;
         BOOST_CHECK(generate_block(cli_app, result_block));

         // get the ID:
         htlc_id_type htlc_id = result_block.transactions[result_block.transactions.size()-1].operation_results[0].get<object_id_type>();
         alice_htlc_id_as_string = (std::string)(object_id_type)htlc_id;
         BOOST_TEST_MESSAGE("Alice shares the HTLC ID with Bob. The HTLC ID is: " + alice_htlc_id_as_string);
      }

      // Bob can now look over Alice's HTLC, to see if it is what was agreed to.
      BOOST_TEST_MESSAGE("Bob retrieves the HTLC Object by ID to examine it.");
      auto alice_htlc = connection->api()->get_htlc(alice_htlc_id_as_string);
      BOOST_TEST_MESSAGE("The HTLC Object is: " + fc::json::to_pretty_string(alice_htlc));

      // Bob likes what he sees, so he creates an HTLC, using the info he retrieved from Alice's HTLC
      connection->api()->htlc_create("bob", "alice",
            "3", "BOBCOIN", "SHA256", hash_str, preimage_string.size(), timelock, true);

      // normally, a wallet would watch block production, and find the transaction. Here, we can cheat:
      std::string bob_htlc_id_as_string;
      {
         BOOST_TEST_MESSAGE("The system is generating a block");
         graphene::chain::signed_block result_block;
         BOOST_CHECK(generate_block(cli_app, result_block));

         // get the ID:
         htlc_id_type htlc_id = result_block.transactions[result_block.transactions.size()-1].operation_results[0].get<object_id_type>();
         bob_htlc_id_as_string = (std::string)(object_id_type)htlc_id;
         BOOST_TEST_MESSAGE("Bob shares the HTLC ID with Alice. The HTLC ID is: " + bob_htlc_id_as_string);
      }

      // Alice can now look over Bob's HTLC, to see if it is what was agreed to:
      BOOST_TEST_MESSAGE("Alice retrieves the HTLC Object by ID to examine it.");
      auto bob_htlc = connection->api()->get_htlc(bob_htlc_id_as_string);
      BOOST_TEST_MESSAGE("The HTLC Object is: " + fc::json::to_pretty_string(bob_htlc));

      // Alice likes what she sees, so uses her preimage to get her BOBCOIN
      {
         BOOST_TEST_MESSAGE("Alice uses her preimage to retrieve the BOBCOIN");
         std::string secret = "My Secret";
         connection->api()->htlc_redeem(bob_htlc_id_as_string, "alice", secret, true);
         BOOST_TEST_MESSAGE("The system is generating a block");
         BOOST_CHECK(generate_block(cli_app));
      }

      // TODO: Bob can look at Alice's history to see her preimage
      // Bob can use the preimage to retrieve his BTS
      {
         BOOST_TEST_MESSAGE("Bob uses Alice's preimage to retrieve the BOBCOIN");
         std::string secret = "My Secret";
         connection->api()->htlc_redeem(alice_htlc_id_as_string, "bob", secret, true);
         BOOST_TEST_MESSAGE("The system is generating a block");
         BOOST_CHECK(generate_block(cli_app));
      }

      // wait for everything to finish up
      fc::usleep(fc::seconds(1));
   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
   app1->shutdown();
}
#endif

BOOST_AUTO_TEST_SUITE_END()

}
