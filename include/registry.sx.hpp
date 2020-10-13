#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

namespace sx {
    class [[eosio::contract("registry.sx")]] registry : public contract {
    public:
        using contract::contract;

        /**
         * ## TABLE `swap`
         *
         * - `{name} contract` - swap contract account name
         * - `{set<symbol_code>} tokens` - supported tokens
         * - `{set<extended_symbol>} ext_tokens` - supported extended symbols
         *
         * ### example
         *
         * ```json
         * {
         *     "contract": "swap.sx",
         *     "tokens": ["EOS", "EOSDT", "USDT"],
         *     "ext_tokens": [
         *         { "sym": "4,EOS", "contract": "eosio.token" },
         *         { "sym": "4,USDT", "contract": "tethertether" },
         *         { "sym": "9,EOSDT", "contract": "eosdtsttoken" }
         *     ]
         * }
         * ```
         */
        struct [[eosio::table("swap")]] swap_row {
            name                    contract;
            set<symbol_code>        tokens;
            set<extended_symbol>    ext_tokens;

            uint64_t primary_key() const { return contract.value; }
        };
        typedef eosio::multi_index< "swap"_n, swap_row > swap_table;

        /**
         * ## TABLE `defibox`
         *
         * - `{extended_symbol} base` - base symbols
         * - `{map<symbol_code, uint64_t>} pair_ids` - pair ids
         * - `{map<symbol_code, extended_symbol>} contracts` - contracts
         *
         * ### example
         *
         * ```json
         * {
         *     "base": {"contract":"eosio.token", "symbol": "4,EOS"},
         *     "pair_ids": [
         *         {"key": "USDT", "value": 12}
         *     ],
         *     "contracts": [
         *         {"key": "USDT", "value": "tethertether"}
         *     ]
         * }
         * ```
         */
        struct [[eosio::table("defibox")]] defibox_row {
            extended_symbol                     base;
            map<symbol_code, uint64_t>          pair_ids;
            map<symbol_code, name>              contracts;

            uint64_t primary_key() const { return base.get_symbol().code().raw(); }
        };
        typedef eosio::multi_index< "defibox"_n, defibox_row > defibox_table;

        struct [[eosio::table("dfs")]] dfs_row {
            extended_symbol                     base;
            map<symbol_code, uint64_t>          pair_ids;
            map<symbol_code, name>              contracts;

            uint64_t primary_key() const { return base.get_symbol().code().raw(); }
        };
        typedef eosio::multi_index< "dfs"_n, dfs_row > dfs_table;

        struct [[eosio::table("hamburger")]] hamburger_row {
            extended_symbol                     base;
            map<symbol_code, uint64_t>          pair_ids;
            map<symbol_code, name>              contracts;

            uint64_t primary_key() const { return base.get_symbol().code().raw(); }
        };
        typedef eosio::multi_index< "hamburger"_n, hamburger_row > hamburger_table;

        struct [[eosio::table("pizza")]] pizza_row {
            extended_symbol                     base;
            map<symbol_code, uint64_t>          pair_ids;
            map<symbol_code, name>              contracts;

            uint64_t primary_key() const { return base.get_symbol().code().raw(); }
        };
        typedef eosio::multi_index< "pizza"_n, pizza_row > pizza_table;

        /**
         * ## ACTION `setswap`
         *
         * Set swap contract
         *
         * - **authority**: `get_self()`
         *
         * ### params
         *
         * - `{name} contract` - swap contract account name
         *
         * ### example
         *
         * ```bash
         * cleos push action registry.sx setswap '["eosdt.sx"]' -p registry.sx
         * ```
         */
        [[eosio::action]]
        void setswap( const name contract );

        [[eosio::action]]
        void clear();

        /**
         * ## ACTION `setdefibox`
         *
         * Set defibox pairs
         *
         * - **authority**: `get_self()`
         *
         * ### params
         *
         * - `{extended_asset} requirement` - minimum requirements
         *
         * ### example
         *
         * ```bash
         * cleos push action registry.sx setdefibox '[{"contract": "eosio.token", "quantity": "1000.0000 EOS"}]' -p registry.sx
         * ```
         */
        [[eosio::action]]
        void setdefibox( const extended_asset requirement );

        [[eosio::action]]
        void setdfs( const extended_asset requirement );

        [[eosio::action]]
        void sethamburger( const extended_asset requirement );

        [[eosio::action]]
        void setpizza( const extended_asset requirement );

        // action wrappers
        using setswap_action = eosio::action_wrapper<"setswap"_n, &registry::setswap>;
        using setdefibox_action = eosio::action_wrapper<"setdefibox"_n, &registry::setdefibox>;
        using setdfs_action = eosio::action_wrapper<"setdfs"_n, &registry::setdfs>;
        using sethamburger_action = eosio::action_wrapper<"sethamburger"_n, &registry::sethamburger>;
        using setpizza_action = eosio::action_wrapper<"setpizza"_n, &registry::setpizza>;

    private:
        bool is_requirement( const name contract, const asset reserve, const extended_asset requirement );

        template <typename T>
        void add_pair( T& table, const extended_symbol base, const extended_symbol quote, const uint64_t pair_id );

        template <typename T>
        void clear_table( T& table );

        // void add_token( const symbol sym, const name contract );
    };
}