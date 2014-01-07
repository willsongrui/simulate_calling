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
var TString agentID;
var TString passwd;
var TString vccID;
var TString header;
var TString body;
var TInteger fshutdown;
var TInteger agent_state;
var TString timeStamp;
void main(TCommandLine cmd)
{
	fshutdown=-1;
	agent_state = 3;
	vccID = cmd.argv(0);
	agentID = cmd.argv(1);
	passwd = cmd.argv(2);
			
	var TInteger FSM;
	FSM = getfsmid();
	CorrelationKey = sprintf('%04d', FSM);

	print(sprintf('***The SL  %s starts ,vccID:%s agentID:%s passwd:%s ***\n', CorrelationKey,vccID,agentID,passwd));

	initialize_packet();
	initial();
        while(fshutdown==-1)
	{
		print(sprintf('***The SL %s is alive', CorrelationKey));
		sleep(2);
	}
	     
        on(serviceEvent, TCommandLine backevent)
        {
                
		print(sprintf('***The SL  %s got message is %s***\n', CorrelationKey, backevent.argv()));
		var TString xmlstr = strstr(backevent.argv(),'<acp');        
		var TXMLTree root = hxmlparse(xmlstr);
		var TString type, name;
		type = hxmlgetattr(root, 'acpMessage/body/type' , 'null');
		if(type!='null')
		{
			name = hxmlgetattr(root, 'acpMessage/body/name', 'null');
			switch(type)
			{
				case 'response':
					if(name == 'Initial')
					{
						
						var TString ip = hxmlgetattr(root,'acpMessage/body/parameter/ip', 'null');
						var TInteger port = atoi(hxmlgetattr(root,'acpMessage/body/parameter/port','0'));
						sign_in(ip, port);	
					}
					else if(name == 'SignIn')
					{
						agent_state=2;
						timeStamp = hxml(root,'acpMessage/header/timeStamp', 'null');
					
						send_state_msg();						
						set_idle();
					}
					else if(name == 'SetIdle')
					{
						agent_state=0;
						
						var TString temp = hxmlgetattr(root, 'acpMessage/header/timeStamp', 'null');
						if(timeStamp == '')
						{
							if(temp != 'null')
								timeStamp = temp;
						}
						send_state_msg();
					}
					break;

				case 'event':
					if(name == 'OnAnswerRequest')
						answer_request();
					else if(name == 'OnForceOut')
					{
						agent_state=3;
						send_state_msg();	
						force_out();
					}	
					else if(name == 'OnAnswerSuccess')
						answer_success();
					break;
				default:
					print(sprintf('***The SL %s got unknown cmd***\n', CorrelationKey));
			}	
		}
		else
		{
			type = hxml(root, 'command' , 'null');
			if(type!='null')
			{
				switch(type)
				{
					case 'shutdown':
						fshutdown=0;
						break;
						
				}
			}
		}
	     
	}	
	
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
	print(sprintf('***The SL %s initial %s***\n', CorrelationKey, msg));
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
	print(sprintf('***The SL %s signin %s***\n', CorrelationKey, cmd));
	function(cmd);	
}

extern var TInteger agent_state;
void send_state_msg()
{
	var TString msg = sprintf('State_report %s |delimeter| %d', CorrelationKey, agent_state);
	print(sprintf('***The SL %s report its state %d***\n', CorrelationKey, agent_state));
	function(msg);
}

extern var TString timeStamp;
extern var TString header;
extern var TString body;
void set_idle()
{	
	
	var TString cur_header = header;
	//print(sprintf('***The SL %s timeStamp is: %s', CorrelationKey, timeStamp));
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
        print(sprintf('***The SL %s signin %s***\n', CorrelationKey, cmd));
        function(cmd);


}
void force_out()
{}



