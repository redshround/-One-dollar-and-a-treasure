#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/transaction.hpp>


using namespace eosio;
using namespace std;
enum {
	USER = 1,
	NUM = 3
};
class eoswin : public eosio::contract
{

public:

	eoswin(account_name self)
		: eosio::contract(self),
		_accounts(_self, _self),
		_gamerooms(_self, _self),
		_gametasks(_self, _self)
	{}

private:

	///游戏房间
	//@abi table gamerooms i64
	struct st_gameroom
	{
		uint64_t id;
		string roomtitle;		//房间标题
		string roomdesc;		//房间描述
		string img;				//房间头像
		uint64_t status;		//状态
		uint64_t min_bets;		//投注下限
		uint64_t max_bets;		//投注上限
		int64_t gametask_id;	//当前任务的ID
		uint64_t effec_time;	//游戏有效时间，单位是秒		
		uint64_t create_time;	//创建时间
		uint64_t update_time;	//更新时间		

		auto primary_key() const
		{
			return id;
		}
	};


	struct st_bet
	{
		account_name user;
		uint64_t lottery_num;
		uint64_t bet_time;
	};

	///游戏任务
	//@abi table gametasks i64
	struct st_gametask
	{
	    uint64_t id;
		uint64_t roomid;			//房间号			
		uint64_t periods_num;		//期数
		uint64_t token_sum;			//已投注数量
		uint64_t token_limit;		//投注限制
		uint64_t lottery_seed;		//开奖种子		
		uint64_t lottery_time;		//开奖时间
		uint64_t lottery_num;		//中奖号码
		uint64_t min_num;			//最小编号
		uint64_t max_num;			//最大编号
		uint64_t last_num;			//最后分配出去的编号		
		uint64_t status;			//任务状态 0： 已创建， 1： 已启动 投注中， 2： 时间到，等待开奖 3： 已开奖
		account_name lott_account;	//中奖帐号
		account_name start_acc;		//启动帐号
		uint64_t start_time;		//启动时间
		uint64_t create_time;		//创建时间

		vector<st_bet>   bets{};	//投注记录

		auto primary_key() const
		{
			return id;
		}

		void add_bet(account_name const& player, uint64_t const& lottery_num, uint64_t bet_time) {
			st_bet bet{player, lottery_num, bet_time};
			bets.emplace_back(bet);
		}
	};

	//@abi table accounts i64
	struct account
	{
		account_name owner;
		uint64_t balance;

		account_name primary_key()const
		{
			return owner;
		}

		EOSLIB_SERIALIZE(account, (owner)(balance))
	};


	multi_index<N(gamerooms), st_gameroom> _gamerooms;
	multi_index<N(gametasks), st_gametask> _gametasks;
	multi_index<N(accounts), account> _accounts;


public:

	struct transfer_blt
	{
		account_name from;
		account_name to;
		asset        quantity;
		string       memo;
	};
	

	void issue(uint64_t quantity)
	{
		require_auth( _self );
		add_balance(_self, _self, quantity);
	}

	//初始奖励
	void initreward( account_name  to, uint64_t quantity)
	{
		require_auth( _self );
		add_balance(_self, to, quantity);
		sub_balance(_self, quantity);
	}

	///创建 或 修改游戏房间信息
	//@abi action
	void updateroom(uint64_t id, string title, string desc, string img, uint64_t status, uint64_t min_bets, uint64_t max_bets, uint64_t effec_time)
	{
		require_auth(_self);
		auto gameroom_itr = _gamerooms.find(id);		

		if(gameroom_itr == _gamerooms.end())
		{
			auto itr = _gamerooms.emplace(_self, [&](auto & a)
			{
				a.id = id;
				a.roomtitle = title,
				a.roomdesc = desc;
				a.img = img;
				a.status = status;
				a.min_bets = min_bets;
				a.max_bets = max_bets;
				a.effec_time = effec_time;
				a.gametask_id = 0;
				a.create_time = current_time();
			});
		} 
		else 
		{
			_gamerooms.modify(gameroom_itr, 0, [&](auto & a)
			{
				a.roomtitle = title;
				a.roomdesc = desc;
				a.img = img;
				a.min_bets = min_bets;
				a.max_bets = max_bets;
				a.status = status;
				a.effec_time = effec_time;
				a.update_time = current_time();			
			});
		}
	}

	/// 删除房间
	void delroom(uint64_t roomid) {
		
		require_auth(_self);
		auto gameroom_itr = _gamerooms.find(roomid);
		eosio_assert(gameroom_itr != _gamerooms.end(), "can not find gameroom by roomid");
		_gamerooms.erase(gameroom_itr);		
		for (const auto& item : _gamerooms) {
			print(item.id, "==");
		}
	}

	// 创建游戏任务
    void createtask(uint64_t taskid, uint64_t roomid, uint64_t periods_num, uint64_t token_limit)
	{
		print("create_gametask:", taskid, "=======");
		_gametasks.emplace(_self, [&](auto & a)
		{
			a.id = taskid;
			a.roomid = roomid;
			a.token_sum = 0;
			a.token_limit = token_limit;
			a.lottery_time = 0;
			a.min_num = 100000001;
			a.max_num = 100000001 + token_limit - 1;
			a.last_num = 0;
			a.status = 0;
			a.create_time = current_time();
		});			
	}


