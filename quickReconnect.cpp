#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc/malloc.h>
#include <cstdlib>

#include <iostream>
#include <vector>
#include <regex>
#include <fstream>
#include <unistd.h>
#include <sstream>  
using namespace std;


struct pfctlRules{
	string rules;

};

class autoCloseStream{
	
	char output_c_str[500];

	int get_output(string &s){
		if(!x) return 0;
		while(fgets(output_c_str, 500, x) != NULL){
			s+= output_c_str;

		}
		return 1;
	}
	public:
		FILE *x;
		string s;
		vector<string> s_split;
		int is_fail;
		autoCloseStream(){};
		autoCloseStream(const char* command){
			x = popen(command, "r");
			//get_output(output);
			is_fail = get_output(s);
			split_by_line(s_split);

		}
		autoCloseStream(string command): autoCloseStream(command.c_str()){}

		~autoCloseStream(){
			pclose(x);
		}
		int split_by_line(vector<string> &lst){
			if (s.length() == 0) return 0;

			for(int i = 0, beg = i; i < s.length() ; i++){
				if (s[i] == '\n'){
					lst.push_back(s.substr(beg, i - beg + 1));
					beg = i+1;

				}
			}
			return 1;
		}
};

struct checkPoint{

	autoCloseStream x;
	int is_running_with_sudo;
	int is_enabled;
	int is_lasttime_aborted;
	checkPoint(): x("pfctl -s i  2>&1") {
		regex r( "Permission denied");
		smatch m;
		is_running_with_sudo = regex_search(x.s, m, r)? 0:1;
		if (is_running_with_sudo){
			
			r =  "Status: Enabled";
			is_enabled = regex_search(x.s, m, r) ? 1:0;
		}
	}
};


void read_current_rules(string &rules){
	autoCloseStream stream("pfctl -s rules");
	rules = stream.s;

}


int find_hearthstone_pid(string &hearthstone_pid){
	autoCloseStream stream("ps -e ");

	regex hs_regex("Hearthstone.app");
	regex PID_regex( "[0-9]+[0-9]" );
	smatch match;



	for(string s: stream.s_split){
		
		if(regex_search(s, match, hs_regex)){
			if (!regex_search(s, match, PID_regex)) printf("something went wrong...\n");
			//strcpy(ptr, match.str().c_str());
			hearthstone_pid += match.str();
			return 1;

		}
	}

	return 0;

}


int capture_hs_ip(vector<string> &ip_list){
	

	string hearthstone_pid;


	if (find_hearthstone_pid(hearthstone_pid)){
		
		
		
		//char command[100];
		string command("lsof -i tcp -a -p ");
		command += hearthstone_pid;
		//sprintf(command, "lsof -i tcp -a -p %s", hearthstone_pid);


		autoCloseStream stream(command);
		for(string s: stream.s_split){
			regex hearthstoneIP_regex( "->[^:]+" );
			smatch m;
			if (regex_search(s, m, hearthstoneIP_regex))ip_list.push_back(m.str().substr(2));
		}
		return 1;
		

	}

	return 0;


}

void append_rules(string &rules, vector<string> &ip_list){
	ostringstream out;
	for (int i = 0; i < ip_list.size(); i++){

		//out << "block drop out quick proto tcp to " << ip_list[i] << "\n" ;
		out << "block drop out to " << ip_list[i] << "\n" ;
		out << "block in quick from " << ip_list[i] << "\n" ;
		rules += out.str();
		out.str("");
	}
}


int main(int argc, char** argv){

	checkPoint cp;

	if (!cp.is_running_with_sudo){
		printf("please run with sudo\n");
		
		return 0;
	}

	int sleep_time = 6;
	if(argc > 1) sleep_time = atoi(argv[1]);
	printf("wait for %d seconds to reconnect. \n" , sleep_time);


	vector<string> ip_list;
	if(!capture_hs_ip(ip_list)){
		printf("Hearthstone is not running. \n");
		return 0;
	}
	


	string ori_rules;
	read_current_rules(ori_rules);
	string new_rules = ori_rules;
	append_rules(new_rules, ip_list);

	string test_rules;
	append_rules(test_rules, ip_list);

	ostringstream command;
	command << "printf \"" << test_rules << "\" | pfctl -ef -  &>/dev/null";
	system(command.str().c_str());



	printf("disconnecting............\n");
	sleep(sleep_time);
	printf(".............reconnecting\n");

	/*
	command.str("");
	command << "printf \"" << ori_rules << "\" | pfctl -ef -   &>/dev/null";
	system(command.str().c_str());
	*/
	system("pfctl -f /etc/pf.conf &>/dev/null");
	if (!cp.is_enabled) system("pfctl -d");




	return 0;
}
