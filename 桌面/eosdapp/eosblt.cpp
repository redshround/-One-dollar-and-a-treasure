/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "eosblt.hpp"

namespace eosio {
void xxx();

//创建货币
//@abi action
void eosblt::create( account_name issuer,		//创建者
                    asset        maximum_supply //最大发行量
)
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
	print("\nsym :",sym);
	print("\nsym :", sym.name());
	print("\nsym :", maximum_supply);
	print("\nsym :", maximum_supply.symbol);

    eosio_assert( sym.is_valid(), "invalid symbol name" );  // 确保正确的代币符号
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.name() );//(code, scope)	//player域指由谁来支付这个数据存储费用
	print("\n_self :", name{ _self });
    auto existing = statstable.find( sym.name() ); // 根据代币的符号，比如"EOS"，在合约中进行查找.
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}

//发行货币
//@abi action
void eosblt::issue( account_name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stats statstable( _self, sym_name );//实例化数据库,参数:uint64_t code(表的拥有者), uint64_t scope (表的细分名称,谁的表)
	auto existing = statstable.find( sym_name );
    

    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");
    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });
    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {	//如果不是发行给自己,就会先发行给自己,再调用转账.
       SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    }
}
//cleos push action yydb.token transfer '{"from":"yydb.token","to":"yydb11111111","quantity":"10.0000 BLT","memo":"321321321231"}' -p yydb.token
//转账
//@abi action
void eosblt::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );   //验证from账户权限合法性.需要from的签名. 
    
    eosio_assert( is_account( to ), "to account does not exist");//检查eos账户数据库,判断是否有该账号;

    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

	print("\nst.supply.symbol.name(): ", st.supply);
	print("\nst.max_supply.symbol.name(): ",st.max_supply);
	print("\nst.issuer : ", name{ st.issuer });

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

//减少某个账户中的余额
void eosblt::sub_balance( account_name owner, asset value ) {
	print("\n减少某个账户中的余额 owner : ", name{ owner });
	print("\n减少某个账户中的余额 value : ", value);
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );
   print("\n当前账户中的余额 : ", from.balance);

   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );	//erase删除
	  print("\nfrom_acnts.erase( from )");
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {	//更新数据
          a.balance -= value;
		  print("\n from_acnts.modify() : ", a.balance);
      });
   }
}

//增加某个账户中的余额
void eosblt::add_balance( account_name owner, asset value, account_name ram_payer )
{
	print("\nowner : ", name{owner});
	print("\nvalue : ", value);
	print("\nram_payer : ", name{ram_payer});
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){  //添加数据(由ram_payer提供储存空间.)
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {  //修改数据(迭代器,储存空间支付账户(已经注册过了))
        a.balance += value;
      });
   }
}

//检测accounts表中是否存在该账户;
bool eosblt::is_bltacc(account_name to ){
    print("\n要检测的账户名: ",name{to});

    accounts acct(_self,to);//yydb.token,n:指定用户名
    auto prod = acct.find( to );
    if(prod == acct.end()) {//find后,找到结尾都没找到即为 无;
        print("\n没找到");
        return false;
    }
    else{
        print("\n找到");
        return true;
    } 
}


	//新建账户,并给予初始奖励.
    //账号名,weststarqp 初始代币. 代币名BLT
    //@abi action
void eosblt::newaccount( string to, asset quantity)
{
		require_auth( _self );  
        const char *pUser = to.c_str();
        account_name user = eosio::string_to_name(pUser);
       // bool is_temporary = false;
        if(is_account( user )){//判断是否是EOS账户
        print("\n是eos标准账户,添加到数据表.");//标准账户直接用 add_balance sub_balance转账.

        accounts to_acnts( _self, user );  //是EOS账号,直接用其账号名开户.
        auto existing = to_acnts.find( quantity.symbol.name() );  // 根据账号名查找数据库
        eosio_assert( existing == to_acnts.end(), "该账号已经存在.请勿重复领取." );
        sub_balance( _self, quantity );         //扣除本账户代币金额
        add_balance( user, quantity, _self );   //增加对方账号代币金额

        }else{  //不是EOS标准账号名

        print("\n非标准账户,创建本地账号.");

        accounts to_acnts( _self, _self );  //使用自己账号名建表,其余信息保存进tmps.
        auto selfacct = to_acnts.find( quantity.symbol.name() );
        eosio_assert( selfacct != to_acnts.end(),"self表不存在!,是否忘记创建代币?请检查!");

        for (const auto& tmp : selfacct->tmps) {        //判断重复账号
			print("\ntmp.user : ", tmp.user);
            print("\ntmp.balance : ", tmp.balance);
            eosio_assert( to != tmp.user, "该账号已经存在.请勿重复领取." );
		}

        to_acnts.modify( selfacct, 0, [&]( auto& s ) {	//更新数据
           s.balance -= quantity;      //减少本账户代币金额
           s.add_tmp(to,quantity);     //增加代投账户金额
		   print("\n数据更新完毕,本账户金额余剩 : ", s.balance);
        });
    }
	
}