	/// 删除游戏任务
	void delgametask(uint64_t start, uint64_t end) {
		require_auth(_self);	
		eosio_assert(start <= end, "start 必须小于 end");
		for (int i = start; i <= end; i++) {
			auto gametask_itr = _gametasks.find(i);
			if(gametask_itr != _gametasks.end()){
				_gametasks.erase(gametask_itr);
			}
		}
	}


	///投注
    //@abi action
    void bet(account_name user, uint64_t gametask_id, uint64_t tokens)
    {
		print("\nbet 下注id : ",gametask_id);
        require_auth(_self);
        eosio_assert(tokens > 0, "illegal argument: tokens");
		
		//先根据游戏任务ID查询出任务信息
        auto gametask_itr = _gametasks.find(gametask_id);
        eosio_assert(gametask_itr != _gametasks.end(), "can not find gametask by gametask_id");
        eosio_assert(gametask_itr->lottery_time == 0, "task is Finished");
		st_gametask gametask = *gametask_itr;
        uint64_t limit = gametask_itr->token_limit - gametask_itr->token_sum;

		print("tokens=", tokens);

		uint64_t bet_num = gametask_itr->last_num;
		account_name start_acc = gametask_itr->start_acc;
		uint64_t start_time =gametask_itr->start_time;
		//分配幸运号码
		for (int i = 0; i < tokens; i++) {

			if (bet_num == 0) {
				bet_num = gametask_itr->min_num;
				start_time = current_time();
				start_acc = user;
			} else {
				bet_num++;
			}		
			gametask.add_bet(user, bet_num, current_time());
		}
		//先把投注的tokens转到庄家帐号下
		print("====开始转账====，"); //这步骤在token合约中完成.
	//	transfer(user, N(yydb), tokens);

		_gametasks.modify(gametask_itr, 0, [&](auto & a)
		{
			a.bets = gametask.bets;
			a.last_num = bet_num;
			a.token_sum += tokens;
			a.start_acc = start_acc;
			a.start_time = start_time;
			a.status = 1;
		});
    }


	///开奖处理，参数：开奖任务ID 和 开奖种子
	//@abi action
	void winner(uint64_t gametask_id, uint64_t lottery_seed)
	{
		require_auth(_self);
		//查询游戏任务 
		auto gametask_itr = _gametasks.find(gametask_id);
		account_name winner;
		uint64_t sum_bet_time = 0;
		auto bets = gametask_itr->bets;
		uint64_t lottery_time = current_time();
		for (const auto& bet : bets) {
			uint64_t t = bet.bet_time / 1000000;
			print("\nt: ", t, "=========");
			sum_bet_time += t;			
		}
		print("\nsum_bet_time", sum_bet_time, "=========");
		print("\ngametask_itr->token_sum", gametask_itr->token_sum, "=========");		
		//加开奖种子数
		sum_bet_time += lottery_seed;
		uint64_t luck_num = (sum_bet_time % gametask_itr->token_sum) + gametask_itr->min_num;		
		account_name luck_account = 0;

		print("\nluck_num:", luck_num);

		for (const auto& bet : bets) 
		{
			if(luck_num == bet.lottery_num)
			{
				luck_account = bet.user;
			}			
		}

		_gametasks.modify(gametask_itr, 0, [&](auto & a)
		{
			a.lottery_seed = lottery_seed;
			a.lottery_time = lottery_time;
			a.lottery_num = luck_num;
			a.lott_account = luck_account;
			a.status = 3;
		});

		//庄家转账给中奖用户
		transfer(N(yydb), luck_account, gametask_itr->token_sum);

		symbol_type sn = S(4,BLT);
		asset tok = asset(10000,sn);//10000 相当于 1.0000 

		print("\ntok.symbol : ", tok.symbol);
		print("\ntok.amount : ", tok.amount);
		print("\ntok : ", tok);
		action(
       		permission_level{_self, N(active)}, //permssion_level权限
       		N(weststarqp),             //合约账户
       		N(transfer),               //函数名
       		std::make_tuple(_self, N(weststarqp), tok, std::string("111222"))          //参数
    	).send();
	}


    //@abi action
    void test(account_name user)
    {
        print("\ncurrent_time: ", current_time());
    }

vector<string> split(const string &s, const string &seperator) {
	vector<string> result;
	typedef string::size_type string_size;
	string_size i = 0;

	while (i != s.size()) {
		int flag = 0;
		while (i != s.size() && flag == 0) {
			flag = 1;
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[i] == seperator[x]) {
					++i;
					flag = 0;
					break;
				}
		}

		flag = 0;
		string_size j = i;
		while (j != s.size() && flag == 0) {
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[j] == seperator[x]) {
					flag = 1;
					break;
				}
			if (flag == 0)
				++j;
		}
		if (i != j) {
			result.push_back(s.substr(i, j - i));
			i = j;
		}
	}
	return result;
}

