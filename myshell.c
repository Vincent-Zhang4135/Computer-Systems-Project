#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_LIMIT 515
#define FILE_SIZE_LIMIT 1024

/*------------------------------------------------------------------
 *---------------------BASIC PRINTING FUNCTIONS---------------------
 *------------------------------------------------------------------*/

/* Default Error Message that will be printed out whenever there is an error
 */
void myErrorMsg()
{
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

/* Our print function using write() system call 
 */
void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

/*------------------------------------------------------------------
 *------HELPER FUNCTION FOR INSERTING SPACE ON A GIVEN INDEX--------
 *------------------------------------------------------------------*/

/*  spliceSubstring takes a string, and splices it from the start pos
 *  inclusive, and ends it at the end pos exclusive
 */
char* spliceSubstring(char* string, int start_pos, int end_pos)
{
    int len = end_pos - start_pos;
    char* rtn = (char*)malloc(sizeof(char) * (len + 1));
    if (rtn == NULL) {
        fprintf(stderr, "malloc failed");
    }

    for (int i = 0; i < len; i++) {
        rtn[i] = string[start_pos + i];
    }

    rtn[len] = '\0';
    return rtn;
}

/*  insertSpace takes a string, and inserts a " " at the given pos
 *  and then returns the new string with the inserted space
 */
char* insertSpace(char* string, int pos)
{
    char* space = (char*)malloc(sizeof(char) * 2);
    space[0] = ' ';
    space[1] = '\0';
    int len = strlen(string);

    char* left = spliceSubstring(string, 0, pos);
    char* right = spliceSubstring(string, pos, len);

    char* rtn = (char*)malloc(sizeof(char) * (len + 1));
    strcpy(rtn, left);
    strcat(rtn, space);
    strcat(rtn, right);

    free(left);
    free(right);
    free(space);

    return rtn;
}

/*------------------------------------------------------------------
 *------------------------------FREE--------------------------------
 *------------------------------------------------------------------*/

void freeCommands(int len, char** cmds)
{
    for (int i = 0; i < len; i++) {
        free(cmds[i]);
    }

    free(cmds);
}

void freeArgvs(int arg_len, char** argvs)
{
    for (int i = 0; i < arg_len - 1; i++) {
        free(argvs);
    }

    free(argvs);
}

/*------------------------------------------------------------------
 *---------------------COMMAND PARSING FUNCTIONS--------------------
 *----------------------------------------------------------------*/

/* the fgets() takes in a buffer of 515 characters, while
 * we will take in a the first 514 charachers. If the two 
 * characters are different, that means that the command line 
 * exceeded 512 + 2 = 514 characters. Returns 1 if is too long.
 */ 
int tooLongCommandHandler(char *pinput)
{
    char cmp_input[514];
    // copy over the pinput
    for (int i = 0; i < 512; i++) {
        cmp_input[i] = pinput[i];
    }

    // add the newline and null_terminator
    cmp_input[512] = '\n';
    cmp_input[513] = '\0';

    if (strcmp(cmp_input, pinput)) {
        myPrint(cmp_input);
        myErrorMsg();
        return 1;
    }

    return 0;
}

/* testing function to print out the arguments that are formed by parsing
 * the command line string, seperated by whitespace.
 */
void printArgs(int argc, char** argvs)
{
    // printf("this is an command argvs array:\n");
    // printf("the array is %d strs long\n", argc);
    for (int i = 0; i < argc; i++) {
        if (argvs[i] == NULL) {
            printf("%d: NULL\n", i);
        } else {
            printf("%d: %s\n", i, argvs[i]);
        }
    }
}

/* given the user input in the command line, return the number of
 * commands they have provided
 */
int numCommands(char *pinput)
{
    int count = 0;
    char* save_ptr = NULL;
    char* delim = ";\n";
    // use strdup to preserve the original string
    char* pinput_cpy = strdup(pinput);
    char* curr = strtok_r(pinput_cpy, delim, &save_ptr);
    while (curr != NULL) {
        count++;
        curr = strtok_r(NULL, delim, &save_ptr);
    }

    return count;
}

/* given the user input in the command line, return an array
 * of the commands the users have provided
 */
char** splitCommands(char *pinput)
{
    int num_cmds = numCommands(pinput);
    char** commands = (char **)malloc(sizeof(char*) * num_cmds);
    if (commands == NULL) {
        fprintf(stderr, "malloc failed");
    }

    // use strtok to break the whole string into commands_str split by ";"
    int curr_cmd_i = 0;
    char* save_ptr = NULL;
    char* delim = ";\n";
    // use strdup to preserve the original string
    char* pinput_cpy = strdup(pinput);
    char* cmd = strtok_r(pinput_cpy, delim, &save_ptr);
    while (cmd != NULL) {
        char* cmd_cpy = strdup(cmd);
        commands[curr_cmd_i] = cmd_cpy;
        curr_cmd_i++;

        cmd = strtok_r(NULL, delim, &save_ptr);
    }

    return commands;
}

/* For every command string we have, such as "ls -la /tmp> output.txt",
 * "cat>+hello.txt", we need to reformat these strings so that there is a
 * space between each arg and the redirection notation. This will allow us
 * to more easily parse the command into its relevant arguments.
 */
char* reformatCommand(char *command_str)
{
    int i = 0;
    int len = strlen(command_str);

    if (len <= 1) {
        return command_str;
    }

    while (i < len) {
        if (command_str[i] == '>') {
            if (i == 0) {
                if ((i + 1) < len) {
                    i++;
                    if (command_str[i] == '+') {
                        if ((i + 1) < len) {
                            i++;
                            command_str = insertSpace(command_str, i);
                            len++;
                        } else {
                            i++;
                            continue;
                        }
                    } else {
                        if ((i + 1) < len) {
                            command_str = insertSpace(command_str, i);
                            len++;
                            i++;
                        } else {
                            i++;
                            continue;
                        }
                    }
                } else {
                    i++;
                }
                continue;
            }

            // check to see that there is possibly something to the right
            if ((i + 1) < len) {
                i++;
                // check to see if it is advanced redirection
                if (command_str[i] == '+') {
                    // need to check again that there is something to the right
                    if ((i + 1) < len) {
                        i++;
                        command_str = insertSpace(command_str, i);
                        command_str = insertSpace(command_str, i - 2);
                        len += 2;
                        i += 2;
                    } else {
                        command_str = insertSpace(command_str, i - 1);
                        len++;
                        i++;
                    }
                } else { // if it is not a +, then it is normal redirection
                    command_str = insertSpace(command_str, i);
                    command_str = insertSpace(command_str, i - 1);
                    len += 2;
                    i += 2;
                }
            } else {
                command_str = insertSpace(command_str, i);
                len++;
                i += 2;
            }
        } else {
            i++;
            continue;
        }
    }

    return command_str;
}

/* given the single command line string, determine how many arguments
 * the command line string will be breaken up into so we can malloc
 */
int detCmdArgSize(char *command_str)
{
    int count = 0;

    char* save_ptr = NULL;
    char* delim = " \n";
    // use strdup to preserve the original string
    char* command_str_cpy = strdup(command_str);
    char* curr_arg = strtok_r(command_str_cpy, delim, &save_ptr);
    while (curr_arg != NULL) {
        count++;
        curr_arg = strtok_r(NULL, delim, &save_ptr);
    }

    return (count + 1);
}

/* given a cmd input from the user, parse the command into
 * its relevants parts, such as the program name, and the arguments
 */
char** parseCommand(char *command_str)
{
    int size = detCmdArgSize(command_str);
    char** rtn = (char **)malloc(sizeof(char*) * size);
    if (rtn == NULL) {
        fprintf(stderr, "malloc failed");
    }

    char* save_ptr = NULL;
    char* delim = " \t\0";
    // use strdup to preserve the original string
    char* command_str_cpy = strdup(command_str);
    char* curr_arg = strtok_r(command_str_cpy, delim, &save_ptr);

    int i = 0;
    while (curr_arg != NULL) {
        char* arg_cpy = strdup(curr_arg);
        rtn[i] = arg_cpy;
        curr_arg = strtok_r(NULL, delim, &save_ptr);
        i++;
    }

    rtn[i] = NULL;
    return rtn;
}

/*------------------------------------------------------------------
 *-----------------------SHELL COMMAND FUNCTIONS--------------------
 *----------------------------------------------------------------*/

/*  exit command
 */ 
void exitCmd()
{
    exit(0);
}

/*  pwd command
 */ 
void pwdCmd()
{
    char cwd[FILE_SIZE_LIMIT];
    if (getcwd(cwd, FILE_SIZE_LIMIT) != NULL) {
        myPrint(cwd);
        myPrint("\n");
    } else {
        myErrorMsg();
    }
}

/*  cd command (have to check for additional args)
 */ 
void cdCmd(int arg_len, char** cmd_argvs)
{
    char *path;
    if (arg_len < 3) {
        path = getenv("HOME");
    } else if (arg_len == 3) {
        path = cmd_argvs[1];
    } else {
        myErrorMsg();
    }

    int is_path_invalid = chdir(path);
    if (is_path_invalid) {
        myErrorMsg();
    }
}

/*  executeCmd() will take care of any other commands that
 *  are not built in (which are exit, pwd, and cd)
 */ 
int executeCmd(char** cmd_argvs)
{
    int status;
    int ret = fork();
    if (ret == 0) {   // the child process
        //printf("pid of child is %d", getpid());
        return execvp(cmd_argvs[0], cmd_argvs);
    } else {          // the parent process]
        //printf("pid of parent is %d", getpid());
        waitpid(ret, &status, 0);
        return 0;
    }
}

/*------------------------------------------------------------------
 *-------------------------REDIRECTION FUNCTIONS--------------------
 *----------------------------------------------------------------*/

/* checkRedirection() checks to see i the argvs inputted in the command
 * includes a > or >+ as the 2nd to last arg, meaning it is a valid redirection
 */
int checkRedirection(int arg_len, char** cmd_argvs)
{
    
    if (arg_len < 3) {
        return 0;
    } 

    // if the second to last argument is > or >+, we have redirection
    char* potential_redir = cmd_argvs[arg_len - 3];
    if ((!strcmp(potential_redir, ">")) || (!strcmp(potential_redir, ">+"))) {
        return 1;
    }

    return 0;
}

int createWrite(char* file, int fd, int arg_len, char** cmd_argvs)
{
    int ret = -1;
    int status;
    int rv = fork();
    if (rv == 0) {  // child process
        fd = creat(file, S_IRWXU);
        dup2(fd, STDOUT_FILENO);
        execvp(cmd_argvs[0], cmd_argvs);
        ret = close(fd);
    } else {         // parent process
        waitpid(-1, &status, 0);
    }

    return ret;
}

// O_RDONLY, O_WRONLY, or O_RDWR
/* redirect() is the implementation of a normal redirect 
 */
int redirect(int arg_len, char** cmd_argvs)
{
    char* file = cmd_argvs[arg_len - 2];
    // make the > become the NULL to trick the execvp to 
    // think that it hits the NULL terminator there.
    cmd_argvs[arg_len - 3] = NULL;

    int exists = access(file, R_OK);
    int ret = -1;
    int fd = -1;

    // if the file does not exist in the cwd, we want to create it and write
    if (exists < 0) {
        ret = createWrite(file, fd, arg_len, cmd_argvs);
    } else {  // the file already exists in the cwd
        myErrorMsg();
        return ret;
    }

    return ret;
}

/* advancedRedirect() is the implementation of the advanced redirect 
 */
int advancedRedirect(int arg_len, char** cmd_argvs)
{
    char* file = cmd_argvs[arg_len - 2];
    // make the > become the NULL to trick the execvp to 
    // think that it hits the NULL terminator there.
    cmd_argvs[arg_len - 3] = NULL;

    int exists = access(file, R_OK);
    int ret = -1;
    int fd = -1;

    if (exists < 0) {
        ret = createWrite(file, fd, arg_len, cmd_argvs);
    } else {  // the file is in the cwd, and we want to add to that file
        int rv = fork();
        if (rv == 0) {    // child process
        } else {          // parent process
        }
    }

    return ret;
}

/* executeRedirection() is called if we see that the command is a redirection
 * it will check to see if its an advanced redirection or normal redirect, and
 * execute those redirections
 */
int executeRedirection(int arg_len, char** cmd_argvs)
{
    char* redir = cmd_argvs[arg_len - 3];
    if (!strcmp(redir, ">")) {
        return redirect(arg_len, cmd_argvs);
    } else if (!strcmp(redir, ">+")) {
        return advancedRedirect(arg_len, cmd_argvs);
    } else {
        myErrorMsg();
    }

    return 0;
}

/*------------------------------------------------------------------
 *--------------------------BATCH FUNCTIONS-------------------------
 *----------------------------------------------------------------*/

int checkBatch(int argc, char* argv[])
{
    // if there are too many inputs
    if (argc > 2) {
        myErrorMsg();
        exit(0);
        return 0;
    }

    // if there are no inputs (just the "./myshell")
    if (argc == 1) {
        return 0;
    }

    // else:
    return 1;
}

FILE* openBatch(int argc, char*argv[])
{
    // open the batchfile, which is the second argument
    FILE *batchfile = fopen(argv[1], "r");
    if (batchfile == NULL) {
        myErrorMsg();
        exit(0);
    }

    return batchfile;
}

/*------------------------------------------------------------------
 *---------------------------MAIN FUNCTIONS-------------------------
 *----------------------------------------------------------------*/

int main(int argc, char *argv[]) 
{
    char cmd_buff[BUFFER_LIMIT];
    char *pinput;
    FILE* batchfile = NULL;

    int batch_included = checkBatch(argc, argv);
    if (batch_included) {
        batchfile = openBatch(argc, argv);
    }

    while (1) {
        if (batch_included) {
            if ((pinput = fgets(cmd_buff, BUFFER_LIMIT, batchfile)) == NULL) {
                exit(0);
            } else {
                myPrint(pinput);
            }
        } else {
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, BUFFER_LIMIT, stdin);
        }

        // check if the command line input is too long
        int too_long = tooLongCommandHandler(pinput);
        if (too_long) {
            continue;
        }

        // split the commands (via ;) into an array of commands
        int num_commands = numCommands(pinput);
        char** commands = splitCommands(pinput);

        // iterate over the commands
        for (int i = 0; i < num_commands; i++) {
            // parse each commands
            
            int arg_len = detCmdArgSize(commands[i]);
            char* reformatted_cmd = reformatCommand(commands[i]);
            char** cmd_argvs = parseCommand(reformatted_cmd);

// printArgs(arg_len, cmd_argvs);

            // if a command is invalid, then it will have its first arg be NULL
            // which will also be how I will be testing to see which commands v
            if (cmd_argvs[0] == NULL) {
                continue;
            }

            // check for redirection
            int is_redir = checkRedirection(arg_len, cmd_argvs);
            if (is_redir) {
                executeRedirection(arg_len, cmd_argvs);
                continue;
            } else {
                if (!(strcmp(cmd_argvs[0], "exit"))) {
                    exitCmd();
                }
                else if (!(strcmp(cmd_argvs[0], "pwd"))) {
                    pwdCmd();
                }
                else if (!(strcmp(cmd_argvs[0], "cd"))) {
                    cdCmd(arg_len, cmd_argvs);
                }
                else {
                    int success = executeCmd(cmd_argvs);
                    if (success == -1) {
                        myErrorMsg();
                    }
                }
            }
            // free(argv);
        }
        // free(commands);
    }
}