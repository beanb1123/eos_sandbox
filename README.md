# eos_sandbox
Learning EOS smart contracts


* ### donbox
  Simple donation box contract

  Users can transfer tokens to `donbox` account specifying receiver in the memo. Anyone can try to withdraw but will only succeed if there are at least 2000 tokens donated in their name
  
  `cleos push action eosio.token transfer '["alice", "donbox", "1000.0000 SYS", "bob"]' -p alice`
  
  `cleos push action donbox withdraw '["bob"]' -p bob`
