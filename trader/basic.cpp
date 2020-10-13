#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

#include <eosio.token.hpp>
#include <defibox.hpp>
#include <uniswap.hpp>
#include <flash.sx.hpp>
#include <swap.sx.hpp>
#include <hamburger.hpp>
#include <pizza.hpp>
#include <registry.sx.hpp>
#include <dfs.hpp>

#include "basic.hpp"

using namespace eosio;
using namespace std;

    
[[eosio::action]]
void basic::mine(asset eos_tokens) {
  
  if (!has_auth("miner.sx"_n)) require_auth(get_self());

  auto arb = get_best_arb_opportunity(eos_tokens);
  
  print( "Best profit: " + arb.exp_profit.to_string() + " EOS/" + arb.symcode.to_string() + " " + arb.dex_sell + "->" + arb.dex_buy );
  
  check( arb.exp_profit.amount > 0, "No profits for "+eos_tokens.to_string()+". Closest: " + arb.exp_profit.to_string() + " with " + arb.symcode.to_string() + " " + arb.dex_sell + "=>" + arb.dex_buy );
  
  basic::arbplan _arbplan( get_self(), get_self().value );
  _arbplan.set(arb, get_self());

  //borrow stake
  flash::borrow_action borrow( "flash.sx"_n, { get_self(), "active"_n });
  borrow.send( get_self(), "eosio.token"_n, eos_tokens, "Loan", "" );
  

}

[[eosio::action]]
void basic::trade(asset tokens, asset minreturn, string exchange){
  
  check( tokens.amount > 0 && minreturn.amount > 0, "Invalid tokens amount" );
  
  auto [dex, out, tcontract, memo] = get_trade_data(exchange, tokens, minreturn.symbol.code());
  
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

basic::tradeparams basic::get_trade_data(string exchange, asset tokens, symbol_code to){
  
  if(exchange == "defibox") 
    return get_defi_trade_data(tokens, to);
  if(exchange == "dfs") 
    return get_dfs_trade_data(tokens, to);
  if(exchange == "swap.sx" || exchange == "vigor.sx" || exchange == "stable.sx") 
    return get_swap_trade_data(name{exchange}, tokens, to);
  if(exchange == "hamburger") 
    return get_hbg_trade_data(tokens, to);
  if(exchange == "pizza") 
    return get_pizza_trade_data(tokens, to);
  
  check(false, exchange + " exchange is not supported");
  return {"null"_n, {0, tokens.symbol}, "null"_n, ""};  //dummy

}

    
basic::tradeparams basic::get_defi_trade_data(asset tokens, symbol_code to){
  
  sx::registry::defibox_table defi_table( "registry.sx"_n, "registry.sx"_n.value );
  
  uint64_t pair_id = 0;
  name tcontract;
  for(const auto& row: defi_table){
    if(row.base.get_symbol() == tokens.symbol){
      check( row.pair_ids.count(to), to.to_string()+" target currency is not supported on Defibox" );
      pair_id = row.pair_ids.at(to);
      tcontract = row.base.get_contract();
      break;
    }
  }

 if(pair_id == 0)
    return  {};  //This pair is not supported. 
  
  // get reserves
  const auto [ reserve_in, reserve_out ] = defibox::get_reserves( pair_id, tokens.symbol );
  const uint8_t fee = defibox::get_fee();

  // calculate out price  
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );
  
  return {"swap.defi"_n, out, tcontract, "swap,0," + to_string((int) pair_id)};
}


    
basic::tradeparams basic::get_dfs_trade_data(asset tokens, symbol_code to){
  
  sx::registry::dfs_table dfs_table( "registry.sx"_n, "registry.sx"_n.value );
  
  uint64_t pair_id = 0;
  name tcontract;
  for(const auto& row: dfs_table){
    if(row.base.get_symbol() == tokens.symbol){
      check( row.pair_ids.count(to), to.to_string()+" target currency is not supported on DFS" );
      pair_id = row.pair_ids.at(to);
      tcontract = row.base.get_contract();
      break;
    }
  }

  if(pair_id == 0)
    return  {};  //This pair is not supported. 
  
  // get reserves
  const auto [ reserve_in, reserve_out ] = dfs::get_reserves( pair_id, tokens.symbol );
  const uint8_t fee = dfs::get_fee();

  // calculate out price  
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"defisswapcnt"_n, out, tcontract, "swap:" + to_string((int) pair_id)+":0"};
}


    
basic::tradeparams basic::get_hbg_trade_data(asset tokens, symbol_code to){
  
  sx::registry::hamburger_table hbg_table( "registry.sx"_n, "registry.sx"_n.value );
  
  uint64_t pair_id = 0;
  name tcontract;
  for(const auto& row: hbg_table){
    if(row.base.get_symbol() == tokens.symbol){
      check( row.pair_ids.count(to), to.to_string()+" target currency is not supported on Hamburger" );
      pair_id = row.pair_ids.at(to);
      tcontract = row.base.get_contract();
      break;
    }
  }

  if(pair_id == 0)
    return  {};  //This pair is not supported. 
  
  // get reserves
  const auto [ reserve_in, reserve_out ] = hamburger::get_reserves( pair_id, tokens.symbol );
  const uint8_t fee = hamburger::get_fee();

  // calculate out price  
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"hamburgerswp"_n, out, tcontract, "swap:" + to_string((int) pair_id)};
}
  
