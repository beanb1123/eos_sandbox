#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

#include <eosio.token.hpp>
#include <defibox.hpp>
#include <uniswap.hpp>


using namespace eosio;
using namespace std;

class [[eosio::contract]] basic : public eosio::contract {       
    
public:
    basic(eosio::name rec, eosio::name code, datastream<const char*> ds) 
      : contract(rec, code, ds)
    {};

    map<eosio::symbol, pair<eosio::name, uint64_t>> _tcontracts = {
      {{"EOS", 4}, {"eosio.token"_n, 12}},
      {{"USDT", 4}, {"tethertether"_n, 12}}
    };

    [[eosio::action]]
    void trade(eosio::asset tokens){
      
      check(_tcontracts.count(tokens.symbol), "This currency is not supported");
    
      const auto [tcontract, pair_id] = _tcontracts[tokens.symbol];
      
      // get reserves
      const auto [ reserve_in, reserve_out ] = defibox::get_reserves( pair_id, tokens.symbol );
      const uint8_t fee = defibox::get_fee();

      print( reserve_in );
      print( reserve_out );  

      // calculate out price      // calculate out price
      const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

      
      // log out price
      log_action log( get_self(), { get_self(), "active"_n });
      log.send( out );    
      
      // make a swap
      token::transfer_action transfer( tcontract, { get_self(), "active"_n });
      transfer.send( get_self(), "swap.defi"_n, tokens, "swap,0," + to_string((int) pair_id) );
              
    }

    [[eosio::action]]
    void log( const asset out )
    {
        require_auth( get_self() );
    }

    using log_action = eosio::action_wrapper<"log"_n, &basic::log>;
         
};