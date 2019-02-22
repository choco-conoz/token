#include "traveldnation.token.hpp"

namespace traveldnation {

void traveldnationtoken::create( name issuer, 
								   asset maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats_t statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "traveldnationtoken with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
		s.supply.symbol = maximum_supply.symbol;
		s.max_supply    = maximum_supply;
		s.issuer        = issuer;
    });
}

void traveldnationtoken::issue( name to, asset quantity, string memo )
{
	auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats_t statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "traveldnationtoken with symbol does not exist, create traveldnationtoken before issue" );
    const auto& st = *existing;

	require_auth( st.issuer );
	eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

	eosio_assert( is_blacklist( to ) == false, "not allowed issue to blacklist");

	statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
		SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} }, 
						   { st.issuer, to, quantity, memo }
	   );
	}
}

void traveldnationtoken::retire( asset quantity, string memo ) 
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats_t statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "traveldnationtoken with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void traveldnationtoken::transfer( name from, 
									 name to, 
									 asset quantity, 
									 string memo )
{
	eosio_assert( from != to, "cannot transfer to self" );
	require_auth( from );
	eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats_t statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

	require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	eosio_assert( is_blacklist( from ) == false, "not allowed transfer to blacklist");

	auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
	add_balance( to, quantity, from );
}

void traveldnationtoken::sub_balance( name owner, asset value ) 
{
   accounts_t from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

	from_acnts.modify( from, owner, [&]( auto& a ) {
		a.balance -= value;
	});
}

void traveldnationtoken::add_balance( name owner, asset value, name ram_payer )
{
   accounts_t to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void traveldnationtoken::open( name owner, const symbol& symbol, name ram_payer ) 
{
    require_auth( ram_payer );

    auto sym_code_raw = symbol.code().raw();

    stats_t statstable( _self, sym_code_raw );
    const auto st = statstable.get( sym_code_raw, "symbol does not exist" );
    eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

    accounts_t acnts( _self, owner.value );
    const auto it = acnts.find( sym_code_raw );
    if( it == acnts.end() ) {
        acnts.emplace( ram_payer, [&]( auto& a ){
            a.balance = eosio::asset{0, symbol};
        });
    }
}

void traveldnationtoken::close( name owner, const symbol& symbol ) 
{
    require_auth( owner );
    accounts_t acnts( _self, owner.value );
    const auto it = acnts.find( symbol.code().raw() );
    eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
    eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
    acnts.erase( it );
}

void traveldnationtoken::addblacklist( name user/*, const symbol& symbol*/ )
{
	eosio_assert( is_account( user ), "account does not exist");
	eosio_assert( is_blacklist( user ) == false, "account already exist in blacklist");

	/*eosio_assert( symbol.is_valid(), "invalid symbol name" );

    auto sym_code_raw = symbol.code().raw();
	stats_t statstable( _self, sym_code_raw );

    auto existing = statstable.find( sym_code_raw );
    eosio_assert( existing != statstable.end(), "does not exist traveldnationtoken with symbol" );

	const auto& st = *existing;
	require_auth( st.issuer );*/

	_blacklists.emplace( get_self(), [&]( auto &a ) {
        a.reg_time = eosio::time_point_sec( now() ).utc_seconds;
        a.user = user;
    });        
}

void traveldnationtoken::rmvblacklist( name user/*, const symbol& symbol*/ )
{
	eosio_assert( is_account( user ), "account does not exist");
	eosio_assert( is_blacklist( user ) != false, "account is empty in blacklist");

	/*eosio_assert( symbol.is_valid(), "invalid symbol name" );

    auto sym_code_raw = symbol.code().raw();
	stats_t statstable( _self, sym_code_raw );

    auto existing = statstable.find( sym_code_raw );
    eosio_assert( existing != statstable.end(), "does not exist traveldnationtoken with symbol" );

	const auto& st = *existing;
	require_auth( st.issuer );*/

	auto itr = _blacklists.find( user.value );
    if( itr != _blacklists.end() ) {
        _blacklists.erase( itr );
    }
}

bool traveldnationtoken::is_blacklist( name user )
{
	auto itr = _blacklists.find(user.value);    
	return itr == _blacklists.end() ? false : true ;
}

} // namespace traveldnation

EOSIO_DISPATCH( traveldnation::traveldnationtoken, (create)(issue)(retire)(transfer)(open)(close)(addblacklist)(rmvblacklist) )
