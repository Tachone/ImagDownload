/*下载图片 C++ Winsock 网络编程*/
#define _CRT_SECURE_NO_WARNINGS   //vs 2013用于忽略c语言安全性警告
#include <cstdio>
#include <iostream> 
#include <fstream>
#include <string>
#include <cstring>
#include <regex>
#include <vector>
#include <queue>
#include <algorithm>
#include <winsock2.h>
#include <map>
using namespace std;
#pragma comment(lib, "ws2_32.lib")
char  host[500];
int num = 1;
char othPath[800];
string allHtml;
vector <string> photoUrl;
vector <string> comUrl;
map <string, int> mp; //防止相同网址重复遍历
SOCKET sock;
bool analyUrl(char *url) //仅支持http协议,解析出主机和IP地址
{
	char *pos = strstr(url, "http://");
	if (pos == NULL)
		return false;
	else
		pos += 7;
	sscanf(pos, "%[^/]%s", host, othPath);   //http:// 后一直到/之前的是主机名
	cout << "host: " << host << "   repath:" << othPath << endl;
	return true;
}


void regexGetimage(string &allHtml)  //C++11 正则表达式提取图片url
{
	smatch mat;
	regex pattern("src=\"(.*?\.jpg)\"");
	string::const_iterator start = allHtml.begin();
	string::const_iterator end = allHtml.end();
	while (regex_search(start, end, mat, pattern))
	{
		string msg(mat[1].first, mat[1].second);
		photoUrl.push_back(msg);
		start = mat[0].second;
	}
}

void regexGetcom(string &allHtml) //提取网页中的http://的url
{
	smatch mat;
	//regex pattern("href=\"(.*?\.html)\"");
	regex pattern("href=\"(http://[^\s'\"]+)\"");
	string::const_iterator start = allHtml.begin();
	string::const_iterator end = allHtml.end();
	while (regex_search(start, end, mat, pattern))
	{
		string msg(mat[1].first, mat[1].second);
		comUrl.push_back(msg);
		start = mat[0].second;
	}
}
void preConnect()  //socket进行网络连接
{
	WSADATA wd;
	WSAStartup(MAKEWORD(2, 2), &wd);
    sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cout << "建立socket失败！ 错误码： " << WSAGetLastError() << endl;
		return;
	}
	sockaddr_in sa = { AF_INET };
	int n = bind(sock, (sockaddr*)&sa, sizeof(sa));
	if (n == SOCKET_ERROR)
	{
		cout << "bind函数失败！ 错误码： " << WSAGetLastError() << endl;
		return;
	}
	struct hostent  *p = gethostbyname(host);　
	if (p == NULL)
	{
		cout << "主机无法解析出ip! 错误吗： " << WSAGetLastError() << endl;
		return;
	}
	sa.sin_port = htons(80);
	memcpy(&sa.sin_addr, p->h_addr, 4);//   with some problems ???
	//sa.sin_addr.S_un.S_addr = inet_addr(*(p->h_addr_list));
	//cout << *(p->h_addr_list) << endl;
	n = connect(sock, (sockaddr*)&sa, sizeof(sa));
	if (n == SOCKET_ERROR)
	{
		cout << "connect函数失败！ 错误码： " << WSAGetLastError() << endl;
		return;
	}
	//像服务器发送GET请求，下载图片
	string  reqInfo = "GET " +(string)othPath+ " HTTP/1.1\r\nHost: " + (string)host + "\r\nConnection:Close\r\n\r\n";
	if (SOCKET_ERROR == send(sock, reqInfo.c_str(), reqInfo.size(), 0))
	{
		cout << "send error! 错误码： " << WSAGetLastError() << endl;
		closesocket(sock);
		return;
	}
	//PutImagtoSet();
	
}

void OutIamge(string imageUrl) //将图片命名，保存在目录下
{
	int n;
	char temp[800];
	strcpy(temp, imageUrl.c_str());
	analyUrl(temp);
	preConnect();
	string photoname;
	photoname.resize(imageUrl.size());
	int k = 0;
	for (int i = 0; i<imageUrl.length(); i++){
		char ch = imageUrl[i];
		if (ch != '\\'&&ch != '/'&&ch != ':'&&ch != '*'&&ch != '?'&&ch != '"'&&ch != '<'&&ch != '>'&&ch != '|')
			photoname[k++] = ch;
	}
	photoname = "./img/"+photoname.substr(0, k) + ".jpg";

	fstream file;
	file.open(photoname, ios::out | ios::binary);
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	n = recv(sock, buf, sizeof(buf)-1, 0);
	char *cpos = strstr(buf, "\r\n\r\n");

	//allHtml = "";
	//allHtml += string(cpos);
	file.write(cpos + strlen("\r\n\r\n"), n - (cpos - buf) - strlen("\r\n\r\n"));
	while ((n = recv(sock, buf, sizeof(buf)-1, 0)) > 0)
	{
		file.write(buf, n);
		//buf[n] = '\0';
		//allHtml += string(buf);
	}
	//file.write(allHtml.c_str(), allHtml.length());
	file.close();
	//closesocket(sock);
}
void PutImagtoSet()  //解析整个html代码
{
	int n;
	//preConnect();
	char buf[1024];
	while ((n = recv(sock, buf, sizeof(buf)-1, 0)) > 0)
	{
		buf[n] = '\0';
		allHtml += string(buf);
	}
	regexGetimage(allHtml);
	regexGetcom(allHtml);
}


void bfs(string beginUrl)  //宽度优先搜索，像爬虫一样遍历网页
{
	queue<string> q;
	q.push(beginUrl);
	while (!q.empty())
	{
		string cur = q.front();
		mp[cur]++;
		q.pop();
		char tmp[800];
		strcpy(tmp, cur.c_str());
		analyUrl(tmp);
		preConnect();
		PutImagtoSet();
		vector<string>::iterator ita = photoUrl.begin();
		for (ita; ita != photoUrl.end(); ++ita)
		{
			OutIamge(*ita);
		}
		photoUrl.clear();
		vector <string>::iterator it = comUrl.begin();
		for (it; it != comUrl.end(); ++it)
		{
			if (mp[*it]==0)
			q.push(*it);
		}
		comUrl.clear();
	}
}
int main()
{
	CreateDirectoryA("./img", 0);  //在工程下创建文件夹
	string beginUrl= "http://news.qq.com/"; //输入起始网址
	bfs(beginUrl);
	return 0;
	
}
