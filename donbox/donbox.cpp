#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>
#include <math.h>
#include "include/eosio.token.hpp"

using namespace eosio;
using namespace std;

const uint64_t MIN_WITHDRAWAL = 1000;


//holds donation balance and allows donation withdrawal when there is more than threshold SYS
//donation can be made by sending tokens to donbox account with donation recepient in the memo

class [[eosio::contract("donbox")]] donbox : public eosio::contract{       
    
    const uint64_t _threshold;
    const map<eosio::symbol, eosio::name> _currencies = {
      {{"EOS", 4}, "eosio.token"_n},
      {{"SYS", 4}, "eosio.token"_n}
    };

    struct [[eosio::table]] record {
        eosio::name username;
        uint32_t donors;
        std::map<eosio::symbol_code, eosio::asset>  funds;
        uint64_t primary_key() const { return username.value;} 
    };

    typedef eosio::multi_index< "balances"_n, record > balances;

public:
    donbox(eosio::name rec, eosio::name code, datastream<const char*> ds) 
      : contract(rec, code, ds)
      , _threshold(MIN_WITHDRAWAL)
    {}

    [[eosio::on_notify("*::transfer")]]   // called when transfer happens
    void donation_in(eosio::name& from, eosio::name& to, eosio::asset& sum, string& memo ){
      
      if(from == get_self()) return;  //if it's a withdrawal

      const auto contrit = _currencies.find(sum.symbol);
      check(contrit != _currencies.end(), "This currency is not accepted");
      check(get_first_receiver()==contrit->second, "Wrong token contract");

      //memo = account name donation is for
      check(memo.length()<13, "Specify donation receiver in the memo and try again");

      name receiver(memo);
      check(is_account(receiver), "Memo should contain the donation receiver. No such user: " + memo);

      balances balance(get_self(), get_self().value);  
      auto it = balance.find(receiver.value);

      if (it != balance.end()){
        
        balance.modify(it, get_self(), [&](auto &row) {
          if(row.funds.count(sum.symbol.code()))
            row.funds[sum.symbol.code()] += sum;
          else
            row.funds[sum.symbol.code()] = sum;
          row.donors++;
        });
      }
      else {
        balance.emplace(get_self(), [&](auto &row) {
          row.username = receiver;
          row.funds[sum.symbol.code()] = sum;
          row.donors = 1;
        });
      }

    }
    [[eosio::action]]
    void deletedata(){

      require_auth(_self);
      balances tab(_self, get_self().value);
      auto itr = tab.begin();
      while(itr != tab.end()){
          itr = tab.erase(itr);
      }
    }
    [[eosio::action]]
    void withdraw(eosio::name user){

      require_auth(user);

      balances balance(get_self(), get_self().value);    
    
      const auto& rec = balance.get(user.value, "You don't have any donations, sorry");

      vector<asset> to_transfer;
      for(auto& p: rec.funds){
        if(p.second.amount/pow(10, p.second.symbol.precision()) >= _threshold)
          to_transfer.push_back(p.second);
      }

      check(to_transfer.size(), "Nothing to witdraw. Must have at least " + to_string(_threshold) + " of any tokens");
      
      eosio::token::transfer_action transfer("eosio.token"_n, { get_self(), "active"_n });
      for(auto& tokens: to_transfer){
      
        print("Transferring ", tokens.to_string(), " to ", user, "\n");
        transfer.send( get_self(), user, tokens, tokens.to_string()+" from "+to_string(rec.donors)+" donor(s)");
        
        /*  //another way to action
          action{
            permission_level{get_self(), "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(get_self(), user, tokens, tokens.to_string()+" from "+to_string(rec.donors)+" donor(s)")
          }.send();
        */
        balance.modify(rec, get_self(), [&](auto &row) {
          row.funds.erase(tokens.symbol.code());
        });

      }

    }
};