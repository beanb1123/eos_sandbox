# eos_sandbox
Learning EOS smart contracts


* ### donbox
  Simple donation box contract

  Users can transfer tokens to `donbox` account specifying receiver in the memo. Anyone can try to withdraw but will only succeed if there are at least 2000 tokens donated in their name
  
  `cleos push action eosio.token transfer '["alice", "donbox", "1000.0000 SYS", "bob"]' -p alice`
  
  `cleos push action donbox withdraw '["bob"]' -p bob`

* ### loaner
  Contract that makes use of [sx.flash](https://github.com/stableex/sx.flash) instant loan functionality. 

  Contract has 2 actions
  
  * `loan(asset tokens)` - loan `tokens` from flash.sx contract and repay back immediately

  *  `loancallback(name to, asset tokens)` - loan `tokens` from flash.sx paid to `to` donbox account, then withdraw from donbox and repay to flash.sx
   

* ### trader
  Makes swap trade with defibox.
  
  Contract has 2 actions:
  * `trade(asset tokens, asset minreturn, string exchange)` - trade `tokens` to `exchange` with anticipated `minreturn` and log calculated return
  * `getcommon()` - list EOS-traded tokens that are traded on all available exchanges 
