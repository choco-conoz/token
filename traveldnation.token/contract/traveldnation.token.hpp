/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>

#include <string>

namespace eosiosystem {
	class system_contract;
}

namespace traveldnation {

	using namespace std;
	using namespace eosio;
	
	class [[eosio::contract("traveldnation.token")]] traveldnationtoken : public contract {
		using contract::contract;
			
		public:
			traveldnationtoken( name receiver, name code, datastream<const char*> ds ) : contract( receiver, code, ds ),
				_blacklists( receiver, code.value ) {}
				
			[[eosio::action]]
			void create( name issuer, 
						 asset maximum_supply );
						 
			[[eosio::action]]	 
			void issue( name to, asset quantity, string memo );
			
			
			[[eosio::action]]	 
			void retire( asset quantity, string memo );
			
			[[eosio::action]]	 
			void transfer( name from, 
						   name to, 
						   asset quantity, 
						   string memo );
						   
			[[eosio::action]]	 
			void open( name owner, const symbol& symbol, name ram_payer );
			
			[[eosio::action]]	 
			void close( name owner, const symbol& symbol );

			[[eosio::action]]	 
			void addblacklist( name user/*, 
							   const symbol& symbol*/ );
			
			[[eosio::action]]	 
			void rmvblacklist( name user/*, 
							   const symbol& symbol*/ );

			static asset get_supply( name token_contract_account, symbol_code sym_code )
			{
				stats_t statstable( token_contract_account, sym_code.raw() );
				const auto& st = statstable.get( sym_code.raw() );
				return st.supply;
			}

			static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
			{
				accounts_t accountstable( token_contract_account, owner.value );
				const auto& ac = accountstable.get( sym_code.raw() );
				return ac.balance;
			}
			
			using create_action = eosio::action_wrapper<"create"_n, &traveldnationtoken::create>;
			using issue_action = eosio::action_wrapper<"issue"_n, &traveldnationtoken::issue>;
			using retire_action = eosio::action_wrapper<"retire"_n, &traveldnationtoken::retire>;
			using transfer_action = eosio::action_wrapper<"transfer"_n, &traveldnationtoken::transfer>;
			using open_action = eosio::action_wrapper<"open"_n, &traveldnationtoken::open>;
			using close_action = eosio::action_wrapper<"close"_n, &traveldnationtoken::close>;
			using addblacklist_action = eosio::action_wrapper<"addblacklist"_n, &traveldnationtoken::addblacklist>;
			using rmvblacklist_action = eosio::action_wrapper<"rmvblacklist"_n, &traveldnationtoken::rmvblacklist>;

		private:
			struct [[eosio::table]] account {
				asset    balance;

				uint64_t primary_key()const { return balance.symbol.code().raw(); }
			};

			struct [[eosio::table]] currency_stats {
				asset          supply;
				asset          max_supply;
				name		   issuer;

				uint64_t primary_key()const { return supply.symbol.code().raw(); }
			};

			struct [[eosio::table]] blacklist {
				uint64_t	   reg_time;
				name		   user;
			
				uint64_t primary_key()const { return user.value; }
			};

			typedef eosio::multi_index< "accounts"_n, account > accounts_t;
			typedef eosio::multi_index< "stat"_n, currency_stats > stats_t;
			typedef eosio::multi_index< "blacklist"_n, blacklist > blacklists_t;

			void sub_balance( name owner, asset value );
			void add_balance( name owner, asset value, name ram_payer );

			bool is_blacklist( name user );
		
		private:
			// local instances of the multi indexes
		    blacklists_t _blacklists;
	};

} /// namespace eosio