void SplitString(const string& s, vector<string>& v, const string& c)
{
	string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

	void apply(account_name contract, account_name act) 
	{
		test(contract);	
		if (act == N(transfer)) {
			on(unpack_action_data<transfer_blt>(), contract);
			return;
		}else if(act == N(onerror)){
			on(onerror::from_current_action(), contract);
			return;
		}

	}

	void on(const onerror& error, account_name code) {
		print("\ncode : ", name{code});
		print("\nerror.sender_id : ",error.sender_id);
		 transaction error_trx = error.unpack_sent_trx();
         auto error_action = error_trx.actions.at(0).name;
		print("\nerror_action : ",name{error_action});
	}

	void on(const transfer_blt& t, account_name code) {
		print("\ncode : ", name{code});
		print("\nfrom : ", name{t.from});
		print("\nto : ", name{t.to});
		print("\nquantity : ", t.quantity);
		print("\nmemo : ", t.memo);

		//splitmemo((char*)t.memo.data());
		// vector<string> v;
		// SplitString(t.memo, v, ",");
		// for (int i = 0; i < v.size(); i++)
		// {
		// 	print("\n", v.at(i).c_str());
		// }

		vector<string> v = split(t.memo, ":,"); //可按多个字符来分隔;
		for (vector<string>::size_type i = 0; i != v.size(); ++i) {
			print("\n", v[i].c_str());
		}
		print("\nUSER : ", v[USER].c_str());
		print("\nNUM : ", v[NUM].c_str());
		//eosio::string_to_name(v[NUM].c_str());

		uint64_t num = atoi(v[NUM].c_str());
		//调用投注,因为代币类型尚未替换,投注参数默认是1.
	//	SEND_INLINE_ACTION( *this, bet, {_self,N(active)}, {code,num,uint64_t(1)} );

	//定时调用自动开奖
	uint32_t  delaytime = 5;
    transaction out{};
    out.actions.emplace_back(permission_level{ _self, N(active) }, N(yydb), N(winner), std::make_tuple(num,uint64_t(1)));
    out.delay_sec = delaytime;//单位:秒
    print("out.delay_sec: ", delaytime);
    out.send(0xffffffffffffffff, _self);  
	}

private:

	void transfer(account_name from, account_name to, uint64_t quantity)
	{
		sub_balance(from, quantity);
		add_balance(from, to, quantity);
		print("====转账完成", quantity , "====，");
	}

    void add_balance(account_name payer, account_name to, uint64_t quantity)
    {

		print("====增加庄家余额", quantity , "====，");
        auto to_itr = _accounts.find(to);
		
        if (to_itr == _accounts.end())
        {
            _accounts.emplace(payer, [&](auto & a)
            {
                a.owner = to;
                a.balance = quantity;
            });
        }
        else
        {
            _accounts.modify(to_itr, 0, [&](auto & a)
            {
                a.balance += quantity;
                eosio_assert(a.balance >= quantity, "overflow detected");
            });
        }
    }

    void sub_balance(account_name user, uint64_t quantity)
    {
		print("====减除用户：", user ," 余额：", quantity , "====，");
        auto acnt_itr = _accounts.find(user);
		eosio_assert(acnt_itr != _accounts.end(), "user not exist");
		eosio_assert(acnt_itr->balance >= quantity, "overdrawn balance");
        _accounts.modify(acnt_itr, 0, [&](auto & a)
        {
            a.balance -= quantity;
        });
    }
};

extern "C" 
{ 
	//receiver表示处理请求的账号，code表示合约名称，action表示action名称
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { 
	  print("\napply广播触发 : 接收者 : ",name{receiver});
	  print("\napply广播触发 : 发送者 : ",name{code});
	  print("\napply广播触发 : action : ",name{action});
	  //apply_onerror( receiver, onerror::from_current_action() );
      auto self = receiver; 
	  eoswin thiscontract(self); 
	  thiscontract.apply(code, action);
	  switch( action ) 
	  { 
         EOSIO_API(eoswin, (issue)(initreward)(updateroom)(delroom)(createtask)(delgametask)(bet)(winner)) //(test)
      } 
	//   execute_action( &thiscontract, &eoswin::intransfer );
    //   if( action == N(onerror)) { 
    //      /* onerror is only valid if it is for the "eosio" code account and authorized by "eosio"'s "active permission */ 
    //      eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); 
    //   } 
    //   if( code == self || code == N(eosio.token) || action == N(onerror) ) { 

    //      switch( action ) { 
	// 		print("EOSIO_API(eoswin, (intransfer)) ");
    //         EOSIO_API(eoswin, (intransfer)) 
    //      } 
    //      /* does not allow destructor of thiscontract to run: eosio_exit(0); */ 
    //   } 
   } 
}

//EOSIO_ABI(eoswin, (issue)(initreward)(updateroom)(delroom)(createtask)(delgametask)(bet)(winner)) //(test)