#progma c
void initialize_packet();
void sign_in(TString ip, TInteger port);
void set_idle();
void set_busy();
void initial();
void answer_request();
void answer_success();
void force_out();
void send_state_msg();
void send_heartBeat();
void send_call_attempt_msg();
void send_call_success_msg();
void sign_out();
void send_err_msg(TString err_msg);
var TString call_session_id;
var TString agentID;
var TString passwd;
var TString vccID;
var TString header;
var TString body;
var TInteger fshutdown;
var TInteger agent_state;
var TString timeStamp;
var TInteger calling;
void main(TCommandLine cmd)
{
	calling = 0;
	fshutdown=-1;
	agent_state = 0;
	vccID = cmd.argv(0);
	agentID = cmd.argv(1);
	passwd = cmd.argv(2);
			
	var TInteger FSM;
	FSM = getfsmid();
	CorrelationKey = sprintf('%04d', FSM);

	//print(sprintf('***The SL %s starts ,vccID:%s agentID:%s passwd:%s ***\n', CorrelationKey,vccID,agentID,passwd));

	initialize_packet();
	initial();
        while(fshutdown==-1)
	{
	//	print(sprintf('***SL %s still alive ***',CorrelationKey));
		sleep(1);
	}
        on(serviceEvent, TCommandLine backevent)
        {
                var TInteger err = 0;
	//	print(sprintf('***The SL  %s got message is %s***\n', CorrelationKey, backevent.argv()));
		var TString xmlstr = strstr(backevent.argv(),'<acp');        
        var TString errCode;
		var TXMLTree root;
		try
		{		
			root = hxmlparse(xmlstr);
		}
		catch()
		{
			err=1;
			send_err_msg(backevent.argv());
			print(sprintf('***!!! ERROR %s',xmlstr));
		}
		if(err==0)
		{
		var TString type, name;
		type = hxmlgetattr(root, 'acpMessage/body/type' , 'null');
		if(type != 'null')
		{
			
			name = hxmlgetattr(root, 'acpMessage/body/name', 'null');
	//		print(sprintf('***The SL %s got type %s, name %s***', CorrelationKey,type, name));
			switch(type)
			{
				case 'response':
					if(name == 'Initial')
					{
						
						var TString ip = hxmlgetattr(root,'acpMessage/body/parameter/ip', 'null');
						var TInteger port = atoi(hxmlgetattr(root,'acpMessage/body/parameter/port', '0'));
						sign_in(ip, port);
						agent_state = 1;
						send_state_msg();
							
					}
					else if(name == 'SignIn')
					{
						agent_state = 2;
						timeStamp = hxml(root,'acpMessage/header/timeStamp', 'null');
					
						send_state_msg();
						set_idle();
					}
					else if(name == 'SetIdle')
					{
						
						errCode = hxmlgetattr(root,'acpMessage/body/cause/code', 'null');
						agent_state = 4;
						var TString temp = hxmlgetattr(root, 'acpMessage/header/timeStamp', 'null');
						if(timeStamp == '')
						{
							if(temp != 'null')
								timeStamp = temp;
						}
						send_state_msg();
					}
					else if(name == 'SignOut')
					{
						timeStamp = hxml(root,'acpMessage/header/timeStamp', 'null');
						agent_state = 3;
						send_state_msg();
					}
					break;
				case 'request':
					if(name == 'heartBeat')
					{
						
						//sessionID = hxml(root,'acpMessage/header/sessionID', 'null');
						send_heartBeat();
					}
					break;
				case 'event':
					if(name == 'OnAnswerRequest')
					{
						
						agent_state = 5;
						call_session_id = hxml(root,'acpMessage/header/sessionID', 'null');
						answer_request();
						send_state_msg();
						send_call_attempt_msg();
					}
					else if(name == 'OnForceOut')
					{
						agent_state = 3 ;
						force_out();
						send_state_msg();
					}
					else if(name == 'OnBeginRecordSuccess')	
					{
						
					}
					else if(name == 'OnStopRecordSuccess')
					{	
						
					}
					else if(name == 'OnAnswerSuccess')
					{
						calling = 1;
						answer_success();
						
					}
					else if(name == 'OnReleaseSuccess')
					{

						set_idle();
						if(calling == 1)
						{
						//fputs('log_simu',gettimeofday());	
							send_call_success_msg();
							print('***Got release message***\n');
						//	set_idle();
							calling = 0;
						}
					}
					break;
			}	
		}
		else
		{
			type = hxml(root, 'acpMessage/cmd' , 'null');
			if(type!='null')
			{
				if(type == 'shutdown')
				{
						fshutdown = 0;
						//print('***SL fshutdown = 0***');
				}
				if(type == 'setIdle')
				{
					set_idle();
				}
				if(type == 'signOut')
				{
					sign_out();
					break;
				}
				
			}
		}
		delete(root);
		delete(backevent);
	     }
	}	
	
}
void send_err_msg(TString err_msg)
{
	var TString cmd = sprintf('ERROR %s |delimeter| %s',CorrelationKey,err_msg);
	function(cmd);
}
void answer_request()
{
}

