#include "ArgsParser.h"
#include "Server.h"

#include <csignal>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

// running server as a daemon
static void run_as_deamon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    // TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
        close(x);
}

int main(int argc, char** argv)
{
    run_as_deamon();
    ArgsParser args_parser { argc, argv };
    try {
        Server server { args_parser.getServerParams() };
        server.Run();
    } catch (const std::runtime_error& ex) {
        std::cerr << "Exception caught: " << ex.what() << '\n';
    }

    std::cout << args_parser.getServerParams().ip << args_parser.getServerParams().port << args_parser.getServerParams().home_dir << "\n";
}