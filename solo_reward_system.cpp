#include <eosio/eosio.hpp>

using namespace eosio;
using namespace std;

#define pb push_back
#define stake_period 4*60*60

class[[eosio::contract("solo")]] solo : public eosio::contract {
	public:
		solo(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}
		
		asset BSI.symbol = "BSI";
		asset BSI.amount = 0.0000;
		asset BSM.symbol = "BSM";
		asset BSM.amount = 0.0000;
		asset BSG.symbol = "BSG";
		asset BSG.amount = 0.0000;
		
		[[eosio::action]]
		void stake(name user, vector<uint64_t> nfts, string msg) {
			require_auth(user);
			stakeList_index = stakes(get_self(), get_first_receiver().value);
			check(nfts.size()>0, "no stake data");
			
			auto iterator = stakes.find(user);
			
			if(iterator != stakes.end()) { // not exist yet
				stakes.emplace(user, [&](auto& row) {
					row.user = user;
					row.nfts = nfts;
					row.t_start = time(0);
					row.t_end = time(0) + stake_period;
					row.t_update = time(0);
				});
			}
			else { // find exist one
				stakes.modify(iterator, user, [&](auto& row){
					row.t_update = time(0);
					auto (nft : nfts) row.nfts.pb(nft);
				});
			}
			
			//check rewarded user
			for (auto it : stakes) {
				if(it.t_end >= time(0)) {
					
					// set default values  --- stake start
					stakes.modify(iterator, user, [&](auto& row){
						row.t_update = time(0);
						row.t_start = time(0);
						row.t_end = time(0) + stake_period;
					});
					
					// increase BSG token
					person_index people(get_self(), get_first_receiver().value);
					auto iterator_p = people.find(user);
					check(iterator_p != people.end(), "User does not exist");
					
					people.modify(iterator_p, user, [&](auto& row) {
						row.balance_BSG.amount += 1;
					});
					
					// remove contract BSG balance in owner balance
					person_index people(get_self(), get_first_receiver().value);
					auto iterator_o = people.find(get_self());
					check(iterator_o != people.end(), "Owner does not exist");
					
					people.modify(iterator_o, user, [&](auto& row) {
						row.balance_BSG.amount -= 1;
					});
				}
			}
		}

		[[eosio::action]]
		void remove_stake(name user, vector<uint64_t> nfts, string msg) {
			require_auth(user);
			stakeList_index = stakes(get_self(), get_first_receiver().value);
			check(nfts.size()>0, "no stake data");
			
			auto iterator = stakes.find(user);
			
			check (iterator != stakes.end(), "data not exist");
				
			// remove stake data from user data
			stakes.modify(iterator, user, [&](auto& row){
				row.t_update = time(0);
				auto (nft : nfts) row.nfts.remove(nft);
			});
		}
		
	private:
		struct [[eosio::table]] person {
			name user;
			
			asset balance_BSI, balance_BSM, balance_BSG;
			
			//for solo
			int solo_killed;
			int level;
			int respawn_time;
			
			person(){
				balance_BSI = BSI;
				balance_BSM = BSM;
				balance_BSG = BSG;
				solo_killed = 0;
				respawn_time = 0;
				level = 1;
			}
			
			uint64_t primary_key() const { return user.value; }
		};
		using person_index = eosio::multi_index<"people"_n, person>;
		
		struct [[eosio:table]] stakeList {
			name user;
			vector<asset> nfts;
			time t_start;
			time t_end;
			time t_update;
			
			uint64_t primary_key() const { return user.value; }
		}
		
		using stakeList_index = eosio::multi_index<"stakeLists"_n, stakeList>;
}
