#!/bin/bash
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

eosio-cpp trader.cpp -I ../include
cleos -u https://eos.eosn.io set contract basic.sx . basic.wasm basic.abi -p basic.sx@active