//遍历数据库(参数类型account_name,给定的参数必须是用户名.12位字符)
//@abi action
void eosblt::traversaltab(account_name who,account_name n){
    print("\n要查询的账号: ",name{n});

    // //查询自己的realacct表
    // realacct racct(_self,_self);
    // auto rcust_itr = racct.begin();
    // print("\n迭代开始.");
    // while (rcust_itr != racct.end() ) {
    //       print("\n name : ",name{rcust_itr->name});//a123451a1231
    //       print("\n balance : ",rcust_itr->balance.balance);
    // rcust_itr++;//迭代器自增，指向下一条数据
    // } 
    // print("\n迭代结束.");


    //查询自己的accounts表
    print("\n查询自己的accounts表 :");
    accounts acct(_self,n);//yydb.token,n:指定用户名
    auto cust_itr = acct.begin();
    print("\n迭代开始.");
    while (cust_itr != acct.end() ) {
		auto tmps = cust_itr->tmps;
		for (const auto& tmp : tmps) {
			print("\ntmp string: ", tmp.user);
            print("\ntmp balance: ", tmp.balance);
            print("\ntmp balance: ", tmp.balance.amount);
		}
        //  auto sym = cust_itr->balance.symbol;
        //   print("\n cust_itr[i]符号名 : ",sym);//4,EOS
        //   print("\n cust_itr[i]币名 : ",name{cust_itr->user});//5459781
        //   print("\n cust_itr[i]是否代投 : ",cust_itr->is_temporary);//
        //   print("\n cust_itr[i]余额balance : ",cust_itr->balance);//14.5000 EOS
        //   print("\n cust_itr[i]余额balance.amount : ",cust_itr->balance.amount);//145000
    cust_itr++;//迭代器自增，指向下一条数据
    } 
    print("\n迭代结束.");

    // //查询自己的accounts表
    // print("\n查询自己的accounts表 :");
    // accounts acct(_self,n);//yydb.token,n:指定用户名
    // auto cust_itr = acct.begin();
    // print("\n迭代开始.");
    // while (cust_itr != acct.end() ) {
    //       auto sym = cust_itr->balance.symbol;
    //       print("\n cust_itr[i]符号名 : ",sym);//4,EOS
    //       print("\n cust_itr[i]币名 : ",name{cust_itr->user});//5459781
    //       print("\n cust_itr[i]是否代投 : ",cust_itr->is_temporary);//
    //       print("\n cust_itr[i]余额balance : ",cust_itr->balance);//14.5000 EOS
    //       print("\n cust_itr[i]余额balance.amount : ",cust_itr->balance.amount);//145000
    // cust_itr++;//迭代器自增，指向下一条数据
    // } 
    // print("\n迭代结束.");



    //  //查询yydb的gamerooms表
    // _gamerooms gameroom(who,n);
    // auto cust_itr = gameroom.begin();
    // print("\n迭代开始.");
    // while (cust_itr != gameroom.end() ) {
    //   print("\n cust_itr.id : ",cust_itr->id);
    //   print("\n cust_itr.roomtitle : ",cust_itr->roomtitle);
    //   print("\n cust_itr.roomdesc : ",cust_itr->roomdesc);
    //   print("\n cust_itr.img : ",cust_itr->img);
    //   print("\n cust_itr.create_time : ",cust_itr->create_time);
    //   print("\n cust_itr.update_time : ",cust_itr->update_time);
    //   cust_itr++;//迭代器自增，指向下一条数据
    // } 
    // print("\n迭代结束.");
}

//@abi action
void eosblt::test(account_name who, account_name tabname) {
	print("\naccount_name : ", who);
	print("\ntabname : ", tabname);
   action(
       permission_level{who, N(active)}, //permssion_level权限
       N(yydb),                     //合约名
       N(createtask),               //函数名
       std::make_tuple(std::string("1"),std::string("1"),std::string("2"),std::string("33"))          //参数
    ).send();

}


//创建游戏账户(name:用户名(真实账户),quantity:初始分配代币)
//a123451a1231
//a123451a1111

//@abi action 
void eosblt::cetrealacct(account_name user,asset quantity){
    // require_auth(_self);    //验证合约调用者
    // print("\ncreateacct : ", name{user});
    // print("\n创建real_account用户");

    //  require_auth( user );   //验证账户名合法性.
    //  realacct realacc(_self,_self);
    //  auto cust_itr = realacc.find(user);   //使用迭代器指向所需数据
    //  eosio_assert(cust_itr == realacc.end(), "Address for account already exists");

    //  print("\n未到账号,开始创建");
    //  realacc.emplace( _self, [&]( auto& s ) {    //增加新用户数据
    //   s.name      = user;       
    //   s.balance.balance   = quantity;
    //  });
}

//创建游戏账户(name:用户名,quantity:初始分配代币)
//user类型如果是account_name在传参时就会有12位字符限制,必须使用string接收后再转换成account_name
//@abi action
void eosblt::cetdummyacct(string user,asset quantity){
    // require_auth(_self);    //验证合约调用者
    // print("\n创建dummy_account用户");
    // print("\ncreateacct : ", user);
    // //此处应该有name合法判断,尚未有此标准.
    // const char *_Ptr = user.c_str();
    // account_name customer_acct = eosio::string_to_name(_Ptr);
    // print("\ncustomer_acct : ",name{customer_acct});
    //  dummyacct dummyacc(_self,_self);
    //  auto cust_itr = dummyacc.find(customer_acct);   //使用迭代器指向所需数据
    //  eosio_assert(cust_itr == dummyacc.end(), "Address for account already exists");

    //  print("\n未到账号,开始创建");
    //  dummyacc.emplace( _self, [&]( auto& s ) {    //增加新用户数据
    //   s.name      = customer_acct;       
    //   s.balance.balance   = quantity;
    //  });
}

} /// namespace eosio


EOSIO_ABI(eosio::eosblt, (create)(issue)(transfer)(test)(cetrealacct)(cetdummyacct)(traversaltab)(newaccount) )
