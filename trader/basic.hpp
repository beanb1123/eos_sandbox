#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract]] basic : public contract {       
    
public:
    basic(name rec, name code, datastream<const char*> ds) 
      : contract(rec, code, ds)
    {};
    
    //find arbitrage opportunity and execute it
    [[eosio::action]]
    void mine(asset eos_tokens);

    //log asset
    [[eosio::action]]
    void log( asset& out ){
        require_auth( get_self() );
    }
    using log_action = action_wrapper<"log"_n, &basic::log>;

    [[eosio::on_notify("eosio.token::transfer")]]   
    void on_transfer(name& from, name& to, asset& sum, string& memo );


private:
    struct [[eosio::table("tradeplan")]] tradeparams {
        asset           stake;
        string          dex_sell;
        string          dex_buy;
        symbol_code     symbol;
        asset           expected_out;
    };
    typedef eosio::singleton< "tradeplan"_n, tradeparams > tradeplan;

    //get parameters for trade of {tokens} on {exchange}
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tuple<name, asset, name, string> 
    get_trade_data(string exchange, asset tokens, symbol_code to);

    //get parameters for trade of {tokens} on defibox
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tuple<name, asset, name, string> 
    get_defi_trade_data(asset tokens, symbol_code to);
    
    //get parameters for trade of {tokens} on dfs
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tuple<name, asset, name, string> 
    get_dfs_trade_data(asset tokens, symbol_code to);

    //get parameters for trade of {tokens} on {dex_contract} swap exchange
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tuple<name, asset, name, string> 
    get_swap_trade_data(name dex_contract, asset tokens, symbol_code to);
    
    //get vector of maps of all pairs we can trade {sym} for based on registry.sx tables
    //out: i.e. {defi->{BOX,IQ,BTC},dfs->{PIZZA,BTC,ETH}}
    map<string, vector<symbol_code>>
    get_all_pairs(extended_symbol sym);
    
    //based on trade pairs {pairs} and base assets {tokens} build map of quotes 
    //out: {symbol -> {out_tokens -> dex},...} 
    //i.e. from: {defi->{BOX,IQ,BTC},dfs->{PIZZA,BTC,ETH}}
    //to: {{dfs->{BOX,BTC,ETH},defi->{BTN,IQ,BTC}}} => {BOX->{{0.1234 EOS->dfs},{1.2345 EOS->defi}},{BTC->{{0.123 EOS->dfs},..}}}
    map<symbol_code, map<asset, string>> 
    get_quotes(map<string, vector<symbol_code>>& pairs, asset tokens);
    
    //find best arbitrage opportunity based on {eos_tokens} bet
    //out: {possible profit, symbol, dex to sell, dex to buy}
    tuple<asset, symbol_code, string, string> 
    get_best_arb_opportunity(asset eos_tokens);

    //trade {tokens} to {sym} currency on {exchange}
    //out: expected return
    asset make_trade(asset tokens, symbol_code sym, string exchange);
    
};