#!/bin/bash


# Just loan from flash.sx and repay immediately on transfer
cleos push action loaner loan '["100.0000 SYS"]' -p loaner

# Loan from flash.sx with payment directly to donbox, then withdraw from donbox and repay on callback 
cleos push action loaner loancallback '["donbox", "1000.0000 SYS"]' -p loaner