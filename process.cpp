/*!
 * The process program simulates a running process. It sleeps until a signal is received, then handles the signal either
 * by printing out the number of the signal the the process_output file, or by writing then terminating (in case of the
 * SIGTERM signal).
 *
 * \author $Author: Ege C. Kaya $
 *
 * \date $Date: 2020/12/28 17:31:51 $
 */

#include <iostream>
#include <string>
#include <unistd.h>
#include <csignal>
#include <sstream>
#include <fstream>

using namespace std;
struct timespec delta = {0 /*secs*/, 300000000 /*nanosecs*/}; ///< A time struct to allow waiting briefly
string process_output; ///< Name of the output file this process wil write to
int processNo; ///< The number of this process
std::ofstream out; ///< The ofstream that will write to the output file
int watchdogPid; ///< The PID of the watchdog process

/*!
 * The signal handler for the process. Handles the SIGHUP, SIGINT, SIGILL, SIGTRAP, SIFGPE, SIGSEGV, SIGTERM and SIGXCPU
 * signals.
 * @param signalNo: number of the signal received
 * @param siginfo: a structure which holds valuable information about the signal received
 * @param context: in this case, an unused parameter that is nonetheless required by the POSIX API
 */
void signalHandler(int signalNo, siginfo_t* siginfo, void* context) {
    (void) signalNo;
    (void) context;

    /*!
     * Access the PID of the process that sent the signal. This is crucial to know while handling the SIGTERM signal. If
     * that signal was sent from the executor process, this process exits with the status code of 0. If, on the other
     * hand, the SIGTERM signal was sent from the watchdog process, this process exits with the status code of 1. In
     * case of any other signal but SIGTERM, the process only prints the number of the signal received and continues to
     * sleep until another signal. Also note that we need to reopen and reclose the ofstream every time we want to write
     * something to the file, in order to prevent synchronization problems in the output.
     */
    int sender_pid = siginfo->si_pid;
    if (signalNo != 15) {
        out.open(process_output, ios::app);
        cout << "P" << processNo << " received signal" << " " << signalNo << endl;
        out.close();
    }
    else {
        if (sender_pid == watchdogPid) {
            out.open(process_output, ios::app);
            cout << "P" << processNo << " received signal 15, terminating gracefully" << endl;
            out.close();
            exit(1);
        }
        else {
            out.open(process_output, ios::app);
            cout << "P" << processNo << " received signal 15, terminating gracefully" << endl;
            out.close();
            exit(0);
        }
    }
}

/*!
 * The driver function.
 * @param argc: unused, number of command line arguments passed in
 * @param argv: an array of command line arguments passed in
 * @return
 */
int main(int argc, char *argv[]) {

    /// Get the command line arguments
    process_output = argv[1];
    processNo = atoi(argv[2]);
    watchdogPid = atoi(argv[3]);

    /// A structure to deal with signal handling
    struct sigaction sa;

    /*!
     * This flag must be set in order to invoke the signal handling function with three arguments instead of one,
     * allowing the programmer to access the siginfo_t construct, seeing detailed information about the received signal.
     */
    sa.sa_flags = SA_SIGINFO;

    /// Designates the signalHandler function as the signal handler
    sa.sa_sigaction = signalHandler;

    /*!
     * These must be called in order to redirect the signals to the signalHandler function. (8 signals in total)
     */
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGTRAP, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGXCPU, &sa, NULL);

    /*!
     * Opens the process_output file with the ofstream defined earlier. Note that we need to include the ios::app
     * parameter in order to append to the end of the output file instead of overwriting its content, since multiple
     * processes will be writing to this file simultaneously.
     */
    out.open(process_output, ios::app);
    std::cout.rdbuf(out.rdbuf());

    /// Sleeps until a signal is received.
    cout << "P" << processNo << " is waiting for a signal" << endl;

    /// Closes the ofstream to prevent synchronization problems in the output.
    out.close();
    while(true)
        nanosleep(&delta, &delta);
}

