#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

#include <eosio.token.hpp>
#include <defibox.hpp>
#include <uniswap.hpp>
#include <registry.sx.hpp>


using namespace eosio;
using namespace std;

class [[eosio::contract]] basic : public eosio::contract {       
    
public:
    basic(eosio::name rec, eosio::name code, datastream<const char*> ds) 
      : contract(rec, code, ds)
    {};

    [[eosio::action]]
    void trade(eosio::asset tokens, eosio::asset minreturn, std::string exchange){
      
      check( tokens.amount > 0 && minreturn.amount > 0, "Invalid tokens amount" );
      check( exchange=="defibox", "Only defibox is supported right now" );

      registrySx::defibox_table defi_table( "registry.sx"_n, "registry.sx"_n.value );
      
      uint64_t pair_id = 0;
      name tcontract;
      for(const auto& row: defi_table){
        if(row.base.get_symbol() == tokens.symbol){
          check( row.pair_ids.count(minreturn.symbol.code()), "Target currency is not supported" );
          pair_id = row.pair_ids.at(minreturn.symbol.code());
          tcontract = row.base.get_contract();
          break;
        }
      }

      check( pair_id > 0, "This pair is not supported");
      
      // get reserves
      const auto [ reserve_in, reserve_out ] = defibox::get_reserves( pair_id, tokens.symbol );
      const uint8_t fee = defibox::get_fee();

      print( reserve_in );
      print( reserve_out );  

      // calculate out price  
      const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

      check(minreturn <= out, "Return is not enough");
      
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