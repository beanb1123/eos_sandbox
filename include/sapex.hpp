#pragma once

#include <eosio/asset.hpp>

namespace sapex {

    using eosio::asset;
    using eosio::symbol;
    using eosio::name;

    /**
     * SAPEX markets
     */
    struct [[eosio::table]] sapextbl_row {
        symbol_code            ticker;
        name                   contract;
        uint64_t               token;
        uint64_t               sapex;
        uint64_t primary_key() const { return ticker.raw(); }
    };
    typedef eosio::multi_index< "sapextbl"_n, sapextbl_row > sapextbl;

    /**
     * ## STATIC `get_fee`
     *
     * Get Sapex total fee. 0.10% for each hop + 0.01% rounding error
     *
     * ### params
     *
     * - `{symbol} sym1` - incoming symbol
     * - `{symbol} sym2` - outgoing symbol
     *
     * ### returns
     *
     * - `{uint8_t}` - total fee (trade + protocol)
     *
     * ### example
     *
     * ```c++
     * const uint8_t fee = sapex::get_fee();
     * // => 0
     * ```
     */
    static uint8_t get_fee(symbol& from, symbol& to)
    {
        return from.code()==symbol_code{"SAPEX"} || to.code()==symbol_code{"SAPEX"} ? 11 : 22;
    }

    /**
     * ## STATIC `get_reserves`
     *
     * Get reserves for a pair
     *
     * ### params
     *
     * - `{symbol} sym1` - symbol for reserve1
     * - `{symbol} sym2` - symbol for reserve2
     *
     * ### returns
     *
     * - `{pair<asset, asset>}` - pair of reserve assets
     *```
     */
    static std::pair<asset, asset> get_reserves( symbol sym1, symbol sym2)
    {
        check(sym1!=sym2, "SAPEXLibrary: INVALID_PAIR");
        // table
        sapex::sapextbl _markets( "sapexamm.eo"_n, "sapexamm.eo"_n.value );
        if(sym1.code().to_string()=="SAPEX"){   //SAPEX->XXX
            auto row = _markets.get( sym2.code().raw(), "SAPEXLibrary: INVALID_SYMBOL" );
            return { { (int64_t)row.sapex,sym1 }, { (int64_t)row.token, sym2 } };
        }
        if(sym2.code().to_string()=="SAPEX"){   //XXX->SAPEX
            auto row = _markets.get( sym1.code().raw(), "SAPEXLibrary: INVALID_SYMBOL" );
            return { { (int64_t)row.token, sym1 }, { (int64_t)row.sapex, sym2 } };
        }
        //neither is SAPEX
        auto row1 = _markets.get( sym1.code().raw(), "SAPEXLibrary: INVALID_SYMBOL" );
        auto row2 = _markets.get( sym2.code().raw(), "SAPEXLibrary: INVALID_SYMBOL" );

        auto out = row2.token * ( ((double)row1.sapex) / row2.sapex);
        return { { (int64_t)row1.token, sym1 }, { (int64_t)out, sym2 } };
    }
}