#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <xmlapi.h>
#include <iostream>
#include "simulate.h"
#include "ines.h"
#include <time.h>
#include <signal.h>
using namespace std;
struct timeval tv;
struct timeval last_tv;
void get_stat();
map<int, fd_info> fd_map;
map<string, int> key_fd_map;
map<int, string> fd_key_map;
map<string, agent_state> agents_state;
void print_usage()
{
	cout<<"ines start [-p offset] [-c config_file_location]"<<endl;
	cout<<"parameters:"<<endl;
	cout<<"offset is an integer indicating the offset between the first agentID in the config file and the actual first agentID here"<<endl;
	cout<<"config_file_location is the absolute config file location you specified.('~/cin/etc/config.xml' by default)"<<endl;
}
bool debug=false;
stat_info_t stat_info;
int epoll_fd;
simu_info_t simu_info;
CLog* log;
string last_key="null";
vector<fd_info>error_sock;
map<string,int>key_retrial;
map<string,Call_num>agents_call;
map<int,int>sock_port;
int eventcount;
bool started;
int temp_count;
void reconnect(int fd, fd_info fdInfo);
void delete_fd(int fd)
{
	fd_map.erase(fd_map.find(fd));
	
}
void log_call_num()
{
	map<string,Call_num>::iterator iter;
	log->LOG(INFO,"%15s %15s %15s", "CorrelationKey","Total Call Num","Success Call Num");
	log->LOG(INFO,"%15s %15d %15d", "Total Agents",stat_info.total_call, stat_info.success_call);
	for(iter=agents_call.begin();iter!=agents_call.end();iter++)
	{
		log->LOG(INFO,"%15s %15d %15d",(iter->first).c_str(), (iter->second).total_call_num,(iter->second).success_call_num);
		
	}
}
int create_nonblock_tcp(string ip, int port)
{
   stat_info.attempt_connect++;
   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if(sockfd < 0)
   {
      return -1;
   }
   
   
   int flag = fcntl(sockfd, F_GETFL, 0);
   flag = flag|O_NONBLOCK;

   if(fcntl(sockfd, F_SETFL, flag) < 0)
      return -1;

   return sockfd;

}

int create_nonblock_connect(int sockfd,string ip, int port)
{
    sockaddr_in server;
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip.c_str());   
        
    return connect(sockfd, (sockaddr*)&server, sizeof(server));
}

