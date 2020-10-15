#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

#include <eosio.token.hpp>
#include <registry.sx.hpp>
#include <defibox.hpp>
#include <dfs.hpp>
#include <uniswap.hpp>
#include <flash.sx.hpp>
#include <swap.sx.hpp>
#include <hamburger.hpp>
#include <pizza.hpp>
#include <sapex.hpp>


#include "basic.hpp"

using namespace eosio;
using namespace std;


[[eosio::action]]
void basic::mine(name executor, extended_asset ext_quantity) {

  if (!has_auth("miner.sx"_n)) require_auth(get_self());


  auto arb = get_best_arb_opportunity(ext_quantity);
  auto quantity = ext_quantity.quantity;
  auto contract = ext_quantity.contract;
/*
  print( "Best profit: " + arb.exp_profit.to_string() +
    " "+quantity.symbol.code().to_string()+"/" + arb.symbol.code().to_string() +
    " " + arb.dex_sell + "->" + arb.dex_buy );
*/
  check( arb.exp_profit.amount > 0,
    "No profits for "+quantity.to_string()+
    ". Closest: " + arb.exp_profit.to_string() + " with " + arb.symbol.code().to_string() +
    " " + arb.dex_sell + "=>" + arb.dex_buy );

  basic::arbplan _arbplan( get_self(), get_self().value );
  _arbplan.set(arb, get_self());

  //borrow stake
  flash::borrow_action borrow( "flash.sx"_n, { get_self(), "active"_n });
  borrow.send( get_self(), contract, quantity, "Loan", "" );

}

[[eosio::action]]
void basic::trade(asset tokens, asset minreturn, string exchange){

  check( tokens.amount > 0 && minreturn.amount > 0, "Invalid tokens amount" );
  auto [dex, out, tcontract, memo] = get_trade_data(exchange, tokens, minreturn.symbol);

  check(minreturn <= out || minreturn.amount==0, "Return is not enough");

  // log out price
  log_action log( get_self(), { get_self(), "active"_n });
  log.send( out );

  // make a swap
  token::transfer_action transfer( tcontract, { get_self(), "active"_n });
  transfer.send( get_self(), dex, tokens, memo);

}

asset basic::make_trade(asset tokens, symbol sym, string exchange){

  check( tokens.amount > 0, "Invalid tokens amount" );

  auto [dex, out, tcontract, memo] = get_trade_data(exchange, tokens, sym);

  check(out.amount > 0, "Trade pair not supported");
  print("Sending "+tokens.to_string()+" to "+dex.to_string()+" to buy "+sym.code().to_string()+" with memo "+memo+"\n");

  // make a trade
  token::transfer_action transfer( tcontract, { get_self(), "active"_n });
  transfer.send( get_self(), dex, tokens, memo);

  return out;

}

basic::tradeparams basic::get_trade_data(string exchange, asset tokens, symbol to){

  if(exchange == "defibox")
    return get_defi_trade_data(tokens, to);
  if(exchange == "dfs")
    return get_dfs_trade_data(tokens, to);
  if(exchange == "swap.sx")
    return get_swapsx_trade_data(tokens, to);
  if(exchange == "stable.sx")
    return get_stablesx_trade_data(tokens, to);
  if(exchange == "vigor.sx")
    return get_vigorsx_trade_data(tokens, to);
  if(exchange == "hamburger")
    return get_hbg_trade_data(tokens, to);
  if(exchange == "pizza")
    return get_pizza_trade_data(tokens, to);
  if(exchange == "sapex")
    return get_sapex_trade_data(tokens, to);

  check(false, exchange + " exchange is not supported");
  return {"null"_n, {0, tokens.symbol}, "null"_n, ""};  //dummy

}


basic::tradeparams basic::get_defi_trade_data(asset tokens, symbol to){

  sx::registry::swap_defi_table defi_table( "registry.sx"_n, "registry.sx"_n.value );

  auto rowit = defi_table.find(tokens.symbol.code().raw());
  if(rowit==defi_table.end()) return {};
  name tcontract = rowit->base.get_contract();
  string pair_id;
  for(auto& p: rowit->quotes){
    if(p.first.get_symbol()==to){
      pair_id = p.second; break;
    }
  }
  if(pair_id=="") return {};

  // get reserves
  const auto [ reserve_in, reserve_out ] = defibox::get_reserves( stoi(pair_id), tokens.symbol );
  const uint8_t fee = defibox::get_fee();

  // calculate out price
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"swap.defi"_n, out, tcontract, "swap,0," + pair_id};
}



