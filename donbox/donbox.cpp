#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>
//#include <eosio.token/eosio.token.hpp>

using namespace eosio;
using namespace std;


//holds donation balance and allows donation withdrawal when there is more than threshold SYS
//donation can be made by sending tokens to donbox account with donation recepient in the memo

class [[eosio::contract("donbox")]] donbox : public eosio::contract{       
    
    const uint64_t _threshold;
    const eosio::symbol _symbol;

    struct [[eosio::table]] balance {
        eosio::name username;
        uint32_t donors;
        eosio::asset funds;
        //uint64_t primary_key() const { return username.value;} 
        uint64_t primary_key() const { return funds.symbol.code().raw(); }
    };

    typedef eosio::multi_index< "balances"_n, balance > balances;

public:
    donbox(eosio::name receiver, eosio::name code, datastream<const char*> ds) 
      : contract(receiver, code, ds)
      , _symbol("SYS", 4)
      , _threshold(2000)
    {}

    [[eosio::on_notify("eosio.token::transfer")]]   // called when transfer happens
    void donation_in(eosio::name& from, eosio::name& to, eosio::asset& sum, string& memo ){
      
      if(from == get_self()) return;  //if it's a withdrawal

      check(sum.symbol == _symbol, "Too bad. We can only hold " + _symbol.code().to_string());
      
      //memo = account name donation is for
      check(memo.length()<13, "Specify donation receiver in the memo and try again");

      name receiver(memo);
      check(is_account(receiver), "Memo should contain the donation receiver. No such user: " + memo);

      auto sym_code_raw = _symbol.code().raw();
      balances balance(get_self(), receiver.value);  
      auto it = balance.find(sym_code_raw);

      if (it != balance.end()){
        balance.modify(it, get_self(), [&](auto &row) {
          row.funds += sum;
          row.donors++;
        });
      }
      else {
        balance.emplace(get_self(), [&](auto &row) {
          row.username = receiver;
          row.funds = sum;
          row.donors = 1;
        });
      }

    }

    [[eosio::action]]
    void withdraw(eosio::name user){

      require_auth(user);

      balances balance(get_self(), user.value);    
    
      auto it = balance.find(_symbol.code().raw()); 
      check(it != balance.end(), "You don't have any donations, sorry");
      check(it->funds.amount/10000 >= _threshold, "Minimum withdrawal: " + to_string(_threshold) + ". You only have " + it->funds.to_string());
      //check(false, "Available: "+to_string(it->funds.amount)+", Threshold: "+ to_string(_threshold));
      
      print("Transferring ", it->funds.to_string(), " to ", user);
    
      action{
        permission_level{get_self(), "active"_n},
        "eosio.token"_n,
        "transfer"_n,
        std::make_tuple(get_self(), user, it->funds, it->funds.to_string()+" from "+to_string(it->donors)+" donor(s)")
      }.send();
  
 /*
      using transfer_action = eosio::action_wrapper<"transfer"_n, &transfer>;
      transfer_action transfer( get_contract( get_self(), quantity.symbol.code() ), { get_self(), "active"_n });
      transfer.send( get_self(), to, quantity, memo );
  */    
      balance.erase(it);
    }
};