void answer_success()
{
}

extern var TString header;
extern var TString body;
void initialize_packet()
{
	header='<header><sessionID>#_sessionID</sessionID><serialID>#_serialID</serialID><serviceID>#_serviceID</serviceID><timeStamp>#_timeStamp</timeStamp></header>';
	body='<body type="#_type" name="#_name"><agent vccID="#_vccID" agentID="#_agentID" deviceID="#_deviceID"/>#_parameter</body>';

}

extern var TString header;
extern var TString body;
void initial()
{
 
	var TString curpacket='1000 off Initial <acpMessage ver="2.0.0"><header ><sessionID>0</sessionID></header><body type="request" name="Initial"><parameter vdcode=""/></body></acpMessage>';
	var TString msg=sprintf('%s %s %s %s', 'Initialize ',CorrelationKey,'|delimeter|', curpacket);
//	print(sprintf('***The SL %s initial %s***\n', CorrelationKey, msg));
	function(msg);
}

extern var TString call_session_id;
void send_heartBeat()
{
	//var TString sessionID = sprintf('888888-000002%s', substr(agentID,6));
	var TString curpacket=sprintf('1000 off heartBeat <acpMessage ver="2.0.0"><header><sessionID>%s</sessionID></header><body type="response" name="heartBeat"></acpMessage>' ,call_session_id);
	var TString msg=sprintf('%s %s %s %s', 'HeartBeat',CorrelationKey,'|delimeter|',curpacket);
//	print(sprintf('***The SL %s response %s ***\n',CorrelationKey,msg));
	function(msg);
}

extern var TString header;
extern var TString body;
extern var TString passwd;
extern var TString agentID;
extern var TString vccID;
void sign_in(TString ip, TInteger port)
{
	
	var TString sessionID = sprintf('888888-000002%s', substr(agentID,6));
	var TString deviceID = substr(sessionID,7);
        var TString cur_header;
	header = strreplace(header,'#_sessionID',sessionID);
	cur_header=strreplace(header,'#_serialID', '');
	cur_header=strreplace(cur_header,'#_serviceID', '');
	cur_header=strreplace(cur_header,'#_timeStamp', '');
	body=strreplace(body,'#_type', 'request');
	body=strreplace(body,'#_vccID',vccID);
	body=strreplace(body,'#_agentID',agentID);
	body=strreplace(body,'#_deviceID',deviceID);
	var TString cur_body=strreplace(body,'#_name', 'SignIn');
	var TString parameter=sprintf('<parameter  deviceID="%s"   passwd="%s" ready="0" taskId=""/>',deviceID, passwd);
	cur_body=strreplace(cur_body,'#_parameter',parameter);
	var TString msg = sprintf('1000 off SignIn <acpMessage ver="2.0.0"> %s %s </acpMessage>',cur_header,cur_body);
	var TString cmd;
	var TString delimeter = '|delimeter|';
	cmd = sprintf('%s %s %s %d %s %s', 'Sign_in' ,CorrelationKey, ip , port, delimeter, msg);
//	print(sprintf('***The SL %s signin %s***\n', CorrelationKey, cmd));
	function(cmd);	
}

