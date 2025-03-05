/**
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

enum side {
  L = 0,
  R = 1,
};


/**
 * error is null unless something is wrong, then it is some explanation string
 */
struct args {
  int curDepth;
  int maxDepth;
  enum side lr;

  char *error;
};

struct args args_err(char* message) {
  struct args r = {
    -1, -1, -1,
    strdup(message)
  };
  return r;
}



struct args parseArgs(int argc, char *argv[]) {
  if (argc != 4) return args_err("Wrong arguments.");
  
  struct args args = {
    atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
    NULL
  };

  // validation
  if (args.curDepth > args.maxDepth) return args_err("curDepth must be less than or equal to ma");
  if (args.lr != L && args.lr != R) return args_err("lr must be one of 0, 1");

  return args;
}


/**
 * pid: pid of child
 * tx:  file descriptor of pipe end this process can write to
 * rx   file descriptor of pipe end this process should read from
 */
struct PipedChild {
  // pid of the child
  int pid;
  // file descriptor to write to in order to send this child data
  int tx;
  // file descriptor to read in order to get data from this child
  int rx;
};

/**
 * this function: creates a pipe, sets parent stdout to pipe write, forks, returns id to parent. child: sets stdin to pipe read, uses execvp to become program
 * @param argv from exec manpage: The char *const argv[] argument is an array
 *             of pointers to nulintl-terminated strings that represent the
 *             argument list available to the new program. The first argument,
 *             by convention, should point to the filename associated with the
 *             file being executed. The array of pointers must be terminated by
 *             a null pointer.
 */
 struct PipedChild createPipedChild(char *program, char *argv[]) {
  // create two pipes, one to send, one to recieve 
  // (parent) [send (tx), recieve (rx)]
  // inside: [ read, write ]
  int pipes[2][2];
  if (pipe(pipes[0]) == -1) exit(1);
  if (pipe(pipes[1]) == -1) exit(1);

  int pid = fork();
  
  // some kinda error
  if (pid == -1) exit(1);

  if (pid) {
    // is parent
    // close child ends of pipes
    close(pipes[0][0]); /// send pipe, close read end
    close(pipes[1][1]); // recieve pipe, close write end
    struct PipedChild r = {
      pid, pipes[0][1], pipes[1][0]
    };
    return r;
    
  } else { // is child
    // close parent ends of pipes
    close(pipes[0][1]);
    close(pipes[1][0]);

    dup2(pipes[0][0], 0); // use pipe as stdin
    dup2(pipes[1][1], 1); // use pipe as stdout

    // go into whatever program
    execvp(program, argv);

    // exec only returns if problem.
    exit(1);
  }

}



/**
 * @brief wraps createPipedChild
 * 
 * executes a right/left
 * by passing in the required numbers
 */
int doLR(char *program, int num1, int num2) {

  char* argv[] = {program, NULL}; // argv must be null terminated
  struct PipedChild child = createPipedChild(program, argv);
  // // from assignment spec, strings through pipe are max length 10
  // // when writing to pipe, string should be null terminated.
  // char w_buffer[11];
  // w_buffer[10] = NULL;

  // // from manpage: writes max n characters into some string
  // snprintf(w_buffer, 10, "%d", num1);
  // if (write(child.tx, w_buffer, 11) == -1) exit(1);
  
  // // same for num2
  // snprintf(w_buffer, 10, "%d", num2);
  // if (write(child.tx, w_buffer, 11) == -1) exit(1);


  // also from assignment suggestion: use dprintf?
  dprintf(child.tx, "%d\n", num1);
  dprintf(child.tx, "%d\n", num2);

  // wait for child to finish executing
  waitpid(child.pid, NULL, 0);

  // read 11 bytes because we know by spec that it is max 10
  char r_buffer[11];
  if (read(child.rx, r_buffer, 11) == -1) exit(1);
  return (atoi(r_buffer));
}

/**
 * @brief wraps createPipedChild
 * 
 * creates and runs a new node until it exits
 * returns printed nuber from the node
 */
int doNode(char* program, int num1, struct args childArgs) {
  char* argv[5] = {program};
  argv[4] = NULL; // must be null terminated.

  // argument 1 curDepth
  // maximum depth is 2 so we can safely assume depth arguments are 4 characters.
  argv[1] = malloc(sizeof(char) * (4 + 1)); // +1 for null terminator
  sprintf(argv[1], "%d", childArgs.curDepth); 

  // argument 2 maxDepth
  // maximum depth is 2 so we can safely assume depth arguments are 4 characters.
  argv[2] = malloc(sizeof(char) * (4 + 1));
  sprintf(argv[2], "%d", childArgs.maxDepth);

  // argument 3 lr
  // this is a single character of 0 or 1
  argv[3] = malloc(sizeof(char) * (1 + 1));
  if (childArgs.lr == L) {
    argv[3][0] = '0';
    argv[3][1] = '\0';
  } else if (childArgs.lr == R) {
    argv[3][0] = '1';
    argv[3][1] = '\0';
  } else exit(1); // something went wrong???

  // same as doLR, see that for info
  struct PipedChild child = createPipedChild(program, argv);
  dprintf(child.tx, "%d\n", num1);

  waitpid(child.pid, NULL, 0);

  char r_buffer[11];
  if (read(child.rx, r_buffer, 11) == -1) exit(1);
  return (atoi(r_buffer));
}


int main(int argc, char *argv[]) {
  // three arguments from command line
  struct args args = parseArgs(argc, argv);
  fprintf(stderr, "args in number: %d %d %d\n", args.curDepth, args.maxDepth, args.lr);

  // num1 from scanf
  int num1;

  // shamelessly lifted from provided pr.c
  scanf("%d", &num1);
  fprintf(stderr, "im depth=%d and got passed num1=%d\n", args.curDepth, num1);

  // spawn left child for num2 unless im leaf
  int num2 = 1;
  if (args.curDepth != args.maxDepth) {
    struct args childArgs = {
      args.curDepth + 1,
      args.maxDepth,
      L, 
      NULL
    };
    num2 = doNode(argv[0], num1, childArgs);
  }

  fprintf(stderr, "im depth=%d and read num2=%d\n", args.curDepth, num2);

  // decide what operation im doing: left or right
  if (args.lr == L) {
    num2 = doLR("./left", num1, num2);
  } else if (args.lr == R) {
    num2 = doLR("./right", num1, num2);
  } else exit(1);

  // spawn right child, get result
  if (args.curDepth != args.maxDepth) { 
    struct args childArgs = {
      args.curDepth + 1,
      args.maxDepth,
      R, 
      NULL
    };
    num2 = doNode(argv[0], num2, childArgs);
  }
  // printf result
  printf("%d\n", num2);
  // done  
  return 0;
}
