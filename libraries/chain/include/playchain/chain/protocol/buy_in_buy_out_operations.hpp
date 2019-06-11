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

#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <playchain/chain/protocol/playchain_types.hpp>

namespace playchain { namespace chain {

   using namespace graphene::chain;

   struct buy_in_table_operation : public base_operation
   {
       struct fee_parameters_type {
           int64_t fee = 0;
       };

       asset                               fee;
       account_id_type                     player;
       table_id_type                       table;
       account_id_type                     table_owner;
       asset                               amount;

       void get_required_active_authorities(flat_set<account_id_type>& a) const {
           a.insert(player);
           a.insert(table_owner); //permission to enter the game
       }

       account_id_type   fee_payer()const { return player; }
       void              validate()const;
   };

   struct buy_out_table_operation : public base_operation
   {
      struct fee_parameters_type {
        uint64_t fee = 0;
        uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      asset                               fee;
      account_id_type                     player;
      table_id_type                       table;
      account_id_type                     table_owner; //< to facilitate the creation of a statistic by table_owner
      asset                               amount;

      string                              reason;

      void get_required_active_authorities( flat_set<account_id_type>& a) const {
        a.clear(); //don't require active authority form fee payer
      }
      void get_required_authorities( vector<authority>& )const;

      account_id_type   fee_payer()const { return table_owner; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k) const
      {
          return calculate_fee_with_kbyte(this, k);
      }
   };

   struct buy_in_reserve_operation : public base_operation
   {
       struct fee_parameters_type {
           int64_t fee = 0;
           uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
       };

       asset                               fee;
       account_id_type                     player;
       string                              uid;
       asset                               amount;
       string                              metadata;
       string                              protocol_version;

       void get_required_active_authorities(flat_set<account_id_type>& a) const {
           a.insert(player);
       }

       account_id_type   fee_payer()const { return player; }
       void              validate()const;
       share_type        calculate_fee(const fee_parameters_type& k) const
       {
           return calculate_fee_with_kbyte(this, k);
       }
   };

   struct buy_in_reserving_cancel_operation : public base_operation
   {
       struct fee_parameters_type {
         uint64_t fee = 0;
         uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
       };

       asset                               fee;
       account_id_type                     player;
       string                              uid;

       void get_required_active_authorities( flat_set<account_id_type>& a) const {
           a.insert(player);
       }

       account_id_type   fee_payer()const { return player; }
       void              validate()const;
       share_type        calculate_fee(const fee_parameters_type& k) const
       {
           return calculate_fee_with_kbyte(this, k);
       }
   };

   struct buy_in_reserving_allocated_table_operation : public base_operation
   {
       struct fee_parameters_type {};

       buy_in_reserving_allocated_table_operation(){}
       buy_in_reserving_allocated_table_operation( const account_id_type &player,
                                const string &uid,
                                const asset &amount,
                                const string &metadata,
                                const table_id_type &table,
                                const account_id_type &table_owner,
                                const int32_t table_weight,
                                const int32_t room_rating)
          :player(player), uid(uid), amount(amount), metadata(metadata),
           table(table),
           table_owner(table_owner),
           table_weight(table_weight),
           room_rating(room_rating)
       {}

       asset                     fee; // always zero
       account_id_type           player;
       string                    uid;
       asset                     amount;
       string                    metadata;
       table_id_type             table;
       account_id_type           table_owner;
       int32_t                   table_weight;
       int32_t                   room_rating;

       account_id_type fee_payer()const { return GRAPHENE_TEMP_ACCOUNT; }
       void            validate()const { FC_ASSERT( !"virtual operation" ); }
       share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
   };

   struct buy_in_reserving_expire_operation : public base_operation
   {
      struct fee_parameters_type {};

      buy_in_reserving_expire_operation(){}
      buy_in_reserving_expire_operation( const account_id_type &player,
                               const string &uid,
                               const asset &amount,
                               const string &metadata,
                               bool expire_by_replacement)
         :player(player), uid(uid), amount(amount), metadata(metadata),
          expire_by_replacement(expire_by_replacement)
      {
      }

