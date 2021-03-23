/*!
 * The watchdog program opens a named pipe, writes the created processes' process numbers and PIDs to the pipe and
 * creates new processes by fork(). It listens until one of the processes dies and restarts that process. If the process
 * P1 dies, it kills and restarts all running processes.
 *
 * \author $Author: Ege C. Kaya $
 *
 * \date $Date: 2020/12/28 16:07:51 $
 */

#include <iostream>
#include <string>
#include <unistd.h>
#include <csignal>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <map>
#include <bits/stdc++.h>

using namespace std;
struct timespec delta = {0 /*secs*/, 300000000 /*nanosecs*/}; ///< A time struct to allow waiting briefly
int numProcess; ///< Number of total processes (in addition to watchdog) that should be running at any time
map<pid_t, int> pidMap; ///< A map that maps each PID to that process's number (in order to facilitate searching)
pid_t watchdogPid; ///< PID of the watchdog process

/*!
 * The signal handler for the watchdog process. Handles the SIGTERM signal (when the watchdog process needs to be killed
 * at the end).
 * @param signalNo: number of the signal received
 */
void signalHandler(int signalNo) {
    cout << "Watchdog is terminating gracefully" << endl;
    exit(0);
}

/*!
 * The driver function.
 * @param argc: unused, number of command line arguments passed in
 * @param argv: an array of command line arguments passed in
 * @return
 */
