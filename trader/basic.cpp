#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

#include <eosio.token.hpp>
#include <defibox.hpp>
#include <uniswap.hpp>
#include <flash.sx.hpp>
#include <swap.sx.hpp>
#include <registry.sx.hpp>
#include <dfs.hpp>

#include "basic.hpp"

using namespace eosio;
using namespace std;

    
[[eosio::action]]
void basic::mine(asset eos_tokens) {
  
  if (!has_auth("miner.sx"_n)) require_auth(get_self());

  auto [profit, sym, dex_sell, dex_buy] = get_best_arb_opportunity(eos_tokens);
  
  print( "Best profit: " + profit.to_string() + " EOS<->" + sym.to_string() + " " + dex_sell + "=>" + dex_buy );
  
  check( profit.amount > 0, "No profits for "+eos_tokens.to_string()+". Closest: " + profit.to_string() + " with " + sym.to_string() + " " + dex_sell + "=>" + dex_buy );
  
  basic::tradeplan _tradeplan( get_self(), get_self().value );
  _tradeplan.set({eos_tokens, dex_sell, dex_buy, sym, profit}, get_self());

  //borrow stake
  flash::borrow_action borrow( "flash.sx"_n, { get_self(), "active"_n });
  borrow.send( get_self(), "eosio.token"_n, eos_tokens, "Loan", "" );
  

}

[[eosio::action]]
void basic::trade(asset tokens, asset minreturn, string exchange){
  
  check( tokens.amount > 0 && minreturn.amount > 0, "Invalid tokens amount" );
  

  auto [dex, out, tcontract, memo] = get_trade_data(exchange, tokens, minreturn.symbol.code());
  
  check(out.amount > 0, "Trade pair not supported");
  check(minreturn <= out || minreturn.amount==0, "Return is not enough");
  
  // log out price
  log_action log( get_self(), { get_self(), "active"_n });
  log.send( out );    
  
  // make a swap
  token::transfer_action transfer( tcontract, { get_self(), "active"_n });
  transfer.send( get_self(), dex, tokens, memo);
          
}

asset basic::make_trade(asset tokens, symbol_code sym, string exchange){
  
  check( tokens.amount > 0, "Invalid tokens amount" );
  
  auto [dex, out, tcontract, memo] = get_trade_data(exchange, tokens, sym);
  
  check(out.amount > 0, "Trade pair not supported");
  print("Sending "+tokens.to_string()+" to "+dex.to_string()+" to buy "+sym.to_string()+" with memo "+memo+"\n");

  // make a trade
  token::transfer_action transfer( tcontract, { get_self(), "active"_n });
  transfer.send( get_self(), dex, tokens, memo);

  return out;

}

tuple<name, asset, name, string> basic::get_trade_data(string exchange, asset tokens, symbol_code to){
  
  if(exchange == "defibox") return get_defi_trade_data(tokens, to);
  if(exchange == "dfs") return get_dfs_trade_data(tokens, to);
  if(exchange == "swapsx") return get_swapsx_trade_data(tokens, to);
  
  check(false, exchange + " exchange is not supported");
  return get_dfs_trade_data(tokens, to);  //dummy

}

    
tuple<name, asset, name, string> basic::get_defi_trade_data(asset tokens, symbol_code to){
  
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

 if(pair_id == 0)
    return  {"null"_n, {0, tokens.symbol}, "null"_n, ""};  //This pair is not supported. 
  
  // get reserves
  const auto [ reserve_in, reserve_out ] = defibox::get_reserves( pair_id, tokens.symbol );
  const uint8_t fee = defibox::get_fee();

  // calculate out price  
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );
  
  return {"swap.defi"_n, out, tcontract, "swap,0," + to_string((int) pair_id)};
}


    
tuple<name, asset, name, string> basic::get_dfs_trade_data(asset tokens, symbol_code to){
  
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

  if(pair_id == 0)
    return  {"null"_n, {0, tokens.symbol}, "null"_n, ""};  //This pair is not supported. 
  
  // get reserves
  const auto [ reserve_in, reserve_out ] = dfs::get_reserves( pair_id, tokens.symbol );
  const uint8_t fee = dfs::get_fee();

  // calculate out price  
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"defisswapcnt"_n, out, tcontract, "swap:" + to_string((int) pair_id)+":0"};
}


tuple<name, asset, name, string> basic::get_swapsx_trade_data(asset tokens, symbol_code to){
  
  swapSx::tokens _tokens( "swap.sx"_n, "swap.sx"_n.value );
  name tcontract = "null"_n;
  bool foundto=false;
  for ( const auto token : _tokens ) {
    if(token.sym.code()==tokens.symbol.code())    
      tcontract = token.contract;
    if(token.sym.code()==to)
      foundto = true;
  }

  if(!foundto || tcontract == "null"_n)
    return  {"null"_n, {0, tokens.symbol}, "null"_n, ""};  //This pair is not supported. 
  
  asset out = swapSx::get_amount_out( "swap.sx"_n, tokens, to );
  
  return {"swap.sx"_n, out, tcontract, to.to_string()};
}