int set_config(string config_file_location,int pianyi)
{
	if(config_file_location.size()==0)
	{

		config_file_location = string(getenv("CINDIR"));
		config_file_location = config_file_location + "/etc/config.xml";
	}
	printf("%s",config_file_location.c_str());
	FILE* fd = fopen(config_file_location.c_str(), "r");
		  
	if(fd==NULL)
    	    log->LOG(ERROR,"Please verify the existence of %s",config_file_location.c_str());
	fseek(fd, 0, SEEK_END);
	long size = ftell(fd);
	rewind(fd);
	char* contents = (char*) malloc(sizeof(char)*size);

	fread(contents, 1, size, fd);

	TXMLHandle root = xmlGetTree(contents);

  	char ip[100];
  	if(xmlGetValue(root, "config/ip", ip)!=0)
  	{
   		  log->LOG(ERROR, "%s", "Cannot get the config ip ");
    		return -1;
  	}
  	simu_info.ip = string(ip);
        log->LOG(INFO, "ip: %s", simu_info.ip.c_str());
		
  	char port[100];
  	if(xmlGetValue(root,"config/port",port)!=0)
 	{
    		log->LOG(ERROR, "%s", "Cannot get the config port ");
    		return -1;
  	}
	simu_info.port = atoi(port);
 	log->LOG(INFO, "port: %d", simu_info.port);
  	
        char agent_num[100];
	if(xmlGetValue(root,"config/agent_num",agent_num)!=0)
	{
		  log->LOG(ERROR, "%s", "Cannot get the config agent_num");
		  return -1;
	}
	simu_info.agent_num=atoi(agent_num);
	log->LOG(INFO, "agent_num: %d", simu_info.agent_num);
		
        char vccID[100];
  	if(xmlGetValue(root,"config/vccID",vccID)!=0)
  	{
		
    		log->LOG(WARNING, "%s", "Cannot get the config vccID , it will set to be 8888 by default");
    		simu_info.vccID="888888"; 
  	}
  	else
    		simu_info.vccID = string(vccID);
	log->LOG(DETAIL, "vccID:  %s", simu_info.vccID.c_str());
  
  	char agentID[100] = "0000108888888500";
  	if(xmlGetValue(root,"config/agentID",agentID)!=0)
  	{
    		log->LOG(WARNING, "%s", "Cannot get the config agentID , it will set to be 0000108888888500 by default");
    		
  	}	
  	
    sprintf(agentID,"%016lld",atoll(agentID) + pianyi);
    simu_info.agentID = string(agentID);

	log->LOG(DETAIL, "agentID: %s", simu_info.agentID.c_str());
		
    char deviceID[100];
	if(xmlGetValue(root,"config/deviceID", deviceID)!=0)
	  {
		    log->LOG(WARNING, "%s", "Cannot get the config deviceID, it will set to be the same as deviceID");
		    simu_info.deviceID = simu_info.agentID;
	  }
	  else
		    simu_info.deviceID = string(deviceID);
	  log->LOG(DETAIL, "deviceID: %s",simu_info.deviceID.c_str());
  	
    char passwd[100];
	  if(xmlGetValue(root,"config/passwd", passwd)!=0)
	  {
		    log->LOG(WARNING, "%s", "Cannot get the config passwd, it will set to be 111111 by default ");
		    simu_info.passwd="111111";
	  }
	  else
		    simu_info.passwd = string(passwd);
	  log->LOG(DETAIL, "passwd: %s", simu_info.passwd.c_str());
    
    free(contents);
  	return 0;
}

string find_key(int fd)
{
	map<int, string>::iterator iter;
	iter = fd_key_map.find(fd);
	if(iter==fd_key_map.end())
		return "error";
	return iter->second;

}

int find_fd(string correlationKey)
{
	map<string, int>::iterator iter;
	iter = key_fd_map.find(correlationKey);
	if(iter == key_fd_map.end())
		return -1;
	return iter->second;
}

int send_by_key(string correlationKey, string packet)
{
   int fd = find_fd(correlationKey);
   if(fd < 0)
   {
      log->LOG(ERROR, "%s", "The fd corresponds to the key does not exist");
      return -1;
   }
   map<int,fd_info >::iterator iter;
   iter = fd_map.find(fd);
   if(iter==fd_map.end())
   {
      log->LOG(ERROR, "%s", "Cannot find the fd in the message queue");
      return -1;
   }
   (iter->second).message_to_send.push(packet);
   return 0;
}

int reply_event(string correlationKey, string packet)
{
	  return send_by_key(correlationKey, packet); 
}

int request_setIdle(string correlationKey, string packet)
{
	  return send_by_key(correlationKey, packet);
}
int response_heartBeat(string correlationKey,string packet)
{
	return send_by_key(correlationKey,packet);
}
int request_signOut(string correlationKey, string packet)
{
	return send_by_key(correlationKey, packet);
}
int request_signIn(string correlationKey, string ip, int port, string packet)
{
    int sockfd = create_nonblock_tcp(ip, port);
    int ret = create_nonblock_connect(sockfd,ip,port);
    if(debug==true)
    	log->LOG(INFO,"add sock %d for %s to sign in", sockfd, correlationKey.c_str());    
    if(ret < 0)
    {
   		if(errno!=EINPROGRESS)
   		{
   			return -1;
   		}
   		
    }
    key_fd_map[correlationKey] = sockfd;
    fd_key_map[sockfd] = correlationKey;

    fd_info cur_fd_info;
    cur_fd_info.in_set = false;
    cur_fd_info.type = Signed_in;
    cur_fd_info.message_to_send.push(packet);
    cur_fd_info.correlationKey = correlationKey;
    fd_map[sockfd]=cur_fd_info;
    
	return 0;	

}

