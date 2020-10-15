#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>
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

        struct [[eosio::table("sapex")]] sapex_row {
            name                    contract;
            set<symbol_code>        tokens;
            set<extended_symbol>    ext_tokens;

            uint64_t primary_key() const { return contract.value; }
        };
        typedef eosio::multi_index< "sapex"_n, sapex_row > sapex_table;

        /**
         * ## TABLE `defibox`
         *
         * - `{extended_symbol} base` - base symbol
         * - `{map<extended_symbol, string>} quotes` - quote symbols (`string` for additional exchange requirements)
         *
         * ### example
         *
         * ```json
         * {
         *     "base": {"contract":"eosio.token", "symbol": "4,EOS"},
         *     "quotes": [
         *         {"key": {"contract": "tethertether", "symbol": "4,USDT"}, "value": "12"}
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

        struct schema {
            extended_symbol                    base;
            map<extended_symbol, string>       quotes;

            uint64_t primary_key() const { return base.get_symbol().code().raw(); }
        };

        // DFS - https://apps.defis.network/
        struct [[eosio::table("defisswapcnt")]] defisswapcnt_row : schema {};
        typedef eosio::multi_index< "defisswapcnt"_n, defisswapcnt_row > defisswapcnt_table;

        // Defibox BOX - https://defibox.io
        struct [[eosio::table("swap.defi")]] swap_defi_row : schema {};
        typedef eosio::multi_index< "swap.defi"_n, swap_defi_row > swap_defi_table;

        // HamburgerSwap HBG - https://hbg.finance/swap
        struct [[eosio::table("hamburgerswp")]] hamburgerswp_row : schema {};
        typedef eosio::multi_index< "hamburgerswp"_n, hamburgerswp_row > hamburgerswp_table;

        // PIZZA - https://v1.pizza.live/swap
        struct [[eosio::table("pzaswapcntct")]] pzaswapcntct_row : schema {};
        typedef eosio::multi_index< "pzaswapcntct"_n, pzaswapcntct_row > pzaswapcntct_table;

        // SAPEX - http://sapex.one
        struct [[eosio::table("sapexamm.eo")]] sapexamm_eo_row : schema {};
        typedef eosio::multi_index< "sapexamm.eo"_n, sapexamm_eo_row > sapexamm_eo_table;

        // SX - swap.sx
        struct [[eosio::table("swap.sx")]] swap_sx_row : schema {};
        typedef eosio::multi_index< "swap.sx"_n, swap_sx_row > swap_sx_table;

        // SX - stable.sx
        struct [[eosio::table("stable.sx")]] stable_sx_row : schema {};
        typedef eosio::multi_index< "stable.sx"_n, stable_sx_row > stable_sx_table;

        // SX - vigor.sx
        struct [[eosio::table("vigor.sx")]] vigor_sx_row : schema {};
        typedef eosio::multi_index< "vigor.sx"_n, vigor_sx_row > vigor_sx_table;

        /**
         * ## TABLE `global`
         *
         * - `{set<name>} contracts` - exchange contracts
         *
         * ### example
         *
         * ```json
         * {
         *     "contracts": ["swap.sx", "vigor.sx", "stable.sx"]
         * }
         * ```
         */
        struct [[eosio::table("sx")]] sx_row {
            set<name>         contracts;
        };
        typedef eosio::singleton< "sx"_n, sx_row > sx_table;

        /**
         * ## ACTION `setexchange`
         *
         * Update exchange pairs
         *
         * - **authority**: `get_self()`
         *
         * ### params
         *
         * - `{name} contract` - exchange contract
         * - `{extended_asset} requirement` - minimum requirements
         *
         * ### example
         *
         * ```bash
         * cleos push action registry.sx update '["swap.sx", {"contract": "eosio.token", "quantity": "1000.0000 EOS"}]' -p registry.sx
         * ```
         */
        [[eosio::action]]
        void update( const name contract, const extended_asset requirement );

        [[eosio::action]]
        void clear( const name table );

        // action wrappers
        using update_action = eosio::action_wrapper<"update"_n, &registry::update>;
        using clear_action = eosio::action_wrapper<"clear"_n, &registry::clear>;

    private:
        bool is_requirement( const name contract, const asset reserve, const extended_asset requirement );

        template <typename T>
        void add_pair( T& table, const extended_symbol base, const extended_symbol quote, const string optional = "");

        template <typename T>
        void clear_table( T& table );

        // update tables
        void set_defisswapcnt( const extended_asset requirement );
        void set_swap_defi( const extended_asset requirement );
        void set_hamburgerswp( const extended_asset requirement );
        void set_pzaswapcntct( const extended_asset requirement );
        void set_sapexamm_eo();
        void set_sx();
    };
}