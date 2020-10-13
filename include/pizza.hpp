#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

namespace pizza {

    using eosio::asset;
    using eosio::symbol;
    using eosio::name;
    using eosio::multi_index;
    using eosio::singleton;

    using std::pair;

    /**
     * pair
     */
    struct [[eosio::table]] pair_row {
        name                pair;
        symbol              minor_symbol;
        name                minor_contract;
        symbol              major_symbol;
        name                major_contract;
        uint32_t            status;
        uint32_t            time;

        uint64_t primary_key() const { return pair.value; }
    };
    typedef eosio::multi_index< "pair"_n, pair_row > pairs;

    /**
     * total
     */
    struct [[eosio::table]] total_row {
        uint64_t        id;
        asset           total_minor;
        asset           total_major;
        uint64_t        total_mmf;
        asset           total_fee;
        asset           history_mm_minor;
        asset           history_mm_major;
        asset           history_bank_minor;
        asset           history_bank_major;
        asset           history_fee;
        uint32_t        time;

        uint64_t primary_key() const { return 0; }
    };
    typedef eosio::multi_index< "total"_n, total_row > total;

    /**
     * ## STATIC `get_fee`
     *
     * Get total fee
     *
     * ### returns
     *
     * - `{uint8_t}` - total fee (trade + protocol)
     *
     * ### example
     *
     * ```c++
     * const uint8_t fee = pizza::get_fee();
     * // => 30
     * ```
     */
    static uint8_t get_fee()
    {
        return 30;
    }

    /**
     * ## STATIC `get_reserves`
     *
     * Get reserves for a pair
     *
     * ### params
     *
     * - `{uint64_t} pair_id` - pair id
     * - `{symbol} sort` - sort by symbol (reserve0 will be first item in pair)
     *
     * ### returns
     *
     * - `{pair<asset, asset>}` - pair of reserve assets
     *
     * ### example
     *
     * ```c++
     * const uint64_t pair_id = "eos2usde"_n.value;
     * const symbol sort = symbol{"EOS", 4};
     *
     * const auto [reserve0, reserve1] = pizza::get_reserves( pair_id, sort );
     * // reserve0 => "4638.5353 EOS"
     * // reserve1 => "13614.8381 USDE"
     * ```
     */
    static pair<asset, asset> get_reserves( const uint64_t pair_id, const symbol sort )
    {
        // table
        pizza::total _total( "pzaswapcntct"_n, name{pair_id}.value );
        auto total = _total.get(0, "PizzaLibrary: INVALID_PAIR_ID");
        eosio::check( total.total_minor.symbol == sort || total.total_major.symbol == sort, "sort symbol does not match" );

        return sort == total.total_minor.symbol ?
            pair<asset, asset>{ total.total_minor, total.total_major } :
            pair<asset, asset>{ total.total_major, total.total_minor };
    }
}