int request_initial(string correlationKey, string packet)
{
   usleep(60000); 
   int sockfd = create_nonblock_tcp(simu_info.ip, simu_info.port);
   if(sockfd < 0)
      log->LOG(ERROR, "%s", "create nonblock tcp error ");
   int ret = create_nonblock_connect(sockfd,simu_info.ip, simu_info.port);
   while(ret < 0)
   {  
   		if( errno == EINPROGRESS)
   		{
   			break;
   		}
   		else
   			return -1;
   }
   if(debug==true)
   	log->LOG(INFO, "add sock fd %d for %s to ininitial", sockfd, correlationKey.c_str());
   fd_info cur_fd_info;
   
   cur_fd_info.in_set = false;
   cur_fd_info.type = Initial;
   cur_fd_info.correlationKey = correlationKey;
   cur_fd_info.message_to_send.push(packet);
   fd_map[sockfd]=cur_fd_info;
   key_fd_map[correlationKey]=sockfd;
   fd_key_map[sockfd]=correlationKey;
   return 0;
}
void reconnect(int fd, fd_info fdInfo)
{
	if(key_retrial[fdInfo.correlationKey]<=3)
		request_initial(fdInfo.correlationKey,fdInfo.message_to_send.front());
	else
	{
		log->LOG(ERROR,"The correlationKey %s failed to initial finally", fdInfo.correlationKey.c_str());
	}
}

char* specialFunction(int argc, char* argv[])
{
   string correlationKey;
   string packet=argv[argc];
   correlationKey = argv[1];
   packet=packet.substr(packet.find("|delimeter|")+12);
   if(debug==true)
   	log->LOG(INFO,"function cmd:%s key:%s packet:%s",argv[0],correlationKey.c_str(), packet.c_str());
   if(strcmp(argv[0],"Initialize")==0)
   {
	 	 stat_info.success_connect++;
         int ret = request_initial(correlationKey, packet);
         if(ret < 0)
         {
            log->LOG(ERROR, "Failed to request initial of correlationKey : %s", correlationKey.c_str());
         }
   }   
   else if(strcmp(argv[0],"Sign_in")==0)
   {
      stat_info.success_connect++;
      int ret = request_signIn(correlationKey, string(argv[2]), atoi(argv[3]),packet);
      if(ret < 0)
      {
         log->LOG(ERROR, "Failed to request signIn of correlationKey: %s", correlationKey.c_str());
      }
   }
   else if(strcmp(argv[0],"Sign_out")==0)
   {
	log->LOG(INFO,"Signing out %s",correlationKey.c_str());
	int ret = request_signOut(correlationKey, packet);
	if(ret < 0)
	{
		log->LOG(ERROR, "Failed to request signOut of correlationKey: %s", correlationKey.c_str());
	}
	
   }
   else if(strcmp(argv[0],"Set_idle")==0)
   {

      int ret = request_setIdle(correlationKey, packet);
      if(ret < 0)
      {
         log->LOG(ERROR, "Failed to request setIdle of correlationKey: %s", correlationKey.c_str());
      }      
   }
   else if(strcmp(argv[0],"HeartBeat")==0)
   {
	log->LOG(INFO,"Enter Heartbeat correlionKey:%s, packet:%s",correlationKey.c_str(),packet.c_str());
	int ret = response_heartBeat(correlationKey,packet);
	if(ret<0)
	{
		log->LOG(ERROR,"Failed to response heartBeat of correlationKey: %s",correlationKey.c_str());
	}
   }
   else if(strcmp(argv[0],"Reply_event")==0)
   {
      /*int ret = reply_event(string(argv[1]), string(argv[2]));
      if(ret < 0)
      {
         log->LOG(ERROR, "%s", "Failed to reply_event of correlationKey: %s", argv[1]);       
      }*/
         
   }
   else if(strcmp(argv[0],"ERROR")==0)
   {
	log->LOG(ERROR,"%s got wrong when hxmlparsing %s",correlationKey.c_str(),packet.c_str());
   }
   else if(strcmp(argv[0],"State_report")==0)
   {
      if(atoi(packet.c_str())==3)
	log->LOG(WARNING,"Agent %s is forced out", correlationKey.c_str());
      agents_state[correlationKey] = agent_state(atoi(packet.c_str()));
      get_stat();
      if(stat_info.idle==simu_info.agent_num)
	log->LOG(INFO,"%s","All agents has been set idle successfully");
      //log->LOG(INFO, "agent %s report its state %s", correlationKey.c_str(), state_table[atoi(packet.c_str())].c_str());       
   }
  
   else if(strcmp(argv[0],"Total_call_add")==0)
   {
//	log->LOG(INFO,"Receive total call %s","");	
	stat_info.total_call++;
	if(agents_call.find(correlationKey)!=agents_call.end())
		agents_call[correlationKey].total_call_num++;
	else
	{
		agents_call[correlationKey].total_call_num = 1;
		agents_call[correlationKey].success_call_num = 0;
	}
   }
   else if(strcmp(argv[0],"Success_call_add")==0)
   {
//	if(debug==true)
//	{
//		log->LOG(INFO,"Receive successful call %d times from %s",stat_info.success_call+1,correlationKey.c_str());
//	}
	gettimeofday(&tv,NULL);
	if(debug==true)
		log->LOG(INFO,"success_call_add %d %d",tv.tv_sec,tv.tv_usec);
	
        if(last_key != correlationKey)
	{
		stat_info.success_call++;
		agents_call[correlationKey].success_call_num++;
		last_key = correlationKey;
	}
	
   }
   
   return "return";
}