      buy_in_reserving_expire_operation(
                               const account_id_type &player,
                               const string &uid,
                               const asset &amount,
                               const string &metadata,
                               bool expire_by_replacement,
                               const table_id_type &table,
                               const account_id_type &table_owner)
         :player(player), uid(uid), amount(amount), metadata(metadata),
          table(table), table_owner(table_owner),
          expire_by_replacement(expire_by_replacement)
      {}

      asset                     fee; // always zero
      account_id_type           player;
      string                    uid;
      asset                     amount;
      string                    metadata;
      table_id_type             table = PLAYCHAIN_NULL_TABLE;
      account_id_type           table_owner = GRAPHENE_NULL_ACCOUNT;

      bool                      expire_by_replacement = false;

      account_id_type fee_payer()const { return GRAPHENE_TEMP_ACCOUNT; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
      share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
   };

   struct buy_in_reserving_resolve_operation : public base_operation
   {
       struct fee_parameters_type {
         uint64_t fee = 0;
       };

      asset                     fee;
      table_id_type             table;
      account_id_type           table_owner;
      pending_buy_in_id_type    pending_buyin;

      void get_required_active_authorities(flat_set<account_id_type>& a) const {
          a.insert(table_owner);
      }

      account_id_type   fee_payer()const { return table_owner; }
      void              validate()const;
   };

   struct buy_in_reserving_cancel_all_operation : public base_operation
   {
       struct fee_parameters_type {
           int64_t fee = 0;
       };

       asset                               fee;
       account_id_type                     player;

       void get_required_active_authorities(flat_set<account_id_type>& a) const {
           a.insert(player);
       }

       account_id_type   fee_payer()const { return player; }
       void              validate()const;
   };

   struct buy_in_expire_operation : public base_operation
   {
      struct fee_parameters_type {};

      buy_in_expire_operation(){}
      buy_in_expire_operation( const account_id_type &player,
                               const table_id_type &table,
                               const account_id_type &table_owner,
                               const asset &amount)
         :player(player),
          table(table),
          table_owner(table_owner),
          amount(amount)
      {}

      asset                     fee; // always zero
      account_id_type           player;
      table_id_type             table;
      account_id_type           table_owner;
      asset                     amount;

      account_id_type fee_payer()const { return GRAPHENE_TEMP_ACCOUNT; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
      share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
   };

}} // namespace graphene::chain

FC_REFLECT( playchain::chain::buy_in_table_operation::fee_parameters_type, (fee))
FC_REFLECT( playchain::chain::buy_out_table_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::buy_in_reserve_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::buy_in_reserving_cancel_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::buy_in_reserving_allocated_table_operation::fee_parameters_type, )
FC_REFLECT( playchain::chain::buy_in_reserving_expire_operation::fee_parameters_type, )
FC_REFLECT( playchain::chain::buy_in_reserving_resolve_operation::fee_parameters_type, (fee))
FC_REFLECT( playchain::chain::buy_in_reserving_cancel_all_operation::fee_parameters_type, (fee))
FC_REFLECT( playchain::chain::buy_in_expire_operation::fee_parameters_type, )

FC_REFLECT( playchain::chain::buy_in_table_operation, (fee)(player)(table)(table_owner)(amount))
FC_REFLECT( playchain::chain::buy_out_table_operation, (fee)(player)(table)(table_owner)(amount)(reason))
FC_REFLECT( playchain::chain::buy_in_reserve_operation, (fee)(player)(uid)(amount)(metadata)(protocol_version))
FC_REFLECT( playchain::chain::buy_in_reserving_cancel_operation, (fee)(player)(uid))
FC_REFLECT( playchain::chain::buy_in_reserving_allocated_table_operation, (fee)(player)(uid)(amount)(metadata)
            (table)(table_owner)(table_weight)(room_rating))
FC_REFLECT( playchain::chain::buy_in_reserving_expire_operation, (fee)(player)(uid)(amount)(metadata)(expire_by_replacement)(table)(table_owner))
FC_REFLECT( playchain::chain::buy_in_reserving_resolve_operation, (fee)(table)(table_owner)(pending_buyin))
FC_REFLECT( playchain::chain::buy_in_reserving_cancel_all_operation, (fee)(player))
FC_REFLECT( playchain::chain::buy_in_expire_operation, (fee)(player)(table)(table_owner)(amount))
