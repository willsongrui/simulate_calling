#progma c


TInteger parseMessage(TXMLTree XMLTree);
TString parseMessageStr(TString message);
TInteger sendMessage(TString message);

void main(THttpServer httpServer)//TCommandLine cmd)
{
	var THttpContent httpContent;
	var TXMLTree XMLTree;
	var TString msgStr;
	var TString resStr;

	
	var TString msgMsg;
	var TString msgHead;
	var TString msgAgent;
	var TString msgBody;
	var TString msgSend;
	
	
	//接受http消息
	httpContent = new (THttpContent);
	httpContent = httpServer.getcontent();
	msgStr = httpContent.getbuffer();
	print('msgStr = ' + msgStr);
	XMLTree = hxmlparse(msgStr);
	
//	parseMessage(XMLTree);
	msgMsg = parseMessageStr(msgStr);
	
	msgHead =  sprintf('<header><sessionID>%s</sessionID><serialID>%s</serialID><serviceID>%s</serviceID><timeStamp>%s</timeStamp></header>', '*****', '*****', '*****', '*****');
	msgAgent = sprintf('<agent vccID="%s" agentID="%s" deviceID="%s" agentStatus="%s" phoneStatus="%s" master="%s"></agent>', 'xxx', 'xxx', 'xxx', 'xxx', 'xxx', 'xxx');
	msgBody = sprintf('<body type="event" name=" SendWeiboMsg ">%s%s</body>' , msgAgent, msgMsg);
	msgSend = sprintf('<acpMessage ver="2.0.0">%s%s</acpMessage>', msgHead, msgBody);
	
	print(msgSend);
	sendMessage(msgSend);
	
	resStr = sprintf('<response><result>%s</result></response>', 'success');
	httpContent.setbuffer('text/xml', resStr);
	httpServer.sendresponse(httpContent);	
	
}

TInteger parseMessage(TXMLTree XMLTree){
	var TString call_id;
	var TString corp_id;
	var TString account;
	var TString sender;
	var TString receiver;
	var TString contact;
	var TString type;
	var TString m_type;
	var TString m_source;
	var TString m_content;
	
	var TString path_header = 'message-list/header/';
	var TString path_message = 'message-list/message/';

	
	call_id = XMLTree.getvalue(path_header + 'Call-ID', '');
  corp_id = XMLTree.getvalue(path_header + 'corp-id', '');
  account = XMLTree.getvalue(path_header + 'account', '');
  sender = XMLTree.getvalue(path_header + 'sender', '');
  receiver = XMLTree.getvalue(path_header + 'receiver', '');
  contact = XMLTree.getvalue(path_header + 'contact', '');
  type = XMLTree.getvalue(path_header + 'type', '');
  m_type = XMLTree.getvalue(path_message + 'type', '');
  m_source = XMLTree.getvalue(path_message + 'source', '');
  m_content = XMLTree.getvalue(path_message + 'content', '');
  
	print('call_id=' + call_id);
	print('corpId=' + corp_id);
	print('account=' + account);
	print('m_content=' + m_content);
	
	return 0;
}

TString parseMessageStr(TString message){
	
	var TString result;
	result = strstr(message, '<message-list>');

	print('result=' + result);
	
	return result;
}

TInteger sendMessage(TString message){
		
	var TINESObj ines;
	var TString ines_message, result;
	
	ines = new(TINESObj);
	ines.init('INESWEIBO');
	
	ines_message = 'invokeservice 1010 ines progma test';
	result = ines.synctoines(ines_message);
	print('------result1: ' + result + '------');
	if(pickword(result, ' ', 5) == 'success')
	{
		result = ines.synctoines(message);
		print('------result2: ' + result + '------');
		//sendtoines(message);
	}
	else
	{
		return -1;
	}
	
	return 0;
}