map<string, vector<symbol_code>> basic::get_all_pairs(extended_symbol sym){
  map<string, vector<symbol_code>> res;

  sx::registry::dfs_table dfs_table( "registry.sx"_n, "registry.sx"_n.value );
  auto dfsrow = dfs_table.get(sym.get_symbol().code().raw(), "DFS doesn't trade this currency");
  for(auto& p: dfsrow.quotes) 
    res["dfs"].push_back(p.first);

  
  sx::registry::defibox_table defi_table( "registry.sx"_n, "registry.sx"_n.value );
  auto defirow = defi_table.get(sym.get_symbol().code().raw(), "Defibox doesn't trade this currency");
  for(auto& p: defirow.quotes) 
    res["defibox"].push_back(p.first);

  vector<symbol_code> swapv;
  bool found=false;
  swapSx::tokens _tokens( "swap.sx"_n, "swap.sx"_n.value );
  for ( const auto token : _tokens ) {
      if(token.sym!=sym.get_symbol()) swapv.push_back(token.sym.code());   
      else found=true;
  }
  if(found) res["swapsx"] = swapv;

  return res;
}
    
   
map<symbol_code, map<asset, string>> basic::get_quotes(map<string, vector<symbol_code>>& pairs, asset tokens)  {
  
  //for common symbols build a map {BOX->{{0.1234 BOX,"defi"},{0.1345 BOX, "dfs"}},...}
  map<symbol_code, map<asset, string>> prices;  
  for(auto& p: pairs){
    auto dex = p.first;
    for(auto sym: p.second){
      auto [ex, out, tcontract, memo] = get_trade_data(dex, tokens, sym);
      if(out.amount > 0) prices[sym][out] = dex;
    }
  }

  return prices;
}

tuple<asset, symbol_code, string, string> basic::get_best_arb_opportunity(asset eos_tokens) {
  
  check(eos_tokens.symbol.code().to_string()=="EOS", "Only EOS is supported as base token at the moment");

  auto pairs = get_all_pairs({{"EOS",4}, "eosio.token"_n}); 
  auto quotes = get_quotes(pairs, eos_tokens);

  //check(false, "Quotes: " + to_string(quotes.size()));
  //calculate best profits for each symbol and find the biggest
  tuple<asset, symbol_code, string, string> best;
  asset best_gain{-100*10000, {"EOS",4}};
  for(auto& p: quotes){
    if(p.second.size()<2) continue;   //pair traded only on one exchange
    auto sym = p.first;
    auto sellit = p.second.rbegin(); //highest return for this symbol (best to sell)
    auto buyit = p.second.begin();   //lowest return (best to buy)
    auto [ex, out, tcontract, memo] = get_trade_data(buyit->second, sellit->first, eos_tokens.symbol.code());
    auto gain = out - eos_tokens;
    if(out.amount > 0 && gain > best_gain){
      best_gain = gain;
      best = {gain, sym, sellit->second, buyit->second};
    }
    print(sym.to_string() + ": " + sellit->second + "("+sellit->first.to_string()+ ")->" 
                + buyit->second + "("+out.to_string() +"@" + buyit->first.to_string()+ ") =" + gain.to_string() + "\n");
  }

  return best;
} 


[[eosio::on_notify("eosio.token::transfer")]]   
void basic::on_transfer(eosio::name& from, eosio::name& to, eosio::asset& sum, string& memo ){

    if(from!="flash.sx"_n) return;      //only handle flash.sx loan notifies

    print("Received "+sum.to_string()+" from "+ from.to_string()+"\n");
  
    basic::tradeplan _tradeplan( get_self(), get_self().value );
    auto [stake, dex_sell, dex_buy, sym, expected_out] = _tradeplan.get();

    check(stake==sum, "Received wrong loan from flash.sx: "+sum.to_string());
  
    auto symret = make_trade(stake, sym, dex_sell);

    auto eosret = make_trade(symret, stake.symbol.code(), dex_buy);
    
    auto profit = eosret-stake;
    check(profit.amount>0, "There was no profit");

    eosio::token::transfer_action transfer("eosio.token"_n, { get_self(), "active"_n });
    transfer.send( get_self(), "flash.sx"_n, stake, "Repaying the loan");
    
    if(profit.amount>0)
      transfer.send( get_self(), "fee.sx"_n, profit, "Profits");
    
    //delete singleton?

    print("All done. Profit: "+profit.to_string()+". Expected: "+expected_out.to_string()+"\n");

}