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
		autoCloseStream(char* command){
			x = popen(command, "r");
			//get_output(output);
			is_fail = get_output(s);

		}

		~autoCloseStream(){
			pclose(x);
		}
		int split_by_line(vector<string> &lst){
			if (s.length() == 0) return 0;
			string tmp;
			auto it = lst.begin();
			for(auto x: s){
				tmp += x;
				if(x == '\n'){
					lst.push_back(tmp);
					tmp.clear();
				}
				
			}
			return 1;
		}
};

class reqCheck{

	autoCloseStream x;
	int is_running_with_sudo;
	int is_enabled;
	reqCheck(): x("pfctl -s i") {
		is_running_with_sudo = x.output_s.length();
		is_enabled = 0;
		regex tregex( "Disabled" );
		smatch match;
		
	}
};


void read_current_rules(char *ptr, size_t size){
	char temp[1000];
	autoCloseStream stream;
	stream.x = popen("pfctl -s rules", "r");

	while (fgets(temp, sizeof(1000), stream.x) != NULL) {
		strcat(ptr, temp);
		
	}

}



int is_pfctl_enabled_(){
	autoCloseStream output("pfctl -s i");
	if (output.output_s.length()){

	}
}


int find_hearthstone_pid(char *ptr){
	int count;

	regex PID_regex( "[0-9]+[0-9]" );
	cmatch match;


	char output_s[500];
	autoCloseStream stream;
	stream.x = popen("ps -e ", "r");

	while (fgets(output_s, 500, stream.x) != NULL) {
		//output_s[strlen(output_s) - 1] = ' ';
		//printf("%d\n", output_s[strlen(output_s) - 1] == '\n');
		regex hs_regex("Hearthstone.app");
		if(regex_search(output_s, match, hs_regex)){
			if (!regex_search(output_s, match, PID_regex)) printf("something went wrong...\n");;
			//printf("%s\n", match.str().c_str());
			strcpy(ptr, match.str().c_str());
			return 1;
		}
	}

	return 0;

}


int capture_hs_ip(vector<string> &ip_list){
	char hearthstone_pid[50];
	autoCloseStream stream;

	if (find_hearthstone_pid(hearthstone_pid)){
		
		char command[100];

		sprintf(command, "lsof -i tcp -a -p %s", hearthstone_pid);

		stream.x = popen(command, "r");
		char output_s[500];



		while (fgets(output_s, 500, stream.x) != NULL) {
			//output_s[strlen(output_s) - 1] = '\0';
			
			regex hearthstoneIP_regex( "->[^:]+" );
			cmatch m;
			if (regex_search(output_s, m, hearthstoneIP_regex)){

				ip_list.push_back(m.str().substr(2));

			}
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
		out.clear();
		out.str("");
	}
}


int main(int argc, char** argv){

	int sleep_time = 6;
	if(argc > 1){

		sleep_time = atoi(argv[1]);
		printf("sleep time: %d\n" , sleep_time);
	}

	autoCloseStream output("pfctl -s i");


	vector<string> ip_list;
	if(!capture_hs_ip(ip_list)){
		printf("Hearthstone is not running. \n");
		return 0;
	}
	


	string rules;
	append_rules(rules, ip_list);

	ostringstream command;
	command << "printf \"" << rules << "\" | pfctl -ef -   &>/dev/null";
	system(command.str().c_str());


	printf("disconnecting......\n");
	sleep(sleep_time);
	printf("re-connecting......\n");
	system("pfctl -f /etc/pf.conf &>/dev/null");



	return 0;
}
