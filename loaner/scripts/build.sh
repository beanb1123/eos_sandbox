#!/bin/bash
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

eosio-cpp loaner.cpp -I ../include
cleos set contract loaner . loaner.wasm loaner.abi