extern var TString header;
extern var TString body;
extern var TString timeStamp;
void sign_out()
{
	var TString cur_header = header;
	cur_header = strreplace(cur_header,'#_timeStamp', timeStamp);
	cur_header = strreplace(cur_header,'#_serialID', '');
	cur_header = strreplace(cur_header,'#_serviceID', '');
	var TString cur_body = body;
	cur_body = strreplace(cur_body, '#_type', 'request');
	cur_body = strreplace(cur_body, '#_name', 'SignOut');
	cur_body = strreplace(cur_body, '#_parameter', '<parameter />');
	var TString msg = sprintf('1000 off SignOut <acpMessage ver="2.0.0"> %s %s</acpMessage>', cur_header,cur_body);
	var TString cmd = sprintf('Sign_out %s |delimeter| %s', CorrelationKey, msg);
//	print(sprintf('***The SL %s signout %s***\n', CorrelationKey, cmd));
	function(cmd);
}

extern var TInteger agent_state;
extern var TInteger responsed_num;
void send_state_msg()
{
	var TString msg = sprintf('State_report %s |delimeter| %d ', CorrelationKey, agent_state);
	function(msg);
}


void send_call_attempt_msg()
{
	var TString msg = sprintf('Total_call_add %s |delimeter| somepac', CorrelationKey);
	function(msg);
}

void send_call_success_msg()
{
	var TString msg = sprintf('Success_call_add %s |delimter| somepac', CorrelationKey);
	function(msg);
}

extern var TString timeStamp;
extern var TString header;
extern var TString body;
void set_idle()
{	
	
	var TString cur_header = header;
	cur_header = strreplace(cur_header,'#_timeStamp', timeStamp);
	cur_header = strreplace(cur_header,'#_serialID', '');
	cur_header = strreplace(cur_header,'#_serviceID', '');
	var TString cur_body = body;
	cur_body = strreplace(cur_body,'#_type', 'request');
	cur_body = strreplace(cur_body,'#_name', 'SetIdle');
	cur_body=strreplace(cur_body,'#_parameter', '<parameter />');
	var TString msg=sprintf('1000 off SetIdle <acpMessage ver="2.0.0"> %s %s</acpMessage>', cur_header,cur_body);
	var TString cmd;
	var TString delimeter = '|delimeter|';
	cmd = sprintf('%s %s %s %s' ,'Set_idle', CorrelationKey, delimeter, msg);
	print(sprintf('***The SL %s set_idle %s***\n', CorrelationKey, cmd));
	function(cmd);
	
}

extern var TString timeStamp;
extern var TString header;
extern var TString body;
void set_busy()
{

        var TString cur_header = header;
        cur_header = strreplace(cur_header,'#_timeStamp', timeStamp);
        cur_header = strreplace(cur_header,'#_serialID', '');
        cur_header = strreplace(cur_header,'#_serviceID', '');
        var TString cur_body = body;
        cur_body = strreplace(cur_body,'#_type', 'request');
        cur_body = strreplace(cur_body,'#_name', 'SetIdle');
        cur_body=strreplace(cur_body,'#_parameter', '<parameter />');
        var TString msg=sprintf('1000 off SetIdle <acpMessage ver="2.0.0"> %s %s</acpMessage>', cur_header,cur_body);
        var TString cmd;
        var TString delimeter = '|delimeter|';
        cmd = sprintf('%s %s %s %s' ,'Set_idle', CorrelationKey, delimeter, msg);
        //print(sprintf('***The SL %s signin %s***\n', CorrelationKey, cmd));
        function(cmd);


}
void force_out()
{}



