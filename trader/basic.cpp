#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

#include <eosio.token.hpp>
#include <defibox.hpp>
#include <uniswap.hpp>
#include <registry.sx.hpp>
#include <dfs.hpp>


using namespace eosio;
using namespace std;

class [[eosio::contract]] basic : public eosio::contract {       
    
public:
    basic(eosio::name rec, eosio::name code, datastream<const char*> ds) 
      : contract(rec, code, ds)
    {};

    [[eosio::action]]
    void getcommon() {
      auto pairs = get_all_pairs({{"EOS",4}, "eosio.token"_n}); 
      
      //fetch common symbols into vector
      vector<symbol_code> common;
      for(auto& p: pairs[0]){
        for(int i=1; i<pairs.size(); i++){
          if(pairs[i].count(p.first)==0) break;
          if(i==pairs.size()-1) common.push_back(p.first);
        }
      }
      //dump all prices into map of strings and log
      map<symbol_code, string> prices;
      asset tokens{10*10000, {"EOS", 4}};
      for(auto sym: common){
        for(auto& dex: {"defibox", "dfs"}){
          auto [ex, out, tcontract, memo] = get_trade(dex, tokens, sym);
          prices[sym] += (string(dex) + ": " + out.to_string() + "; ");
        }
      }
      
      logcommon_action logcommon( get_self(), { get_self(), "active"_n });
      logcommon.send( prices ); 
/*
      //get best prices for common symbols into map
      map<symbol_code, asset> best_prices;
      asset tokens{10*10000, {"EOS", 4}};
      for(auto sym: common){
        for(auto& dex: {"defibox", "dfs"}){
          auto [ex, out, tcontract, memo] = get_trade(dex, tokens, sym);
          if(best_prices.count(sym))
            best_prices[sym] = min(best_prices[sym], out);
          else
            best_prices[sym] = out;
        }
      }

      //just log the result
      logbestcomm_action logbestcomm( get_self(), { get_self(), "active"_n });
      logbestcomm.send( best_prices );  
      */  
    }

    [[eosio::action]]
    void trade(eosio::asset tokens, eosio::asset minreturn, std::string exchange){
      
      check( tokens.amount > 0 && minreturn.amount > 0, "Invalid tokens amount" );
      

      auto [dex, out, tcontract, memo] = get_trade(exchange, tokens, minreturn.symbol.code());
      

      check(minreturn <= out, "Return is not enough");
      
      // log out price
      log_action log( get_self(), { get_self(), "active"_n });
      log.send( out );    
      
      // make a swap
      token::transfer_action transfer( tcontract, { get_self(), "active"_n });
      transfer.send( get_self(), dex, tokens, memo);
              
    }

    [[eosio::action]]
    void log( asset& out )
    {
        require_auth( get_self() );
    }

    [[eosio::action]]
    void logbestcomm( const map<symbol_code, asset>& prices )
    {
        require_auth( get_self() );
    }

    [[eosio::action]]
    void logcommon( const map<symbol_code, string>& prices )
    {
        require_auth( get_self() );
    }

    using log_action = eosio::action_wrapper<"log"_n, &basic::log>;
    using logbestcomm_action = eosio::action_wrapper<"logbestcomm"_n, &basic::logbestcomm>;
    using logcommon_action = eosio::action_wrapper<"logcommon"_n, &basic::logcommon>;
        
  private:
    std::tuple<eosio::name, eosio::asset, eosio::name, std::string> get_trade(std::string exchange, eosio::asset& tokens, eosio::symbol_code to){
      
      check(exchange == "defibox" || exchange == "dfs", exchange + " exchange is not supported");

      if(exchange == "defibox") return get_defi_trade(tokens, to);
      
      return get_dfs_trade(tokens, to);
    }

    
    std::tuple<eosio::name, eosio::asset, eosio::name, std::string> get_defi_trade(eosio::asset& tokens, eosio::symbol_code to){
      
      sx::registry::defibox_table defi_table( "registry.sx"_n, "registry.sx"_n.value );
      
      uint64_t pair_id = 0;
      name tcontract;
      for(const auto& row: defi_table){
        if(row.base.get_symbol() == tokens.symbol){
          check( row.quotes.count(to), "Target currency is not supported" );
          pair_id = row.quotes.at(to);
          tcontract = row.base.get_contract();
          break;
        }
      }

      check( pair_id > 0, "This pair is not supported");
      
      // get reserves
      const auto [ reserve_in, reserve_out ] = defibox::get_reserves( pair_id, tokens.symbol );
      const uint8_t fee = defibox::get_fee();

      // calculate out price  
      const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );
      
      return {"swap.defi"_n, out, tcontract, "swap,0," + to_string((int) pair_id)};
    }


    
    std::tuple<eosio::name, eosio::asset, eosio::name, std::string> get_dfs_trade(eosio::asset& tokens, eosio::symbol_code to){
      sx::registry::dfs_table dfs_table( "registry.sx"_n, "registry.sx"_n.value );
      
      uint64_t pair_id = 0;
      name tcontract;
      for(const auto& row: dfs_table){
        if(row.base.get_symbol() == tokens.symbol){
          check( row.quotes.count(to), "Target currency is not supported" );
          pair_id = row.quotes.at(to);
          tcontract = row.base.get_contract();
          break;
        }
      }

      check( pair_id > 0, "This pair is not supported");
      
      // get reserves
      const auto [ reserve_in, reserve_out ] = dfs::get_reserves( pair_id, tokens.symbol );
      const uint8_t fee = dfs::get_fee();

      // calculate out price  
      const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

      return {"defisswapcnt"_n, out, tcontract, "swap:" + to_string((int) pair_id)+":0"};
    }

    std::vector<std::map<symbol_code, uint64_t>> get_all_pairs(extended_symbol sym){
      vector<map<symbol_code, uint64_t>> res;

      sx::registry::dfs_table dfs_table( "registry.sx"_n, "registry.sx"_n.value );
      auto dfsrow = dfs_table.get(sym.get_symbol().code().raw(), "DFS doesn't trade this currency");
      res.push_back(dfsrow.quotes);

      
      sx::registry::defibox_table defi_table( "registry.sx"_n, "registry.sx"_n.value );
      auto defirow = defi_table.get(sym.get_symbol().code().raw(), "Defibox doesn't trade this currency");
      res.push_back(defirow.quotes);

      return res;
    }
};