void inesInitialize(char* name)
{
   //int logfd = open("/home/WsxcdE1/cin/simulate.log",'w');
   //close(logfd);
   debug = false;
   started = false; 
   log = new CLog();
   epoll_fd = epoll_create(MAXEPOLL);
   if(epoll_fd<0)
   {
	log->LOG(ERROR, "%s", "The epoll creation failed");
   }
   stat_info.clear_stat(); 

}

void cmd_ines(int argc, char* argv[])
{
    //log->LOG(DETAIL,"cmd_ines: %s", argv[argc]);  		
    if(strcmp(argv[0],"ines")!=0)
    {
      term("This is not an ines command\n");
      return;
    }
    if(strcmp(argv[1],"all_idle")==0)
    {
	if(started==false)
		log->LOG(WARNING,"Please start first! %s","");
	else
	{
        	
      		map<string,int>::iterator iter;
      		char cmd[100]="<acpMessage><cmd>setIdle</cmd></acpMessage>";
     		// log_call_num();      
     		 char temp_key[25];
     		 for(iter = key_fd_map.begin(); iter != key_fd_map.end(); iter++)
     		 {
			
	 		 snprintf(temp_key,sizeof(temp_key),"%s",(iter->first).c_str());
          
         		 backEvent(temp_key,cmd);
	 		 //log->LOG(INFO,"Shutting down %s",temp_key);
         	 //close(iter->second);
          
     		 }
	}
    }
    if(strcmp(argv[1],"start")==0)
    {
      if(started==true)
      {
			log->LOG(WARNING,"%s","The program has been started already");
			return;
      }
      started=true;
      int pianyi=0;
      string config_file_location;
      bool accepted = true;
      if(argc>3)
      {
      	if(strcmp(argv[2],"-p")==0)
      	{
      		pianyi = atoi(argv[3]);
      		if(argc == 6)
      		{
      			if(strcmp(argv[4],"-c")==0)
      			{
      				config_file_location = string(argv[5]);
      			}
      			else
      				accepted = false;
      		}
		else if(argc != 4)
			accepted = false;	

      	}
      	else if(strcmp(argv[2],"-c")==0)
      	{
      		config_file_location = string(argv[3]);
      		if(argc == 6)
      		{
      			if(strcmp(argv[4],"-p")==0)
      			{
      				pianyi = atoi(argv[5]);
      			}
      			else
      				accepted = false;
      		}
		else if(argc != 4)
			accepted = false;
      	}
      	else
      	{
      		accepted = false;
      	}

      }
      if(pianyi<0||pianyi>10000)
		accepted = false;
      if(accepted==false)
      {
      		print_usage();
      		return;
      }
      if(set_config(config_file_location,pianyi)==-1)
      {
	    	
	    	return;
      }
      stat_info.total = simu_info.agent_num;   
      for(int i = 1; i <= simu_info.agent_num; i++ )
      {
         char buf[100];
         char agentID[100];
         sprintf(agentID, "%016lld", atoll(simu_info.agentID.c_str())+i);
	 //log->LOG(DETAIL,"agent id is:%s",agentID);
	 sprintf(buf,"%s %s %s %s",simu_info.vccID.c_str(), agentID, simu_info.passwd.c_str(), agentID);
         //usleep(1000);      
	 invokeServiceLogic(100, buf);
      }

   }
   else if(strcmp(argv[1],"ports")==0)
   {
	struct sockaddr_in local_addr;
	socklen_t len=sizeof(sockaddr_in);
	int sockfd;
	int local_port;
	map<int,string>::iterator iter;
	int ret;
        for(iter=fd_key_map.begin();iter!=fd_key_map.end();iter++)
        {
		bzero(&local_addr,sizeof(local_addr));
		sockfd = iter->first;
		ret  = getsockname(sockfd,(sockaddr*)&local_addr,&len);
    		if(ret==-1)
        		log->LOG(ERROR,"sock %d cannot get the local port",sockfd);
    		else
    		{
        	        local_port = ntohs(local_addr.sin_port);
        		if(debug==true)
                	log->LOG(INFO,"sock %d 's correlationKey is %s local port is %d ",sockfd,(iter->second).c_str(),local_port);
    		}		
        }
   }
   else if(strcmp(argv[1],"dbgon")==0)
   {
	debug = true;
   }
   else if(strcmp(argv[1], "dbgoff")==0)
	debug = false;

   
   else if(strcmp(argv[1], "signout")==0)
   {
      map<string,int>::iterator iter;
      char cmd[100]="<acpMessage><cmd>signOut</cmd></acpMessage>";
     // log_call_num();      
      char temp_key[25];
      for(iter = key_fd_map.begin(); iter != key_fd_map.end(); iter++)
      {
	  snprintf(temp_key,sizeof(temp_key),"%s",(iter->first).c_str());
          
          backEvent(temp_key,cmd);
	  //log->LOG(INFO,"Shutting down %s",temp_key);
          //close(iter->second);
          
      }

      //close(epoll_fd);

   }
   else if(strcmp(argv[1],"showBusy")==0)
   {
	map<string,agent_state>::iterator iter;
	for(iter = agents_state.begin();iter!=agents_state.end();iter++)
	{
		if(iter->second==agent_Busy)
			log->LOG(INFO,"key %s busy",(iter->first).c_str());
	}
   }
   else if(strcmp(argv[1], "shutdown")==0)
   {
      map<string,int>::iterator iter;
      char cmd[100]="<acpMessage><cmd>shutdown</cmd></acpMessage>";
      log_call_num();      
      char temp_key[25];
      for(iter = key_fd_map.begin(); iter != key_fd_map.end(); iter++)
      {
	  snprintf(temp_key,sizeof(temp_key),"%s",(iter->first).c_str());
          
          backEvent(temp_key,cmd);
	  //log->LOG(INFO,"Shutting down %s",temp_key);
          close(iter->second);
          
      }

      close(epoll_fd);

   }
   
   else if(strcmp(argv[1], "agents")==0)
   {
   	get_stat();
	log->LOG(DETAIL, "Agents state:(signed_in,idle, busy) %d   %d   %d",stat_info.signed_in, stat_info.idle, stat_info.busy);
   }
   else if(strcmp(argv[1], "calls")==0)
   {
	
	log->LOG(DETAIL, "Agents call statistics(total,success) %d  %d", stat_info.total_call,stat_info.success_call);
   }
   else if(strcmp(argv[1],"clearCallState")==0)
   {
	stat_info.clear_call_num();
   }
   
}
void get_stat()
{
    stat_info.clear_agent_state();
    map<string, agent_state>::iterator iter;
    for(iter=agents_state.begin(); iter!=agents_state.end(); iter++)
    {
        if(iter->second == agent_Signed_out)
          stat_info.signed_out++;
        else
        {
          stat_info.signed_in++;
          if(iter->second == agent_Busy)
            stat_info.busy++;
          else if(iter->second == agent_Idle)
            stat_info.idle++;
        }
    }
   
}
void checkINESEvent()
{
/*
   if(eventcount==1000)
   {
	last_tv.tv_sec = tv.tv_sec;
	last_tv.tv_usec	= tv.tv_usec;
	if(gettimeofday(&tv,NULL)==0)
	{
		int timecount=(tv.tv_sec-last_tv.tv_sec)*1000+(tv.tv_usec-last_tv.tv_usec)/1000;
		if(timecount>5000)		
			log->LOG(INFO,"past %d msec ",timecount);
		last_tv.tv_sec = tv.tv_sec;
		last_tv.tv_usec = tv.tv_usec;
	}
	eventcount=0;
   }
   else 
   {
	eventcount++;
   }*/
   map<int,fd_info>::iterator it;
   epoll_event event, events[MAXEPOLL];
  
   map<int, fd_info>::iterator iter;
   
   for(iter = fd_map.begin(); iter != fd_map.end(); iter++)
   {
	    if((iter->second).in_set==false)
	    {
		    if(debug==true)
		    	log->LOG(INFO, "Add new sock %d", iter->first);
   		    event.data.fd = iter->first;
        	    event.events = EPOLLIN|EPOLLOUT|EPOLLRDHUP;
        	    int ret=epoll_ctl(epoll_fd, EPOLL_CTL_ADD, iter->first, &event);
		    if(ret==0)
		    	(iter->second).in_set = true;
		    else
	            {
			log->LOG(ERROR,"sock %d error %s when add to epoll",iter->first,strerror(errno));
		    }
	    }

   }
      
   int active_n = epoll_wait(epoll_fd, events, MAXEPOLL, 0);
   
   char buf[30000];
   int status;
   string cur_key;
   for(int i = 0; i < active_n; i++)
   {
      epoll_event ev = events[i];
      if(ev.events & EPOLLERR)
      {
         log->LOG(ERROR,"epoll error, fd: %d", ev.data.fd);
         log->LOG(ERROR,"This error fd type is %d ,correlation key is %s",fd_map[ev.data.fd].type,fd_map[ev.data.fd].correlationKey.c_str());
	 string errmsg;
	 string temp;
	 while( fd_map[ev.data.fd].message_to_send.empty() == false)
	 {
		 
		temp = fd_map[ev.data.fd].message_to_send.front();
		fd_map[ev.data.fd].message_to_send.pop();
		errmsg = errmsg+temp;
	 }
	 log->LOG(ERROR,"This error fd has %s msg to send", errmsg.c_str());
	 int error=0;
	 socklen_t errlen = sizeof(error);
	 if(getsockopt(ev.data.fd,SOL_SOCKET, SO_ERROR,(void*)&error,&errlen)==0)
	 {		
		log->LOG(ERROR, "ErrOr info is %s",strerror(error));
	 }
	 string cur_key = fd_map[ev.data.fd].correlationKey;
	 error_sock.push_back(fd_map[ev.data.fd]);
	 int epoll_ret = epoll_ctl(epoll_fd,EPOLL_CTL_DEL,ev.data.fd,NULL);
	 //EPOLL_CTL(epoll)
	 if(key_retrial.find(cur_key)==key_retrial.end())
		key_retrial[cur_key]=1;
	 else
		 key_retrial[cur_key]++;
	 //reconnect(ev.data.fd,fd_map[ev.data.fd]);	  
         //log->LOG(INFO,"Reconnecting %s for the %d time", cur_key.c_str(), key_retrial[cur_key]);
	 continue;
      }
      
      if(ev.events & EPOLLOUT)
      {
      	  int sockfd = ev.data.fd;
       	  int val;
       	  socklen_t len = (socklen_t)sizeof(int);
       	  if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR,(char*)&val,&len)==-1)
       	  {
            log->LOG(ERROR, "getsockopt for %d error", sockfd);
		        continue;
       	  }
       	  if(val!=0)
       	  {
            log->LOG(ERROR, "Sock num %d failed to connect", sockfd);
       	  	continue;
       	  }
	  
	  //stat_info.cur_connect++;
          //stat_info.success_connect++;
          map<int, fd_info>::iterator iter;
          iter = fd_map.find(sockfd);
         
          if(iter==fd_map.end())
            continue;
          
          
          while((iter->second).message_to_send.empty() == false)
          {
        
      	    int ret = send(sockfd, (iter->second).message_to_send.front().c_str(), (iter->second).message_to_send.front().size()+1,0);
            stat_info.send_msg++;
	    //usleep(10000);
	    if(debug==true)  
	    	log->LOG(DETAIL , "%s", (iter->second).message_to_send.front().c_str());
      	    (iter->second).message_to_send.pop();
          }
      }
     /* if(ev.events & EPOLLRDHUP)
      {
      	int epoll_ret = epoll_ctl(epoll_fd,EPOLL_CTL_DEL,ev.data.fd,NULL);
		if(epoll_ret!=0)
			log->LOG(ERROR,"%s",strerror(errno));
		close(ev.data.fd);
		delete_fd(ev.data.fd);
		log->LOG(INFO,"Sock %d close,EPOLLRDHUP",ev.data.fd);
	 	
      }*/
      if(ev.events & EPOLLIN)
      {
        int len = recv(ev.data.fd, buf, sizeof(buf), 0);
        if(len == sizeof(buf))
			log->LOG(WARNING,"Sock %d Recieve imcomplete",ev.data.fd);
	 	if(len > 0)
        {
            stat_info.resv_msg++;
            cur_key = find_key(ev.data.fd);
            if(cur_key == "error")
            {
               log->LOG(ERROR, "%s", "The key cannot be found");
            }
            else
      	    {
		//		if(debug==true)
		    string tempstr=string(buf);
		    if(tempstr.find("OnRelease")!=tempstr.npos)
			{
			
			gettimeofday(&tv,NULL);
			//log->LOG(INFO,"Release %d %d ",tv.tv_sec,tv.tv_usec);
			}	
			  // log->LOG(INFO, "Got message from key %d %d chars --- %s",ev.data.fd,len,buf);
	            char c_key[20];
	          	sprintf(c_key,"%s",cur_key.c_str());
	            backEvent(c_key, buf);
	          	if(fd_map[ev.data.fd].type==Initial)
	          	{
					int epoll_ret = epoll_ctl(epoll_fd,EPOLL_CTL_DEL,ev.data.fd,NULL);
	                 
			 delete_fd(ev.data.fd);
	                if(epoll_ret!=0)
	                    log->LOG(ERROR,"%s",strerror(errno));
		       if(debug==true)	
                       log->LOG(INFO, "The sock %d for initial close as wish ", ev.data.fd);
	          		close(ev.data.fd);
		           	stat_info.cur_connect--;
	          	}
      		}
        }
      }
   }
}

void initialVersionInfo(){}
void printINESData(void* p){}
int scfPreSelect(fd_set *readMask,fd_set *writeMask,fd_set *exceptMask,timeval * timer){return 0;}
void scfPostSelect(fd_set *readMask,fd_set *writeMask,fd_set *exceptMask){}
void checkINESEventOnTick(){};