basic::tradeparams basic::get_pizza_trade_data(asset tokens, symbol_code to){
  
  sx::registry::pizza_table pz_table( "registry.sx"_n, "registry.sx"_n.value );
  
  uint64_t pair_id = 0;
  name tcontract;
  for(const auto& row: pz_table){
    if(row.base.get_symbol() == tokens.symbol){
      check( row.pair_ids.count(to), to.to_string()+" target currency is not supported on Pizza" );
      pair_id = row.pair_ids.at(to);
      tcontract = row.base.get_contract();
      break;
    }
  }

  if(pair_id == 0)
    return  {};  //This pair is not supported. 
  
  // get reserves
  const auto [ reserve_in, reserve_out ] = pizza::get_reserves( pair_id, tokens.symbol );
  const uint8_t fee = pizza::get_fee();

  // calculate out price  
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"pzaswapcntct"_n, out, tcontract, name{pair_id}.to_string()+"-swap-0"};
}


basic::tradeparams basic::get_swap_trade_data(name dex_contract, asset tokens, symbol_code to){
  
  swapSx::tokens _tokens( dex_contract, dex_contract.value );
  name tcontract = "null"_n;
  bool foundto=false;
  for ( const auto token : _tokens ) {
    if(token.sym.code()==tokens.symbol.code())    
      tcontract = token.contract;
    if(token.sym.code()==to)
      foundto = true;
  }

  if(!foundto || tcontract == "null"_n)
    return {};  //This pair is not supported. 
  
  asset out = swapSx::get_amount_out( dex_contract, tokens, to );
  
  return {dex_contract, out, tcontract, to.to_string()};
}

map<string, vector<symbol_code>> basic::get_all_pairs(extended_symbol sym){
  map<string, vector<symbol_code>> res;

  sx::registry::dfs_table dfs_table( "registry.sx"_n, "registry.sx"_n.value );
  auto dfsrow = dfs_table.get(sym.get_symbol().code().raw(), "DFS doesn't trade this currency");
  for(auto& p: dfsrow.pair_ids) 
    res["dfs"].push_back(p.first);

  
  sx::registry::defibox_table defi_table( "registry.sx"_n, "registry.sx"_n.value );
  auto defirow = defi_table.get(sym.get_symbol().code().raw(), "Defibox doesn't trade this currency");
  for(auto& p: defirow.pair_ids) 
    res["defibox"].push_back(p.first);

  sx::registry::hamburger_table hbg_table( "registry.sx"_n, "registry.sx"_n.value );
  auto hbgrow = hbg_table.get(sym.get_symbol().code().raw(), "Hamburger doesn't trade this currency");
  for(auto& p: hbgrow.pair_ids) 
    res["hamburger"].push_back(p.first);

  sx::registry::pizza_table pz_table( "registry.sx"_n, "registry.sx"_n.value );
  auto pzrow = pz_table.get(sym.get_symbol().code().raw(), "Pizza doesn't trade this currency");
  for(auto& p: pzrow.pair_ids) 
    res["pizza"].push_back(p.first);

  sx::registry::swap_table swap_table( "registry.sx"_n, "registry.sx"_n.value );
  
  //handle all contracts from Swap
  for(auto contract: {"swap.sx"_n, "stable.sx"_n, "vigor.sx"_n}) {
    vector<symbol_code> swapv;
    bool found=false;
    
    auto row = swap_table.get(contract.value, "Can't find swap.sx record in registry.sx");
    for(auto& s: row.tokens){
      if(s!=sym.get_symbol().code()) swapv.push_back(s);
      else found=true;
    }
    if(found) res[contract.to_string()] = swapv;
  }

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

basic::arbparams basic::get_best_arb_opportunity(asset eos_tokens) {
  
  check(eos_tokens.symbol.code().to_string()=="EOS", "Only EOS is supported as base token at the moment");

  auto pairs = get_all_pairs({{"EOS",4}, "eosio.token"_n}); 
  auto quotes = get_quotes(pairs, eos_tokens);

  //calculate best profits for each symbol and find the best option
  arbparams best;
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
      best = {eos_tokens, sellit->second, buyit->second, sym, gain};
    }
    print(sym.to_string() +"("+to_string(p.second.size())+"): " + sellit->second + "("+sellit->first.to_string()+ ")->" 
                + buyit->second + "("+buyit->first.to_string()+ ")@"+out.to_string() +" =" + gain.to_string() + "\n");
  }

  return best;
} 


[[eosio::on_notify("eosio.token::transfer")]]   
void basic::on_transfer(eosio::name& from, eosio::name& to, eosio::asset& sum, string& memo ){

    if(from!="flash.sx"_n) return;      //only handle flash.sx loan notifies

    basic::arbplan _arbplan( get_self(), get_self().value );
    basic::arbparams arb = _arbplan.get();

    check(arb.stake==sum, "Received wrong loan from flash.sx: "+sum.to_string());
  
    auto symret = make_trade(arb.stake, arb.symcode, arb.dex_sell);

    auto eosret = make_trade(symret, arb.stake.symbol.code(), arb.dex_buy);
    
    auto profit = eosret-arb.stake;
    check(profit.amount>0, "There was no profit");

    eosio::token::transfer_action transfer("eosio.token"_n, { get_self(), "active"_n });
    transfer.send( get_self(), "flash.sx"_n, arb.stake, "Repaying the loan");
    transfer.send( get_self(), "fee.sx"_n, profit, arb.stake.symbol.code().to_string()+"/"+arb.symcode.to_string()+" "+arb.dex_sell+"->"+arb.dex_buy);
    
    //delete singleton?

    print("All done. Profit: "+profit.to_string()+". Expected: "+arb.exp_profit.to_string()+"\n");

}