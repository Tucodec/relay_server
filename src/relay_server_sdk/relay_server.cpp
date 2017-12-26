
#include <string>
#include <curl.h>
#include "simple_uv/config.h"
#include "simple_uv/log4z.h"  
#include "relay_server.h"
#include "media_server.h"
#include "signaling_server.h"
using namespace std;
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	std::string *str = (std::string*)stream;
	str->append((char*)ptr, size*nmemb);
	return size * nmemb;
}
// http GET  
CURLcode curl_get_req(const std::string &url, std::string &response)
{
	// init curl  
	CURL *curl = curl_easy_init();
	// res code  
	CURLcode res;
	if (curl)
	{
		// set params  
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url  
														  //curl_easy_setopt(curl, CURLOPT_HTTPGET, "?test=string");
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3); // set transport and time out time  
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		// start req  
		res = curl_easy_perform(curl);
	}
	// release curl  
	curl_easy_cleanup(curl);
	return res;
}

// http POST
CURLcode curl_post_req(const string &url, const string &postParams, string &response)
{
	// init curl  
	CURL *curl = curl_easy_init();
	// res code  
	CURLcode res;
	if (curl)
	{
		// set params  
		curl_easy_setopt(curl, CURLOPT_POST, 1); // post req  
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url  
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParams.c_str()); // params  
																		// start req  
		res = curl_easy_perform(curl);
	}
	// release curl  
	curl_easy_cleanup(curl);
	return res;
}

void post() {
	string postUrlStr = "https://www.baidu.com/s";
	string postParams = "f=8&rsv_bp=1&rsv_idx=1&word=picture&tn=98633779_hao_pg";
	string postResponseStr;
	auto res = curl_post_req(postUrlStr, postParams, postResponseStr);
	if (res != CURLE_OK)
		cerr << "curl_easy_perform() failed: " + string(curl_easy_strerror(res)) << endl;
	else
		cout << postResponseStr << endl;
}
CRelayServer::CRelayServer():
	m_nMasterPort(10000),
	m_nServerPort(12345)
{
	// global init  
	curl_global_init(CURL_GLOBAL_ALL);
	m_pTcpServer = make_shared<CSignalingServer>();
	m_pUdpServer = make_shared<CMediaServer>(m_pTcpServer->GetLoop(),this);
}
CRelayServer::~CRelayServer() {
	// global release  
	curl_global_cleanup();
}

void CRelayServer::Close() {
	m_pTcpServer->Close();
	m_pUdpServer->Close();
}
bool CRelayServer::Start() {
	bool success = m_pUdpServer->Start("0.0.0.0", m_nServerPort);
	success |= m_pTcpServer->Start("0.0.0.0", m_nServerPort);
	LOGFMTI("server bind at %s:%u", "0.0.0.0", m_nServerPort);
	//LOGFMTI("udp bind at %s:%u", "0.0.0.0", m_nServerPort);
	m_pUdpServer->setSignalingServer(m_pTcpServer.get());
	m_pTcpServer->setUdpServer(m_pUdpServer.get());
	m_pTcpServer->SetKeepAlive(1, 5);
	return success;
}

int CRelayServer::Validate(uint32_t uid1, uint32_t sessionId, uint32_t uid2,string *_url){
	string getResponseStr;
	string url = *_url + "?uid1=" + to_string(uid1) + "&sessionId=" + to_string(sessionId) + "&uid2=" + to_string(uid2);
	LOGFMTI("Validate HttpCallback on: %s", url.c_str());
	auto res = curl_get_req(url, getResponseStr);
	if (res != CURLE_OK) {
		LOGFMTF("HttpCallback on curl_easy_perform() failed: %s", string(curl_easy_strerror(res)).c_str());
	}
	else {
		//cout << getResponseStr << endl;
		if (getResponseStr.size() && getResponseStr[0] == '1')
			return 0;
	}
	return 1;
}
int CRelayServer::ValidateOnLogin(uint32_t uid1, uint32_t sessionId, std::string *_url) {
	string getResponseStr;
	string url = *_url + "?uid=" + to_string(uid1) + "&sessionId=" + to_string(sessionId);
	LOGFMTI("Validate Login HttpCallback on: %s", url.c_str());
	auto res = curl_get_req(url, getResponseStr);
	if (res != CURLE_OK) {
		LOGFMTF("HttpCallback on curl_easy_perform() failed: %s", string(curl_easy_strerror(res)).c_str());
	}
	else {
		//cout << getResponseStr << endl;
		if (getResponseStr.size() && getResponseStr[0] == '1')
			return 0;
	}
	return 1;
}

void CRelayServer::LoadConfiguration() {
	const char ConfigFile[] = "slave.config";
	Config configSettings;
	if (!configSettings.FileExist(ConfigFile)) {
		LOGFMTI("config file:\"%s\" not exist", ConfigFile);
		return;
	}
	configSettings.ReadFile(ConfigFile);
	m_nServerPort = configSettings.Read("slave_port", this->m_nServerPort);
	m_strServerIP = configSettings.Read("slave_ip", this->m_strServerIP);
	m_nMasterPort = configSettings.Read("master_port", m_nMasterPort);
	m_strMasterIP = configSettings.Read("master_ip", this->m_strMasterIP);
	m_strValidateUrl = configSettings.Read("http_validate", this->m_strValidateUrl);
	m_strValidateUrlOnLogin = configSettings.Read("http_validate_on_login", this->m_strValidateUrlOnLogin);
	m_pUdpServer->setValidUrl(m_strValidateUrl);
	m_pUdpServer->setLoginValidUrl(m_strValidateUrlOnLogin);
	m_pUdpServer->setMasterIPAndPort(m_strMasterIP, m_nMasterPort);
	m_pUdpServer->setServerIPAndPort(m_strServerIP, m_nServerPort);
}