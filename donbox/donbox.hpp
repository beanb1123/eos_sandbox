#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

const uint64_t MIN_WITHDRAWAL = 1000;

class [[eosio::contract("donbox")]] donbox : public eosio::contract{       
    
    const uint64_t _threshold;
    const map<eosio::symbol, eosio::name> _currencies = {
      {{"EOS", 4}, "eosio.token"_n},
      {{"SYS", 4}, "eosio.token"_n}
    };

    struct [[eosio::table]] bal_record {
        eosio::name username;
        uint32_t donors;
        std::map<eosio::symbol_code, eosio::asset>  funds;
        uint64_t primary_key() const { return username.value;} 
    };

    typedef eosio::multi_index< "balances"_n, bal_record > balances;

    struct [[eosio::table]] hist_record {
      uint64_t id;                        //auto-inc id
      eosio::name sender;                 
      eosio::name receiver;
      eosio::asset sum;
      eosio::time_point timestamp;  
      uint64_t primary_key() const { return id; }
    };

    typedef eosio::multi_index <"history"_n, hist_record> history;

public:
    donbox(eosio::name rec, eosio::name code, datastream<const char*> ds) 
      : contract(rec, code, ds)
      , _threshold(MIN_WITHDRAWAL)
    {};

    // clear database
    [[eosio::action]]
    void deletedata();

    // called when transfer happens to update tables
    [[eosio::on_notify("*::transfer")]]   
    void donation_in(eosio::name& from, eosio::name& to, eosio::asset& sum, string& memo );

    // user withdraws 
    [[eosio::action]]
    void withdraw(eosio::name user);

    using withdraw_action = eosio::action_wrapper<"withdraw"_n, &donbox::withdraw>;
};
   
