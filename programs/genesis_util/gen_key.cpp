#include <iostream>
#include <string>

#include <fc/crypto/elliptic.hpp>
#include <fc/io/json.hpp>

#include <graphene/chain/protocol/address.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/utilities/key_conversion.hpp>

#ifndef WIN32
#include <csignal>
#endif

using namespace std;

int main( int argc, char** argv )
{
    try
    {
        std::string dev_key_prefix;
        bool need_help = false;
        if( argc <= 1 )
        {
            need_help = true;
        }

        if( need_help )
        {
            std::cerr << "usage: gen-key <secret>" << "\n";
            return 1;
        }

        auto show_key = []( const fc::ecc::private_key& priv_key )
        {
            fc::limited_mutable_variant_object mvo(5);
            graphene::chain::public_key_type pub_key = priv_key.get_public_key();
            mvo( "private_key", graphene::utilities::key_to_wif( priv_key ) )
                    ( "public_key", std::string( pub_key ) )
                    ( "address", graphene::chain::address( pub_key ) )
                    ;
            std::cout << fc::json::to_string( fc::mutable_variant_object(mvo) );
        };

        std::string secret = argv[1];

        std::cout << secret << "\n";

        show_key( fc::ecc::private_key::regenerate( fc::sha256::hash( secret ) ) );

    }
    catch ( const fc::exception& e )
    {
        std::cout << e.to_detail_string() << "\n";
        return 1;
    }
    return 0;
}
