
add_custom_target( build_hardfork_hpp
                   COMMAND cat-parts "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp" )
set_source_files_properties( "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp" PROPERTIES GENERATED TRUE )

add_dependencies( build_hardfork_hpp cat-parts )

option(PRINT_PLAYCHAIN_TABLES_RANGE "Switch printing for playchain tables range in debug (ON OR OFF)" OFF)

if ( PRINT_PLAYCHAIN_TABLES_RANGE )
    add_definitions(-DPRINT_PLAYCHAIN_TABLES_RANGE)
endif()

file(GLOB HEADERS "include/graphene/chain/*.hpp")
file(GLOB_RECURSE PLAYCHAIN_HEADERS "include/playchain/chain/*.hpp")
file(GLOB PROTOCOL_HEADERS "include/graphene/chain/protocol/*.hpp")
file(GLOB BLOCK_TASKS_HEADERS "playchain/block_tasks/*.hpp")
file(GLOB MAINTAIN_TASKS_HEADERS "playchain/maintain_tasks/*.hpp")

if( GRAPHENE_DISABLE_UNITY_BUILD )
   set( GRAPHENE_DB_FILES
        db_balance.cpp
        db_block.cpp
        db_debug.cpp
        db_getter.cpp
        db_init.cpp
        db_maint.cpp
        db_management.cpp
        db_market.cpp
        db_update.cpp
        db_witness_schedule.cpp
        db_notify.cpp
      )
   message( STATUS "Graphene database unity build disabled" )
else( GRAPHENE_DISABLE_UNITY_BUILD )
   set( GRAPHENE_DB_FILES
        database.cpp )
   message( STATUS "Graphene database unity build enabled" )
endif( GRAPHENE_DISABLE_UNITY_BUILD )

## SORT .cpp by most likely to change / break compile
add_library( graphene_chain

             # As database takes the longest to compile, start it first
             ${GRAPHENE_DB_FILES}
             fork_database.cpp

             protocol/types.cpp
             protocol/address.cpp
             protocol/authority.cpp
             protocol/asset.cpp
             protocol/assert.cpp
             protocol/account.cpp
             protocol/transfer.cpp
             protocol/chain_parameters.cpp
             protocol/committee_member.cpp
             protocol/witness.cpp
             protocol/market.cpp
             protocol/proposal.cpp
             protocol/withdraw_permission.cpp
             protocol/asset_ops.cpp
             protocol/memo.cpp
             protocol/worker.cpp
             protocol/custom.cpp
             protocol/operations.cpp
             protocol/transaction.cpp
             protocol/block.cpp
             protocol/fee_schedule.cpp
             protocol/confidential.cpp
             protocol/vote.cpp
             protocol/htlc.cpp
             protocol/sign_state.cpp

             genesis_state.cpp
             get_config.cpp

             pts_address.cpp

             evaluator.cpp
             balance_evaluator.cpp
             account_evaluator.cpp
             assert_evaluator.cpp
             witness_evaluator.cpp
             committee_member_evaluator.cpp
             asset_evaluator.cpp
             transfer_evaluator.cpp
             proposal_evaluator.cpp
             market_evaluator.cpp
             vesting_balance_evaluator.cpp
             withdraw_permission_evaluator.cpp
             worker_evaluator.cpp
             htlc_evaluator.cpp
             confidential_evaluator.cpp
             special_authority.cpp
             buyback.cpp

             account_object.cpp
             asset_object.cpp
             fba_object.cpp
             market_object.cpp
             proposal_object.cpp
             vesting_balance_object.cpp

             block_database.cpp

             is_authorized_asset.cpp

             playchain/playchain_config.cpp
             playchain/playchain_utils.cpp
             playchain/version.cpp
             playchain/impacted.cpp
             playchain/block_tasks/block_tasks.cpp
             playchain/maintain_tasks/maintain_tasks.cpp
             playchain/protocol/game_operations.cpp
             playchain/protocol/table_operations.cpp
             playchain/protocol/buy_in_buy_out_operations.cpp
             playchain/protocol/player_invitation_operations.cpp
             playchain/protocol/room_operations.cpp
             playchain/protocol/player_operations.cpp
             playchain/protocol/common_operations.cpp
             playchain/protocol/playchain_committee_member.cpp
             playchain/evaluators/game_evaluators.cpp
             playchain/evaluators/table_evaluators.cpp
             playchain/evaluators/buy_in_buy_out_evaluators.cpp
             playchain/evaluators/player_invitation_evaluators.cpp
             playchain/evaluators/room_evaluators.cpp
             playchain/evaluators/validators.cpp
             playchain/evaluators/db_helpers.cpp
             playchain/evaluators/player_evaluators.cpp
             playchain/evaluators/common_evaluators.cpp
             playchain/evaluators/playchain_committee_member_evaluator.cpp
             playchain/block_tasks/player_invitation.cpp
             playchain/block_tasks/table_check.cpp
             playchain/block_tasks/pending_buy_in.cpp
             playchain/maintain_tasks/room_rating.cpp
             playchain/maintain_tasks/deposit_pending_fees.cpp
             playchain/maintain_tasks/committee_applying.cpp
             playchain/schema/playchain_objects.cpp
             playchain/schema/table_object.cpp

             ${BLOCK_TASKS_HEADERS}
             ${MAINTAIN_TASKS_HEADERS}
             ${PLAYCHAIN_HEADERS}
             ${HEADERS}
             ${PROTOCOL_HEADERS}
             "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp"
           )

add_dependencies( graphene_chain build_hardfork_hpp )
target_link_libraries( graphene_chain fc graphene_db )
target_include_directories( graphene_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

# to view hardfork files in IDE
set(hardfork_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d)
file(GLOB HARDFORK_FILES "${hardfork_root_dir}/*.hf")

add_custom_target( hardforks SOURCES
                   ${HARDFORK_FILES})

INSTALL( TARGETS
   graphene_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/chain" )
INSTALL( FILES ${PROTOCOL_HEADERS} DESTINATION "include/graphene/chain/protocol" )