basic::tradeparams basic::get_dfs_trade_data(asset tokens, symbol to){

  sx::registry::defisswapcnt_table dfs_table( "registry.sx"_n, "registry.sx"_n.value );

  auto rowit = dfs_table.find(tokens.symbol.code().raw());
  if(rowit==dfs_table.end()) return {};
  name tcontract = rowit->base.get_contract();
  string pair_id;
  for(auto& p: rowit->quotes){
    if(p.first.get_symbol()==to){
      pair_id = p.second; break;
    }
  }
  if(pair_id=="") return {};

  // get reserves
  const auto [ reserve_in, reserve_out ] = dfs::get_reserves( stoi(pair_id), tokens.symbol );
  const uint8_t fee = dfs::get_fee();

  // calculate out price
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"defisswapcnt"_n, out, tcontract, "swap:" + pair_id+":0"};
}



basic::tradeparams basic::get_hbg_trade_data(asset tokens, symbol to){

  sx::registry::hamburgerswp_table hbg_table( "registry.sx"_n, "registry.sx"_n.value );

  auto rowit = hbg_table.find(tokens.symbol.code().raw());
  if(rowit==hbg_table.end()) return {};
  name tcontract = rowit->base.get_contract();
  string pair_id;
  for(auto& p: rowit->quotes){
    if(p.first.get_symbol()==to){
      pair_id = p.second; break;
    }
  }
  if(pair_id=="") return {};


  // get reserves
  const auto [ reserve_in, reserve_out ] = hamburger::get_reserves( stoi(pair_id), tokens.symbol );
  const uint8_t fee = hamburger::get_fee();

  // calculate out price
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"hamburgerswp"_n, out, tcontract, "swap:" + pair_id};
}

basic::tradeparams basic::get_pizza_trade_data(asset tokens, symbol to){

  sx::registry::pzaswapcntct_table pz_table( "registry.sx"_n, "registry.sx"_n.value );

  auto rowit = pz_table.find(tokens.symbol.code().raw());
  if(rowit==pz_table.end()) return {};
  name tcontract = rowit->base.get_contract();
  string pair_id;
  for(auto& p: rowit->quotes){
    if(p.first.get_symbol()==to){
      pair_id = p.second; break;
    }
  }
  if(pair_id=="") return {};

  // get reserves
  const auto [ reserve_in, reserve_out ] = pizza::get_reserves( name{pair_id}.value, tokens.symbol );
  const uint8_t fee = pizza::get_fee();

  // calculate out price
  const asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"pzaswapcntct"_n, out, tcontract, name{pair_id}.to_string()+"-swap-0"};
}


basic::tradeparams basic::get_swapsx_trade_data(asset tokens, symbol to){

  sx::registry::swap_sx_table swap_sx_table( "registry.sx"_n, "registry.sx"_n.value );

  auto rowit = swap_sx_table.find(tokens.symbol.code().raw());
  if(rowit==swap_sx_table.end()) return {};
  name tcontract = "null"_n;
  for(auto& p: rowit->quotes){
    if(p.first.get_symbol()==to){
      tcontract = rowit->base.get_contract(); break;
    }
  }
  if(tcontract=="null"_n) return {};

  asset out = swapSx::get_amount_out( "swap.sx"_n, tokens, to.code() );

  return {"swap.sx"_n, out, tcontract, to.code().to_string()};
}

basic::tradeparams basic::get_stablesx_trade_data(asset tokens, symbol to){

  sx::registry::stable_sx_table stable_sx_table( "registry.sx"_n, "registry.sx"_n.value );

  auto rowit = stable_sx_table.find(tokens.symbol.code().raw());
  if(rowit==stable_sx_table.end()) return {};
  name tcontract = "null"_n;
  for(auto& p: rowit->quotes){
    if(p.first.get_symbol()==to){
      tcontract = rowit->base.get_contract(); break;
    }
  }
  if(tcontract=="null"_n) return {};

  asset out = swapSx::get_amount_out( "stable.sx"_n, tokens, to.code() );

  return {"stable.sx"_n, out, tcontract, to.code().to_string()};
}

