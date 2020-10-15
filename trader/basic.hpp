#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract]] basic : public contract {

public:
    basic(name rec, name code, datastream<const char*> ds)
      : contract(rec, code, ds)
    {};

    //find arbitrage opportunity for {quantity} asset and execute it
    [[eosio::action]]
    void mine(name executor, extended_asset ext_quantity);

    //trade {tokens} on {exchange} with expected return of >= {minreturn}
    //mainly for testing new exchanges
    [[eosio::action]]
    void trade(asset quantity, asset minreturn, string exchange);

    //log asset
    [[eosio::action]]
    void log( asset& out ){
        require_auth( get_self() );
    };
    using log_action = action_wrapper<"log"_n, &basic::log>;

    [[eosio::on_notify("eosio.token::transfer")]]
    void on_transfer(name& from, name& to, asset& sum, string& memo );

    [[eosio::action]]
    void flush(name contract, symbol_code symcode, string memo);
    using flush_action = action_wrapper<"flush"_n, &basic::flush>;

private:
    //arbitration parameters to save into singleton
    struct [[eosio::table("arbplan")]] arbparams {
        extended_asset  stake;          //our stake we borrow from flash.sx
        string          dex_sell;       //exchange to sell stake to
        string          dex_buy;        //exchange to buy from
        symbol          symbol;        //symbol code to arbitrage via
        asset           exp_profit;     //expected profit from arbitrage
    };
    typedef eosio::singleton< "arbplan"_n, arbparams > arbplan;

    //trade parameters for trade
    struct tradeparams {
        name            dex = "null"_n;         //exchange name
        asset           out = {0, {"EOS", 4}};  //calculated return
        name            tcontract = "null"_n;   //token contract for trade
        string          memo = "";              //memo to make trade happen
    };

    //get parameters for trade of {tokens} on {exchange}
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tradeparams get_trade_data(string exchange, asset tokens, symbol to);

    //get parameters for trade of {tokens} on defibox
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tradeparams get_defi_trade_data(asset tokens, symbol to);

    //get parameters for trade of {tokens} on dfs
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tradeparams get_dfs_trade_data(asset tokens, symbol to);

    //get parameters for trade of {tokens} on hamburger dex
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tradeparams get_hbg_trade_data(asset tokens, symbol to);

    //get parameters for trade of {tokens} on pizza dex
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tradeparams get_pizza_trade_data(asset tokens, symbol to);

    //get parameters for trade of {tokens} on sapex dex
    //out: {exchange name, calculated return, token contract name, memo needed for trade}
    tradeparams get_sapex_trade_data(asset tokens, symbol to);

    //get parameters for trade of {tokens} on swap.sx exchange
    //out: { calculated return, token contract name, memo needed for trade}
    tradeparams get_swapsx_trade_data( asset tokens, symbol to);
    tradeparams get_stablesx_trade_data( asset tokens, symbol to);
    tradeparams get_vigorsx_trade_data( asset tokens, symbol to);

    //get vector of maps of all pairs we can trade {sym} for based on registry.sx tables
    //out: i.e. {defi->{BOX,IQ,BTC},dfs->{PIZZA,BTC,ETH}}
    map<string, vector<extended_symbol>> get_all_pairs(extended_symbol sym);

    //based on trade pairs {pairs} and base assets {tokens} build map of quotes
    //out: {symbol -> {out_tokens -> dex},...}
    //i.e. from: {defi->{BOX,IQ,BTC},dfs->{PIZZA,BTC,ETH}}
    //to: {{dfs->{BOX,BTC,ETH},defi->{BTN,IQ,BTC}}} => {BOX->{{0.1234 EOS->dfs},{1.2345 EOS->defi}},{BTC->{{0.123 EOS->dfs},..}}}
    map<symbol, map<asset, string>>  get_quotes(map<string, vector<extended_symbol>>& pairs, asset ext_tokens);

    //find best arbitrage opportunity based on {eos_tokens} bet
    //out: {expected profit, symbol, dex to sell, dex to buy}
    arbparams get_best_arb_opportunity(extended_asset ext_tokens);

    //trade {tokens} to {sym} currency on {exchange}
    //out: expected return
    asset make_trade(asset tokens, symbol sym, string exchange);

};