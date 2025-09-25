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

	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
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
			args[1] = NULL;
			execvp(args[0], args);
		}
		else{
			string bufstr = to_string(buffer);
			char* args[4];
			args[0] = (char*)"./server";
			args[1] = (char*)"-m";
			args[2] = (char*)bufstr.c_str(); 
			args[3] = NULL;
			execvp(args[0], args);
		}
	   _exit(1);
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
		activeChannel -> cwrite(buf, sizeof(datamsg)); // question
		double reply = 0.0;
		activeChannel -> cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	//Input for the 1000 line 1 person case
	else if(p != -1 && t == -1.0 && e == -1 && filename == "") {
		//777 sets permissions for new directory in this case its everyone can access
		mkdir("received", 0777);
		//file we are told to write to
		ofstream fout("received/x1.csv", ios::binary);


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
			fout << (i/250.0) << "," << reply1 << "," << reply2 << "\n";
		}
		fout.close();
	}
    else if (filename != "") {
        // Request file size first
        filemsg fmsg(0, 0);  // offset=0, length=0  tells server we want file size
        string fname = filename;
        int len = sizeof(filemsg) + fname.size() + 1;
        vector<char> sendbuf(len);
        memcpy(sendbuf.data(), &fmsg, sizeof(filemsg));
        strcpy(sendbuf.data() + sizeof(filemsg), fname.c_str());

        activeChannel->cwrite(sendbuf.data(), len);
        int64_t file_size = 0;
        activeChannel->cread(&file_size, sizeof(int64_t));

        // Make sure recieved dir exists
        mkdir("received", 0777);
        string outpath = "received/" + fname;
        ofstream fout(outpath, ios::binary);

        // Send file in chunks of at most buffer
    	int64_t remaining = file_size;
    	int64_t offset = 0;
        while (remaining > 0) {
            int chunk = min((int64_t)buffer, remaining-offset);
            filemsg f(offset, chunk);
			//File message changes so we have to change the length we use
			int flen = sizeof(filemsg) + filename.size() + 1;
			//Using vectors to avoid dealing with memory
			vector<char> sendbuf2(flen);
            memcpy(sendbuf2.data(), &f, sizeof(filemsg));
            strcpy(sendbuf2.data() + sizeof(filemsg), fname.c_str());

            activeChannel->cwrite(sendbuf2.data(), len);

            vector<char> recvbuf(chunk);
            activeChannel->cread(recvbuf.data(), chunk);
            fout.write(recvbuf.data(), chunk);

            offset += chunk;
        }
        fout.close();
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
	int status = 0;
	waitpid(pid, &status, 0); // wait for the child process to terminate

	return 0;
}