basic::tradeparams basic::get_vigorsx_trade_data(asset tokens, symbol to){

  sx::registry::vigor_sx_table vigor_sx_table( "registry.sx"_n, "registry.sx"_n.value );

  auto rowit = vigor_sx_table.find(tokens.symbol.code().raw());
  if(rowit==vigor_sx_table.end()) return {};
  name tcontract="null"_n;
  for(auto& p: rowit->quotes){
    if(p.first.get_symbol()==to){
      tcontract = rowit->base.get_contract(); break;
    }
  }
  if(tcontract=="null"_n) return {};

  asset out = swapSx::get_amount_out( "vigor.sx"_n, tokens, to.code() );

  return {"vigor.sx"_n, out, tcontract, to.code().to_string()};
}

basic::tradeparams basic::get_sapex_trade_data(asset tokens, symbol to){

  sx::registry::sapexamm_eo_table sapexamm_eo_table( "registry.sx"_n, "registry.sx"_n.value );

  auto rowit = sapexamm_eo_table.find(tokens.symbol.code().raw());
  if(rowit==sapexamm_eo_table.end()) return {};
  name tcontract="null"_n;
  for(auto& p: rowit->quotes){
    if(p.first.get_symbol()==to){
      tcontract = rowit->base.get_contract(); break;
    }
  }
  if(tcontract=="null"_n) return {};

  // get reserves
  const auto [ reserve_in, reserve_out ] = sapex::get_reserves(tokens.symbol, to);
  const uint8_t fee = sapex::get_fee(tokens.symbol, to);

  asset out = uniswap::get_amount_out( tokens, reserve_in, reserve_out, fee );

  return {"sapexamm.eo"_n, out, tcontract, to.code().to_string()};
}


map<string, vector<extended_symbol>> basic::get_all_pairs(extended_symbol sym){
  map<string, vector<extended_symbol>> res;

  sx::registry::swap_defi_table defi_table( "registry.sx"_n, "registry.sx"_n.value );
  sx::registry::defisswapcnt_table dfs_table( "registry.sx"_n, "registry.sx"_n.value );
  auto dfsrowit = dfs_table.find(sym.get_symbol().code().raw());
  if(dfsrowit!=dfs_table.end())
    for(auto& p: dfsrowit->quotes)
      res["dfs"].push_back(p.first);

  sx::registry::hamburgerswp_table hbg_table( "registry.sx"_n, "registry.sx"_n.value );
  auto hbgrowit = hbg_table.find(sym.get_symbol().code().raw());
  if(hbgrowit!=hbg_table.end())
    for(auto& p: hbgrowit->quotes)
      res["hamburger"].push_back(p.first);

  sx::registry::pzaswapcntct_table pz_table( "registry.sx"_n, "registry.sx"_n.value );
  auto pzrowit = pz_table.find(sym.get_symbol().code().raw());
  if(pzrowit!=pz_table.end())
    for(auto& p: pzrowit->quotes)
      res["pizza"].push_back(p.first);

  sx::registry::swap_sx_table swap_sx_table( "registry.sx"_n, "registry.sx"_n.value );
  auto swapsxrowit = swap_sx_table.find(sym.get_symbol().code().raw());
  if(swapsxrowit!=swap_sx_table.end())
    for(auto& p: swapsxrowit->quotes)
      res["swap.sx"].push_back(p.first);

  sx::registry::vigor_sx_table vigor_sx_table( "registry.sx"_n, "registry.sx"_n.value );
  auto vigorsxrowit = vigor_sx_table.find(sym.get_symbol().code().raw());
  if(vigorsxrowit!=vigor_sx_table.end())
    for(auto& p: vigorsxrowit->quotes)
      res["vigor.sx"].push_back(p.first);

  sx::registry::stable_sx_table stable_sx_table( "registry.sx"_n, "registry.sx"_n.value );
  auto stablesxrowit = stable_sx_table.find(sym.get_symbol().code().raw());
  if(stablesxrowit!=stable_sx_table.end())
    for(auto& p: stablesxrowit->quotes)
      res["stable.sx"].push_back(p.first);

  sx::registry::sapexamm_eo_table sapex_table( "registry.sx"_n, "registry.sx"_n.value );
  auto sapexrowit = sapex_table.find(sym.get_symbol().code().raw());
  if(sapexrowit!=sapex_table.end())
    for(auto& p: sapexrowit->quotes)
      res["sapex"].push_back(p.first);

  return res;
}


