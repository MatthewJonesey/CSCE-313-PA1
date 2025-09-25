/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Matthew Jones
	UIN: 533005190
	Date: 9/19/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <unistd.h>
using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	//Basically these are impossible values so if they are
	//still like this after input it means there wasnt a flag for them
	int p = -1;
	double t = -1.0;
	int e = -1;
	
	string filename = "";
	int buffer = 257;
	bool c = false;

	while ((opt = getopt(argc, argv, "p:t:e:f:m:c:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				buffer = atoi (optarg);
				break;
			case 'c':
				c = true;
				break;
		}
	}
	// Giving arguments for the server: './server', '-m' '<val for -m arg>', 'NULL'
	// fork
	// In child run execvp using the server arguments
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: run the server
       
        // Build argv for exec
		if(buffer == 257) {
			char *args[2];
			args[0] = (char*)"./server" ;
			args[1] = nullptr;
			execvp(args[0], args);
		}
		else{
			string bufstr = to_string(buffer);
			char* args[4];
			vector<char> adaptSize(bufstr.begin(), bufstr.end());
			adaptSize.push_back('\0');
			args[0] = (char*)"./server";
			args[1] = (char*)"-m";
			args[2] = adaptSize.data(); 
			args[3] = nullptr;
			execvp(args[0], args);
		}
	   _exit(1);
    } 
	else if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
	
	//We have to accomadate multiple channels for this program we keep track
	//of active with activeChannel
    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel *activeChannel = &chan;
	char buf[MAX_MESSAGE]; // 256

	if(c){
		MESSAGE_TYPE request = NEWCHANNEL_MSG;
		chan.cwrite(&request, sizeof(request));
		chan.cread(buf, sizeof(buf));
		FIFORequestChannel *newChannel = new FIFORequestChannel(buf, FIFORequestChannel::CLIENT_SIDE);
		activeChannel = newChannel;
	}
    

	//Standard input for one point of data
	if(p != -1 && t != -1.0 && e != -1 && filename == "") {
		datamsg x(p, t, e);
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	//Input for the 1000 line 1 person case
	else if(p != 1 && t == -1.0 && e == -1 && filename == "") {
		//777 sets permissions for new directory in this case its everyone can access
		mkdir("recieved", 0777);
		//file we are told to write to
		ofstream out("recieved/x1.csv", ios::binary);


		for(int i = 0; i < 1000; i++) {
			//ekg 1 values
			datamsg x1(p, (i/250.0), 1);
			memcpy(buf, &x1, sizeof(datamsg));
			//Since there is a chance of different channels we can't simply use
			//crwite we need to specify it in whichever channel we are using
			activeChannel -> cwrite(buf, sizeof(datamsg)); // question
			//Address gets modified in the reply which is where we get out answer
			double reply1 = 0.0;
			activeChannel -> cread(&reply1, sizeof(double)); //answer

			//ekg2 values
			datamsg x2(p, (i/250.0), 2);
			memcpy(buf, &x2, sizeof(datamsg));
			activeChannel->cwrite(buf, sizeof(datamsg));
			double reply2 = 0.0;
			activeChannel->cread(&reply2, sizeof(double));
			out << (i/250.0) << "," << reply1 << "," << reply2 << "\n";
		}
		out.close();
	}
    

	
	// closing the channel    


    MESSAGE_TYPE m = QUIT_MSG;
	//If we made a new channel we close it
	if(activeChannel != &chan){
		activeChannel -> cwrite(&m, sizeof(MESSAGE_TYPE));
		delete activeChannel;
	}
	//also delete the control channel
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	int status;
	waitpid(pid, &status, 0); // wait for the child process to terminate

	return 0;
}
