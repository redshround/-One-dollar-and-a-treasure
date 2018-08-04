/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/action.h>
#include <eosiolib/types.hpp>
#include <eosiolib/multi_index.hpp>
#include <string>

// namespace eosiosystem {
//    class system_contract;
// }

namespace eosio {

   using std::string;

   class eosblt : public contract {
      public:
		  eosblt(account_name self) :contract(self)//账户部署 token 合约时该合约会将self存到到父类的 _self 属性中。
		  {}

         void create( account_name issuer,
                      asset        maximum_supply);

         void issue( account_name to, asset quantity, string memo );

         void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );
      
        //  inline asset get_supply( symbol_name sym )const;
         
        //  inline asset get_balance( account_name owner, symbol_name sym )const;

		 void test(account_name who, account_name tabname);
    public:
        bool is_bltacc(account_name to );
        void newaccount( string  to, asset quantity);

        //测试
        void cetrealacct(account_name user,asset quantity);   
        void cetdummyacct(string user,asset quantity);
        void testremote(account_name n);
        void traversaltab(account_name who,account_name n);
    private:

	struct st_gameroom
	{
		uint64_t id;
		string roomtitle;		//房间标题
		string roomdesc;		//房间描述
		string img;				//房间头像
		uint64_t status;		//状态
		uint64_t multiple;		//房间倍数
		int64_t gametask_id;	//当前任务的ID
		uint64_t effec_time;	//游戏有效时间，单位是秒		
		uint64_t create_time;	//创建时间
		uint64_t update_time;	//更新时间		

		auto primary_key() const
		{
			return id;
		}
	};

    	struct st_tmp
	    {
		    string user;
		    asset balance;
	    };
          //cleos get table eosio.token eosio accounts 保管该数据表中创建过的所有代币
		  //@abi table accounts i64
         struct account {
            asset    balance;	        //account balance
            vector<st_tmp>   tmps{};	//代投账号列表
			auto primary_key()const { return balance.symbol.name(); }
           // auto primary_key()const { return user; }
			//EOSLIB_SERIALIZE(account,(user)(is_temporary)(balance))
            void add_tmp(string user,asset balance) {
			    st_tmp tmp{user,balance};
			    tmps.emplace_back(tmp);
		    }   
         };

         //代投账户列表
		 //@abi table dummyacct
		 struct dummy_account {
             uint64_t id;
			 string user;		//用户名(代投账户不使用account_name类型)
			 asset balance;	    //资产
             //设置主键
             auto primary_key()const { return balance.symbol.name(); }
             //EOSLIB_SERIALIZE(dummy_account,(name)(balance))
		 };

         //记录代币创建时状态
		 //@abi table stats
         struct eosblt_stats {
            account_name   issuer;			//发行者
            asset          supply;			//发行量;
            asset          max_supply;		//最大发行量;
			auto primary_key()const {return supply.symbol.name();}
			//EOSLIB_SERIALIZE(eosblt_stats,(issuer)(supply)(max_supply))
         };



		 //真实账户列表
		 //@abi table realacct
		 struct real_account {
			 account_name name;
			 account balance;
             //设置主键
             auto primary_key()const { return name; }
              //SERIALIZE 宏可以帮助提高编译速度
            // EOSLIB_SERIALIZE(real_account,(name)(balance))
		 };

		 //multi_index<N(表名称),模板名>
         typedef eosio::multi_index<N(accounts), account> accounts;
         typedef eosio::multi_index<N(stats), eosblt_stats> stats;

		 typedef eosio::multi_index<N(dummyacct), dummy_account> dummyacct;
		 typedef eosio::multi_index<N(realacct), real_account> realacct;

	     typedef eosio::multi_index<N(gamerooms), st_gameroom> _gamerooms;


         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );

      public:
         struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
         };
   };
   

//    asset eosblt::get_supply( symbol_name sym )const
//    {
//       stats statstable( _self, sym );
//       const auto& st = statstable.get( sym );
//       return st.supply;
//    }

//    asset eosblt::get_balance( account_name owner, symbol_name sym )const
//    {
//       accounts accountstable( _self, owner );
//       const auto& ac = accountstable.get( sym );
//       return ac.balance;
//    }

} /// namespace eosio