map<symbol, map<asset, string>> basic::get_quotes(map<string, vector<extended_symbol>>& pairs, asset tokens)  {

  //for common symbols build a map {BOX->{{0.1234 BOX,"defi"},{0.1345 BOX, "dfs"}},...}
  map<symbol, map<asset, string>> prices;
  for(auto& p: pairs){
    auto dex = p.first;
    for(auto ext_sym: p.second){
      auto [ex, out, tcontract, memo] = get_trade_data(dex, tokens, ext_sym.get_symbol());
      if(out.amount > 0) prices[ext_sym.get_symbol()][out] = dex;
    }
  }
  //check(false, "Quotes: "+to_string(prices.size()));

  return prices;
}

basic::arbparams basic::get_best_arb_opportunity(extended_asset ext_tokens) {
  auto ext_sym = ext_tokens.get_extended_symbol();
  auto tokens = ext_tokens.quantity;


  auto pairs = get_all_pairs(ext_sym);
  auto quotes = get_quotes(pairs, tokens);

  //check(false, "Quotes: "+to_string(quotes.size())+" Pairs: "+to_string(pairs.size()));
  //calculate best profits for each symbol and find the best option
  arbparams best;
  asset best_gain{-100*10000, ext_sym.get_symbol()};
  for(auto& p: quotes){
    if(p.second.size()<2) continue;   //pair traded only on one exchange
    auto sym = p.first;
    auto sellit = p.second.rbegin(); //highest return for this symbol (best to sell)
    auto buyit = p.second.begin();   //lowest return (best to buy)
    auto [ex, out, tcontract, memo] = get_trade_data(buyit->second, sellit->first, tokens.symbol);
    auto gain = out - tokens;
    if(out.amount > 0 && gain > best_gain){
      best_gain = gain;
      best = {ext_tokens, sellit->second, buyit->second, sym, gain};
    }
    print(sym.code().to_string() +"("+to_string(p.second.size())+"): " + sellit->second + "("+sellit->first.to_string()+ ")->"
                + buyit->second + "("+buyit->first.to_string()+ ")@"+out.to_string() +" =" + gain.to_string() + "\n");
  }

  return best;
}


[[eosio::on_notify("*::transfer")]]
void basic::on_transfer(eosio::name& from, eosio::name& to, eosio::asset& sum, string& memo ){

    if(from!="flash.sx"_n) return;      //only handle flash.sx loan notifies

    basic::arbplan _arbplan( get_self(), get_self().value );
    basic::arbparams arb = _arbplan.get();

    check(arb.stake.quantity==sum, "Received wrong loan from flash.sx: "+sum.to_string());

    auto symret = make_trade(arb.stake.quantity, arb.symbol, arb.dex_sell);

    auto ret = make_trade(symret, arb.stake.quantity.symbol, arb.dex_buy);

    check(ret>=arb.stake.quantity, "No profits");

    eosio::token::transfer_action transfer(arb.stake.contract, { get_self(), "active"_n });
    transfer.send( get_self(), "flash.sx"_n, arb.stake.quantity, "Repaying the loan");

    //auto profit = ret-arb.stake.quantity;
    //transfer.send( get_self(), "fee.sx"_n, profit, arb.stake.quantity.symbol.code().to_string()+"/"+arb.symcode.to_string()+" "+arb.dex_sell+"->"+arb.dex_buy);

    //transfer all balance of the base currency to fee.sx
    flush_action flush( get_self(), { get_self(), "active"_n });
    flush.send( arb.stake.contract, arb.stake.quantity.symbol.code(), arb.stake.quantity.symbol.code().to_string()+"/"+arb.symbol.code().to_string()+" "+arb.dex_sell+"->"+arb.dex_buy );

    _arbplan.remove();
}

[[eosio::action]]
void basic::flush(name contract, symbol_code symcode, string memo){

    if (!has_auth("miner.sx"_n)) require_auth(get_self());

    auto balance = eosio::token::get_balance(contract, get_self(), symcode);
    eosio::token::transfer_action transfer(contract, { get_self(), "active"_n });
    transfer.send( get_self(), "fee.sx"_n, balance, memo);
}