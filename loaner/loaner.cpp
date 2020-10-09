#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

#include <eosio.token.hpp>
#include <flash.sx.hpp>

#include "../donbox/donbox.hpp"

class [[eosio::contract("loaner")]] loaner : public eosio::contract {       
    
public:
    loaner(eosio::name rec, eosio::name code, datastream<const char*> ds) 
      : contract(rec, code, ds)
    {};

    
    //borrow from flash.sx for yourself and wait for transfer - see on_transfer()
    [[eosio::action]]
    void loan(eosio::asset tokens){

        check(tokens.amount>0, "You should borrow > 0");
        
        flash::borrow_action borrow( "flash.sx"_n, { get_self(), "active"_n });
        borrow.send( get_self(), "eosio.token"_n, tokens, "memo", "" );
    }

    //borrow from flash.sx paid to "to" account and wait for transfer callback notify - see on_callback()
    [[eosio::action]]
    void loancallback(eosio::name to, eosio::asset tokens){
        
        check(tokens.amount>0, "You should borrow > 0");
        
        flash::borrow_action borrow( "flash.sx"_n, { get_self(), "active"_n });
        borrow.send(to, "eosio.token"_n, tokens, get_self().to_string(), get_self() );
    }
    
    // called when transfer happens
    [[eosio::on_notify("eosio.token::transfer")]]   
    void on_transfer(eosio::name& from, eosio::name& to, eosio::asset& sum, string& memo ){

        if(from == get_self()  || from=="donbox"_n) return;      //outgoing or withdrawal from donbox - skip those

        //1) do something
        //2) transfer sum back to flash.sx
        check(from=="flash.sx"_n, "Expecting tokens from flash.sx, not from "+from.to_string());

        eosio::token::transfer_action transfer("eosio.token"_n, { get_self(), "active"_n });
        transfer.send( get_self(), "flash.sx"_n, sum, "sending back");
    }

    //called when paid on behalf of loaner
    [[eosio::on_notify("flash.sx::callback")]]  
	void on_callback(const name from, const name to, const name tcontract, asset sum, const string memo, const name recipient )
	{
		//donbox should have our loaned donation in our name - withdraw it
        donbox::withdraw_action withdraw("donbox"_n, { get_self(), "active"_n });
        withdraw.send( get_self());

        //now repay back to flash.sx        
        token::transfer_action transfer( tcontract, { get_self(), "active"_n });
		transfer.send( get_self(), "flash.sx"_n, sum, memo );
	}

};