int main(int argc, char *argv[]) {
    watchdogPid = ::getpid(); ///< Initializes the watchdog process's PID.
    signal(SIGTERM, signalHandler); ///< Makes sure that the signal handler handles the SIGTERM signal
    pid_t pid = ::getpid(); ///< The pid variable will hold different PID values throughout its lifetime.

    /// Gets the command line variables
    numProcess = stoi(argv[1]);
    char* process_output = argv[2];
    char* watchdog_output = argv[3];

    /// Opens an ofstream and redirecting its output to watchdog_output (3rd CL argument)
    ofstream out(watchdog_output);
    cout.rdbuf(out.rdbuf());

    /// Opens the write end of the pipe opened in executor.cpp
    int namedPipe;
    char* myfifo = (char*) "/tmp/myfifo";
    mkfifo(myfifo, 0644);
    char temp[30];
    namedPipe = open(myfifo, O_WRONLY);

    /// Writes the watchdog process's process number and PID to the pipe
    string str = "P0 " + to_string(pid) + "\n";
    strcpy(temp, str.c_str());
    write(namedPipe, temp, 30);

    /*!
     * Makes sure that the process_output file is clear of any content before moving on with the operations. This is an
     * issue caused by multiple processes writing to the same process_output file during execution. In order to allow
     * this, those processes APPEND to the end of the file (so as not to overwrite each other's outputs). However,this
     * leaves residue from previous executions in the output file. The following two lines clear the process_output
     * file to get rid of the residue.
     */
    ofstream clear(process_output);
    clear.clear();
    pid_t pidList[numProcess+1]; ///< An array that will hold Pi's PID at the ith index
    pidList[0] = pid;
    pidMap.insert({pid, 0});

    /*!
     * Starts up all the processes (numProcess of them) for the first time
     */
    for (int i = 1; i <= numProcess; i++) {
        pid = fork();
        if (pid == 0) { ///< If this is the child process
        // burda bir line vardı
            char* processNo = (char*) to_string(i).c_str(); ///< The # in P#
            char* watchdogNo = (char*) to_string(watchdogPid).c_str(); ///< Passing in the PID of the watchdog process
            /// Creating the argv vector for new process
            char *args[]={(char*) "./process", process_output, processNo, watchdogNo, NULL};
            /// Replaces the text segment of the newly forked child process with the content of process.cpp
            execvp(args[0],args);
        }

        /// Writes the process number and the PID of the newly forked process to the pipe
        str = "P" + to_string(i) + " " + to_string(pid) + "\n";
        strcpy(temp, str.c_str());
        write(namedPipe, temp, 30);
        nanosleep(&delta, &delta);

        /// Writes the requested message to the watchdog_output file on process startup
        cout << "P" << i << " is started and it has a pid of " << pid << endl;

        /// Updates the pidList and pidMap
        pidList[i] = pid;
        pidMap.insert({pid, i});
    }

    /*!
     * This is the sleep-and-intercept part of the watchdog process. This part runs until the watchdog process is
     * killed, waiting for a process to change state and acting accordingly.
     */
    while (true) {

        /*!
         * Waits for a child process to change state (to be terminated, for the purposes of this program). When this
         * happens, the watchdog process is notified along with the exit status of the child process that was
         * terminated. The exit status is needed in order to determine the response of the watchdog process.
         */
        int status;
        pid_t changedPid = wait(&status);
        const int exitStatus = WEXITSTATUS(status);

        /*!
         * If the terminated process was P1, then all processes must be killed and restarted. This can be detected by
         * comparing the PID of the terminated process wıth the PID of P1.
         */
        if (changedPid == pidList[1]) {
            cout << "P1 is killed, all processes must be killed" << endl;

            /*!
             * Sending the kill signal (SIGTERM) to all processes. Note that the processes killed by this process exit
             * with the exit status 1.
             */
            for (int i=2; i<=numProcess; i++){
                pid_t curpid = pidList[i];
                kill(curpid , SIGTERM);
            }

            /*!
             * Restarts all processes by forking from the watchdog process, in the same manner as the initial startup of
             * the processes.
             */
            cout << "Restarting all processes" << endl;
            for (int j = 1; j <= numProcess; j++) {
                pid = fork();
                if (pid == 0) {
                    // char* process_output = argv[2];
                    char* processNo = (char*) to_string(j).c_str();
                    char* watchdogNo = (char*) to_string(watchdogPid).c_str();
                    char *args[]={(char*) "./process", process_output, processNo, watchdogNo, NULL};
                    execvp(args[0],args);
                }

                /// Writes the process number and the PID of the newly forked process to the pipe
                str = "P" + to_string(j) + " " + to_string(pid) + "\n";
                strcpy(temp, str.c_str());
                write(namedPipe, temp, 30);

                /// Writes the requested message to the watchdog_output file on process startup
                cout << "P" << j << " is started and it has a pid of " << pid << endl;

                /// Updates the pidList and pidMap
                pidList[j] = pid;
                pidMap.insert({pid, j});
            }
        }

        /*!
         * If the terminated process was not P1, then the terminated process must be found and restarted. Note that we
         * are only concerned with the processes which terminated with an exit status of 0 (i.e., killed by the executor
         * process) because the others are already handled by the previous if condition.
         */
        else if (exitStatus == 0) {

            /// Finds the process number of the process by its PID, using the pidMap.
            int x = pidMap[changedPid];
            cout << "P" << x << " is killed" << endl;

            /*!
             * Restarts only the process that was terminated, in the same manner as before.
             */
            cout << "Restarting P" << x << endl;
            pid = fork();
            if (pid == 0) {
                // char* process_output = argv[2];
                char* processNo = (char*) to_string(x).c_str();
                char* watchdogNo = (char*) to_string(watchdogPid).c_str();
                char *args[]={(char*) "./process", process_output, processNo, watchdogNo, NULL};
                execvp(args[0],args);
            }

            /// Writes the process number and the PID of the newly forked process to the pipe
            str = "P" + to_string(x) + " " + to_string(pid) + "\n";
            strcpy(temp, str.c_str());
            write(namedPipe, temp, 30);

            /// Writes the requested message to the watchdog_output file on process startup
            cout << "P" << x << " is started and it has a pid of " << pid << endl;

            /// Updates the pidList and pidMap
            pidList[x] = pid;
            pidMap.insert({pid, x});
        }
    }
}

