#include <queue>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <vector>
#include <fcntl.h>
#include "log.h"
#define MAXEPOLL 5000

using namespace std;
string state_table[]={"Start","Initial","Signed_in","Signed_out","Idle","Busy"};
enum fd_type
{
	Initial,Signed_in,Signed_out,Default
};
typedef struct
{
	int total_call_num;
	int success_call_num;
}Call_num;
enum agent_state
{
	agent_Start,agent_Initial,agent_Signed_in,agent_Signed_out,agent_Idle,agent_Busy,
};

typedef struct fd_info
{
	bool in_set;
	queue<string> message_to_send;
	fd_type type;
	string correlationKey;

}fd_info; 

typedef struct 
{
	string ip;
	int port;
	int agent_num;
	string agentID;
	string deviceID;
	string passwd;
	string vccID;
}simu_info_t;

typedef struct
{
	int signed_in;
	int total;
	int idle;
	int signed_out;
	int busy;
	int attempt_connect;
	int success_connect;
	int send_msg;
	int resv_msg;
	int cur_connect;
	int total_call;
	int success_call;
	void clear_stat()
	{
		attempt_connect = 0;
		success_connect = 0;
		total_call = 0;
		success_call = 0; 
		signed_out = 0;
		idle = 0;
		signed_in = 0;
		busy = 0;
	}
	void clear_call_num()
	{
		total_call = 0;
		success_call = 0;
	}
	void clear_agent_state()
	{
		signed_out = 0;
		idle = 0;
		signed_in = 0;
		busy = 0;
	}
}stat_info_t;

